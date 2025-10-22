#include <stdint.h>

/* ===================== RCC ===================== */
#define RCC_BASE        0x40021000
#define RCC_APB2ENR     (*(volatile uint32_t*)(RCC_BASE + 0x18))
#define RCC_APB1ENR     (*(volatile uint32_t*)(RCC_BASE + 0x1C))
#define RCC_AHBENR      (*(volatile uint32_t*)(RCC_BASE + 0x14))
#define RCC_APB2ENR_IOPAEN   (1 << 2)
#define RCC_APB2ENR_SPI1EN   (1 << 12)
#define RCC_APB2ENR_ADC1EN   (1 << 9)
#define RCC_APB1ENR_TIM3EN   (1 << 1)
#define RCC_AHBENR_DMA1EN    (1 << 0)

/* ===================== GPIOA ===================== */
#define GPIOA_BASE      0x40010800
#define GPIOA_CRL       (*(volatile uint32_t*)(GPIOA_BASE + 0x00))
#define GPIOA_CRH       (*(volatile uint32_t*)(GPIOA_BASE + 0x04))
#define GPIOA_ODR       (*(volatile uint32_t*)(GPIOA_BASE + 0x0C))
#define GPIOA_BSRR      (*(volatile uint32_t*)(GPIOA_BASE + 0x10))
#define GPIOA_BRR       (*(volatile uint32_t*)(GPIOA_BASE + 0x14))

/* ===================== ADC1 ===================== */
#define ADC1_BASE       0x40012400
#define ADC1_SR         (*(volatile uint32_t*)(ADC1_BASE + 0x00))
#define ADC1_CR1        (*(volatile uint32_t*)(ADC1_BASE + 0x04))
#define ADC1_CR2        (*(volatile uint32_t*)(ADC1_BASE + 0x08))
#define ADC1_SMPR2      (*(volatile uint32_t*)(ADC1_BASE + 0x10))
#define ADC1_SQR1       (*(volatile uint32_t*)(ADC1_BASE + 0x2C))
#define ADC1_SQR3       (*(volatile uint32_t*)(ADC1_BASE + 0x34))
#define ADC1_DR         (*(volatile uint32_t*)(ADC1_BASE + 0x4C))
#define ADC_CR2_DMA              (1 << 8)
#define ADC_CR2_RSTCAL           (1 << 3)
#define ADC_CR2_CAL              (1 << 2)
#define ADC_CR2_EXTTRIG          (1 << 20)
#define ADC_CR2_EXTSEL_TIM3_TRGO (0b100 << 17)
#define ADC_CR2_ADON             (1 << 0)

/* ===================== TIM3 ===================== */
#define TIM3_BASE       0x40000400
#define TIM3_CR1        (*(volatile uint32_t*)(TIM3_BASE + 0x00))
#define TIM3_CR2        (*(volatile uint32_t*)(TIM3_BASE + 0x04))
#define TIM3_SMCR       (*(volatile uint32_t*)(TIM3_BASE + 0x08))
#define TIM3_DIER       (*(volatile uint32_t*)(TIM3_BASE + 0x0C))
#define TIM3_SR         (*(volatile uint32_t*)(TIM3_BASE + 0x10))
#define TIM3_EGR        (*(volatile uint32_t*)(TIM3_BASE + 0x14))
#define TIM3_CNT        (*(volatile uint32_t*)(TIM3_BASE + 0x24))
#define TIM3_PSC        (*(volatile uint32_t*)(TIM3_BASE + 0x28))
#define TIM3_ARR        (*(volatile uint32_t*)(TIM3_BASE + 0x2C))

/* ===================== DMA1 ===================== */
#define DMA1_BASE        0x40020000
#define DMA1_ISR         (*(volatile uint32_t*)(DMA1_BASE + 0x00))
#define DMA1_IFCR        (*(volatile uint32_t*)(DMA1_BASE + 0x04))
#define DMA1_CCR1        (*(volatile uint32_t*)(DMA1_BASE + 0x08))
#define DMA1_CNDTR1      (*(volatile uint32_t*)(DMA1_BASE + 0x0C))
#define DMA1_CPAR1       (*(volatile uint32_t*)(DMA1_BASE + 0x10))
#define DMA1_CMAR1       (*(volatile uint32_t*)(DMA1_BASE + 0x14))

/* ===================== SPI1 ===================== */
#define SPI1_BASE     0x40013000
#define SPI1_CR1      (*(volatile uint32_t*)(SPI1_BASE + 0x00))
#define SPI1_CR2      (*(volatile uint32_t*)(SPI1_BASE + 0x04))
#define SPI1_SR       (*(volatile uint32_t*)(SPI1_BASE + 0x08))
#define SPI1_DR       (*(volatile uint32_t*)(SPI1_BASE + 0x0C))

/* ===================== APP CONFIG ===================== */
#define SIZE 250
#define FRAME_BYTES 512
#define PAYLOAD_BYTES (SIZE*2)

uint16_t ecg_buffer[SIZE];
static uint8_t  seq = 0;
/* ===================== FUNCTION PROTOTYPES ===================== */
void setupRCC(void);
void setupTIM3(void);
void setupGPIOA(void);
void setupADC1CH1(void);
void setupDMA(void);
void setupSPI(void);

static inline void CS_L(void) { GPIOA_BRR  = (1u << 4); }
static inline void CS_H(void) { GPIOA_BSRR = (1u << 4); }
static void     spi1_tx_byte(uint8_t b);
static uint8_t  crc8(const uint8_t *d, uint16_t n);
static void     send_frame_250_samples(const uint16_t *samples);
/* ===================== MAIN ===================== */
int main(void)
{
    setupRCC();
    setupGPIOA();
    setupTIM3();
    setupADC1CH1();
    setupDMA();
    setupSPI();

    while (1)
    {
        if (DMA1_CNDTR1 == 0)
        {
            DMA1_CCR1 &= ~1;  // Disable DMA
            send_frame_250_samples(ecg_buffer);
            DMA1_CNDTR1 = SIZE;  // Reload count
            DMA1_CCR1 |= 1u;      // Enable DMA again
        }
    }
}

/* ===================== SETUP FUNCTIONS ===================== */
void setupRCC(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN;   // GPIOA
    RCC_APB2ENR |= RCC_APB2ENR_SPI1EN;   // SPI1
    RCC_AHBENR  |= RCC_AHBENR_DMA1EN;    // DMA1
    RCC_APB1ENR |= RCC_APB1ENR_TIM3EN;   // TIM3
    RCC_APB2ENR |= RCC_APB2ENR_ADC1EN;   // ADC1
}

void setupGPIOA(void)
{
    // PA1 = analog input
    GPIOA_CRL &= ~(0xF << 4); // MODE1=00, CNF1=00

    // SPI pins PA4–PA7
    // clear config of PA4-5-6-7
    GPIOA_CRL &= ~((0xFu<<16)|(0xFu<<20)|(0xFu<<24)|(0xFu<<28));
    GPIOA_CRL |= (0b1011u << 16) |  // PA4: NSS (output)
                 (0b1011u << 20) |  // PA5: SCK AF push-pull
                 (0b0100u << 24) |  // PA6: MISO input floating
                 (0b1011u << 28);   // PA7: MOSI AF push-pull
    CS_H(); // idle
}

void setupTIM3(void)
{
    TIM3_PSC = 71;      // 1 MHz timer clock
    TIM3_ARR = 3999;    // 250 Hz update rate
    TIM3_CR2 &= ~(0b111 << 4);
    TIM3_CR2 |= (0b010 << 4); // MMS = 010 (Update event as TRGO)
    TIM3_CR1 |= 1u;      // Enable counter
}

void setupADC1CH1(void)
{
    ADC1_CR1 = 0;
    ADC1_SQR1 = 0;
    ADC1_SQR3 = 1;

    // Sample time ch1 = 71.5 cycles (SMPR2[5:3] = 110)
    ADC1_SMPR2 &= ~(0x7u<<3);
    ADC1_SMPR2 |=  (0x6u<<3);

    // DMA + External trigger TIM3 TRGO
    ADC1_CR2 = ADC_CR2_DMA | ADC_CR2_EXTTRIG | ADC_CR2_EXTSEL_TIM3_TRGO;
    ADC1_CR2 |= ADC_CR2_ADON; // Power on ADC
    // calibrate sequence
    for (volatile int i = 0; i < 10000; i++);
    ADC1_CR2 |= ADC_CR2_RSTCAL; while(ADC1_CR2 & ADC_CR2_RSTCAL);
    ADC1_CR2 |= ADC_CR2_CAL;    while(ADC1_CR2 & ADC_CR2_CAL);


}

void setupDMA(void)
{
    DMA1_CCR1 &= ~1; // disable

    DMA1_CPAR1 = (uint32_t)&ADC1_DR; // peripheral
    DMA1_CMAR1 = (uint32_t)ecg_buffer; // memory
    DMA1_CNDTR1 = SIZE;

    // Cấu hình: 16-bit, circular, MINC, high priority
    DMA1_CCR1 = (0b10u << 12) | (0b01u << 10) | (0b01u << 8) | (1u << 7) | (1u << 5);
    DMA1_CCR1 |= 1u;
}

void setupSPI(void)
{
    SPI1_CR1 = 0; //clear
    SPI1_CR1 = (1u << 2) | (1u << 9) | (1u << 8) | (3u << 3); // Master, SSM+SSI, f16 
    SPI1_CR1 |= (1u << 6);  // Enable SPI
}

/* ===================== SPI HELPERS ===================== */
// write data into spi1_dr 
static void spi1_tx_byte(uint8_t data){
    while(!(SPI1_SR & (1u<<1))); // TXE(Transmit buffer empty)
    *((volatile uint8_t*) &SPI1_DR) = data; // write 1 byte 
    while(!(SPI1_SR & (1u<<0))); // RXNE(Receive buffer not empty)
    (void)*((volatile uint8_t*) &SPI1_DR);
}
// checksum
static uint8_t crc8(const uint8_t *data, uint16_t len){
    uint8_t c = 0x00;
    for(uint16_t i=0;i<len;i++){
        c ^= data[i];
        for(uint8_t b = 0; b < 8; b++)
            c = (c & 0x80) ? ((c<<1)^0x07) : (c<<1);
    }
    return c;
}
static void send_frame_250_samples(const uint16_t *samples){
    uint8_t buf[FRAME_BYTES];
    // header 
    buf[0] = 0xAA;
    buf[1] = 0x55;
    buf[2] = seq++;
    buf[3]=(uint8_t)(PAYLOAD_BYTES & 0xFF);      // 500
    buf[4]=(uint8_t)((PAYLOAD_BYTES>>8)&0xFF);

    
    for (int i = 0; i < SIZE; i++){
        uint16_t tmp = samples[i];
        buf[5 + 2*i] = (uint8_t)(tmp & 0xFF); // low byte 
        buf[5 + 2*i + 1] =  (uint8_t)(tmp >> 8); // hight byte 
    }

    // CRC & pad
    buf[5 + PAYLOAD_BYTES] = crc8(&buf[5], PAYLOAD_BYTES);
    // keep CS low until comletely send data 
    CS_L();
    for(int i=0;i<FRAME_BYTES;i++) spi1_tx_byte(buf[i]);
    CS_H();
}

