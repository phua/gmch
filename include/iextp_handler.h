#pragma once
#ifndef IEXTP_HANDLER_H
#define IEXTP_HANDLER_H

#include "iextp.h"

#define _U_ __attribute__ ((__unused__))

struct iextp_handler
{
  struct iextp_handler *next;

  void (*init)(struct iextp_handler *);
  void (*open)(struct iextp_handler *);
  void (*close)(struct iextp_handler *);
  void (*free)(struct iextp_handler *);

  int (*onHeartbeat)(const struct iextp_handler *, const IEXTP_Heartbeat *);
  int (*onGapFillTestResponse)(const struct iextp_handler *, const IEXTP_GapFillTestResponse *);
  int (*onSystemEvent)(const struct iextp_handler *, const struct IEXTP_SystemEvent *);
  int (*onSecurityDirectory)(const struct iextp_handler *, const struct IEXTP_SecurityDirectory *);
  int (*onTradingStatus)(const struct iextp_handler *, const struct IEXTP_TradingStatus *);
  int (*onOperationalHaltStatus)(const struct iextp_handler *, const struct IEXTP_OperationalHaltStatus *);
  int (*onShortSalePriceTestStatus)(const struct iextp_handler *, const struct IEXTP_ShortSalePriceTestStatus *);
  int (*onSecurityEvent)(const struct iextp_handler *, const struct IEXTP_SecurityEvent *);
  int (*onTradeReport)(const struct iextp_handler *, const struct IEXTP_TradeReport *);
  int (*onOfficialPrice)(const struct iextp_handler *, const struct IEXTP_OfficialPrice *);
  int (*onTradeBreak)(const struct iextp_handler *, const struct IEXTP_TradeBreak *);
  int (*onQuoteUpdate)(const struct iextp_handler *, const struct IEXTP_QuoteUpdate *);
  int (*onPriceLevelUpdate)(const struct iextp_handler *, const struct IEXTP_PriceLevelUpdate *);
  int (*onAuctionInformation)(const struct iextp_handler *, const struct IEXTP_AuctionInformation *);
  int (*onDefault)(const struct iextp_handler *, const iextp_byte_t *, uint16_t);
};

struct iextp_handler *iextp_handler_append(struct iextp_handler *, struct iextp_handler *);

#define VTABLE                                  \
  V('S', SystemEvent)                           \
  V('D', SecurityDirectory)                     \
  V('H', TradingStatus)                         \
  V('O', OperationalHaltStatus)                 \
  V('P', ShortSalePriceTestStatus)              \
  V('E', SecurityEvent)                         \
  V('T', TradeReport)                           \
  V('X', OfficialPrice)                         \
  V('B', TradeBreak)                            \
  V('Q', QuoteUpdate)                           \
  V('5', PriceLevelUpdate)                      \
  V('8', PriceLevelUpdate)                      \
  V('A', AuctionInformation)

#define FTABLE                                  \
  F(SecurityDirectory)                          \
    F(TradingStatus)                            \
    F(OperationalHaltStatus)                    \
    F(ShortSalePriceTestStatus)                 \
    F(SecurityEvent)                            \
    F(TradeReport)                              \
    F(OfficialPrice)                            \
    F(TradeBreak)                               \
    F(QuoteUpdate)                              \
  F(PriceLevelUpdate)                           \
  F(AuctionInformation)

void iextp_segment(unsigned char *, uint16_t, const struct iextp_handler *);

void iextp_message(iextp_byte_t, const unsigned char *, uint16_t, const struct iextp_handler *);

void iextp_gapFillRequest(struct IEXTP_Header *, iextp_long_t, iextp_long_t);

#endif
