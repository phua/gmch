#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/msg.h>

#include "../include/sysmq.h"

#define F(T)                                                    \
  static int on##T(const struct iextp_handler *h,               \
                   const struct IEXTP_##T *p)                   \
  {                                                             \
    struct iextp_sysmq *mq = (struct iextp_sysmq *) h;          \
    mq->msgbuf.mtype = p->msgtype;                              \
    iextp_sysmq_send(mq, (void *) p, sizeof(struct IEXTP_##T)); \
    return 0;                                                   \
  }
FTABLE
#undef F

static int touch(const char *path)
{
#define S_IRWU 0600
  int fd = openat(AT_FDCWD, path, O_WRONLY|O_CREAT|O_NOCTTY|O_NONBLOCK, S_IRWU);
  if (fd == -1) {
    perror("openat");
    return -1;
  }
  if (utimensat(AT_FDCWD, path, NULL, 0) == -1) {
    perror("utimensat");
  }
  if (close(fd) == -1) {
    perror("close");
  }
  return 0;
}

int iextp_sysmq_open(struct iextp_sysmq *mq, int flg)
{
  assert(mq->keypath);
  assert(mq->keyid >= 0);

  key_t key;
  if ((key = ftok(mq->keypath, mq->keyid)) == -1) {
    perror("ftok");
    mq->msqid = -1;
    return -1;
  }
  if ((mq->msqid = msgget(key, S_IRWU | (flg ? IPC_CREAT : 0))) == -1) {
    perror("msgget");
    return -1;
  }
  mq->msgbuf.mtype = 1;
  return mq->msqid;
}

int iextp_sysmq_send_init(struct iextp_sysmq *mq)
{
  assert(mq->keypath);

#define F(T)                                    \
  mq->on##T = on##T;
  FTABLE
#undef F

  return touch(mq->keypath);
}

int iextp_sysmq_send_open(struct iextp_sysmq *mq) {
  return iextp_sysmq_open(mq, 1);
}

int iextp_sysmq_send(struct iextp_sysmq *mq, const void *p, size_t n)
{
  assert(mq->msqid >= 0);
  memcpy(mq->msgbuf.mtext, p, n);
  int m = msgsnd(mq->msqid, &mq->msgbuf, n, 0);
  if (m == -1) {
    perror("msgsnd");
  }
  return m;
}

int iextp_sysmq_recv_open(struct iextp_sysmq *mq) {
  return iextp_sysmq_open(mq, 0);
}

int iextp_sysmq_recv(struct iextp_sysmq *mq)
{
  assert(mq->msqid >= 0);
  ssize_t m = msgrcv(mq->msqid, &mq->msgbuf, MTU, 0, 0);
  if (m == -1) {
    perror("msgrcv");
  }
  return m;
}

int iextp_sysmq_recv_loop(struct iextp_sysmq *mq, struct iextp_handler *h)
{
  assert(mq->msqid >= 0);
  ssize_t m = 0;
  while ((m = msgrcv(mq->msqid, &mq->msgbuf, MTU, 0, 0)) > -1) {
    iextp_message(mq->msgbuf.mtype, mq->msgbuf.mtext, m, h);
  }
  return m;
}

int iextp_sysmq_close(struct iextp_sysmq *mq)
{
  if (mq->msqid >= 0) {
    if (msgctl(mq->msqid, IPC_RMID, NULL) == -1) {
      perror("msgctl(IPC_RMID)");
      return -1;
    }
    return 0;
  }
  return 1;
}
