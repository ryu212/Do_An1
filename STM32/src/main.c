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
#define ADC1_SQR1       (*(volatile uint32_t*)(ADC1_BASE + 0x2C))
#define ADC1_SQR3       (*(volatile uint32_t*)(ADC1_BASE + 0x34))
#define ADC1_DR         (*(volatile uint32_t*)(ADC1_BASE + 0x4C))
#define ADC_CR2_DMA              (1 << 8)
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
uint16_t ecg_buffer[SIZE];

/* ===================== FUNCTION PROTOTYPES ===================== */
void setupRCC(void);
void setupTIM3(void);
void setupGPIOA(void);
void setupADC1CH1(void);
void setupDMA(void);
void setupSPI(void);

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

            for (int i = 0; i < SIZE; i++)
            {
                SPI1_DR = ecg_buffer[i];
                while (!(SPI1_SR & (1 << 1))); // Wait TXE = 1
            }

            DMA1_CNDTR1 = SIZE;  // Reload count
            DMA1_CCR1 |= 1;      // Enable DMA again
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
    GPIOA_CRL &= ~0xFFFF0000;
    GPIOA_CRL |= (0b1011 << 16) |  // PA4: NSS (output)
                 (0b1011 << 20) |  // PA5: SCK AF push-pull
                 (0b0100 << 24) |  // PA6: MISO input floating
                 (0b1011 << 28);   // PA7: MOSI AF push-pull
}

void setupTIM3(void)
{
    TIM3_PSC = 71;      // 1 MHz timer clock
    TIM3_ARR = 3999;    // 250 Hz update rate
    TIM3_CR2 &= ~(0b111 << 4);
    TIM3_CR2 |= (0b010 << 4); // MMS = 010 (Update event as TRGO)
    TIM3_CR1 |= 1;      // Enable counter
}

void setupADC1CH1(void)
{
    ADC1_CR1 = 0;
    ADC1_SQR1 = 0;
    ADC1_SQR3 = 1;

    ADC1_CR2 = ADC_CR2_DMA | ADC_CR2_EXTTRIG | ADC_CR2_EXTSEL_TIM3_TRGO;
    ADC1_CR2 |= ADC_CR2_ADON; // Power on ADC
}

void setupDMA(void)
{
    DMA1_CCR1 &= ~1;

    DMA1_CPAR1 = (uint32_t)&ADC1_DR;
    DMA1_CMAR1 = (uint32_t)ecg_buffer;
    DMA1_CNDTR1 = SIZE;

    // Cấu hình: 16-bit, circular, MINC, high priority
    DMA1_CCR1 = (0b10 << 12) | (0b01 << 10) | (0b01 << 8) | (1 << 7) | (1 << 5);
    DMA1_CCR1 |= 1;
}

void setupSPI(void)
{
    SPI1_CR1 = (1 << 9) | (1 << 8) | (1 << 2) | (3 << 3) | (1 << 11); // Master, SSM+SSI, f/16
    SPI1_CR1 |= (1 << 6);  // Enable SPI
}
