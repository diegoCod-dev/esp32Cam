idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp32Cam sd wifi_server mqtt_manager espnow_manager capture_timer)
