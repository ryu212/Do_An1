#include "JDY23.h"

#define JDY23_DEFAULT_TX_TIMEOUT_MS   100U
#define JDY23_DEFAULT_RX_TIMEOUT_MS   300U
#define JDY23_RX_POLL_TIMEOUT_MS      10U
#define JDY23_RX_IDLE_GAP_MS          20U

// send an attention comd and read the response
static JDY23_Result jdy23_send_at_internal(JDY23_HandleTypeDef *hjdy,
                                           const char *cmd,
                                           char *response,
                                           size_t response_len);
// flush buffer to avoid rabage data before sending a new cmd 
static void jdy23_flush_rx(JDY23_HandleTypeDef *hjdy);
// check if response contains "+OK"
static bool jdy23_response_has_ok(const char *response);
// get the value part after a prefix in response 
static JDY23_Result jdy23_copy_value_after_prefix(const char *response,
                                                  const char *prefix,
                                                  char *out,
                                                  size_t out_len);
// clear string 
static void jdy23_trim_trailing(char *str);

void jdy23_init(JDY23_HandleTypeDef *hjdy,
                UART_HandleTypeDef *huart,
                GPIO_TypeDef *pwrc_port,
                uint16_t pwrc_pin,
                uint8_t pwrc_available)
{
    if (hjdy == NULL)
    {
        return;
    }

    hjdy->huart          = huart;
    hjdy->pwrc_port      = pwrc_port;
    hjdy->pwrc_pin       = pwrc_pin;
    hjdy->pwrc_available = (pwrc_available != 0U);
    hjdy->tx_timeout_ms  = JDY23_DEFAULT_TX_TIMEOUT_MS;
    hjdy->rx_timeout_ms  = JDY23_DEFAULT_RX_TIMEOUT_MS;
    hjdy->rx_index       = 0U;
    hjdy->response_complete = false;
    hjdy->massage_buffer[0] = '\0';

    if (hjdy->pwrc_available && (hjdy->pwrc_port != NULL))
    {
        // pwrc is active low, keep it inactive by default
        HAL_GPIO_WritePin(hjdy->pwrc_port, hjdy->pwrc_pin, GPIO_PIN_SET);
    }
}

JDY23_Result jdy23_write(JDY23_HandleTypeDef *hjdy, const uint8_t *data, uint16_t len)
{
    if ((hjdy == NULL) || (hjdy->huart == NULL) || (data == NULL) || (len == 0U))
    {
        return JDY23_BAD_PARAM;
    }

    if (HAL_UART_Transmit(hjdy->huart, (uint8_t *)data, len, hjdy->tx_timeout_ms) == HAL_OK)
    {
        return JDY23_OK;
    }

    return JDY23_ERROR;
}

JDY23_Result jdy23_read(JDY23_HandleTypeDef *hjdy, char *buffer, size_t buffer_len)
{
    uint32_t start_tick;

    if ((hjdy == NULL) || (hjdy->huart == NULL) || (buffer == NULL) || (buffer_len < 2U))
    {
        return JDY23_BAD_PARAM;
    }
    start_tick = HAL_GetTick();
    while (!hjdy->response_complete)
    {
        
        if ((HAL_GetTick() - start_tick) > hjdy->rx_timeout_ms)
        {
            return JDY23_TIMEOUT;
        }
    }
    if(hjdy->rx_index < (buffer_len - 1))
    {
        memcpy(buffer, hjdy->massage_buffer, hjdy->rx_index);
        buffer[hjdy->rx_index] = '\0';
    }
    else {
        return JDY23_ERROR;
    }
    hjdy->rx_index = 0;
    hjdy->response_complete = false;
    memset(hjdy->massage_buffer, 0, JDY23_MAX_RESPONSE_LEN);
    return JDY23_OK;
}

JDY23_Result JDY23_SetName(JDY23_HandleTypeDef *hjdy, const char *name)
{
    char response[JDY23_MAX_RESPONSE_LEN];
    char command[40];

    if ((hjdy == NULL) || (name == NULL))
    {
        return JDY23_BAD_PARAM;
    }

    if ((strlen(name) == 0U) || (strlen(name) > 24U))
    {
        return JDY23_BAD_PARAM;
    }

    (void)snprintf(command, sizeof(command), "AT+NAME%s", name);

    if (jdy23_send_at_internal(hjdy, command, response, sizeof(response)) != JDY23_OK)
    {
        return JDY23_ERROR;
    }

    return jdy23_response_has_ok(response) ? JDY23_OK : JDY23_ERROR;
}

JDY23_Result JDY23_GetName(JDY23_HandleTypeDef *hjdy, char *name, size_t len)
{
    char response[JDY23_MAX_RESPONSE_LEN];
    JDY23_Result ret;

    if ((hjdy == NULL) || (name == NULL) || (len == 0U))
    {
        return JDY23_BAD_PARAM;
    }

    ret = jdy23_send_at_internal(hjdy, "AT+NAME", response, sizeof(response));
    if (ret != JDY23_OK)
    {
        return ret;
    }

    return jdy23_copy_value_after_prefix(response, "+NAME:", name, len);
}

JDY23_Result jdy23_reset(JDY23_HandleTypeDef *hjdy)
{
    char response[JDY23_MAX_RESPONSE_LEN];
    JDY23_Result ret;

    ret = jdy23_send_at_internal(hjdy, "AT+RST", response, sizeof(response));
    if (ret != JDY23_OK)
    {
        return ret;
    }

    return jdy23_response_has_ok(response) ? JDY23_OK : JDY23_ERROR;
}

JDY23_Result jdy23_disconnect(JDY23_HandleTypeDef *hjdy)
{
    char response[JDY23_MAX_RESPONSE_LEN];
    JDY23_Result ret;

    ret = jdy23_send_at_internal(hjdy, "AT+DISC", response, sizeof(response));
    if (ret != JDY23_OK)
    {
        return ret;
    }

    return jdy23_response_has_ok(response) ? JDY23_OK : JDY23_ERROR;
}

JDY23_Result jdy23_get_link_status(JDY23_HandleTypeDef *hjdy, JDY23_LinkStatus *status)
{
    char response[JDY23_MAX_RESPONSE_LEN];
    char value[8];
    JDY23_Result ret;

    if ((hjdy == NULL) || (status == NULL))
    {
        return JDY23_BAD_PARAM;
    }

    ret = jdy23_send_at_internal(hjdy, "AT+STAT", response, sizeof(response));
    if (ret != JDY23_OK)
    {
        return ret;
    }

    ret = jdy23_copy_value_after_prefix(response, "+STAT:", value, sizeof(value));
    if (ret != JDY23_OK)
    {
        return ret;
    }

    if (strcmp(value, "00") == 0)
    {
        *status = JDY23_LINK_DISCONNECTED;
        return JDY23_OK;
    }

    if (strcmp(value, "01") == 0)
    {
        *status = JDY23_LINK_CONNECTED;
        return JDY23_OK;
    }

    return JDY23_PARSE_ERROR;
}

JDY23_Result jdy23_set_baud_param(JDY23_HandleTypeDef *hjdy, JDY23_BaudParam baud_param)
{
    char response[JDY23_MAX_RESPONSE_LEN];
    char command[20];
    JDY23_Result ret;

    if ((hjdy == NULL) || (baud_param > JDY23_BAUD_2400))
    {
        return JDY23_BAD_PARAM;
    }

    (void)snprintf(command, sizeof(command), "AT+BAUD%u", (unsigned int)baud_param);

    ret = jdy23_send_at_internal(hjdy, command, response, sizeof(response));
    if (ret != JDY23_OK)
    {
        return ret;
    }

    return jdy23_response_has_ok(response) ? JDY23_OK : JDY23_ERROR;
}
JDY23_Result jdy23_get_mac(JDY23_HandleTypeDef *hjdy, char *mac, size_t len)
{
    char response[JDY23_MAX_RESPONSE_LEN];
    JDY23_Result ret;

    // check input parameters
    if ((hjdy == NULL) || (mac == NULL) || (len == 0U))
    {
        return JDY23_BAD_PARAM;
    }

    // send AT command to query MAC address
    ret = jdy23_send_at_internal(hjdy, "AT+MAC", response, sizeof(response));
    if (ret != JDY23_OK)
    {
        return ret;
    }

    // try to parse response in format: +MAC:xxxxxxxxxxxx
    ret = jdy23_copy_value_after_prefix(response, "+MAC:", mac, len);
    if (ret == JDY23_OK)
    {
        return JDY23_OK;
    }

    // fallback: some firmware may return +MAC=xxxxxxxxxxxx
    ret = jdy23_copy_value_after_prefix(response, "+MAC=", mac, len);
    if (ret == JDY23_OK)
    {
        return JDY23_OK;
    }

    // fallback: copy raw response if prefix is not found
    strncpy(mac, response, len - 1U);
    mac[len - 1U] = '\0';
    jdy23_trim_trailing(mac);

    // return success if something was copied, otherwise parse error
    return (strlen(mac) > 0U) ? JDY23_OK : JDY23_PARSE_ERROR;
}
JDY23_Result jdy23_set_start_mode(JDY23_HandleTypeDef *hjdy, JDY23_StartMode mode)
{
    char response[JDY23_MAX_RESPONSE_LEN];
    char command[24];
    JDY23_Result ret;

    if ((hjdy == NULL) || (mode > JDY23_START_WAKE))
    {
        return JDY23_BAD_PARAM;
    }

    (void)snprintf(command, sizeof(command), "AT+STARTEN%u", (unsigned int)mode);

    ret = jdy23_send_at_internal(hjdy, command, response, sizeof(response));
    if (ret != JDY23_OK)
    {
        return ret;
    }

    return jdy23_response_has_ok(response) ? JDY23_OK : JDY23_ERROR;
}

JDY23_Result jdy23_set_adv_interval(JDY23_HandleTypeDef *hjdy, JDY23_AdvInterval interval)
{
    char response[JDY23_MAX_RESPONSE_LEN];
    char command[20];
    JDY23_Result ret;

    if ((hjdy == NULL) || (interval > JDY23_ADVIN_1000MS))
    {
        return JDY23_BAD_PARAM;
    }

    (void)snprintf(command, sizeof(command), "AT+ADVIN%u", (unsigned int)interval);

    ret = jdy23_send_at_internal(hjdy, command, response, sizeof(response));
    if (ret != JDY23_OK)
    {
        return ret;
    }

    return jdy23_response_has_ok(response) ? JDY23_OK : JDY23_ERROR;
}

JDY23_Result jdy23_sleep(JDY23_HandleTypeDef *hjdy, JDY23_SleepMode mode)
{
    char response[JDY23_MAX_RESPONSE_LEN];
    char command[20];
    JDY23_Result ret;

    if ((hjdy == NULL) || ((mode != JDY23_SLEEP_LIGHT) && (mode != JDY23_SLEEP_DEEP)))
    {
        return JDY23_BAD_PARAM;
    }

    (void)snprintf(command, sizeof(command), "AT+SLEEP%u", (unsigned int)mode);

    ret = jdy23_send_at_internal(hjdy, command, response, sizeof(response));
    if (ret != JDY23_OK)
    {
        return ret;
    }

    return jdy23_response_has_ok(response) ? JDY23_OK : JDY23_ERROR;
}

JDY23_Result jdy23_wake_by_pwrc(JDY23_HandleTypeDef *hjdy, uint32_t pulse_ms)
{
    if (hjdy == NULL)
    {
        return JDY23_BAD_PARAM;
    }

    if ((!hjdy->pwrc_available) || (hjdy->pwrc_port == NULL))
    {
        return JDY23_NO_PWRC_PIN;
    }

    if (pulse_ms == 0U)
    {
        pulse_ms = 5U;
    }

    // pwrc is active low according to the datasheet
    HAL_GPIO_WritePin(hjdy->pwrc_port, hjdy->pwrc_pin, GPIO_PIN_RESET);
    HAL_Delay(pulse_ms);
    HAL_GPIO_WritePin(hjdy->pwrc_port, hjdy->pwrc_pin, GPIO_PIN_SET);
    HAL_Delay(5U);

    return JDY23_OK;
}

JDY23_Result jdy23_turn_off_led(JDY23_HandleTypeDef *hjdy){
    char response[JDY23_MAX_RESPONSE_LEN];
    JDY23_Result ret;

    if (hjdy == NULL)
    {
        return JDY23_BAD_PARAM;
    }

    ret = jdy23_send_at_internal(hjdy, "AT+ALED0", response, sizeof(response));
    if (ret != JDY23_OK)
    {
        return ret;
    }

    return jdy23_response_has_ok(response) ? JDY23_OK : JDY23_ERROR;
}
static JDY23_Result jdy23_send_at_internal(JDY23_HandleTypeDef *hjdy,
                                           const char *cmd,
                                           char *response,
                                           size_t response_len)
{
    char tx_buf[64];
    int tx_len;
    JDY23_Result ret;

    if ((hjdy == NULL) || (cmd == NULL) || (response == NULL) || (response_len < 2U))
    {
        return JDY23_BAD_PARAM;
    }

    tx_len = snprintf(tx_buf, sizeof(tx_buf), "%s\r\n", cmd);
    if ((tx_len <= 0) || ((size_t)tx_len >= sizeof(tx_buf)))
    {
        return JDY23_BAD_PARAM;
    }

    jdy23_flush_rx(hjdy);

    ret = jdy23_write(hjdy, (const uint8_t *)tx_buf, (uint16_t)tx_len);
    if (ret != JDY23_OK)
    {
        return ret;
    }

    return jdy23_read(hjdy, response, response_len);
}

static void jdy23_flush_rx(JDY23_HandleTypeDef *hjdy)
{
    __HAL_UART_DISABLE_IT(hjdy->huart, UART_IT_RXNE);
    
    
    uint32_t tmpreg = hjdy->huart->Instance->SR;
    tmpreg = hjdy->huart->Instance->DR;
    (void)tmpreg; 

    
    hjdy->rx_index = 0;
    hjdy->response_complete = false;
    memset(hjdy->massage_buffer, 0, JDY23_MAX_RESPONSE_LEN);
    
    
    __HAL_UART_CLEAR_FEFLAG(hjdy->huart); 
    __HAL_UART_CLEAR_NEFLAG(hjdy->huart); 
    __HAL_UART_CLEAR_OREFLAG(hjdy->huart); 
    
    __HAL_UART_ENABLE_IT(hjdy->huart, UART_IT_RXNE);
}
static bool jdy23_response_has_ok(const char *response)
{
    return (response != NULL) && (strstr(response, "+OK") != NULL);
}

static JDY23_Result jdy23_copy_value_after_prefix(const char *response,
                                                  const char *prefix,
                                                  char *out,
                                                  size_t out_len)
{
    const char *start;
    size_t n = 0U;

    if ((response == NULL) || (prefix == NULL) || (out == NULL) || (out_len < 2U))
    {
        return JDY23_BAD_PARAM;
    }

    start = strstr(response, prefix);
    if (start == NULL)
    {
        return JDY23_PARSE_ERROR;
    }

    start += strlen(prefix);

    while ((start[n] != '\0') && (start[n] != '\r') && (start[n] != '\n'))
    {
        n++;
    }

    if (n >= out_len)
    {
        return JDY23_ERROR;
    }

    memcpy(out, start, n);
    out[n] = '\0';
    jdy23_trim_trailing(out);

    return JDY23_OK;
}

static void jdy23_trim_trailing(char *str)
{
    size_t len;

    if (str == NULL)
    {
        return;
    }

    len = strlen(str);
    while (len > 0U)
    {
        char c = str[len - 1U];
        if ((c == '\r') || (c == '\n') || (c == ' ') || (c == '\t'))
        {
            str[len - 1U] = '\0';
            len--;
        }
        else
        {
            break;
        }
    }
}
