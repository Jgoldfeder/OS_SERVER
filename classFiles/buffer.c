#include "buffer.h"
#include <limits.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

#define ERROR      42

int dispatch_count = 0;
pthread_mutex_t dispatch_mutex= PTHREAD_MUTEX_INITIALIZER;


buffer* createBuffer(int capacity,int policy){
    entry** buff = malloc(sizeof(entry*)*capacity);
    int size = 0;
    int top = 0;
    int bottom = 0;
    buffer* b = malloc(sizeof(buffer));
    b->buff = buff;
    b->size = size;
    b->cap = capacity;
    b->policy = policy;
    return b;
}

unsigned long get_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	unsigned long ret = tv.tv_usec;
	ret /= 1000;
	ret += (tv.tv_sec * 1000);
	return ret;
}


int getPriority(buffer* b,entry* e){
  int hit,html,policy;
  hit = e->hit;
  html=e->html;
  policy = b->policy;
  if(policy==FIFO){
    //new entries simply have a lower getPriority
    return INT_MAX-hit;
  }
  if(policy == HPIC){
    //always give pics a higher getPriority
    if(html) return -hit;
    return INT_MAX-hit;
  }
  if(policy == HPHC){
    //always give html a higher getPriority
    if(!html) return -hit;
    return INT_MAX-hit;
  }
  //policy is ill specifified
  return 0;
}


//act as a priority queue
int add(buffer* b, entry* e){
    //if queue is full, return
    if(b->cap==b->size) return -1;

    e->priority = getPriority(b,e);
    //insert into queue
    int i;
    for(i=0;i<b->size;i++){
      if(b->buff[i]->priority > e->priority) break;
      b->buff[i]->stat_req_age++;

    }
    for(int j =b->size;j>i;j--){
      b->buff[j] = b->buff[j-1];
    }
    b->buff[i] = e;

    b->size++;
    return 0;
}

entry* get(buffer* b, long server_time){
    int retu;

    if(b->size==0) return NULL;

    entry* ret = b->buff[b->size-1];
    ret->dispatched_time = (get_time() - server_time);

    retu = pthread_mutex_lock(&dispatch_mutex);

    if (retu != 0) {
//        logger(ERROR,"Could not lock mutex",0,getpid());
        exit(1);
    }

    ret->prior_dispatch_count = dispatch_count++;
    retu = pthread_mutex_unlock(&dispatch_mutex);

    if (retu != 0) {
//        logger(ERROR,"Could not unlock mutex",0,getpid());
        exit(1);
    }

    b->size--;
    return ret;
}

void freeBuffer(buffer* b){
    free(b->buff);
}


void freeEntry(entry* e){
  free(e->info);
  free(e);
}
