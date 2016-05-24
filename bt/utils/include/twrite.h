#ifndef TWRITE_H
#define TWRITE_H

#include <pthread.h>

#define MAX_PATH 256
//#define MAX_NODE_SIZE (64 * 1024)
#define MAX_NODE_SIZE 1024


#define RW(pointer, member) ((*pointer)->member)

// log
//#if defined(ANDROID) || defined(__ANDROID__)
// android
//#include <utils/Log.h>
//#include <android/log.h>
#define tw_dbg LOG_DEBUG
#define tw_err LOG_ERROR
//#else
// other
//#define tw_dbg(format, ...) fprintf(stdout, format "\n", ##__VA_ARGS__)
//#define tw_err(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)
//#endif

typedef struct _BUFFER_NODE {
    struct _BUFFER_NODE *next;
    unsigned char buffer[MAX_NODE_SIZE];
    volatile unsigned char *start, *end;
} BUFFER_NODE;

typedef struct _BUFFER_Q {
    volatile BUFFER_NODE *bufferQHead;
    volatile BUFFER_NODE *bufferQTail;
} BUFFER_Q;

typedef struct _TWRITE_FD {
    int fd;
    int writeThreadRun;
    int error;
    BUFFER_Q bufferQ;

    pthread_t writeThread;
    pthread_cond_t writeCond;
    pthread_mutex_t mtx;
} TWRITE_FD;

TWRITE_FD *twrite_init(int fd);
int twrite_deinit(TWRITE_FD *twrite_fd);
int twrite_write(TWRITE_FD *twrite_fd, const unsigned char *data, int len);

#endif//TWRITE_H
