/**************************************************
 * @file timer.h
 * @date 07-01-2021
 * @author LVL & JWA
 * @brief Timer functions
 * @copyright (c) Copyright Cleantron 2021
 * @ingroup TIMER
 *
 * \addtogroup TIMER
 * @{
 ****************************************************/
#ifndef TIMER_H
#define TIMER_H

/* Includes ---------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "define.h"
#include "stm32_hal.h"

#define TEMP_STEP        1000u /* Run the temperature reading every x ms */
#define GAUGE_STEP       250u  /* Run the gauge reading evey x ms */
#define TEMP_STEP_START  150u  /* Time offset to start reading */
#define GAUGE_STEP_START 0u    /* Time offset to start reading */

#define MAX_TIM_PERIPHERALS 10u /* Max amount of timer peripherals supported by this module */

/****************************************************************
 * @brief Errors of the timer module
 ***************************************************************/
typedef enum
{
    TIMER_MEM_ERROR = 0x0u, /**< Memory coherency error */
    TIMER_FRQ_ERROR = 0x1u, /**< Memory tick frequency error */
    TIMER_UST_ERROR = 0x2u, /**< Microsecond timer configuration error */
    TIMER_PWM_ERROR = 0x3u, /**< PWM configuration error */
    TIMER_OOR_ERROR = 0x4u, /**< Memory out of range error */
} TIMER_ERROR;

/* Function prototypes ----------------------------------------*/
/***************************************************
 * @brief Initialize timer functions
 ***************************************************/
void timer_init(void);

/***************************************************
 * @brief Set the current tick to the given timer function
 ***************************************************/
void timer_reset_module_timer(uint32_t* module_timer);

/***************************************************
 * @brief Calculate the elapsed time (delta time) using the module_time
 ***************************************************/
uint32_t timer_get_elapsed_module_timer(uint32_t module_timer);

/***************************************************
 * @brief Delay msec
 * @param delay_ms specifies the delay time length, in milliseconds
 ***************************************************/
void timer_delay(uint32_t delay_ms);

#ifdef ENABLE_PWM_US_TIMER_PERIPHERALS
/* PWM timer functions --------------------------------*/
/***************************************************
 * @brief Associate a timer peripheral and channel with a pin on the microcontroller for PWMing
 *
 * Doesn't actually configure anything in the PWM signal, but other PWM functions in this module are called
 * using references to pins instead of peripherals, so this function is needed to make sure those
 * functions know which peripheral to use for that pin.
 *
 * @param htim Handle for timer peripheral
 * @param channel Timer peripheral output channel (TIM_Channel)
 * @param alt_function Alternate function as used in GPIO_InitStruct.Alternate
 * @param port GPIO port to be used
 * @param pin Pin number to be used
 ***************************************************/
void timer_pwm_set_pin(TIM_HandleTypeDef* htim, uint32_t channel, uint8_t alt_function, GPIO_TypeDef* port, uint16_t pin);

/***************************************************
 * @brief Initialize a pin for use as PWM using HAL_GPIO_Init()
 *
 * @param port GPIO port for PWM output
 * @param pin Pin for PWM output
 * @param gpio_speed Either GPIO_SPEED_FREQ_LOW, GPIO_SPEED_FREQ_MEDIUM or GPIO_SPEED_FREQ_HIGH
 ***************************************************/
void timer_pwm_init_pin(GPIO_TypeDef* port, uint16_t pin, uint32_t gpio_speed);

/***************************************************
 * @brief Set duty cycle for an output PWM signal
 *
 * @param port GPIO port for PWM output
 * @param pin Pin for PWM output
 * @param duty_cycle duty cycle of PWN output
 ***************************************************/
void timer_pwm_set_duty_cycle(const GPIO_TypeDef* port, uint16_t pin, uint8_t duty_cycle);

/***************************************************
 * @brief Start PWM signal
 *
 * @param port GPIO port for PWM output
 * @param pin Pin for PWM output
 ***************************************************/
void timer_pwm_start(const GPIO_TypeDef* port, uint16_t pin);

/***************************************************
 * @brief Stop PWM signal
 *
 * @param port GPIO port for PWM output
 * @param pin Pin for PWM output
 ***************************************************/
void timer_pwm_stop(const GPIO_TypeDef* port, uint16_t pin);

/* Microsecond timer functions --------------------------------*/
/***************************************************
 * @brief Set timer peripheral to be used for microsecond timer
 *
 * Peripheral must be configured to 1MHz and ARR 2^16-1.
 *
 * @param handle Timer peripheral handle (HAL)
 ***************************************************/
void timer_set_us_timer_handle(TIM_HandleTypeDef* handle);

/***************************************************
 * @brief Get counter of us timer peripheral
 *
 * Peripheral must be configured to 1MHz and ARR 2^16-1.
 *
 * @return Value in us_timer counter register
 ***************************************************/
uint16_t timer_get_us_timer(void);

/***************************************************
 * @brief Get elapsed us time based on local timer
 *
 * Peripheral must be configured to 1MHz and ARR 2^16-1.
 *
 * @param us_timer Previous value returned by us_timer
 ***************************************************/
uint16_t timer_get_elapsed_us_timer(uint16_t us_timer);

/***************************************************
 * @brief Reset local us_timer
 *
 * Peripheral must be configured to 1MHz and ARR 2^16-1.
 *
 * @param us_timer Pointer to timer to reset
 ***************************************************/
void timer_reset_us_timer(uint16_t* us_timer);

/***************************************************
 * @brief Delay execution for a certain amount of time
 *
 * Peripheral must be configured to 1MHz and ARR 2^16-1.
 *
 * @param delay Delay in microseconds
 ***************************************************/
void timer_us_delay(uint16_t delay);

/***************************************************
 * @brief Enable HW timer for cyclic event
 *
 * Peripheral must be configured to generate interrupt in every 1ms
 *
 * @param htim Pointer to timer handler
 ***************************************************/
void timer_enable_cyclic_event_timer(TIM_HandleTypeDef* htim);

/***************************************************
 * @brief Set callback function for cyclic event timer
 *
 * This callback is called every 1ms except critical section.
 * This shall not be used for safety feature.
 * Callback shall not be called when BMS is in critical section = some calls may be skipped
 *
 * @param callback_func function pointer to the callback function
 * @return true if the callback is registered otherwise false
 ***************************************************/
bool timer_set_event_callback(void (*callback_func)(void));

/**
 * @brief Performs checks on safety related operation of this module
 *
 * @param hspi Handle of the peripheral to verify
 *
 * Call once every PST when TIM is safety-related
 */
bool tim_pst_test(const TIM_HandleTypeDef* htim);

#endif

/* Get functions ----------------------------------------------*/
uint32_t timer_get_uptime(void);
bool timer_get_status(void);
uint32_t timer_get_error_code(void);

#endif // TIMER_H
/** @}*/
