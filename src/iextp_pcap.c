#define _GNU_SOURCE

#include <stdio.h>
#include <pcap/pcap.h>
#include <zlib.h>

#include "../include/config.h"
#include "../include/iextp_logger.h"
#include "../include/mcast.h"
#include "../include/sysmq.h"

#define ETHERNET_HEADER_LENGTH 14
#define IP_HEADER_LENGTH       20
#define UDP_HEADER_LENGTH      8
#define UDP_LENGTH_OFFSET      4

typedef struct {
  struct iextp_mcast   *mc;
  struct iextp_handler *h;
} user_t;

void callback(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes)
{
  /* Skip Ethernet header */
  u_char *p = (u_char *) bytes + ETHERNET_HEADER_LENGTH;

  /* Skip IP header */
  uint16_t n = (*p & 0x0F) << 2;
  if (n < IP_HEADER_LENGTH) {
    fprintf(stderr, "Invalid IP header length: %u\n", n);
    return;
  }
  p += n;

  /* Skip UDP header */
  n = ntohs(*((uint16_t *) (p + UDP_LENGTH_OFFSET)));
  if (n < UDP_HEADER_LENGTH) {
    fprintf(stderr, "Invalid UDP header length: %u\n", n);
    return;
  }
  p += UDP_HEADER_LENGTH;

  /* IEX-TP outbound segment */
  n -= UDP_HEADER_LENGTH;
  if (n < IEXTP_HEADER_LENGTH) {
    fprintf(stderr, "Invalid IEX-TP header length: %u\n", n);
    return;
  }
  if (h->caplen < n) {
    fprintf(stderr, "Incomplete IEX-TP outbound segment: %u / %u (%u)\n", h->caplen, n, n - h->caplen);
    return;
  }
  user_t *u = (user_t *) user;
  if (u->mc) {
    iextp_mcast_sendto(u->mc, p, n); /* Replay mUDP packet */
  }
  if (u->h) {
    iextp_segment(p, n, u->h);
  }
}

ssize_t cookie_read(void *cookie, char *buf, size_t size)
{
  return gzread((gzFile) cookie, (voidp) buf, (unsigned int) size);
}

int cookie_seek(void *cookie, off64_t *offset, int whence)
{
  return gzseek((gzFile) cookie, (z_off_t) offset, whence);
}

int cookie_close(void *cookie)
{
  return gzclose((gzFile) cookie);
}

FILE *gzopencookie(const char *path)
{
  gzFile cookie = gzopen(path, "r");
  cookie_io_functions_t io_funcs = {
    .read  = cookie_read,
    .write = NULL,
    .seek  = cookie_seek,
    .close = cookie_close
  };
  return fopencookie(cookie, "r", io_funcs);
}

int iextp_pcap_loop(const char *path, user_t *user)
{
  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_t *p = pcap_fopen_offline(gzopencookie(path), errbuf);
  if (!p) {
    fprintf(stderr, "pcap_fopen_offline: %s\n", errbuf);
    return -1;
  }
  if ((pcap_loop(p, -1, callback, (u_char *) user)) == PCAP_ERROR) {
    fprintf(stderr, "pcap_loop: %s\n", pcap_geterr(p));
    pcap_perror(p, "pcap_loop");
    pcap_close(p);
    return -1;
  }
  pcap_close(p);
  return 0;
}

int iextp_pcap_main(int argc, char *argv[])
{
  struct iextp_config c;
  iextp_config_open(&c, argc, argv);
  iextp_config_dump(&c);

  user_t u = { .mc = NULL, .h = NULL };

  struct iextp_mcast mc = { .address = c.address, .service = c.service };
  if (c.mcast) {
    iextp_mcast_init(&mc);
    iextp_mcast_send_open(&mc);
    u.mc = &mc;
  }

  struct iextp_filter filt = { .msgtypes = c.msgtypes };
  if (c.filter) {
    iextp_filter_init(&filt);
    u.h = iextp_handler_append(u.h, (struct iextp_handler *) &filt);
  }

  struct iextp_logger log = { .logpath = c.logpath_pcap };
  if (c.log) {
    iextp_logger_init(&log);
    iextp_logger_open(&log);
    u.h = iextp_handler_append(u.h, (struct iextp_handler *) &log);
  }

  struct iextp_sysmq mq = { .keypath = c.keypath, .keyid = c.keyid };
  if (c.sysmq) {
    iextp_sysmq_send_init(&mq);
    iextp_sysmq_send_open(&mq);
    u.h = iextp_handler_append(u.h, (struct iextp_handler *) &mq);
  }

  iextp_pcap_loop(c.pcappath, &u);

  if (c.mcast) {
    iextp_mcast_close(&mc);
  }

  if (c.sysmq) {
    iextp_sysmq_close(&mq);
  }

  if (c.log) {
    iextp_logger_close(&log);
  }

  iextp_config_free(&c);

  return 0;
}

int main(int argc, char *argv[])
{
  return iextp_pcap_main(argc, argv);
}
