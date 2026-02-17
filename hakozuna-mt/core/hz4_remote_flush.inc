// hz4_remote_flush.inc - RemoteFlushBox (TLS rbuf → page.remote_head)
// Box Theory: 境界 API (2箇所のうち1つ)
//
// 設計:
// - TLS rbuf は固定長配列 (stale entry 問題を回避)
// - flush 時に page ごとにまとめて publish
// - n 本固定ループ (while(next) 禁止)
// - CAS retry 時に tail.next を毎回再設定 (Lane16 バグ修正)

#ifndef HZ4_REMOTE_FLUSH_INC
#define HZ4_REMOTE_FLUSH_INC

#include "hz4_tls.h"
#include "hz4_page.h"
#include "hz4_seg.h"
#include "hz4_os.h"
#include "hz4_segq.inc"
#if HZ4_REMOTE_INBOX
#include "hz4_inbox.inc"
#endif
#include "hz4_remote_page_rbuf.inc"


#include "hz4_remote_flush_core.inc"

#include "hz4_remote_free_entry.inc"

#undef HZ4_REMOTE_FLUSH_CLEAR_NEXT

#endif // HZ4_REMOTE_FLUSH_INC
