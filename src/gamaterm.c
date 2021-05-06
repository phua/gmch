#include <ctype.h>
#include <locale.h>
#include <signal.h>
/* #include <curses.h> */
#include <panel.h>
/* #include <cdk.h> */

#include "../include/gamaterm.h"

#define _U_ __attribute__ ((__unused__))

#define CMDTYN 8
#define INDEXN 9
#define CRNCYN 10
char *CMDTYS[CMDTYN] = { "CL=F", "GC=F", "SI=F", "LBS=F", "ES=F", "YM=F", "NQ=F", "RTY=F", };
char *INDEXS[INDEXN] = { "^GSPC", "^DJI", "^IXIC", "^RUT", "^TNX", "^VIX", "^CMC200", "^FTSE", "^N225", };
char *CRNCYS[CRNCYN] = { "BTC-USD", "BCH-USD", "BSV-USD", "ETH-USD", "ETC-USD", "DOGE-USD", "LTC-USD",
  "EURUSD=X", "GBPUSD=X", "JPY=X", };

int min(int, int);              /* yql.c */
int max(int, int);              /* yql.c */

static char *strfdt(int64_t ts, const char *s)
{
  static char buf[80];          /* Not thread-safe */
  time_t sec = ts;
  strftime(buf, sizeof(buf), s, localtime(&sec));
  return buf;
}

static char *strdatetime(int64_t ts)
{
  return strfdt(ts, "%B %d, %Y %I:%M %p %Z");
}

static char *strdate(int64_t ts)
{
  return strfdt(ts, "%b %d, %Y");
}

static char *strjoin(char *dest, char *src[], size_t n, const char *delim)
{
  for (size_t i = 0; i < n; i++) {
    strncat(dest, src[i], strlen(src[i]));
    strncat(dest, delim, strlen(delim));
  }
  return dest;
}

static int maxy, maxx, begy, begx, cury, curx;

#define getallyx(win)                           \
  getmaxyx(win, maxy, maxx);                    \
  getbegyx(win, begy, begx);                    \
  getyx(win, cury, curx)

#define DEBUG(f, ...)                           \
  fprintf(stderr, f, __VA_ARGS__)

#define DBGYX(s)                                                        \
  DEBUG("DBGYX %s: maxy=%d, maxx=%d, begy=%d, begx=%d, cury=%d, curx=%d\n", \
        s, maxy, maxx, begy, begx, cury, curx)

#define waddstrcp(win, cp, s)                   \
  wattron(win, cp);                             \
  waddstr(win, s);                              \
  wattroff(win, cp)

#define mvwaddstrcp(win, cp, y, x, s)           \
  wmove(win, (y), (x));                         \
  waddstrcp(win, cp, s)

#define mvwaddstrcn(win, cp, y, x, w, s)                                \
  mvwaddstrcp(win, cp, (y), (x) + (((w) - (x) * 2 - strlen(s)) / 2), s)

#define wprintwcp(win, cp, f, ...)              \
  wattron(win, cp);                             \
  wprintw(win, f, __VA_ARGS__);                 \
  wattroff(win, cp)

#define mvwprintwcp(win, cp, y, x, f, ...)      \
  wmove(win, (y), (x));                         \
  wprintwcp(win, cp, f, __VA_ARGS__);

#define mvwprintkey(win, y, x, w, L)                                    \
  mvwaddstrcp(win, COLOR_PAIR_KEY, (y) + L.keyy, (x) + (w) * L.keyx, L.key)

#define mvwprintkeys(win, y, x, w, L, n)            \
  getallyx(win);                                    \
  for (int i = 0; i < (n); i++) {                   \
    mvwprintkey(win, (y), (x), maxx / (w), L[i]);   \
  }

#define mvwprintval(win, y, x, w, L, ...)                               \
  mvwprintw(win, (y) + L.valy, (x) + (w) * L.valx, L.fmt, __VA_ARGS__)

#define mvwprintvnz(win, y, x, w, L, i)         \
  if (i != 0) {                                 \
    mvwprintval(win, (y), (x), (w), L, i);      \
  }

#define mvwprintvcp(win, cp, y, x, w, L, ...)       \
  wattron(win, cp);                                 \
  mvwprintval(win, (y), (x), (w), L, __VA_ARGS__);  \
  wattroff(win, cp)

#define wclearl(win, y, x, w)                           \
  mvwprintw(win, (y), (x), "%*s", (w) - (x) - 1, "")

static WINDOW *w_nfo;
static WINDOW *w_cmd;
static char srchs[32];
static int  srchi = 0;

static PANEL *panels[CLIENT + 1];
static enum PanelType curpan = HELP;
static struct Spark sparks[CLIENT + 1] = {
  { .pan = HELP  , .count = 0, },
  { .pan = GOVT  , .count = 0, },
  { .pan = CORP  , .count = 0, },
  { .pan = MTGE  , .count = 0, },
  { .pan = MMKT  , .count = 0, },
  { .pan = MUNI  , .count = 0, },
  { .pan = PFD   , .count = 0, },
  { .pan = EQUITY, .count = 0, },
  { .pan = CMDTY , .count = CMDTYN, .symbols = CMDTYS, },
  { .pan = INDEX , .count = INDEXN, .symbols = INDEXS, },
  { .pan = CRNCY , .count = CRNCYN, .symbols = CRNCYS, },
  { .pan = CLIENT, .count = 0, },
};

static void repaint()
{
  wnoutrefresh(stdscr);
  touchwin(panels[curpan]->win);
  wnoutrefresh(panels[curpan]->win);
  update_panels();
  doupdate();
}

static void paint(enum PanelType newpan)
{
  hide_panel(panels[curpan]);
  curpan = newpan;
  show_panel(panels[curpan]);
  update_panels();
  doupdate();
}

static void reset(enum PanelType newpan)
{
  curpan = newpan;
  wclear(panels[curpan]->win);
  void Spark_print(struct Spark *);
  Spark_print(&sparks[curpan]);
}

static void wprint_nfo(WINDOW *win)
{
  getallyx(win);
  box(win, 0, 0);
  mvwaddstr(win, MARGIN_Y, MARGIN_X, TERM_NAME);
  mvwaddstrcn(win, COLOR_PAIR_INFO, MARGIN_Y, MARGIN_X, maxx, TERM_KEYS);
}

static void wprint_cmd(WINDOW *win)
{
  getallyx(win);
  box(win, 0, 0);
  mvwaddstrcp(win, COLOR_PAIR_CMD, MARGIN_Y, MARGIN_X, TERM_PROMPT);
}

static void wprintvbox(WINDOW *win)
{
  getallyx(win);
  box(win, 0, 0);
  mvwaddstrcn(win, COLOR_PAIR_INFO, MARGIN_Y, MARGIN_X, maxx, "This page intentionally left blank.");
}

static void wprintwbox(WINDOW *win, int y, int x, int w, const char *s, struct Layout L[], int n)
{
  box(win, 0, 0);
  if (s) {
    mvwaddstrcp(win, COLOR_PAIR_TITLE, MARGIN_Y, MARGIN_X, s);
  }
  if (L) {
    mvwprintkeys(win, y, x * MARGIN_X, w, L, n);
  }
}

static void wprint_help(WINDOW *win)
{
  getallyx(win);
  int y = MARGIN_Y, x = MARGIN_X, cp = COLOR_PAIR_DEFAULT;

  box(win, 0, 0);
  cp = COLOR_PAIR_CANCEL;
  mvwaddstrcp(win, cp, y++, x, "ESC    CANCEL        Cancel");
  cp = COLOR_PAIR_GO;
  mvwaddstrcp(win, cp, y++, x, "F1     HELP          Help");
  cp = COLOR_PAIR_INFO;
  mvwaddstrcp(win, cp, y++, x, "F2     GOVT          Government securities");
  mvwaddstrcp(win, cp, y++, x, "F3     CORP          Corporate debt");
  mvwaddstrcp(win, cp, y++, x, "F4     MTGE          Mortgage securities");
  mvwaddstrcp(win, cp, y++, x, "F5     M-MKT         Money market");
  mvwaddstrcp(win, cp, y++, x, "F6     MUNI          Municipal debt");
  mvwaddstrcp(win, cp, y++, x, "F7     PFD           Preferred shares");
  mvwaddstrcp(win, cp, y++, x, "F8     EQUITY        Equity shares");
  mvwaddstrcp(win, cp, y++, x, "F9     CMDTY         Commodity markets");
  mvwaddstrcp(win, cp, y++, x, "F10    INDEX         Indices");
  mvwaddstrcp(win, cp, y++, x, "F11    CRNCY         Currency markets");
  mvwaddstrcp(win, cp, y++, x, "F12    CLIENT/ALPHA  Portfolio functionality");
  cp = COLOR_PAIR_GO;
  mvwaddstrcp(win, cp, y++, x, "ENTER  GO            Go");
  mvwhline(win, y++, x, ACS_HLINE, maxx - MARGIN_X * 2);
  cp = COLOR_PAIR_BLUE;
  mvwaddstrcp(win, cp, y++, x, "<F(n)>               Refresh page");
  mvwaddstrcp(win, cp, y++, x, "O                    Open portfolio");
  mvwaddstrcp(win, cp, y++, x, "Q                    Quit");
  mvwhline(win, y++, x, ACS_HLINE, maxx - MARGIN_X * 2);
  cp = COLOR_PAIR_CMD;
  mvwaddstrcp(win, cp, y++, x, "/SYMBOL <F(n)> <GO>  Search symbol");
  mvwaddstrcp(win, cp, y++, x, "@VENUE <GO>          Search venue");
  mvwaddstrcp(win, cp, y++, x, ":COMMAND <GO>        Run command");
}

static void mvwprintq(WINDOW *win, int y, int x, struct YQuote *q, double price, double change, double percent, int64_t time, char *state)
{
  mvwprintwcp(win, COLOR_PAIR_CHANGE(change), y, x, "%'.2f %+.2f (%+.2f%%)", price, change, percent);
  if (!IS_CLOSED(state)) {
    mvwprintwcp(win, COLOR_PAIR_INFO, y + 1, x, "%'.2f x %ld / %'.2f x %ld", q->bid, q->bidSize, q->ask, q->askSize);
  }
  mvwprintw(win, y + 2, x, "%s (", strdatetime(time));
  waddstrcp(win, COLOR_PAIR_MARKET(state), state);
  waddstr(win, ")");
}

static void mvwprintq_beta(WINDOW *win, double beta)
{
  getallyx(win);
  int y = MARGIN_Y + 8, x = MARGIN_X, w = maxx / QuoteLayoutC;
  mvwprintvnz(win, y, x, w, QuoteLayout[18], beta);
}

static void mvwprintq_longName(WINDOW *win, YString longName)
{
  int y = MARGIN_Y + 2, x = MARGIN_X * 2;
  mvwaddstr(win, y, x, longName);
}

static void wprint_quote(WINDOW *win, struct YQuote *q)
{
  getallyx(win);
  int y = MARGIN_Y, x = MARGIN_X, w = maxx / QuoteLayoutC, cp = COLOR_PAIR_DEFAULT;

  cp = COLOR_PAIR_TITLE;
  mvwprintwcp(win, cp, y++, x, "%s %s-%s", q->symbol, q->quoteType, q->exchange);
  mvwaddstrcp(win, cp, y++, x, q->shortName);
  mvwprintwcp(win, cp, y++, x, "%s: %s - %s (%s)", q->fullExchangeName, q->symbol, q->quoteSourceName, q->currency);

  mvwprintq(win, ++y, x, q, q->regularMarketPrice, q->regularMarketChange, q->regularMarketChangePercent, q->regularMarketTime, IS_REGULAR(q->marketState) ? q->marketState : "CLOSED");
  w = maxx / 2, x += w;
  if (q->preMarketTime) {
    mvwprintq(win, y, x, q, q->preMarketPrice, q->preMarketChange, q->preMarketChangePercent, q->preMarketTime, q->marketState);
  } else if (q->postMarketTime) {
    mvwprintq(win, y, x, q, q->postMarketPrice, q->postMarketChange, q->postMarketChangePercent, q->postMarketTime, q->marketState);
  } else {
    wclearl(win, y + 0, x, w);
    wclearl(win, y + 1, x, w);
    wclearl(win, y + 2, x, w);
  }

  y += 3 + 1, x = MARGIN_X, w = maxx / QuoteLayoutC;
  mvwprintval(win, y, x, w, QuoteLayout[0], q->regularMarketPreviousClose);
  cp = COLOR_PAIR_CHANGE(q->regularMarketOpen - q->regularMarketPreviousClose);
  mvwprintvcp(win, cp, y, x, w, QuoteLayout[1], q->regularMarketOpen);
  cp = COLOR_PAIR_INFO;
  mvwprintvcp(win, cp, y, x, w, QuoteLayout[2], q->bid, q->bidSize);
  mvwprintvcp(win, cp, y, x, w, QuoteLayout[3], q->ask, q->askSize);
  cp = COLOR_PAIR_CHANGE(q->regularMarketDayLow - q->regularMarketPreviousClose);
  mvwprintvcp(win, cp, y, x, w, QuoteLayout[4], q->regularMarketDayLow);
  cp = COLOR_PAIR_CHANGE(q->regularMarketDayHigh - q->regularMarketPreviousClose);
  mvwprintvcp(win, cp, y, x, w, QuoteLayout[5], q->regularMarketDayHigh);
  mvwprintval(win, y, x, w, QuoteLayout[6], q->regularMarketVolume);
  cp = COLOR_PAIR_CHANGE(q->fiftyDayAverageChange);
  mvwprintvcp(win, cp, y, x, w, QuoteLayout[7], q->fiftyDayAverage, q->fiftyDayAverageChangePercent * 100);
  cp = COLOR_PAIR_CHANGE(q->twoHundredDayAverageChange);
  mvwprintvcp(win, cp, y, x, w, QuoteLayout[9], q->twoHundredDayAverage, q->twoHundredDayAverageChangePercent * 100);
  mvwprintval(win, y, x, w, QuoteLayout[11], q->fiftyTwoWeekLow);
  mvwprintval(win, y, x, w, QuoteLayout[12], q->fiftyTwoWeekHigh);
  mvwprintvnz(win, y, x, w, QuoteLayout[14], q->marketCap);
  mvwprintvnz(win, y, x, w, QuoteLayout[16], q->trailingPE);
  mvwprintvnz(win, y, x, w, QuoteLayout[17], q->epsTrailingTwelveMonths);
  /* mvwprintvnz(win, y, x, w, QuoteLayout[18], q->beta); */
  mvwprintval(win, y, x, w, QuoteLayout[19], q->averageDailyVolume10Day);
  mvwprintval(win, y, x, w, QuoteLayout[20], q->averageDailyVolume3Month);
  mvwprintvnz(win, y, x, w, QuoteLayout[21], q->sharesOutstanding);

  if (IS_EQUITY(q->quoteType)) {
    mvwprintval(win, y, x, w, QuoteLayout[23], strdate(q->earningsTimestampStart));
    if (q->earningsTimestampStart != q->earningsTimestampEnd) {
      wprintw(win, " - %s", strdate(q->earningsTimestampEnd));
    }
    mvwprintval(win, y, x, w, QuoteLayout[25], strdate(q->dividendDate));
    mvwprintval(win, y, x, w, QuoteLayout[27], q->trailingAnnualDividendRate, q->trailingAnnualDividendYield * 100);
  }

  if (IS_CRYPTO(q->quoteType)) {
    mvwprintval(win, y, x, w, QuoteLayout[28], q->circulatingSupply);
    mvwprintval(win, y, x, w, QuoteLayout[32], q->volume24Hr);
    mvwprintval(win, y, x, w, QuoteLayout[34], q->volumeAllCurrencies);
  }

  y += 3 * 5;
  mvwaddstrcp(win, COLOR_PAIR_KEY, y, x, "Average Analyst Rating");
  mvwaddstr(win, y, x + w * 2, q->averageAnalystRating);
}

static void wprint_assetProfile(WINDOW *win, struct YQuoteSummary *q)
{
  getallyx(win);
  int y = MARGIN_Y + 2, x = MARGIN_X * 2, w = maxx / AssetProfileC;

  /* mvwaddstr(win, y++, x, q->longName); */ y++;
  mvwaddstr(win, y++, x, q->address1);
  mvwprintw(win, y++, x, "%s, %s %s", q->city, q->state, q->zip);
  mvwaddstr(win, y++, x, q->country);
  mvwprintwcp(win, COLOR_PAIR_LINK, y++, x, "tel://%s", q->phone);
  mvwaddstrcp(win, COLOR_PAIR_LINK, y++, x, q->website);

  y = MARGIN_Y + 2, w = maxx / 3, x = w * 1;
  mvwprintval(win, y, x, w, AssetProfileLayout[0], q->sector);
  mvwprintval(win, y, x, w, AssetProfileLayout[1], q->industry);
  mvwprintval(win, y, x, w, AssetProfileLayout[2], q->fullTimeEmployees);
}

static void wprint_keyStatistics(WINDOW *win, struct YQuoteSummary *q)
{
  getallyx(win);
  int y = MARGIN_Y + 2, x = MARGIN_X * 2, w = maxx / KeyStatisticsC;

  mvwprintval(win, y, x, w, KeyStatisticsLayout[0], q->beta);
  mvwprintval(win, y, x, w, KeyStatisticsLayout[1], q->bookValue);
  mvwprintval(win, y, x, w, KeyStatisticsLayout[2], q->earningsQuarterlyGrowth * 100);
  mvwprintval(win, y, x, w, KeyStatisticsLayout[3], q->enterpriseToEbitda);
  mvwprintval(win, y, x, w, KeyStatisticsLayout[4], q->enterpriseValue);
  mvwprintval(win, y, x, w, KeyStatisticsLayout[5], q->floatShares);
  mvwprintval(win, y, x, w, KeyStatisticsLayout[6], q->forwardEps);
  mvwprintval(win, y, x, w, KeyStatisticsLayout[7], q->forwardPE);
  mvwprintval(win, y, x, w, KeyStatisticsLayout[8], q->heldPercentInsiders * 100);
  mvwprintval(win, y, x, w, KeyStatisticsLayout[9], q->profitMargins * 100);
  mvwprintval(win, y, x, w, KeyStatisticsLayout[10], q->sharesOutstanding);
  mvwprintval(win, y, x, w, KeyStatisticsLayout[11], q->sharesShort);
  mvwprintval(win, y, x, w, KeyStatisticsLayout[12], q->shortRatio);
  mvwprintval(win, y, x, w, KeyStatisticsLayout[13], q->fiftyTwoWeekChange * 100);
  mvwprintval(win, y, x, w, KeyStatisticsLayout[14], q->priceToBook);
  mvwprintval(win, y, x, w, KeyStatisticsLayout[15], q->enterpriseToRevenue);
  mvwprintval(win, y, x, w, KeyStatisticsLayout[16], q->shortPercentOfFloat * 100);
  mvwprintval(win, y, x, w, KeyStatisticsLayout[17], q->trailingEps);
  mvwprintval(win, y, x, w, KeyStatisticsLayout[18], q->pegRatio);
  mvwprintval(win, y, x, w, KeyStatisticsLayout[19], q->heldPercentInstitutions * 100);
  mvwprintval(win, y, x, w, KeyStatisticsLayout[20], q->netIncomeToCommon);
  mvwprintval(win, y, x, w, KeyStatisticsLayout[21], q->sharesPercentSharesOut * 100);
  mvwprintval(win, y, x, w, KeyStatisticsLayout[22], q->sharesShortPriorMonth);
}

static void wprint_chart(WINDOW *win, struct YChart *c)
{
  getallyx(win);
  int y = MARGIN_Y + 3, x = MARGIN_X * 2, w = maxx / ChartLayoutC, cp = COLOR_PAIR_DEFAULT;

  int n = min(c->count, maxy - y - MARGIN_Y * 2);
  for (int i = y, j = c->count - n; i < y + n && j < c->count; i++, j++) {
    int k = max(j - 1, 0);
    mvwprintval(win, i, x, w, ChartLayout[0], strdate(c->timestamp[j]));
    cp = COLOR_PAIR_CHANGE(c->open[j] - c->open[k]);
    mvwprintvcp(win, cp, i, x, w, ChartLayout[1], c->open[j]);
    cp = COLOR_PAIR_CHANGE(c->high[j] - c->high[k]);
    mvwprintvcp(win, cp, i, x, w, ChartLayout[2], c->high[j]);
    cp = COLOR_PAIR_CHANGE(c->low[j] - c->low[k]);
    mvwprintvcp(win, cp, i, x, w, ChartLayout[3], c->low[j]);
    cp = COLOR_PAIR_CHANGE(c->close[j] - c->close[k]);
    mvwprintvcp(win, cp, i, x, w, ChartLayout[4], c->close[j]);
    cp = COLOR_PAIR_CHANGE(c->adjclose[j] - c->adjclose[k]);
    mvwprintvcp(win, cp, i, x, w, ChartLayout[5], c->adjclose[j]);
    cp = COLOR_PAIR_CHANGE(c->volume[j] - c->volume[k]);
    mvwprintvcp(win, cp, i, x, w, ChartLayout[6], c->volume[j]);
  }
}

static void wprint_options(WINDOW *win, struct YOptionChain *o)
{
  getallyx(win);
  int y = MARGIN_Y + 4, x = MARGIN_X * 2, w = maxx / OptionLayoutC, cp = COLOR_PAIR_DEFAULT;

  mvwaddstrcn(win, COLOR_PAIR_LINK, MARGIN_Y + 1, x, maxx, strdate(o->expirationDate));

  int n = min(o->countCalls, maxy - y - MARGIN_Y * 2);
  for (int i = y, j = (o->countCalls - n) / 2; i < y + n; i++, j++) {
    cp = COLOR_PAIR_CHANGE(o->calls[j].change);
    mvwprintvcp(win, cp, i, x, w, OptionLayout[0], o->calls[j].change, o->calls[j].percentChange);
    cp = COLOR_PAIR_BOOL(o->calls[j].inTheMoney);
    mvwprintvcp(win, cp, i, x, w, OptionLayout[1], o->calls[j].lastPrice);
    cp = o->calls[j].inTheMoney ? COLOR_PAIR_INFO : COLOR_PAIR_DEFAULT;
    mvwprintvcp(win, cp, i, x, w, OptionLayout[2], o->calls[j].strike);
    cp = COLOR_PAIR_BOOL(o->puts[j].inTheMoney);
    mvwprintvcp(win, cp, i, x, w, OptionLayout[3], o->puts[j].lastPrice);
    cp = COLOR_PAIR_CHANGE(o->puts[j].change);
    mvwprintvcp(win, cp, i, x, w, OptionLayout[4], o->puts[j].change, o->puts[j].percentChange);
  }
}

static void wprint_spark(WINDOW *win, char *s[], size_t n)
{
  getallyx(win);
  int y = MARGIN_Y + 3, x = MARGIN_X * 2, w = maxx / SparkLayoutC, cp = COLOR_PAIR_DEFAULT;

  size_t m = min(n, maxy - y - MARGIN_Y * 2);
  for (size_t i = y, j = 0; i < y + m && j < m; i++, j++) {
    struct YQuote *q = g_hash_table_lookup(quotes, s[j]);
    if (q) {
      cp = COLOR_PAIR_TITLE;
      mvwprintvcp(win, cp, i, x, w, SparkLayout[0], q->symbol);
      cp = COLOR_PAIR_CHANGE(q->regularMarketChange);
      mvwprintvcp(win, cp, i, x, w, SparkLayout[1], q->regularMarketPrice, q->regularMarketChangePercent);
      cp = COLOR_PAIR_CHANGE(q->regularMarketOpen - q->regularMarketPreviousClose);
      mvwprintvcp(win, cp, i, x, w, SparkLayout[2], q->regularMarketOpen);
      cp = COLOR_PAIR_CHANGE(q->regularMarketDayHigh - q->regularMarketPreviousClose);
      mvwprintvcp(win, cp, i, x, w, SparkLayout[3], q->regularMarketDayHigh);
      cp = COLOR_PAIR_CHANGE(q->regularMarketDayLow - q->regularMarketPreviousClose);
      mvwprintvcp(win, cp, i, x, w, SparkLayout[4], q->regularMarketDayLow);
      mvwprintval(win, i, x, w, SparkLayout[5], q->regularMarketPreviousClose);
      mvwprintval(win, i, x, w, SparkLayout[6], q->regularMarketVolume);
      mvwprintval(win, i, x, w, SparkLayout[7], q->fiftyTwoWeekLow);
      mvwprintval(win, i, x, w, SparkLayout[8], q->fiftyTwoWeekHigh);
    }
  }
}

void Spark_init(struct Spark *s)
{
  WINDOW *win = panels[s->pan]->win;
  getallyx(win);

  s->cursym[0] = s->query[0] = 0;
  switch (s->pan) {
  case HELP:
  case GOVT:
  case CORP:
  case MTGE:
  case MMKT:
  case MUNI:
  case PFD:
    break;
  case EQUITY:
    strncpy(s->cursym, "GME", YSTRING_LENGTH);

    s->w_quote         = subwin(win, maxy / 2, maxx / 2, begy + 0 * maxy / 2, begx + 0 * maxx / 2);
    s->w_options       = subwin(win, maxy / 2, maxx / 2, begy + 1 * maxy / 2, begx + 0 * maxx / 2);
    s->w_chart         = subwin(win, maxy / 3, maxx / 2, begy + 0 * maxy / 3, begx + 1 * maxx / 2);
    s->w_assetProfile  = subwin(win, maxy / 3, maxx / 2, begy + 1 * maxy / 3, begx + 1 * maxx / 2);
    s->w_keyStatistics = subwin(win, maxy / 3, maxx / 2, begy + 2 * maxy / 3, begx + 1 * maxx / 2);
    break;
  case CMDTY:
  case INDEX:
  case CRNCY:
    strncpy(s->cursym, s->symbols[0], YSTRING_LENGTH);
    strjoin(s->query, s->symbols, s->count, ",");

    s->w_quote = subwin(win, maxy / 2, maxx / 2, begy + 0 * maxy / 2, begx + 0 * maxx / 2);
    s->w_chart = subwin(win, maxy / 2, maxx / 2, begy + 0 * maxy / 2, begx + 1 * maxx / 2);
    s->w_spark = subwin(win, maxy / 2, maxx    , begy + 1 * maxy / 2, begx + 0 * maxx / 2);
    break;
  case CLIENT:
    break;
  }
}

void Spark_free(struct Spark *s)
{
  switch (s->pan) {
  case HELP:
  case GOVT:
  case CORP:
  case MTGE:
  case MMKT:
  case MUNI:
  case PFD:
    break;
  case EQUITY:
    delwin(s->w_quote);
    delwin(s->w_options);
    delwin(s->w_chart);
    delwin(s->w_assetProfile);
    delwin(s->w_keyStatistics);
    break;
  case CMDTY:
  case INDEX:
  case CRNCY:
    delwin(s->w_quote);
    delwin(s->w_chart);
    delwin(s->w_spark);
    break;
  case CLIENT:
    break;
  }
}

void Spark_print(struct Spark *s)
{
  switch (s->pan) {
  case HELP:
    wprint_help(panels[s->pan]->win);
    break;
  case GOVT:
  case CORP:
  case MTGE:
  case MMKT:
  case MUNI:
  case PFD:
    wprintvbox(panels[s->pan]->win);
    break;
  case EQUITY:
    wprintwbox(s->w_quote, 9, 1, QuoteLayoutC, NULL, QuoteLayout, QuoteLayoutN);
    wprintwbox(s->w_assetProfile, 3, 24, AssetProfileC, "Asset Profile", AssetProfileLayout, AssetProfileN);
    wprintwbox(s->w_assetProfile, 10, 2, 5, NULL, CompanyOfficerLayout, 5);
    wprintwbox(s->w_keyStatistics, 3, 2, KeyStatisticsC, "Key Statistics", KeyStatisticsLayout, KeyStatisticsN);
    wprintwbox(s->w_chart, 3, 2, ChartLayoutC, "Chart", ChartLayout, ChartLayoutN);
    wprintwbox(s->w_options, 3, 5 * MARGIN_X, 2, "Option Chain", OptionsLayout, 2);
    wprintwbox(s->w_options, 4, 2, OptionLayoutC, NULL, OptionLayout, OptionLayoutN);
    break;
  case CMDTY:
  case INDEX:
  case CRNCY:
    wprintwbox(s->w_quote, 9, 1, QuoteLayoutC, NULL, QuoteLayout, QuoteLayoutN);
    wprintwbox(s->w_chart, 3, 2, ChartLayoutC, "Chart", ChartLayout, ChartLayoutN);
    wprintwbox(s->w_spark, 3, 2, SparkLayoutC, "Market Overview", SparkLayout, SparkLayoutN);

    getallyx(s->w_spark);
    mvwhline(s->w_spark, 4, MARGIN_X, ACS_HLINE, maxx - MARGIN_X * 2);
    break;
  case CLIENT:
    wprintvbox(panels[s->pan]->win);
    break;
  }
}

static struct Spark *Spark_fromQuoteType(const char *q)
{
  if (IS_EQUITY(q) || IS_ETF(q)) {
    return &sparks[EQUITY];
  } else if (IS_FUTURE(q)) {
    return &sparks[CMDTY];
  } else if (IS_INDEX(q)) {
    return &sparks[INDEX];
  } else if (IS_CURRENCY(q) || IS_CRYPTO(q)) {
    return &sparks[CRNCY];
  } else {
    return &sparks[curpan];
  }
}

void Spark_update(const char *symbol)
{
  if (yql_quote(symbol) != -1) {
    struct YQuote *q = g_hash_table_lookup(quotes, symbol);
    if (q) {
      struct Spark *s = Spark_fromQuoteType(q->quoteType);
      reset(s->pan);
      wprint_quote(s->w_quote, q);
      if (s->pan == EQUITY) {
        mvwprintq_longName(s->w_assetProfile, q->longName);
        if (yql_quoteSummary(symbol) != -1) {
          struct YQuoteSummary *q = g_hash_table_lookup(summaries, symbol);
          if (q) {
            wprint_assetProfile(s->w_assetProfile, q);
            wprint_keyStatistics(s->w_keyStatistics, q);
            mvwprintq_beta(s->w_quote, q->beta);
          }
        }
        if (yql_options(symbol) != -1) {
          struct YOptionChain *o = g_hash_table_lookup(options, symbol);
          if (o) {
            wprint_options(s->w_options, o);
          }
        }
        /* wprint_client(sparks[CLIENT].w_spark); */
      }
      if (yql_chart(symbol) != -1) {
        struct YChart *c = g_hash_table_lookup(charts, symbol);
        if (c) {
          wprint_chart(s->w_chart, c);
        }
      }
      if (s->count > 0 && yql_quote(s->query) != -1) {
        wprint_spark(s->w_spark, s->symbols, s->count);
      }
    }
  }
}

void Spark_refresh()
{
  struct Spark *s = &sparks[curpan];
  if (s->cursym[0] != 0) {
    Spark_update(s->cursym);
  }
}

static void *start_clock(void *arg)
{
  static time_t tm;
  static char s[32];

  tm = time(NULL);
  ctime_r(&tm, s);
  WINDOW *win = (WINDOW *) arg;
  getallyx(win);
  mvwprintw(win, MARGIN_Y, maxx - 1 - strlen(s), "%.24s", s);
  wnoutrefresh(win);

  return NULL;
}

static void wclear_cmd(WINDOW *win, const char *s)
{
  srchs[0] = srchi = 0;
  getallyx(win);
  wclearl(win, MARGIN_Y, MARGIN_X, maxx);
  mvwaddstrcp(win, COLOR_PAIR_CMD, MARGIN_Y, MARGIN_X, s);
  wrefresh(win);
}

static void wsearch_cmd(WINDOW *win)
{
  wclear_cmd(win, "/");

  int c;
  while ((c = wgetch(win))) {
    switch (c) {
    case KEY_ENTER:
    case 015:
    ENTER:
      srchs[srchi] = 0;
      Spark_update(srchs);
      strncpy(sparks[curpan].cursym, srchs, srchi + 1);
      goto ESC;
    case KEY_F(8):
      goto ENTER;
    case KEY_F(9):              /* strappend(srchs, "=F"); */
      goto ENTER;
    case KEY_F(10):             /* strprepend(srchs, "^"); */
      goto ENTER;
    case KEY_F(11):             /* strappend(srchs, "=X"); or "-USD" */
      goto ENTER;
    case KEY_EIC:
    case 033:
    ESC:
      wclear_cmd(win, TERM_PROMPT);
      return;
    case KEY_BACKSPACE:
    case 0177:
      srchs[--srchi] = 0;
      getyx(win, cury, curx);
      mvwaddch(win, cury, curx - 1, ' ');
      wmove(win, cury, curx - 1);
      wrefresh(win);
      break;
    case '-':
    case '=':
    case 'A' ... 'Z':
    case '^':
    case 'a' ... 'z':
      c = toupper(c);
      srchs[srchi++] = c;
      waddch(win, c);
      wrefresh(win);
      break;
    }
  }
}

static void init()
{
  setlocale(LC_ALL, "");

  initscr();
  cbreak();
  halfdelay(1);
  keypad(stdscr, TRUE);
  noecho();
  nonl();
  curs_set(0);

  if (has_colors()) {
    start_color();
    use_default_colors();
    init_pair(COLOR_BLACK, COLOR_BLACK, -1);
    init_pair(COLOR_RED, COLOR_RED, -1);
    init_pair(COLOR_GREEN, COLOR_GREEN, -1);
    init_pair(COLOR_YELLOW, COLOR_YELLOW, -1);
    init_pair(COLOR_BLUE, COLOR_BLUE, -1);
    init_pair(COLOR_MAGENTA, COLOR_MAGENTA, -1);
    init_pair(COLOR_CYAN, COLOR_CYAN, -1);
    init_pair(COLOR_WHITE, COLOR_WHITE, -1);
  }

  getallyx(stdscr);
  w_nfo = derwin(stdscr, 3, maxx - MARGIN_X * 2, begy    , MARGIN_X);
  w_cmd = derwin(stdscr, 3, maxx - MARGIN_X * 2, maxy - 3, MARGIN_X);
  keypad(w_cmd, TRUE);
  for (int i = 0; i <= CLIENT; i++) {
    panels[i] = new_panel(newwin(maxy - 3 * 2, maxx - MARGIN_X * 2, begy + 3, MARGIN_X));
  }
}

static void start()
{
  yql_init();
  yql_open();

  wprint_nfo(w_nfo);
  wprint_cmd(w_cmd);
  for (int i = 0; i <= CLIENT; i++) {
    Spark_init(&sparks[i]);
    Spark_print(&sparks[i]);
  }

  int c;
  while ((c = getch())) {
    switch (c) {
    case KEY_F(1):
    case 071:                   /* CTX_MENU */
      curpan = HELP;
      goto REFRESH;
    case KEY_F(2):
      curpan = GOVT;
      goto REFRESH;
    case KEY_F(3):
      curpan = CORP;
      goto REFRESH;
    case KEY_F(4):
      curpan = MTGE;
      goto REFRESH;
    case KEY_F(5):
      curpan = MMKT;
      goto REFRESH;
    case KEY_F(6):
      curpan = MUNI;
      goto REFRESH;
    case KEY_F(7):
      curpan = PFD;
      goto REFRESH;
    case KEY_F(8):
      curpan = EQUITY;
      goto REFRESH;
    case KEY_F(9):
      curpan = CMDTY;
      goto REFRESH;
    case KEY_F(10):
      curpan = INDEX;
      goto REFRESH;
    case KEY_F(11):
      curpan = CRNCY;
      goto REFRESH;
    case KEY_F(12):
      curpan = CLIENT;
    REFRESH:
      Spark_refresh();
      paint(curpan);
      break;
    case '/':
      wsearch_cmd(w_cmd);
      break;
    case 'Q':
    case 'q':
      return;
    case ERR:
      start_clock(w_nfo);
      repaint();
      break;
    }
  }
}

static void destroy()
{
  yql_close();
  yql_free();

  clear();
  wrefresh(stdscr);
  delwin(w_nfo);
  delwin(w_cmd);
  for (int i = 0; i <= CLIENT; i++) {
    Spark_free(&sparks[i]);
    delwin(panels[i]->win);
    del_panel(panels[i]);
  }
  endwin();
}

static void _sigwinch(int sig _U_)
{
  endwin();
  refresh();
}

static void _sigint(int sig _U_)
{
  destroy();
  exit(EXIT_SUCCESS);
}

int main(int argc _U_, char *argv[] _U_)
{
  if (signal(SIGWINCH, _sigwinch) == SIG_ERR) {
    perror("signal(SIGWINCH)");
  }
  if (signal(SIGINT, _sigint) == SIG_ERR) {
    perror("signal(SIGINT)");
  }

  init();
  start();
  destroy();

  return EXIT_SUCCESS;
}
