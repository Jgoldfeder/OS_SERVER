#include <unistd.h>  //Header file for sleep(). man 3 sleep for details.
#include <stdlib.h>
#include "pool.h"
#include <stdio.h>
#define LOG        44

void logger(int type, char *s1, char *s2, int socket_fd);

pthread_mutex_t mutex= PTHREAD_MUTEX_INITIALIZER;
void* workerThread(void* b);

void createPool(int size,buffer* b){
    for(int i =0;i<size;i++){
        pthread_t* thread = malloc(sizeof(pthread_t));
        int ret = pthread_create(thread,NULL,workerThread,b);
        if(ret!=0){
            printf("%s\n","Could not create thread");
        }
    }

    sem_init(&full, 0, 0);
    sem_init(&empty, 0, size);


}


void* workerThread(void* v){
  logger(LOG,"thread starting....",0,getpid());
    buffer* b = (buffer*) v;
    while(1){
        logger(LOG,"thread waiting ...",0,getpid());
        sem_wait(&full);
        pthread_mutex_lock(&mutex);
        logger(LOG,"reading from buffer",0,getpid());
        entry* info = get(b);
        pthread_mutex_unlock(&mutex);
        sem_post(&empty);
        //do something
        logger(LOG,"processing request",0,getpid());
      //  logger(LOG,itoa(info),0,getpid());

        web(info, 0);//this should not be zero, change later
    }

}
