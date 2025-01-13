#include <string.h>
#include <ctype.h>
#include "unity.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
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
    uint64_t start = esp_timer_get_time(); 
    int bytes = http_client_reader(HTTP_CLIENT_URI_IMAGE, buffer, BUFFER_LEN);
    uint64_t end = esp_timer_get_time();
    free(buffer);
    wifi_sta_stop();    
    uint64_t time_ms = (end - start) / 1000;
    TEST_ASSERT_GREATER_THAN(0, bytes);
    int rate_bps = bytes * 1000 * 8 / time_ms; // bits per sec
    ESP_LOGI(TAG, "bytes: %d time_ms: %llu rate_bps: %d", bytes, time_ms, rate_bps);
}

TEST_CASE("image get native", "[client]")
{
    TEST_ASSERT_EQUAL(ESP_OK, wifi_sta_start(WIFI_STA_SSID, WIFI_STA_PASS, NULL, 0,0));
    
    char *buffer = NULL;
    bool chunked = false;
    uint64_t start = esp_timer_get_time(); 
    int bytes = http_client_get_native(HTTP_CLIENT_URI_IMAGE, &buffer, chunked);
    if (bytes > 0) {
        free(buffer);
    }
    uint64_t end = esp_timer_get_time();
    TEST_ASSERT_GREATER_THAN(0, bytes); 
    uint64_t time_ms = (end - start) / 1000;
    int rate_bps = bytes * 1000 * 8 / time_ms; 
    ESP_LOGI(TAG, "bytes: %d chunked: %d time_ms: %llu rate_bps: %d", bytes, chunked, time_ms, rate_bps);
    
    chunked = true;
    start = esp_timer_get_time();
    bytes = http_client_get_native(HTTP_CLIENT_URI_IMAGE, &buffer, chunked);
    if (bytes > 0) {
        free(buffer);
    }
    TEST_ASSERT_GREATER_THAN(0, bytes); 
    end = esp_timer_get_time();
    time_ms = (end - start) / 1000;
    rate_bps = bytes * 1000 * 8 / time_ms; 
    ESP_LOGI(TAG, "bytes: %d chunked: %d time_ms: %llu rate_bps: %d", bytes, chunked, time_ms, rate_bps);
    
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

