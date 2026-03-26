#ifndef Controller_H
#define Controller_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdio.h>
#include "time.h"
#include "Wifi.h"
#include "Mqtt.h"
#include "Spi.h"
#include "SDCard.h"
#include "cJSON.h"


#define LENGHT_BUFFER 360
extern int32_t received_signal[LENGHT_BUFFER];

char *create_json_two_arrays(int32_t *arr1, int32_t *arr2);
char *create_json_one_arrays(int32_t *arr);
void get_daily_filename(char *filename, size_t len);
void mqtt_task(void* arg);
void write_SD_card_task(void *arg);

#ifdef __cplusplus
}
#endif
#endif