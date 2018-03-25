#ifndef HTTPPARSER_H_
#define HTTPPARSER_H_

#include "tool.h"
#include "picohttpparser/picohttpparser.h"

enum {
    max_headers = 8,
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
ssize_t readall(struct bufer * buf,int fd);
ssize_t writeall(struct bufer buf,int fd);

bool isHTTPS(int fd);
bool isHTTP(int fd);

struct phr_header * pos_header_field(struct phr_header headers[],
                                     size_t headers_size,
                                     const char * field_name,int len);

#endif