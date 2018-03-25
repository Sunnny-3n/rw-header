#ifndef REWRITE_H_
#define REWRITE_H_

#include "httpparser.h"



//method
#define METHOD (int)src_header.method_len,src_header.method
//url
#define URL (int)src_header.path_len,src_header.path
#define OTHER_HEADERS tmp_buf.buf

//用于免流,删除 Host 头等
//删除请求头的个数
#define HTTPS_DEL_COUNT 1
#define HTTPS_DEL "Host"

//CONNECT / \nrd.go.10086.cn HTTP/1.0\n\thttp://[H]\rHost:[H]\nHost: rd.go.10086.cn\n
#define HTTPS_RW                          \
"CONNECT %.*s HTTP/1.1\r\n"                           \
"X-Online-Host: rd.go.10086.cn\r\n"                      \
"Connection: keep-alive\rX-Online-Host: %.*s\r\n"          \
"Host: rd.go.10086.cn\r\n"                        \
"%s"                                               \
"\r\n"                                              \
,value("Host"),value("Host"),OTHER_HEADERS         \


#define HTTP_DEL_COUNT 1
#define HTTP_DEL "Host"

//[M] [U] [V]\n\thttp://[H]\rHost:[H]\r\nHost: rd.go.10086.cn\r\n
#define HTTP_RW                      \
"%.*s %.*s HTTP/1.1\n"               \
"\thttp://%.*s\rHost:%.*s\r\n"       \
"Host: rd.go.10086.cn\r\n"           \
"%s"                                 \
"\r\n"                               \
,METHOD,URL,value("Host"),value("Host"),OTHER_HEADERS



void rewrite_HTTPS_header_and_send(int srcfd,int dstfd);
void rewrite_HTTP_header_and_send(int srcfd,int dstfd);

#endif