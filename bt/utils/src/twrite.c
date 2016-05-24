#define LOG_TAG "TWRITE"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#ifdef TWRITE_TEST
#include <time.h>
#endif

#include "twrite.h"
#include "osi/include/log.h"

static void initNode(volatile BUFFER_NODE *node);
static void initQ(BUFFER_Q *bufferQ);
static void deinitQ(BUFFER_Q *bufferQ);
// incress buffer size, should only use when buffer fill
static void pushQ(BUFFER_Q *bufferQ);
// free empyt buffer node
static void popQ(BUFFER_Q *bufferQ);
static void *writeThread();

#define NUM_OF_TWFD 4
static int tw_fd_idx = 0;
static TWRITE_FD * tw_fds[NUM_OF_TWFD] = { NULL };
static int q_cnt = 0;

static void sig_handler(int signo) {
  tw_dbg("%s sig:%d", __func__, signo);

  if (signo == SIGUSR1) {
    tw_dbg("Received SIGUSR1\n");

    /* deinit all twrite thread when receiving a sigusr signal */
    int i = 0;
    while (tw_fd_idx > 0 && i < NUM_OF_TWFD) {
      if (tw_fds[i] != NULL) {
        int fd = tw_fds[i]->fd;
        twrite_deinit(tw_fds[i]);
        close(fd);
      }
      i++;
    }
    tw_dbg("i=%d", i);

    tw_dbg("twrite is down\n");
  }
}

TWRITE_FD *twrite_init(int fd)
{
    TWRITE_FD *twrite_fd = (TWRITE_FD *)malloc(sizeof(TWRITE_FD));
    if (NULL == twrite_fd) {
        tw_err("%s cannot malloc twrite_fd errno = %d (%s)",
                __func__, errno, strerror(errno));
        return NULL;
    }

    // Example for enabling signal handling
    //if (signal(SIGUSR1, sig_handler) == SIG_ERR) {
    //  tw_dbg("Will be unable to catch signals\n");
    //}

    initQ(&twrite_fd->bufferQ);
    twrite_fd->fd = fd;
    twrite_fd->writeThreadRun = 1;
    twrite_fd->error = 0;
    pthread_mutex_init(&twrite_fd->mtx, NULL);
    pthread_cond_init(&twrite_fd->writeCond, NULL);
    pthread_mutex_lock(&twrite_fd->mtx);

    if (0 != pthread_create(&twrite_fd->writeThread, NULL, writeThread,
                (void*)twrite_fd)) {
        tw_err("create write thread failed");
        free(twrite_fd);
        return NULL;
    }

    pthread_mutex_lock(&twrite_fd->mtx);
    pthread_mutex_unlock(&twrite_fd->mtx);

    if (tw_fd_idx < NUM_OF_TWFD) {
      tw_dbg("%s twrite_fd=0x%x", __func__, twrite_fd);
      tw_fds[tw_fd_idx] = twrite_fd;
      tw_fd_idx++;
    } else {
      tw_err("tw_fd_idx(%d) exceeds the value of NUM_OF_TWFD", tw_fd_idx);
    }

    return twrite_fd;
}

int twrite_deinit(TWRITE_FD *twrite_fd)
{
    int i = 0;

    if (NULL == twrite_fd) {
        tw_err("deinit with NULL fd");
        return -1;
    }
    if (0 == twrite_fd->writeThreadRun) { // if deinit twice ignore
        tw_err("sniffer thread already end");
        return -2;
    }
    twrite_fd->writeThreadRun = 0;
    if (pthread_cond_signal(&twrite_fd->writeCond)) {
        tw_err("deinit pthread_cond_signal failed");
    }
    pthread_join(twrite_fd->writeThread, NULL);
    deinitQ(&twrite_fd->bufferQ);
    free(twrite_fd);

    /* clear twrite entry equal to twrite_fd */
    while (tw_fd_idx > 0 && i < NUM_OF_TWFD) {
      if (tw_fds[i] == twrite_fd) {
        tw_dbg("%s twrite_fd=0x%x", __func__, twrite_fd);
        tw_fds[i] = NULL;
        tw_fd_idx--;
        break;
      }
      i++;
    }

    tw_dbg("write Thread end");
    return 0;
}

int twrite_write(TWRITE_FD *twrite_fd, const unsigned char *data, int len)
{
    const unsigned char *p, *e;
    volatile unsigned char *bufferEnd;
    volatile unsigned char *QTailEnd;
    volatile BUFFER_NODE **bQTail = &twrite_fd->bufferQ.bufferQTail;

    //tw_dbg("data len = %d", len);
    if (twrite_fd->error) {
        tw_err("write thread have error, ignore err=%s, errno=%d",
                strerror(twrite_fd->error), twrite_fd->error);
        return -1;
    }
    p = data;
    e = data + len;
    if (NULL == *bQTail) {
        pushQ(&twrite_fd->bufferQ);
    }
    // log longer than left buffer size
    while (RW(bQTail, buffer) + MAX_NODE_SIZE - RW(bQTail, end) < (e - p)) {
        bufferEnd = RW(bQTail, buffer) + MAX_NODE_SIZE;
        QTailEnd = RW(bQTail, end);
        while (QTailEnd != bufferEnd) { // memcpy
            *(QTailEnd++) = *(p++);
        }
        RW(bQTail, end) = QTailEnd;
        pushQ(&twrite_fd->bufferQ);
    }
    QTailEnd = RW(bQTail, end);
    while (e != p) { // memcpy
        *(QTailEnd++) = *(p++);
    }
    RW(bQTail, end) = QTailEnd;
    if (pthread_cond_signal(&twrite_fd->writeCond)) {
        tw_err("pthread_cond_signal failed");
        return -1;
    }
    return len;
}

static void initQ(BUFFER_Q *bufferQ)
{
    bufferQ->bufferQHead = malloc(sizeof(BUFFER_NODE));
    bufferQ->bufferQTail = bufferQ->bufferQHead;
    initNode(bufferQ->bufferQHead);
    return;
}

static void deinitQ(BUFFER_Q* bufferQ)
{
    while(bufferQ->bufferQHead->next) {
        popQ(bufferQ);
    }
    free((void*)bufferQ->bufferQHead);
    return;
}

static void initNode(volatile BUFFER_NODE *node)
{
    node->next = NULL;
    node->start = node->buffer;
    node->end = node->buffer;
}

static void popQ(BUFFER_Q *bufferQ)
{
    volatile BUFFER_NODE *tmp;
    volatile BUFFER_NODE **bQHead = &bufferQ->bufferQHead;
    if (NULL == *bQHead || NULL == RW(bQHead, next) ||
            RW(bQHead, end) > RW(bQHead, start)) {
        // keep one node in queue, and check if all buffer was wrote
        return;
    } else {
        tmp = *bQHead;
    }

    *bQHead = RW(bQHead, next);

    free((void*)tmp);
    q_cnt--;
    return;
}

static void pushQ(BUFFER_Q *bufferQ)
{
    volatile BUFFER_NODE **bQHead = &bufferQ->bufferQHead;
    volatile BUFFER_NODE **bQTail = &bufferQ->bufferQTail;
    BUFFER_NODE *node;
    node = malloc(sizeof(BUFFER_NODE));
    initNode(node);
    q_cnt++;

    if (NULL == *bQHead && NULL == *bQTail) {
        *bQHead = node;
        *bQTail = node;
    } else {
        RW(bQTail, next) = node;
        *bQTail = node;
    }
    return;
}

static void *writeThread(void *t_fd)
{
    TWRITE_FD *twrite_fd = (TWRITE_FD *)t_fd;
    volatile BUFFER_NODE **bQHead = &twrite_fd->bufferQ.bufferQHead;
    BUFFER_Q *bufferQ = &twrite_fd->bufferQ;
    int nWritten;
    int bytesToWrite;

    if (prctl(PR_SET_NAME, (unsigned long)"twrite_logout", 0, 0, 0) == -1) {
      tw_dbg("%s unable to set thread name: %s", __func__, strerror(errno));
    }

    tw_dbg("write Thread start");
#ifdef TWRITE_TEST
    srand(time(NULL));
#endif
    do {
        pthread_cond_wait(&twrite_fd->writeCond, &twrite_fd->mtx);
        tw_dbg("g Cond: q_cnt(%d)", q_cnt);
        popQ(bufferQ); // try to remove empty Node
        while (RW(bQHead, start) != RW(bQHead, end)) {
#ifdef TWRITE_TEST
            usleep(rand() % 1000000);
#endif
            nWritten = 0;
            bytesToWrite = RW(bQHead, end) - RW(bQHead, start);

            while (bytesToWrite > 0) {
                nWritten = write(twrite_fd->fd, (void*)RW(bQHead, start),
                        bytesToWrite);
                if (nWritten < 0) {
                    twrite_fd->error = errno;
                    tw_err("write failed : nWritten=%d, err=%s, errno=%d",
                            nWritten, strerror(errno), errno);
                    return NULL;
                }
                tw_dbg("%db wr fd=%d", nWritten, twrite_fd->fd);
                bytesToWrite -= nWritten;
                RW(bQHead, start) += nWritten;
            }
            if (RW(bQHead, start) == RW(bQHead, buffer) + MAX_NODE_SIZE) {
                popQ(bufferQ);
            }
        }
    } while (twrite_fd->writeThreadRun);

    tw_dbg("exit loop of writeThread");
    return NULL;
}
