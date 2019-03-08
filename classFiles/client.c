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
#include <sys/types.h>
pid_t gettid(void);

/* Function headers */
int connect_and_send_request (int argc, char **argv);
int create_threads (int numThreads);
void *workerFunc (void *argv);

#define BUF_SIZE 100
pthread_barrier_t mybarrier;

/* Data structures*/
typedef struct thread_information {
    int id;
    char ** argv;
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


    if (argc == 4) {
        connect_and_send_request(argc, argv);
    } else if (argc != 6) {
        if (argc != 7) {
            fprintf(stderr, "USAGE: ./httpclient <hostname> <port> <request path>\n");
            return 1;
        }
    }

    int numThreads = atoi(argv[3]);

    create_threads(numThreads);

    //-----------------------------------------------------------

    pthread_t threadIds[numThreads];
    thread_info tInfo[numThreads];

    pthread_barrier_init(&mybarrier, NULL, numThreads);

    int i;

    for (i = 0; i < numThreads; i++) {
        tInfo[i].id = i;
        tInfo[i].argv = argv;

        int ret = pthread_create(&threadIds[i], NULL, workerFunc, (void *) &tInfo[i]);

        if(ret != 0){
            printf("Could not create thread\n");
        }
    }

//    pthread_barrier_wait(&mybarrier);

//    printf("main()\n");

    pthread_exit(NULL);

    return 0;
}

void *workerFunc (void *argv) {
    thread_info *tInfo = argv;
    int j = 0;

    while (1) {
        //        printf("Thread: %d Loop: %d after\n", tInfo->id, j);

        connect_and_send_request(6, (char **) tInfo->argv);

        printf("%d Loop: %d before\n", tInfo->id, j);

        pthread_barrier_wait(&mybarrier);
        j++;
    }
    return 0;
}

int connect_and_send_request (int argc, char **argv) {
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
    if (argc > 4)
        GET(clientfd, argv[5]);
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


