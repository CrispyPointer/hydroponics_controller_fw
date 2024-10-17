/**
 * @file bootloader.h
 * @author PL
 * @brief Jumps to bootloader after reset and before watchdog is started
 * @copyright (c) Copyright Nekoco 2016
 * @ingroup bootloader
 *
 * \addtogroup bootloader
 * @{
 */
#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdint.h>

#include "define.h"

/***************************************************
 * @brief set the bootloader flag and reset CPU, this flag will be serviced by bootloader_jump
 ****************************************************/
void bootloader_start(void);

/***************************************************
 * @brief read the bootloader flag, if set, jump to STM32 bootloader
 ***************************************************/
void bootloader_jump(void);

#ifdef ENABLE_CAN_BOOTLOADER
/***************************************************
 * @brief set the canbus bootloader(CBL) flag and reset CPU, this flag will be serviced by CBL
 * @param baudrate Baudrate used for CBL
 * @param termination termination resistor setting for CBL
 ***************************************************/
void canbootloader_start(uint32_t baudrate, uint32_t termination);
#endif

void SysMemBootJump(uint32_t bootloader_stack, uint32_t start_address); /*lint !e526 !e2701 defined in startup_stm32xxx.s */

#endif /* BOOTLOADER_H */
/** @}*/
