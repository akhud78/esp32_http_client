idf_component_register( SRCS "http_client.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash esp_http_client esp-tls esp_event esp_netif esp_timer)
