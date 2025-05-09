/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32u5xx_hal.h"
#include "stm32u5xx_nucleo.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
#define USE_ENCRYPTION true
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
// TODO: Remove when this issue gets fixed
// These generate an error when compiling, but they should not do that
__attribute__((weak)) void _close(void){}
__attribute__((weak)) void _lseek(void){}
__attribute__((weak)) void _read(void){}
__attribute__((weak)) void _fstat(void){}
__attribute__((weak)) void _isatty(void){}
__attribute__((weak)) void _getpid(void){}
__attribute__((weak)) void _kill(void){}
__attribute__((weak)) void _write(void){}
__attribute__((weak)) void _gettimeofday(void){}
__attribute__((weak)) void _open(void){}
__attribute__((weak)) void _stat(void){}

// For LPBAM applications only sram4 is in same power domain as LPDMA
#define __ATR_SRAM4__ __attribute__((section(".sram4")))
/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define VBUS_SENSE_Pin GPIO_PIN_2
#define VBUS_SENSE_GPIO_Port GPIOC
#define LoRa_DIO0_Pin GPIO_PIN_3
#define LoRa_DIO0_GPIO_Port GPIOA
#define LoRa_DIO0_EXTI_IRQn EXTI3_IRQn
#define UCPD_FLT_Pin GPIO_PIN_14
#define UCPD_FLT_GPIO_Port GPIOB
#define SPI_NSS_Pin GPIO_PIN_9
#define SPI_NSS_GPIO_Port GPIOC
#define UCPD_DBn_Pin GPIO_PIN_5
#define UCPD_DBn_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
