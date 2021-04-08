#pragma once
#ifndef IEXTP_SYSMQ_H
#define IEXTP_SYSMQ_H

#include <sys/types.h>

#include "iextp_handler.h"

struct iextp_sysmq
{
  struct iextp_handler;

  char *keypath;
  int keyid;

  int msqid;
  struct
  {
    long mtype;
    unsigned char mtext[MTU];
  } msgbuf;
};

int iextp_sysmq_open(struct iextp_sysmq *, int);
int iextp_sysmq_close(struct iextp_sysmq *);

int iextp_sysmq_send_init(struct iextp_sysmq *);
int iextp_sysmq_send_open(struct iextp_sysmq *);
int iextp_sysmq_send(struct iextp_sysmq *, const void *, size_t);

int iextp_sysmq_recv_open(struct iextp_sysmq *);
int iextp_sysmq_recv(struct iextp_sysmq *);
int iextp_sysmq_recv_loop(struct iextp_sysmq *, struct iextp_handler *);

#endif
