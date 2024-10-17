/**
 * @file rtc.c
 * @author PL
 * @brief Functions for interfacing with the RTC
 * @copyright (c) Copyright Nekoco 2024
 * @ingroup RTC
 *
 */

#include "rtc.h"

#include <stdbool.h>
#include <stdio.h>

#include "console.h"
#include "stm32_hal.h"

/* variables --------------------------------------------------*/
STATIC RTC_HandleTypeDef* rtc_hand = NULL;
STATIC bool rtc_wu_flag;
STATIC uint32_t wakeup_seconds;

/***************************************************
 * @brief Clear WUCKSEL register
 * Clear WUCKSEL to 0b000
 ****************************************************/
STATIC void rtc_clear_wucksel(void)
{
    __HAL_RTC_WRITEPROTECTION_DISABLE(rtc_hand);
    rtc_hand->Instance->CR &= (uint32_t)~RTC_CR_WUCKSEL;
    __HAL_RTC_WRITEPROTECTION_ENABLE(rtc_hand);
}

/***************************************************
 * @brief Deactivate wakeup timer
 ****************************************************/
STATIC void rtc_deactivate_wakeup_timer(void)
{
    (void)HAL_RTCEx_DeactivateWakeUpTimer(rtc_hand);
    rtc_clear_wucksel();
}

void rtc_init(RTC_HandleTypeDef* rtc)
{
    rtc_hand = rtc; /* remember the RTC handler */
    wakeup_seconds = 0u;
    rtc_deactivate_wakeup_timer(); /* we are awake, so stop wake-up calls from ...... */
}

void rtc_read(RTC_TimeTypeDef* sTime, RTC_DateTypeDef* sDate)
{
    (void)HAL_RTC_GetTime(rtc_hand, sTime, RTC_FORMAT_BIN);
    (void)HAL_RTC_GetDate(rtc_hand, sDate, RTC_FORMAT_BIN);
}

void rtc_write(RTC_TimeTypeDef* sTime, RTC_DateTypeDef* sDate)
{
    (void)HAL_RTC_SetTime(rtc_hand, sTime, RTC_FORMAT_BIN);
    (void)HAL_RTC_SetDate(rtc_hand, sDate, RTC_FORMAT_BIN);
}

uint32_t rtc_read_with_index(uint32_t index)
{
    uint32_t ret;
    RTC_DateTypeDef rtc_date;
    RTC_TimeTypeDef rtc_time;

    rtc_read(&rtc_time, &rtc_date);

    if (index == RTC_I_YEAR)
    {
        ret = (uint32_t)rtc_date.Year + 2000u;
    }
    else if (index == RTC_I_MONTH)
    {
        ret = (uint32_t)rtc_date.Month;
    }
    else if (index == RTC_I_DATE)
    {
        ret = (uint32_t)rtc_date.Date;
    }
    else if (index == RTC_I_HOURS)
    {
        ret = (uint32_t)rtc_time.Hours;
    }
    else if (index == RTC_I_MINUTES)
    {
        ret = (uint32_t)rtc_time.Minutes;
    }
    else if (index == RTC_I_SECONDS)
    {
        ret = (uint32_t)rtc_time.Seconds;
    }
    else
    {
        ret = 0u;
    }

    return ret;
}

bool rtc_validate_and_correct(RTC_TimeTypeDef* sTime, RTC_DateTypeDef* sDate)
{
    bool success = true;
    if ((sDate->Year < 16u) || (sDate->Year > 99u) ||  /* 2016..2099 */
        (sDate->Month < 1u) || (sDate->Month > 12u) || /*      1..12 */
        (sDate->Date < 1u) || (sDate->Date > 31u) ||   /*      1..31 */
        (sTime->Hours > 23u) ||                        /*      0..23 */
        (sTime->Minutes > 59u) ||                      /*      0..59 */
        (sTime->Seconds > 59u))                        /*      0..59 */
    {
        success = false;
    }
    if (!success)
    {
        sDate->Year = 0u; /* Reset to default value 2000-1-1-00:00:00 */
        sDate->Month = 1u;
        sDate->Date = 1u;
        sTime->Hours = 0u;
        sTime->Minutes = 0u;
        sTime->Seconds = 0u;
    }

    /* Set default values for some parameters */
    sDate->WeekDay = RTC_WEEKDAY_TUESDAY;
    sTime->SecondFraction = 0u;
    sTime->DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime->StoreOperation = RTC_STOREOPERATION_RESET;

    return success;
}

void rtc_write_with_index(uint32_t index, uint32_t value)
{
    bool no_changes = false;

    RTC_TimeTypeDef rtc_time;
    RTC_DateTypeDef rtc_date;
    rtc_read(&rtc_time, &rtc_date);

    if (index == RTC_I_YEAR)
    {
        rtc_date.Year = (uint8_t)(value - 2000u);
    }
    else if (index == RTC_I_MONTH)
    {
        rtc_date.Month = (uint8_t)value;
    }
    else if (index == RTC_I_DATE)
    {
        rtc_date.Date = (uint8_t)value;
    }
    else if (index == RTC_I_HOURS)
    {
        rtc_time.Hours = (uint8_t)value;
    }
    else if (index == RTC_I_MINUTES)
    {
        rtc_time.Minutes = (uint8_t)value;
    }
    else if (index == RTC_I_SECONDS)
    {
        rtc_time.Seconds = (uint8_t)value;
    }
    else
    {
        no_changes = true;
    }

    if (!no_changes)
    {
        (void)rtc_validate_and_correct(&rtc_time, &rtc_date);

        rtc_write(&rtc_time, &rtc_date);
    }
}

uint32_t rtc_get_datatime_n_entries(void)
{
    return RTC_I_SECONDS + 1u;
}

void rtc_set_wd_flag(uint32_t val)
{
    const uint32_t wd_flag = ((uint32_t)WD_FLAG) << 24u;
    HAL_RTCEx_BKUPWrite(rtc_hand, RTC_WD_REG, wd_flag | (val & 0x00FFFFFFu));
}

bool rtc_get_wd_flag(uint32_t* pc)
{
    const uint32_t ret = HAL_RTCEx_BKUPRead(rtc_hand, RTC_WD_REG);
    const uint8_t flag = (uint8_t)(ret >> 24u);
    if (flag == WD_FLAG)
    {
        HAL_RTCEx_BKUPWrite(rtc_hand, RTC_WD_REG, 0x0u);
        const uint32_t val = ret & 0x00FFFFFFu;
        *pc = val; /* return the program counter where it did go wrong */
        return true;
    }
    return false;
}

void rtc_set_loader_flag(void)
{
    HAL_RTCEx_BKUPWrite(rtc_hand, RTC_LD_REG, LD_FLAG);
}

bool rtc_get_loader_flag(void)
{
    /* Before initialize HAL RTC module, same functionality as HAL_RTCEx_BKUPPRead()*/
#ifdef RTC_CLK_ENABLE
    __HAL_RCC_RTCAPB_CLK_ENABLE();
#endif
    volatile uint32_t* regptr = &((volatile uint32_t*)BKP_REG_PERIPHERAL)[BKP_REG_UINT32_OFFSET + RTC_LD_REG]; /*lint !e9078 conversion between object pointer type */
    const uint32_t ret = *regptr;

    if (ret == LD_FLAG)
    {
        /* Before initialize HAL RTC module, same functionality as HAL_RTCEx_BKUPPWrite()*/
        __HAL_RCC_PWR_CLK_ENABLE(); /* PWR clock enable */
        HAL_PWR_EnableBkUpAccess(); /* Disable Backup Domain write protection */
        *regptr = 0;
        return true;
    }

    return false;
}

void rtc_set_cmd_setup_flag(void)
{
    HAL_RTCEx_BKUPWrite(rtc_hand, RTC_CMD_SETUP_REG, CMD_SETUP_FLAG);
}

bool rtc_get_cmd_setup_flag(void)
{
    uint32_t ret;
    ret = HAL_RTCEx_BKUPRead(rtc_hand, RTC_CMD_SETUP_REG);
    if (ret == CMD_SETUP_FLAG)
    {
        HAL_RTCEx_BKUPWrite(rtc_hand, RTC_CMD_SETUP_REG, 0x00u);
        return true;
    }
    return false;
}

bool rtc_set_general_flag(uint32_t flag)
{
    if (HAL_RTCEx_BKUPRead(rtc_hand, RTC_GF_REG) == 0u)
    {
        HAL_RTCEx_BKUPWrite(rtc_hand, RTC_GF_REG, flag);
        return true;
    }
    return false;
}

bool rtc_check_general_flag(uint32_t flag)
{
    uint32_t ret;
    ret = HAL_RTCEx_BKUPRead(rtc_hand, RTC_GF_REG);
    if (ret == flag)
    {
        rtc_clear_general_flag();
        /* console was set silent mode to stop printing normal wakeup-message in special reset (reset with any general flag) */
        console_enable_silent_printf(false); /* disable silent mode */
        return true;
    }
    return false;
}

bool rtc_check_any_general_flag(void)
{
    uint32_t flag;
    flag = HAL_RTCEx_BKUPRead(rtc_hand, RTC_GF_REG);
    if (flag == 0u)
    {
        return false;
    }
    return true;
}

void rtc_clear_general_flag(void)
{
    HAL_RTCEx_BKUPWrite(rtc_hand, RTC_GF_REG, 0x00000000);
}

void rtc_set_wakeup(uint32_t seconds)
{
    uint32_t wakeup_clock;
    uint32_t sec;

    rtc_wu_flag = false;
    wakeup_seconds = 0;
    if (seconds <= 0xFFFFu)
    {
        sec = seconds;
        wakeup_clock = RTC_WAKEUPCLOCK_CK_SPRE_16BITS;
    }
    else
    {
        wakeup_clock = RTC_WAKEUPCLOCK_CK_SPRE_17BITS;
        if (seconds <= 0x1FFFFu)
        {
            sec = seconds & 0xFFFFu;
        }
        else
        {
            wakeup_seconds = seconds - 0x1FFFFu;
            sec = 0xFFFFu;
        }
    }
#if defined(STM32U5XX_MCU)
    if (HAL_RTCEx_SetWakeUpTimer_IT(rtc_hand, sec, wakeup_clock, 0u) != HAL_OK)
    {
        printf("Can not set the Wakeup time\r\n");
    }
#endif
}

bool rtc_check_wakeup(void)
{
    rtc_deactivate_wakeup_timer(); /* we are awake, so stop further wake-up calls */
    if (!rtc_wu_flag)
    {
        return false; /* no wakeup from the RTC */
    }
    if (wakeup_seconds != 0u)
    {
        rtc_wu_flag = false;
        rtc_set_wakeup(wakeup_seconds);
        return false; /* restart wakeup counter for next time slice */
    }

    rtc_wu_flag = false;
    return true; /* this was a real RTC wakeup for the BMS */
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef* hrtc) /*lint !e818 could be pointer to const [MISRA 2012 Rule 8.13, advisory] */
{
    (void)hrtc;
    rtc_wu_flag = true; /* remember we woke up from an RTC timer */
}

uint32_t rtc_get_hours(void)
{
    static const uint32_t tb_days_year[12] = {0u, 31u, 59u, 90u, 120u, 151u, 181u, 212u, 243u, 273u, 304u, 334u};
    static const uint32_t tb_days_leap_year[12] = {0u, 31u, 60u, 91u, 121u, 152u, 182u, 213u, 244u, 274u, 305u, 335u};
    RTC_TimeTypeDef s_time;
    RTC_DateTypeDef s_date;

    rtc_read(&s_time, &s_date);

    uint32_t days = ((uint32_t)s_date.Year * 365u) + ((uint32_t)s_date.Year / 4u);
    if (((uint32_t)s_date.Year % 4u) == 0u)
    {
        days += tb_days_leap_year[(uint32_t)s_date.Month - 1u] + (uint32_t)s_date.Date - 1u;
    }
    else
    {
        days += 1u; /* add a day for prev leap year */
        days += tb_days_year[(uint32_t)s_date.Month - 1u] + (uint32_t)s_date.Date - 1u;
    }

    const uint32_t hours = (days * 24u) + (uint32_t)s_time.Hours;
    return hours;
}

uint32_t rtc_get_seconds(void)
{
    RTC_TimeTypeDef s_time;
    RTC_DateTypeDef s_date;
    rtc_read(&s_time, &s_date);

    uint32_t hours = rtc_get_hours();
    return (hours * 60u * 60u) + ((uint32_t)s_time.Minutes * 60u) + (uint32_t)s_time.Seconds;
}

void rtc_set_serial_num(uint32_t serial_num)
{
    HAL_RTCEx_BKUPWrite(rtc_hand, RTC_SERIAL_REG, serial_num);
}

uint32_t rtc_get_serial_num(void)
{
    return HAL_RTCEx_BKUPRead(rtc_hand, RTC_SERIAL_REG);
}

uint32_t rtc_get_rst_flags(void)
{
    uint32_t ret = HAL_RTCEx_BKUPRead(rtc_hand, RTC_RST_REG);
    MCU_RESET_CAUSE rst_cause;

    printf("RCC->CSR: 0x%08lX\r\n", ret);

    if ((ret & (1UL << ((RCC_FLAG_OBLRST)&RCC_FLAG_MASK))) != 0u) //lint !e9027 !e9053 !e504
    {
        rst_cause = OBL_RST;
    }
    else if ((ret & (1UL << ((RCC_FLAG_LPWRRST)&RCC_FLAG_MASK))) != 0u) //lint !e9027 !e9053 !e504
    {
        rst_cause = LPW_RST;
    }
    else if ((ret & (1UL << ((RCC_FLAG_WWDGRST)&RCC_FLAG_MASK))) != 0u) //lint !e9027 !e9053 !e504
    {
        rst_cause = WWDG_RST;
    }
    else if ((ret & (1UL << ((RCC_FLAG_IWDGRST)&RCC_FLAG_MASK))) != 0u) //lint !e9027 !e9053 !e504
    {
        rst_cause = IWDG_RST;
    }
    else if ((ret & (1UL << ((RCC_FLAG_SFTRST)&RCC_FLAG_MASK))) != 0u) //lint !e9027 !e9053 !e504
    {
        rst_cause = SFW_RST;
    }
#ifdef STM32F0XX_MCU
    else if ((ret & (1UL << ((RCC_FLAG_PORRST)&RCC_FLAG_MASK))) != 0u) //lint !e9027 !e9053 !e504
    {
        rst_cause = POR_POD_RST;
    }
#elif (defined(STM32G0XX_MCU))
    /* STM32G0 series has a different name for this reset flag */
    else if ((ret & (1UL << ((RCC_FLAG_PWRRST)&RCC_FLAG_MASK))) != 0u) //lint !e9027 !e9053 !e504
    {
        rst_cause = POR_POD_RST;
    }
#endif                                                                 // DEBUG
    else if ((ret & (1UL << ((RCC_FLAG_PINRST)&RCC_FLAG_MASK))) != 0u) //lint !e9027 !e9053 !e504
    {
        rst_cause = EXT_PIN_RST;
    }
    else
    {
        rst_cause = UNKNOWN_RST;
    }

    return (uint32_t)rst_cause;
}
