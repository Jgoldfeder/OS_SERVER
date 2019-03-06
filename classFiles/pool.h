#ifndef POOL_H
#define POOL_H


#include <pthread.h> 
#include <semaphore.h>
#include "buffer.h"

pthread_mutex_t mutex= PTHREAD_MUTEX_INITIALIZER;
sem_t full;
sem_t empty;

void web(int fd, int hit);

void createPool(int size,buffer* b);








#endif

