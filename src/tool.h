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
#include <stdarg.h>
#include <sys/syscall.h>
#include "httpparser.h"

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define SPLICE_FLAGS (SPLICE_F_MOVE | SPLICE_F_NONBLOCK)
#define gettid() syscall(SYS_gettid)

typedef struct sockaddr SA;

enum{
    evsize = 64,
    mapsize = 1024 * 2, // 并发 1024 个连接
    listenq = 1024 //from csapp.h  second argument to listen()
};

//防止重名 
enum protocol {HTTP_,HTTPS_,Unknow};
//enum method {OPTIONS,PUT,DELETE,TRACE,GET,POST,HEAD,CONNECT,UNKNOW};
enum tag       {Connect,HTTP,HTTPS,Forward};
enum log_level {Error,Error_exit,Warn,Info,Debug};

struct args{
    int lport;   // listening port
    char * paddr;// proxy addr
    int pport;   // proxy port
};

/*  map 快速查找 connfd-proxyfd 对
* connfd | proxyfd | tag
1   1         2      HTTP
2   1         2      HTTP
3   3         4     HTTPS
4   3         4     HTTPS
*/
struct map{
    int connfd;
    int proxyfd;
    enum tag tag;
};

//log
void print_log(enum log_level level,const char * func,char * file,int line,char * format,...);

#define LOG(log_level,...) do{                                          \
    print_log(log_level,__FUNCTION__,__FILE__,__LINE__,__VA_ARGS__);    \
}while(0);                                                              \


#define log_error(...) LOG(Error,__VA_ARGS__);
#define log_info(...)  LOG(Info,__VA_ARGS__);
#define log_warn(...)  LOG(Warn,__VA_ARGS__);
#define log_debug(...) LOG(Debug,__VA_ARGS__);
#define log_error_exit(...) do{LOG(Error,__VA_ARGS__);exit(-1);}while(0);
#define log_strerror(void)    LOG(Error,"%s",strerror(errno));



void Add_epoll_fd(int epfd,int fd,__uint32_t events);
void Del_epoll_fd(int epfd,int fd);
void Mod_epoll_fd(int epfd,int fd,__uint32_t events);
void Close(int fd);
void print_evmode(int fd,__uint32_t ev);
int Init_epfd(void);
bool isHTTPS(int fd);
bool isHTTP(int fd);

void Pipe(int * pipe);

int Set_nonblocking(int sockfd);
void Socketpair(int * socks);

//from csapp.h
int Open_clientfd(char * hostname, int port);
int Open_listenfd(int port);


#endif