/**
 * @file commands.h
 * @author PL
 * @brief Public header for Command interpreter for commands received via the UART port
 * @copyright (c) Copyright Nekoco 2024
 * @ingroup COMMAND
 *
 * \addtogroup COMMAND
 * @{
 ***/

#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32_hal.h"

/**
 * @brief Initialize command module
 */
void command_init(void);

/**
 * @brief Execute received command
 * execute command if any command is received via console.
 */
void command_execute(void);

/**
 * @brief De-initialize low layer modules.
 */
void command_deinit(void);

/**
 * @brief Re-initialize low layer modules and initialize internal flags
 */
void command_reinit(void);

/**
 * @brief process for command UI
 * This function calls
 * - console_background_print()
 * - log_background_print()
 * - command_execute();
 */
void commands_proc(void);

/**
 * @brief Go setup mode by setting a frag to RTC and reset BMS
 * @param reset true to reset BMS immediately
 */
void commands_go_setup_mode(bool reset);

/**
 * @brief Check RTC backup resistor and put BMS in setup mode if the flag is set.
 */
void commands_check_setup_mode(void);

#endif /*COMMANDS_H */
/** @}*/
