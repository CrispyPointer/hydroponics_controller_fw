/**************************************************
 * @file uart.h
 * @author PL
 * @brief Public function prototypes, defines and data structures for interfacing with the UART
 * @copyright (c) Copyright Nekoco 2021
 * @ingroup UART
 *
 * This module is split out from command module.
 *
 * \addtogroup UART
 * @{
 ****************************************************/
#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stdint.h>
#include "define.h"
#include "stm32_hal.h"

/***************************************************
 * @brief Initialize uart module.
 * @param huart Pointer to an UART handler
 * @param gpio_port Pointer to a gpio port which include tx_pin
 * @param tx_pin Pin number of the tx_pin
 ***************************************************/
void uart_init(const UART_HandleTypeDef* huart, GPIO_TypeDef* gpio_port, uint16_t tx_pin);

/***************************************************
 * @brief Set a callback when a data received
 * @param huart Pointer to an UART handler
 * @param callback_func Pointer to the function
 ***************************************************/
void uart_set_rx_cplt_callback(const UART_HandleTypeDef* huart, void (*callback_func)(void));

/***************************************************
 * @brief Set a callback when an error happens
 * @param huart Pointer to an UART handler
 * @param callback_func Pointer to the function
 ***************************************************/
void uart_set_error_callback(const UART_HandleTypeDef* huart, void (*callback_func)(void));

/***************************************************
 * @brief De-initialize uart module and set TX pin low.
 * @param huart Pointer to an UART handler
 ***************************************************/
void uart_sleep(UART_HandleTypeDef* huart);

/***************************************************
 * @brief Re-initialize uart module
 * @param huart Pointer to an UART handler
 ***************************************************/
void uart_wakeup(UART_HandleTypeDef* huart);

/***************************************************
 * @brief Transmit data
 * This is an blocking function. This function lock huart until it transmits all data or timeout.
 * USART_X_IRQn is also disabled to block rx interrupt.
 * @param huart Pointer to an UART handler
 * @param tx Pointer to transmit data
 * @param length length of the data.
 * @param timeout timeout
 * @return Status
 ***************************************************/
HAL_StatusTypeDef uart_transmit(UART_HandleTypeDef* huart, uint8_t* tx, uint16_t length, uint32_t timeout);

#ifdef UART_USE_TRANSMIT_DMA
/**
 * @brief Transmit data with DMA
 *
 * @param huart Pointer to an UART handler
 * @param tx Pointer to transmitting data
 * @param length Length of data
 * @return HAL_StatusTypeDef HAL_OK if no problem, otherwise false.
 */
HAL_StatusTypeDef uart_transmit_dma(UART_HandleTypeDef* huart, uint8_t* tx, uint16_t length);

/**
 * @brief Return DMA transmission status
 *
 * @param huart Pointer to an UART handler
 * @return true if transmission is ongoing, otherwise false
 */
bool uart_is_transmit_dma_busy(const UART_HandleTypeDef* huart);

/**
 * @brief Pause DMA UART transmission
 * @note This function doesn't call HAL_UART_DMAPause() because it doesn't work if RX uses interrupt
 *
 * @param huart Pointer to an UART handler
 */
void uart_transmit_dma_pause(UART_HandleTypeDef* huart);

/**
 * @brief Resume DMA UART transmission
 * @note This function doesn't call HAL_UART_DMAResume() because it doesn't work if RX uses interrupt
 *
 * @param huart Pointer to an UART handler
 */
void uart_transmit_dma_resume(UART_HandleTypeDef* huart);

#endif /* UART_USE_TRANSMIT_DMA */

#ifdef UART_USE_RECEIVE_DMA
/**
 * @brief Receive data though DMA with interription when an idle line (1 byte frame) occurs of max data received
 *
 * @param huart Pointer to an UART handler
 * @param rx Pointer to a receive buffer
 * @param length length of the data.
 * @return Status
 */
HAL_StatusTypeDef uart_receive_to_idle_dma(UART_HandleTypeDef* huart, uint8_t* rx, uint16_t length);
#endif /* UART_USE_RECEIVE_DMA */

/***************************************************
 * @brief Receive data with interruption
 * rx_clipt_callback can be called when it received data.
 * @param huart Pointer to an UART handler
 * @param rx Pointer to a receive buffer
 * @param length length of the data.
 * @return Status
 ***************************************************/
HAL_StatusTypeDef uart_receive_it(UART_HandleTypeDef* huart, uint8_t* rx, uint16_t length);

/** @}*/
#endif /* UART_H */
