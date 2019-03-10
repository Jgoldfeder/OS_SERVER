#include <unistd.h>  //Header file for sleep(). man 3 sleep for details.
#include <stdlib.h>
#include "pool.h"
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#define LOG        44
#define ERROR      42

void logger(int type, char *s1, char *s2, int socket_fd);

pthread_mutex_t mutex= PTHREAD_MUTEX_INITIALIZER;
void* workerThread(void* b);


typedef struct t{

    buffer* b;
    int thread_id;
    long server_t;

} thread_info;

void createPool(int size,buffer* b, long server_time){
    for(int i =0;i<size;i++){
        pthread_t* thread = malloc(sizeof(pthread_t));

	thread_info* t_info = malloc(sizeof(thread_info));
	t_info->b = b;
	t_info->thread_id = i;
	t_info->server_t = server_time;
        //int ret = pthread_create(thread,NULL,workerThread,b);
	int ret = pthread_create(thread,NULL,workerThread,t_info);
        if(ret!=0){
          logger(ERROR,"Could not create thread",0,getpid());
        }
    }

    sem_init(&full, 0, 0);
    sem_init(&empty, 0, size);
}


void* workerThread(void* v){
    logger(LOG,"thread starting....",0,getpid());
 
// can save struct info as local vars and free vars. 
    thread_info* t_info = (thread_info*) v;

    int html_count = 0;
    int pic_count = 0;
    while(1){
        logger(LOG,"thread waiting ...",0,getpid());
        sem_wait(&full);
        pthread_mutex_lock(&mutex);
        logger(LOG,"reading from buffer",0,getpid());
       
	entry* info = get(t_info->b, t_info->server_t);
	//info contains the hit number which is the reqquest count. Can deduce the number of request that arrived before this one.
        pthread_mutex_unlock(&mutex);
        sem_post(&empty);
        logger(LOG,"processing request ",0,getpid());
	
	if (info->html > 0){
	   html_count++;
	}
	else{
	   pic_count++;
	}

        web(info, t_info->thread_id, html_count, pic_count);
    }

}
