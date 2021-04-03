#pragma once
#ifndef IEXTP_LOGGER_H
#define IEXTP_LOGGER_H

#include <stdio.h>

#include "iextp_handler.h"

struct iextp_logger
{
  struct iextp_handler;         /* -fms-extensions */

  char *logpath;
  FILE *log;
};

int iextp_logger_init(struct iextp_logger *);
int iextp_logger_open(struct iextp_logger *);
int iextp_logger_close(struct iextp_logger *);
int iextp_logger_free(struct iextp_logger *);

#endif
