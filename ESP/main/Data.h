#ifndef DATA_H
#define DATA_H
#include <stdio.h>
#include "cJSON.h"

#define LENGHT_DATA 250
char *create_json_two_arrays(float *arr1, float *arr2);
char *create_json_one_arrays(float *arr);
#endif
