#include "../include/gamaterm.h"

struct Layout QuoteLayout[] = {
  { .key = "Prev Close"    , .fmt = "%'.2f"         , .keyy = 0 , .keyx = 0, .valy = 1 , .valx = 0 },
  { .key = "Open"          , .fmt = "%'.2f"         , .keyy = 0 , .keyx = 1, .valy = 1 , .valx = 1 },
  { .key = "Bid"           , .fmt = "%'.2f x %ld"   , .keyy = 0 , .keyx = 2, .valy = 1 , .valx = 2 },
  { .key = "Ask"           , .fmt = "%'.2f x %ld"   , .keyy = 0 , .keyx = 3, .valy = 1 , .valx = 3 },
  { .key = "Low"           , .fmt = "%'.2f"         , .keyy = 0 , .keyx = 4, .valy = 1 , .valx = 4 },
  { .key = "High"          , .fmt = "%'.2f"         , .keyy = 0 , .keyx = 5, .valy = 1 , .valx = 5 },
  { .key = "Volume"        , .fmt = "%'ld"          , .keyy = 0 , .keyx = 6, .valy = 1 , .valx = 6 },

  { .key = "50-D Average"  , .fmt = "%'.2f (%+.2f%)", .keyy = 3 , .keyx = 0, .valy = 4 , .valx = 0 },
  { .key = ""              , .fmt = ""              , .keyy = 3 , .keyx = 1, .valy = 4 , .valx = 1 },
  { .key = "200-D Average" , .fmt = "%'.2f (%+.2f%)", .keyy = 3 , .keyx = 2, .valy = 4 , .valx = 2 },
  { .key = ""              , .fmt = ""              , .keyy = 3 , .keyx = 3, .valy = 4 , .valx = 3 },
  { .key = "52-W Low"      , .fmt = "%'.2f"         , .keyy = 3 , .keyx = 4, .valy = 4 , .valx = 4 },
  { .key = "52-W High"     , .fmt = "%'.2f"         , .keyy = 3 , .keyx = 5, .valy = 4 , .valx = 5 },
  { .key = ""              , .fmt = ""              , .keyy = 3 , .keyx = 6, .valy = 4 , .valx = 6 },

  { .key = "Market Cap"    , .fmt = "%'ld"          , .keyy = 6 , .keyx = 0, .valy = 7 , .valx = 0 },
  { .key = ""              , .fmt = ""              , .keyy = 6 , .keyx = 1, .valy = 7 , .valx = 1 },
  { .key = "P/E (TTM)"     , .fmt = "%.2f"          , .keyy = 6 , .keyx = 2, .valy = 7 , .valx = 2 },
  { .key = "EPS (TTM)"     , .fmt = "%.2f"          , .keyy = 6 , .keyx = 3, .valy = 7 , .valx = 3 },
  { .key = "Beta"          , .fmt = "%.2f"          , .keyy = 6 , .keyx = 4, .valy = 7 , .valx = 4 },
  { .key = "10-D Volume"   , .fmt = "%'ld"          , .keyy = 6 , .keyx = 5, .valy = 7 , .valx = 5 },
  { .key = "3-M Volume"    , .fmt = "%'ld"          , .keyy = 6 , .keyx = 6, .valy = 7 , .valx = 6 },

  { .key = "Shares Outstanding" , .fmt = "%'ld"     , .keyy = 9 , .keyx = 0, .valy = 10, .valx = 0 },
  { .key = ""              , .fmt = ""              , .keyy = 9 , .keyx = 1, .valy = 10, .valx = 1 },
  { .key = "Earnings Date" , .fmt = "%s"            , .keyy = 9 , .keyx = 2, .valy = 10, .valx = 2 },
  { .key = ""              , .fmt = ""              , .keyy = 9 , .keyx = 3, .valy = 10, .valx = 3 },
  { .key = "Dividend Date" , .fmt = "%s"            , .keyy = 9 , .keyx = 4, .valy = 10, .valx = 4 },
  { .key = ""              , .fmt = ""              , .keyy = 9 , .keyx = 5, .valy = 10, .valx = 5 },
  { .key = "Dividend"      , .fmt = "%.2f (%.2f%)"  , .keyy = 9 , .keyx = 6, .valy = 10, .valx = 6 },

  { .key = "Circulating Supply" , .fmt = "%'ld"     , .keyy = 12, .keyx = 0, .valy = 13, .valx = 0 },
  { .key = ""              , .fmt = ""              , .keyy = 12, .keyx = 1, .valy = 13, .valx = 1 },
  { .key = ""              , .fmt = ""              , .keyy = 12, .keyx = 2, .valy = 13, .valx = 2 },
  { .key = ""              , .fmt = ""              , .keyy = 12, .keyx = 3, .valy = 13, .valx = 3 },
  { .key = "24-H Volume"   , .fmt = "%'ld"          , .keyy = 12, .keyx = 4, .valy = 13, .valx = 4 },
  { .key = ""              , .fmt = ""              , .keyy = 12, .keyx = 5, .valy = 13, .valx = 5 },
  { .key = "24-H Volume All", .fmt = "%'ld"         , .keyy = 12, .keyx = 6, .valy = 13, .valx = 6 },
};

struct Layout AssetProfileLayout[] = {
  { .key = "Sector"             , .fmt = "%s"  , .keyy = 0, .keyx = 0, .valy = 0, .valx = 1 },
  { .key = "Industry"           , .fmt = "%s"  , .keyy = 1, .keyx = 0, .valy = 1, .valx = 1 },
  { .key = "Full-Time Employees", .fmt = "%'ld", .keyy = 2, .keyx = 0, .valy = 2, .valx = 1 },
};

struct Layout CompanyOfficerLayout[] = {
  { .key = "Name"     , .fmt = "%s"  , .keyy = 0, .keyx = 0, .valy = 1, .valx = 0 },
  { .key = "Title"    , .fmt = "%s"  , .keyy = 0, .keyx = 1, .valy = 1, .valx = 1 },
  { .key = "Pay"      , .fmt = "%'ld", .keyy = 0, .keyx = 2, .valy = 1, .valx = 2 },
  { .key = "Exercised", .fmt = "%'ld", .keyy = 0, .keyx = 3, .valy = 1, .valx = 3 },
  { .key = "Born"     , .fmt = "%'ld", .keyy = 0, .keyx = 4, .valy = 1, .valx = 4 },
};

struct Layout KeyStatisticsLayout[] = {
  { .key = "Beta (5Y Monthly)"   , .fmt = "%.2f"  , .keyy = 0 , .keyx = 0, .valy = 0 , .valx = 1 },
  { .key = "Book Value"          , .fmt = "%.2f"  , .keyy = 1 , .keyx = 0, .valy = 1 , .valx = 1 },
  { .key = "Earnings Growth (Q)" , .fmt = "%.2f%%", .keyy = 2 , .keyx = 0, .valy = 2 , .valx = 1 },
  { .key = "Enterprise / EBITDA" , .fmt = "%.2f"  , .keyy = 3 , .keyx = 0, .valy = 3 , .valx = 1 },
  { .key = "Enterprise Value"    , .fmt = "%'ld"  , .keyy = 4 , .keyx = 0, .valy = 4 , .valx = 1 },
  { .key = "Float Shares"        , .fmt = "%'ld"  , .keyy = 5 , .keyx = 0, .valy = 5 , .valx = 1 },
  { .key = "Forward EPS"         , .fmt = "%.2f"  , .keyy = 6 , .keyx = 0, .valy = 6 , .valx = 1 },
  { .key = "Forward P/E"         , .fmt = "%.2f"  , .keyy = 7 , .keyx = 0, .valy = 7 , .valx = 1 },
  { .key = "Held % Insiders"     , .fmt = "%.2f%%", .keyy = 8 , .keyx = 0, .valy = 8 , .valx = 1 },
  { .key = "Profit Margins"      , .fmt = "%.2f%%", .keyy = 9 , .keyx = 0, .valy = 9 , .valx = 1 },
  { .key = "Shares Outstanding"  , .fmt = "%'ld"  , .keyy = 10, .keyx = 0, .valy = 10, .valx = 1 },
  { .key = "Shares Short"        , .fmt = "%'ld"  , .keyy = 11, .keyx = 0, .valy = 11, .valx = 1 },
  { .key = "Short Ratio"         , .fmt = "%.2f"  , .keyy = 12, .keyx = 0, .valy = 12, .valx = 1 },

  { .key = "52-W Change"         , .fmt = "%.2f%%", .keyy = 0 , .keyx = 2, .valy = 0 , .valx = 3 },
  { .key = "Price / Book"        , .fmt = "%.2f"  , .keyy = 1 , .keyx = 2, .valy = 1 , .valx = 3 },
  { .key = "Enterprise / Revenue", .fmt = "%.2f"  , .keyy = 3 , .keyx = 2, .valy = 3 , .valx = 3 },
  { .key = "Short % of Float"    , .fmt = "%.2f%%", .keyy = 5 , .keyx = 2, .valy = 5 , .valx = 3 },
  { .key = "Trailing EPS"        , .fmt = "%.2f"  , .keyy = 6 , .keyx = 2, .valy = 6 , .valx = 3 },
  { .key = "PEG Ratio"           , .fmt = "%.2f"  , .keyy = 7 , .keyx = 2, .valy = 7 , .valx = 3 },
  { .key = "Held % Institutions" , .fmt = "%.2f%%", .keyy = 8 , .keyx = 2, .valy = 8 , .valx = 3 },
  { .key = "Net Income / Common" , .fmt = "%'ld"  , .keyy = 9 , .keyx = 2, .valy = 9 , .valx = 3 },
  { .key = "Shares % Shares Out" , .fmt = "%.2f%%", .keyy = 10, .keyx = 2, .valy = 10, .valx = 3 },
  { .key = "Shares Short (-1M)"  , .fmt = "%'ld"  , .keyy = 11, .keyx = 2, .valy = 11, .valx = 3 },
};

struct Layout ChartLayout[] = {
  { .key = "Timestamp", .fmt = "%s"   , .keyy = 0, .keyx = 0, .valy = 0, .valx = 0 },
  { .key = "Open"     , .fmt = "%'.2f", .keyy = 0, .keyx = 1, .valy = 0, .valx = 1 },
  { .key = "High"     , .fmt = "%'.2f", .keyy = 0, .keyx = 2, .valy = 0, .valx = 2 },
  { .key = "Low"      , .fmt = "%'.2f", .keyy = 0, .keyx = 3, .valy = 0, .valx = 3 },
  { .key = "Close"    , .fmt = "%'.2f", .keyy = 0, .keyx = 4, .valy = 0, .valx = 4 },
  { .key = "Adj Close", .fmt = "%'.2f", .keyy = 0, .keyx = 5, .valy = 0, .valx = 5 },
  { .key = "Volume"   , .fmt = "%'ld" , .keyy = 0, .keyx = 6, .valy = 0, .valx = 6 },
};

struct Layout OptionsLayout[] = {
  { .key = "Calls", .fmt = "%s", .keyy = 0, .keyx = 0, .valy = 0, .valx = 0 },
  { .key = "Puts" , .fmt = "%s", .keyy = 0, .keyx = 1, .valy = 0, .valx = 0 },
};

struct Layout OptionLayout[] = {
  { .key = "Change"    , .fmt = "%+6.2f (%+6.2f%%)", .keyy = 0, .keyx = 0, .valy = 0, .valx = 0 },
  { .key = "Last Price", .fmt = "%5.2f"            , .keyy = 0, .keyx = 1, .valy = 0, .valx = 1 },
  { .key = "Strike"    , .fmt = "%.2f"             , .keyy = 0, .keyx = 2, .valy = 0, .valx = 2 },
  { .key = "Last Price", .fmt = "%5.2f"            , .keyy = 0, .keyx = 3, .valy = 0, .valx = 3 },
  { .key = "Change"    , .fmt = "%+6.2f (%+6.2f%%)", .keyy = 0, .keyx = 4, .valy = 0, .valx = 4 },
};

struct Layout FullOptionLayout[] = {
  { .key = "Strike"       , .fmt = "%8.2f" , .keyy = 1, .keyx = 0, .valy = 2, .valx = 0 },
  { .key = "Bid"          , .fmt = "%8.2f" , .keyy = 1, .keyx = 1, .valy = 2, .valx = 1 },
  { .key = "Ask"          , .fmt = "%8.2f" , .keyy = 1, .keyx = 2, .valy = 2, .valx = 2 },
  { .key = "Last"         , .fmt = "%8.2f" , .keyy = 1, .keyx = 3, .valy = 2, .valx = 3 },
  { .key = "Mark"         , .fmt = "%8.2f" , .keyy = 1, .keyx = 4, .valy = 2, .valx = 4 },
  { .key = "Mark Chg"     , .fmt = "%+8.2f", .keyy = 1, .keyx = 5, .valy = 2, .valx = 5 },
  { .key = "Delta"        , .fmt = "%8.2f" , .keyy = 1, .keyx = 6, .valy = 2, .valx = 6 },
  { .key = "Vega"         , .fmt = "%8.2f" , .keyy = 1, .keyx = 7, .valy = 2, .valx = 7 },
  { .key = "Volume"       , .fmt = "%8.2f" , .keyy = 1, .keyx = 8, .valy = 2, .valx = 8 },
  { .key = "Open Interest", .fmt = "%8.2f" , .keyy = 1, .keyx = 9, .valy = 2, .valx = 9 },
};

struct Layout SparkLayout[] = {
  { .key = "Symbol"   , .fmt = "%s"               , .keyy = 0, .keyx = 0, .valy = 1, .valx = 0 },
  { .key = "Last"     , .fmt = "%'10.2f (%+.2f%%)", .keyy = 0, .keyx = 1, .valy = 1, .valx = 1 },
  { .key = "Open"     , .fmt = "%'10.2f"          , .keyy = 0, .keyx = 2, .valy = 1, .valx = 2 },
  { .key = "High"     , .fmt = "%'10.2f"          , .keyy = 0, .keyx = 3, .valy = 1, .valx = 3 },
  { .key = "Low"      , .fmt = "%'10.2f"          , .keyy = 0, .keyx = 4, .valy = 1, .valx = 4 },
  { .key = "Close"    , .fmt = "%'10.2f"          , .keyy = 0, .keyx = 5, .valy = 1, .valx = 5 },
  { .key = "Volume"   , .fmt = "%'ld"             , .keyy = 0, .keyx = 6, .valy = 1, .valx = 6 },
  { .key = "52-W Low" , .fmt = "%'10.2f"          , .keyy = 0, .keyx = 7, .valy = 1, .valx = 7 },
  { .key = "52-W High", .fmt = "%'10.2f"          , .keyy = 0, .keyx = 8, .valy = 1, .valx = 8 },
};

struct Layout ClientLayout[] = {
  { .key = "Symbol"      , .fmt = "%s"      , .keyy = 0, .keyx = 0, .valy = 1, .valx = 0 },
  { .key = "Last Price $", .fmt = "%'10.2f" , .keyy = 0, .keyx = 1, .valy = 1, .valx = 1 },
  { .key = "Change $"    , .fmt = "%'+10.2f", .keyy = 0, .keyx = 2, .valy = 1, .valx = 2 },
  { .key = "Change %"    , .fmt = "%'+10.2f", .keyy = 0, .keyx = 3, .valy = 1, .valx = 3 },
  { .key = "Quantity"    , .fmt = "%'10.4f" , .keyy = 0, .keyx = 4, .valy = 1, .valx = 4 },
  { .key = "Price Paid $", .fmt = "%'10.2f" , .keyy = 0, .keyx = 5, .valy = 1, .valx = 5 },
  { .key = "Day's Gain $", .fmt = "%'10.2f" , .keyy = 0, .keyx = 6, .valy = 1, .valx = 6 },
  { .key = "Total Gain $", .fmt = "%'10.2f" , .keyy = 0, .keyx = 7, .valy = 1, .valx = 7 },
  { .key = "Total Gain %", .fmt = "%'10.2f" , .keyy = 0, .keyx = 8, .valy = 1, .valx = 8 },
  { .key = "Value $"     , .fmt = "%'10.2f" , .keyy = 0, .keyx = 9, .valy = 1, .valx = 9 },
};
