#include "httpparser.h"

static void init_header(struct header * header)
{
    header->headers_size = max_headers;
}

int parse_req(struct bufer buf,struct header * header)
{
    init_header(header);
    return phr_parse_request(buf.buf,buf.len,
        (const char **)&(header->method),&(header->method_len),
        (const char **)&(header->path),&(header->path_len),
        &(header->minor_version),
        header->headers,&(header->headers_size),0);
}

ssize_t readall(struct bufer * buf,int fd)
{
    errno = 0;
    ssize_t n = 0;
    for(n = read(fd,buf->buf,max_bufsize);
        (n > 0) & (errno != 0) & (n < max_bufsize);
        n += read(fd,buf->buf + n,max_bufsize - n)){
        ;
    }
    if(errno == EAGAIN)
        errno = 0;
    buf->len = n;
    return n;
}

void view_data(int fd,struct bufer * buf)
{
    buf->len = recv(fd,buf->buf,max_bufsize,MSG_PEEK);
}

ssize_t writeall(struct bufer buf,int fd)
{
    errno = 0;
    ssize_t n = 0;

    for(;n < buf.len;n += write(fd,buf.buf + n,buf.len - n))
        ;
    return n;
}

static int cut(const char * s,const char c)
{
    for(int i = 0;s[i] != '\0';i++){
        if(s[i] == c)
            return i;
    }
    return 0;
}

#define GET  0x4ba4 //hash("GET",3);
#define HEAD 0x4c954 //hash("HEAD",4);
#define POST 0x55484 //hash("POST",4);
#define PUT  0x55a4 //hash("PUT",3);
#define DELETE 0x48a0a85 //hash("DELETE",6);
#define TRACE  0x596575 //hash("TRACE",5);
#define OPTIONS 0x458e463 //hash("OPTIONS",7);
#define CONNECT 0x84329c4 //hash("CONNECT",7);
//print hash value
//log_info("0x%x",GET);
//log_info("0x%x",HEAD);
//log_info("0x%x",POST);
//log_info("0x%x",PUT);
//log_info("0x%x",DELETE);
//log_info("0x%x",TRACE);
//log_info("0x%x",OPTIONS);
//log_info("0x%x",CONNECT);

bool isHTTP(int fd)
{ 
    struct bufer buf;
    view_data(fd,&buf);
    int method_len = cut(buf.buf,' ');

    switch(hash(buf.buf,method_len)){
        case GET:
        case HEAD:
        case POST:
        case PUT:
        case DELETE:
        case TRACE:
        case OPTIONS:
            return true;
        case CONNECT:
            return false;
        default:
            log_error("Bad header\n%s",buf.buf);
            buf.buf[method_len] = '\0';
            log_error("unknow method: %s",buf.buf);
            return false;
    }
}
bool isHTTPS(int fd)
{
    struct bufer buf;
    view_data(fd,&buf);
    int method_len = cut(buf.buf,' ');

    switch(hash(buf.buf,method_len)){
        case CONNECT:
            return true;
        case GET:
        case HEAD:
        case POST:
        case PUT:
        case DELETE:
        case TRACE:
        case OPTIONS:
            return false;
        default:
            log_error("Bad header\n%s",buf.buf);
            buf.buf[method_len] = '\0';
            log_error("unknow method: %s",buf.buf);
            return false;
    }
}

#undef GET  
#undef HEAD 
#undef POST
#undef PUT  
#undef DELETE 
#undef TRACE 
#undef OPTIONS
#undef CONNECT 

struct phr_header * pos_header_field(struct phr_header headers[],
                                     size_t headers_size,
                                     const char * field_name,int len)
{
    for(size_t i = 0;i < headers_size;i++){

        const char * name = headers[i].name;
        size_t name_len = headers[i].name_len;
        
        if(hash(name,name_len) == hash(field_name,len))
            return &headers[i];
    }
    return NULL;
}