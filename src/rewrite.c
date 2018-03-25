#include "rewrite.h"

static void init_hash_table(uint64_t hash_table[],int table_size,va_list args)
{
    for(int i = 0;i < table_size;i++){
        char * key = va_arg(args,char *);
        hash_table[i] = hash(key,strlen(key));
    }

    va_end(args);
}

static bool is_in_hash_table(uint64_t hash_table[],int table_size,uint64_t hash)
{
    for(int i = 0;i < table_size;i++){
        if(hash_table[i] == hash)
            return true;
    }

    return false;
}

/*将 headers(除了可变参数中的字段名及其参数) 打印到 buf 中去,返回成功打印的字符数
*
* -----------headers-----------
     name       |      value
*   User-Agent  | Mozilla/5.0 
*   Connection  | keep-alive
*   Host        | secure.gravatar.com:443
*
* e.g. snprintf_other_headers(buf,headers,4,2,"Host","User-Agent")
* 打印后 buf 中的内容
* Connection: keep-alive\r\n
*/
static ssize_t snprintf_other_headers(struct bufer * buf,
                                      struct phr_header headers[],              size_t headers_size,
                                      int blacklist_count,...)
{
    ssize_t n = 0;
    va_list args;
    va_start(args,blacklist_count);

    uint64_t hash_table[blacklist_count];
    init_hash_table(hash_table,blacklist_count,args);

    for(int i = 0;i < headers_size;i++){
        uint64_t hash_sum = hash(headers[i].name,headers[i].name_len);

        if(!is_in_hash_table(hash_table,blacklist_count,hash_sum)){
            n += snprintf(buf->buf + n,max_bufsize - n,"%.*s: %.*s\r\n",
                          (int)headers[i].name_len,headers[i].name,
                          (int)headers[i].value_len,headers[i].value);
        }
    }
    buf->len = n;
    return n;
}

#define pos(name) \
pos_header_field(src_header.headers,src_header.headers_size,name,strlen(name))

#define value(name) (int)pos(name)->value_len,pos(name)->value

#define COMMON                         \
    struct bufer src_buf,              \
                 tmp_buf;              \
                                       \
    struct header src_header;          \
    struct bufer out;                  \
                                       \
    readall(&src_buf,srcfd);           \
    if(src_buf.len <= 0){              \
        log_error("Buffer no data");   \
        return;                        \
    }                                  \
    parse_req(src_buf,&src_header);    \

void rewrite_HTTPS_header_and_send(int srcfd,int dstfd)
{
    //宏
    COMMON
    snprintf_other_headers(&tmp_buf,src_header.headers,src_header.headers_size,                        HTTPS_DEL_COUNT,HTTPS_DEL);
    out.len = snprintf(out.buf,max_bufsize,HTTPS_RW);
    writeall(out,dstfd);
}

void rewrite_HTTP_header_and_send(int srcfd,int dstfd)
{
    //宏
    COMMON
    snprintf_other_headers(&tmp_buf,src_header.headers,src_header.headers_size,                        HTTPS_DEL_COUNT,HTTP_DEL);
    out.len = snprintf(out.buf,max_bufsize,HTTP_RW);
    writeall(out,dstfd);
}