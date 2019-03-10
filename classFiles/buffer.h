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
    int prior_dispatch_count;
    int prior_completed_requests;
    int stat_req_age;
}entry;


typedef struct b{
    entry** buff;
    int cap;
    int size;
    int policy;
}buffer;

buffer* createBuffer(int capacity,int policy);

//source-https://gist.github.com/afrachioni/f441de4ab75b8c7de264
unsigned long get_time();

//return -1 if full
int add(buffer* b, entry* e);

//return -1 if empty
entry* get(buffer* b, long server_time);

void freeBuffer(buffer* b);

void freeEntry(entry* e);

#endif
