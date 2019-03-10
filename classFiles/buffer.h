#ifndef BUFFER_H
#define BUFFER_H
#define FIFO 20
#define HPIC 30
#define HPHC 40


typedef struct e{
    char* info;
    int html;
    int fd;
    int hit;
    int priority;
    long time_arrival;
    long dispatched_time;
}entry;


typedef struct b{
    entry** buff;
    int cap;
    int size;
    int policy;
}buffer;

buffer* createBuffer(int capacity,int policy);

unsigned long get_time();

//return -1 if full
int add(buffer* b, entry* e);

//return -1 if empty
entry* get(buffer* b, long server_time);

void freeBuffer(buffer* b);

void freeEntry(entry* e);

#endif
