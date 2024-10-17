/**************************************************
 * @file uart.c
 * @author PL
 * @brief Implements functionality for interfacing with HAL_UART
 * @copyright (c) Copyright Nekoco 2021
 * @ingroup UART
 *
 * This module is split out from command module.
 ****************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "define.h"
#include "uart.h"

/***************************************************
 * @brief Struct to store a parameter for a uart port
 ***************************************************/
typedef struct
{
    void (*tx_cplt_callback)(void); /**< pointer to tx_cplt_callback */
    void (*rx_cplt_callback)(void); /**< pointer to rx_cplt_callback */
    void (*error_callback)(void);   /**< pointer to error_callback */

    GPIO_TypeDef* gpio_port; /**< gpio port includes the tx pin */
    uint16_t tx_pin;         /**< pin number of the tx pin */
    IRQn_Type IRQn;          /**< IRQ number */
} UART_PARAMS;

/* Private variables --------------------------------------------------*/
#if (defined(STM32U545xx))
static UART_PARAMS uart_1;
static UART_PARAMS uart_3;
static UART_PARAMS uart_4;
static UART_PARAMS uart_5;
#endif

/***************************************************
 * @brief get uart_param for the uart port
 * @warning Currently this function only support UART1.
 * @param huart Pointer to a uart handler
 * @return pointer of UART_PARAMS to parameters for the uart port
 ***************************************************/
static UART_PARAMS* get_uart_params(const UART_HandleTypeDef* huart);

void uart_init(const UART_HandleTypeDef* huart, GPIO_TypeDef* gpio_port, uint16_t tx_pin)
{
    UART_PARAMS* uart_params = get_uart_params(huart);
    uart_params->gpio_port = gpio_port;
    uart_params->tx_pin = tx_pin;

    uart_params->rx_cplt_callback = NULL;
    uart_params->error_callback = NULL;

#if (defined(STM32U545xx))
    if (huart->Instance == USART1)
    {
        uart_params->IRQn = USART1_IRQn;
    }
    else if (huart->Instance == USART3)
    {
        uart_params->IRQn = USART3_IRQn;
    }
    else if (huart->Instance == UART4)
    {
        uart_params->IRQn = UART4_IRQn;
    }
    else if (huart->Instance == UART5)
    {
        uart_params->IRQn = UART5_IRQn;
    }
    else
    {
        uart_params->IRQn = LPUART1_IRQn;
    }
#endif
}

void uart_sleep(UART_HandleTypeDef* huart)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    UART_PARAMS* uart_params = get_uart_params(huart);

#ifdef UART_USE_TRANSMIT_DMA
    /* Non transmitted data will be lost here */
    (void)HAL_UART_DMAStop(huart);
#endif

    HAL_UART_MspDeInit(huart);

    GPIO_InitStruct.Pin = uart_params->tx_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(uart_params->gpio_port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(uart_params->gpio_port, uart_params->tx_pin, GPIO_PIN_RESET); /* pull TxD low, to safe energy in iso tranmitter */
}

void uart_wakeup(UART_HandleTypeDef* huart)
{
    UART_PARAMS* uart_params = get_uart_params(huart);

    HAL_GPIO_DeInit(uart_params->gpio_port, uart_params->tx_pin);
    HAL_UART_MspInit(huart);
}

static UART_PARAMS* get_uart_params(const UART_HandleTypeDef* huart)
{
    UART_PARAMS* ret;

    if (huart->Instance == USART1)
    {
        ret = &uart_1;
    }
#if (defined(STM32U545xx))
    else if (huart->Instance == USART3)
    {
        ret = &uart_3;
    }
    else if (huart->Instance == UART4)
    {
        ret = &uart_4;
    }
    else if (huart->Instance == UART5)
    {
        ret = &uart_5;
    }
#endif /*STM32U545xx */
    else
    {
        ret = NULL;
    }
    return ret;
}

void uart_set_rx_cplt_callback(const UART_HandleTypeDef* huart, void (*callback_func)(void))
{
    UART_PARAMS* uart_params = get_uart_params(huart);
    uart_params->rx_cplt_callback = callback_func;
}

void uart_set_error_callback(const UART_HandleTypeDef* huart, void (*callback_func)(void))
{
    UART_PARAMS* uart_params = get_uart_params(huart);
    uart_params->error_callback = callback_func;
}

HAL_StatusTypeDef uart_transmit(UART_HandleTypeDef* huart, uint8_t* tx, uint16_t length, uint32_t timeout) //lint !e818 could be pointer to const [MISRA 2012 Rule 8.13, advisory]
{
    HAL_StatusTypeDef ret;
    const UART_PARAMS* uart_params = get_uart_params(huart);

    HAL_NVIC_DisableIRQ(uart_params->IRQn);
    ret = HAL_UART_Transmit(huart, tx, length, timeout);
    HAL_NVIC_EnableIRQ(uart_params->IRQn);

    return ret;
}

HAL_StatusTypeDef uart_receive_it(UART_HandleTypeDef* huart, uint8_t* rx, uint16_t length)
{
    HAL_StatusTypeDef ret;
    const UART_PARAMS* uart_params = get_uart_params(huart);

    HAL_NVIC_DisableIRQ(uart_params->IRQn);
    ret = HAL_UART_Receive_IT(huart, rx, length);
    HAL_NVIC_EnableIRQ(uart_params->IRQn);

    return ret;
}

/**
 * @brief HAL error interrupt callback routine
 * @param huart Pointer to uart handler
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart) //lint !e818
{
    const UART_PARAMS* uart_params = get_uart_params(huart);

    if (uart_params->error_callback != NULL)
    {
        uart_params->error_callback();
    }
}

#ifdef UART_USE_TRANSMIT_DMA
HAL_StatusTypeDef uart_transmit_dma(UART_HandleTypeDef* huart, uint8_t* tx, uint16_t length)
{
    HAL_StatusTypeDef ret = HAL_ERROR;
    if (huart->hdmatx != NULL)
    {
        ret = HAL_UART_Transmit_DMA(huart, tx, length);
    }
    return ret;
}

bool uart_is_transmit_dma_busy(const UART_HandleTypeDef* huart)
{
#if (defined(STM32U545xx))
    return HAL_IS_BIT_SET(huart->Instance->CR1, USART_CR1_TCIE);
#else
    return false;
#endif
}

void uart_transmit_dma_pause(UART_HandleTypeDef* huart)
{
    /* Disable UART DMA Tx request */
    CLEAR_BIT(huart->Instance->CR3, USART_CR3_DMAT);
}

void uart_transmit_dma_resume(UART_HandleTypeDef* huart)
{
    if (huart->gState == HAL_UART_STATE_BUSY_TX)
    {
        /* Enable UART DMA Tx request (if there was) */
        SET_BIT(huart->Instance->CR3, USART_CR3_DMAT);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
    const UART_PARAMS* uart_params = get_uart_params(huart);

    if (uart_params->tx_cplt_callback != NULL)
    {
        uart_params->tx_cplt_callback();
    }
}

#endif /* UART_USE_TRANSMIT_DMA */

#ifdef UART_USE_RECEIVE_DMA
HAL_StatusTypeDef uart_receive_to_idle_dma(UART_HandleTypeDef* huart, uint8_t* rx, uint16_t length)
{
    HAL_StatusTypeDef ret;
    const UART_PARAMS* uart_params = get_uart_params(huart);

    HAL_NVIC_DisableIRQ(uart_params->IRQn);
    ret = HAL_UARTEx_ReceiveToIdle_DMA(huart, rx, length);
    HAL_NVIC_EnableIRQ(uart_params->IRQn);

    return ret;
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t Size) //lint !e818
{
    UNUSED(Size);
    const UART_PARAMS* uart_params = get_uart_params(huart);

    if (uart_params->rx_cplt_callback != NULL)
    {
        uart_params->rx_cplt_callback();
    }
}

/**
 * @brief HAL interrupt callback routine for Rx interrupt
 * @param huart Pointer to uart handler
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
    const UART_PARAMS* uart_params = get_uart_params(huart);

    if (uart_params->rx_cplt_callback != NULL)
    {
        uart_params->rx_cplt_callback();
    }
}
#endif /* UART_USE_RECEIVE_DMA */
