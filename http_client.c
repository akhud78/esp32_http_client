// https://github.com/espressif/esp-idf/blob/v5.1.1/examples/protocols/esp_http_client/main/esp_http_client_example.c

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_timer.h"

#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "http_client.h"

// As default is 512 without setting buffer_size property in esp_http_client_config_t
#define HTTP_CLIENT_BUFFER_SIZE    (1024 * 3)

static const char *TAG = "http_client";

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                if (evt->user_data) {
                    memcpy(evt->user_data + output_len, evt->data, evt->data_len);
                } else {
                    if (output_buffer == NULL) {
                        output_buffer = (char *) malloc(esp_http_client_get_content_length(evt->client));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    memcpy(output_buffer + output_len, evt->data, evt->data_len);
                }
                output_len += evt->data_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                output_len = 0;
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
            
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "user@example.com");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
#endif
            
    }
    return ESP_OK;
}

static int post_with_url(char *url, char *content_type, char *post_data, char *response_buffer, size_t buffer_size)
{
    ESP_LOGD(TAG, "http_post_with_url url=[%s]", url);

    int bytes = 0;

    esp_http_client_config_t config = {
        .event_handler = _http_event_handler,
        .method = HTTP_METHOD_POST,
        .disable_auto_redirect = true,
    };
    config.url = url;
    config.buffer_size = buffer_size;
    config.user_data = response_buffer;
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    esp_http_client_set_header(client, "Content-Type", content_type); 
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    
    uint64_t start = esp_timer_get_time();
    esp_err_t err = esp_http_client_perform(client);
    uint64_t run_time_ms = (esp_timer_get_time() - start)/1000;
    ESP_LOGD(TAG, "%s esp_http_client_perform %llu ms", __func__, run_time_ms);
    
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        bytes = esp_http_client_get_content_length(client);
        ESP_LOGD(TAG, "HTTP POST Status = %d, content_length = %d", status, bytes);
        if (bytes > 0 && bytes < buffer_size) {
            response_buffer[bytes] = '\0';
            ESP_LOGD(TAG, "response_buffer=[%s]", response_buffer);
        } else {  
            // D (5114) http_client: HTTP POST Status = 404, content_length = -1
            // TODO!!!
            bytes = 0;
        }
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
        bytes = 0;
    }

    esp_http_client_cleanup(client);
    return bytes;
}


bool http_client_post_plain(char *url, char *post_data)
{
    char *response_buffer = (char*)malloc(HTTP_CLIENT_BUFFER_SIZE);  
    assert(response_buffer != NULL);  
    response_buffer[0] = '\0';
    
    // text/xml application/json
    int bytes = post_with_url(url, "text/plain", post_data, response_buffer, HTTP_CLIENT_BUFFER_SIZE);
    free(response_buffer);
    
    return bytes > 0 ? true : false;
}


int http_client_post(char *url, char *content_type, char *post_data, char *response_buffer, size_t buffer_size)
{
    if (response_buffer == NULL || buffer_size == 0)
        return -1;
        
    return post_with_url(url, content_type, post_data, response_buffer, buffer_size);
}


static int get_with_url(char *url)
{
    int bytes = 0;
    
    char *response_buffer = (char*)malloc(HTTP_CLIENT_BUFFER_SIZE);    
    response_buffer[0] = '\0';
    /**
     * NOTE: All the configuration parameters for http_client must be spefied either in URL or as host and path parameters.
     * If host and path parameters are not set, query parameter will be ignored. In such cases,
     * query parameter should be specified in URL.
     *
     * If URL as well as host and path parameters are specified, values of host and path will be considered.
     */
    esp_http_client_config_t config = {
        //.host = "httpbin.org",
        //.path = "/get",
        //.query = "esp",
        .event_handler = _http_event_handler,
        .user_data = response_buffer,        // Pass address of local buffer to get response
        .disable_auto_redirect = true,
    };
    config.url = url;
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // GET
    ESP_LOGD(TAG, "HTTP GET url: %s ", config.url);
    
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        bytes = esp_http_client_get_content_length(client);
        if (bytes >= 0) {
            response_buffer[bytes] = '\0';
        }
        ESP_LOGD(TAG, "HTTP GET Status = %d, content_length = %d", status, bytes);
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        bytes = 0; // ERROR
    }
    
    ESP_LOG_BUFFER_HEX(TAG, response_buffer, strlen(response_buffer));
    
    esp_http_client_cleanup(client);
    free(response_buffer);
    return bytes;
}

// https://github.com/espressif/esp-idf/blob/release/v5.1/examples/protocols/esp_http_client/main/esp_http_client_example.c#L679
// buffer - You MUST free the pointer once you are done with it!
int http_client_get_native(char *url, char **buffer)
{
    int bytes = 0;
    
    esp_http_client_config_t config = {
        .auth_type = HTTP_AUTH_TYPE_BASIC,
        .max_authorization_retries = -1,
    };
    config.url = url;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        return 0; 
    }    

    esp_err_t err;
    if ((err = esp_http_client_open(client, 0)) != ESP_OK) { 
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        return 0;
    }

    int content_length = esp_http_client_fetch_headers(client);
    if (content_length <= 0) {
        return 0;
    }
    size_t buffer_size = content_length + 1;
    *buffer = (char*)calloc(buffer_size, sizeof(char));
    if (*buffer == NULL) {
        ESP_LOGE(TAG, "%s Memory allocation for %d bytes failed", __func__, buffer_size); 
        return 0;
    }

    int read_len = esp_http_client_read(client, *buffer, content_length);
    if (read_len <= 0) {
        ESP_LOGE(TAG, "Error read data");
        free(*buffer);
        return 0; 
    }
    buffer[read_len] = 0;
    bytes = read_len;

    ESP_LOGD(TAG, "status_code: %d, content_length: %d", esp_http_client_get_status_code(client),
                    (uint32_t)esp_http_client_get_content_length(client));
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    
    return bytes;
}

// https://github.com/espressif/esp-idf/blob/release/v5.2/examples/protocols/esp_http_client/main/esp_http_client_example.c
// https://github.com/espressif/esp-idf/blob/master/examples/protocols/esp_http_client/main/esp_http_client_example.c#L718C5-L718C57
static int reader(char *url, char *buffer, int len)
{
    int bytes = 0;

    esp_http_client_config_t config = {
        .auth_type = HTTP_AUTH_TYPE_BASIC,
        .max_authorization_retries = -1,
    };
    config.url = url;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) { // if any errors
        ESP_LOGE(TAG, "%s init", __func__);
        return 0; 
    }
    
    esp_err_t err;
    if ((err = esp_http_client_open(client, -1)) != ESP_OK) {  // write_len=-1 sets header "Transfer-Encoding: chunked" 
                                                               // and method to POST
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        return 0;
    }

    int content_length = esp_http_client_fetch_headers(client);
    if (content_length <= 0) {
        ESP_LOGE(TAG, "%s fetch", __func__);
    }
    int total_read_len = 0, read_len;
    
    if (content_length > len) {
        ESP_LOGW(TAG, "content_length: %d > len: %d", content_length, len);
    } else {        
        if (total_read_len < content_length) {
            read_len = esp_http_client_read(client, buffer, content_length);
            if (read_len <= 0) {
                ESP_LOGE(TAG, "Error read data");
                bytes = 0; 
            }
            buffer[read_len] = 0;
            //ESP_LOG_BUFFER_HEX(TAG, buffer, 8); // first bytes
            bytes = read_len;
        }
    }
    ESP_LOGD(TAG, "status_code: %d, content_length: %d", esp_http_client_get_status_code(client),
                    (uint32_t)esp_http_client_get_content_length(client));
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    
    return bytes;
}

// buffer - You MUST free the pointer once you are done with it!
int http_client_loader(char *url, char **buffer)
{
    int bytes = 0;
    
    esp_http_client_config_t config = {
        .auth_type = HTTP_AUTH_TYPE_BASIC,
        .max_authorization_retries = -1,
    };
    config.url = url;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        return 0; 
    }    

    esp_err_t err;
    if ((err = esp_http_client_open(client, -1)) != ESP_OK) {  // write_len=-1 sets header "Transfer-Encoding: chunked" 
                                                               // and method to POST
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        return 0;
    }

    int content_length = esp_http_client_fetch_headers(client);
    if (content_length <= 0) {
        return 0;
    }
    size_t buffer_size = content_length + 1;
    *buffer = (char*)heap_caps_malloc(buffer_size, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    if (*buffer == NULL) {
        ESP_LOGE(TAG, "%s Memory allocation for %d bytes failed", __func__, buffer_size); 
        return 0;
    }

    int read_len = esp_http_client_read(client, *buffer, content_length);
    if (read_len <= 0) {
        ESP_LOGE(TAG, "Error read data");
        free(*buffer);
        return 0; 
    }
    buffer[read_len] = 0;
    bytes = read_len;

    ESP_LOGD(TAG, "status_code: %d, content_length: %d", esp_http_client_get_status_code(client),
                    (uint32_t)esp_http_client_get_content_length(client));
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    
    return bytes;
}


/*
static void _get_with_url_task(void *pvParameters)
{
    get_with_url();
    
    vTaskDelay(4000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "_get_with_url_task finished.");
    vTaskDelete(NULL);
}
*/

bool http_client_get(char *url)
{
    //xTaskCreate(&_get_task, "_get_with_url_task", 8192, NULL, 5, NULL);
    
    int bytes = get_with_url(url);
    
    return bytes > 0 ? true : false;
}


int http_client_reader(char *url, char *buffer, int len)
{
    
    return reader(url, buffer, len);
}







