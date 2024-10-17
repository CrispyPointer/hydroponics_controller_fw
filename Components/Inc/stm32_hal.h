/**
 * @file stm32_hal.h
 * @author PL
 * @brief Selects proper HAL drivers
 * @copyright (c) Copyright Nekoco 2024
 *
 * This file includes the proper HAL drivers based on defines. If no define is set, STM32U5XX is assumed, because that was the first.
 *
 */

#ifndef STM32_HAL_H
#define STM32_HAL_H

#include "define.h"

#if (defined(STM32U5XX_MCU))
#include "stm32u5xx_hal.h"
#else
#include "stm32u5xx_hal.h" /* Default HAL drivers */
#define STM32U5XX_MCU      /* STM32U5XX_MCU assumed from now on */
#endif

#endif /*STM32_HAL_H*/
