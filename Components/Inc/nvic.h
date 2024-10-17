/**************************************************
 * @file nvic.h
 * @date 22-03-2021
 * @author YM
 * @brief Public function prototypes, defines and data structures for interfacing with the NVIC
 * @copyright (c) Copyright Cleantron 2021
 * @ingroup NVIC
 *
 * \addtogroup NVIC
 * @{
 ****************************************************/
#ifndef NVIC_H
#define NVIC_H

#include "stm32_hal.h"
#include <stdint.h>

/**************************************************
 * @brief Disable IRQs.
 * @return Previous value which set to NVIC IRQ.
 ****************************************************/
uint32_t nvic_disable_IRQs(void);

/**************************************************
 * @brief Enable IRQs
 * @param IRQ_value Value to be set to NVIC->ISER
 ****************************************************/
void nvic_enable_IRQs(uint32_t IRQ_value);

/**************************************************
 * @brief Delay software reset duirng any other process is running
 *
 * Delay the software reset to finish logging.
 * Functions listed below don't call nvic_safe_system_reset for timing/safety reason.
 *  - HAL_WWDG_WakeupCallback : wdog.c
 *  - bms_state_bms_error : bms_state_p4l.c
 *  - mcu_hard_fault_handler : mcu.c
 ****************************************************/
void nvic_safe_system_reset(void);

/** @}*/

#endif //NVIC_H
