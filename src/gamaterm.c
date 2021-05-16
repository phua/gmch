#include <ctype.h>
#include <locale.h>
#include <signal.h>
/* #include <curses.h> */
#include <panel.h>
/* #include <cdk.h> */

#include "../include/gamaterm.h"
#include "../include/config.h"
#include "../include/pm.h"

#define _U_ __attribute__ ((__unused__))

int min(int, int);              /* yql.c */
int max(int, int);              /* yql.c */

#define tm_day (60 * 60 * 24)
#define tm_cnt (7 * 4 * 3)

static time_t tm_bod, tm_bow, tm_bom, tm_bop, tm_eop;
static struct tm stm_bod, stm_bow;

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

static char *strwdate(int64_t ts)
{
  return strfdt(ts, "%a %b %d, %Y");
}

static char *strptime(int64_t ts)
{
  return strfdt(ts, "%I:%M %p");
}

struct Event *Event_new(enum EventType type, int64_t timestamp, char *symbol, char *shortName)
{
  struct Event *e = malloc(sizeof(struct Event));
  e->next = NULL;
  e->type = type;
  e->timestamp = timestamp;
  e->symbol = symbol;
  e->shortName = shortName;
  return e;
}

void Event_destroy(struct Event *e)
{
  struct Event *p = e, *q = NULL;
  while (p) {
    q = p->next;
    free(p);
    p = q;
  }
}

int Event_cmp(const struct Event *p, const struct Event *q)
{
  int cmp = 0;
  return (cmp = strcmp(p->symbol, q->symbol)) ? cmp :
    (cmp = p->type - q->type) ? cmp : p->timestamp - q->timestamp;
}

int Event_insert(struct Event **d, struct Event *e)
{
  if (*d) {
    struct Event *p = *d, *q = NULL;
    int cmp = 0;
    while (p && (cmp = Event_cmp(p, e)) < 0) {
      q = p, p = p->next;
    }
    if (cmp) {
      if (q) {
        e->next = q->next, q->next = e;
      } else {
        *d = e, e->next = p;
      }
      return 1;
    }
  } else {
    *d = e;
    return 1;
  }
  return 0;
}

void EventDate_init(struct EventDate *e)
{
  for (int i = 0; i < tm_cnt; i++) {
    e[i].date = tm_bop + i * tm_day;
    e[i].count = 0;
    e[i].events = NULL;
  }
}

void EventDate_free(struct EventDate *e)
{
  for (int i = 0; i < tm_cnt; i++) {
    e[i].count = 0;
    Event_destroy(e[i].events);
    e[i].events = NULL;
  }
}

void EventDate_insert(struct EventDate *d, struct Event *e)
{
#define eventdate_index(d, tm) (d + (((tm) - tm_bop) / tm_day))
  d = eventdate_index(d, e->timestamp);
  d->count += Event_insert(&d->events, e);
}

void EventDate_add(struct EventDate *d, enum EventType type, int64_t timestamp, char *symbol, char *shortName)
{
  struct Event *e = Event_new(type, timestamp, symbol, shortName);
  EventDate_insert(d, e);
}

static void updateEvent(void *symbol _U_, void *quote, void *calendar)
{
  struct YQuote *q = quote;
#define event_contains(tm) (tm_bop <= (tm) && (tm) < tm_eop)
  if (event_contains(q->earningsTimestampStart)) {
    EventDate_add(calendar, EARNINGS, q->earningsTimestampStart, q->symbol, q->shortName);
  }
  if (event_contains(q->dividendDate)) {
    EventDate_add(calendar, DIVIDEND, q->dividendDate, q->symbol, q->shortName);
  }
}

static int maxy, maxx, begy, begx, cury, curx;

#define getallyx(win)                           \
  getmaxyx(win, maxy, maxx);                    \
  getbegyx(win, begy, begx);                    \
  getyx(win, cury, curx)

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
  mvwaddstrcp(win, cp, (y), (x) + (((w) - (1) * 2 - strlen(s)) / 2), s)

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

static PANEL *panels[CLIENT + 1];
static enum PanelType curpan = HELP;
static struct Spark *sparks[CLIENT + 1];

static PANEL *bigpan;
static WINDOW *w_err;
static WINDOW *w_nfo;
static WINDOW *w_cmd;
static YString srchs;
static int srchi = 0;

static struct iextp_config *config;
static GPtrArray *portfolios;
static int curpfm = 0;
static struct EventDate calendar[tm_cnt];

#define setnext(i, n) (i = ((i) + 1) % (n))
#define setprev(i, n) (i = ((i) + ((n) - 1)) % (n))

#define getcurrpan()  (panels[curpan])
#define setnextpan()  (setnext(curpan, CLIENT + 1))
#define setprevpan()  (setprev(curpan, CLIENT + 1))
#define getcurrwin()  (panels[curpan]->win)
#define getcurrspk()  (sparks[curpan])

#define getcurrpfm()  (g_ptr_array_index(portfolios, curpfm))
#define setnextpfm()  (setnext(curpfm, portfolios->len))
#define setprevpfm()  (setprev(curpfm, portfolios->len))

static void prepaint()
{
  update_panels();
  doupdate();
}

static void repaint()
{
  wnoutrefresh(stdscr);
  touchwin(panels[curpan]->win);
  wnoutrefresh(panels[curpan]->win);
  prepaint();
}

static void paint(enum PanelType newpan)
{
  hide_panel(panels[curpan]);
  curpan = newpan;
  show_panel(panels[curpan]);
  prepaint();
}

static void reset(enum PanelType newpan)
{
  curpan = newpan;
  wclear(panels[curpan]->win);
  void Spark_print(struct Spark *);
  Spark_print(sparks[curpan]);
}

static WINDOW *wbox_new(int n)
{
  getallyx(stdscr);
  double h = maxy / n, w = maxx / n, m = (n - 1.0) / (2.0 * n);
  return newwin(h, w, begy + maxy * m, begx + maxx * m);
}

static bool getch_go()
{
  while (1) {
    switch (getch()) {
    case ASKEY_ENTER:
      return true;
    case ASKEY_ESC:             /* ESCDELAY */
    case 'Q':
    case 'q':
      return false;
    }
  }
  /* __builtin_unreachable(); */
  return false;
}

static void mpaint(PANEL *p)
{
  hide_panel(panels[curpan]);
  show_panel(p);
  prepaint();
  getch_go();
  hide_panel(p);
  show_panel(panels[curpan]);
  prepaint();
}

static void wprint_alert(WINDOW *win, const char *resp, const char *code, const char *desc, const char *symbol)
{
  getallyx(win);
  int y = MARGIN_Y, x = MARGIN_X;

  wclear(win);
  box(win, 0, 0);
  mvwaddstrcn(win, COLOR_PAIR_ERROR, y++, x, maxx, code);
  mvwhline(win, y++, x, ACS_HLINE, maxx - x * 2);
  mvwaddstrcp(win, COLOR_PAIR_KEY, y, x, "symbol: ");
  waddstr(win, symbol);
  mvwprintwcp(win, COLOR_PAIR_KEY, ++y, x, "%s.description: ", resp);
  mvwaddstr(win, ++y, MARGIN_X + x, desc);
  mvwaddstrcn(win, COLOR_PAIR_GO    , maxy - 2, x + 0 * maxx / 2, maxx / 2, TERM_GO);
  mvwaddstrcn(win, COLOR_PAIR_CANCEL, maxy - 2, x + 1 * maxx / 2, maxx / 2, TERM_CANCEL);
  wrefresh(win);
  getch_go();
}

static void wprint_error(WINDOW *win, struct YError *err, const char *symbol)
{
  wprint_alert(win, err->response, err->code, err->description, symbol);
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
  mvwaddstrcp(win, cp, y++, x, "[<S>]-<TAB>          Previous / next portfolio");
  mvwaddstrcp(win, cp, y++, x, "hjkl                 Previous / next page");
  mvwaddstrcp(win, cp, y++, x, "E                    Events calendar");
  mvwaddstrcp(win, cp, y++, x, "H                    Historical data");
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

static void wprint_spark(WINDOW *win, const GPtrArray *p)
{
  getallyx(win);
  int y = MARGIN_Y + 3, x = MARGIN_X * 2, w = maxx / SparkLayoutC, cp = COLOR_PAIR_DEFAULT;

  size_t n = min(p->len, maxy - y - MARGIN_Y * 2);
  for (size_t i = y, j = 0; i < y + n && j < n; i++, j++) {
    struct YQuote *q = g_hash_table_lookup(quotes, g_ptr_array_index(p, j));
    if (q) {
      cp = COLOR_PAIR_TITLE;
      mvwprintvcp(win, cp, i, x, w, SparkLayout[0], q->shortName);
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

static void wprint_client(WINDOW *win)
{
  getallyx(win);
  int y = MARGIN_Y + 1, x = MARGIN_X, w = maxx / ClientLayoutC, cp = COLOR_PAIR_DEFAULT;

  struct Portfolio *p = getcurrpfm();
  size_t n = min(p->positions->len, maxy - y - MARGIN_Y * 2 - 2);
  size_t m = max(p->positions->len - n, 0);
  for (size_t i = m; i < m + n; i++, y++) {
    struct Position *q = g_ptr_array_index(p->positions, i);
    cp = i % 2 ? COLOR_PAIR_BLUE : COLOR_PAIR_YELLOW;
    mvwprintvcp(win, cp, y, x, w, ClientLayout[0], q->symbol);
    mvwprintvcp(win, cp, y, x, w, ClientLayout[4], q->quantity);
    mvwprintvcp(win, cp, y, x, w, ClientLayout[5], q->price);
    cp = COLOR_PAIR_CHANGE(q->change);
    mvwprintvcp(win, cp, y, x, w, ClientLayout[1], q->last);
    mvwprintvcp(win, cp, y, x, w, ClientLayout[2], q->change);
    mvwprintvcp(win, cp, y, x, w, ClientLayout[3], q->changePercent);
    mvwprintvcp(win, cp, y, x, w, ClientLayout[6], q->daysGain);
    cp = COLOR_PAIR_CHANGE(q->last - q->price);
    mvwprintvcp(win, cp, y, x, w, ClientLayout[7], q->totalGain);
    mvwprintvcp(win, cp, y, x, w, ClientLayout[8], q->totalGainPercent);
    mvwprintvcp(win, cp, y, x, w, ClientLayout[9], q->value);
  }
  mvwhline(win, ++y, x, ACS_HLINE, maxx - MARGIN_X * 2);
  mvwprintval(win, y, x, w, ClientLayout[0], "TOTAL");
  mvwprintval(win, y, x, w, ClientLayout[5], p->totalPrice);
  mvwprintval(win, y, x, w, ClientLayout[6], p->totalDaysGain);
  mvwprintval(win, y, x, w, ClientLayout[7], p->totalGain);
  mvwprintval(win, y, x, w, ClientLayout[8], p->totalGainPercent);
  mvwprintval(win, y, x, w, ClientLayout[9], p->totalValue);
}

static void wprint_risk(WINDOW *win)
{
  getallyx(win);
  int y = MARGIN_Y, x = MARGIN_X, w = maxx / 14;

  struct Portfolio *p = getcurrpfm();
  mvwprintval(win, y, x, w, RiskLayout[0], p->alpha);
  mvwprintval(win, y, x, w, RiskLayout[1], p->beta);
  mvwprintval(win, y, x, w, RiskLayout[2], p->meanAnnualReturn);
  mvwprintval(win, y, x, w, RiskLayout[3], p->rsquared);
  mvwprintval(win, y, x, w, RiskLayout[4], p->stddev);
  mvwprintval(win, y, x, w, RiskLayout[5], p->sharpe);
  mvwprintval(win, y, x, w, RiskLayout[6], p->treynor);
}

static void wprintp_hist(PANEL *pan)
{
  WINDOW *win = pan->win;
  struct Spark *s = getcurrspk();
  if (s->cursym->len) {
    if (yql_chart(s->cursym->str) != -1) {
      struct YChart *c = g_hash_table_lookup(charts, s->cursym->str);
      if (c) {
        wclear(win);
        wprintwbox(win, 3, 2, ChartLayoutC, "Historical Data", ChartLayout, ChartLayoutN);
        wprint_chart(win, c);
        mpaint(pan);
      } else {
        wprint_alert(w_err, "hist", "Internal error", "No data found", s->cursym->str);
      }
    } else {
      wprint_error(w_err, &y_error, s->cursym->str);
    }
  } else {
    wprint_alert(w_err, "hist", "User error", "No symbol found: {<F(n)> H}", "");
  }
}

static void wprintp_event(PANEL *pan, struct EventDate *cal)
{
  WINDOW *win = pan->win;
  getallyx(win);
  int y = MARGIN_Y, x = MARGIN_X, cp = COLOR_PAIR_DEFAULT;

  wclear(win);
  box(win, 0, 0);
  mvwaddstrcp(win, COLOR_PAIR_TITLE, y++, x, "Events Calendar");
  waddstr(win, " (");
  waddstrcp(win, COLOR_PAIR_DIVIDEND, "Dividend");
  waddstr(win, ", ");
  waddstrcp(win, COLOR_PAIR_EARNINGS, "Earnings");
  waddstr(win, ", ");
  waddstrcp(win, COLOR_PAIR_SPLIT, "Split");
  waddstr(win, ")");

  int n = maxy - y - MARGIN_Y * 2, c = 0;
  void _flow()
  {                             /* TODO */
    if (y >= n && c < 3) {
      y = MARGIN_Y + 1, x = ++c * (maxx / 3);
    }
  }

  g_hash_table_foreach(quotes, updateEvent, cal);
  for (int i = 0; i < tm_cnt; i++) {
    int64_t dt = cal[i].date;
    struct Event *e = cal[i].events;
    if (e) {
      _flow();
      cp = COLOR_PAIR_KEY | (dt >= tm_bod && dt < tm_bod + tm_day ? A_REVERSE : 0);
      mvwaddstrcp(win, cp, ++y, x + MARGIN_X * 1, strwdate(dt));
      while (e) {
        _flow();
        cp = COLOR_PAIR_EVENT(e->type);
        mvwprintwcp(win, cp, ++y, x + MARGIN_X * 2, "%-8s %-32s", e->symbol, e->shortName);
        wprintw(win, "%s", strptime(e->timestamp));
        e = e->next;
      }
    }
  }
  mpaint(pan);
}

void Spark_init(struct Spark *s)
{
  WINDOW *win = panels[s->pan]->win;
  getallyx(win);

  s->cursym = g_string_sized_new(8);
  s->query = g_string_sized_new(8 * 4);
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
    g_string_assign(s->cursym, "GME");

    s->w_quote         = subwin(win, maxy / 2, maxx / 2, begy + 0 * maxy / 2, begx + 0 * maxx / 2);
    s->w_options       = subwin(win, maxy / 2, maxx / 2, begy + 1 * maxy / 2, begx + 0 * maxx / 2);
    s->w_chart         = subwin(win, maxy / 3, maxx / 2, begy + 0 * maxy / 3, begx + 1 * maxx / 2);
    s->w_assetProfile  = subwin(win, maxy / 3, maxx / 2, begy + 1 * maxy / 3, begx + 1 * maxx / 2);
    s->w_keyStatistics = subwin(win, maxy / 3, maxx / 2, begy + 2 * maxy / 3, begx + 1 * maxx / 2);
    break;
  case CMDTY:
    s->symbols = config->g_cmdty;
    goto SPARK_INIT;
  case INDEX:
    s->symbols = config->g_index;
    goto SPARK_INIT;
  case CRNCY:
    s->symbols = config->g_crncy;
  SPARK_INIT:
    if (s->symbols->len) {
      g_string_assign(s->cursym, g_ptr_array_index(s->symbols, 0));
    }
    for (size_t i = 0; i < s->symbols->len; i++) {
      g_string_append_printf(s->query, "%s,", (char *) g_ptr_array_index(s->symbols, i));
    }

    s->w_quote = subwin(win, maxy / 2, maxx / 2, begy + 0 * maxy / 2, begx + 0 * maxx / 2);
    s->w_chart = subwin(win, maxy / 2, maxx / 2, begy + 0 * maxy / 2, begx + 1 * maxx / 2);
    s->w_spark = subwin(win, maxy / 2, maxx    , begy + 1 * maxy / 2, begx + 0 * maxx / 2);
    break;
  case CLIENT:
    s->w_spark   = subwin(win, maxy - 3, maxx, begy    , begx);
    s->w_options = subwin(win, 3       , maxx, begy + (maxy - 3), begx);
    break;
  }
}

void Spark_free(struct Spark *s)
{
  g_string_free(s->cursym, TRUE);
  g_string_free(s->query, TRUE);
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
    delwin(s->w_spark);
    delwin(s->w_options);
    break;
  }
  free(s);
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
    wprintwbox(s->w_spark, 1, 1, ClientLayoutC, NULL, ClientLayout, ClientLayoutN);
    wprintwbox(s->w_options, 1, 1, 7 * 2, NULL, RiskLayout, 7);

    getallyx(s->w_spark);
    mvwhline(s->w_spark, 2, MARGIN_X, ACS_HLINE, maxx - MARGIN_X * 2);
    break;
  }
}

static struct Spark *Spark_fromQuoteType(const char *q)
{
  if (IS_ECNQUOTE(q) || IS_EQUITY(q) || IS_ETF(q)) {
    return sparks[EQUITY];
  } else if (IS_FUTURE(q)) {
    return sparks[CMDTY];
  } else if (IS_INDEX(q)) {
    return sparks[INDEX];
  } else if (IS_CURRENCY(q) || IS_CRYPTO(q)) {
    return sparks[CRNCY];
  } else {
    return getcurrspk();
  }
}

int Spark_update(const char *symbol)
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
        } else {
          wprint_error(w_err, &y_error, symbol);
        }
        if (yql_options(symbol) != -1) {
          struct YOptionChain *o = g_hash_table_lookup(options, symbol);
          if (o) {
            wprint_options(s->w_options, o);
          }
        } else {
          wprint_error(w_err, &y_error, symbol);
        }
        /* updateEvent(cal, q); */
      }
      if (yql_chart(symbol) != -1) {
        struct YChart *c = g_hash_table_lookup(charts, symbol);
        if (c) {
          wprint_chart(s->w_chart, c);
        }
      } else {
        wprint_error(w_err, &y_error, symbol);
      }
      if (s->symbols) {
        if (yql_quote(s->query->str) != -1) {
          wprint_spark(s->w_spark, s->symbols);
        } else {
          wprint_error(w_err, &y_error, symbol);
        }
      }
    }
  } else {
    wprint_error(w_err, &y_error, symbol);
    return -1;
  }
  return 0;
}

void Spark_updateClient()
{
  struct Portfolio *p = getcurrpfm();
  if (yql_quote(p->query->str) != -1) {
    Portfolio_reset(p);
    for (unsigned int i = 0; i < p->positions->len; i++) {
      struct Position *r = g_ptr_array_index(p->positions, i);
      struct YQuote *q = g_hash_table_lookup(quotes, r->symbol);
      if (q) {
        Portfolio_update(p, i, q->regularMarketPrice, q->regularMarketChange, q->regularMarketChangePercent, q->regularMarketPreviousClose);
      }
    }
    struct Spark *s = sparks[CLIENT];
    reset(s->pan);
    wprint_client(s->w_spark);
    wprint_risk(s->w_options);
  }
}

void Spark_refresh()
{
  struct Spark *s = getcurrspk();
  if (s->cursym->len) {
    Spark_update(s->cursym->str);
  } else if (s->pan == CLIENT) {
    Spark_updateClient();
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
    case ASKEY_ENTER:
    ENTER:
      srchs[srchi] = 0;
      if (Spark_update(srchs) != -1) {
        g_string_assign(sparks[curpan]->cursym, srchs);
      }
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
    case ASKEY_ESC:
    ESC:
      wclear_cmd(win, TERM_PROMPT);
      return;
    case KEY_BACKSPACE:
    case ASKEY_BACKSPACE:
      if (srchi > 0) {
        srchs[--srchi] = 0;
        getyx(win, cury, curx);
        mvwaddch(win, cury, curx - 1, ' ');
        wmove(win, cury, curx - 1);
        wrefresh(win);
      }
      break;
    case '-':
    case '0' ... '9':
    case '=':
    case 'A' ... 'Z':
    case '^':
    case 'a' ... 'z':
      if (srchi < YSTRING_LENGTH - 1) {
        c = toupper(c);
        srchs[srchi++] = c;
        waddch(win, c);
        wrefresh(win);
      }
      break;
    }
  }
}

static void inittm()
{
  time_t tm = time(NULL);
  struct tm *stm = localtime(&tm);
  stm->tm_sec = 0, stm->tm_min = 0, stm->tm_hour = 0;
  tm_bod = mktime(stm);
  localtime_r(&tm_bod, &stm_bod);

  memcpy(stm, &stm_bod, sizeof(struct tm));
  stm->tm_mday -= stm->tm_wday - 1;
  tm_bow = mktime(stm);
  localtime_r(&tm_bow, &stm_bow);

  memcpy(stm, &stm_bod, sizeof(struct tm));
  stm->tm_mday = 1;
  tm_bom = mktime(stm);
  if (stm->tm_wday % 6 == 0) {
    stm->tm_mday += (stm->tm_wday + 6) / 6;
    tm_bom = mktime(stm);
  }

  tm_bop = tm_bow;
  memcpy(stm, &stm_bow, sizeof(struct tm));
  stm->tm_mday += tm_cnt;
  tm_eop = mktime(stm);
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
  bigpan = new_panel(newwin(maxy - 3 * 2, maxx - MARGIN_X * 2, begy + 3, MARGIN_X));
  w_err = wbox_new(4);
  w_nfo = derwin(stdscr, 3, maxx - MARGIN_X * 2, begy    , MARGIN_X);
  w_cmd = derwin(stdscr, 3, maxx - MARGIN_X * 2, maxy - 3, MARGIN_X);
  keypad(w_cmd, TRUE);
  for (int i = 0; i <= CLIENT; i++) {
    panels[i] = new_panel(newwin(maxy - 3 * 2, maxx - MARGIN_X * 2, begy + 3, MARGIN_X));
    sparks[i] = malloc(sizeof(struct Spark)), sparks[i]->pan = i;
  }
}

static void start()
{
  inittm();

  yql_init();
  yql_open();

  portfolios = Portfolios_new(config->g_pfs);
  EventDate_init(calendar);

  wprint_nfo(w_nfo);
  wprint_cmd(w_cmd);
  for (int i = 0; i <= CLIENT; i++) {
    Spark_init(sparks[i]);
    Spark_print(sparks[i]);
  }

  int c;
  while ((c = getch())) {
    switch (c) {
    case KEY_F(1):
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
    case ASKEY_TAB:
      setnextpfm();
      goto REFRESH;
    case ASKEY_STAB:
      setprevpfm();
      goto REFRESH;
    case '/':
      wsearch_cmd(w_cmd);
      break;
    case KEY_DOWN:
    case 'j':
    case KEY_RIGHT:
    case 'l':
      setnextpan();
      goto REFRESH;
    case KEY_UP:
    case 'k':
    case KEY_LEFT:
    case 'h':
      setprevpan();
      goto REFRESH;
    case 'E':
    case 'e':
      wprintp_event(bigpan, calendar);
      break;
    case 'H':
      wprintp_hist(bigpan);
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

  g_ptr_array_free(portfolios, TRUE);
  EventDate_free(calendar);

  clear();
  wrefresh(stdscr);
  delwin(bigpan->win);
  del_panel(bigpan);
  delwin(w_err);
  delwin(w_nfo);
  delwin(w_cmd);
  for (int i = 0; i <= CLIENT; i++) {
    Spark_free(sparks[i]);
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

  struct iextp_config c;
  iextp_config_open(&c, argc, argv);
  /* iextp_config_dump(&c); */
  config = &c;

  init();
  start();
  destroy();

  iextp_config_free(&c);

  return EXIT_SUCCESS;
}
