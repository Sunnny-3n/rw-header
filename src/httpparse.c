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