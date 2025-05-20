#pragma once
#include <stdint.h>   // para int64_t
#include <stddef.h>   // para size_t

typedef struct {
    const char* name;
    int value;
    int64_t timestamp;
} SensorReading;

void sensor_manager_init(void);
void read_all_sensors(SensorReading *buffer, size_t *count);
