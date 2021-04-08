#include <assert.h>
#include <limits.h>
#include <ctype.h>
#include <time.h>

#include "../include/iextp_logger.h"

void bindump(FILE *f, uint64_t b, size_t n)
{
  for (size_t i = 0; i < n; i++) {
    fputc(getbitf(b, i) ? '1' : '0', f);
    if (i % CHAR_BIT + 1 == CHAR_BIT) {
      fputs("  ", f);
    }
  }
  fputc('\n', f);
}

void hexdump(FILE *f, const unsigned char *p, size_t n)
{
  size_t i = 0, j = 0;

  for (i = 0; i < n / 16 + 1; i++) {
    fprintf(f, "%04x  ", (int) i * 16);
    for (j = i * 16; j < i * 16 + 16; j++) {
      fprintf(f, "%02x%s", j < n ? p[j] : 0, j % 16 == 15 ? " |" : j % 8 == 7 ? "  " : " ");
    }
    for (j = i * 16; j < i * 16 + 16; j++) {
      fprintf(f, "%c", j < n ? isprint(p[j]) ? p[j] : '.' : ' ');
    }
    fprintf(f, "|\n");
  }
}

char *strntime(iextp_time_t ts)
{
  static char buf[32];          /* Not thread-safe */
  time_t sec = TSSEC(ts);
  size_t off = strftime(buf, sizeof(buf), "%FT%T", gmtime(&sec));
  sprintf(buf + off, ".%09ld", TSNSEC(ts));
  return buf;
}

char *strstime(iextp_event_time_t ts)
{
  static char buf[32];          /* Not thread-safe */
  time_t sec = ts;
  strftime(buf, sizeof(buf), "%FT%T", gmtime(&sec));
  return buf;
}

static char *system_event(char c)
{
  switch (c) {
  case 'O': return "Start of messages";
  case 'S': return "Start of system hours";
  case 'R': return "Start of regular market hours";
  case 'M': return "End of regular market hours";
  case 'E': return "End of system hours";
  case 'C': return "End of messages";
  default : return "Unknown system event";
  }
}

static int onHeartbeat(const struct iextp_handler *h, const IEXTP_Heartbeat *p)
{
  struct iextp_logger *log = (struct iextp_logger *) h;
  fprintf(log->log, "%s Heartbeat\n", strntime(p->sendtime));
  return 0;
}

static int onGapFillTestResponse(const struct iextp_handler *h, const IEXTP_GapFillTestResponse *p)
{
  struct iextp_logger *log = (struct iextp_logger *) h;
  fprintf(log->log, "%s GapFillTestResponse\n", strntime(p->sendtime));
  return 0;
}

static int onSystemEvent(const struct iextp_handler *h, const struct IEXTP_SystemEvent *p)
{
  struct iextp_logger *log = (struct iextp_logger *) h;
  fprintf(log->log, "%s SystemEvent: %s\n", strntime(p->timestamp), system_event(p->event));
  return 0;
}

static int onSecurityDirectory(const struct iextp_handler *h, const struct IEXTP_SecurityDirectory *p)
{
  struct iextp_logger *log = (struct iextp_logger *) h;
  fprintf(log->log, "%s SecurityDirectory: %.8s, %08u, %016.4f, %u\n",
          strntime(p->timestamp), p->symbol, p->roundlot, PRICE(p->adjclose), p->luldtier);
  return 0;
}

static int onTradingStatus(const struct iextp_handler *h, const struct IEXTP_TradingStatus *p)
{
  struct iextp_logger *log = (struct iextp_logger *) h;
  fprintf(log->log, "%s TradingStatus: %c, %.8s, %.4s\n",
          strntime(p->timestamp), p->status, p->symbol, p->reason);
  return 0;
}

static int onOperationalHaltStatus(const struct iextp_handler *h, const struct IEXTP_OperationalHaltStatus *p)
{
  struct iextp_logger *log = (struct iextp_logger *) h;
  fprintf(log->log, "%s OperationalHaltStatus: %c, %.8s\n",
          strntime(p->timestamp), p->status, p->symbol);
  return 0;
}

static int onShortSalePriceTestStatus(const struct iextp_handler *h, const struct IEXTP_ShortSalePriceTestStatus *p)
{
  struct iextp_logger *log = (struct iextp_logger *) h;
  fprintf(log->log, "%s ShortSalePriceTestStatus: %u, %.8s, %c\n",
          strntime(p->timestamp), p->status, p->symbol, p->detail);
  return 0;
}

static int onSecurityEvent(const struct iextp_handler *h, const struct IEXTP_SecurityEvent *p)
{
  struct iextp_logger *log = (struct iextp_logger *) h;
  fprintf(log->log, "%s SecurityEvent: %c, %.8s\n",
          strntime(p->timestamp), p->event, p->symbol);
  return 0;
}

static int onTradeReport(const struct iextp_handler *h, const struct IEXTP_TradeReport *p)
{
  struct iextp_logger *log = (struct iextp_logger *) h;
  fprintf(log->log, "%s TradeReport: %.8s, %08u, %016.4f, %ld\n",
          strntime(p->timestamp), p->symbol, p->size, PRICE(p->price), p->trade_id);
  return 0;
}

static int onOfficialPrice(const struct iextp_handler *h, const struct IEXTP_OfficialPrice *p)
{
  struct iextp_logger *log = (struct iextp_logger *) h;
  fprintf(log->log, "%s OfficialPrice: %c, %.8s, %016.4f\n",
          strntime(p->timestamp), p->prctype, p->symbol, PRICE(p->price));
  return 0;
}

static int onTradeBreak(const struct iextp_handler *h, const struct IEXTP_TradeBreak *p)
{
  struct iextp_logger *log = (struct iextp_logger *) h;
  fprintf(log->log, "%s TradeBreak: %.8s, %08u, %016.4f, %ld\n",
          strntime(p->timestamp), p->symbol, p->size, PRICE(p->price), p->trade_id);
  return 0;
}

static int onQuoteUpdate(const struct iextp_handler *h, const struct IEXTP_QuoteUpdate *p)
{
  struct iextp_logger *log = (struct iextp_logger *) h;
  fprintf(log->log, "%s QuoteUpdate: %.8s, %08u, %016.4f, %016.4f, %08u\n",
          strntime(p->timestamp), p->symbol, p->bids, PRICE(p->bid), PRICE(p->ask), p->asks);
  return 0;
}

static int onPriceLevelUpdate(const struct iextp_handler *h, const struct IEXTP_PriceLevelUpdate *p)
{
  struct iextp_logger *log = (struct iextp_logger *) h;
  fprintf(log->log, "%s PriceLevelUpdate: %c, %u, %.8s, %08u, %016.4f\n",
          strntime(p->timestamp), p->msgtype, p->event, p->symbol, p->size, PRICE(p->price));
  return 0;
}

static int onAuctionInformation(const struct iextp_handler *h, const struct IEXTP_AuctionInformation *p)
{
  struct iextp_logger *log = (struct iextp_logger *) h;
  fprintf(log->log, "%s AuctionInformation: %c, %.8s, %08u, %016.4f, %016.4f, %08u, %c, %u, %s, %016.4f, %016.4f, %016.4f, %016.4f\n",
          strntime(p->timestamp), p->auctype, p->symbol, p->paired, PRICE(p->reference), PRICE(p->indicative),
          p->imbalance, p->side, p->extensions, strstime(p->scheduled), PRICE(p->clearing),
          PRICE(p->collar_reference), PRICE(p->collar_lower), PRICE(p->collar_upper));
  return 0;
}

static int onDefault(const struct iextp_handler *h, const iextp_byte_t *p, uint16_t n)
{
  struct iextp_logger *log = (struct iextp_logger *) h;
  hexdump(log->log, p, n);
  return 0;
}

int iextp_logger_init(struct iextp_logger *log)
{
  log->onHeartbeat = onHeartbeat;
  log->onGapFillTestResponse = onGapFillTestResponse;
#define V(T, U)                                 \
  log->on##U = on##U;
  VTABLE
#undef V
  log->onDefault = onDefault;
  return 0;
}

int iextp_logger_open(struct iextp_logger *log)
{
  assert(log->logpath);

  if (!log->log) {
    log->log = fopen(log->logpath, "w");
    if (!log->log) {
      perror("fopen");
      return -1;
    }
    return 0;
  }
  return 1;
}

int iextp_logger_close(struct iextp_logger *log)
{
  if (log->log) {
    if (fclose(log->log)) {
      perror("fclose");
      log->log = NULL;
      return -1;
    }
    log->log = NULL;
    return 0;
  }
  return 1;
}

int iextp_logger_free(struct iextp_logger *log _U_)
{
  return 0;
}
