#include "../include/config.h"
#include "../include/iextp_logger.h"
#include "../include/mcast.h"
#include "../include/sysmq.h"

int main(int argc, char *argv[])
{
  struct iextp_config c;
  iextp_config_open(&c, argc, argv);
  iextp_config_dump(&c);

  struct iextp_handler *h = NULL;

  struct iextp_filter filt = { .msgtypes = c.msgtypes };
  if (c.filter) {
    iextp_filter_init(&filt);
    h = iextp_handler_append(h, (struct iextp_handler *) &filt);
  }

  struct iextp_logger log = { .logpath = c.logpath_live };
  if (c.log) {
    iextp_logger_init(&log);
    iextp_logger_open(&log);
    h = iextp_handler_append(h, (struct iextp_handler *) &log);
  }

  struct iextp_sysmq mq = { .keypath = c.keypath, .keyid = c.keyid };
  if (c.sysmq) {
    iextp_sysmq_recv_open(&mq);
    iextp_sysmq_recv_loop(&mq, h);
  }

  struct iextp_mcast mc = { .address = c.address, .service = c.service };
  if (c.mcast) {
    iextp_mcast_init(&mc);
    iextp_mcast_recv_open(&mc);
    iextp_mcast_recv_loop(&mc, h);
    iextp_mcast_recv_close(&mc);
  }

  if (c.log) {
    iextp_logger_close(&log);
  }

  iextp_config_free(&c);

  return 0;
}
