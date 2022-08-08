#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

/* The examples use HTTP configuration that you can set via project configuration menu */
#define HTTP_CLIENT_URI_GET           CONFIG_HTTP_CLIENT_URI_GET    // http://httpbin.org/get
#define HTTP_CLIENT_URI_IMAGE         CONFIG_HTTP_CLIENT_URI_IMAGE  // http://httpbin.org/image/jpeg

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t http_client_get(char *url);
int http_client_reader(char *url, char *buffer, int len);

#ifdef __cplusplus
}
#endif


#endif // HTTP_CLIENT_H 
