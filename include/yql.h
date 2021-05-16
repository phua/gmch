#pragma once
#ifndef Y_FINANCE_H
#define Y_FINANCE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <gmodule.h>

#define Y_HOST1    "https://query1.finance.yahoo.com/"
#define Y_HOST2    "https://query2.finance.yahoo.com/"
#define Y_SPARK    Y_HOST1 "v7/finance/spark"
#define Y_QUOTE    Y_HOST1 "v7/finance/quote"
#define Y_SUMMARY  Y_HOST1 "v10/finance/quoteSummary"
#define Y_CHART    Y_HOST1 "v8/finance/chart"
#define Y_OPTIONS  Y_HOST1 "v7/finance/options"

#define YARRAY_LENGTH  64
#define YSTRING_LENGTH 32
#define YTEXT_LENGTH   128

typedef char YString[YSTRING_LENGTH];
typedef char YText[YTEXT_LENGTH];

#define YERROR_CODE        "Not Found"
#define YERROR_DESCRIPTION "No data found, symbol may be delisted"

struct YError
{
  YString response;
  YString code;
  YText   description;
};

struct YQuote
{
  double  ask;
  int64_t askSize;
  YString averageAnalystRating;
  int64_t averageDailyVolume10Day;
  int64_t averageDailyVolume3Month;
  double  bid;
  int64_t bidSize;
  double  bookValue;
  YString currency;
  YString displayName;
  int64_t dividendDate;
  int64_t earningsTimestamp;
  int64_t earningsTimestampEnd;
  int64_t earningsTimestampStart;
  double  epsCurrentYear;
  double  epsForward;
  double  epsTrailingTwelveMonths;
  bool    esgPopulated;
  YString exchange;
  int64_t exchangeDataDelayedBy;
  YString exchangeTimezoneName;
  YString exchangeTimezoneShortName;
  double  fiftyDayAverage;
  double  fiftyDayAverageChange;
  double  fiftyDayAverageChangePercent;
  double  fiftyTwoWeekHigh;
  double  fiftyTwoWeekHighChange;
  double  fiftyTwoWeekHighChangePercent;
  double  fiftyTwoWeekLow;
  double  fiftyTwoWeekLowChange;
  double  fiftyTwoWeekLowChangePercent;
  YString fiftyTwoWeekRange;
  YString financialCurrency;
  int64_t firstTradeDateMilliseconds;
  double  forwardPE;
  YString fullExchangeName;
  int64_t gmtOffSetMilliseconds;
  /* YString language; */
  YString longName;
  YString market;
  int64_t marketCap;
  YString marketState;
  double  postMarketChange;
  double  postMarketChangePercent;
  double  postMarketPrice;
  int64_t postMarketTime;
  double  preMarketChange;
  double  preMarketChangePercent;
  double  preMarketPrice;
  int64_t preMarketTime;
  double  priceEpsCurrentYear;
  int64_t priceHint;
  double  priceToBook;
  YString quoteSourceName;
  YString quoteType;
  /* YString region; */
  double  regularMarketChange;
  double  regularMarketChangePercent;
  double  regularMarketDayHigh;
  double  regularMarketDayLow;
  YString regularMarketDayRange;
  double  regularMarketOpen;
  double  regularMarketPreviousClose;
  double  regularMarketPrice;
  int64_t regularMarketTime;
  int64_t regularMarketVolume;
  int64_t sharesOutstanding;
  YString shortName;
  int64_t sourceInterval;
  YString symbol;
  bool    tradeable;
  double  trailingAnnualDividendRate;
  double  trailingAnnualDividendYield;
  double  trailingPE;
  bool    triggerable;
  double  twoHundredDayAverage;
  double  twoHundredDayAverageChange;
  double  twoHundredDayAverageChangePercent;

  /* QuoteType.CRYPTOCURRENCY */
  int64_t circulatingSupply;
  YString fromCurrency;
  YString lastMarket;
  int64_t startDate;
  YString toCurrency;
  int64_t volume24Hr;
  int64_t volumeAllCurrencies;
};

struct YQuoteSummary
{
  /* struct AssetProfile {} assetProfile; */
  YString address1, address2, city, state, zip, country;
  YString phone, website, twitter;
  int64_t fullTimeEmployees;
  YString industry;
  /* YText   longBusinessSummary; */
  YString sector;
  struct CompanyOfficer
  {
    int64_t age;
    int64_t exercisedValue;
    int64_t fiscalYear;
    YString name;
    YString title;
    int64_t totalPay;
    int64_t unexercisedValue;
    int64_t yearBorn;
  } companyOfficers[0];
  int64_t auditRisk;
  int64_t boardRisk;
  int64_t compensationAsOfEpochDate;
  int64_t compensationRisk;
  int64_t governanceEpochDate;
  int64_t overallRisk;
  int64_t shareHolderRightsRisk;

  /* struct DefaultKeyStatistics {} defaultKeyStatistics; */
  double  beta;
  double  bookValue;
  int64_t dateShortInterest;
  double  earningsQuarterlyGrowth;
  double  enterpriseToEbitda;
  double  enterpriseToRevenue;
  int64_t enterpriseValue;
  double  fiftyTwoWeekChange;
  int64_t floatShares;
  double  forwardEps;
  double  forwardPE;
  double  heldPercentInsiders;
  double  heldPercentInstitutions;
  double  impliedSharesOutstanding;
  int64_t lastDividendDate;
  double  lastDividendValue;
  int64_t lastFiscalYearEnd;
  int64_t lastSplitDate;
  YString lastSplitFactor;
  int64_t mostRecentQuarter;
  int64_t netIncomeToCommon;
  int64_t nextFiscalYearEnd;
  double  pegRatio;
  int64_t priceHint;
  double  priceToBook;
  double  profitMargins;
  double  SandP52WeekChange;
  int64_t sharesOutstanding;
  double  sharesPercentSharesOut;
  int64_t sharesShort;
  int64_t sharesShortPreviousMonthDate;
  int64_t sharesShortPriorMonth;
  double  shortPercentOfFloat;
  double  shortRatio;
  double  trailingEps;
};

struct YChart
{
  int64_t count, timestamp[YARRAY_LENGTH];

  /* struct Meta {} meta; */
  YString symbol;
  YString instrumentType;

  /* $.chart.result[0].indicators.adjclose[0] */
  double  adjclose [YARRAY_LENGTH];

  /* $.chart.result[0].indicators.quote[0] */
  double  close    [YARRAY_LENGTH];
  double  high     [YARRAY_LENGTH];
  double  low      [YARRAY_LENGTH];
  double  open     [YARRAY_LENGTH];
  int64_t volume   [YARRAY_LENGTH];
};

struct YOption
{
  double  ask;
  double  bid;
  double  change;
  YString contractSize;
  YString contractSymbol;
  YString currency;
  int64_t expiration;
  double  impliedVolatility;
  bool    inTheMoney;
  double  lastPrice;
  int64_t lastTradeDate;
  int64_t openInterest;
  double  percentChange;
  double  strike;
  int64_t volume;
};

struct YOptionChain
{
  /* int64_t expirationDates[0]; */
  /* bool    hasMiniOptions; */
  /* double  strikes[0]; */
  YString underlyingSymbol;

  /* $.optionChain.result[0].options[0] */
  int64_t expirationDate;
  bool    hasMiniOptions;
  int64_t countCalls, countPuts;
  struct YOption calls[YARRAY_LENGTH];
  struct YOption puts [YARRAY_LENGTH];
};

extern struct YError y_error;

extern GHashTable *quotes;     /*< YString -> struct Quote * */
extern GHashTable *summaries;  /*< YString -> struct QuoteSummary * */
extern GHashTable *charts;     /*< YString -> struct Chart * */
extern GHashTable *options;    /*< YString -> struct OptionChain * */

int yql_init();
int yql_open();
int yql_close();
int yql_free();

int yql_quote(const char *);
int yql_quoteSummary(const char *);
int yql_chart(const char *);
int yql_options(const char *);

#endif
