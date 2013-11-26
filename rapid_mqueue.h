#ifndef __RAPID_MSG_QUEUE_H__
#define __RAPID_MSG_QUEUE_H__

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MSG_QUEUE_DEF_SIZE 4096
#define MSG_ITEM_SIZE 1024

enum {
    RAPID_SAMPLE_MSG = 0,
    RAPID_ERROR_MSG
};

//message header
typedef struct {
    int type;//message type
    int size;//message size
}rapid_msg_header_t;

typedef struct {
    rapid_msg_header_t header;
    char data[MSG_ITEM_SIZE];
}rapid_msg_item_t;

typedef struct {
    long next;
    long current;
    long cursor;
    long capacity;
}rapid_mqueue_header_t;

typedef struct {
    rapid_mqueue_header_t *header;
    void *pos;
}rapid_mqueue_t;

rapid_mqueue_t* rapid_mqueue_init(int key,int cap);
rapid_mqueue_t* rapid_mqueue_get(int key,int cap);
int rapid_mqueue_slot(rapid_mqueue_t *rq);
int rapid_mqueue_push(rapid_mqueue_t *rq,int slot,char *msg,int size,int type);
int rapid_mqueue_publish(rapid_mqueue_t *rq,int slot);
int rapid_mqueue_pop(rapid_mqueue_t *rq,void *msg,int *type);
void rapid_mqueue_destory(rapid_mqueue_t *rq);

#endif
