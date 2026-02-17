    S93_ATEXIT_ONCE();
    S93_STAT_INC(g_s93_pop_calls);

    Hz3TCache* tc = &t_hz3_cache;

#if HZ3_S93_OWNER_STASH_PACKET_V2
    // Step 0: consume per-sc spill (S67-2 spill array + overflow list).
    {
        uint8_t spill_n = tc->spill_count[sc];
        if (spill_n > 0) {
            int take = (spill_n < (uint8_t)want) ? (int)spill_n : want;
            uint32_t start = (uint32_t)spill_n - (uint32_t)take;
            memcpy(out + got, &tc->spill_array[sc][start], (size_t)take * sizeof(void*));
            tc->spill_count[sc] = (uint8_t)start;
            got += take;
            if (got >= want) {
                STASH_BOUNDARY_CHECK_OUT("stash_pop:s93_v2_spill_array", owner, sc, out, got);
                return got;
            }
        }

        void* ov = tc->spill_overflow[sc];
        while (ov && got < want) {
            out[got++] = ov;
            void* next_ov = hz3_obj_get_next(ov);
#if HZ3_S154_SPILL_PREFETCH && (defined(__GNUC__) || defined(__clang__))
            if (next_ov) {
                __builtin_prefetch(next_ov, 0, 3);
            }
#endif
            ov = next_ov;
        }
        tc->spill_overflow[sc] = ov;
        if (got >= want) {
            STASH_BOUNDARY_CHECK_OUT("stash_pop:s93_v2_spill_overflow", owner, sc, out, got);
            return got;
        }
    }
#else
    // Step 0: consume per-sc overflow packet list (stored in stash_spill[sc]).
    Hz3S93StashPacket* ov = (Hz3S93StashPacket*)tc->stash_spill[sc];
    while (ov && got < want) {
        Hz3S93StashPacket* next = ov->next;
#if HZ3_S93_FAILFAST
        if (ov->sc != (uint8_t)sc) {
            fprintf(stderr, "[HZ3_S93_FAILFAST] where=pop:spill_sc_mismatch owner=%u want_sc=%d pkt_sc=%u\n",
                    owner, sc, (unsigned)ov->sc);
            abort();
        }
#endif
        int need = want - got;
        int take = (ov->n < (uint16_t)need) ? (int)ov->n : need;
        memcpy(out + got, ov->ptrs, (size_t)take * sizeof(void*));
        got += take;
        ov->n = (uint16_t)(ov->n - (uint16_t)take);
        if (ov->n == 0) {
            hz3_s93_pkt_free(owner, ov);
            ov = next;
            continue;
        }
        // Partial: shift remaining pointers to front and keep packet as new overflow head.
        memmove(ov->ptrs, &ov->ptrs[take], (size_t)ov->n * sizeof(void*));
        ov->next = next;
        tc->stash_spill[sc] = ov;
        S93_STAT_INC(g_s93_pop_partial_packets);
        STASH_BOUNDARY_CHECK_OUT("stash_pop:s93_spill_partial", owner, sc, out, got);
        return got;
    }
    tc->stash_spill[sc] = ov;  // remainder or NULL
#endif  // HZ3_S93_OWNER_STASH_PACKET_V2

    // Step 1: drain stash packets.
    //
    // V2 uses S67-2 spill (array + overflow list) and aims to be mimalloc-like:
    // take the entire packet chain with one atomic_exchange (no per-packet CAS),
    // then spill leftovers to TLS.
#if HZ3_S93_OWNER_STASH_PACKET_V2
    Hz3S93StashPacket* pkts =
        (Hz3S93StashPacket*)atomic_exchange_explicit(headp, NULL, memory_order_acquire);

    uint8_t scnt = tc->spill_count[sc];
    void* ov_head = tc->spill_overflow[sc];  // prepend onto existing overflow (normally NULL here)
    tc->spill_overflow[sc] = NULL;
    int spilled = 0;

    while (pkts) {
        Hz3S93StashPacket* pkt = pkts;
        pkts = pkt->next;

#if HZ3_S93_FAILFAST
        if (pkt->sc != (uint8_t)sc || pkt->n > (uint16_t)HZ3_S93_PACKET_K) {
            fprintf(stderr,
                    "[HZ3_S93_FAILFAST] where=pop:bad_pkt owner=%u want_sc=%d pkt_sc=%u pkt_n=%u\n",
                    owner, sc, (unsigned)pkt->sc, (unsigned)pkt->n);
            abort();
        }
#endif
        S93_STAT_INC(g_s93_pop_packets_consumed);

        for (uint16_t i = 0; i < pkt->n; i++) {
            void* obj = pkt->ptrs[i];
            if (got < want) {
                out[got++] = obj;
                continue;
            }
            spilled = 1;
            if (scnt < (uint8_t)HZ3_S67_SPILL_CAP) {
                tc->spill_array[sc][scnt++] = obj;
                continue;
            }
            hz3_obj_set_next(obj, ov_head);
            ov_head = obj;
        }

        hz3_s93_pkt_free(owner, pkt);
    }

    tc->spill_count[sc] = scnt;
    if (ov_head) {
        tc->spill_overflow[sc] = ov_head;
    }
    if (spilled) {
        S93_STAT_INC(g_s93_pop_partial_packets);
    }
    STASH_BOUNDARY_CHECK_OUT("stash_pop:s93_v2_final", owner, sc, out, got);
    return got;
#else
    // V1: pop packets from stash head (one-by-one) until want is satisfied.
    while (got < want) {
        void* head = atomic_load_explicit(headp, memory_order_acquire);
        if (!head) {
            break;
        }
        Hz3S93StashPacket* pkt = (Hz3S93StashPacket*)head;
        Hz3S93StashPacket* next = pkt->next;
        if (!atomic_compare_exchange_weak_explicit(
                headp, &head, next,
                memory_order_acquire, memory_order_relaxed)) {
            continue;
        }

#if HZ3_S93_FAILFAST
        if (pkt->sc != (uint8_t)sc) {
            fprintf(stderr, "[HZ3_S93_FAILFAST] where=pop:head_sc_mismatch owner=%u want_sc=%d pkt_sc=%u\n",
                    owner, sc, (unsigned)pkt->sc);
            abort();
        }
#endif
        S93_STAT_INC(g_s93_pop_packets_consumed);

        int need = want - got;
        int take = (pkt->n < (uint16_t)need) ? (int)pkt->n : need;
        memcpy(out + got, pkt->ptrs, (size_t)take * sizeof(void*));
        got += take;
        pkt->n = (uint16_t)(pkt->n - (uint16_t)take);
        if (pkt->n == 0) {
            hz3_s93_pkt_free(owner, pkt);
            continue;
        }

        memmove(pkt->ptrs, &pkt->ptrs[take], (size_t)pkt->n * sizeof(void*));
        pkt->next = (Hz3S93StashPacket*)tc->stash_spill[sc];
        tc->stash_spill[sc] = pkt;
        S93_STAT_INC(g_s93_pop_partial_packets);
        break;
    }

    STASH_BOUNDARY_CHECK_OUT("stash_pop:s93_final", owner, sc, out, got);
    return got;
#endif  // HZ3_S93_OWNER_STASH_PACKET_V2
