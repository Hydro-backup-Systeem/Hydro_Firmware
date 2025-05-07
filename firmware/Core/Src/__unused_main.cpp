///* USER CODE BEGIN Header */
///**
//  ******************************************************************************
//  * @file           : main.c
//  * @brief          : Main program body
//  ******************************************************************************
//  * @attention
//  *
//  * Copyright (c) 2025 STMicroelectronics.
//  * All rights reserved.
//  *
//  * This software is licensed under terms that can be found in the LICENSE file
//  * in the root directory of this software component.
//  * If no LICENSE file comes with this software, it is provided AS-IS.
//  *
//  ******************************************************************************
//  */
///* USER CODE END Header */
///* Includes ------------------------------------------------------------------*/
//#include "main.h"
//
///* Private includes ----------------------------------------------------------*/
///* USER CODE BEGIN Includes */
//#include "audio_processing.h"
//
//#include "stai.h"
//#include "network.h"
//#include "network_data.h"
//
//#include "lora_sx1276.h"
//
//#include <stdio.h>
///* USER CODE END Includes */
//
///* Private typedef -----------------------------------------------------------*/
///* USER CODE BEGIN PTD */
//
///* USER CODE END PTD */
//
///* Private define ------------------------------------------------------------*/
///* USER CODE BEGIN PD */
//#define CHUNK_SIZE 256
///* USER CODE END PD */
//
///* Private macro -------------------------------------------------------------*/
///* USER CODE BEGIN PM */
///* USER CODE END PM */
//
///* Private variables ---------------------------------------------------------*/
//
//ADC_HandleTypeDef hadc1;
//
//SPI_HandleTypeDef hspi1;
//
//TIM_HandleTypeDef htim2;
//
//UART_HandleTypeDef huart1;
//
//HCD_HandleTypeDef hhcd_USB_OTG_HS;
//
///* USER CODE BEGIN PV */
//volatile uint8_t recording_done = 0;
//volatile uint32_t adcIndex = 0;
//
//
//// TODO: Temporaraly
//#define BUFFER_SIZE 16000
//#define STFFT_FRAMES 124
//#define FFT_SIZE     256
//#define NUM_BINS	 (FFT_SIZE / 2)
//
////* Audio input buffer
//uint16_t adcBuffer[BUFFER_SIZE];
//
////* Spectrogram AI input buffer
//uint8_t spectrogramBuffer[(STFFT_FRAMES * (NUM_BINS / 2)) + NUM_BINS];
//
////* Variables for AI Network
//ai_buffer* ai_input;
//ai_buffer* ai_output;
//
//ai_u8 out_data[AI_NETWORK_OUT_1_SIZE_BYTES];
//ai_u8 activations[AI_NETWORK_DATA_ACTIVATIONS_SIZE];
//
///* USER CODE END PV */
//
///* Private function prototypes -----------------------------------------------*/
//void SystemClock_Config(void);
//static void SystemPower_Config(void);
//static void MX_GPIO_Init(void);
//static void MX_ICACHE_Init(void);
//static void MX_USB_OTG_HS_HCD_Init(void);
//static void MX_ADC1_Init(void);
//static void MX_TIM2_Init(void);
//static void MX_USART1_UART_Init(void);
//static void MX_SPI1_Init(void);
///* USER CODE BEGIN PFP */
//const char *labels[] = { "down", "go", "left", "no", "right", "stop", "up", "yes" };
//
//void softmax(ai_u8 *input, float *output, int size) {
//    float max_val = input[0];
//    for (int i = 1; i < size; i++) {
//        if (input[i] > max_val) {
//            max_val = input[i];  // Find max value for numerical stability
//        }
//    }
//
//    float sum = 0.0;
//
//    // Compute exponentials (with max subtraction)
//    for (int i = 0; i < size; i++) {
//        output[i] = exp((float)input[i] - max_val);
//        sum += output[i];
//    }
//
//    // Normalize values
//    for (int i = 0; i < size; i++) {
//        output[i] /= sum;
//    }
//}
//
//void send_softmax_results(ai_u8 *out_data, int size) {
//    float softmax_out[size];
//    softmax(out_data, softmax_out, size);
//
//    char buffer[100]; // Buffer for UART transmission
//
//    for (int i = 0; i < size; i++) {
//        snprintf(buffer, sizeof(buffer), "%s: %.6f\n\r", labels[i], softmax_out[i]);
//        HAL_UART_Transmit(&huart1, (uint8_t *)buffer, strlen(buffer), 1000);
//    }
//}
///* USER CODE END PFP */
//
///* Private user code ---------------------------------------------------------*/
///* USER CODE BEGIN 0 */
//
///* USER CODE END 0 */
//
///**
//  * @brief  The application entry point.
//  * @retval int
//  */
//int _____unsused_main(void)
//{
//
//  /* USER CODE BEGIN 1 */
//
//  /* USER CODE END 1 */
//
//  /* MCU Configuration--------------------------------------------------------*/
//
//  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
//  HAL_Init();
//
//  /* USER CODE BEGIN Init */
//
//  /* USER CODE END Init */
//
//  /* Configure the System Power */
//  SystemPower_Config();
//
//  /* Configure the system clock */
//  SystemClock_Config();
//
//  /* USER CODE BEGIN SysInit */
//
//  /* USER CODE END SysInit */
//
//  /* Initialize all configured peripherals */
//  MX_GPIO_Init();
//  MX_ICACHE_Init();
//  MX_USB_OTG_HS_HCD_Init();
//  MX_ADC1_Init();
//  MX_TIM2_Init();
//  MX_USART1_UART_Init();
//  MX_SPI1_Init();
//  /* USER CODE BEGIN 2 */
//
//  /* USER CODE END 2 */
//
//  /* Initialize leds */
//  BSP_LED_Init(LED_GREEN);
//  BSP_LED_Init(LED_BLUE);
//  BSP_LED_Init(LED_RED);
//
//  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
//  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);
//
//  /* Infinite loop */
//  /* USER CODE BEGIN WHILE */
//  AudioProcessor processor;
//
//	ai_handle network = AI_HANDLE_NULL;
//	ai_error err;
//	ai_network_report report;
//
//	// Initialize network
//	const ai_handle acts[] = { activations };
//
//	err = ai_network_create_and_init(&network, acts, NULL);
//	if (err.type != AI_ERROR_NONE) {
//	  Error_Handler();
//	}
//
//	 if (ai_network_get_report(network, &report) != true) {
//	   Error_Handler();
//	 }
//
//	ai_input = ai_network_inputs_get(network, NULL);
//	ai_output = ai_network_outputs_get(network, NULL);
//
//	ai_input[0].data = AI_HANDLE_PTR(spectrogramBuffer);
//	ai_output[0].data = AI_HANDLE_PTR(out_data);
//
//	ai_i32 n_batch;
//
//	// Lora initialization
//	lora_sx1276 lora;
//
//	uint8_t res = lora_init(&lora, &hspi1, SPI_NSS_GPIO_Port, SPI_NSS_Pin, LORA_BASE_FREQUENCY_EU);
//	if (res != LORA_OK) {
//		Error_Handler();
//	}
//
//	lora_set_tx_power(&lora, 20);
//
//  while (1) {
//
//    /* USER CODE END WHILE */
//
//    /* USER CODE BEGIN 3 */
//
//	  res = lora_send_packet(&lora, (uint8_t *)"test", 4);
//	  if (res != LORA_OK) {
//		  Error_Handler();
//	  }
//
//	  HAL_Delay(1000);
//
//    if (recording_done == 1) {
//		// Set rest of the buffer to 0
//		if (adcIndex < BUFFER_SIZE) {
//		  memset(&adcBuffer[adcIndex], 0, (BUFFER_SIZE - adcIndex) * sizeof(uint16_t));
//		}
//
//
////		 //* Transmit data to be used with read_stm32.py script
////		 // Send a frame header for synchronization
////		 const uint8_t frameHeader[2] = {0xAA, 0x55};
////
////		 // Send ADC buffer in smaller chunks
////		 for (int i = 0; i < BUFFER_SIZE; i += CHUNK_SIZE) {
////		   HAL_UART_Transmit(&huart1, (uint8_t*)&adcBuffer[i], CHUNK_SIZE * sizeof(uint16_t), HAL_MAX_DELAY);
////		 }
////
////		 // Send spectrogram data - Time frame per Time frame
////		 for (uint8_t i = 0; i < STFFT_FRAMES; i++) {
////		   HAL_UART_Transmit(&huart1, frameHeader, 2, 100);
////		   if (HAL_UART_Transmit(&huart1, &spectrogramBuffer[i * (NUM_BINS / 2)], NUM_BINS / 2, 1000) != HAL_OK) Error_Handler();
////		 }
//
//		n_batch = ai_network_run(network, ai_input, ai_output);
//		if (n_batch != 1) {
//		  Error_Handler();
//		}
//
//		send_softmax_results(out_data, 8);
//		HAL_UART_Transmit(&huart1, (const uint8_t*)"\n\n", 3, 100);
//
//		adcIndex = 0;  // Reset index for new recording
//		recording_done = 0;  // Allow new recordings
//	  }
//	}
//  /* USER CODE END 3 */
//}
//
///**
//  * @brief System Clock Configuration
//  * @retval None
//  */
//void SystemClock_Config(void)
//{
//  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
//  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
//
//  /** Configure the main internal regulator output voltage
//  */
//  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
//  {
//    Error_Handler();
//  }
//
//  /** Initializes the CPU, AHB and APB buses clocks
//  */
//  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE
//                              |RCC_OSCILLATORTYPE_MSI;
//  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
//  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
//  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
//  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
//  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
//  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_0;
//  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
//  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
//  RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV1;
//  RCC_OscInitStruct.PLL.PLLM = 1;
//  RCC_OscInitStruct.PLL.PLLN = 10;
//  RCC_OscInitStruct.PLL.PLLP = 8;
//  RCC_OscInitStruct.PLL.PLLQ = 2;
//  RCC_OscInitStruct.PLL.PLLR = 1;
//  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLLVCIRANGE_1;
//  RCC_OscInitStruct.PLL.PLLFRACN = 0;
//  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
//  {
//    Error_Handler();
//  }
//
//  /** Initializes the CPU, AHB and APB buses clocks
//  */
//  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
//                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
//                              |RCC_CLOCKTYPE_PCLK3;
//  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
//  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
//  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
//  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
//  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;
//
//  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
//  {
//    Error_Handler();
//  }
//}
//
///**
//  * @brief Power Configuration
//  * @retval None
//  */
//static void SystemPower_Config(void)
//{
//  HAL_PWREx_EnableVddIO2();
//
//  /*
//   * Disable the internal Pull-Up in Dead Battery pins of UCPD peripheral
//   */
//  HAL_PWREx_DisableUCPDDeadBattery();
//
//  /*
//   * Switch to SMPS regulator instead of LDO
//   */
//  if (HAL_PWREx_ConfigSupply(PWR_SMPS_SUPPLY) != HAL_OK)
//  {
//    Error_Handler();
//  }
///* USER CODE BEGIN PWR */
///* USER CODE END PWR */
//}
//
///**
//  * @brief ADC1 Initialization Function
//  * @param None
//  * @retval None
//  */
//static void MX_ADC1_Init(void)
//{
//
//  /* USER CODE BEGIN ADC1_Init 0 */
//
//  /* USER CODE END ADC1_Init 0 */
//
//  ADC_ChannelConfTypeDef sConfig = {0};
//
//  /* USER CODE BEGIN ADC1_Init 1 */
//
//  /* USER CODE END ADC1_Init 1 */
//
//  /** Common config
//  */
//  hadc1.Instance = ADC1;
//  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
//  hadc1.Init.Resolution = ADC_RESOLUTION_14B;
//  hadc1.Init.GainCompensation = 0;
//  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
//  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
//  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
//  hadc1.Init.LowPowerAutoWait = DISABLE;
//  hadc1.Init.ContinuousConvMode = DISABLE;
//  hadc1.Init.NbrOfConversion = 1;
//  hadc1.Init.DiscontinuousConvMode = DISABLE;
//  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
//  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
//  hadc1.Init.DMAContinuousRequests = DISABLE;
//  hadc1.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH;
//  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
//  hadc1.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
//  hadc1.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
//  hadc1.Init.OversamplingMode = DISABLE;
//  if (HAL_ADC_Init(&hadc1) != HAL_OK)
//  {
//    Error_Handler();
//  }
//
//  /** Configure Regular Channel
//  */
//  sConfig.Channel = ADC_CHANNEL_8;
//  sConfig.Rank = ADC_REGULAR_RANK_1;
//  sConfig.SamplingTime = ADC_SAMPLETIME_5CYCLE;
//  sConfig.SingleDiff = ADC_SINGLE_ENDED;
//  sConfig.OffsetNumber = ADC_OFFSET_NONE;
//  sConfig.Offset = 0;
//  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  /* USER CODE BEGIN ADC1_Init 2 */
//
//  /* USER CODE END ADC1_Init 2 */
//
//}
//
///**
//  * @brief ICACHE Initialization Function
//  * @param None
//  * @retval None
//  */
//static void MX_ICACHE_Init(void)
//{
//
//  /* USER CODE BEGIN ICACHE_Init 0 */
//
//  /* USER CODE END ICACHE_Init 0 */
//
//  /* USER CODE BEGIN ICACHE_Init 1 */
//
//  /* USER CODE END ICACHE_Init 1 */
//
//  /** Enable instruction cache in 1-way (direct mapped cache)
//  */
//  if (HAL_ICACHE_ConfigAssociativityMode(ICACHE_1WAY) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  if (HAL_ICACHE_Enable() != HAL_OK)
//  {
//    Error_Handler();
//  }
//  /* USER CODE BEGIN ICACHE_Init 2 */
//
//  /* USER CODE END ICACHE_Init 2 */
//
//}
//
///**
//  * @brief SPI1 Initialization Function
//  * @param None
//  * @retval None
//  */
//static void MX_SPI1_Init(void)
//{
//
//  /* USER CODE BEGIN SPI1_Init 0 */
//
//  /* USER CODE END SPI1_Init 0 */
//
//  SPI_AutonomousModeConfTypeDef HAL_SPI_AutonomousMode_Cfg_Struct = {0};
//
//  /* USER CODE BEGIN SPI1_Init 1 */
//
//  /* USER CODE END SPI1_Init 1 */
//  /* SPI1 parameter configuration*/
//  hspi1.Instance = SPI1;
//  hspi1.Init.Mode = SPI_MODE_MASTER;
//  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
//  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
//  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
//  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
//  hspi1.Init.NSS = SPI_NSS_SOFT;
//  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
//  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
//  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
//  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
//  hspi1.Init.CRCPolynomial = 0x7;
//  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
//  hspi1.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
//  hspi1.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
//  hspi1.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
//  hspi1.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
//  hspi1.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
//  hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
//  hspi1.Init.IOSwap = SPI_IO_SWAP_DISABLE;
//  hspi1.Init.ReadyMasterManagement = SPI_RDY_MASTER_MANAGEMENT_INTERNALLY;
//  hspi1.Init.ReadyPolarity = SPI_RDY_POLARITY_HIGH;
//  if (HAL_SPI_Init(&hspi1) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerState = SPI_AUTO_MODE_DISABLE;
//  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerSelection = SPI_GRP1_GPDMA_CH0_TCF_TRG;
//  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerPolarity = SPI_TRIG_POLARITY_RISING;
//  if (HAL_SPIEx_SetConfigAutonomousMode(&hspi1, &HAL_SPI_AutonomousMode_Cfg_Struct) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  /* USER CODE BEGIN SPI1_Init 2 */
//
//  /* USER CODE END SPI1_Init 2 */
//
//}
//
///**
//  * @brief TIM2 Initialization Function
//  * @param None
//  * @retval None
//  */
//static void MX_TIM2_Init(void)
//{
//
//  /* USER CODE BEGIN TIM2_Init 0 */
//
//  /* USER CODE END TIM2_Init 0 */
//
//  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
//  TIM_MasterConfigTypeDef sMasterConfig = {0};
//
//  /* USER CODE BEGIN TIM2_Init 1 */
//
//  /* USER CODE END TIM2_Init 1 */
//  htim2.Instance = TIM2;
//  htim2.Init.Prescaler = 99;
//  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
//  htim2.Init.Period = 99;
//  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
//  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
//  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
//  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
//  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
//  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  /* USER CODE BEGIN TIM2_Init 2 */
//
//  /* USER CODE END TIM2_Init 2 */
//
//}
//
///**
//  * @brief USART1 Initialization Function
//  * @param None
//  * @retval None
//  */
//static void MX_USART1_UART_Init(void)
//{
//
//  /* USER CODE BEGIN USART1_Init 0 */
//
//  /* USER CODE END USART1_Init 0 */
//
//  /* USER CODE BEGIN USART1_Init 1 */
//
//  /* USER CODE END USART1_Init 1 */
//  huart1.Instance = USART1;
//  huart1.Init.BaudRate = 115200;
//  huart1.Init.WordLength = UART_WORDLENGTH_8B;
//  huart1.Init.StopBits = UART_STOPBITS_1;
//  huart1.Init.Parity = UART_PARITY_NONE;
//  huart1.Init.Mode = UART_MODE_TX_RX;
//  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
//  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
//  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
//  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
//  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
//  if (HAL_UART_Init(&huart1) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  /* USER CODE BEGIN USART1_Init 2 */
//
//  /* USER CODE END USART1_Init 2 */
//
//}
//
///**
//  * @brief USB_OTG_HS Initialization Function
//  * @param None
//  * @retval None
//  */
//static void MX_USB_OTG_HS_HCD_Init(void)
//{
//
//  /* USER CODE BEGIN USB_OTG_HS_Init 0 */
//
//  /* USER CODE END USB_OTG_HS_Init 0 */
//
//  /* USER CODE BEGIN USB_OTG_HS_Init 1 */
//
//  /* USER CODE END USB_OTG_HS_Init 1 */
//  hhcd_USB_OTG_HS.Instance = USB_OTG_HS;
//  hhcd_USB_OTG_HS.Init.Host_channels = 16;
//  hhcd_USB_OTG_HS.Init.speed = HCD_SPEED_HIGH;
//  hhcd_USB_OTG_HS.Init.dma_enable = DISABLE;
//  hhcd_USB_OTG_HS.Init.phy_itface = USB_OTG_HS_EMBEDDED_PHY;
//  hhcd_USB_OTG_HS.Init.Sof_enable = DISABLE;
//  hhcd_USB_OTG_HS.Init.low_power_enable = DISABLE;
//  hhcd_USB_OTG_HS.Init.use_external_vbus = ENABLE;
//  if (HAL_HCD_Init(&hhcd_USB_OTG_HS) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  /* USER CODE BEGIN USB_OTG_HS_Init 2 */
//
//  /* USER CODE END USB_OTG_HS_Init 2 */
//
//}
//
///**
//  * @brief GPIO Initialization Function
//  * @param None
//  * @retval None
//  */
//static void MX_GPIO_Init(void)
//{
//  GPIO_InitTypeDef GPIO_InitStruct = {0};
//  /* USER CODE BEGIN MX_GPIO_Init_1 */
//
//  /* USER CODE END MX_GPIO_Init_1 */
//
//  /* GPIO Ports Clock Enable */
//  __HAL_RCC_GPIOC_CLK_ENABLE();
//  __HAL_RCC_GPIOH_CLK_ENABLE();
//  __HAL_RCC_GPIOA_CLK_ENABLE();
//  __HAL_RCC_GPIOB_CLK_ENABLE();
//  __HAL_RCC_GPIOG_CLK_ENABLE();
//
//  /*Configure GPIO pin Output Level */
//  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_2, GPIO_PIN_RESET);
//
//  /*Configure GPIO pin Output Level */
//  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7|SPI_NSS_Pin, GPIO_PIN_RESET);
//
//  /*Configure GPIO pin Output Level */
//  HAL_GPIO_WritePin(GPIOB, UCPD_DBn_Pin|GPIO_PIN_7, GPIO_PIN_RESET);
//
//  /*Configure GPIO pin : PC13 */
//  GPIO_InitStruct.Pin = GPIO_PIN_13;
//  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
//
//  /*Configure GPIO pin : UCPD_FLT_Pin */
//  GPIO_InitStruct.Pin = UCPD_FLT_Pin;
//  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  HAL_GPIO_Init(UCPD_FLT_GPIO_Port, &GPIO_InitStruct);
//
//  /*Configure GPIO pin : PB15 */
//  GPIO_InitStruct.Pin = GPIO_PIN_15;
//  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
//
//  /*Configure GPIO pin : PG2 */
//  GPIO_InitStruct.Pin = GPIO_PIN_2;
//  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
//  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
//
//  /*Configure GPIO pins : PC7 SPI_NSS_Pin */
//  GPIO_InitStruct.Pin = GPIO_PIN_7|SPI_NSS_Pin;
//  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
//  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
//
//  /*Configure GPIO pin : PA15 */
//  GPIO_InitStruct.Pin = GPIO_PIN_15;
//  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
//
//  /*Configure GPIO pins : UCPD_DBn_Pin PB7 */
//  GPIO_InitStruct.Pin = UCPD_DBn_Pin|GPIO_PIN_7;
//  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
//  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
//
//  /* EXTI interrupt init*/
//  HAL_NVIC_SetPriority(EXTI13_IRQn, 0, 0);
//  HAL_NVIC_EnableIRQ(EXTI13_IRQn);
//
//  /* USER CODE BEGIN MX_GPIO_Init_2 */
//
//  /* USER CODE END MX_GPIO_Init_2 */
//}
//
///* USER CODE BEGIN 4 */
//
//
//
///* USER CODE END 4 */
//
///**
//  * @brief  This function is executed in case of error occurrence.
//  * @retval None
//  */
//void Error_Handler(void)
//{
//  /* USER CODE BEGIN Error_Handler_Debug */
//  /* User can add his own implementation to report the HAL error return state */
//  __disable_irq();
//  while (1)
//  {
//	  BSP_LED_Toggle(LED_RED);
//	  HAL_Delay(250);
//  }
//  /* USER CODE END Error_Handler_Debug */
//}
//
//#ifdef  USE_FULL_ASSERT
///**
//  * @brief  Reports the name of the source file and the source line number
//  *         where the assert_param error has occurred.
//  * @param  file: pointer to the source file name
//  * @param  line: assert_param error line source number
//  * @retval None
//  */
//void assert_failed(uint8_t *file, uint32_t line)
//{
//  /* USER CODE BEGIN 6 */
//  /* User can add his own implementation to report the file name and line number,
//     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
//  /* USER CODE END 6 */
//}
//#endif /* USE_FULL_ASSERT */
