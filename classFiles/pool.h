#ifndef POOL_H
#define POOL_H


#include <pthread.h>
#include <semaphore.h>
#include "buffer.h"

extern pthread_mutex_t mutex;
sem_t full;
sem_t empty;

void web(entry* fd, int id, int html, int pic);

void createPool(int size,buffer* b, long server_time);
unsigned long get_time();








#endif
