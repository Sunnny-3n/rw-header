#define _GNU_SOURCE
#include <fcntl.h>
#include "csapp/csapp.h"
#include <sys/epoll.h>
#include <limits.h>
#include <pthread.h>
#include <time.h>

#define BUFSIZE 4096
#define EVSIZE 1024

#define PORT 9998
#define ADDR "127.0.0.1"

#define PROXY "127.0.0.1"
#define PROXY_PORT 9999
#define LOGSIZE 200

#define MAIN 0
#define THREAD 1

#define SPLICE_FLAGS (SPLICE_F_MOVE | SPLICE_F_NONBLOCK)
#define PRINT_NUM(x) fprintf(stderr,"#x %d\n",x);

#define LOG()


enum mode {Forward,Closefd};
enum level {Error,Warn,Normal,Debug};
struct task{
    enum mode mode;
    int fd;
};

/* 快速查找 socket
    local_fd       local_fd
    upstream_fd    upstream_fd
        1            2
        2            3
        3            4
        4            3
    其中 1 2 是 local_fd
        3 4 是  upstream_fd
*/
int streams[EVSIZE][2];
//第一个 listen_fd 的值
static int offset;


//[Error] [13:01:22] [tid 6711] [main.c:23 Set_nonblocking()] Set_nonblocking
void build_log(int level,long int tid,char * func,char * file,
               int line,char * msg);
//e.g 13:01:22
char get_time(void)
int Set_nonblocking(int sockfd);
void Socketpair(int * socks);
int Init_epfd(void);
void Add_epoll_fd(int epfd,int fd,__uint32_t events);
void Del_epoll_fd(int epfd,int fd);
void Mod_epoll_fd(int epfd,int fd,__uint32_t events);
void Print_fd_host_addr(int fd,struct sockaddr_in addr);
void bind_stream(int local_fd,int upstream_fd);
void Init_stream(int listenfd);
int another_fd(int fd);
void Accept_fd(int epfd,int listenfd);
void Pipe(int * pipe);
void forward_data(int src_fd,int dest_fd);
void full_msghdr(struct msghdr * msg,struct iovec * iov,size_t iov_size);
struct task build_task(int fd,enum mode mode);
struct task recv_task(int sockpair);
int send_task(int sockpair,struct task task);
void add_tasks(int epfd,int sockpair);
void print_evmode(__uint32_t ev);
void New_forward_thread(int * sockpair);

//thread
void forward(int  * sockpair_p);

int main(void)
{
    int listenfd = Open_listenfd(PORT),
        epfd     = Init_epfd();
        offset   = listenfd;
    PRINT_NUM(listenfd);
    Set_nonblocking(listenfd);

    int sockpair[2];
    Socketpair(sockpair);
    Set_nonblocking(sockpair[0]);
    Set_nonblocking(sockpair[1]);
    New_forward_thread(&sockpair[THREAD]);

    Add_epoll_fd(epfd,listenfd,EPOLLIN | EPOLLOUT);
    Add_epoll_fd(epfd,sockpair[MAIN],EPOLLIN);
    struct epoll_event evs[EVSIZE];

    for(int epfs = 0;;){
        epfs = epoll_wait(epfd,evs,EVSIZE,-1);

        for(int i = 0,fd = evs[i].data.fd,ev = evs[i].events;i < epfs;i++){
            
            if(ev & (EPOLLERR | EPOLLHUP)){
                fprintf(stderr,"epoll error %d\n",fd);
                Close(fd);
                continue;
            }

            if(fd == listenfd){
                Accept_fd(epfd,fd);
                continue;
            }

            if(fd == sockpair[MAIN] && ev & EPOLLIN){
                struct task task = recv_task(sockpair[MAIN]);
                task = recv_task(sockpair[MAIN]);

                switch(task.mode){
                    case Closefd:
                        fprintf(stderr,"fd %d Closefd\n",task.fd);
                        Close(task.fd);
                        break;                        
                    default: ;
                }
                
                continue;
                
            }

            if(ev & EPOLLIN){
                Del_epoll_fd(epfd,fd);
                Init_stream(fd);
                send_task(sockpair[MAIN],build_task(fd,Forward));
                continue;
            }
        }
    }
    Close(listenfd);
    Close(epfd);
    return 0;
}

void forward(int  * sockpair_p)
{
    int sockpair = *sockpair_p;
    int epfd = Init_epfd();
    struct epoll_event evs[EVSIZE];
    Add_epoll_fd(epfd,sockpair,EPOLLIN);

    for(int epfs = 0;;){
        epfs = epoll_wait(epfd,evs,EVSIZE,-1);

        for(int i = 0,fd = evs[i].data.fd,ev = evs[i].events;i < epfs;i++){

            if(ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)){
                Del_epoll_fd(epfd,fd);
                Del_epoll_fd(epfd,another_fd(fd));
                print_evmode(ev);
                PRINT_NUM(fd);
                PRINT_NUM(another_fd(fd));
                //send_task(sockpair,build_task(fd,Closefd));
                send_task(sockpair,build_task(another_fd(fd),Closefd));
                send_task(sockpair,build_task(another_fd(fd),Closefd));

                send_task(sockpair,build_task(fd,Closefd));
                send_task(sockpair,build_task(fd,Closefd));

                continue;
            }

            if(fd != sockpair && ev & EPOLLIN)
                forward_data(fd,another_fd(fd));

            if(fd == sockpair && ev & EPOLLIN){
                
                add_tasks(epfd,sockpair);
                continue;
            }

        }
    }
}

int Set_nonblocking(int sockfd)
{
    int flags = fcntl (sockfd, F_GETFL, 0);

    if(flags == -1)
        unix_error("Set_nonblocking error");
    if(fcntl (sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
        unix_error("Set_nonblocking error");

    return 0;
}

void Socketpair(int * socks)
{
    if(socketpair(AF_UNIX,SOCK_STREAM,0,socks) == -1)
        unix_error("Socketpair error");
}

int Init_epfd(void)
{
    int epfd = epoll_create1(EPOLL_CLOEXEC);
    if(epfd == -1)
        unix_error("Epoll_create error");
    return epfd;
}

void Add_epoll_fd(int epfd,int fd,__uint32_t events)
{
    struct epoll_event event = {};
    event.data.fd = fd;
    event.events = events;

    if(epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&event) == -1)
        unix_error("add_epoll_fd error");
}
    
void Mod_epoll_fd(int epfd,int fd,__uint32_t events)
{
    struct epoll_event event = {};
    event.data.fd = fd;
    event.events = events;

    if(epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&event) == -1)
        unix_error("mod_epoll_fd error");
}

void Del_epoll_fd(int epfd,int fd)
{
    if(epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL) == -1)
        unix_error("del_epoll_fd error");
}

void Print_fd_host_addr(int fd,struct sockaddr_in addr)
{
    char hbuf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, hbuf, sizeof(hbuf));
    fprintf(stderr,"accept fd %d %s:%d\n",fd,hbuf,addr.sin_port);
}



void bind_stream(int local_fd,int upstream_fd)
{
    streams[local_fd - offset][0] = local_fd;
    streams[local_fd - offset][1] = upstream_fd;
    streams[upstream_fd - offset][0] = upstream_fd;
    streams[upstream_fd - offset][1] = local_fd;
}

void Init_stream(int listenfd)
{
    int upstream_fd = Open_clientfd(PROXY,PROXY_PORT);
    Set_nonblocking(upstream_fd);
    bind_stream(listenfd,upstream_fd);
}

int another_fd(int fd)
{
    return streams[fd - offset][1];
}

void Accept_fd(int epfd,int listenfd)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    for(;;){
        int connfd = accept(listenfd,(SA *)&addr,&len);
        if(connfd == -1){
            if((errno == EAGAIN) || (errno == EWOULDBLOCK))
                break;
            else
                unix_error("Accept_fd error");
        }
        Print_fd_host_addr(connfd,addr);
        Set_nonblocking(connfd);
        Add_epoll_fd(epfd,connfd,EPOLLIN | EPOLLONESHOT);
    }
}

void Pipe(int * pipe)
{
    if(pipe2(pipe,0) == -1)
        unix_error("Pipe error");
}

void forward_data(int src_fd,int dest_fd)
{/*
    int pipe[2];
    Pipe(pipe);
    Set_nonblocking(pipe[0]);
    Set_nonblocking(pipe[1]);

    splice(src_fd,NULL,pipe[1],NULL,SSIZE_MAX,SPLICE_FLAGS);
    splice(pipe[0],NULL,dest_fd,NULL,SSIZE_MAX,SPLICE_FLAGS);
    */
    char buf[2048];
    int n = read(src_fd,buf,2048);
    write(dest_fd,buf,n);
}

void full_msghdr(struct msghdr * msg,struct iovec * iov,size_t iov_size)
{
    msg->msg_name = NULL;
    msg->msg_namelen = 0;

    msg->msg_iov     = iov;
    msg->msg_iovlen  = iov_size;

    msg->msg_control = NULL;
    msg->msg_controllen = 0;
    msg->msg_flags = 0;
}

struct task build_task(int fd,enum mode mode)
{
    struct task task;
    task.fd = fd;
    task.mode = mode;
    return task;
}

struct task recv_task(int sockpair)
{
    //fprintf(stderr,"send_task fd %d\n",fd);
    struct task task;
    struct msghdr msg;
    struct iovec iov[1];

    iov[0].iov_base = &task;
    iov[0].iov_len  = sizeof(struct task);
    full_msghdr(&msg,iov,1);

    if(recvmsg(sockpair,&msg,0) < 0)
        task.fd = -1;
    return task;

    //write(sockpair,&fd,sizeof(fd));
}

int send_task(int sockpair,struct task task)
{
    struct msghdr msg;
    struct iovec iov[1];

    iov[0].iov_base = &task;
    iov[0].iov_len  = sizeof(struct task);

    full_msghdr(&msg,iov,1);

    return sendmsg(sockpair,&msg,0);

    /*
    int fd;
    if(read(sockpair,&fd,sizeof(fd)) == -1)
        return -1;
    return fd;
    */
}

void add_tasks(int epfd,int sockpair)
{
    struct task task = recv_task(sockpair);
    for(;task.fd != -1;task = recv_task(sockpair)){
        fprintf(stderr,"add_task fd %d\n",task.fd);
        Add_epoll_fd(epfd,task.fd,EPOLLIN);
        Add_epoll_fd(epfd,another_fd(task.fd),EPOLLIN);
    }
}

void print_evmode(__uint32_t ev)
{
    int offset = 0;
    char modes[100];
    if(ev & EPOLLERR)
        offset = snprintf(modes,100,"EPOLLERR ");
    if(ev & EPOLLHUP)
        offset += snprintf(modes + offset,100,"EPOLLHUP ");
    if(ev & EPOLLRDHUP)
        offset += snprintf(modes + offset,100,"EPOLLRDHUP ");
    if(ev & EPOLLET)
        offset += snprintf(modes + offset,100,"EPOLLET ");
    if(ev & EPOLLONESHOT)
        offset += snprintf(modes + offset,100,"EPOLLONESHOT ");
    if(ev & EPOLLIN)
        offset += snprintf(modes + offset,100,"EPOLLIN ");
    if(ev & EPOLLPRI)
        offset += snprintf(modes + offset,100,"EPOLLPRI ");
    if(ev & EPOLLOUT)
        offset += snprintf(modes + offset,100,"EPOLLOUT ");
    if(ev & EPOLLWAKEUP)
        offset += snprintf(modes + offset,100,"EPOLLWAKEUP ");
    fprintf(stderr,"evmode %s\n",modes);
}

void New_forward_thread(int * sockpair)
{
    pthread_t pth;
    pthread_create(&pth,NULL,(void *)forward,sockpair);
}

void log(int level,long int tid,char * func,                      char * file,int line,char * msg)
{
    snprintf(buf,size,"")
}