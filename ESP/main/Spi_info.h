// config queue / frame 
#define SPI_FRAME_BYTES 512     
#define ECG_SAMPLES_PER_FRAME   250
#define ECG_PAYLOAD_BYTES   (ECG_SAMPLES_PER_FRAME * 2) 
#define SPI_QUEUE_LEN   4     

// header/CRC
#define SPI_HDR0    0xAA
#define SPI_HDR1    0x55