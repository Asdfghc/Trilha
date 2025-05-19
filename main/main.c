#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi_manager.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_websocket_client.h"
#include "esp_event.h"
#include "string.h"
#include "esp_timer.h"
#include "cJSON.h"
#include "esp_sntp.h"
#include <time.h>
#include <sys/time.h>


#define TAG "main"

esp_websocket_client_handle_t client;

void init_sntp(void) {
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");  // ou outro servidor NTP
    sntp_init();
}

void wait_for_time_sync(void) {
    time_t now = 0;
    struct tm timeinfo = { 0 };

    int retry = 0;
    const int retry_count = 10;
    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI("TIME", "Aguardando sincronização do tempo...");
        vTaskDelay(pdMS_TO_TICKS(2000));
        time(&now);
        localtime_r(&now, &timeinfo);
    }
}

typedef struct {
    int sample;
    int64_t timestamp;
} SampleData;

char *create_json_from_samples(SampleData *samples, size_t count) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;

    cJSON *data_array = cJSON_CreateArray();
    if (!data_array) {
        cJSON_Delete(root);
        return NULL;
    }

    cJSON_AddItemToObject(root, "data", data_array);

    for (size_t i = 0; i < count; ++i) {
        cJSON *sample_obj = cJSON_CreateObject();
        if (!sample_obj) {
            cJSON_Delete(root);
            return NULL;
        }

        cJSON_AddNumberToObject(sample_obj, "sample", samples[i].sample);
        cJSON_AddNumberToObject(sample_obj, "timestamp", samples[i].timestamp);

        cJSON_AddItemToArray(data_array, sample_obj);
    }

    char *json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root); // Libera a memória da estrutura, mas não da string

    return json_string; // Você deve dar `free(json_string)` depois de usar
}

static void websocket_event_handler(void *handler_args, esp_event_base_t base,
                                    int32_t event_id, void *event_data) {
    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "WebSocket connected");
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "WebSocket disconnected");
            break;
        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGE(TAG, "WebSocket error");
            break;
        default:
            break;
    }
}

void websocket_app_start() {
    esp_websocket_client_config_t websocket_cfg = {
        .uri = "ws://192.168.1.145:5001/ws",  // Altere para o IP do seu servidor
    };

    client = esp_websocket_client_init(&websocket_cfg);
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)client);
    esp_websocket_client_start(client);
}

void sensor_task(void *pvParameters) {
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config, &adc_handle);

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_4, &config); // GPIO32

    while (1) {
        int raw;
        adc_oneshot_read(adc_handle, ADC_CHANNEL_4, &raw);

        char msg[100];

        
        time_t now;
        time(&now);

        struct timeval tv;
        gettimeofday(&tv, NULL);
        int64_t timestamp_ms = ((int64_t)tv.tv_sec) * 1000 + tv.tv_usec / 1000;

        SampleData samples[3];
        samples[0] = (SampleData){.sample = raw, .timestamp = timestamp_ms};
        //samples[1] = (SampleData){.sample = 130, .timestamp = 12345688};
        //samples[2] = (SampleData){.sample = 140, .timestamp = 12345698};

        char *json = create_json_from_samples(samples, 1);
        if (json) {
            // printf("JSON gerado: %s\n", json);
            // aqui você pode enviar via WebSocket
            
            

            //snprintf(json, sizeof(json), "data: [{sample: %d, timestamp: %lld}, {sample: %d, timestamp: %lld}]", raw, timestamp_us, raw, timestamp_us);
            
            ESP_LOGI(TAG, "Sending: %s", json);
            
            if (esp_websocket_client_is_connected(client)) {
                esp_websocket_client_send_text(client, json, strlen(json), portMAX_DELAY);
            }

            free(json);
        }
            
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 segundo
    }
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    wifi_init_sta();
    websocket_app_start();

    init_sntp();
    wait_for_time_sync();
    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);
}
