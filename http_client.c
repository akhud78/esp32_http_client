/* ESP HTTP Client Example

    https://github.com/espressif/esp-idf/blob/v4.3/examples/protocols/esp_http_client/main/esp_http_client_example.c
*/

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_system.h"

#include "esp_event.h"
#include "esp_netif.h"

#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"

#include "sdkconfig.h"
#include "http_client.h"

#define MAX_HTTP_RECV_BUFFER        512
#define MAX_HTTP_OUTPUT_BUFFER      1024 //2048


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
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                if (output_buffer != NULL) {
                    free(output_buffer);
                    output_buffer = NULL;
                }
                output_len = 0;
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            break;
    }
    return ESP_OK;
}


static int _get_with_url(char *url)
{
    int ret = 0;  // OK
    
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
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
        .query = "esp",
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer,        // Pass address of local buffer to get response
        .disable_auto_redirect = true,
    };
    config.url = url;
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // GET
    ESP_LOGI(TAG, "HTTP GET url: %s ", config.url);
    
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        ret = -1; // ERROR
    }
    
    ESP_LOG_BUFFER_HEX(TAG, local_response_buffer, strlen(local_response_buffer));
    
    
    esp_http_client_cleanup(client);
    
    return ret;
}

static int _reader(char *url, char *buffer, int len)
{
    int bytes = 0; // OK

    esp_http_client_config_t config = {
        .auth_type = HTTP_AUTH_TYPE_BASIC,
        .max_authorization_retries = -1,
    };
    config.url = url;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) { // if any errors
        ESP_LOGE(TAG, "%s %d", __func__, __LINE__);
        return 0; 
    }
        
    ESP_LOGI(TAG, "HTTP url: %s ", config.url);
    
    esp_err_t err;
    if ((err = esp_http_client_open(client, 0)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        return 0;
    }

    int content_length = esp_http_client_fetch_headers(client);
    if (content_length <= 0) {
        ESP_LOGE(TAG, "%s %d", __func__, __LINE__);
    }
    
    int total_read_len = 0, read_len;
    if (total_read_len < content_length && content_length <= len) {
        read_len = esp_http_client_read(client, buffer, content_length);
        if (read_len <= 0) {
            ESP_LOGE(TAG, "Error read data");
            bytes = 0; 
        }
        buffer[read_len] = 0;
        ESP_LOGI(TAG, "read_len = %d", read_len);
        ESP_LOG_BUFFER_HEX(TAG, buffer, 8); // first bytes
        bytes = read_len;
    }
    ESP_LOGI(TAG, "HTTP Stream reader Status = %d, content_length = %d",
                    esp_http_client_get_status_code(client),
                    esp_http_client_get_content_length(client));
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    
    return bytes;
}

/*
static void _get_with_url_task(void *pvParameters)
{
    _get_with_url();
    
    vTaskDelay(4000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "_get_with_url_task finished.");
    vTaskDelete(NULL);
}
*/

esp_err_t http_client_get(char *url)
{
    esp_err_t ret = ESP_OK;
    
    //xTaskCreate(&_get_task, "_get_with_url_task", 8192, NULL, 5, NULL);
    
    if (_get_with_url(url)) ret = ESP_FAIL;
    
    return ret;
}


int http_client_reader(char *url, char *buffer, int len)
{
    
    return _reader(url, buffer, len);
}



