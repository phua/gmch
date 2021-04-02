#include <stdio.h>

#include "../include/iextp_handler.h"

static int pendingGapFillTestRequests = 0;

static struct IEXTP_GapFillRequestHeader gapFillRequestHeader = {
  .version   = IEXTP_VERSION,
  .reqtype   = SEQUENCED_MESSAGES,
  .protocol  = IEXTP_TOPS,
  .channel   = IEXTP_CHANNEL,
  .session   = 0x00,
  .reqrngcnt = 0
};

void iextp_gapFillRequest(struct IEXTP_Header *h, iextp_long_t begin, iextp_long_t end)
{
  fprintf(stderr, "gapFillRequest(%ld, %ld)\n", begin, end);
  /* gapFillRequestHeader.version = h->version; */
  /* gapFillRequestHeader.protocol = h->protocol; */
  /* gapFillRequestHeader.channel = h->channel; */
  /* gapFillRequestHeader.session = h->session; */
  /* gapFillRequestHeader.reqrngcnt = end - begin + 1; */
  /* struct IEXTP_RequestRangeBlock r = { .begin = begin, .end = end }; */
}

struct iextp_handler *iextp_handler_append(struct iextp_handler *h, struct iextp_handler *i)
{
  if (h) {
    struct iextp_handler *j = h;
    while (j->next) {
      j = j->next;
    }
    j->next = i;
    return h;
  }
  return i;
}

void iextp_segment(unsigned char *p, uint16_t n, const struct iextp_handler *h)
{
  static iextp_long_t nextmsgseqnum = 1;

  struct IEXTP_Header *q = (struct IEXTP_Header *) p;
  iextp_short_t m = IEXTP_HEADER_LENGTH + q->msgblklen;
  if (n < m) {
    fprintf(stderr, "Incomplete IEX-TP outbound segment: %u / %u (%u)\n", n, m, m - n);
    return;
  }
  if (q->msgseqnum > nextmsgseqnum) {
    iextp_gapFillRequest(q, nextmsgseqnum, q->msgseqnum - 1);
    nextmsgseqnum = q->msgseqnum;
  }
  if (!q->msgblklen && !q->msgblkcnt) {
    if (pendingGapFillTestRequests) {
      if (h->onGapFillTestResponse) {
        h->onGapFillTestResponse(h, q);
      }
    } else if (h->onHeartbeat) {
      h->onHeartbeat(h, q);
    }
  }
  p += IEXTP_HEADER_LENGTH;
  for (size_t i = 0; i < q->msgblkcnt; i++) {
    struct IEXTP_MessageBlock *b = (struct IEXTP_MessageBlock *) p;
    iextp_message(b->msgdat[0], b->msgdat, b->msglen, h);
    p += sizeof(b->msglen) + b->msglen;
    nextmsgseqnum++;
  }
}

void iextp_message(iextp_byte_t b, const unsigned char *p, uint16_t n, const struct iextp_handler *h)
{
  const struct iextp_handler *i = h;

  switch (b) {
#define V(T, S)                                         \
    case T:                                             \
      do {                                              \
        if (i->on##S) {                                 \
          if (i->on##S(i, (struct IEXTP_##S *) p)) {    \
            break;                                      \
          }                                             \
        }                                               \
      } while ((i = i->next));                          \
      break;
    VTABLE
#undef V
  default:
    do {
      if (i->onDefault) {
        if (i->onDefault(i, p, n)) {
          break;
        }
      }
    } while ((i = i->next));
  }
}
