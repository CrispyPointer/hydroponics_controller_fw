/**************************************************
 * @file bootloader.c
 * @date 28-6-2017
 * @author JWA
 * @brief Jumps to bootloader after reset and before watchdog is started
 * @copyright (c) Copyright Cleantron 2016
 * @ingroup bootloader
 *
 ****************************************************/
#include "bootloader.h"

#include <stddef.h>

#include "nvic.h"
#include "rtc.h"
#include "stm32_hal.h"

#if defined(STM32U5XX_MCU)
#define START_ADDRESS 0x0BF90004u /* System memory start from  0x0BF90000. Application starts from +4 bytes. */
#else
#error
"MCU family is not supported"
#endif

#define BOOTLOADER_STACK (START_ADDRESS - 4u) /* First 4 bytes are the MSP. */

void bootloader_jump(void)
{
    if (rtc_get_loader_flag())
    {
        /* Prepare to jump to bootloader */
        (void)HAL_RCC_DeInit(); /* Resets the RCC clock configuration to the default reset state. */
        SysTick->CTRL = 0u;     /* reset the systick timer */
        SysTick->LOAD = 0u;
        SysTick->VAL = 0u;

        __set_PRIMASK(1u); /* disable interrupts */

        /* set main stack pointer to bootloader's default
         * and to jump to bootloader */
        SysMemBootJump(BOOTLOADER_STACK, START_ADDRESS);
    }
}

void bootloader_start(void)
{
    rtc_set_loader_flag(); /* remember we go to bootloader after reset */

    nvic_safe_system_reset(); /* reset the CPU so we start without watchdog */
}
