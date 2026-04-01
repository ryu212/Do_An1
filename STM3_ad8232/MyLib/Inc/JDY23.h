#ifndef JDY23_H
#define JDY23_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define JDY23_MAX_RESPONSE_LEN 128U

typedef enum
{
    JDY23_OK = 0,
    JDY23_ERROR,
    JDY23_TIMEOUT,
    JDY23_BAD_PARAM,
    JDY23_NO_PWRC_PIN,
    JDY23_PARSE_ERROR     
} JDY23_Result;

typedef enum
{
    JDY23_LINK_DISCONNECTED = 0,
    JDY23_LINK_CONNECTED    = 1
} JDY23_LinkStatus;

typedef enum
{
    JDY23_BAUD_115200 = 0,
    JDY23_BAUD_57600  = 1,
    JDY23_BAUD_38400  = 2,
    JDY23_BAUD_19200  = 3,
    JDY23_BAUD_9600   = 4,   // normal default baud rate
    JDY23_BAUD_4800   = 5,
    JDY23_BAUD_2400   = 6
} JDY23_BaudParam;

typedef enum
{
    JDY23_SLEEP_LIGHT = 1,  //with broadcast
    JDY23_SLEEP_DEEP  = 2   // no broadcast
} JDY23_SleepMode;

typedef enum
{
    JDY23_START_SLEEP = 0,
    JDY23_START_WAKE  = 1
} JDY23_StartMode;

typedef enum // Broadcast interval
{
    JDY23_ADVIN_100MS  = 0,
    JDY23_ADVIN_200MS  = 1,
    JDY23_ADVIN_300MS  = 2,
    JDY23_ADVIN_400MS  = 3,
    JDY23_ADVIN_500MS  = 4,
    JDY23_ADVIN_600MS  = 5,
    JDY23_ADVIN_700MS  = 6,
    JDY23_ADVIN_800MS  = 7,
    JDY23_ADVIN_900MS  = 8,
    JDY23_ADVIN_1000MS = 9
} JDY23_AdvInterval;

typedef struct
{
    UART_HandleTypeDef *huart;

    GPIO_TypeDef *pwrc_port;
    uint16_t pwrc_pin;       //used for sleep / wake / control-related functions.
    bool     pwrc_available;

    uint32_t tx_timeout_ms;
    uint32_t rx_timeout_ms;

    char massage_buffer[JDY23_MAX_RESPONSE_LEN];
    volatile size_t rx_index;
    volatile bool response_complete;
} JDY23_HandleTypeDef;


void jdy23_init(JDY23_HandleTypeDef *hjdy,
                UART_HandleTypeDef *huart,
                GPIO_TypeDef *pwrc_port,
                uint16_t pwrc_pin,
                uint8_t pwrc_available);

JDY23_Result jdy23_write(JDY23_HandleTypeDef *hjdy, const uint8_t *data, uint16_t len);
JDY23_Result jdy23_read(JDY23_HandleTypeDef *hjdy, char *buffer, size_t buffer_len);
                      
JDY23_Result JDY23_SetName(JDY23_HandleTypeDef *hjdy, const char *name);
JDY23_Result JDY23_GetName(JDY23_HandleTypeDef *hjdy, char *name, size_t len);

JDY23_Result jdy23_get_mac(JDY23_HandleTypeDef *hjdy, char *mac, size_t len);

JDY23_Result jdy23_reset(JDY23_HandleTypeDef *hjdy);
JDY23_Result jdy23_disconnect(JDY23_HandleTypeDef *hjdy);

JDY23_Result jdy23_get_link_status(JDY23_HandleTypeDef *hjdy, JDY23_LinkStatus *status);
JDY23_Result jdy23_set_baud_param(JDY23_HandleTypeDef *hjdy, JDY23_BaudParam baud_param);

JDY23_Result jdy23_set_start_mode(JDY23_HandleTypeDef *hjdy, JDY23_StartMode mode);
JDY23_Result jdy23_set_adv_interval(JDY23_HandleTypeDef *hjdy, JDY23_AdvInterval interval);

JDY23_Result jdy23_sleep(JDY23_HandleTypeDef *hjdy, JDY23_SleepMode mode);
JDY23_Result jdy23_wake_by_pwrc(JDY23_HandleTypeDef *hjdy, uint32_t pulse_ms);
JDY23_Result jdy23_turn_off_led(JDY23_HandleTypeDef *hjdy);
#ifdef __cplusplus
}
#endif

#endif