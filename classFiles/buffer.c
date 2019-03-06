#include "buffer.h"
#include <stdlib.h>
buffer* createBuffer(int capacity){
    int* buff = malloc(sizeof(int)*capacity);
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

int add(buffer* b, int element){
    if(b->cap==b->size) return -1;
    b->buff[b->bottom] = element;
    b->bottom++;
    b->bottom = b->bottom%b->cap;
    b->size++;
    return 0;
}

int get(buffer* b){
    if(b->size==0) return -1;
    int ret = b->buff[b->top];
    b->top++;
        b->top = b->top%b->cap;
    b->size--;
    return ret;
}

void freeBuffer(buffer* b){
    free(b->buff);
}