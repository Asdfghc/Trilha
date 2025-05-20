#include <sys/time.h>
#include "sensor_manager.h"
#include "esp_timer.h"
#include "esp_adc/adc_oneshot.h"
#include "dht.h"

static adc_oneshot_unit_handle_t adc_handle;

void sensor_manager_init(void) {
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config, &adc_handle);

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_4, &config); // Ex: GPIO32
}

void read_all_sensors(SensorReading *buffer, size_t *count) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t timestamp = ((int64_t)tv.tv_sec) * 1000 + tv.tv_usec / 1000;

    int index = 0;

    // Leitura do sensor de ruído
    int noise_raw = 0;
    adc_oneshot_read(adc_handle, ADC_CHANNEL_4, &noise_raw);  // ADC_CHANNEL_4 para ruído (ajuste se necessário)
    buffer[index++] = (SensorReading){ .name = "noise", .value = noise_raw, .timestamp = timestamp };

    // Leitura do sensor de luminosidade (LDR)
    int ldr_raw = 0;
    adc_oneshot_read(adc_handle, ADC_CHANNEL_5, &ldr_raw);  // ADC_CHANNEL_5 para LDR (ajuste o canal conforme o pino)
    buffer[index++] = (SensorReading){ .name = "light", .value = ldr_raw, .timestamp = timestamp };

    // Leitura do sensor DHT (temperatura e umidade)
    float temp, hum;
    if (read_dht_data(&temp, &hum)) {
        buffer[index++] = (SensorReading){ .name = "temperature", .value = (int)(temp * 10), .timestamp = timestamp };
        buffer[index++] = (SensorReading){ .name = "humidity", .value = (int)(hum * 10), .timestamp = timestamp };
    }

    *count = index;
}
