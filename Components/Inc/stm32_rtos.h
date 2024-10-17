/**
 * @file stm32_rtos.h
 * @author PL
 * @brief Selects proper RTOS drivers
 * @copyright (c) Copyright Nekoco 2024
 *
 * This file includes the proper RTOS drivers based on defines. If no define is set, FREERTOS is assumed, because that was the first.
 *
 */

#ifndef STM32_RTOS_H
#define STM32_RTOS_H

#include "define.h"

#if (defined(USE_FREERTOS))
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#else
#include "FreeRTOS.h" /* Default FREERTOS */
#include "FreeRTOSConfig.h"
#include "task.h"
#endif

#endif /*STM32_RTOS_H*/
