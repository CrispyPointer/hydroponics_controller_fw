/**
 * @file define.h
 * @author PL
 * @brief Defines some general values
 * @copyright (c) Copyright Nekoco 2024
 */
#ifndef DEFINE_H
#define DEFINE_H

/* Make static functions available for unit testing with the Ceedling testing framework */
#ifdef TEST
#define STATIC
#define CONST
#else
#define STATIC static
#define CONST  const
#endif /* TEST */

#define UART_USE_TRANSMIT_DMA
#define UART_USE_RECEIVE_DMA
#define STM32U5XX_MCU
#define USE_FREERTOS
#define DISABLE_TEMPERATURE
#define DISABLE_LED
#define DISABLE_LOGGING

/**
 * @def CONSOLE_TX_DMA_BUF_LEN
 * @brief, buffer size used for console DMA transmission.
 * To optimize RAM usage and data rate, size should be close number of data can be sent in 1 BMS cycle, but less.
 *
 * Task cycle is assumed at 50ms, baudrate is 115200bps. Therefore, the size should be less than 576.
 */
#define CONSOLE_TX_DMA_BUF_LEN 550u

#endif /* DEFINE_H */
