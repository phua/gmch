#pragma once
#ifndef IEXTP_CONFIG_H
#define IEXTP_CONFIG_H

#include <stdbool.h>
#include <gmodule.h>

#include "iextp_handler.h"

#define DEBUG(f, ...)                           \
  fprintf(stderr, f, __VA_ARGS__)

typedef struct
{
  uint16_t digitcut;
  uint64_t alphacut;
} bitset_t;

struct iextp_filter
{
  struct iextp_handler;

  bitset_t msgtypes;
};

int iextp_filter_init(struct iextp_filter *);

struct iextp_config
{
  char *confpath;

  bool     filter;
  bitset_t msgtypes;
  bool     log;
  char    *logpath;
  char    *logpath_pcap;
  char    *logpath_live;
  bool     db;
  char    *dbpath;
  bool     sysmq;
  char    *keypath;
  int      keyid;
  bool     mcast;
  char    *address;
  char    *service;
  char    *pcappath;
  char    *token;

  GPtrArray *g_equity;
  GPtrArray *g_cmdty;
  GPtrArray *g_index;
  GPtrArray *g_crncy;
  GPtrArray *g_pfs;
};

int iextp_config_init(struct iextp_config *);
int iextp_config_open(struct iextp_config *, int, char **);
int iextp_config_free(struct iextp_config *);

void iextp_config_dump(const struct iextp_config *);

#endif
