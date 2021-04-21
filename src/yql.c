#define _GNU_SOURCE             /* strdupa, vasprintf */

#include <assert.h>
#include <curl/curl.h>
#include <json-glib/json-glib.h>

#include "../include/yql.h"

#define _U_ __attribute__ ((__unused__))

struct JSONBuffer
{
  char  *bytes;
  size_t size;
};

GHashTable *quotes;
GHashTable *summaries;
GHashTable *charts;
GHashTable *options;

static CURL *easy;
static JsonPath *path;

int min(int x, int y)
{
  return x <= y ? x : y;
}

int max(int x, int y)
{
  return x >= y ? x : y;
}

static void *ght_get(GHashTable *t, const char *k, size_t n)
{
  void *v = g_hash_table_lookup(t, k);
  if (!v) {
    if ((k = strndup(k, YSTRING_LENGTH))) {
      if ((v = calloc(1, n))) {
        g_hash_table_insert(t, (char *) k, v);
      } else {
        perror("calloc");
        free((char *) k);
      }
    } else {
      perror("strndup");
    }
  }
  return v;
}

static void json_bool(JsonReader *r, const char *n, void *v)
{
  if (json_reader_is_value(r)) {
    *((bool *) v) = !json_reader_get_null_value(r) ? json_reader_get_boolean_value(r) : false;
    return;
  } else if (json_reader_read_member(r, n)) {
    json_bool(r, "raw", v);
  } else {
    fprintf(stderr, "json_reader_read_member(%s)\n", n);
    *((bool *) v) = false;
  }
  json_reader_end_member(r);
}

static void json_int(JsonReader *r, const char *n, void *v)
{
  if (json_reader_is_value(r)) {
    *((int64_t *) v) = !json_reader_get_null_value(r) ? json_reader_get_int_value(r) : 0L;
    return;
  } else if (json_reader_read_member(r, n)) {
    json_int(r, "raw", v);
  } else {
    fprintf(stderr, "json_reader_read_member(%s)\n", n);
    *((int64_t *) v) = 0L;
  }
  json_reader_end_member(r);
}

static void json_double(JsonReader *r, const char *n, void *v)
{
  if (json_reader_is_value(r)) {
    *((double *) v) = !json_reader_get_null_value(r) ? json_reader_get_double_value(r) : 0.0d;
    return;
  } else if (json_reader_read_member(r, n)) {
    json_double(r, "raw", v);
  } else {
    fprintf(stderr, "json_reader_read_member(%s)\n", n);
    *((double *) v) = 0.0d;
  }
  json_reader_end_member(r);
}

static size_t json_string(JsonReader *r, const char *n, char *v)
{
  size_t length = 0;
  if (json_reader_is_value(r)) {
    const char *s = !json_reader_get_null_value(r) ? json_reader_get_string_value(r) : "\0";
    strncpy(v, s, YSTRING_LENGTH);
    v[YSTRING_LENGTH - 1] = 0;
    return strlen(v);
  } else if (json_reader_read_member(r, n)) {
    if ((length = json_string(r, "fmt", v)) == 0) {
      length = json_string(r, "longFmt", v);
    }
  } else {
    fprintf(stderr, "json_reader_read_member(%s)\n", n);
    strncpy(v, "\0", YSTRING_LENGTH);
  }
  json_reader_end_member(r);
  return length;
}

static int json_array(JsonReader *r, const char *n, void *v, int m,
                      void (*get)(JsonReader *, const char *, void *))
{
#define T int64_t
  int i = 0, j = 0, k = 0;
  if (json_reader_read_member(r, n) && json_reader_is_array(r)) {
    k = json_reader_count_elements(r), j = k > m ? k - m : 0;
    for (i = j; i < k; i++) {
      if (json_reader_read_element(r, i)) {
        get(r, n, (T *) v + (i - j));
      } else {
        fprintf(stderr, "json_reader_read_member(%s[%d])\n", n, i);
      }
      json_reader_end_element(r);
    }
  } else {
    fprintf(stderr, "json_reader_read_member(%s)\n", n);
    memset(v, 0, sizeof(T) * m);
  }
  json_reader_end_member(r);
  return i - j;
#undef T
}

static int json_int_array(JsonReader *r, const char *n, int64_t v[], int m)
{
  return json_array(r, n, v, m, json_int);
}

static int json_double_array(JsonReader *r, const char *n, double v[], int m)
{
  return json_array(r, n, v, m, json_double);
}

static struct YQuote *json_quote(JsonReader *r, const char *s)
{
  assert(json_reader_is_object(r));
  struct YQuote *q = ght_get(quotes, s, sizeof(struct YQuote));

  json_double (r, "ask", &q->ask);
  json_int    (r, "askSize", &q->askSize);
  json_string (r, "averageAnalystRating", q->averageAnalystRating);
  json_int    (r, "averageDailyVolume10Day", &q->averageDailyVolume10Day);
  json_int    (r, "averageDailyVolume3Month", &q->averageDailyVolume3Month);
  json_double (r, "bid", &q->bid);
  json_int    (r, "bidSize", &q->bidSize);
  json_double (r, "bookValue", &q->bookValue);
  json_string (r, "currency", q->currency);
  json_string (r, "displayName", q->displayName);
  json_int    (r, "dividendDate", &q->dividendDate);
  json_int    (r, "earningsTimestamp", &q->earningsTimestamp);
  json_int    (r, "earningsTimestampEnd", &q->earningsTimestampEnd);
  json_int    (r, "earningsTimestampStart", &q->earningsTimestampStart);
  json_double (r, "epsCurrentYear", &q->epsCurrentYear);
  json_double (r, "epsForward", &q->epsForward);
  json_double (r, "epsTrailingTwelveMonths", &q->epsTrailingTwelveMonths);
  json_bool   (r, "esgPopulated", &q->esgPopulated);
  json_string (r, "exchange", q->exchange);
  json_int    (r, "exchangeDataDelayedBy", &q->exchangeDataDelayedBy);
  json_string (r, "exchangeTimezoneName", q->exchangeTimezoneName);
  json_string (r, "exchangeTimezoneShortName", q->exchangeTimezoneShortName);
  json_double (r, "fiftyDayAverage", &q->fiftyDayAverage);
  json_double (r, "fiftyDayAverageChange", &q->fiftyDayAverageChange);
  json_double (r, "fiftyDayAverageChangePercent", &q->fiftyDayAverageChangePercent);
  json_double (r, "fiftyTwoWeekHigh", &q->fiftyTwoWeekHigh);
  json_double (r, "fiftyTwoWeekHighChange", &q->fiftyTwoWeekHighChange);
  json_double (r, "fiftyTwoWeekHighChangePercent", &q->fiftyTwoWeekHighChangePercent);
  json_double (r, "fiftyTwoWeekLow", &q->fiftyTwoWeekLow);
  json_double (r, "fiftyTwoWeekLowChange", &q->fiftyTwoWeekLowChange);
  json_double (r, "fiftyTwoWeekLowChangePercent", &q->fiftyTwoWeekLowChangePercent);
  json_string (r, "fiftyTwoWeekRange", q->fiftyTwoWeekRange);
  json_string (r, "financialCurrency", q->financialCurrency);
  json_int    (r, "firstTradeDateMilliseconds", &q->firstTradeDateMilliseconds);
  json_double (r, "forwardPE", &q->forwardPE);
  json_string (r, "fullExchangeName", q->fullExchangeName);
  json_int    (r, "gmtOffSetMilliseconds", &q->gmtOffSetMilliseconds);
  /* json_string (r, "language", q->language); */
  json_string (r, "longName", q->longName);
  json_string (r, "market", q->market);
  json_int    (r, "marketCap", &q->marketCap);
  json_string (r, "marketState", q->marketState);
  json_double (r, "postMarketChange", &q->postMarketChange);
  json_double (r, "postMarketChangePercent", &q->postMarketChangePercent);
  json_double (r, "postMarketPrice", &q->postMarketPrice);
  json_int    (r, "postMarketTime", &q->postMarketTime);
  json_double (r, "preMarketChange", &q->preMarketChange);
  json_double (r, "preMarketChangePercent", &q->preMarketChangePercent);
  json_double (r, "preMarketPrice", &q->preMarketPrice);
  json_int    (r, "preMarketTime", &q->preMarketTime);
  json_double (r, "priceEpsCurrentYear", &q->priceEpsCurrentYear);
  json_int    (r, "priceHint", &q->priceHint);
  json_double (r, "priceToBook", &q->priceToBook);
  json_string (r, "quoteSourceName", q->quoteSourceName);
  json_string (r, "quoteType", q->quoteType);
  /* json_string (r, "region", q->region); */
  json_double (r, "regularMarketChange", &q->regularMarketChange);
  json_double (r, "regularMarketChangePercent", &q->regularMarketChangePercent);
  json_double (r, "regularMarketDayHigh", &q->regularMarketDayHigh);
  json_double (r, "regularMarketDayLow", &q->regularMarketDayLow);
  json_string (r, "regularMarketDayRange", q->regularMarketDayRange);
  json_double (r, "regularMarketOpen", &q->regularMarketOpen);
  json_double (r, "regularMarketPreviousClose", &q->regularMarketPreviousClose);
  json_double (r, "regularMarketPrice", &q->regularMarketPrice);
  json_int    (r, "regularMarketTime", &q->regularMarketTime);
  json_int    (r, "regularMarketVolume", &q->regularMarketVolume);
  json_int    (r, "sharesOutstanding", &q->sharesOutstanding);
  json_string (r, "shortName", q->shortName);
  json_int    (r, "sourceInterval", &q->sourceInterval);
  json_string (r, "symbol", q->symbol);
  json_bool   (r, "tradeable", &q->tradeable);
  json_double (r, "trailingAnnualDividendRate", &q->trailingAnnualDividendRate);
  json_double (r, "trailingAnnualDividendYield", &q->trailingAnnualDividendYield);
  json_double (r, "trailingPE", &q->trailingPE);
  json_bool   (r, "triggerable", &q->triggerable);
  json_double (r, "twoHundredDayAverage", &q->twoHundredDayAverage);
  json_double (r, "twoHundredDayAverageChange", &q->twoHundredDayAverageChange);
  json_double (r, "twoHundredDayAverageChangePercent", &q->twoHundredDayAverageChangePercent);

  json_int    (r, "circulatingSupply", &q->circulatingSupply);
  json_string (r, "fromCurrency", q->fromCurrency);
  json_string (r, "lastMarket", q->lastMarket);
  json_int    (r, "startDate", &q->startDate);
  json_string (r, "toCurrency", q->toCurrency);
  json_int    (r, "volume24Hr", &q->volume24Hr);
  json_int    (r, "volumeAllCurrencies", &q->volumeAllCurrencies);

  return q;
}

static struct YQuoteSummary *json_quoteSummary(JsonReader *r, const char *s)
{
  assert(json_reader_is_object(r));
  struct YQuoteSummary *q = ght_get(summaries, s, sizeof(struct YQuoteSummary));

  if (json_reader_read_member(r, "assetProfile")) {
    json_string (r, "address1", q->address1);
    json_string (r, "address2", q->address2);
    json_string (r, "city", q->city);
    json_string (r, "state", q->state);
    json_string (r, "zip", q->zip);
    json_string (r, "country", q->country);
    json_string (r, "phone", q->phone);
    json_string (r, "website", q->website);
    json_string (r, "twitter", q->twitter);
    json_int    (r, "fullTimeEmployees", &q->fullTimeEmployees);
    json_string (r, "industry", q->industry);
    /* json_text   (r, "longBusinessSummary", q->longBusinessSummary); */
    json_string (r, "sector", q->sector);
    json_int    (r, "auditRisk", &q->auditRisk);
    json_int    (r, "boardRisk", &q->boardRisk);
    json_int    (r, "compensationAsOfEpochDate", &q->compensationAsOfEpochDate);
    json_int    (r, "compensationRisk", &q->compensationRisk);
    json_int    (r, "governanceEpochDate", &q->governanceEpochDate);
    json_int    (r, "overallRisk", &q->overallRisk);
    json_int    (r, "shareHolderRightsRisk", &q->shareHolderRightsRisk);
  } else {
    fprintf(stderr, "json_reader_read_member(assetProfile)\n");
  }
  json_reader_end_member(r);

  if (json_reader_read_member(r, "defaultKeyStatistics")) {
    json_double (r, "beta", &q->beta);
    json_double (r, "bookValue", &q->bookValue);
    json_int    (r, "dateShortInterest", &q->dateShortInterest);
    json_double (r, "earningsQuarterlyGrowth", &q->earningsQuarterlyGrowth);
    json_double (r, "enterpriseToEbitda", &q->enterpriseToEbitda);
    json_double (r, "enterpriseToRevenue", &q->enterpriseToRevenue);
    json_int    (r, "enterpriseValue", &q->enterpriseValue);
    json_double (r, "fiftyTwoWeekChange", &q->fiftyTwoWeekChange);
    json_int    (r, "floatShares", &q->floatShares);
    json_double (r, "forwardEps", &q->forwardEps);
    json_double (r, "forwardPE", &q->forwardPE);
    json_double (r, "heldPercentInsiders", &q->heldPercentInsiders);
    json_double (r, "heldPercentInstitutions", &q->heldPercentInstitutions);
    json_double (r, "impliedSharesOutstanding", &q->impliedSharesOutstanding);
    json_int    (r, "lastDividendDate", &q->lastDividendDate);
    json_double (r, "lastDividendValue", &q->lastDividendValue);
    json_int    (r, "lastFiscalYearEnd", &q->lastFiscalYearEnd);
    json_int    (r, "lastSplitDate", &q->lastSplitDate);
    json_string (r, "lastSplitFactor", q->lastSplitFactor);
    json_int    (r, "mostRecentQuarter", &q->mostRecentQuarter);
    json_int    (r, "netIncomeToCommon", &q->netIncomeToCommon);
    json_int    (r, "nextFiscalYearEnd", &q->nextFiscalYearEnd);
    json_double (r, "pegRatio", &q->pegRatio);
    json_int    (r, "priceHint", &q->priceHint);
    json_double (r, "priceToBook", &q->priceToBook);
    json_double (r, "profitMargins", &q->profitMargins);
    json_double (r, "SandP52WeekChange", &q->SandP52WeekChange);
    json_int    (r, "sharesOutstanding", &q->sharesOutstanding);
    json_double (r, "sharesPercentSharesOut", &q->sharesPercentSharesOut);
    json_int    (r, "sharesShort", &q->sharesShort);
    json_int    (r, "sharesShortPreviousMonthDate", &q->sharesShortPreviousMonthDate);
    json_int    (r, "sharesShortPriorMonth", &q->sharesShortPriorMonth);
    json_double (r, "shortPercentOfFloat", &q->shortPercentOfFloat);
    json_double (r, "shortRatio", &q->shortRatio);
    json_double (r, "trailingEps", &q->trailingEps);
  } else {
    fprintf(stderr, "json_reader_read_member(defaultKeyStatistics)\n");
  }
  json_reader_end_member(r);

  return q;
}

static struct YChart *json_chart(JsonReader *r, const char *s)
{
  assert(json_reader_is_object(r));
  struct YChart *c = ght_get(charts, s, sizeof(struct YChart));

  c->count = json_int_array(r, "timestamp", c->timestamp, YARRAY_LENGTH);

  if (json_reader_read_member(r, "meta")) {
    json_string(r, "symbol", c->symbol);
    json_string(r, "instrumentType", c->instrumentType);
  } else {
    fprintf(stderr, "json_reader_read_member(meta)\n");
  }
  json_reader_end_member(r);

  if (json_reader_read_member(r, "indicators")) {
    if (json_reader_read_member(r, "adjclose") && json_reader_is_array(r)) {
      for (int i = 0; i < json_reader_count_elements(r); i++) {
        if (json_reader_read_element(r, i)) {
          json_double_array (r, "adjclose", c->adjclose, YARRAY_LENGTH);
        } else {
          fprintf(stderr, "json_reader_read_element(adjclose[%d])\n", i);
        }
        json_reader_end_element(r);
      }
    } else {
      fprintf(stderr, "json_reader_read_member(adjclose)\n");
    }
    json_reader_end_member(r);

    if (json_reader_read_member(r, "quote") && json_reader_is_array(r)) {
      for (int i = 0; i < json_reader_count_elements(r); i++) {
        if (json_reader_read_element(r, i)) {
          json_double_array (r, "close", c->close, YARRAY_LENGTH);
          json_double_array (r, "high", c->high, YARRAY_LENGTH);
          json_double_array (r, "low", c->low, YARRAY_LENGTH);
          json_double_array (r, "open", c->open, YARRAY_LENGTH);
          json_int_array    (r, "volume", c->volume, YARRAY_LENGTH);
        } else {
          fprintf(stderr, "json_reader_read_element(quote[%d])\n", i);
        }
        json_reader_end_element(r);
      }
    } else {
      fprintf(stderr, "json_reader_read_member(quote)\n");
    }
    json_reader_end_member(r);
  } else {
    fprintf(stderr, "json_reader_read_member(indicators)\n");
  }
  json_reader_end_member(r);

  return c;
}

static struct YOption *json_option(JsonReader *r, struct YOption *o)
{
  assert(json_reader_is_object(r));

  json_double (r, "ask", &o->ask);
  json_double (r, "bid", &o->bid);
  json_double (r, "change", &o->change);
  json_string (r, "contractSize", o->contractSize);
  json_string (r, "contractSymbol", o->contractSymbol);
  json_string (r, "currency", o->currency);
  json_int    (r, "expiration", &o->expiration);
  json_double (r, "impliedVolatility", &o->impliedVolatility);
  json_bool   (r, "inTheMoney", &o->inTheMoney);
  json_double (r, "lastPrice", &o->lastPrice);
  json_int    (r, "lastTradeDate", &o->lastTradeDate);
  json_int    (r, "openInterest", &o->openInterest);
  json_double (r, "percentChange", &o->percentChange);
  json_double (r, "strike", &o->strike);
  json_int    (r, "volume", &o->volume);

  return o;
}

static int json_options(JsonReader *r, const char *n, struct YOption o[], int m, double k0,
                        double kn _U_)
{
  assert(json_reader_is_object(r));
  int j = 0;
  double k = 0.0d;

  if (json_reader_read_member(r, n) && json_reader_is_array(r)) {
    for (int i = 0; i < json_reader_count_elements(r) && j < m; i++) {
      if (json_reader_read_element(r, i)) {
        json_double(r, "strike", &k);
        if (k0 <= k /* && k <= kn */) {
          json_option(r, &o[j++]);
        }
      } else {
        fprintf(stderr, "json_reader_read_element(%s[%d])\n", n, i);
      }
      json_reader_end_element(r);
    }
  } else {
    fprintf(stderr, "json_reader_read_member(%s)\n", n);
  }
  json_reader_end_member(r);

  return j;
}

static struct YOptionChain *json_optionChain(JsonReader *r, const char *s)
{
  assert(json_reader_is_object(r));
  struct YOptionChain *o = ght_get(options, s, sizeof(struct YOptionChain));

  json_string(r, "underlyingSymbol", o->underlyingSymbol);

  struct YQuote *q = NULL;
  if (json_reader_read_member(r, "quote")) {
    q = json_quote(r, o->underlyingSymbol);
  } else {
    fprintf(stderr, "json_reader_read_member(quote)\n");
    q = ght_get(quotes, o->underlyingSymbol, sizeof(struct YQuote));
  }
  json_reader_end_member(r);

  double k _U_ = 0.0d, ka = 0.0d, kb = 0.0d;
  void findNearestStrikes(JsonReader *r, const char *n, double *ka, double *kb)
  {
    if (json_reader_read_member(r, n) && json_reader_is_array(r)) {
      int m = json_reader_count_elements(r);
      for (int i = 0; i < m; i++) {
        if (json_reader_read_element(r, i)) {
          double k = json_reader_get_double_value(r);
          if (k > q->regularMarketPrice) {
            json_reader_end_element(r);

            int a = i - (YARRAY_LENGTH / 2), b = i + (YARRAY_LENGTH / 2);
            if (a < 0) {
              b = min(b + -a, m), a = max(0, a);
            }
            if (b > m) {
              a = max(0, a - (b - m)), b = min(b, m);
            }

            if (json_reader_read_element(r, a)) {
              *ka = json_reader_get_double_value(r);
            }
            json_reader_end_element(r);
            if (json_reader_read_element(r, b)) {
              *kb = json_reader_get_double_value(r);
            }
            json_reader_end_element(r);

            break;
          }
        } else {
          fprintf(stderr, "json_reader_read_element(%s[%d])\n", n, i);
        }
        json_reader_end_element(r);
      }
    } else {
      fprintf(stderr, "json_reader_read_member(%s)\n", n);
    }
    json_reader_end_member(r);
  }
  findNearestStrikes(r, "strikes", &ka, &kb);

  if (json_reader_read_member(r, "options") && json_reader_is_array(r)) {
    for (int i = 0; i < json_reader_count_elements(r); i++) {
      if (json_reader_read_element(r, i)) {
        json_int  (r, "expirationDate", &o->expirationDate);
        json_bool (r, "hasMiniOptions", &o->hasMiniOptions);
        o->countCalls = json_options(r, "calls", o->calls, YARRAY_LENGTH, ka, kb);
        o->countPuts  = json_options(r, "puts", o->puts, YARRAY_LENGTH, ka, kb);
      } else {
        fprintf(stderr, "json_reader_read_element(options[%d])\n", i);
      }
      json_reader_end_element(r);
    }
  } else {
    fprintf(stderr, "json_reader_read_member(options)\n");
  }
  json_reader_end_member(r);

  return o;
}

static int json_read(JsonNode *node)
{
  static YString symbol = "";

  JsonReader *reader = json_reader_new(node);
  if (json_reader_is_object(reader)) {
    if (json_reader_read_element(reader, 0)) {
      const char *response = json_reader_get_member_name(reader);
      if (json_reader_read_member(reader, "error")) {
        if (!json_reader_get_null_value(reader)) {
          const char *code, *description;
          if (json_reader_read_member(reader, "code")) {
            if (!json_reader_get_null_value(reader)) {
              code = json_reader_get_string_value(reader);
            }
          }
          json_reader_end_member(reader);
          if (json_reader_read_member(reader, "description")) {
            if (!json_reader_get_null_value(reader)) {
              description = json_reader_get_string_value(reader);
            }
          }
          json_reader_end_member(reader);
          fprintf(stderr, "YError: response=%s, code=%s, description=%s\n", response, code, description);
          return -1;
        }
      }
      json_reader_end_member(reader);

      if (json_reader_read_member(reader, "result")) {
        if (json_reader_is_array(reader)) {
          JsonNode *matched = json_path_match(path, node);
          JsonArray *matches = json_node_get_array(matched);
          int notches = json_array_get_length(matches);
          for (int i = 0; i < json_reader_count_elements(reader); i++) {
            if (json_reader_read_element(reader, i)) {
              if (i < notches) {
                strncpy(symbol, json_array_get_string_element(matches, i), YSTRING_LENGTH);
              }
              if (strcmp(response, "quoteResponse") == 0) {
                json_quote(reader, symbol);
              } else if (strcmp(response, "quoteSummary") == 0) {
                json_quoteSummary(reader, symbol);
              } else if (strcmp(response, "chart") == 0) {
                json_chart(reader, symbol);
              } else if (strcmp(response, "optionChain") == 0) {
                json_optionChain(reader, symbol);
              } else {
                fprintf(stderr, "YError: response=%s\n", response);
              }
            }
            json_reader_end_element(reader);
          }
          json_node_unref(matched);
        }
      }
      json_reader_end_member(reader);
    }
    json_reader_end_element(reader);
  }
  g_object_unref(reader);

  return 0;
}

static int json_parse(struct JSONBuffer *data)
{
  JsonParser *parser = json_parser_new();
  GError *error = NULL;
  json_parser_load_from_data(parser, data->bytes, data->size, &error);
  if (error) {
    g_print("json_parser_load_from_data: %s\n", error->message);
    g_error_free(error);
    g_object_unref(parser);
    return -1;
  }
  JsonNode *root = json_parser_get_root(parser);
  /* fprintf(stderr, "%s\n", json_to_string(root, TRUE)); */
  int status = json_read(root);
  g_object_unref(parser);
  return status;
}

int yql_init()
{
  quotes = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);
  summaries = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);
  charts = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);
  options = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);

  curl_global_init(CURL_GLOBAL_ALL);

  path = json_path_new();
  json_path_compile(path, "$..symbol", NULL);

  return 0;
}

static size_t write_func(char *p, size_t size, size_t nmemb, void *user)
{
  size_t nsize = size * nmemb;
  struct JSONBuffer *data = (struct JSONBuffer *) user;
  data->bytes = realloc(data->bytes, data->size + nsize + 1);
  if (!data->bytes) {
    perror("realloc");
    return 0;
  }
  memcpy(&data->bytes[data->size], p, nsize);
  data->size += nsize;
  data->bytes[data->size] = 0;
  return nsize;
}

int yql_open()
{
  if (!easy) {
    easy = curl_easy_init();
    if (!easy) {
      fprintf(stderr, "curl_easy_init");
      return -1;
    }
    /* char errbuf[CURL_ERROR_SIZE]; */
    /* CURLcode code = curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, errbuf); */
    /* if (code != CURLE_OK) { */
    /*   fprintf(stderr, "curl_easy_setopt(CURLOPT_ERRORBUFFER): %s\n", curl_easy_strerror(code)); */
    /* } */
    /* code = curl_easy_setopt(easy, CURLOPT_VERBOSE, 1L); */
    /* code = curl_easy_setopt(easy, CURLOPT_URL, url); */
    CURLcode code = curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, write_func);
    if (code != CURLE_OK) {
      fprintf(stderr, "curl_easy_setopt(CURLOPT_WRITEFUNCTION):%s\n", curl_easy_strerror(code));
      yql_close();
      return -1;
    }
    /* struct JSONBuffer data = { .bytes = malloc(0), .size = 0 }; */
    /* data.bytes = malloc(0), data.size = 0; */
    /* code = curl_easy_setopt(easy, CURLOPT_WRITEDATA, (void *) &data); */
    return 0;
  }
  return 1;
}

int yql_close()
{
  if (easy) {
    /* free(data.bytes); */
    curl_easy_cleanup(easy);
    easy = NULL;
    return 0;
  }
  return 1;
}

int yql_free()
{
  g_object_unref(path);

  curl_global_cleanup();

  g_hash_table_destroy(quotes);
  g_hash_table_destroy(summaries);
  g_hash_table_destroy(charts);
  g_hash_table_destroy(options);

  return 0;
}

static int yql_fetch(const char *url)
{
  if (!easy) {
    yql_open();
  }
  CURLcode code = curl_easy_setopt(easy, CURLOPT_URL, url);
  struct JSONBuffer data = { .bytes = malloc(0), .size = 0 };
  code = curl_easy_setopt(easy, CURLOPT_WRITEDATA, (void *) &data);
  if ((code = curl_easy_perform(easy)) != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform: %s\n", curl_easy_strerror(code));
    /* fprintf(stderr, "curl_easy_perform: %s\n", errbuf); */
    yql_close();
    return -1;
  }
  int status = json_parse(&data);
  free(data.bytes);
  return status;
}

static int yql_vafetch(const char *urlpath, ...)
{
  char *url = strdupa(urlpath);
  va_list args;
  va_start(args, urlpath);
  vasprintf(&url, url, args);
  va_end(args);
  return yql_fetch(url);
}

int yql_quote(const char *s)
{
  return yql_vafetch(Y_QUOTE "?symbols=%s", s);
}

int yql_quoteSummary(const char *s)
{
  /* modules=assetProfile,defaultKeyStatistics,esgScores,fundPerformance,fundProfile,topHolding */
  return yql_vafetch(Y_SUMMARY "/%s?modules=assetProfile,defaultKeyStatistics", s);
}

int yql_chart(const char *s)
{
  /* interval := [ "2m", "1d", "1wk", "1mo" ] */
  /* range    := [ "1d", "5d", "1mo", "3mo", "6mo", "1y", "2y", "5y", "10y", "ytd", "max" ] */
  return yql_vafetch(Y_CHART "/%s?symbol=%s"
                     "&events=div|split|earn" "&includeAdjustedClose=true" "&includePrePost=true"
                     "&interval=%s&range=%s" "&useYfid=true",
                     s, s, "1d", "1mo");
}

int yql_options(const char *s)
{
  return yql_vafetch(Y_OPTIONS "/%s?straddle=false", s);
}
