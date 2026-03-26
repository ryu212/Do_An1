/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#ifdef __GNUC__
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TASK02_FLAG_INT              (1U << 0)

#define MAX30003_STATUS_EINT         (1UL << 23)
#define MAX30003_STATUS_RTOR         (1UL << 10)

#define MAX30003_RTOR_REG_OFFSET     10U
#define MAX30003_RTOR_LSB_RES        0.0078125f
#define MAX30003_ETAG_BITS           0x07U

#define FIFO_VALID_SAMPLE            0x00U
#define FIFO_FAST_SAMPLE             0x01U
#define FIFO_OVF                     0x07U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
void ecg_config(MAX30003_SPI *dev);
void store_sample_to_buff(int32_t sample);
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;

UART_HandleTypeDef huart1;

/* Definitions for transmit_data2s */
osThreadId_t transmit_data2sHandle;
const osThreadAttr_t transmit_data2s_attributes = {
  .name = "transmit_data2s",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for read_sensor_tas */
osThreadId_t read_sensor_tasHandle;
const osThreadAttr_t read_sensor_tas_attributes = {
  .name = "read_sensor_tas",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* USER CODE BEGIN PV */
int32_t data_buffer1[LENGTH_BUFFER] = {0};
int32_t data_buffer2[LENGTH_BUFFER] = {0};
volatile int32_t *fill_buffer =  data_buffer1;
volatile int32_t *tx_buffer = NULL;
volatile bool buffer_ready = false;
volatile uint16_t fill_index = 0;
MAX30003_SPI max30003_spi;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
static void MX_USART1_UART_Init(void);
void StartDefaultTask(void *argument);
void StartTask02(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
PUTCHAR_PROTOTYPE{
	HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
	  return ch;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of transmit_data2s */
  transmit_data2sHandle = osThreadNew(StartDefaultTask, NULL, &transmit_data2s_attributes);

  /* creation of read_sensor_tas */
  read_sensor_tasHandle = osThreadNew(StartTask02, NULL, &read_sensor_tas_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_8)
    {
      printf("exti fired\r\n");
      if (read_sensor_tasHandle != NULL)
      {
          osThreadFlagsSet(read_sensor_tasHandle, TASK02_FLAG_INT);
      }
    }
}

void ecg_config(MAX30003_SPI *dev)
{
  HAL_StatusTypeDef status;

  GeneralConfiguration_u      cnfg_gen;
  ECGConfiguration_u          cnfg_ecg;
  RtoR1Configuration_u        cnfg_rtor;
  ManageInterrupts_u          mngr_int;
  EnableInterrupts_u          en_int;
  ManageDynamicModes_u        mngr_dyn;
  MuxConfiguration_u          cnfg_mux;

  if (dev == NULL)
  {
    printf("ecg_config: dev is NULL\r\n");
    return;
  }

  printf("ecg_config: start\r\n");

  // Reset ECG to clear registers
  printf("ecg_config: write SW_RST\r\n");
  status = max30003_writeRegister(dev, SW_RST, 0);
  if (status != HAL_OK)
  {
    printf("ecg_config: write SW_RST failed, status = %d\r\n", status);
    return;
  }

  printf("ecg_config: SW_RST ok\r\n");
  osDelay(100);

  // general config register
  cnfg_gen.all = 0;
  cnfg_gen.bits.en_ecg = 1;
  cnfg_gen.bits.rbiasn = 1;
  cnfg_gen.bits.rbiasp = 1;
  cnfg_gen.bits.en_rbias = 1;
  cnfg_gen.bits.imag = 2;
  cnfg_gen.bits.en_dcloff = 1;

  printf("ecg_config: write CNFG_GEN = 0x%08lX\r\n", cnfg_gen.all);
  status = max30003_writeRegister(dev, CNFG_GEN, cnfg_gen.all);
  if (status != HAL_OK)
  {
    printf("ecg_config: write CNFG_GEN failed, status = %d\r\n", status);
    return;
  }
  printf("ecg_config: CNFG_GEN ok\r\n");

  // ECG config register
  cnfg_ecg.all = 0;
  cnfg_ecg.bits.dlpf = 1;
  cnfg_ecg.bits.dhpf = 1;
  cnfg_ecg.bits.gain = 3;
  cnfg_ecg.bits.rate = 2;

  printf("ecg_config: write CNFG_ECG = 0x%08lX\r\n", cnfg_ecg.all);
  status = max30003_writeRegister(dev, CNFG_ECG, cnfg_ecg.all);
  if (status != HAL_OK)
  {
    printf("ecg_config: write CNFG_ECG failed, status = %d\r\n", status);
    return;
  }
  printf("ecg_config: CNFG_ECG ok\r\n");

  // R-to-R config
  cnfg_rtor.all = 0;
  cnfg_rtor.bits.wndw = 0x03;
  cnfg_rtor.bits.rgain = 0x0F;
  cnfg_rtor.bits.pavg = 0x03;
  cnfg_rtor.bits.ptsf = 0x03;
  cnfg_rtor.bits.en_rtor = 1;

  printf("ecg_config: write CNFG_RTOR1 = 0x%08lX\r\n", cnfg_rtor.all);
  status = max30003_writeRegister(dev, CNFG_RTOR1, cnfg_rtor.all);
  if (status != HAL_OK)
  {
    printf("ecg_config: write CNFG_RTOR1 failed, status = %d\r\n", status);
    return;
  }
  printf("ecg_config: CNFG_RTOR1 ok\r\n");

  // manage interrupts register
  mngr_int.all = 0;
  mngr_int.bits.efit = 0x03;
  mngr_int.bits.clr_rrint = 0x01;

  printf("ecg_config: write MNGR_INT = 0x%08lX\r\n", mngr_int.all);
  status = max30003_writeRegister(dev, MNGR_INT, mngr_int.all);
  if (status != HAL_OK)
  {
    printf("ecg_config: write MNGR_INT failed, status = %d\r\n", status);
    return;
  }
  printf("ecg_config: MNGR_INT ok\r\n");

  // Enable interrupts register setting
  en_int.all = 0;
  en_int.bits.en_eint = 1;
  en_int.bits.en_rrint = 1;
  en_int.bits.intb_type = 0x03;   // 01b = CMOS driver

  printf("ecg_config: write EN_INT = 0x%08lX\r\n", en_int.all);
  status = max30003_writeRegister(dev, EN_INT, en_int.all);
  if (status != HAL_OK)
  {
    printf("ecg_config: write EN_INT failed, status = %d\r\n", status);
    return;
  }
  printf("ecg_config: EN_INT ok\r\n");

  // dynamic mode config
  mngr_dyn.all = 0;
  mngr_dyn.bits.fast = 0;

  printf("ecg_config: write MNGR_DYN = 0x%08lX\r\n", mngr_dyn.all);
  status = max30003_writeRegister(dev, MNGR_DYN, mngr_dyn.all);
  if (status != HAL_OK)
  {
    printf("ecg_config: write MNGR_DYN failed, status = %d\r\n", status);
    return;
  }
  printf("ecg_config: MNGR_DYN ok\r\n");

  // MUX config
  cnfg_mux.all = 0;
  cnfg_mux.bits.openn = 0;
  cnfg_mux.bits.openp = 0;

  printf("ecg_config: write CNFG_EMUX = 0x%08lX\r\n", cnfg_mux.all);
  status = max30003_writeRegister(dev, CNFG_EMUX, cnfg_mux.all);
  if (status != HAL_OK)
  {
    printf("ecg_config: write CNFG_EMUX failed, status = %d\r\n", status);
    return;
  }
  printf("ecg_config: CNFG_EMUX ok\r\n");

  printf("ecg_config: done\r\n");
}
void store_sample_to_buff(int32_t sample){
  if(buffer_ready){
    return;
  }
  fill_buffer[fill_index++] = sample;
  if(fill_index >= LENGTH_BUFFER){
    tx_buffer = fill_buffer;
    buffer_ready = true;
    fill_buffer = (fill_buffer == data_buffer1) ? data_buffer2 : data_buffer1;
    fill_index = 0;
  }
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the transmit_data2s thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    if(buffer_ready && tx_buffer != NULL){
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
      HAL_SPI_Transmit(&hspi1, (uint8_t*)(void*)tx_buffer, 
                                LENGTH_BUFFER * sizeof(int32_t), 
                                HAL_MAX_DELAY);
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
      buffer_ready = false;
      tx_buffer = NULL;
    }
    osDelay(1);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartTask02 */
/**
* @brief Function implementing the read_sensor_tas thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask02 */
void StartTask02(void *argument)
{
  /* USER CODE BEGIN StartTask02 */
  max30003_init(&max30003_spi, &hspi2, GPIOB, GPIO_PIN_12);
  ecg_config(&max30003_spi);

  if (max30003_writeRegister(&max30003_spi, SYNCH, 0) != HAL_OK)
  {
      printf("MAX30003 SYNCH failed\r\n");
      Error_Handler();
  }

  printf("MAX30003 init success\r\n");

  uint32_t status = 0;
  uint32_t ecgFIFO = 0;
  uint32_t rtor_raw = 0;

  uint8_t etag = 0;
  uint8_t readECGSamples = 0;
  uint8_t idx = 0;

  int32_t ecgSample[32];
  uint8_t ETAG[32];

  uint16_t RtoR = 0;
  float BPM = 0.0f;

  uint32_t fifo_ovf_count = 0;
  uint32_t valid_sample_count = 0;

  /* Infinite loop */
  for(;;)
  {
    // uint32_t status = 0;

    // if (max30003_readRegister(&max30003_spi, STATUS, &status) == HAL_OK)
    // {
    //     printf("STATUS = 0x%08lX\r\n", status);
    // }
    // else
    // {
    //     printf("read STATUS failed\r\n");
    // }

    // osDelay(500);
    /* Read back ECG samples from the FIFO */
    osThreadFlagsWait(TASK02_FLAG_INT, osFlagsWaitAny, 100);

    //printf("Interrupt received....\r\n");

    // Read the STATUS register
    if (max30003_readRegister(&max30003_spi, STATUS, &status) != HAL_OK)
    {
        //printf("read STATUS failed\r\n");
        continue;
    }

    //printf("Status : 0x%08lX\r\n", status);
    //printf("Current BPM is %3.2f\r\n\r\n", BPM);

    // Check if R-to-R interrupt asserted
    if ((status & MAX30003_STATUS_RTOR) == MAX30003_STATUS_RTOR)
    {
      //printf("R-to-R Interrupt\r\n");

      // Read RtoR register
      if (max30003_readRegister(&max30003_spi, RTOR, &rtor_raw) == HAL_OK)
      {
        RtoR = (uint16_t)(rtor_raw >> MAX30003_RTOR_REG_OFFSET);

        // Convert to BPM
        if (RtoR != 0)
        {
          BPM = 1.0f / ((float)RtoR * MAX30003_RTOR_LSB_RES / 60.0f);
        }
        else
        {
          BPM = 0.0f;
          //printf("warning: RtoR = 0\r\n");
        }

        //printf("RtoR : %u\r\n\r\n", RtoR);
      }
      else
      {
        //printf("read RTOR failed\r\n");
      }
    }

    // Check if EINT interrupt asserted
    if ((status & MAX30003_STATUS_EINT) == MAX30003_STATUS_EINT)
    {
      //printf("FIFO Interrupt\r\n");

      readECGSamples = 0; // Reset sample counter

      do
      {
        // Read FIFO
        if (max30003_readRegister(&max30003_spi, ECG_FIFO, &ecgFIFO) != HAL_OK)
        {
          //printf("read ECG_FIFO failed\r\n");
          break;
        }

        // Isolate voltage data (theo demo của nhà sản xuất)
        ecgSample[readECGSamples] = (int32_t)(ecgFIFO >> 8);

        // Isolate ETAG
        ETAG[readECGSamples] = (uint8_t)((ecgFIFO >> 3) & MAX30003_ETAG_BITS);

        readECGSamples++;

        // chống tràn mảng cục bộ
        if (readECGSamples >= 32)
        {
          //printf("local FIFO buffer full (32 entries), stop reading\r\n");
          break;
        }

        etag = ETAG[readECGSamples - 1];

        if (etag == FIFO_OVF)
        {
          break;
        }
      }
      // Check that sample is not last sample in FIFO
      while (ETAG[readECGSamples - 1] == FIFO_VALID_SAMPLE ||
             ETAG[readECGSamples - 1] == FIFO_FAST_SAMPLE);

      //printf("%u samples read from FIFO\r\n", readECGSamples);

      // Check if FIFO has overflowed
      if ((readECGSamples > 0) && (ETAG[readECGSamples - 1] == FIFO_OVF))
      {
        if (max30003_writeRegister(&max30003_spi, FIFO_RST, 0) == HAL_OK)
        {
          fifo_ovf_count++;
        }
        else
        {
          //printf("FIFO overflow -> FIFO reset failed\r\n");
        }
      }

      // Print results
      for (idx = 0; idx < readECGSamples; idx++)
      {
        // chỉ lưu vào buffer khi là sample hợp lệ
        if ((ETAG[idx] == FIFO_VALID_SAMPLE) ||
            (ETAG[idx] == FIFO_FAST_SAMPLE))
        {
          store_sample_to_buff(ecgSample[idx]);
          valid_sample_count++;
        }
        else
        {
          //printf("skip storing sample[%u] because ETAG = 0x%X\r\n", idx, ETAG[idx]);
        }
      }

      if ((valid_sample_count % 32U) == 0U && valid_sample_count != 0U)
      {
        printf("valid_sample_count=%lu, fifo_ovf_count=%lu, BPM=%0.2f\r\n",
               (unsigned long)valid_sample_count,
               (unsigned long)fifo_ovf_count,
               BPM);
      }

      //printf("\r\n\r\n\r\n");
    }
    else
    {
      //printf("No EINT interrupt flag in STATUS\r\n\r\n");
    }
  }
  /* USER CODE END StartTask02 */
}
/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM3 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM3)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */
  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
