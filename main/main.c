#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi_manager.h"
#include "websocket_client.h"
#include "sensor_manager.h"
#include "esp_sntp.h"
#include <time.h>

static const char *TAG = "main";

void init_time_sync() {
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    time_t now;
    struct tm timeinfo = {0};
    int retry = 0;
    const int max_retries = 10;

    while (timeinfo.tm_year < (2016 - 1900) && ++retry < max_retries) {
        ESP_LOGI(TAG, "Aguardando sincronização do tempo...");
        vTaskDelay(pdMS_TO_TICKS(2000));
        time(&now);
        localtime_r(&now, &timeinfo);
    }
}

void sensor_task(void *pvParameters) {
    SensorReading readings[5];
    size_t count;

    while (1) {
        read_all_sensors(readings, &count);
        websocket_send_readings(readings, count);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init_sta();
    websocket_app_start();
    init_time_sync();

    sensor_manager_init();
    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);
}
