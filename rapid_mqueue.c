#include "rapid_mqueue.h"

//#define LOCK(q) while(__sync_lock_test_and_set(&(q)->lock,1)){}
//#define ULOCK(q) __sync_lock_release(&(q)->lock)

rapid_mqueue_t* rapid_mqueue_init(int key,int cap)
{
    int shmid = 0;
    char *ptr = NULL;
    size_t total = 0;
    rapid_mqueue_t *rq = NULL;
    
    if(cap < 0 || 
       cap < MSG_QUEUE_DEF_SIZE || 
       (cap&(cap-1)) ) 
    {
        cap = MSG_QUEUE_DEF_SIZE;
    }
    
    total = sizeof(rapid_mqueue_header_t) + (cap * sizeof(rapid_msg_item_t));
    shmid = shmget(key,total,IPC_CREAT|0666);
    if(shmid == -1)
    {
        return NULL;
    }
    if((ptr = shmat(shmid,NULL,0)) == (char*)-1)
    {
        return NULL;
    }
    
    rq = (rapid_mqueue_t*)calloc(1,sizeof(rapid_mqueue_t));
    if(rq == NULL)
    {
        return NULL;
    }
    
    rq->pos = ptr + sizeof(rapid_mqueue_header_t);
    
    rq->header = (rapid_mqueue_header_t*)ptr;
    rq->header->next = 0;
    rq->header->current = -1;
    rq->header->cursor = -1;
    rq->header->capacity = cap;
    
    //del the share mem when the last process shmdt
    //if (shmctl(shmid, IPC_RMID, NULL) == -1)
    //{
    //    return NULL;
    //}

    return rq;
    
}

rapid_mqueue_t* rapid_mqueue_get(int key,int cap)
{
    int shmid = 0;
    char *ptr = NULL;
    size_t total = 0;
    rapid_mqueue_t *rq = NULL;
    
    if(cap < 0 || 
       cap < MSG_QUEUE_DEF_SIZE || 
       (cap&(cap-1)) ) 
    {
        cap = MSG_QUEUE_DEF_SIZE;
    }
    
    total = sizeof(rapid_mqueue_header_t) + (cap * sizeof(rapid_msg_item_t));
    shmid = shmget(key,total,0666);
    if(shmid == -1)
    {
        return NULL;
    }
    if((ptr = shmat(shmid,NULL,0)) == (char*)-1)
    {
        return NULL;
    }
    
    rq = (rapid_mqueue_t*)calloc(1,sizeof(rapid_mqueue_t));
    if(NULL == rq)
    {
        return NULL;
    }
    
    rq->pos = ptr + sizeof(rapid_mqueue_header_t);
    rq->header = (rapid_mqueue_header_t*)ptr;
    
    return rq;
}


int rapid_mqueue_slot(rapid_mqueue_t *rq)
{
    int slot = -1;
    struct timespec tv;
    if(NULL == rq)
    {
        return slot;
    }
    
    slot = __sync_fetch_and_add(&(rq->header->next),1);
    __sync_synchronize();
    
    return slot;
}


int rapid_mqueue_push(rapid_mqueue_t *rq,int slot,char *msg,int size,int type)
{
    char *pos = NULL;
    rapid_msg_item_t item = {0};
    size_t len = sizeof(rapid_msg_item_t);
    
    if(NULL == rq ||
       size <= 0 ||
       size > MSG_ITEM_SIZE)
    {
        return -1;
    }
    
    int index = slot & (rq->header->capacity-1);
    pos = rq->pos + index * len;
    
    item.header.type = type;
    item.header.size = size;
    memcpy(item.data,msg,size);
    
    memcpy(pos,&item,len);
    
    return item.header.size;
}


int rapid_mqueue_publish(rapid_mqueue_t *rq,int slot)
{
    while(!__sync_bool_compare_and_swap(&rq->header->cursor,slot-1,slot))
    {
    }
    __sync_synchronize();
    return 0;
}


int rapid_mqueue_pop(rapid_mqueue_t *rq,void *msg,int *type)
{
    char *pos = NULL;
    int slot = 0;
    rapid_msg_item_t item = {0};
    //struct timespec tv;
    size_t len = sizeof(rapid_msg_item_t);
    
    if(NULL == rq)
    {
        return -1;
    }
    
    // while(rq->header->cursor <= rq->header->current)
    // {
    //     tv.tv_sec = 0;
    //     tv.tv_nsec = 250 * 1000 * 1000;
        
    //     nanosleep(&tv,NULL);
    // }
    if (rq->header->cursor <= rq->header->current)
    {
        return 0;
    }
    
    slot = __sync_add_and_fetch(&(rq->header->current),1);
    __sync_synchronize();

    slot = slot & (rq->header->capacity-1);
    pos = rq->pos + slot * len;
    
    memcpy(&item,pos,len);
    memcpy(msg,item.data,MSG_ITEM_SIZE);
    *type = item.header.type;
    
    return item.header.size;
}

void rapid_mqueue_destory(rapid_mqueue_t *rq)
{
    if(NULL == rq)
    {
        return;
    }
    
    shmdt(rq->pos);
    free(rq);
}
