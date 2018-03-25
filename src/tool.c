#include "tool.h"

//[Error] [13:01:22] [tid 6711] [main.c:23 Set_nonblocking()] Set_nonblocking
void print_log(enum log_level level,const char * func,char * file,int line,char * format,...)
{
    va_list ap;
    va_start(ap,format);

    char msg[1024];
    vsnprintf(msg,1024,format,ap);

    time_t t;
    time(&t);   
    size_t timelen = 255;
    char * mode = "Error",* color = RED; //init
    char time[timelen];
    strftime(time,timelen,"%F %T",localtime(&t));

    FILE * stream = stderr;
    switch(level){
        case Error : mode = "Error", stream = stderr;color = RED    ;break;
        case Warn  : mode = "Warn" , stream = stderr; color = YELLOW ;break;
        case Info: mode = "Info",stream = stderr; color = GREEN  ;break;
        case Debug : mode = "Debug", stream = stderr;color = BLUE   ;break;
        case Error_exit : mode = "Error_exit",stream = stderr;color = RED;break;
    }
    
    fprintf(stream,"%s[%s] [%s] [tid %ld] [%s:%d %s()] \033[0m %s\n",
            color,mode,time,gettid(),file,line,func,msg);
}

void Add_epoll_fd(int epfd,int fd,__uint32_t events)
{
    struct epoll_event event = {};
    event.data.fd = fd;
    event.events = events;

    if(epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&event) == -1)
        log_error("epfd:%d fd:%d %s",epfd,fd,strerror(errno));
}

void Mod_epoll_fd(int epfd,int fd,__uint32_t events)
{
    struct epoll_event event = {};
    event.data.fd = fd;
    event.events = events;

    if(epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&event) == -1)
        log_error("epfd:%d fd:%d %s",epfd,fd,strerror(errno));
}

void Del_epoll_fd(int epfd,int fd)
{
    if(epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL) == -1)
        log_error("epfd:%d fd:%d %s",epfd,fd,strerror(errno));
}

int Init_epfd(void)
{
    int epfd = epoll_create1(EPOLL_CLOEXEC);
    if(epfd == -1)
        log_strerror();
    return epfd;
}

void print_evmode(int fd,__uint32_t ev)
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
    log_info("fd:%d evmode %s",fd,modes);
}

int Set_nonblocking(int sockfd)
{
    int flags = fcntl(sockfd,F_GETFL,0);

    if(flags == -1)
        log_error("fd:%d %s",sockfd,strerror(errno));
    if(fcntl(sockfd,F_SETFL,flags|O_NONBLOCK) == -1)
        log_error("fd:%d %s",sockfd,strerror(errno));
    return 0;
}

void Socketpair(int * socks)
{
    if(socketpair(AF_UNIX,SOCK_STREAM,0,socks) == -1)
        log_strerror();
}

void Close(int fd)
{
    if(close(fd) < 0)
        log_strerror();
}

//from csapp.c
static int open_clientfd(char * hostname, int port)
{
    int clientfd;
    struct hostent * hp;
    struct sockaddr_in serveraddr;
    
    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1; /* check errno for cause of error */
    
    /* Fill in the server's IP address and port */
    if ((hp = gethostbyname(hostname)) == NULL)
        return -2; /* check h_errno for cause of error */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)hp->h_addr_list[0],
          (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
    serveraddr.sin_port = htons(port);
    
    /* Establish a connection with the server */
    if (connect(clientfd, (SA *) &serveraddr, sizeof(serveraddr)) < 0)
        return -1;
    return clientfd;
}

//from csapp.c
int Open_clientfd(char *hostname, int port) 
{
    int rc;
    
    if ((rc = open_clientfd(hostname, port)) < 0){
        if (rc == -1){
            log_strerror();
        }else{
            log_error("Dns error: %s",strerror(errno));
        }
    }
    return rc;
}

//from csapp.c
int open_listenfd(int port)
{
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;
    
    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;
    
    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval , sizeof(int)) < 0)
        return -1;
    
    /* Listenfd will be an endpoint for all requests to port
     on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddr.sin_port = htons((unsigned short)port); 
    if (bind(listenfd, (SA *)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;
    
    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, listenq) < 0)
        return -1;
    return listenfd;
}

//from csapp.c
int Open_listenfd(int port) 
{
    int rc;
    
    if ((rc = open_listenfd(port)) < 0)
        log_error_exit("%s",strerror(errno));
    return rc;
}

//ElfHash
uint64_t hash(const char* str, uint64_t len)
{
    uint64_t hash = 0;
    uint64_t x = 0;
    for(int i = 0; i < len; ++i){
        hash = (hash << 4) + (*str++);
        if((x = hash & 0xF0000000L) != 0){
            hash ^= (x >> 24);
        }
        hash &= ~x;
    }
    return hash;
}