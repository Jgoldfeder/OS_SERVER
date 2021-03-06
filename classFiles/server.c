#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include "buffer.h"
#include "pool.h"
#define VERSION 23
#define BUFSIZE 8096
#define ERROR      42
#define LOG        44
#define FORBIDDEN 403
#define NOTFOUND  404
long server_time;

pthread_mutex_t completed_mutex= PTHREAD_MUTEX_INITIALIZER;
int request_completed= 0;


struct {
	char *ext;
	char *filetype;
} extensions [] = {
		{"gif", "image/gif" },
		{"jpg", "image/jpg" },
		{"jpeg","image/jpeg"},
		{"png", "image/png" },
		{"ico", "image/ico" },
		{"zip", "image/zip" },
		{"gz",  "image/gz"  },
		{"tar", "image/tar" },
		{"htm", "text/html" },
		{"html","text/html" },
		{0,0} };

static int dummy; //keep compiler happy

void logger(int type, char *s1, char *s2, int socket_fd)
{
	int fd ;
	char logbuffer[BUFSIZE*2];

	switch (type) {
		case ERROR: (void)sprintf(logbuffer,"ERROR: %s:%s Errno=%d exiting pid=%d",s1, s2, errno,getpid());
			break;
		case FORBIDDEN:
			dummy = write(socket_fd, "HTTP/1.1 403 Forbidden\nContent-Length: 185\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>403 Forbidden</title>\n</head><body>\n<h1>Forbidden</h1>\nThe requested URL, file type or operation is not allowed on this simple static file webserver.\n</body></html>\n",271);
			(void)sprintf(logbuffer,"FORBIDDEN: %s:%s",s1, s2);
			break;
		case NOTFOUND:
			dummy = write(socket_fd, "HTTP/1.1 404 Not Found\nContent-Length: 136\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>404 Not Found</title>\n</head><body>\n<h1>Not Found</h1>\nThe requested URL was not found on this server.\n</body></html>\n",224);
			(void)sprintf(logbuffer,"NOT FOUND: %s:%s",s1, s2);
			break;
		case LOG: (void)sprintf(logbuffer," INFO: %s:%s:%d",s1, s2,socket_fd); break;
	}
	/* No checks here, nothing can be done with a failure anyway */
	if((fd = open("nweb.log", O_CREAT| O_WRONLY | O_APPEND,0644)) >= 0) {
		dummy = write(fd,logbuffer,strlen(logbuffer));
		dummy = write(fd,"\n",1);
		(void)close(fd);
	}
	if(type == ERROR || type == NOTFOUND || type == FORBIDDEN) exit(3);
}



entry* getEntry(int fd, int hit, long server_time ){
	char* buffer = calloc(BUFSIZE+1,sizeof(char));

	long request_arrival = get_time() - server_time; //time of request arrival relative to server time


	long ret =read(fd,buffer,BUFSIZE); 	/* read Web request in one go */
	if(ret == 0 || ret == -1) {	/* read failure stop now */
		logger(FORBIDDEN,"failed to read browser request","",fd);
	}
	if(ret > 0 && ret < BUFSIZE)	/* return code is valid chars */
		buffer[ret]=0;		/* terminate the buffer */
	else buffer[0]=0;
	for(int i=0;i<ret;i++)	/* remove CF and LF characters */
		if(buffer[i] == '\r' || buffer[i] == '\n')
			buffer[i]='*';
	//get if file is html
	int html = 0;
	if (strstr(buffer, ".html") != NULL) {
		html = 1;
	}
	entry* e = malloc(sizeof(entry));
	e->info = buffer;
	e->html = html;
	e->hit = hit;
	e->fd = fd;
	e->time_arrival = request_arrival;
	e->stat_req_age = 0;
	return e;
}







/* this is a child web server process, so we can exit on errors */
void web(entry* e, int id, int html, int pic)
{
	int ret;
	int thread_id = id;
	int html_count = html;
	int pic_count = pic;
	//int total_jobs = html_count + pic_count;

	int hit = e->hit;
	logger(LOG,"WEB",0,hit);

	int j,i, file_fd, buflen;
	long len;
	char * fstr;
	char* buffer = e->info;
	int fd = e->fd;



	logger(LOG,"request",buffer,hit);
	if( strncmp(buffer,"GET ",4) && strncmp(buffer,"get ",4) ) {
		logger(FORBIDDEN,"Only simple GET operation supported",buffer,fd);
	}
	for(i=4;i<BUFSIZE;i++) { /* null terminate after the second space to ignore extra stuff */
		if(buffer[i] == ' ') { /* string is "GET URL " +lots of other stuff */
			buffer[i] = 0;
			break;
		}
	}
	for(j=0;j<i-1;j++) 	/* check for illegal parent directory use .. */
		if(buffer[j] == '.' && buffer[j+1] == '.') {
			logger(FORBIDDEN,"Parent directory (..) path names not supported",buffer,fd);
		}
	if( !strncmp(&buffer[0],"GET /\0",6) || !strncmp(&buffer[0],"get /\0",6) ) /* convert no filename to index file */
		(void)strcpy(buffer,"GET /index.html");

	/* work out the file type and check we support it */
	buflen=strlen(buffer);
	fstr = (char *)0;
	for(i=0;extensions[i].ext != 0;i++) {
		len = strlen(extensions[i].ext);
		if( !strncmp(&buffer[buflen-len], extensions[i].ext, len)) {
			fstr =extensions[i].filetype;
			break;
		}
	}
	if(fstr == 0) logger(FORBIDDEN,"file extension type not supported",buffer,fd);

	if(( file_fd = open(&buffer[5],O_RDONLY)) == -1) {  /* open the file for reading */
		logger(NOTFOUND, "failed to open file",&buffer[5],fd);
	}

	long completed_time = (get_time() -server_time);
	ret = pthread_mutex_lock(&completed_mutex);

	if (ret != 0) {
		logger(ERROR,"Could not lock mutex",0,getpid());
		exit(1);
	}

	e->prior_completed_requests = request_completed;
	request_completed++;

	ret = pthread_mutex_unlock(&completed_mutex);

	if (ret != 0) {
		logger(ERROR,"Could not unlock mutex",0,getpid());
		exit(1);
	}



	logger(LOG,"SEND",&buffer[5],hit);
	len = (long)lseek(file_fd, (off_t)0, SEEK_END); /* lseek to the file end to find the length */
	(void)lseek(file_fd, (off_t)0, SEEK_SET); /* lseek back to the file start ready for reading */
	(void)sprintf(buffer,"HTTP/1.1 200 OK\nServer: nweb/%d.0 \nContent-Length: %ld\nConnection: close\nContent-Type: %s\n", VERSION, len, fstr); /* Header + a blank line */
	logger(LOG,"Header",buffer,hit);
	dummy = write(fd,buffer,strlen(buffer));

	// Send the statistical headers described in the paper, example below

	(void)sprintf(buffer,"X-stat-thread-id: %d\n", thread_id);
	dummy = write(fd,buffer,strlen(buffer));

	(void)sprintf(buffer,"X-stat-thread-html: %d\n", html);
	dummy = write(fd,buffer,strlen(buffer));

	(void)sprintf(buffer,"X-stat-thread-image: %d\n", pic);
	dummy = write(fd,buffer,strlen(buffer));

	(void)sprintf(buffer,"X-stat-thread-count: %d\n", pic_count + html_count);
	dummy = write(fd,buffer,strlen(buffer));


	(void)sprintf(buffer,"X-stat-req-arrival: %d\n", e->hit);
	dummy = write(fd,buffer,strlen(buffer));
	logger(LOG,"hit",0,e->hit);

	(void)sprintf(buffer,"X-stat-req-arrival-time: %ld ms\n", e->time_arrival);
	dummy = write(fd,buffer,strlen(buffer));

	(void)sprintf(buffer,"X-stat-req-dispatch-time: %ld ms\n", e->dispatched_time);
	dummy = write(fd,buffer,strlen(buffer));

	(void)sprintf(buffer,"X-stat-req-dispatch-count: %d\n", e->prior_dispatch_count);
	dummy = write(fd,buffer,strlen(buffer));

	(void)sprintf(buffer,"X-stat-req-complete-time: %ld ms\n", completed_time);
	dummy = write(fd,buffer,strlen(buffer));

	(void)sprintf(buffer,"X-stat-req-complete-count: %d\n", e->prior_completed_requests);
	dummy = write(fd,buffer,strlen(buffer));

	(void)sprintf(buffer,"X-stat-req-age: %d\n\n", e->stat_req_age);
	dummy = write(fd,buffer,strlen(buffer));
	logger(LOG,"stat_req_age",0,e->stat_req_age);

	//int ret;
	/* send file in 8KB block - last block may be smaller */
	while (	(ret = read(file_fd, buffer, BUFSIZE)) > 0 ) {
		dummy = write(fd,buffer,ret);
	}
	sleep(1);	/* allow socket to drain before signalling the socket is closed */
	close(fd);
	freeEntry(e);
}

int main(int argc, char **argv)
{
	server_time = get_time();


	int i, port,  listenfd, socketfd, hit;
	socklen_t length;
	static struct sockaddr_in cli_addr; /* static = initialised to zeros */
	static struct sockaddr_in serv_addr; /* static = initialised to zeros */

	if( argc < 6  || argc > 6 || !strcmp(argv[1], "-?") ) {
		(void)printf("hint: nweb Port-Number Top-Directory\t\tversion %d\n\n"
					 "\tnweb is a small and very safe mini web server\n"
					 "\tnweb only servers out file/web pages with extensions named below\n"
					 "\t and only from the named directory or its sub-directories.\n"
					 "\tThere is no fancy features = safe and secure.\n\n"
					 "\tExample: nweb 8181 /home/nwebdir &\n\n"
					 "\tOnly Supports:", VERSION);
		for(i=0;extensions[i].ext != 0;i++)
			(void)printf(" %s",extensions[i].ext);

		(void)printf("\n\tNot Supported: URLs including \"..\", Java, Javascript, CGI\n"
					 "\tNot Supported: directories / /etc /bin /lib /tmp /usr /dev /sbin \n"
					 "\tNo warranty given or implied\n\tNigel Griffiths nag@uk.ibm.com\n"  );
		exit(0);
	}
	if( !strncmp(argv[2],"/"   ,2 ) || !strncmp(argv[2],"/etc", 5 ) ||
		!strncmp(argv[2],"/bin",5 ) || !strncmp(argv[2],"/lib", 5 ) ||
		!strncmp(argv[2],"/tmp",5 ) || !strncmp(argv[2],"/usr", 5 ) ||
		!strncmp(argv[2],"/dev",5 ) || !strncmp(argv[2],"/sbin",6) ){
		(void)printf("ERROR: Bad top directory %s, see nweb -?\n",argv[2]);
		exit(3);
	}
	if(chdir(argv[2]) == -1){
		(void)printf("ERROR: Can't Change to directory %s\n",argv[2]);
		exit(4);
	}
	/* Become deamon + unstopable and no zombies children (= no wait()) */
	if(fork() != 0)
		return 0; /* parent returns OK to shell */
	(void)signal(SIGCHLD, SIG_IGN); /* ignore child death */
	(void)signal(SIGHUP, SIG_IGN); /* ignore terminal hangups */
	for(i=0;i<32;i++)
		(void)close(i);		/* close open files */
	(void)setpgrp();		/* break away from process group */
	logger(LOG,"nweb starting",argv[1],getpid());
	/* setup the network socket */
	if((listenfd = socket(AF_INET, SOCK_STREAM,0)) <0)
		logger(ERROR, "system call","socket",0);
	port = atoi(argv[1]);
	if(port < 0 || port >60000)
		logger(ERROR,"Invalid port number (try 1->60000)",argv[1],0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	if(bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0)
		logger(ERROR,"system call","bind",0);
	if( listen(listenfd,64) <0)
		logger(ERROR,"system call","listen",0);


	//read in other cmdline args:
	int threadNum = atoi(argv[3]);
	int buffSize = atoi(argv[4]);
	char* schedAlg = argv[5];
	int policy = -1;
	if(!strcmp(schedAlg,"ANY")) policy = FIFO;//FIFO is any policy, after all
	else if(!strcmp(schedAlg,"FIFO")) policy = FIFO;
	else if(!strcmp(schedAlg,"HPIC")) policy = HPIC;
	else if(!strcmp(schedAlg,"HPHC")) policy = HPHC;
	else{
		logger(ERROR,"Error: Invalid scheduling policy",argv[1],0);
	}

	buffer* b = createBuffer(buffSize,policy);
	createPool(threadNum,b, server_time);

	int ret;

	for(hit=0; ;hit++) {
		length = sizeof(cli_addr);
		logger(LOG,"waiting...",argv[1],getpid());
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0)
			logger(ERROR,"system call","accept",0);

		entry* e = getEntry(socketfd,hit, server_time);
		ret = sem_wait(&empty);

		if (ret != 0) {
			logger(ERROR,"Could not sem_wait",0,getpid());
			exit(1);
		}

		ret = pthread_mutex_lock(&mutex);

		if (ret != 0) {
			logger(ERROR,"Could not lock mutex",0,getpid());
			exit(1);
		}

		logger(LOG,"adding to buffer",argv[1],hit);

		add(b,e);
		ret = pthread_mutex_unlock(&mutex);

		if (ret != 0) {
			logger(ERROR,"Could not lock mutex",0,getpid());
			exit(1);
		}

		ret= sem_post(&full);

		if (ret != 0) {
			logger(ERROR,"Could not sem_post",0,getpid());
			exit(1);
		}


	}
}
