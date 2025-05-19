idf_component_register(SRCS "main.c" "wifi_manager.c"
                       INCLUDE_DIRS "."
                       REQUIRES json esp_websocket_client esp_wifi esp_event nvs_flash esp_adc)
