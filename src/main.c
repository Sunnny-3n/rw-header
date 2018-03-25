#include "tool.h"

static int offset = 0;
struct map maps[mapsize];

struct args Parse_args(int argc,char * argv[]);
enum tag gettag(int fd);
bool isconnfd(int fd);
bool isproxyfd(int fd);
int another_fd(int fd);
void settag(int fd,enum tag tag);
void close_link(int epfd,int fd);
void Deal_error_fd(int epfd,int fd);
void Accepf_all_and_init_link(int epfd,int listenfd,char * paddr,int pport);

int main(int argc,char * argv[])
{
    argc = 4;
    //argv[1] = "9998",argv[2] = "192.168.1.1",argv[3] = "8888";
    struct args args = Parse_args(argc,argv);
    struct epoll_event evs[evsize];
    int listenfd = Open_listenfd(args.lport),
        epfd     = Init_epfd();
    offset   = listenfd;

    //防止管道破裂,进程被杀死
    signal(SIGPIPE,SIG_IGN);

    Set_nonblocking(listenfd);
    Add_epoll_fd(epfd,listenfd,EPOLLIN | EPOLLOUT | EPOLLET);
    for(int epfs = 0;;){
        epfs = epoll_wait(epfd,evs,evsize,-1);

        for(int i = 0,fd = evs[i].data.fd,ev = evs[i].events;i < epfs;i++){
            if((ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) && (fd != listenfd)){
                Deal_error_fd(epfd,fd);
                continue;
            }
            if((ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) && (fd == listenfd))
                log_error_exit("listenfd:%d error",fd);
            if(fd == listenfd){
                Accepf_all_and_init_link(epfd,fd,args.paddr,args.pport);
                continue;
            }
            if(isconnfd(fd) && (gettag(fd) == Connect) && (ev & EPOLLIN)){
                if(isHTTP(fd)){
                    settag(fd,HTTP);
                    goto HTTP;
                }
                if(isHTTPS(fd)){
                    settag(fd,HTTPS);
                    goto HTTPS;
                }
                log_error("the connection isn't HTTP or HTTPS,will close "
                          "connection");
                close_link(epfd,fd);
            }
            switch(gettag(fd)){
            case HTTP:
HTTP:
                if(isconnfd(fd) && (ev & EPOLLIN))
                    rewrite_HTTP_header_and_send(fd,another_fd(fd));
                if(isproxyfd(fd))
                    goto Forward;
                continue;
            case HTTPS:
HTTPS:
                if(isconnfd(fd) && (ev & EPOLLIN))
                    rewrite_HTTPS_header_and_send(fd,another_fd(fd));
                if(isproxyfd(fd)){
                    settag(fd,Forward);
                    goto Forward;
                }
                continue;
                
            case Forward:
Forward:
                if(ev & EPOLLIN){
                    int bsize = 4096 * 256;
                    char buf[bsize];
                    for(int n = read(fd,buf,bsize);n > 0;n = read(fd,buf,bsize))
                        write(another_fd(fd),buf,n);
                }
                continue;
            default:
                continue;
            }

        }
    }
}



struct args Parse_args(int argc,char * argv[])
{
    struct args args;
    //./rw-header 9999 127.0.0.1 8888
    if(argc != 4)
        log_error_exit("missing args.\n\
        args listen_port proxy_addr proxy_port");

    args.lport = atoi(argv[1]);
    args.paddr = argv[2];
    args.pport = atoi(argv[3]);
    
    return args;
}

static int pos(int fd)
{
    return fd - offset;
}

bool isconnfd(int fd)
{
    if(maps[pos(fd)].connfd == fd)
        return true;
    else
        return false;
}

bool isproxyfd(int fd)
{
    if(maps[pos(fd)].proxyfd == fd)
        return true;
    else
        return false;
}

int another_fd(int fd)
{
    if(isconnfd(fd))
        return maps[pos(fd)].proxyfd;
    if(isproxyfd(fd))
        return maps[pos(fd)].connfd;
    return -1;
}

enum tag gettag(int fd)
{
    return maps[pos(fd)].tag;
}

void Deal_error_fd(int epfd,int fd)
{
    if(isconnfd(fd))
        log_warn("localfd:%d is closed and will close proxyfd:%d",
                          fd,                 another_fd(fd));
    if(isproxyfd(fd))
        log_warn("proxyfd:%d is closed and will close localfd:%d",
                          fd,                  another_fd(fd));
    Del_epoll_fd(epfd,fd);
    Close(fd);
    Del_epoll_fd(epfd,another_fd(fd));
    Close(another_fd(fd));
}

void print_fd_host_addr(int fd,struct sockaddr_in addr)
{
    char hbuf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, hbuf, sizeof(hbuf));
    log_info("accept fd %d %s:%d",fd,hbuf,addr.sin_port);
}

void settag(int fd,enum tag tag)
{
    maps[pos(fd)].tag = tag;
    maps[pos(another_fd(fd))].tag = tag;
}

static void setlink(int connfd,int proxyfd)
{
    maps[pos(connfd)].connfd = connfd;
    maps[pos(connfd)].proxyfd = proxyfd;
    maps[pos(proxyfd)].connfd = connfd;
    maps[pos(proxyfd)].proxyfd = proxyfd;
}


void Init_link(int connfd,char * paddr,int pport)
{
    int proxyfd = Open_clientfd(paddr,pport);
    Set_nonblocking(connfd);
    Set_nonblocking(proxyfd);
    setlink(connfd,proxyfd);
    settag(connfd,Connect);
}

void Accepf_all_and_init_link(int epfd,int listenfd,char * paddr,int pport)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    for(;;){
        int connfd = accept(listenfd,(SA *)&addr,&len);
        if(connfd == -1){
            if((errno == EAGAIN) || (errno == EWOULDBLOCK))
                break;
            else
                log_strerror();
        }
        print_fd_host_addr(connfd,addr);
        Init_link(connfd,paddr,pport);
        Add_epoll_fd(epfd,connfd,EPOLLIN | EPOLLET);
        Add_epoll_fd(epfd,another_fd(connfd),EPOLLIN | EPOLLET);
    }
}

void close_link(int epfd,int fd)
{
    int anotherfd = another_fd(fd);
    Del_epoll_fd(epfd,fd);
    Del_epoll_fd(epfd,anotherfd);
    Close(fd);
    Close(anotherfd);
    log_info("close link fd:%d fd:%d",fd,anotherfd);
}