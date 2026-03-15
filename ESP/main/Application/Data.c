#include "Data.h"

char *create_json_two_arrays(float *arr1, float *arr2) {
    cJSON *root = cJSON_CreateObject();
    cJSON *input1 = cJSON_AddArrayToObject(root, "input1");
    cJSON *input2 = cJSON_AddArrayToObject(root, "input2");

    for (int i = 0; i < LENGHT_DATA; i++) {
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
    for (int i = 0; i < LENGHT_DATA; i++) 
        cJSON_AddItemToArray(input1, cJSON_CreateNumber(arr[i]));
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);  // giải phóng cây JSON, KHÔNG ảnh hưởng json_str
    return json_str; 
}
