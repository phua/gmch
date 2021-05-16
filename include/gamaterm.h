#pragma once
#ifndef GAMATERM_H
#define GAMATERM_H

#include <curses.h>
/* #include <panel.h> */

#include "yql.h"

#define TERM_NAME   "Γ - さま Terminal"
#define TERM_KEYS   "F1: HELP  F2: GOVT  F3: CORP  F4: MTGE  F5: M-MKT  F6: MUNI  F7: PFD  F8: EQUITY  F9: CMDTY  F10: INDEX  F11: CRNCY  F12: CLIENT"
#define TERM_TIME   "%a %b %d %H:%M:%S %Y"
#define TERM_PROMPT "GO>"
#define TERM_GO     "[ GO ]"
#define TERM_CANCEL "[ CANCEL ]"

#define ASKEY_TAB       011
#define ASKEY_ENTER     015
#define ASKEY_ESC       033
/* #define ASKEY_MENU      071 */
#define ASKEY_BACKSPACE 0177
#define ASKEY_STAB      0541

#define MARGIN_Y 1
#define MARGIN_X 2

#define COLOR_PAIR_BLACK   COLOR_PAIR(COLOR_BLACK)
#define COLOR_PAIR_RED     COLOR_PAIR(COLOR_RED)
#define COLOR_PAIR_GREEN   COLOR_PAIR(COLOR_GREEN)
#define COLOR_PAIR_YELLOW  COLOR_PAIR(COLOR_YELLOW)
#define COLOR_PAIR_BLUE    COLOR_PAIR(COLOR_BLUE)
#define COLOR_PAIR_MAGENTA COLOR_PAIR(COLOR_MAGENTA)
#define COLOR_PAIR_CYAN    COLOR_PAIR(COLOR_CYAN)
#define COLOR_PAIR_WHITE   COLOR_PAIR(COLOR_WHITE)

#define COLOR_PAIR_CANCEL  COLOR_PAIR_RED
#define COLOR_PAIR_GO      COLOR_PAIR_GREEN

#define COLOR_PAIR_CLOSED  COLOR_PAIR_RED
#define COLOR_PAIR_POST    COLOR_PAIR_YELLOW
#define COLOR_PAIR_PRE     COLOR_PAIR_YELLOW
#define COLOR_PAIR_REGULAR COLOR_PAIR_GREEN

#define COLOR_PAIR_DIVIDEND COLOR_PAIR_GREEN
#define COLOR_PAIR_EARNINGS COLOR_PAIR_YELLOW
#define COLOR_PAIR_SPLIT    COLOR_PAIR_RED

#define COLOR_PAIR_DOWN    COLOR_PAIR_RED
#define COLOR_PAIR_UNCH    COLOR_PAIR_YELLOW
#define COLOR_PAIR_UP      COLOR_PAIR_GREEN

#define COLOR_PAIR_CMD     COLOR_PAIR_CYAN
#define COLOR_PAIR_DEFAULT COLOR_PAIR_WHITE
#define COLOR_PAIR_ERROR   COLOR_PAIR_RED
#define COLOR_PAIR_INFO    COLOR_PAIR_YELLOW
#define COLOR_PAIR_KEY     COLOR_PAIR_MAGENTA
#define COLOR_PAIR_LINK    COLOR_PAIR_BLUE
#define COLOR_PAIR_TITLE   COLOR_PAIR_CYAN

#define COLOR_PAIR_MARKET(m)                    \
  (IS_PRE(m) ? COLOR_PAIR_PRE :                 \
   IS_REGULAR(m) ? COLOR_PAIR_REGULAR :         \
   IS_POST(m) ? COLOR_PAIR_POST :               \
   COLOR_PAIR_CLOSED)

#define COLOR_PAIR_EVENT(e)                     \
  (((e) == DIVIDEND) ? COLOR_PAIR_DIVIDEND :    \
   ((e) == EARNINGS) ? COLOR_PAIR_EARNINGS :    \
   ((e) == SPLIT) ? COLOR_PAIR_SPLIT :          \
   COLOR_PAIR_DEFAULT)

#define COLOR_PAIR_CHANGE(c)                                            \
  ((c) < 0 ? COLOR_PAIR_DOWN : (c) > 0 ? COLOR_PAIR_UP : COLOR_PAIR_UNCH)

#define COLOR_PAIR_BOOL(b)                      \
  ((b) ? COLOR_PAIR_GREEN : COLOR_PAIR_RED)

#define IS_CLOSED(m)   (strncmp(m, "CLOSED", 6) == 0)
#define IS_POST(m)     (strncmp(m, "POST", 4) == 0)
#define IS_PRE(m)      (strncmp(m, "PRE", 3) == 0)
#define IS_REGULAR(m)  (strncmp(m, "REGULAR", 7) == 0)

#define IS_CRYPTO(q)   (strncmp(q, "CRYPTOCURRENCY", 14) == 0)
#define IS_CURRENCY(q) (strncmp(q, "CURRENCY", 8) == 0)
#define IS_ECNQUOTE(q) (strncmp(q, "ECNQUOTE", 8) == 0)
#define IS_EQUITY(q)   (strncmp(q, "EQUITY", 6) == 0)
#define IS_ETF(q)      (strncmp(q, "ETF", 3) == 0)
#define IS_FUTURE(q)   (strncmp(q, "FUTURE", 6) == 0)
#define IS_INDEX(q)    (strncmp(q, "INDEX", 5) == 0)

enum PanelType
{
  HELP, GOVT, CORP, MTGE, MMKT, MUNI, PFD, EQUITY, CMDTY, INDEX, CRNCY, CLIENT,
};

struct Spark
{
  enum PanelType pan;
  GPtrArray *symbols;           /*< char * */
  GString *cursym;
  GString *query;

  WINDOW *w_quote;
  WINDOW *w_assetProfile;
  WINDOW *w_keyStatistics;
  WINDOW *w_chart;
  WINDOW *w_options;
  WINDOW *w_spark;
};

struct EventDate
{
  int64_t date;
  size_t count;
  struct Event
  {
    struct Event *next;

    enum EventType
    {
      DIVIDEND, EARNINGS, SPLIT,
    } type;
    char   *shortName;
    char   *symbol;
    int64_t timestamp;
  } *events;
};

struct Layout
{
  char *key, *fmt;
  int keyy, keyx;
  int valy, valx;
};

#define QuoteLayoutN 35
#define QuoteLayoutC 7
extern struct Layout QuoteLayout[];

#define AssetProfileN 3
#define AssetProfileC 2
extern struct Layout AssetProfileLayout[];
extern struct Layout CompanyOfficerLayout[];
#define KeyStatisticsN 23
#define KeyStatisticsC 4
extern struct Layout KeyStatisticsLayout[];

#define ChartLayoutN 7
#define ChartLayoutC 7
extern struct Layout ChartLayout[];

extern struct Layout OptionsLayout[];
#define OptionLayoutN 5
#define OptionLayoutC 5
extern struct Layout OptionLayout[];
extern struct Layout FullOptionLayout[];

#define SparkLayoutN 9
#define SparkLayoutC 9
extern struct Layout SparkLayout[];

#define ClientLayoutN 10
#define ClientLayoutC 10
extern struct Layout ClientLayout[];
extern struct Layout RiskLayout[];

#endif
