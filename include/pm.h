#pragma once

#include <gmodule.h>

struct Position
{
  char  *symbol;
  double quantity;
  double price;

  double last;
  double change;
  double changePercent;
  double daysGain;
  double totalGain;
  double totalGainPercent;
  double value;
};

struct Portfolio
{
  GPtrArray *positions;         /*< struct Position * */
  GString *query;

  double totalPrice;
  double totalDaysGain;
  double totalGain;
  double totalGainPercent;
  double totalValue;

  double alpha;
  double beta;
  double meanAnnualReturn;
  double rsquared;
  double stddev;
  double sharpe;
  double treynor;
};

void Portfolio_init(struct Portfolio *);

void Portfolio_reset(struct Portfolio *);
struct Position *Portfolio_add(struct Portfolio *, const char *, double, double);
void Portfolio_update(struct Portfolio *, int, double, double, double, double);

GPtrArray *Portfolios_new(const GPtrArray *);
