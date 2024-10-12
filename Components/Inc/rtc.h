/**
 * @file rtc.h
 * @date 29-9-2016
 * @author JWA
 * @brief Public function prototypes and defines for interfacing with the RTC
 * @copyright (c) Copyright Cleantron 2016
 * @ingroup RTC
 *
 * \addtogroup RTC
 * @{
 */
#ifndef RTC_H
#define RTC_H

#include <stdbool.h>

#include "define.h"
#include "stm32_hal.h"

#ifdef CBL
#define WD_FLAG 0x39u /* just another value, to mark this as a WD reset of the CANbus BootLoader */
#else
#define WD_FLAG 0x5Au /* just a value to mark this as a WD reset by the BMS application */
#endif

#if (defined(STM32U5XX_MCU))
#define RTC_CLK_ENABLE                  /* REC APB clock needs to be enabled */
#define BKP_REG_PERIPHERAL    TAMP_BASE /* Backup register stored in TAMP */
#define BKP_REG_UINT32_OFFSET 0x40u     /* TAMP_BASE BKP0R Address offset: 0x100 [byte] */
#else
#error "MCU family not defined"
#endif

/* Flags can be used as general_flags */
#define CBL_FLAG                     0x1Bu /* just an other value, to mark to go to the CANbus bootloader */
#define CBL_PARAMETER_FLAG           0x1Bu /* this value should match CBL_FLAG */
#define CMD_SETUP_FLAG               0xF9u /* just an other value */
#define FLASH_ERASE_FLAG             0x3Cu
#define GILOAD_NO_RESTORE_CALIB_FLAG 0xC6u
#define GILOAD_RESTORE_CALIB_FLAG    0xC9u
#define LD_FLAG                      0xC3u /* just an other value */
#define LOG_PRINT_FLAG               0x6Cu
#define MCU_HARDFAULT_FLAG           0xFFu
#ifdef INT_TEST
#define ITEST_DEBUG_MODE_FLAG 0x4A /**< Activate debug mode upon start-up */
#endif                             /* INT_TEST */

#ifdef ENABLE_CAN_BOOTLOADER
#define CBL_BAUDRATE_REGBIT    16u
#define CBL_BAUDRATE_MASK      0x0000FFFFu
#define CBL_TERMINATION_REGBIT 8u
#define CBL_TERMINATION_MASK   0x0000000Fu
#endif /* ENABLE_CAN_BOOTLOADER */

#define RTC_CBL_PARAM_REG 4u
#define RTC_CMD_SETUP_REG 1u /* this register can't be shared with other flags */
#define RTC_GF_REG        4u
#define RTC_LD_REG        0u
#define RTC_SERIAL_REG    2u /* register used for storing the serial number (in case BMS has no Flash) */
#define RTC_WD_REG        0u
#define RTC_RST_REG       3u /* Backup register to store reset cause */

/**
 * @brief Enumarated type for possible MCU reset cause
 *
 */
typedef enum
{
    UNKNOWN_RST = 0u, /**< Unknown reset cause */
    OBL_RST,          /**< Option byte loader reset */
    LPW_RST,          /**< Low Power reset */
    WWDG_RST,         /**< Window Watchdog reset */
    IWDG_RST,         /**< Independent Watchdog reset*/
    SFW_RST,          /**< Software reset */
    POR_POD_RST,      /**< Power On / Power Down reset */
    EXT_PIN_RST,      /**< External Reset Pin reset */
} MCU_RESET_CAUSE;

/**************************************************
 * @brief Index to specify a member of datetime
 **************************************************/
#define RTC_I_YEAR    0u
#define RTC_I_MONTH   1u
#define RTC_I_DATE    2u
#define RTC_I_HOURS   3u
#define RTC_I_MINUTES 4u
#define RTC_I_SECONDS 5u

/* function prototypes ----------------------------------------*/
/**************************************************
 * @brief initialize RTC
 * @param rtc Pointer to the Handler
 **************************************************/
void rtc_init(RTC_HandleTypeDef* rtc) __attribute__((__nonnull__(1)));

/**************************************************
 * @brief RTC read time and date
 * @param sTime Pointer to a struct for sending back the time
 * @param sDate Pointer to a struct for sending back the date
 **************************************************/
void rtc_read(RTC_TimeTypeDef* sTime, RTC_DateTypeDef* sDate) __attribute__((__nonnull__(1, 2)));

/**************************************************
 * @brief RTC read time and date
 * @param sTime Pointer to a struct of the time
 * @param sDate Pointer to a struct of the date
 **************************************************/
void rtc_write(RTC_TimeTypeDef* sTime, RTC_DateTypeDef* sDate) __attribute__((__nonnull__(1, 2)));

/**************************************************
 * @brief RTC read datetime with index
 * @param index select return data [0]:year, ... [5]:sec
 * @return selected data
 **************************************************/
uint32_t rtc_read_with_index(uint32_t index);

/**
 * Validate time and date values and correct to default in case of failure
 * @param sTime Pointer to a struct of the time
 * @param sDate Pointer to a struct of the date
 * @return True when time and date contains correct values, otherwise false
 */
bool rtc_validate_and_correct(RTC_TimeTypeDef* sTime, RTC_DateTypeDef* sDate) __attribute__((__nonnull__(1, 2)));

/**************************************************
 * @brief RTC write datetime with index
 * @param index select writing data [0]:year, ... [5]:sec
 * @param value value to be written
 **************************************************/
void rtc_write_with_index(uint32_t index, uint32_t value);

/**************************************************
 * @brief RTC get read index size
 * @return index size : 6 (year, month, day, hours, minutes, seconds)
 **************************************************/
uint32_t rtc_get_datatime_n_entries(void);

/***************************************************
 * @brief RTC set Watchdog Flag, so we can check it at next start_up
 * store the reason for the WD as well
 * @param val PC value when it happens
 ***************************************************/
void rtc_set_wd_flag(uint32_t val);

/***************************************************
 * @brief RTC get Watchdog Flag and return true when set
 * clear WD Flag for next time
 * @param pc pointer for sending back pc value when BMS got the reset
 * @return true if if the flag is set, otherwise false
 ***************************************************/
bool rtc_get_wd_flag(uint32_t* pc) __attribute__((__nonnull__(1)));

/***************************************************
 * @brief RTC set bootloader Flag, so we can check it at next start_up
 ***************************************************/
void rtc_set_loader_flag(void);

/***************************************************
 * @brief RTC get bootloader Flag and return true when set
 * clear Flag for next time
 ***************************************************/
bool rtc_get_loader_flag(void);

#ifdef ENABLE_CAN_BOOTLOADER
/***************************************************
 * @brief Set CANbus bootloader flag
 * @param baudrate Baudrate used for CBL
 * @param termination termination resistor setting for CBL
 ****************************************************/
void rtc_set_cbl_flag(uint32_t baudrate, uint32_t termination);

/***************************************************
 * @brief RTC clear CAN bootloader start Flag, so after reset the BMS will be started
 ***************************************************/
void rtc_clr_cbl_flag(void);

/***************************************************
 * @brief RTC check CANbus bootloader flag application Flag and return true when set
 *  also stay in the bootloader, when it was a CANbus bootloader WD flag
 *  do not clear this flag, next time we also want the BMS application to run
 * @return value stored at RTC_CBL_PARAM_REG if CBL_FLAG is set, otherwise 0x00000000u
 ***************************************************/
uint32_t rtc_check_cbl_flag(void);

/***************************************************
 * @brief Print RTC flag
 ***************************************************/
void rtc_print_flag(void);
#endif

/***************************************************
 * @brief RTC set bms config mode flag
 ***************************************************/
void rtc_set_cmd_setup_flag(void);

/***************************************************
 * @brief RTC get bms config mode flag and return true when set
 * clear Flag for next time
 ***************************************************/
bool rtc_get_cmd_setup_flag(void);

/***************************************************
 * @brief set a number on rtc backup register for changing a process after reset
 * This function set a flag for calling flash_erase_chip function in init_flash function currently.
 * This will also be used for gauge ic.
 * @return true if flag is set on the backup register, false if flag is used for other purpose (that should never happen)
 **************************************************/
bool rtc_set_general_flag(uint32_t flag);

/***************************************************
 * @brief check a number on rtc backup register for changing a process after reset
 * This function check the flag is set in back up register.
 * @return true if flag is set (flag will be reset) otherwise false
 **************************************************/
bool rtc_check_general_flag(uint32_t flag);

/***************************************************
 * @brief check if any value is set on backup register
 * @return true if backup register for general flag is not 0x00000000 otherwise false
 **************************************************/
bool rtc_check_any_general_flag(void);

/***************************************************
 * @brief clear backup register for general flag
 ***************************************************/
void rtc_clear_general_flag(void);

/***************************************************
 * @brief Set the wakeup time x seconds from now
 * The Inerrupt version of the wakeuptimer is used, because it is otherwise not working
 * @param seconds specify when BMS should wake up (sec)
 ****************************************************/
void rtc_set_wakeup(uint32_t seconds);

/***************************************************
 * @brief Check if the BMS wokeup with RTC interrupt
 * @return true if the BMS wokeup with RTC interrupt after specified time other wise false
 ****************************************************/
bool rtc_check_wakeup(void);

/***************************************************
 * @brief Return elapsed hours from 01/01/2000 00:00
 * This function only support 2000 ~ 2099
 * @return elapsed hours from 01/01/2000 00:00
 ****************************************************/
uint32_t rtc_get_hours(void);

/***************************************************
 * @brief Return elapsed seconds from 01/01/2000 00:00
 * This function only support 2000 ~ 2099
 * @return elapsed seconds from 01/01/2000 00:00
 ****************************************************/
uint32_t rtc_get_seconds(void);

/***************************************************
 * @brief Store the serial number in an RTC NV register
 * @param serial_num the serial number to be stored
 ****************************************************/
void rtc_set_serial_num(uint32_t serial_num);

/***************************************************
 * @brief Return the serial number stored in the RTC NV register
 * @return the serial number
 ****************************************************/
uint32_t rtc_get_serial_num(void);

/**
 * @brief Return the MCU control & status flags from the RTC 3rd backup register
 * @return the register flags
 */
uint32_t rtc_get_rst_flags(void);

#endif /* RTC_H */
/** @}*/
