/* Generic */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Network */
#include <netdb.h>
#include <sys/socket.h>

/* Extras */
#include <pthread.h>
#include <sched.h>
#include <time.h>

/* Function headers */
int connect_and_send_request (int argc, char **argv, int fileNum, int policy);
int create_threads (int numThreads);
void *workerCONCURFunc (void *tInf);
void *workerFIFOFunc (void *tInf);

/*Globals and Defined variables*/
#define BUF_SIZE 100
//Policies
#define CONCUR 1
#define FIFO 2

pthread_barrier_t mybarrier;
pthread_cond_t condVerb = PTHREAD_COND_INITIALIZER;
pthread_mutex_t fifoMutex = PTHREAD_MUTEX_INITIALIZER;
int fifoTurn;
int numberOfThreads;

/* Data structures*/
typedef struct thread_information {
    char ** argv;
    int argc;
    int id;
} thread_info;

// Get host information (used to establishConnection)
struct addrinfo *getHostInfo(char *host, char *port) {
    int r;
    struct addrinfo hints, *getaddrinfo_res;
    // Setup hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if ((r = getaddrinfo(host, port, &hints, &getaddrinfo_res))) {
        fprintf(stderr, "[getHostInfo:21:getaddrinfo] %s\n", gai_strerror(r));
        return NULL;
    }

    return getaddrinfo_res;
}

// Establish connection with host
int establishConnection(struct addrinfo *info) {
    if (info == NULL) return -1;

    int clientfd;
    for (; info != NULL; info = info->ai_next) {
        if ((clientfd = socket(info->ai_family,
                               info->ai_socktype,
                               info->ai_protocol)) < 0) {
            perror("[establishConnection:35:socket]");
            continue;
        }

        if (connect(clientfd, info->ai_addr, info->ai_addrlen) < 0) {
            close(clientfd);
            perror("[establishConnection:42:connect]");
            continue;
        }

        freeaddrinfo(info);
        return clientfd;
    }

    freeaddrinfo(info);
    return -1;
}

// Send GET request
void GET(int clientfd, char *path) {
    char req[1000] = {0};
    sprintf(req, "GET %s HTTP/1.0\r\n\r\n", path);
    send(clientfd, req, strlen(req), 0);
}

int main(int argc, char **argv) {
//    int clientfd;
//    char buf[BUF_SIZE];

    /* Checks for amount of arguments */
    if (argc == 4) {
        connect_and_send_request(argc, argv, 0, -1);
    } else if (argc != 6) {
        if (argc != 7) {
            fprintf(stderr, "USAGE: ./httpclient <hostname> <port> <request path>\n");
            return 1;
        }
    }

    int numThreads = atoi(argv[3]);

    /* getting scheduling policy*/
    char* schedAlg = argv[4];
    int policy = -1;

    if
            (!strcmp(schedAlg,"CONCUR")) policy = CONCUR;//FIFO is any policy, after all
    else if
            (!strcmp(schedAlg,"FIFO")) policy = FIFO;
    else{
        printf("Error: \"%s\" is Invalid scheduling policy, try CONCUR or FIFO\n",schedAlg);
        return 1;
    }

    //-----------------------------------------------------------

    /* --Thread creation-- */
    pthread_t threadIds[numThreads]; //holds the pthread_t for each thread
    thread_info tInfo[numThreads]; //array of thread_info structs for each thread

    int ret;

    ret = pthread_barrier_init(&mybarrier, NULL, numThreads);

    if(ret != 0){
        printf("Could not create thread barrier\n");
        return 1;
    }

//    ret = pthread_cond_init(&condVerb, NULL);
//
//    if(ret != 0){
//        printf("Could not create condition variable\n");
//        return 1;
//    }

    numberOfThreads = numThreads;
    int i;

    if (policy == CONCUR)
        for (i = 0; i < numThreads; i++) {
            //sets struct params
            tInfo[i].id = i;
            tInfo[i].argv = argv;
            tInfo[i].argc = argc;

            //creates a thread
            ret = pthread_create(&threadIds[i], NULL, workerCONCURFunc, (void *) &tInfo[i]);

            if(ret != 0){
                printf("Could not create thread\n");
                return 1;
            }
        }

    if (policy == FIFO) {
        for (i = 0; i < numThreads; i++) {
            fifoTurn = 0;

            //sets struct params
            tInfo[i].id = i;
            tInfo[i].argv = argv;
            tInfo[i].argc = argc;

            //creates a thread
            ret = pthread_create(&threadIds[i], NULL, workerFIFOFunc, (void *) &tInfo[i]);

            if (ret != 0) {
                printf("Could not create thread\n");
                return 1;
            }
        }
    }

//    pthread_barrier_wait(&mybarrier);

//    printf("main()\n");

    pthread_exit(NULL);

    return 0;
}

void *workerCONCURFunc (void *tInf) {
    thread_info *tInfo = tInf;
    int j = 0;

    while (1) {
        if (tInfo->argc == 7) {
            //generating random number for a file picker
//            srand(time(0) + tInfo->id);
//            int argNum = rand() % 2;
            int argNum = j % 2;

            connect_and_send_request(tInfo->argc, (char **) tInfo->argv, argNum, -1);
        }
        else
            connect_and_send_request(tInfo->argc, (char **) tInfo->argv, 0, -1);

        pthread_barrier_wait(&mybarrier);
        j++;
    }
    return 0;
}

void *workerFIFOFunc (void *tInf) {
    thread_info *tInfo = tInf;
    int j = 0;

    while (1) {
//        if (tInfo->id == 0)
//            fifoTurn = 0;

        pthread_mutex_lock( &fifoMutex);
        while (tInfo->id != fifoTurn) {
            printf("%d, fifoTurn = %d\n", tInfo->id, fifoTurn);
//            pthread_mutex_unlock( &fifoMutex);
            pthread_cond_signal(&condVerb);
            pthread_cond_wait( &condVerb, &fifoMutex);

//            printf("%d after cond\n", tInfo->id);
//            pthread_mutex_lock( &fifoMutex);
            printf("%d is here\n", tInfo->id);

        }
//        pthread_mutex_lock( &fifoMutex);


        /* WORK */
        if (tInfo->argc == 7) {
            //generating random number for a file picker
//            srand(time(0) + tInfo->id);
//            int argNum = rand() % 2;
            int argNum = j % 2;

            connect_and_send_request(tInfo->argc, (char **) tInfo->argv, argNum, tInfo->id);
        }
        else
            connect_and_send_request(tInfo->argc, (char **) tInfo->argv, 0, -1);
        /* END WORK*/


        pthread_barrier_wait(&mybarrier);
        j++;
    }
    return 0;
}

int connect_and_send_request (int argc, char **argv, int fileNum, int policy) {
    int clientfd;
    char buf[BUF_SIZE];

    // Establish connection with <hostname>:<port>
    clientfd = establishConnection(getHostInfo(argv[1], argv[2]));
    if (clientfd == -1) {
        fprintf(stderr,
                "[main:73] Failed to connect to: %s:%s%s \n",
                argv[1], argv[2], argv[3]);
        return 3;
    }

    // Send GET request > stdout
    if (policy >= 0) {
        printf("%d done fifo: %d\n", policy, fifoTurn);

        if (fifoTurn == numberOfThreads-1)
            fifoTurn = 0;
        else
            fifoTurn++;
        printf("%d\n", fifoTurn);

        pthread_cond_signal(&condVerb);
        pthread_mutex_unlock( &fifoMutex );

        if (argc == 6)
            GET(clientfd, argv[5]);
        else {
            GET(clientfd, argv[5 + fileNum]);
        }

        return 0;
    }

    if (argc == 6)
        GET(clientfd, argv[5]);
    else if (argc == 7) {
        GET(clientfd, argv[5 + fileNum]);
    }
    else
        GET(clientfd, argv[3]);

    while (recv(clientfd, buf, BUF_SIZE, 0) > 0) {
        fputs(buf, stdout);
        memset(buf, 0, BUF_SIZE);
    }

    close(clientfd);
    return 0;
}

int create_threads (int numThreads) {
    return 0;
}


