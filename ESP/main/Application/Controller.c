#include "Controller.h"

float received_signal[LENGHT_BUFFER] = {0};
char *create_json_two_arrays(float *arr1, float *arr2) {
    cJSON *root = cJSON_CreateObject();
    cJSON *input1 = cJSON_AddArrayToObject(root, "input1");
    cJSON *input2 = cJSON_AddArrayToObject(root, "input2");

    for (int i = 0; i < LENGHT_BUFFER; i++) {
        cJSON_AddItemToArray(input1, cJSON_CreateNumber(arr1[i]));
        cJSON_AddItemToArray(input2, cJSON_CreateNumber(arr2[i]));
    }

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);  // giải phóng cây JSON, KHÔNG ảnh hưởng json_str
    return json_str;     // phải free() sau khi dùng
}

char *create_json_one_arrays(float *arr)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *input1 = cJSON_AddArrayToObject(root, "input1");
    for (int i = 0; i < LENGHT_BUFFER; i++) 
        cJSON_AddItemToArray(input1, cJSON_CreateNumber(arr[i]));
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);  // giải phóng cây JSON, KHÔNG ảnh hưởng json_str
    return json_str; 
}
// mount currently day into filename
void get_daily_filename(char *filename, size_t len)
{
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    strftime(filename, len, "ecg_%Y_%m_%d.bin", &timeinfo);
}
// send json to broker 
void mqtt_task(void* arg)
{
    while(1)
    {
        if (xQueueReceive(mqtt_data_queue, received_signal, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI("MQTT_TASK", "Read from queue");
            char* json_str = create_json_one_arrays(received_signal);
            publish(MQTT_TOPIC_PUBLISH, json_str);
            free(json_str);             
        }
        vTaskDelay(pdMS_TO_TICKS(10));

    }
}
/*
Task store ecg signal into mem card 
This task will call get_daily_filename() to mount day in filename,
and hold until received 10 batch in queue, storing into mem card 
*/
void write_SD_card_task(void *arg)
{
    float sd_batch[10][360];
    int batch_count = 0;
    char filename[64];

    while (1)
    {
        if (xQueueReceive(sd_card_data_queue, sd_batch[batch_count], portMAX_DELAY) == pdTRUE) {
            ESP_LOGI("SD_TASK", "Read block %d from queue", batch_count + 1);
            batch_count++;
        }

        if (batch_count >= 10) {
            get_daily_filename(filename, sizeof(filename));

            esp_err_t ret = sd_card_append(filename,
                                           (const uint8_t *)sd_batch,
                                           sizeof(sd_batch));
            if (ret == ESP_OK) {
                ESP_LOGI("SD_TASK", "Write 10 blocks to SD card success");
            } else {
                ESP_LOGE("SD_TASK", "Write 10 blocks to SD card failed");
            }
            batch_count = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}