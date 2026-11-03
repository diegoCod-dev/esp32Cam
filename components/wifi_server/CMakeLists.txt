idf_component_register(SRCS "wifi_server.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_wifi nvs_flash esp_netif esp_http_server sd)
