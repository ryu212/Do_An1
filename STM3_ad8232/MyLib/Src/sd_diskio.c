/**
 * @file sd_diskio.c
 * @brief SD card disk I/O driver for FatFs
 * 
 * Implements the Diskio_drvTypeDef handler structure that Middlewares/diskio.c calls
 */

#include "diskio.h"
#include "ff.h"
#include "sd_spi.h"
#include <stddef.h>

/* SD driver initialization flag */
static uint8_t sd_initialized = 0;

/**
 * Get disk status
 */
static DSTATUS sd_disk_status(BYTE lun)
{
    (void)lun;  /* Suppress unused warning */
    
    if (!sd_initialized) {
        return STA_NOINIT;
    }
    return 0;  /* OK */
}

/**
 * Initialize disk
 */
static DSTATUS sd_disk_initialize(BYTE lun)
{
    (void)lun;
    
    /* Detect and initialize SD card */
    if (sd_card_detect() != 0) {
        sd_initialized = 0;
        return STA_NOINIT;
    }
    
    sd_initialized = 1;
    return 0;  /* OK */
}

/**
 * Read sector(s)
 */
static DRESULT sd_disk_read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
{
    uint8_t resp;
    
    (void)lun;
    
    if (!sd_initialized) {
        return RES_NOTRDY;
    }
    
    if (buff == NULL || count == 0) {
        return RES_PARERR;
    }
    
    sd_spi_select();
    
    /* Convert sector address based on card type */
    DWORD addr;
    if (sd_card_get_type() >= SD_TYPE_SDHC) {
        addr = sector;      /* Block address for SDHC/SDXC */
    } else {
        addr = sector << 9; /* Byte address for standard SD */
    }
    
    if (count == 1) {
        /* Single block read */
        resp = sd_cmd_send(SD_CMD17_READ_BLOCK, addr, 0);
        if (resp != 0x00) {
            sd_spi_deselect();
            return RES_ERROR;
        }
        
        if (sd_read_data_block(buff, 512) != 0) {
            sd_spi_deselect();
            return RES_ERROR;
        }
    } else {
        /* Multi-block read */
        resp = sd_cmd_send(SD_CMD18_READ_MULTI, addr, 0);
        if (resp != 0x00) {
            sd_spi_deselect();
            return RES_ERROR;
        }
        
        for (UINT i = 0; i < count; i++) {
            if (sd_read_data_block(&buff[i * 512], 512) != 0) {
                sd_cmd_send(SD_CMD12_STOP_TRANS, 0, 0);
                sd_spi_deselect();
                return RES_ERROR;
            }
        }
        
        sd_cmd_send(SD_CMD12_STOP_TRANS, 0, 0);
    }
    
    sd_spi_deselect();
    return RES_OK;
}

/**
 * Write sector(s)
 */
static DRESULT sd_disk_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count)
{
    uint8_t resp;
    
    (void)lun;
    
    if (!sd_initialized) {
        return RES_NOTRDY;
    }
    
    if (buff == NULL || count == 0) {
        return RES_PARERR;
    }
    
    sd_spi_select();
    
    /* Convert sector address based on card type */
    DWORD addr;
    if (sd_card_get_type() >= SD_TYPE_SDHC) {
        addr = sector;
    } else {
        addr = sector << 9;
    }
    
    if (count == 1) {
        /* Single block write */
        resp = sd_cmd_send(SD_CMD24_WRITE_BLOCK, addr, 0);
        if (resp != 0x00) {
            sd_spi_deselect();
            return RES_ERROR;
        }
        
        if (sd_write_data_block(buff, 512, 0xFE) != 0) {
            sd_spi_deselect();
            return RES_ERROR;
        }
    } else {
        /* Multi-block write */
        resp = sd_cmd_send(SD_CMD23_SET_BLK_CNT, count, 0);
        resp = sd_cmd_send(SD_CMD25_WRITE_MULTI, addr, 0);
        
        if (resp != 0x00) {
            sd_spi_deselect();
            return RES_ERROR;
        }
        
        for (UINT i = 0; i < count; i++) {
            if (sd_write_data_block(&buff[i * 512], 512, 0xFC) != 0) {
                sd_spi_deselect();
                return RES_ERROR;
            }
        }
        
        sd_spi_select();
        sd_spi_xfer(0xFD);  /* Stop token */
        
        uint16_t timeout = 100000;
        while (timeout-- && sd_spi_xfer(0xFF) == 0x00) {
            /* Wait for card to finish */
        }
    }
    
    sd_spi_deselect();
    return RES_OK;
}

/**
 * I/O Control
 */
static DRESULT sd_disk_ioctl(BYTE lun, BYTE cmd, void *buff)
{
    (void)lun;
    
    if (!sd_initialized) {
        return RES_NOTRDY;
    }
    
    switch (cmd) {
        case GET_SECTOR_SIZE:
            *(WORD *)buff = 512;
            return RES_OK;
        
        case GET_SECTOR_COUNT:
            /* Return a default value (could read from CSD register) */
            *(DWORD *)buff = 0x10000;  /* 64GB default */
            return RES_OK;
        
        case GET_BLOCK_SIZE:
            *(DWORD *)buff = 1;
            return RES_OK;
        
        case CTRL_SYNC:
        case CTRL_TRIM:
            return RES_OK;
        
        default:
            return RES_PARERR;
    }
}

/* ============================================================================
 * Disk Driver Handler Structure (called by Middlewares/diskio.c)
 * ============================================================================
 */

const Diskio_drvTypeDef SD_Driver = {
    .disk_initialize = sd_disk_initialize,
    .disk_status = sd_disk_status,
    .disk_read = sd_disk_read,
    .disk_write = sd_disk_write,
    .disk_ioctl = sd_disk_ioctl
};

/**
 * Get FatFs time (for file timestamps)
 */
__weak DWORD get_fattime(void)
{
    /* Return fixed time: 2024-01-01 00:00:00 */
    return ((2024 - 1980) << 25) | (1 << 21) | (1 << 16);
}
