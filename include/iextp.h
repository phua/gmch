#pragma once
#ifndef IEXTP_H
#define IEXTP_H

#include <stdint.h>

#define MTU                  1500
#define IEXTP_HEADER_LENGTH  40
#define IEXTP_VERSION        0x01
#define IEXTP_RESERVED       0x00
#define IEXTP_TOPS           0x8003
#define IEXTP_DEEP           0x8004
#define IEXTP_CHANNEL        0x01
#define IEXTP_MESSAGE_LENGTH 128
#define SEQUENCED_MESSAGES   0x01
#define BYTESTREAM           0x02
#define SYMBOL_LENGTH        8
#define REASON_LENGTH        4
#define TOKEN_LENGTH         40

typedef uint8_t  iextp_byte_t;
typedef uint16_t iextp_short_t;
typedef uint32_t iextp_int_t;
typedef int64_t  iextp_long_t;
typedef int64_t  iextp_time_t;
typedef uint32_t iextp_event_time_t;
typedef int64_t  iextp_price_t;
typedef char     iextp_symbol_t[SYMBOL_LENGTH];
typedef char     iextp_reason_t[REASON_LENGTH];
typedef char     iextp_token_t[TOKEN_LENGTH];

#define TSSEC(t)  ((t) / 1000000000)
#define TSNSEC(t) ((t) % 1000000000)
#define PRICE(p)  ((p) / 10000.0000)

/* Outbound Segment Formats */

struct IEXTP_Header
{
  iextp_byte_t  version;
  iextp_byte_t  reserved;
  iextp_short_t protocol;
  iextp_int_t   channel;
  iextp_int_t   session;
  iextp_short_t msgblklen;
  iextp_short_t msgblkcnt;
  iextp_long_t  msgblkoff;
  iextp_long_t  msgseqnum;
  iextp_time_t  sendtime;
};
_Static_assert(sizeof(struct IEXTP_Header) == IEXTP_HEADER_LENGTH, "Unpacked struct IEXTP_Header");

typedef struct IEXTP_Header IEXTP_Heartbeat;

typedef struct IEXTP_Header IEXTP_GapFillTestResponse;

struct IEXTP_MessageBlock
{
  iextp_short_t msglen;
  iextp_byte_t  msgdat[IEXTP_MESSAGE_LENGTH];
};
_Static_assert(sizeof(struct IEXTP_MessageBlock) == 2 + IEXTP_MESSAGE_LENGTH, "Unpacked struct IEXTP_MessageBlock");

/* Administrative Message Formats */

struct IEXTP_SystemEvent
{
  iextp_byte_t msgtype;
  iextp_byte_t event;
  iextp_time_t timestamp;
} __attribute__ ((__packed__));
_Static_assert(sizeof(struct IEXTP_SystemEvent) == 10, "Unpacked struct IEXTP_SystemEvent");

struct IEXTP_SecurityDirectory
{
  iextp_byte_t   msgtype;
iextp_byte_t            : 5;
  iextp_byte_t   flag_e : 1;    /* E: Exchange Traded Product */
  iextp_byte_t   flag_w : 1;    /* W: When Issued Security */
  iextp_byte_t   flag_t : 1;    /* T: Test Security */
  iextp_time_t   timestamp;
  iextp_symbol_t symbol;
  iextp_int_t    roundlot;
  iextp_price_t  adjclose;
  iextp_byte_t   luldtier;
} __attribute__ ((__packed__));
_Static_assert(sizeof(struct IEXTP_SecurityDirectory) == 31, "Unpacked struct IEXTP_SecurityDirectory");

struct IEXTP_TradingStatus
{
  iextp_byte_t   msgtype;
  iextp_byte_t   status;
  iextp_time_t   timestamp;
  iextp_symbol_t symbol;
  iextp_reason_t reason;
} __attribute__ ((__packed__));
_Static_assert(sizeof(struct IEXTP_TradingStatus) == 22, "Unpacked struct IEXTP_TradingStatus");

struct IEXTP_OperationalHaltStatus
{
  iextp_byte_t   msgtype;
  iextp_byte_t   status;
  iextp_time_t   timestamp;
  iextp_symbol_t symbol;
} __attribute__ ((__packed__));
_Static_assert(sizeof(struct IEXTP_OperationalHaltStatus) == 18, "Unpacked struct IEXTP_OperationalHaltStatus");

struct IEXTP_ShortSalePriceTestStatus
{
  iextp_byte_t   msgtype;
  iextp_byte_t   status;
  iextp_time_t   timestamp;
  iextp_symbol_t symbol;
  iextp_byte_t   detail;
} __attribute__ ((__packed__));
_Static_assert(sizeof(struct IEXTP_ShortSalePriceTestStatus) == 19, "Unpacked struct IEXTP_ShortSalePriceTestStatus");

struct IEXTP_SecurityEvent
{
  iextp_byte_t   msgtype;
  iextp_byte_t   event;
  iextp_time_t   timestamp;
  iextp_symbol_t symbol;
} __attribute__ ((__packed__));
_Static_assert(sizeof(struct IEXTP_SecurityEvent) == 18, "Unpacked struct IEXTP_SecurityEvent");

/* Trading Message Formats */

typedef struct SaleConditionFlags
{
iextp_byte_t          : 3;
  iextp_byte_t flag_x : 1;      /* X: Single-price Cross Trade */
  iextp_byte_t flag_8 : 1;      /* 8: Trade Through Exempt (RegNMS) */
  iextp_byte_t flag_i : 1;      /* I: Odd Lot Trade */
  iextp_byte_t flag_t : 1;      /* T: Extended Hours Trade (Form T) */
  iextp_byte_t flag_f : 1;      /* F: Intermarket Sweep Order (ISO) */
} iextp_scfl_t;
_Static_assert(sizeof(struct SaleConditionFlags) == 1, "Unpacked struct SaleConditionFlags");

struct IEXTP_TradeReport
{
  iextp_byte_t   msgtype;
  iextp_scfl_t   salecond;
  iextp_time_t   timestamp;
  iextp_symbol_t symbol;
  iextp_int_t    size;
  iextp_price_t  price;
  iextp_long_t   trade_id;
} __attribute__ ((__packed__));
_Static_assert(sizeof(struct IEXTP_TradeReport) == 38, "Unpacked struct IEXTP_TradeReport");

struct IEXTP_OfficialPrice
{
  iextp_byte_t   msgtype;
  iextp_byte_t   prctype;
  iextp_time_t   timestamp;
  iextp_symbol_t symbol;
  iextp_price_t  price;
} __attribute__ ((__packed__));
_Static_assert(sizeof(struct IEXTP_OfficialPrice) == 26, "Unpacked struct IEXTP_OfficialPrice");

struct IEXTP_TradeBreak
{
  iextp_byte_t   msgtype;
  iextp_scfl_t   salecond;
  iextp_time_t   timestamp;
  iextp_symbol_t symbol;
  iextp_int_t    size;
  iextp_price_t  price;
  iextp_long_t   trade_id;
} __attribute__ ((__packed__));
_Static_assert(sizeof(struct IEXTP_TradeBreak) == 38, "Unpacked struct IEXTP_TradeBreak");

struct IEXTP_QuoteUpdate
{
  iextp_byte_t   msgtype;
iextp_byte_t            : 6;
  iextp_byte_t   flag_p : 1;    /* P: Market Session */
  iextp_byte_t   flag_a : 1;    /* A: Symbol Availability */
  iextp_time_t   timestamp;
  iextp_symbol_t symbol;
  iextp_int_t    bids;
  iextp_price_t  bid;
  iextp_price_t  ask;
  iextp_int_t    asks;
} __attribute__ ((__packed__));
_Static_assert(sizeof(struct IEXTP_QuoteUpdate) == 42, "Unpacked struct IEXTP_QuoteUpdate");

struct IEXTP_PriceLevelUpdate
{
  iextp_byte_t   msgtype;
  iextp_byte_t   event;
  iextp_time_t   timestamp;
  iextp_symbol_t symbol;
  iextp_int_t    size;
  iextp_price_t  price;
} __attribute__ ((__packed__));
_Static_assert(sizeof(struct IEXTP_PriceLevelUpdate) == 30, "Unpacked struct IEXTP_PriceLevelUpdate");

/* Auction Message Formats */

struct IEXTP_AuctionInformation
{
  iextp_byte_t       msgtype;
  iextp_byte_t       auctype;
  iextp_time_t       timestamp;
  iextp_symbol_t     symbol;
  iextp_int_t        paired;
  iextp_price_t      reference;
  iextp_price_t      indicative;
  iextp_int_t        imbalance;
  iextp_byte_t       side;
  iextp_byte_t       extensions;
  iextp_event_time_t scheduled;
  iextp_price_t      clearing;
  iextp_price_t      collar_reference;
  iextp_price_t      collar_lower;
  iextp_price_t      collar_upper;
} __attribute__ ((__packed__));
_Static_assert(sizeof(struct IEXTP_AuctionInformation) == 80, "Unpacked struct IEXTP_AuctionInformation");

/* Snapshot Message Formats */

struct IEXTP_ErrorResponse
{
  iextp_short_t msglen;
  iextp_byte_t  msgtype;
  iextp_byte_t  reason;
};
_Static_assert(sizeof(struct IEXTP_ErrorResponse) == 4, "Unpacked struct IEXTP_ErrorResponse");

struct IEXTP_SnapshotStart
{
  iextp_short_t msglen;
  iextp_byte_t  msgtype;
  iextp_long_t  snaplen;
} __attribute__ ((__packed__));
_Static_assert(sizeof(struct IEXTP_SnapshotStart) == 11, "Unpacked struct IEXTP_SnapshotStart");

struct IEXTP_SnapshotData
{
  iextp_short_t msglen;
  iextp_byte_t  msgtype;
  struct IEXTP_Header header;
  struct IEXTP_MessageBlock msgblk;
} __attribute__ ((__packed__));
_Static_assert(sizeof(struct IEXTP_SnapshotData) == 3 + IEXTP_HEADER_LENGTH + sizeof(struct IEXTP_MessageBlock), "Unpacked struct IEXTP_SnapshotData");

struct IEXTP_SnapshotEnd
{
  iextp_short_t msglen;
  iextp_byte_t  msgtype;
  iextp_long_t  snapseqnum;
} __attribute__ ((__packed__));
_Static_assert(sizeof(struct IEXTP_SnapshotEnd) == 11, "Unpacked struct IEXTP_SnapshotEnd");

/* Inbound Segment Format */

struct IEXTP_GapFillRequestHeader
{
  iextp_byte_t  version;
  iextp_byte_t  reqtype;
  iextp_short_t protocol;
  iextp_int_t   channel;
  iextp_int_t   session;
  iextp_int_t   reqrngcnt;
};
_Static_assert(sizeof(struct IEXTP_GapFillRequestHeader) == 16, "Unpacked struct IEXTP_GapFillRequestHeader");

struct IEXTP_RequestRangeBlock
{
  iextp_long_t begin;
  iextp_long_t end;
};
_Static_assert(sizeof(struct IEXTP_RequestRangeBlock) == 16, "Unpacked struct IEXTP_RequestRangeBlock");

/* Snapshot Message Format */

struct IEXTP_SnapshotRequest
{
  iextp_short_t msglen;
  iextp_byte_t  msgtype;
  iextp_token_t token;
  iextp_int_t   channel;
  iextp_int_t   session;
  iextp_long_t  minseqnum;
} __attribute__ ((__packed__));
_Static_assert(sizeof(struct IEXTP_SnapshotRequest) == 59, "Unpacked struct IEXTP_SnapshotRequest");

#endif
