#ifndef BUFFER_H
#define BUFFER_H
#define FIFO 20
#define HPIC 30
#define HPHC 40


typedef struct e{
    int info;
    int priority;
}entry;


typedef struct b{
    entry** buff;
    int cap;
    int size;
    int top;
    int bottom;
}buffer;

buffer* createBuffer(int capacity,int policy);

//return -1 if full
int add(buffer* b, int element,int priority);

//return -1 if empty
int get(buffer* b);

void freeBuffer(buffer* b);


#endif
