#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <json-c/json.h>

#include "../include/config.h"
#include "../include/iextp_logger.h"

static int bitset_set(bitset_t *b, char c)
{
  if ('0' <= c && c <= '9')  {
    setbitf(b->digitcut, c - '0');
    return 0;
  }
  if ('A' <= c && c <= 'z')  {
    setbitf(b->alphacut, c - 'A');
    return 0;
  }
  return -1;
}

static int bitset_get(const bitset_t *b, char c)
{
  if ('0' <= c && c <= '9' && getbitf(b->digitcut, c - '0'))  {
    return c;
  }
  if ('A' <= c && c <= 'z' && getbitf(b->alphacut, c - 'A'))  {
    return c;
  }
  return -1;
}

#define F(T)                                            \
  static int on##T(const struct iextp_handler *h,       \
                   const struct IEXTP_##T *p)           \
  {                                                     \
    struct iextp_filter *f = (struct iextp_filter *) h; \
    return bitset_get(&f->msgtypes, p->msgtype) == -1;  \
  }
FTABLE
#undef F

int iextp_filter_init(struct iextp_filter *f)
{
#define F(T)                                    \
  f->on##T = on##T;
  FTABLE
#undef F
  return 0;
}

int iextp_config_init(struct iextp_config *c)
{
  assert(c->confpath);

  struct json_object *obj = NULL, *o = NULL, *p = NULL;

  if (!(obj = json_object_from_file(c->confpath))) {
    fprintf(stderr, "json_object_from_file: %s\n", json_util_get_last_err());
    return -1;
  }

  if (json_object_object_get_ex(obj, "iextp_filter", &o)) {
    if (json_object_object_get_ex(o, "enabled", &p)) {
      c->filter = (bool) json_object_get_int(p);
    }
    if (json_object_object_get_ex(o, "msgtypes", &p)) {
      c->msgtypes.digitcut = c->msgtypes.alphacut = 0;
      const char *s = json_object_get_string(p);
      for (int i = 0; i < json_object_get_string_len(p); i++) {
        bitset_set(&c->msgtypes, s[i]);
      }
    }
  }

  if (json_object_object_get_ex(obj, "iextp_logger", &o)) {
    if (json_object_object_get_ex(o, "enabled", &p)) {
      c->log = (bool) json_object_get_int(p);
    }
    if (json_object_object_get_ex(o, "logpath", &p)) {
      c->logpath = strndup(json_object_get_string(p), json_object_get_string_len(p));
    }
  }

  if (json_object_object_get_ex(obj, "iexdb_sqlite", &o)) {
    if (json_object_object_get_ex(o, "enabled", &p)) {
      c->db = (bool) json_object_get_int(p);
    }
    if (json_object_object_get_ex(o, "dbpath", &p)) {
      c->dbpath = strndup(json_object_get_string(p), json_object_get_string_len(p));
    }
  }

  if (json_object_object_get_ex(obj, "iextp_sysmq", &o)) {
    if (json_object_object_get_ex(o, "enabled", &p)) {
      c->sysmq = (bool) json_object_get_int(p);
    }
    if (json_object_object_get_ex(o, "keypath", &p)) {
      c->keypath = strndup(json_object_get_string(p), json_object_get_string_len(p));
    }
    if (json_object_object_get_ex(o, "keyid", &p)) {
      c->keyid = json_object_get_int(p);
    }
  }

  if (json_object_object_get_ex(obj, "iextp_mcast", &o)) {
    if (json_object_object_get_ex(o, "enabled", &p)) {
      c->mcast = (bool) json_object_get_int(p);
    }
    if (json_object_object_get_ex(o, "address", &p)) {
      c->address = strndup(json_object_get_string(p), json_object_get_string_len(p));
    }
    if (json_object_object_get_ex(o, "service", &p)) {
      c->service = strndup(json_object_get_string(p), json_object_get_string_len(p));
    }
  }

  if (json_object_object_get_ex(obj, "iextp_pcap", &o)) {
    if (json_object_object_get_ex(o, "logpath", &p)) {
      c->logpath_pcap = strndup(json_object_get_string(p), json_object_get_string_len(p));
    }
    if (json_object_object_get_ex(o, "pcappath", &p)) {
      c->pcappath = strndup(json_object_get_string(p), json_object_get_string_len(p));
    }
  }

  if (json_object_object_get_ex(obj, "iextp_live", &o)) {
    if (json_object_object_get_ex(o, "logpath", &p)) {
      c->logpath_live = strndup(json_object_get_string(p), json_object_get_string_len(p));
    }
  }

  if (json_object_object_get_ex(obj, "iextp_snap", &o)) {
    if (json_object_object_get_ex(o, "token", &p)) {
      c->token = strndup(json_object_get_string(p), json_object_get_string_len(p));
    }
  }

  return 0;
}

void iextp_config_dump(const struct iextp_config *c)
{
  printf("confpath     : %s\n", c->confpath);
  printf("filter       : %d\n", c->filter);
  printf("msgtypes     : ");
  for (char i = '0'; i <= 'z'; i++) {
    if (bitset_get(&c->msgtypes, i) >= 0) {
      printf("%c ", i);
    }
  }
  printf("\n");
  printf("log          : %d\n", c->log);
  printf("logpath      : %s\n", c->logpath);
  printf("logpath_pcap : %s\n", c->logpath_pcap);
  printf("logpath_live : %s\n", c->logpath_live);
  printf("db           : %d\n", c->db);
  printf("dbpath       : %s\n", c->dbpath);
  printf("sysmq        : %d\n", c->sysmq);
  printf("keypath      : %s\n", c->keypath);
  printf("keyid        : %d\n", c->keyid);
  printf("mcast        : %d\n", c->mcast);
  printf("address      : %s\n", c->address);
  printf("service      : %s\n", c->service);
  printf("pcappath     : %s\n", c->pcappath);
  printf("token        : %s\n", c->token);
}

int iextp_config_open(struct iextp_config *c, int argc, char *argv[])
{
  char *usage = "Usage: %s [-c confpath] [-d dbpath] [-l logpath] [-m] [-p pcappath] [-q]\n";
  c->confpath = "config.json";
  if (argc < 2 && access(c->confpath, F_OK) == -1) {
    fprintf(stderr, usage, argv[0]);
    return -1;
  }
  int opt = 0, mcast = 0, sysmq = 0;
  char *dbpath = NULL, *logpath = NULL, *pcappath = NULL;
  while ((opt = getopt(argc, argv, "c:d:l:mp:q")) != -1) {
    switch (opt) {
    case 'c':
      c->confpath = optarg;
      break;
    case 'd':
      dbpath = optarg;
      break;
    case 'l':
      logpath = optarg;
      break;
    case 'm':
      mcast = 1;
      break;
    case 'p':
      pcappath = optarg;
      break;
    case 'q':
      sysmq = 1;
      break;
    }
  }
  if (access(c->confpath, F_OK) == -1) {
    fprintf(stderr, usage, argv[0]);
    return -1;
  }
  iextp_config_init(c);
  if (dbpath) {
    c->db = true;
    c->dbpath = dbpath;
  }
  if (logpath) {
    c->log = true;
    c->logpath = logpath;
  }
  if (mcast) {
    c->mcast = true;
  }
  if (pcappath) {
    c->pcappath = pcappath;
  }
  if (sysmq) {
    c->sysmq = true;
  }
  return 0;
}

int iextp_config_free(struct iextp_config *c)
{
  if (c) {
    /* free(c->logpath); */
    free(c->logpath_pcap);
    free(c->logpath_live);
    /* free(c->dbpath); */
    free(c->address);
    free(c->service);
    free(c->keypath);
    /* free(c->pcappath); */
    free(c->token);
    return 0;
  }
  return 1;
}
