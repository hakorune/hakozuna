// hz4_remote_page_rbuf.inc - RemotePageRbufBox (mimalloc-style per-page remote)
// Box Theory: remote free の「箱」を inbox から分離し、ページ単位でバッチ drain する。
//
// 設計:
// - remote free: page.remote_head[] に直接 push
// - notify: per-owner global page queue (sc bucket で分離)
// - drain: collect 境界で pageq を drain → hz4_drain_page[_list] を再利用
// - stranding 防止: queued=0 の後に remote_head 再チェック（必要なら requeue）

#ifndef HZ4_REMOTE_PAGE_RBUF_INC
#define HZ4_REMOTE_PAGE_RBUF_INC

#include "hz4_tls.h"
#include "hz4_page.h"
#include "hz4_seg.h"
#include "hz4_sizeclass.h"
#include "hz4_os.h"
#if HZ4_REMOTE_INBOX
#include "hz4_inbox.inc"
#endif


#include "hz4_remote_page_rbuf_tail.inc"
#endif // HZ4_REMOTE_PAGE_RBUF_INC
