#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "../include/mcast.h"

static const int one = 1;

int iextp_mcast_init(struct iextp_mcast *mc)
{
  assert(mc->address);
  assert(mc->service);

  memset(&mc->addr, 0, sizeof(mc->addr));
  mc->addr.sin_family = AF_INET;
  inet_pton(AF_INET, mc->address, (struct in_addr *) &mc->addr.sin_addr.s_addr);
  mc->addr.sin_port = htons(atoi(mc->service));
  mc->addrlen = sizeof(mc->addr);
  /* iextp_mcast_getaddrinfo(mc); */

  if ((mc->socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
    perror("socket");
    return -1;
  }
  if (setsockopt(mc->socket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) == -1) {
    perror("setsockopt(SO_REUSEADDR)");
  }
  return mc->socket;
}

void iextp_mcast_getaddrinfo(struct iextp_mcast *mc)
{
  struct addrinfo hints, *res, *p;
  int errcode;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  if ((errcode = getaddrinfo(mc->address, mc->service, &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
    return;
  }
  for (p = res; p != NULL; p = p->ai_next) {
    /* struct sockaddr_in *addr = (struct sockaddr_in *) p->ai_addr; */
    /* char addrstr[INET_ADDRSTRLEN]; */
    /* inet_ntop(p->ai_family, &addr->sin_addr, addrstr, sizeof(addrstr)); */
    /* printf("IPv4 Address: %s:%d\n", addrstr, ntohs(addr->sin_port)); */
    mc->dest = *p->ai_addr;
    mc->addrlen = p->ai_addrlen;
    break;
  }
  if (!p) {
    fprintf(stderr, "getaddrinfo: %s:%s\n", mc->address, mc->service);
  }
  freeaddrinfo(res);
}

int iextp_mcast_send_open(struct iextp_mcast *mc)
{
  if (setsockopt(mc->socket, IPPROTO_IP, IP_MULTICAST_TTL, &one, sizeof(one)) == -1) {
    perror("setsockopt(IP_MULTICAST_TTL)");
  }
  if (setsockopt(mc->socket, IPPROTO_IP, IP_MULTICAST_LOOP, &one, sizeof(one)) == -1) {
    perror("setsockopt(IP_MULTICAST_LOOP)");
  }
  if (connect(mc->socket, (struct sockaddr *) &mc->addr, mc->addrlen) == -1) {
    perror("connect");
    iextp_mcast_close(mc);
    return -1;
  }
  return 0;
}

int iextp_mcast_sendto(struct iextp_mcast *mc, const void *p, size_t n)
{
  size_t i = 0;
  ssize_t m = 0;

  while (i < n) {
    /* if ((m = sendto(..., (struct sockaddr *) &mc->addr, mc->addrlen)) == -1) { */
    if ((m = send(mc->socket, p + i, n - i, 0)) == -1) {
      perror("send");
      return -i;
    }
    i += m;
  }

  return i;
}

int iextp_mcast_bind(struct iextp_mcast *mc)
{
  mc->addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(mc->socket, (struct sockaddr *) &mc->addr, mc->addrlen) == -1) {
    perror("bind");
    iextp_mcast_close(mc);
    return -1;
  }
  return 0;
}

int iextp_mcast_join(struct iextp_mcast *mc)
{
  inet_pton(AF_INET, mc->address, &mc->mreq.imr_multiaddr.s_addr);
  mc->mreq.imr_interface.s_addr = INADDR_ANY;
  if (setsockopt(mc->socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mc->mreq, sizeof(mc->mreq)) == -1) {
    perror("setsockopt(IP_ADD_MEMBERSHIP)");
    iextp_mcast_close(mc);
    return -1;
  }
  return 0;
}

int iextp_mcast_recv_open(struct iextp_mcast *mc)
{
  if (iextp_mcast_bind(mc) == -1) {
    return -1;
  }
  return iextp_mcast_join(mc);
}

int iextp_mcast_recvfrom(struct iextp_mcast *mc, void *p, size_t n)
{
  /* ssize_t m = recvfrom(..., (struct sockaddr *) &sender, &sender_len); */
  ssize_t m = recv(mc->socket, p, n, 0);
  if (m == -1) {
    perror("recv");
  }
  return m;
}

int iextp_mcast_recv_loop(struct iextp_mcast *mc, struct iextp_handler *h)
{
  ssize_t m = 0;
  unsigned char buf[MTU];

  while ((m = recv(mc->socket, buf, MTU, 0)) > -1) {
    iextp_segment(buf, m, h);
  }

  return m;
}

int iextp_mcast_drop(struct iextp_mcast *mc)
{
  if (setsockopt(mc->socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mc->mreq, sizeof(mc->mreq)) == -1) {
    perror("setsockopt(IP_DROP_MEMBERSHIP)");
    iextp_mcast_close(mc);
    return -1;
  }
  return 0;
}

int iextp_mcast_close(struct iextp_mcast *mc)
{
  if (mc->socket >= 0) {
    if ((mc->socket = close(mc->socket)) == -1) {
      perror("close");
      return -1;
    }
    mc->socket = -1;
    return 0;
  }
  return 1;
}

int iextp_mcast_recv_close(struct iextp_mcast *mc)
{
  if (iextp_mcast_drop(mc) == -1) {
    return -1;
  }
  return iextp_mcast_close(mc);
}
