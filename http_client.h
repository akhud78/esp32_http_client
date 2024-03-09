#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

/* The examples use HTTP configuration that you can set via project configuration menu */
#define HTTP_CLIENT_URI_GET         CONFIG_HTTP_CLIENT_URI_GET      // http://httpbin.org/get
#define HTTP_CLIENT_URI_POST        CONFIG_HTTP_CLIENT_URI_POST     // "http://httpbin.org/post"
#define HTTP_CLIENT_URI_IMAGE       CONFIG_HTTP_CLIENT_URI_IMAGE    // http://httpbin.org/image/jpeg
#define HTTP_CLIENT_LIST_NAME       "list.txt"
        
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

bool http_client_get(char *url);
int http_client_reader(char *url, char *buffer, int len);
int http_client_loader(char *url, char **buffer);

bool http_client_post_plain(char *url, char *post_data);
int http_client_post(char *url, char *content_type, char *post_data, char *response_buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif


#endif // HTTP_CLIENT_H 
