#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "../include/pm.h"

#define LINE_LENGTH 256
#define DELIM       ","
#define QUOTE       "\""

#define strtokn(n)                              \
  for (int i = 0; i < (n); i++) {               \
    strtok(NULL, DELIM);                        \
  }

#define IS_ETFC(s)        (strncmp(s, "Account Summary", 15) == 0 ||    \
                           strncmp(s, "Watch List Name", 15) == 0)
#define IS_ETFC_HEADER(s) (strncmp(s, "Symbol", 6) == 0)
#define IS_ETFC_EQUITY(s) (strnlen(s, 9) < 9)
#define IS_ETFC_FOOTER(s) (strncasecmp(s, "Cash", 4) == 0)
#define IS_JPMC_HEADER(s) (strncmp(s, "Asset Class", 11) == 0)
#define IS_JPMC_EQUITY(s) (strncmp(s, "Equity", 6) == 0)
#define IS_JPMC_FOOTER(s) (strncmp(s, "FOOTNOTES", 9) == 0)
#define IS_CSTM_SYMBOL(s) (s[0] != '\n' && s[0] != '#')

void Portfolio_loadETFC(struct Portfolio *p, FILE *stream)
{
  char line[LINE_LENGTH], *token;
  while (fgets(line, LINE_LENGTH, stream) && !IS_ETFC_HEADER(line));
  while (fgets(line, LINE_LENGTH, stream) && !IS_ETFC_FOOTER(line)) {
    token = strtok(line, DELIM);
    if (IS_ETFC_EQUITY(token)) {
#define g_ptr_array_last(p) (p->len ? g_ptr_array_index(p, p->len - 1) : NULL)
      struct Position *q = g_ptr_array_last(p->positions);
      if (!q || strncmp(token, q->symbol, 8) != 0) {
        q = Portfolio_add(p, token, 0, 0);
      }
      strtokn(3);
      q->quantity += atof(strtok(NULL, DELIM));
      q->price = atof(strtok(NULL, DELIM));
    }
  }
}

void Portfolio_loadJPMC(struct Portfolio *p, FILE *stream)
{
  char line[LINE_LENGTH], *token;
  while (fgets(line, LINE_LENGTH, stream) && !IS_JPMC_FOOTER(line)) {
    token = strtok(line, QUOTE);
    if (IS_JPMC_EQUITY(token)) {
      strtokn(3);
      token = strtok(NULL, QUOTE);
      strtokn(1);
      double quantity = atof(strtok(NULL, QUOTE));
      strtokn(13);
      double price = atof(strtok(NULL, QUOTE));
      Portfolio_add(p, token, quantity, price);
    }
  }
}

void Portfolio_loadCustom(struct Portfolio *p, FILE *stream)
{
  char line[LINE_LENGTH], *token;
  while (fgets(line, LINE_LENGTH, stream) && !IS_ETFC_HEADER(line));
  while (fgets(line, LINE_LENGTH, stream)) {
    token = strtok(line, DELIM);
    if (IS_CSTM_SYMBOL(token)) {
      double quantity = atof(strtok(NULL, DELIM)), price = atof(strtok(NULL, DELIM));
      Portfolio_add(p, token, quantity, price);
    }
  }
}

void Portfolio_load(struct Portfolio *p, const char *pathname)
{
  /* assert(pathname); */
  FILE *stream = fopen(pathname, "r");
  if (!stream) {
    perror("fopen");
    return;
  }
  char line[LINE_LENGTH];
  if (fgets(line, LINE_LENGTH, stream)) {
    if (IS_ETFC(line)) {
      Portfolio_loadETFC(p, stream);
    } else if (IS_JPMC_HEADER(line)) {
      Portfolio_loadJPMC(p, stream);
    } else {
      Portfolio_loadCustom(p, stream);
    }
  }
  fclose(stream);
}

void Position_init(struct Position *p, const char *symbol, double quantity, double price)
{
  memset(p, 0, sizeof(struct Position));
  p->symbol = strdup(symbol);
  p->quantity = quantity;
  p->price = price;
}

struct Position *Position_new(const char *symbol, double quantity, double price)
{
  struct Position *p = malloc(sizeof(struct Position));
  if (!p) {
    perror("malloc");
    return p;
  }
  Position_init(p, symbol, quantity, price);
  return p;
}

void Position_free(void *p)
{
  struct Position *q = p;
  if (q->symbol) {
    free(q->symbol);
    q->symbol = NULL;
    free(p);
  }
}

void Portfolio_init(struct Portfolio *p)
{
  memset(p, 0, sizeof(struct Portfolio));
  p->positions = g_ptr_array_new_full(8, Position_free);
  p->query = g_string_sized_new(8 * 4);
}

struct Portfolio *Portfolio_new()
{
  struct Portfolio *p = malloc(sizeof(struct Portfolio));
  if (!p) {
    perror("malloc");
    return p;
  }
  Portfolio_init(p);
  return p;
}

void Portfolio_free(void *p)
{
  struct Portfolio *q = p;
  if (q->positions) {
    g_ptr_array_free(q->positions, TRUE);
    q->positions = NULL;
    g_string_free(q->query, TRUE);
    q->query = NULL;
    free(p);
  }
}

void Portfolio_reset(struct Portfolio *p)
{
  p->totalPrice = 0;
  p->totalDaysGain = 0;
  p->totalGain = 0;
  p->totalGainPercent = 0;
  p->totalValue = 0;
}

struct Position *Portfolio_add(struct Portfolio *p, const char *symbol, double quantity, double price)
{
  struct Position *q = Position_new(symbol, quantity, price);
  g_ptr_array_add(p->positions, q);
  g_string_append_printf(p->query, "%s" DELIM, symbol);
  return q;
}

void Portfolio_update(struct Portfolio *p, int i, double price, double change, double changePercent, double previousClose)
{
  struct Position *q = g_ptr_array_index(p->positions, i);
  q->last = price;
  q->change = change;
  q->changePercent = changePercent;
  q->daysGain = (price - previousClose) * q->quantity;
  q->totalGain = (price - q->price) * q->quantity;
  q->totalGainPercent = q->price > 0 ? (price - q->price) / q->price * 100 : 0;
  q->value = price * q->quantity;

  p->totalPrice += q->price * q->quantity;
  p->totalDaysGain += q->daysGain;
  p->totalGain += q->totalGain;
  p->totalGainPercent = p->totalPrice > 0 ? p->totalGain / p->totalPrice * 100 : 0;
  p->totalValue += q->value;
}

GPtrArray *Portfolios_new(const GPtrArray *pathnames)
{
  GPtrArray *p = g_ptr_array_new_full(pathnames->len, Portfolio_free);
  for (size_t i = 0; i < pathnames->len; i++) {
    struct Portfolio *q = Portfolio_new();
    Portfolio_load(q, g_ptr_array_index(pathnames, i));
    if (q->positions->len > 0) {
      g_ptr_array_add(p, q);
    } else {
      Portfolio_free(q);
    }
  }
  return p;
}
