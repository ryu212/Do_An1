#include "sd_spi.h"
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
/* Static variables */
static sd_spi_cfg_t g_sd_spi_cfg;
static sd_card_type_t g_sd_card_type = SD_TYPE_UNKNOWN;
static uint32_t g_sd_capacity = 0;

/* Interrupt-driven synchronization */
static osSemaphoreId_t spi_xfer_complete_sem = NULL;
static osSemaphoreId_t spi_rx_complete_sem = NULL;
static bool spi_error_flag = false;

/* SPI transfer buffers for DMA */
#define SPI_DMA_BUFFER_SIZE 512
static uint8_t spi_tx_buffer[SPI_DMA_BUFFER_SIZE];
static uint8_t spi_rx_buffer[SPI_DMA_BUFFER_SIZE];

/**
 * Initialize SD SPI interface with interrupt support
 */
void sd_spi_init(const sd_spi_cfg_t *cfg)
{
    if (cfg == NULL || cfg->hspi == NULL) {
        //printf("[SPI] ERROR: Invalid config or hspi is NULL\n");
        return;
    }
    
    g_sd_spi_cfg = *cfg;
    //printf("[SPI] Initializing SPI on instance %p\n", (void*)g_sd_spi_cfg.hspi->Instance);
    
    /* Create semaphores for interrupt synchronization */
    if (spi_xfer_complete_sem == NULL) {
        spi_xfer_complete_sem = osSemaphoreNew(1, 0, NULL);
        if (spi_xfer_complete_sem == NULL) {
            //printf("[SPI] ERROR: Failed to create spi_xfer_complete_sem\n");
            return;
        }
        //printf("[SPI] Created spi_xfer_complete_sem\n");
    }
    if (spi_rx_complete_sem == NULL) {
        spi_rx_complete_sem = osSemaphoreNew(1, 0, NULL);
        if (spi_rx_complete_sem == NULL) {
            //printf("[SPI] ERROR: Failed to create spi_rx_complete_sem\n");
            return;
        }
        //printf("[SPI] Created spi_rx_complete_sem\n");
    }
    
    /* Enable SPI interrupts in HAL */
    //printf("[SPI] Enabling SPI interrupts...\n");
    __HAL_SPI_ENABLE_IT(g_sd_spi_cfg.hspi, SPI_IT_RXNE | SPI_IT_ERR);
    
    /* Verify that SPI is enabled */
    if ((g_sd_spi_cfg.hspi->Instance->CR1 & SPI_CR1_SPE) == 0) {
        //printf("[SPI] WARNING: SPI peripheral not enabled! Enabling now...\n");
        __HAL_SPI_ENABLE(g_sd_spi_cfg.hspi);
    } else {
        //printf("[SPI] SPI peripheral is enabled\n");
    }
    
    /* Ensure CS is high (deselected) */
    //printf("[SPI] Setting CS pin high (deselected)\n");
    fflush(stdout);  /* Force flush output buffer */
    HAL_GPIO_WritePin(g_sd_spi_cfg.cs_port, g_sd_spi_cfg.cs_pin, GPIO_PIN_SET);
    HAL_Delay(10);  /* Use HAL_Delay, not osDelay - simpler and doesn't need scheduler */
    
    //printf("[SPI] SPI initialization completed\n");
    fflush(stdout);
}

/**
 * SPI RX Complete callback (called from HAL)
 */
void sd_spi_rx_complete_callback(SPI_HandleTypeDef *hspi)
{
    if (hspi == g_sd_spi_cfg.hspi) {
        osSemaphoreRelease(spi_rx_complete_sem);
    }
}

/**
 * SPI TX Complete callback (called from HAL)
 */
void sd_spi_tx_complete_callback(SPI_HandleTypeDef *hspi)
{
    if (hspi == g_sd_spi_cfg.hspi) {
        osSemaphoreRelease(spi_xfer_complete_sem);
    }
}

/**
 * SPI Error callback
 */
void sd_spi_error_callback(SPI_HandleTypeDef *hspi)
{
    if (hspi == g_sd_spi_cfg.hspi) {
        spi_error_flag = true;
        osSemaphoreRelease(spi_xfer_complete_sem);
    }
}

/**
 * Select SD card (pull CS low)
 */
void sd_spi_select(void)
{
    //printf("[CS] Selecting (LOW)\n");
    HAL_GPIO_WritePin(g_sd_spi_cfg.cs_port, g_sd_spi_cfg.cs_pin, GPIO_PIN_RESET);
    /* Small delay for CS to stabilize */
    HAL_Delay(1);
}

/**
 * Deselect SD card (release CS high)
 */
void sd_spi_deselect(void)
{
    //printf("[CS] Deselecting (HIGH)\n");
    HAL_GPIO_WritePin(g_sd_spi_cfg.cs_port, g_sd_spi_cfg.cs_pin, GPIO_PIN_SET);
}

/**
 * SPI transfer single byte (interrupt-driven)
 */
uint8_t sd_spi_xfer(uint8_t byte)
{
    uint8_t rx = 0;
    spi_error_flag = false;
    
    /* Use interrupt-driven transfer */
    spi_tx_buffer[0] = byte;
    
    HAL_StatusTypeDef hal_status = HAL_SPI_TransmitReceive_IT(g_sd_spi_cfg.hspi, spi_tx_buffer, spi_rx_buffer, 1);
    if (hal_status != HAL_OK) {
        // printf("[SPI] ERROR: HAL_SPI_TransmitReceive_IT failed with status %d\n", hal_status);
        return 0xFF; /* Error starting transfer */
    }
    
    /* Wait for transfer complete with timeout (1 second) */
    osStatus_t status = osSemaphoreAcquire(spi_xfer_complete_sem, 1000);
    
    if (status == osOK && !spi_error_flag) {
        rx = spi_rx_buffer[0];
        // printf("[SPI] Transfer OK: sent=0x%02X, rcv=0x%02X\n", byte, rx);
    } else if (status == osErrorTimeout) {
        //printf("[SPI] ERROR: Semaphore timeout waiting for transfer\n");
        rx = 0xFF;
    } else if (spi_error_flag) {
       // printf("[SPI] ERROR: Transfer error flag set\n");
        rx = 0xFF;
    } else {
       // printf("[SPI] ERROR: Semaphore status %d\n", status);
        rx = 0xFF;
    }
    
    return rx;
}

/**
 * SPI transfer block using DMA (read or write)
 */
void sd_spi_xfer_block(uint8_t *buf, uint16_t len, bool is_read)
{
    if (len > SPI_DMA_BUFFER_SIZE) {
        len = SPI_DMA_BUFFER_SIZE; /* Limit to buffer size */
    }
    
    spi_error_flag = false;
    
    if (is_read) {
        /* Read: fill TX buffer with 0xFF and receive into buf */
        for (uint16_t i = 0; i < len; i++) {
            spi_tx_buffer[i] = 0xFF;
        }
        
        if (HAL_SPI_TransmitReceive_DMA(g_sd_spi_cfg.hspi, spi_tx_buffer, buf, len) != HAL_OK) {
            return; /* Error starting DMA */
        }
    } else {
        /* Write: send buf */
        if (HAL_SPI_TransmitReceive_DMA(g_sd_spi_cfg.hspi, buf, spi_rx_buffer, len) != HAL_OK) {
            return; /* Error starting DMA */
        }
    }
    
    /* Wait for DMA transfer complete (2 second timeout) */
    osStatus_t status = osSemaphoreAcquire(spi_xfer_complete_sem, 2000);
    
    if (status != osOK || spi_error_flag) {
        /* Transfer failed - reinitialize SPI */
        HAL_SPI_DeInit(g_sd_spi_cfg.hspi);
        HAL_SPI_Init(g_sd_spi_cfg.hspi);
    }
}

/**
 * Calculate CRC7 for SD card commands
 */
static uint8_t sd_crc7(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        for (uint8_t j = 0; j < 8; j++) {
            crc = (crc << 1) | ((byte >> (7 - j)) & 1);
            if (crc & 0x80) {
                crc ^= 0x89;
            }
            crc &= 0x7F;
        }
    }
    return (crc << 1) | 1;
}

/**
 * Wait for card to be ready with interrupt-driven polling
 */
static int sd_wait_ready(uint16_t timeout_ms)
{
    uint32_t deadline = osKernelGetTickCount() + timeout_ms;
    
    while (osKernelGetTickCount() < deadline) {
        if (sd_spi_xfer(0xFF) == 0xFF) {
            return 0; /* Ready */
        }
        osDelay(1); /* Yield CPU while waiting */
    }
    
    return -1; /* Timeout */
}

/**
 * Send SD command with SPI protocol
 * Returns R1 response
 */
uint8_t sd_cmd_send(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    uint8_t cmd_buf[6];
    uint8_t resp;
    
    /* Build command packet */
    cmd_buf[0] = 0x40 | (cmd & 0x3F);  /* Start bit (01) + command index */
    cmd_buf[1] = (arg >> 24) & 0xFF;    /* Argument MSB */
    cmd_buf[2] = (arg >> 16) & 0xFF;
    cmd_buf[3] = (arg >> 8) & 0xFF;
    cmd_buf[4] = arg & 0xFF;             /* Argument LSB */
    
    /* Calculate CRC if not provided */
    if (crc == 0) {
        cmd_buf[5] = sd_crc7(&cmd_buf[0], 5);
    } else {
        cmd_buf[5] = crc;
    }
    
    /* Wait for card to be ready */
    if (sd_wait_ready(100) != 0) {
        return 0xFF; /* Timeout */
    }
    
    /* Send command */
    for (uint8_t i = 0; i < 6; i++) {
        sd_spi_xfer(cmd_buf[i]);
    }
    
    /* Wait for response with timeout */
    uint32_t deadline = osKernelGetTickCount() + 100;
    do {
        resp = sd_spi_xfer(0xFF);
    } while ((resp & 0x80) && osKernelGetTickCount() < deadline);
    
    return resp;
}

/**
 * Send application command (ACMD) - must be preceded by CMD55
 */
uint8_t sd_acmd_send(uint8_t cmd, uint32_t arg)
{
    uint8_t resp;
    
    /* Send CMD55 (APP_CMD) */
    resp = sd_cmd_send(SD_CMD55_APP_CMD, 0, 0);
    if (resp & 0xFE) {  /* Check for error (except idle bit) */
        return resp;
    }
    
    /* Send application command */
    return sd_cmd_send(cmd, arg, 0);
}

/**
 * Read R1 response (already done by cmd_send)
 * This waits for additional bytes if needed
 */
int sd_read_response_r1(void)
{
    return 0; /* R1 already read */
}

/**
 * Read R3 response (OCR register)
 */
int sd_read_response_r3(uint8_t *ocr)
{
    if (ocr == NULL) {
        return -1;
    }
    
    /* Read 4 bytes of OCR */
    for (uint8_t i = 0; i < 4; i++) {
        ocr[i] = sd_spi_xfer(0xFF);
    }
    
    return 0;
}

/**
 * Read data block from SD card (512 bytes)
 */
int sd_read_data_block(uint8_t *buf, uint16_t len)
{
    uint8_t token;
    uint32_t deadline;
    uint16_t crc;
    
    if (buf == NULL) {
        return -1;
    }
    
    /* Wait for data token (0xFE) with timeout */
    deadline = osKernelGetTickCount() + 1000;
    do {
        token = sd_spi_xfer(0xFF);
        if (token == 0xFE) {
            break;
        }
        if (osKernelGetTickCount() >= deadline) {
            return -1; /* Timeout */
        }
        osDelay(1); /* Yield CPU */
    } while (1);
    
    /* Read data block using DMA */
    sd_spi_xfer_block(buf, len, true);
    
    /* Read and discard CRC */
    crc = sd_spi_xfer(0xFF) << 8;
    crc |= sd_spi_xfer(0xFF);
    
    return 0;
}

/**
 * Write data block to SD card (512 bytes)
 * token: 0xFC for single block, 0xFD for multi-block, 0xFE for stop
 */
int sd_write_data_block(const uint8_t *buf, uint16_t len, uint8_t token)
{
    uint8_t resp;
    
    if (buf == NULL) {
        return -1;
    }
    
    /* Wait for card to be ready */
    if (sd_wait_ready(500) != 0) {
        return -1;
    }
    
    /* Send data token */
    sd_spi_xfer(token);
    
    /* Send data block using DMA */
    sd_spi_xfer_block((uint8_t *)buf, len, false);
    
    /* Send dummy CRC */
    sd_spi_xfer(0xFF);
    sd_spi_xfer(0xFF);
    
    /* Read data response token */
    resp = sd_spi_xfer(0xFF);
    
    if ((resp & 0x1F) != 0x05) {
        return -1; /* Write failed */
    }
    
    /* Wait for write completion with timeout */
    uint32_t deadline = osKernelGetTickCount() + 1000;
    while (osKernelGetTickCount() < deadline && sd_spi_xfer(0xFF) == 0x00) {
        osDelay(1); /* Yield while card is busy */
    }
    
    return 0;
}

/**
 * Initialize and detect SD card
 * Returns 0 on success
 */
int sd_card_detect(void)
{
    uint8_t resp;
    uint8_t ocr[4];
    uint8_t sd_version = 0;
    uint32_t deadline;
    int retry_count = 0;
    
    g_sd_card_type = SD_TYPE_UNKNOWN;
    g_sd_capacity = 0;
    
    printf("[SD] Starting card detection...\n");
    
    sd_spi_deselect();  
    osDelay(100);  /* Ensure CS is high for at least 10ms before starting */
    /* Send 80+ clock pulses with CS Hight (Mosi = 1) to initialize */
    printf("[SD] Sending 80 clock pulses...\n");
    for (uint16_t i = 0; i < 15; i++) {
        sd_spi_xfer(0xFF);
    }
    
    //sd_spi_deselect();
    osDelay(10);
    
    /* Attempt CMD0 - Go Idle State */
    sd_spi_select();
    printf("[SD] Sending CMD0 (GO_IDLE_STATE)...\n");
    resp = sd_cmd_send(SD_CMD0_GO_IDLE_STATE, 0, 0x95);
    printf("[SD] CMD0 Response: 0x%02X (expect 0x%02X)\n", resp, SD_R1_IDLE_STATE);
    if (resp != SD_R1_IDLE_STATE) {
        sd_spi_deselect();
        printf("[SD] ERROR: CMD0 failed!\n");
        return -1; /* CMD0 failed */
    }
    
    /* Send CMD8 - Check voltage range (for SDv2 detection) */
    printf("[SD] Sending CMD8 (SEND_IF_COND)...\n");
    resp = sd_cmd_send(SD_CMD8_SEND_IF_COND, 0x1AA, 0x87);
    //printf("[SD] CMD8 Response: 0x%02X\n", resp);
    if ((resp & 0x04) == 0) {
        /* CMD8 accepted - SDv2 or later */
        sd_version = 2;
        printf("[SD] Detected SDv2/SDHC/SDXC (reading R3 response)\n");
        sd_read_response_r3(ocr);
        printf("[SD] OCR: 0x%02X%02X%02X%02X\n", ocr[0], ocr[1], ocr[2], ocr[3]);
    } else {
        /* CMD8 rejected - SDv1 or MMC */
        sd_version = 1;
        printf("[SD] Detected SDv1 or MMC\n");
    }
    
    /* Send ACMD41 - Initialize (repeat until ready) */
    printf("[SD] Starting ACMD41 initialization loop...\n");
    deadline = osKernelGetTickCount() + 5000;
    retry_count = 0;
    while (osKernelGetTickCount() < deadline) {
        resp = sd_acmd_send(SD_ACMD41_SD_SEND_OP_COND, 
                            sd_version == 2 ? 0x40000000 : 0x00000000);
        printf("[SD] ACMD41[%d] Response: 0x%02X (expect 0x00)\n", retry_count, resp);
        if ((resp & SD_R1_IDLE_STATE) == 0) {
            printf("[SD] Card ready after %d attempts!\n", retry_count);
            break; /* Card is ready */
        }
        osDelay(10); /* Yield CPU instead of busy-wait */
        retry_count++;
    }
    
    if ((resp & SD_R1_IDLE_STATE)) {
        sd_spi_deselect();
        printf("[SD] ERROR: ACMD41 timeout! Card not initialized!\n");
        return -1; /* Initialization failed */
    }
    
    /* Send CMD58 - Read OCR to check card type */
    printf("[SD] Sending CMD58 (READ_OCR)...\n");
    resp = sd_cmd_send(SD_CMD58_READ_OCR, 0, 0xFD);
    printf("[SD] CMD58 Response: 0x%02X\n", resp);
    sd_read_response_r3(ocr);
    printf("[SD] OCR: 0x%02X%02X%02X%02X\n", ocr[0], ocr[1], ocr[2], ocr[3]);
    
    /* Check if SDHC/SDXC (bit 30 set in OCR) */
    if (ocr[0] & 0x40) {
        g_sd_card_type = (sd_version == 2) ? SD_TYPE_SDHC : SD_TYPE_SDXC;
        printf("[SD] Type: %s\n", (g_sd_card_type == SD_TYPE_SDHC) ? "SDHC" : "SDXC");
    } else {
        g_sd_card_type = (sd_version == 2) ? SD_TYPE_SDv2 : SD_TYPE_SDv1;
        printf("[SD] Type: %s\n", (g_sd_card_type == SD_TYPE_SDv2) ? "SDv2" : "SDv1");
    }
    
    /* Set block size to 512 bytes (for SDv1 and standard SD) */
    if (g_sd_card_type == SD_TYPE_SDv1 || g_sd_card_type == SD_TYPE_MMC) {
        printf("[SD] Setting block size to 512 bytes...\n");
        resp = sd_cmd_send(SD_CMD16_SET_BLOCKLEN, 512, 0);
        printf("[SD] CMD16 Response: 0x%02X\n", resp);
    }
    
    sd_spi_deselect();
    
    printf("[SD] Card detection completed successfully!\n");
    return 0;
}

/**
 * Get SD card type
 */
sd_card_type_t sd_card_get_type(void)
{
    return g_sd_card_type;
}

/**
 * Get SD card capacity (in 512-byte sectors)
 */
uint32_t sd_card_get_capacity(void)
{
    return g_sd_capacity;
}

/**
 * HAL SPI Transmit Complete Callback (called by HAL_SPI_IRQHandler)
 * This is the ACTUAL callback that HAL expects - it MUST have this name
 */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == g_sd_spi_cfg.hspi) {
        // printf("[SPI_CB] TxCplt\n");
        osSemaphoreRelease(spi_xfer_complete_sem);
    }
}

/**
 * HAL SPI Receive Complete Callback (called by HAL_SPI_IRQHandler)
 * For DMA RX completion
 */
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == g_sd_spi_cfg.hspi) {
        // printf("[SPI_CB] RxCplt\n");
        osSemaphoreRelease(spi_xfer_complete_sem);
    }
}

/**
 * HAL SPI Transmit-Receive Complete Callback (called by HAL_SPI_IRQHandler)
 * Used for TransmitReceive_IT and TransmitReceive_DMA
 */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == g_sd_spi_cfg.hspi) {
        osSemaphoreRelease(spi_xfer_complete_sem);
    }
}

/**
 * HAL SPI Error Callback (called by HAL_SPI_IRQHandler)
 */
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == g_sd_spi_cfg.hspi) {
        spi_error_flag = true;
        osSemaphoreRelease(spi_xfer_complete_sem);
    }
}
