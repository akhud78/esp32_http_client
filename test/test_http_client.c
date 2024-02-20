#include <string.h>
#include <ctype.h>
#include "unity.h"

#include "wifi.h"
#include "http_client.h"

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


