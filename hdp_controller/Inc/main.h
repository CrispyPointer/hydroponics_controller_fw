/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
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
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32u5xx_hal.h"

    /* Private includes ----------------------------------------------------------*/
    /* USER CODE BEGIN Includes */

    /* USER CODE END Includes */

    /* Exported types ------------------------------------------------------------*/
    /* USER CODE BEGIN ET */

    /* USER CODE END ET */

    /* Exported constants --------------------------------------------------------*/
    /* USER CODE BEGIN EC */

    /* USER CODE END EC */

    /* Exported macro ------------------------------------------------------------*/
    /* USER CODE BEGIN EM */

    /* USER CODE END EM */

    /* Exported functions prototypes ---------------------------------------------*/
    void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define USER_BUTTON_Pin       GPIO_PIN_13
#define USER_BUTTON_GPIO_Port GPIOC
#define LED_GREEN_Pin         GPIO_PIN_5
#define LED_GREEN_GPIO_Port   GPIOA

    /* USER CODE BEGIN Private defines */
    extern ADC_HandleTypeDef hadc1;
    extern ADC_HandleTypeDef hadc4;

    extern CRYP_HandleTypeDef hcryp;
    extern uint32_t pKeyAES[8];

    extern CRC_HandleTypeDef hcrc;

    extern HASH_HandleTypeDef hhash;

    extern I2C_HandleTypeDef hi2c1;

    extern RTC_HandleTypeDef hrtc;

    extern SPI_HandleTypeDef hspi1;
    extern DMA_HandleTypeDef handle_GPDMA1_Channel3;
    extern DMA_HandleTypeDef handle_GPDMA1_Channel2;

    extern TIM_HandleTypeDef htim2;
    extern TIM_HandleTypeDef htim3;

    extern UART_HandleTypeDef huart1;
    extern DMA_HandleTypeDef handle_GPDMA1_Channel1;
    extern DMA_HandleTypeDef handle_GPDMA1_Channel0;

    extern HCD_HandleTypeDef hhcd_USB_DRD_FS;
    /* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
