#include "buffer.h"
#include <stdlib.h>
buffer* createBuffer(int capacity,int policy){
    entry** buff = malloc(sizeof(entry*)*capacity);
    int size = 0;
    int top = 0;
    int bottom = 0;
    buffer* b = malloc(sizeof(buffer));
    b->buff = buff;
    b->size = size;
    b->cap = capacity;
    b->top = top;
    b->bottom = bottom;
    return b;
}

//act as a priority queue
int add(buffer* b, int info,int priority){
    //if queue is full, return
    if(b->cap==b->size) return -1;

    //create entry
    entry* e = malloc(sizeof(entry));
    e->info=info;
    e->priority = priority;

    //insert into queue
    int i;
    for(i=0;i<b->size;i++){
      if(b->buff[i]->priority > priority) break;
    }
    for(int j =b->size;j>i;j--){
      b->buff[j] = b->buff[j-1];
    }
    b->buff[i] = e;

    b->size++;
    return 0;
}

int get(buffer* b){
    if(b->size==0) return -1;

    int ret = b->buff[b->size-1]->info;
    b->size--;
    return ret;
}

void freeBuffer(buffer* b){
    free(b->buff);
}
