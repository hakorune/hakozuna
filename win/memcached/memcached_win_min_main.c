#if !defined(_WIN32)
#error "Windows min main is Windows-only"
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <event2/thread.h>

#define HZ_MEMCACHED_WINDOWS_MIN_MAIN 1
#include "../../private/bench-assets/windows/memcached/source/memcached-1.6.40/memcached.c"

typedef struct hz_windows_memcached_options {
    const char* listen;
    int port;
    int udp_port;
    int threads;
    int mem_mb;
    int verbose;
} hz_windows_memcached_options;

static void hz_windows_memcached_usage(void) {
    fprintf(stderr,
        "memcached-win-min [options]\n"
        "  -p <port>      TCP port (default: 11211)\n"
        "  -U <port>      UDP port (default: 0)\n"
        "  -t <threads>   Worker threads (default: 4)\n"
        "  -m <mb>        Memory limit in MB (default: 64)\n"
        "  -l <addr>      Listen address (default: 127.0.0.1)\n"
        "  -v             Increase verbosity\n"
        "  -h             Show this help\n");
}

static int hz_windows_memcached_parse_int(const char* value, int* out) {
    char* end = NULL;
    long parsed = strtol(value, &end, 10);
    if (value == NULL || *value == '\0' || end == NULL || *end != '\0') {
        return -1;
    }
    *out = (int)parsed;
    return 0;
}

static int hz_windows_memcached_parse_args(int argc, char** argv, hz_windows_memcached_options* options) {
    int i;

    options->listen = "127.0.0.1";
    options->port = 11211;
    options->udp_port = 0;
    options->threads = 4;
    options->mem_mb = 64;
    options->verbose = 0;

    for (i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            hz_windows_memcached_usage();
            return 1;
        }
        if (strcmp(arg, "-v") == 0) {
            options->verbose += 1;
            continue;
        }
        if ((strcmp(arg, "-p") == 0) || (strcmp(arg, "-U") == 0) ||
            (strcmp(arg, "-t") == 0) || (strcmp(arg, "-m") == 0) ||
            (strcmp(arg, "-l") == 0)) {
            if (i + 1 >= argc) {
                fprintf(stderr, "missing value for %s\n", arg);
                return -1;
            }
            if (strcmp(arg, "-l") == 0) {
                options->listen = argv[++i];
                continue;
            }

            {
                int value = 0;
                if (hz_windows_memcached_parse_int(argv[++i], &value) != 0) {
                    fprintf(stderr, "invalid numeric value for %s: %s\n", arg, argv[i]);
                    return -1;
                }
                if (strcmp(arg, "-p") == 0) {
                    options->port = value;
                } else if (strcmp(arg, "-U") == 0) {
                    options->udp_port = value;
                } else if (strcmp(arg, "-t") == 0) {
                    options->threads = value;
                } else if (strcmp(arg, "-m") == 0) {
                    options->mem_mb = value;
                }
            }
            continue;
        }

        fprintf(stderr, "unsupported option for Windows min main: %s\n", arg);
        return -1;
    }

    if (options->port <= 0 || options->port > 65535) {
        fprintf(stderr, "invalid TCP port: %d\n", options->port);
        return -1;
    }
    if (options->udp_port < 0 || options->udp_port > 65535) {
        fprintf(stderr, "invalid UDP port: %d\n", options->udp_port);
        return -1;
    }
    if (options->threads <= 0) {
        fprintf(stderr, "thread count must be > 0\n");
        return -1;
    }
    if (options->mem_mb <= 0) {
        fprintf(stderr, "memory limit must be > 0\n");
        return -1;
    }

    return 0;
}

static int hz_windows_memcached_boot(const hz_windows_memcached_options* options) {
    enum hashfunc_type hash_type = MURMUR3_HASH;
    uint32_t slab_sizes[MAX_NUMBER_OF_SLAB_CLASSES];
    bool use_slab_sizes = false;
    bool preallocate = false;
    bool start_lru_maintainer = true;
    bool start_lru_crawler = true;
    bool start_assoc_maint = true;
    bool reuse_mem = false;
    void* mem_base = NULL;
    void* storage = NULL;
    int retval = EXIT_SUCCESS;
    WSADATA wsa_data;
    struct _mc_meta_data* meta = (struct _mc_meta_data*)calloc(1, sizeof(struct _mc_meta_data));

    if (meta == NULL) {
        fprintf(stderr, "failed to allocate metadata\n");
        return EX_OSERR;
    }

    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        free(meta);
        return EX_OSERR;
    }

    if (evthread_use_windows_threads() != 0) {
        fprintf(stderr, "evthread_use_windows_threads failed\n");
        WSACleanup();
        free(meta);
        return EX_OSERR;
    }

    if (!sanitycheck()) {
        WSACleanup();
        free(meta);
        return EX_OSERR;
    }

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    settings_init();
    verify_default("hash_algorithm", hash_type == MURMUR3_HASH);
    setbuf(stderr, NULL);

    settings.binding_protocol = ascii_prot;
    settings.port = options->port;
    settings.udpport = options->udp_port;
    settings.maxbytes = (size_t)options->mem_mb * 1024 * 1024;
    settings.num_threads = options->threads;
    settings.inter = _strdup(options->listen);
    settings.verbose = options->verbose;
    settings.num_threads_per_udp = settings.num_threads;
    meta->slab_config = "1.25";

    if (settings.inter == NULL) {
        fprintf(stderr, "failed to duplicate listen address\n");
        retval = EX_OSERR;
        goto cleanup;
    }

    if (hash_init(hash_type) != 0) {
        fprintf(stderr, "failed to initialize hash_algorithm\n");
        retval = EX_USAGE;
        goto cleanup;
    }

#if defined(LIBEVENT_VERSION_NUMBER) && LIBEVENT_VERSION_NUMBER >= 0x02000101
    {
        struct event_config* ev_config = event_config_new();
        if (ev_config == NULL) {
            fprintf(stderr, "failed to create event config\n");
            retval = EX_OSERR;
            goto cleanup;
        }
        event_config_set_flag(ev_config, EVENT_BASE_FLAG_NOLOCK);
        main_base = event_base_new_with_config(ev_config);
        event_config_free(ev_config);
    }
#else
    main_base = event_init();
#endif
    if (main_base == NULL) {
        fprintf(stderr, "failed to create main event base\n");
        retval = EX_OSERR;
        goto cleanup;
    }

    stats_init();
    logger_init();
    logger_create();
    conn_init();
    assoc_init(settings.hashpower_init);
    slabs_init(settings.maxbytes, settings.factor, preallocate,
        use_slab_sizes ? slab_sizes : NULL, mem_base, reuse_mem);

    memcached_thread_init(settings.num_threads, storage);
    init_lru_crawler(storage);

    if (start_assoc_maint && start_assoc_maintenance_thread() == -1) {
        retval = EX_OSERR;
        goto cleanup;
    }
    if (start_lru_crawler && start_item_crawler_thread() != 0) {
        fprintf(stderr, "failed to start LRU crawler thread\n");
        retval = EX_OSERR;
        goto cleanup;
    }
    if (start_lru_maintainer && start_lru_maintainer_thread(storage) != 0) {
        fprintf(stderr, "failed to start LRU maintainer thread\n");
        retval = EX_OSERR;
        goto cleanup;
    }

    clock_handler(0, 0, 0);

    if (settings.port && server_sockets(settings.port, tcp_transport, NULL) != 0) {
        fprintf(stderr, "failed to listen on TCP port %d\n", settings.port);
        retval = EX_OSERR;
        goto cleanup;
    }

    if (settings.udpport && server_sockets(settings.udpport, udp_transport, NULL) != 0) {
        fprintf(stderr, "failed to listen on UDP port %d\n", settings.udpport);
        retval = EX_OSERR;
        goto cleanup;
    }

    usleep(1000);
    if (stats_state.curr_conns + stats_state.reserved_fds >= settings.maxconns - 1) {
        fprintf(stderr, "maxconns setting is too low\n");
        retval = EX_USAGE;
        goto cleanup;
    }

    uriencode_init();

    while (!stop_main_loop) {
        if (event_base_loop(main_base, EVLOOP_ONCE) != 0) {
            retval = EXIT_FAILURE;
            break;
        }
    }

cleanup:
    if (stop_main_loop == GRACE_STOP) {
        stop_threads();
    }
    if (settings.inter != NULL) {
        free(settings.inter);
        settings.inter = NULL;
    }
    if (main_base != NULL) {
        event_base_free(main_base);
        main_base = NULL;
    }
    WSACleanup();
    free(meta);
    return retval;
}

int main(int argc, char** argv) {
    hz_windows_memcached_options options;
    int parse_rc = hz_windows_memcached_parse_args(argc, argv, &options);
    if (parse_rc != 0) {
        return parse_rc > 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    return hz_windows_memcached_boot(&options);
}
