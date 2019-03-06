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

int add(buffer* b, int info,int priority){
    if(b->cap==b->size) return -1;
    entry* e = malloc(sizeof(entry));
    e->info=info;
    e->priority = priority;
    b->buff[b->bottom] = e;
    b->bottom++;
    b->bottom = b->bottom%b->cap;
    b->size++;
    return 0;
}

int get(buffer* b){
    if(b->size==0) return -1;
    int ret = b->buff[b->top]->info;
    b->top++;
        b->top = b->top%b->cap;
    b->size--;
    return ret;
}

void freeBuffer(buffer* b){
    free(b->buff);
}
