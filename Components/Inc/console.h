/**
 * @file console.h
 * @author PL
 * @brief Public function prototypes, defines and data structures for console i/o module
 * @copyright (c) Copyright Nekoco 2024
 * @ingroup COMMAND
 *
 * This module is split out from command module.
 */
#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdbool.h>
#include <stdint.h>

#include "stm32_hal.h"
#include "uart.h"

#define CONSOLE_RX_BUF_LEN 320u  /* Length of UART receive buffer */
#define CONSOLE_TX_BUF_LEN 1024u /* Length of UART transmit buffer */
#define CONSOLE_TIMEOUT    10u   /* maximum time to send a string in ms */

/***************************************************
 * @brief Initialize console module
 * @param huart Pointer to uart handler
 ***************************************************/
void console_init(UART_HandleTypeDef* huart);

/***************************************************
 * @brief De-initialize console module
 ***************************************************/
void console_deinit(void);

/***************************************************
 * @brief Re-initialize console module
 ***************************************************/
void console_reinit(void);

/***************************************************
 * @brief Enable/disable echo-back delay
 * @param enable true:delay echo until console receive a complete line, false: echo received character
 ***************************************************/
void console_echo_delay(bool enable);

/***************************************************
 * @brief Enable/disable blocking printf feature
 * @param enable true: enable blocking printf feature(default)
 ***************************************************/
void console_enable_blocking_printf(bool enable);

/***************************************************
 * @brief Enable/disable silent printf feature
 * @param enable true: enable silent mode, false(default): normal mode
 ***************************************************/
void console_enable_silent_printf(bool enable);

/***************************************************
 * @brief Read line (CR+LF) in console
 * @param buf pointer of buffer to store the received line
 * @param max_len maximum length of the buffer
 ***************************************************/
bool console_read_line(char* buf, uint16_t max_len);

/***************************************************
 * @brief Print buffered data until timeout.
 * @param timeout maximum time can be used for background print(ms)
 * @return true if it printed data, otherwise false.
 ***************************************************/
bool console_background_print(uint32_t timeout);

/***************************************************
 * @brief Disable console for a time (nothing will be printed to console in console_background_print())
 * @param disable_time Amount of time console is disabled [ms]
 ***************************************************/
void console_disable(uint32_t disable_time);

/***************************************************
 * @brief Get timer value when console is active in a BMS cycle
 * The timer value is updated when console module received more than a character or,
 * still have buffered data to be send in a BMS cycle.
 * @return timestamp when console is active
 ***************************************************/
uint32_t console_get_active_timer(void);

/***************************************************
 * @brief Get length of free space in printf buffer
 * @return length of free space(byte)
 ***************************************************/
uint16_t console_get_print_buffer_space(void);

/***************************************************
 * @brief Get length of free space in rx buffer
 * @return length of free space(byte)
 ***************************************************/
uint16_t console_get_rx_buffer_space(void);

/**
 * @brief Pre-process before diagnostics
 * If DMA is used for console, pause DMA required before running RAM test
 */
void console_diag_pre_process(void);

/**
 * @brief Post-process after diagnostics
 * If DMA is used for console, resume DMA required after running RAM test
 */
void console_diag_post_process(void);

#if !defined(NDEBUG) && !defined(TEST)

/**
 * This function is specially made for __aeabi_assert(). It is necessary to
 * display a message as soon as possible, because when an assert() is
 * triggered a system reboot will follow.
 * This function tries to display the debugging information to the UART
 * console as soon as possible.
 *
 * @param tx Reference to the debug information message
 * @param length Length of this message
 * @param timeout Timeout value
 */
void uart_transmit_assert(const char* tx, int32_t length, uint32_t timeout) __attribute__((__nonnull__(1)));

#endif /* _DEBUG && !TEST */

#if defined(TEST)

extern uint8_t rx_data[4];

void putchar_(char character);
void console_rx_cplt_callback(void);
void console_error_callback(void);

#endif /* TEST */

#endif /* CONSOLE_H */
