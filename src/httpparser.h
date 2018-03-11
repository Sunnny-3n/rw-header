#ifndef HTTPPARSER_H_
#define HTTPPARSER_H_

#include "tool.h"
#include "picohttpparser/picohttpparser.h"

enum {
    max_headers = 32,
    max_bufsize = 4096 * 32
};

struct bufer{
    char buf[max_bufsize];
    ssize_t len;
};

struct header{
    char * method;
    size_t method_len;
    char * path;
    size_t path_len;
    int minor_version;
    struct phr_header headers[max_headers];
    size_t headers_size;
};

int parse_req(struct bufer buf,struct header * header);

struct header view_header(struct bufer * buf,int fd);

//只适用于 nonblocking fd
int readall(struct bufer * buf,int fd);
int writeto(struct bufer * buf,int fd);


#endif