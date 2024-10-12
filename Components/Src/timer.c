/**************************************************
 * @file timer.c
 * @date 07-01-2021
 * @author LVL & JWA
 * @brief Timer functions
 * @copyright (c) Copyright Cleantron 2021
 * @ingroup TIMER
 *
 * \addtogroup TIMER
 * @{
 ****************************************************/
/* Includes ---------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "stm32_hal.h"
#include "stm32_rtos.h"
#include "timer.h"
#include "define.h"
#include "crc.h"

#define SECOND 1000u /* 1 second = 1000 ms */

#ifdef ENABLE_PWM_US_TIMER_PERIPHERALS
/* TIM Peripheral data -----------------------------------------------------------*/
/***************************************************
 * @brief PWM configuration
 ***************************************************/
typedef struct
{
    TIM_HandleTypeDef* htim;
    uint32_t channel;
    GPIO_TypeDef* port;
    uint16_t pin;
    uint8_t af;
    uint32_t hal_init_crc;
    uint32_t checksum; /* Checksum 4 bytes instead of 2 to force correct alignment and padding */
} TIMER_PWM_CONFIG;

typedef struct
{
    TIM_HandleTypeDef* htim; /* Handle of timer peripheral responsible for us timer */
    uint32_t hal_init_crc;
    uint32_t checksum; /* Checksum 4 bytes instead of 2 to force correct alignment and padding */
} TIMER_US_CONFIG;

/***************************************************
 * @brief PWM configuration instance
 *
 ***************************************************/
STATIC TIMER_PWM_CONFIG pwm_config_inst[MAX_TIM_PERIPHERALS] = {0};

/**
 * @brief US_TIMER configuration instance
 *
 */
STATIC TIMER_US_CONFIG us_config_inst;

/***************************************************
 * @brief Parameters of timer module
 ***************************************************/
typedef struct
{
    TIMER_US_CONFIG* us_config;            /* Configuration of microsecond timer peripheral */
    TIMER_PWM_CONFIG* pwm_config;          /* Associations between timer peripherals and GPIO pins */
    TIM_HandleTypeDef* event_timer_handle; /* Handle of timer peripheral responsible for event timer */
    void (*event_callback)(void);          /* Callback when event timer elapsed */
} TIMER_PARAMS;

STATIC TIMER_PARAMS timer_params = {&us_config_inst, pwm_config_inst, NULL, NULL};

/***************************************************
 * @brief Finds index in pwm_config array of timer_params struct based on port and pin number
 *
 * @param port GPIO port for PWM
 * @param pin Pin number for PWM
 *
 * @return Found index (MAX_TIM_PERIPHERALS if not found)
 ***************************************************/
STATIC uint32_t timer_params_find_pwm_config_index(const GPIO_TypeDef* port, uint16_t pin);

/**********************************************************************************
 * Static setters for timer_params variables
 **********************************************************************************/
STATIC void timer_params_set_pwm_config_htim(uint32_t i, TIM_HandleTypeDef* handle);
STATIC void timer_params_set_pwm_config_channel(uint32_t i, uint32_t channel);
STATIC void timer_params_set_pwm_config_port(uint32_t i, GPIO_TypeDef* port);
STATIC void timer_params_set_pwm_config_pin(uint32_t i, uint16_t pin);
STATIC void timer_params_set_pwm_config_af(uint32_t i, uint8_t af);

/**********************************************************************************
 * Static getters for timer_params variables
 **********************************************************************************/
STATIC TIM_HandleTypeDef* timer_params_get_us_timer_handle(void);
STATIC TIM_HandleTypeDef* timer_params_get_pwm_config_htim(uint32_t i);
STATIC uint32_t timer_params_get_pwm_config_channel(uint32_t i);
STATIC GPIO_TypeDef* timer_params_get_pwm_config_port(uint32_t i);
STATIC uint16_t timer_params_get_pwm_config_pin(uint32_t i);
STATIC uint8_t timer_params_get_pwm_config_af(uint32_t i);
#endif /*ENABLE_PWM_US_TIMER_PERIPHERALS*/

/* General Timer module data -----------------------------------------------------*/
/***************************************************
 * @brief Operational data of the timer
 ***************************************************/
typedef struct
{
    bool status;                     /**< Timer status */
    uint32_t error;                  /**< Timer error */
    uint32_t uptime;                 /**< Timer from the module is initialized */
    uint32_t sec_timer;              /**< Sub timer to measure 1 second */
    uint32_t bms_timer;              /**< Timer to control BMS cycle */
    volatile uint32_t bms_time_flag; /**< True if BMS_STEP time passed */
    volatile uint32_t tick;          /**< Replacement of uwTick */
    uint32_t tick_freq;              /**< Replacement of uwTickFreq */
} TIMER_DATA;

/* timer_data.tick should be initialized before timer_init() */
STATIC TIMER_DATA timer_data = {true, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000001u};

STATIC TIMER_DATA timer_data_red = {false, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xfffffffeu};

/***************************************************
 * @brief Report timer error
 *
 * @param error Type of error
 *************************************** ************/
STATIC void timer_error(TIMER_ERROR error);

/**********************************************************************************
 * Static setters for timer_data variables
 * This setters can be called from HAL_SYSTICK_Callback() or a critical section.
 **********************************************************************************/
STATIC void timer_set_uptime(uint32_t uptime);
STATIC void timer_set_sec_timer(uint32_t sec_timer);
STATIC void timer_set_bms_timer(uint32_t bms_timer);
STATIC void timer_set_bms_time_flag(uint32_t bms_time_flag);
STATIC void timer_set_tick(uint32_t tick);
STATIC void timer_set_tick_freq(uint32_t tick_freq);

/**********************************************************************************
 * Static getters for timer_data variables
 * The getters call __disable_irq() because the timer module variables can be changed in HAL_SYSTICK_Callback() and systick is non-maskable with nvic_disable_IRQs()
 **********************************************************************************/
STATIC uint32_t timer_get_sec_timer(void);
STATIC uint32_t timer_get_tick(void);
STATIC uint32_t timer_get_tick_freq(void);

/***************************************************
 * @brief Called from the Timer IRQ routine every ms
 *
 * This function replaces the weak function in stm32f0xx_hal_cortex.c
 ***************************************************/
void HAL_SYSTICK_Callback(void)
{
    timer_set_sec_timer(timer_get_sec_timer() + 1u);
    if (timer_get_sec_timer() >= SECOND)
    {
        timer_set_uptime(timer_get_uptime() + 1u);
        timer_set_sec_timer(0u);
    }
}

/******************************************************
 * @brief overwrite HAL_IncTick() function
 * ***************************************************/
void HAL_IncTick(void)
{
    const uint32_t tick = timer_get_tick();
    const uint32_t tick_freq = timer_get_tick_freq();
    timer_set_tick(tick + tick_freq);
}

/******************************************************
 * @brief overwrite HAL_GetTick() function
 * ***************************************************/
#ifndef TEST_HAL_DELAY
uint32_t HAL_GetTick(void)
{
    return timer_get_tick();
}
#endif

/******************************************************
 * @brief overwrite HAL_Delay() function
 * @param Delay specifies the delay time length, in milliseconds.
 * ***************************************************/
void HAL_Delay(uint32_t Delay)
{
#if (!defined(TEST)) || (defined(TEST_HAL_DELAY))
    uint32_t tickstart = HAL_GetTick();
    uint32_t wait = Delay;

    uint32_t tickstart_red = tickstart ^ 0xFFFFFFFFU;
    uint32_t wait_red = Delay ^ 0xFFFFFFFFU;

    /* Add a freq to guarantee minimum wait */
    if (wait < HAL_MAX_DELAY)
    {
        wait += (uint32_t)(timer_get_tick_freq());
        wait_red -= (uint32_t)(timer_get_tick_freq());
    }

    while ((HAL_GetTick() - tickstart) < wait)
    {
        /* (Defensive and not unit-tested) */
        if ((tickstart != (tickstart_red ^ 0xFFFFFFFFU)) || (wait != (wait_red ^ 0xFFFFFFFFU)))
        {
            timer_error(TIMER_MEM_ERROR);
        }
    }
#else
    (void)Delay;
#endif
}

void timer_init(void)
{
    uint32_t old_primask;

    old_primask = __get_PRIMASK(); /* systick is already running here. disable interrupt before change the values */
    (void)__disable_irq();
    timer_set_uptime(0u);
    timer_set_sec_timer(0u);
    timer_set_bms_timer(0u);
    timer_set_bms_time_flag(1u); /* Start a loop ASAP */

    /* check uwTickFreq is expected value */
    if ((uint32_t)uwTickFreq != timer_get_tick_freq())
    {
        timer_set_tick_freq((uint32_t)uwTickFreq);
        timer_error(TIMER_FRQ_ERROR);
    }

    __set_PRIMASK(old_primask);

#ifdef ENABLE_PWM_US_TIMER_PERIPHERALS
    timer_set_us_timer_handle(NULL);

    for (uint32_t i = 0u; i < MAX_TIM_PERIPHERALS; i++)
    {
        timer_params_set_pwm_config_htim(i, NULL);
    }
#endif /* ENABLE_PWM_US_TIMER_PERIPHERALS*/
}

void timer_reset_module_timer(uint32_t* module_timer)
{
    *module_timer = timer_get_tick();
}

uint32_t timer_get_elapsed_module_timer(uint32_t module_timer)
{
    return timer_get_tick() - module_timer;
}

void timer_delay(uint32_t delay_ms)
{
    HAL_Delay(delay_ms);
}

STATIC void timer_error(TIMER_ERROR error)
{
    timer_data.status = false;
    timer_data_red.status = !(timer_data.status);

    timer_data.error |= ((uint32_t)0x1u) << ((uint32_t)error);
    timer_data_red.error &= ((((uint32_t)0x1u) << ((uint32_t)error)) ^ 0xFFFFFFFFU);
}

#ifdef ENABLE_PWM_US_TIMER_PERIPHERALS
uint16_t timer_get_elapsed_us_timer(uint16_t us_timer)
{
    uint16_t elapsed_time = 0u;

    if (timer_params_get_us_timer_handle() != NULL)
    {
        elapsed_time = ((uint16_t)__HAL_TIM_GET_COUNTER(timer_params_get_us_timer_handle())) - us_timer; /*lint !e732 loss of sign intentional */
    }
    else
    {
        timer_error(TIMER_UST_ERROR);
    }

    return elapsed_time;
}

void timer_reset_us_timer(uint16_t* us_timer)
{
    if (timer_params_get_us_timer_handle() != NULL)
    {
        *us_timer = (uint16_t)__HAL_TIM_GET_COUNTER(timer_params_get_us_timer_handle());
    }
    else
    {
        timer_error(TIMER_UST_ERROR);
    }
}

uint16_t timer_get_us_timer(void)
{
    uint16_t counter = 0;

    if (timer_params_get_us_timer_handle() != NULL)
    {
        counter = (uint16_t)__HAL_TIM_GET_COUNTER(timer_params_get_us_timer_handle());
    }
    else
    {
        timer_error(TIMER_UST_ERROR);
    }

    return counter;
}

void timer_us_delay(uint16_t delay)
{
    uint16_t elapsed_time;
    uint16_t tickstart = timer_get_us_timer();
    uint16_t wait = delay;

    uint16_t tickstart_red = tickstart ^ 0xFFFFu;
    uint16_t wait_red = delay ^ 0xFFFFu;

    elapsed_time = timer_get_us_timer() - tickstart; /*lint !e732 loss of sign intentional */

    while (elapsed_time < wait)
    {
        /* (Defensive and not unit-tested) */
        if ((tickstart != (tickstart_red ^ 0xFFFFu)) || (wait != (wait_red ^ 0xFFFFu)))
        {
            timer_error(TIMER_MEM_ERROR);
        }

        elapsed_time = timer_get_us_timer() - tickstart; /*lint !e732 loss of sign intentional */
    }
}

void timer_enable_cyclic_event_timer(TIM_HandleTypeDef* htim)
{
    if (HAL_TIM_Base_Start_IT(htim) == HAL_OK)
    {
        timer_params.event_timer_handle = htim;
    }

    return;
}

bool timer_set_event_callback(void (*callback_func)(void))
{
    if (timer_params.event_timer_handle == NULL)
    {
        return false;
    }
    else
    {
        timer_params.event_callback = callback_func;
        return true;
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim) //lint !e818 could be pointer to const [MISRA 2012 Rule 8.13, advisory]
{
    if (htim == timer_params.event_timer_handle)
    {
        if (timer_params.event_callback != NULL)
        {
            timer_params.event_callback();
        }
    }
}
#endif /* ENABLE_PWM_US_TIMER_PERIPHERALS */

uint32_t timer_get_uptime(void)
{
    uint32_t old_primask;
    uint32_t t;

    old_primask = __get_PRIMASK(); /* disable IRQ */
    (void)__disable_irq();
    if (timer_data.uptime != (timer_data_red.uptime ^ 0xFFFFFFFFU))
    {
        timer_error(TIMER_MEM_ERROR);
        t = 0u;
    }
    else
    {
        t = timer_data.uptime;
    }

    __set_PRIMASK(old_primask);

    return t;
}

STATIC uint32_t timer_get_sec_timer(void)
{
    if (timer_data.sec_timer != (timer_data_red.sec_timer ^ 0xFFFFFFFFU))
    {
        timer_error(TIMER_MEM_ERROR);
        return 0u;
    }

    return timer_data.sec_timer;
}

STATIC uint32_t timer_get_tick(void)
{
    uint32_t ret;

    const uint32_t old_primask = __get_PRIMASK(); /* disable IRQ */
    (void)__disable_irq();
    const uint32_t tick = timer_data.tick;
    const uint32_t tick_red = timer_data_red.tick;
    if (tick != (tick_red ^ 0xFFFFFFFFU))
    {
        timer_error(TIMER_MEM_ERROR);
        ret = 0;
    }
    else
    {
        ret = timer_data.tick;
    }

    __set_PRIMASK(old_primask);

    return ret;
}

STATIC uint32_t timer_get_tick_freq(void)
{
    if (timer_data.tick_freq != (timer_data_red.tick_freq ^ 0xFFFFFFFFU))
    {
        timer_error(TIMER_MEM_ERROR);
        return 0u;
    }

    return timer_data.tick_freq;
}

/******************************************************
 * getter and setter functions
 * ***************************************************/
STATIC void timer_set_uptime(uint32_t uptime)
{
    timer_data.uptime = uptime;
    timer_data_red.uptime = uptime ^ 0xFFFFFFFFU;
}

STATIC void timer_set_sec_timer(uint32_t sec_timer)
{
    timer_data.sec_timer = sec_timer;
    timer_data_red.sec_timer = sec_timer ^ 0xFFFFFFFFU;
}

STATIC void timer_set_bms_timer(uint32_t bms_timer)
{
    timer_data.bms_timer = bms_timer;
    timer_data_red.bms_timer = bms_timer ^ 0xFFFFFFFFU;
}

STATIC void timer_set_bms_time_flag(uint32_t bms_time_flag)
{
    timer_data.bms_time_flag = bms_time_flag;
    timer_data_red.bms_time_flag = bms_time_flag ^ 0xFFFFFFFFU;
}

STATIC void timer_set_tick(uint32_t tick)
{
    timer_data.tick = tick;
    timer_data_red.tick = tick ^ 0xFFFFFFFFU;
}

STATIC void timer_set_tick_freq(uint32_t tick_freq)
{
    timer_data.tick_freq = tick_freq;
    timer_data_red.tick_freq = tick_freq ^ 0xFFFFFFFFU;
}

/** @}*/
