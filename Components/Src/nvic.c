/**
 * @file nvic.c
 * @date 22-03-2021
 * @author YM
 * @brief Implements functionality for interfacing with the cortex NVIC
 * @copyright (c) Copyright Cleantron 2021
 * @ingroup NVIC
 *
 */
#include <stdbool.h>
#include "stm32_hal.h"
#include "nvic.h"

/**************************************************
 * @brief IRQs, having this number or higher, can be disabled
 ****************************************************/
#define NVIC_MIN_IRQ_NUM 2 /*WWDG and PVD stay enable*/

uint32_t nvic_disable_IRQs(void)
{
    uint32_t prev_value;
    prev_value = NVIC->ICER[0u];
    NVIC->ICER[0u] = (uint32_t)(0xFFFFFFFFUL << (((uint32_t)NVIC_MIN_IRQ_NUM) & 0x1FUL));
    return prev_value;
}

void nvic_enable_IRQs(uint32_t IRQ_value)
{
    NVIC->ISER[0U] = IRQ_value;
}

void nvic_safe_system_reset(void)
{
    // #ifndef NVIC_RESET_SKIP_BMS_STATE_WAIT
    //     bms_force_safe_state();
    // #endif

    // #ifndef NVIC_RESET_SKIP_FLASH_WAIT
    //     if (flash_is_init())
    //     {
    //         flash_wait_ready();
    //     }
    // #endif
    //     HAL_NVIC_SystemReset();
}
