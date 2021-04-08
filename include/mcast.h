#pragma once
#ifndef IEXTP_MCAST_H
#define IEXTP_MCAST_H

#include <netinet/in.h>

#include "iextp_handler.h"

struct iextp_mcast
{
  char *address;
  char *service;

  int socket;
  union {
    struct sockaddr dest;
    struct sockaddr_in addr;
  };
  size_t addrlen;
  struct ip_mreq mreq;
};

int iextp_mcast_init(struct iextp_mcast *);
int iextp_mcast_close(struct iextp_mcast *);

int iextp_mcast_send_open(struct iextp_mcast *);
int iextp_mcast_sendto(struct iextp_mcast *, const void *, size_t);

int iextp_mcast_recv_open(struct iextp_mcast *);
int iextp_mcast_recvfrom(struct iextp_mcast *, void *, size_t);
int iextp_mcast_recv_loop(struct iextp_mcast *, struct iextp_handler *);
int iextp_mcast_recv_close(struct iextp_mcast *);

#endif
