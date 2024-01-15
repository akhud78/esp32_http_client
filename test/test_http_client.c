#include "unity.h"
#include "sdkconfig.h"

#include "wifi.h"
#include "http_client.h"

TEST_CASE("get", "[client]")
{    
    TEST_ASSERT_EQUAL(ESP_OK, wifi_sta_start(WIFI_STA_SSID, WIFI_STA_PASS, NULL, 0,0));
    TEST_ASSERT_EQUAL(ESP_OK, http_client_get(HTTP_CLIENT_URI_GET));
    
    wifi_sta_stop();
}

TEST_CASE("image reader", "[client]")
{
        
    const int BUFFER_LEN = 60 * 1024;
    char *buffer = malloc(BUFFER_LEN);
    TEST_ASSERT_NOT_NULL(buffer); 
    
    TEST_ASSERT_EQUAL(ESP_OK, wifi_sta_start(WIFI_STA_SSID, WIFI_STA_PASS, NULL, 0,0));
    int len = http_client_reader(HTTP_CLIENT_URI_IMAGE, buffer, BUFFER_LEN);
    free(buffer);
    TEST_ASSERT_GREATER_THAN(0, len);
    wifi_sta_stop();
}


