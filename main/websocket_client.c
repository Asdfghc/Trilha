#include "websocket_client.h"
#include "esp_websocket_client.h"
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "websocket";
static esp_websocket_client_handle_t client;

static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    if (event_id == WEBSOCKET_EVENT_CONNECTED) {
        ESP_LOGI(TAG, "WebSocket connected");
    } else if (event_id == WEBSOCKET_EVENT_DISCONNECTED) {
        ESP_LOGI(TAG, "WebSocket disconnected");
    } else if (event_id == WEBSOCKET_EVENT_ERROR) {
        ESP_LOGE(TAG, "WebSocket error");
    }
}

void websocket_app_start(void) {
    esp_websocket_client_config_t cfg = {
        .uri = "ws://192.168.1.145:5001/ws",
    };

    client = esp_websocket_client_init(&cfg);
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, NULL);
    esp_websocket_client_start(client);
}

void websocket_send_readings(SensorReading *readings, size_t count) {
    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateArray();

    for (size_t i = 0; i < count; ++i) {
        cJSON *entry = cJSON_CreateObject();
        cJSON_AddStringToObject(entry, "sensor", readings[i].name);
        cJSON_AddNumberToObject(entry, "value", readings[i].value);
        cJSON_AddNumberToObject(entry, "timestamp", readings[i].timestamp);
        cJSON_AddItemToArray(data, entry);
    }

    cJSON_AddItemToObject(root, "data", data);
    char *json = cJSON_PrintUnformatted(root);

    if (esp_websocket_client_is_connected(client)) {
        esp_websocket_client_send_text(client, json, strlen(json), portMAX_DELAY);
    }

    cJSON_free(json);
    cJSON_Delete(root);
}
