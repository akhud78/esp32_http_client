#include <string.h>
#include <ctype.h>
#include "unity.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "wifi.h"
#include "http_client.h"

static const char TAG[] = "test_http_client";

TEST_CASE("get", "[client]")
{    
    TEST_ASSERT_EQUAL(ESP_OK, wifi_sta_start(WIFI_STA_SSID, WIFI_STA_PASS, NULL, 0,0));
    TEST_ASSERT_GREATER_THAN(0, http_client_get(HTTP_CLIENT_URI_GET));
    wifi_sta_stop();
}

TEST_CASE("post", "[client]")
{    
    TEST_ASSERT_EQUAL(ESP_OK, wifi_sta_start(WIFI_STA_SSID, WIFI_STA_PASS, NULL, 0,0));
    TEST_ASSERT_GREATER_THAN(0, http_client_post_plain(HTTP_CLIENT_URI_POST, "hello"));

    wifi_sta_stop();
}


TEST_CASE("image reader", "[client]")
{
    const int BUFFER_LEN = 60 * 1024;
    char *buffer = malloc(BUFFER_LEN);
    TEST_ASSERT_NOT_NULL(buffer); 
    
    TEST_ASSERT_EQUAL(ESP_OK, wifi_sta_start(WIFI_STA_SSID, WIFI_STA_PASS, NULL, 0,0));
    int bytes = http_client_reader(HTTP_CLIENT_URI_IMAGE, buffer, BUFFER_LEN);
    free(buffer);
    wifi_sta_stop();
    TEST_ASSERT_GREATER_THAN(0, bytes);
}

TEST_CASE("image get native", "[client]")
{
    TEST_ASSERT_EQUAL(ESP_OK, wifi_sta_start(WIFI_STA_SSID, WIFI_STA_PASS, NULL, 0,0));
    
    char *buffer = NULL;
    bool chunked = false;
    int bytes = http_client_get_native(HTTP_CLIENT_URI_IMAGE, &buffer, chunked);
    if (bytes > 0) {
        free(buffer);
    }
    TEST_ASSERT_GREATER_THAN(0, bytes);    
    ESP_LOGI(TAG, "bytes: %d chunked: %d", bytes, chunked);
    
    chunked = true;
    bytes = http_client_get_native(HTTP_CLIENT_URI_IMAGE, &buffer, chunked);
    if (bytes > 0) {
        free(buffer);
    }
    TEST_ASSERT_GREATER_THAN(0, bytes); 
    ESP_LOGI(TAG, "bytes: %d chunked: %d", bytes, chunked);    
    
    wifi_sta_stop();

    TEST_ASSERT_TRUE(heap_caps_check_integrity_all(true));
    ESP_LOGI(TAG, "free heap size: %d", esp_get_free_heap_size());
}


// Trim a string
char *ltrim(char *s)
{
    while(isspace((unsigned char)*s)) s++;
    return s;
}

TEST_CASE("list reader", "[client]")
{    
    const int BUF_LEN = 60 * 1024;
    char *buf = malloc(BUF_LEN);
    TEST_ASSERT_NOT_NULL(buf); 
    
    TEST_ASSERT_EQUAL(ESP_OK, wifi_sta_start(WIFI_STA_SSID, WIFI_STA_PASS, NULL, 0,0));
    int bytes = http_client_reader(HTTP_CLIENT_URI_GET "/" HTTP_CLIENT_LIST_NAME, buf, BUF_LEN);
    TEST_ASSERT_GREATER_THAN(0, bytes);

    char *buf_list = strdup(buf);
    char *line = strtok(buf_list, "\n");
    
    while (line) {
        char *file_name = ltrim(line);
        if (file_name[0] != '#') {  // skip commented lines
            char url[100] = "";
            sprintf(url, "%s/%s", HTTP_CLIENT_URI_GET, file_name);
            bytes = http_client_reader(url, buf, BUF_LEN);
            if (bytes == 0) 
                break;
        }
        line  = strtok(NULL, "\n");
    }
    
    free(buf_list);
    free(buf);
    
    wifi_sta_stop();
    TEST_ASSERT_GREATER_THAN(0, bytes);
}

