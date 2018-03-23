#include "httpparser.h"

int parse_req(struct bufer buf,struct header * header)
{
    return phr_parse_request(buf.buf,buf.len,
        (const char **)&(header->method),&(header->method_len),
        (const char **)&(header->path),&(header->path_len),
        &(header->minor_version),
        header->headers,&(header->headers_size),0);
}

int readall(struct bufer * buf,int fd)
{
    buf->len = read(fd,buf->buf,max_bufsize);
    ssize_t n = buf->len;
    for(;n > 0;n = read(fd,buf->buf + buf->len,max_bufsize - buf->len))
        buf->len += n;
    if((n == -1) && (errno != EAGAIN))
        return n;
    else
        return buf->len;
}

struct header view_header(struct bufer * buf,int fd)
{
    struct header header;
    buf->len = recv(fd,buf->buf,max_bufsize,MSG_PEEK);
    parse_req(*buf,&header);
    return header;
}

void view_data(int fd,struct bufer * buf)
{
    buf->len = recv(fd,buf->buf,max_bufsize,MSG_PEEK);
}

int writeall(struct bufer * buf,int fd)
{
    return 0;
}

//ElfHash
static uint64_t hash(const char* str, uint64_t len){
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