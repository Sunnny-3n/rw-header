#ifndef TOOL_H_
#define TOOL_H_

#define _GNU_SOURCE
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <netdb.h>
#include <stdbool.h>
#include <sys/syscall.h>

typedef struct sockaddr SA;

#define EVSIZE  64

/*  map 快速查找 connfd-proxy 对
* connfd | proxyfd | tag
1   1         2      HTTP
2   1         2      HTTP
3   3         4     HTTPS
4   3         4     HTTPS
*/
#define MAPSIZE 1024*2  //最多并发 1024 个连接

#define MAIN 0
#define THREAD 1

#define SPLICE_FLAGS (SPLICE_F_MOVE | SPLICE_F_NONBLOCK)
#define gettid() syscall(SYS_gettid)

//from csapp.c
#define LISTENQ  1024  /* second argument to listen() */

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"

enum method {OPTIONS,PUT,DELETE,TRACE,GET,POST,HEAD,CONNECT,UNKNOW};
enum tag       {Connect,HTTP,HTTPS,Forward};
enum log_level {Error,Error_exit,Warn,Info,Debug};

struct args{
    int lport; // listening port
    char * paddr;// proxy addr
    int pport; // proxy port
};

struct map{
    int connfd;
    int proxyfd;
    enum tag tag;
};

#define LOG(log_level,...) do{                                  \
    char buf[128];                                              \
    snprintf(buf,128,__VA_ARGS__);                              \
    Print_log(log_level,__FUNCTION__,__FILE__,__LINE__,buf);    \
    if(log_level == Error_exit)                                 \
        exit(-1);                                               \
}while(0);                                                      \

#define LERROR() do{LOG(Error,"%s",strerror(errno))}while(0);


void Add_epoll_fd(int epfd,int fd,__uint32_t events);
void Del_epoll_fd(int epfd,int fd);
void Mod_epoll_fd(int epfd,int fd,__uint32_t events);
void Close(int fd);
void Init_stream(int listenfd);
void print_evmode(int fd,__uint32_t ev);
int Init_epfd(void);
bool isHTTPS(int fd);
bool isHTTP(int fd);

void Pipe(int * pipe);

//[Error] [13:01:22] [tid 6711] [main.c:23 Set_nonblocking()] Set_nonblocking
void Print_log(enum log_level level,const char * func,char * file,int line,char * msg);
int Set_nonblocking(int sockfd);
void Socketpair(int * socks);

//from csapp.c
int Open_clientfd(char *hostname, int port);
int Open_listenfd(int port);


#endif