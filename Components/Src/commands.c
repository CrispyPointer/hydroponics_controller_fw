/**************************************************
 * @file commands.c
 * @date 14-09-2020
 * @author JWA
 * @brief Command interpreter for commands received via console
 * @copyright (c) Copyright Cleantron 2016
 * @ingroup COMMAND
 *
 ****************************************************/
#include "commands.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "bootloader.h"
#include "property.h"
#include "authentication.h"
#include "console.h"
#include "define.h"
#include "nvic.h"
#include "rtc.h"
#include "stm32_hal.h"
#include "timer.h"

#ifndef DISABLE_TEMPERATURE
#include "temperature.h"
#endif

#ifndef DISABLE_LED
#include "led.h"
#endif

#ifndef DISABLE_LOGGING
#include "logging.h"
#endif

#define COMMAND_SETUP_MODE_MAX   60u   /* command module can keep BMS in initialization mode for X (sec), max */
#define COMMAND_SETUP_MODE_DELAY 2500u /* stay setting_mode for X(ms) after user input */

/* Private function prototypes -------------------------------*/
/* Basic commands BMS should support -------------------------*/
/**************************************************
 * @brief Command: help text
 * @param argc Unused
 * @param argv Unused
 **************************************************/
void cmd_help(int32_t argc, const char* const* argv);
/**************************************************
 * @brief Command: Show firmware version
 * @param argc Unused
 * @param argv Unused
 **************************************************/
void cmd_version(int32_t argc, const char* const* argv);
/**************************************************
 * @brief Command: Clear the terminal
 * @param argc Unused
 * @param argv Unused
 **************************************************/
void cmd_clear(int32_t argc, const char* const* argv);

/**************************************************
and: show the uptime in seconds
 S
 * @param argv Unused
 **************************************************/
void cmd_uptime(int32_t argc, const char* const* argv);
/**************************************************
 * @brief Command: real time clock set and read
 * @param argc argc 7 to write clock otherwise only read
 * @param argv argv[1:6] year, month, date, hour, minutes and seconds
 **************************************************/
void cmd_clock(int32_t argc, const char* const* argv);

/**************************************************
 * @brief Command: show temperature of the sensors
 * @param argc argc 2 for optional print, otherwise classic print
 * @param argv argv[1] number of replay
 **************************************************/
void cmd_temp_status(int32_t argc, const char* const* argv);

/* Basic commands to control BMS -----------------------------*/
/**************************************************
 * @brief Command: password to unlock
 * @param argc argc 1 for send challenge otherwise try password
 * @param argv argv[1] response for challenge or a password
 **************************************************/
void cmd_password(int32_t argc, const char* const* argv);
/**************************************************
 * @brief Command: Reset the processor
 * @param argc argc 1 otherwise print error
 * @param argv Unused
 **************************************************/
void cmd_reset(int32_t argc, const char* const* argv);
/**************************************************
 * @brief Command: Switch BMS off, first shutdown the AFE, then switch off power
 * @param argc argc 1 otherwise print error
 * @param argv Unused
 **************************************************/
void cmd_off(int32_t argc, const char* const* argv);
/**************************************************
 * @brief Command: load new software, jump to bootloader
 * @param argc argc 1 otherwise print error
 * @param argv Unused
 **************************************************/
void cmd_load(int32_t argc, const char* const* argv);

#ifndef DISABLE_LED
/**************************************************
 * @brief Command: set LED's on/off
 * @param argc number of LED+1 otherwise error
 * @param argv argv '0' to off otherwise on
 **************************************************/
void cmd_led(int32_t argc, const char* const* argv);
#endif

#ifndef DISABLE_LOGGING
/**************************************************
 * @brief Command: print log data
 * @param argc argc 2 to specify the number of log, otherwise print latest 20 logs
 * @param argv argv [1] number of log entry
 **************************************************/
void cmd_log(int32_t argc, const char* const* argv);
#endif

/***************************************************
 * @brief Sets replay period for continuous replaying of previous command
 *
 * @param period The time period of the replay. Setting to 0 stops the replay.
 ***************************************************/
STATIC void cmd_set_replay(uint32_t period);
/***************************************************
 * @brief Disable print newline("#\r\n") during the replay
 ***************************************************/
STATIC void prvcmd_disable_newline_in_replay(void);
/***************************************************
 * @brief Reset replay count
 ***************************************************/
STATIC void cmd_reset_replay_count(void);
/***************************************************
 * @brief Increment replay count
 ***************************************************/
STATIC void cmd_incr_replay_count(void);
/***************************************************
 * @brief Get replay count
 * @return Number of replay
 ***************************************************/
STATIC uint32_t cmd_get_replay_count(void);

/* Private variables ------------------------------------------*/
static bool unlocked_flag = false; /* permitted to write parameters? */

static char command_buffer[CONSOLE_RX_BUF_LEN]; /* storage of previous command */

/***************************************************
 * @brief struct for parameters related to command replay
 ***************************************************/
typedef struct
{
    uint32_t timer;       /**> timestamp*/
    uint32_t period;      /**> replay period*/
    uint32_t count;       /**> count number of replay*/
    bool disable_newline; /**> replay command without newline*/
} REPLAY_STRUCT;
static REPLAY_STRUCT replay;

/***************************************************
 * @brief struct for a command
 * This structure includes command text, pointer to the actual implementation and help text
 ***************************************************/
typedef struct
{
    const char* command;                                     /**> Pointer to the command text */
    void (*CMD_FUNC)(int32_t argc, const char* const* argv); /**> Pointer to the actual command */
    const char* help_text;                                   /**> Pointer to explanation text */
} COMMAND_STRUCT;

/***************************************************
 * @brief Table of available commands
 ***************************************************/
static const COMMAND_STRUCT command_table[] = {
    /* Basic commands BMS should support */
    {"help", cmd_help, "Shows available commands"},
    {"?", cmd_help, "Shows available commands"},
    {"version", cmd_version, "Show firmware version"},
    {"clear", cmd_clear, "Clear the terminal"},
    {"uptime", cmd_uptime, "Uptime in seconds"},
    {"clock", cmd_clock, "Clock; year, month, day, hour, minute, sec"},
    /* STATUS command (not all parameters are available) */
    {"temp_stat", cmd_temp_status, "show temperature of sensors <replay period [ms], 0=stop replay>"},
    /* Basic commands to control the system */
    {"password", cmd_password, "password to unlock certain commands"},
    {"reset", cmd_reset, "Reset the CPU"},
    {"off", cmd_off, "Switch off the power"},
    {"load", cmd_load, "Load new software"},
#ifndef DISABLE_LED
    {"led", cmd_led, "SET LED 4x (0=off, 1=on)"},
#endif
#ifndef DISABLE_LOGGING
    {"logging", cmd_log, "print logged data <opt: count>"},
#endif
    {"", NULL, ""}};

static const uint32_t command_num = sizeof(command_table) / sizeof(command_table[0]);

STATIC void cmd_set_replay(uint32_t period)
{
    timer_reset_module_timer(&replay.timer);
    replay.period = period;
    if (period == 0u)
    {
        replay.disable_newline = false;
        cmd_reset_replay_count();
    }
}

STATIC void prvcmd_disable_newline_in_replay(void)
{
    replay.disable_newline = true;
}

STATIC void cmd_reset_replay_count(void)
{
    replay.count = 0;
}

STATIC void cmd_incr_replay_count(void)
{
    replay.count++;
}

STATIC uint32_t cmd_get_replay_count(void)
{
    return replay.count;
}

/**
 * Split command line by given divide character
 * @param in Reference to command line
 * @param div_char The divide character (usually a space)
 * @param out Table of output pointer containing the split commands
 * @param max_num Maximum length if output output pointers
 * @return Number of time the command line was split
 */
STATIC int32_t nsplit(char* const in, const char div_char, char* out[], const uint32_t max_num)
{
    const uint32_t length = strlen(in);
    uint32_t index_in = 0u;
    uint32_t i = 0u;
    uint8_t tq = 0u; // toggle quotes
    uint32_t num = 0u;
    out[num] = in;
    num++;
    while ((i < length) && (num < max_num) && (in[index_in] != '\0'))
    {
        if (in[index_in] == '\"')
        {
            tq ^= 1u;
            if (tq == 1u)
            {
                uint32_t lim = 0u;
                while ((*out[num - 1u] != '\"') && (lim < 5u))
                {
                    out[num - 1u]++; // num-1=current item
                    lim++;
                }
                out[num - 1u]++;
            }
            in[index_in] = ' ';
        }

        if ((in[index_in] == div_char) && (tq == 0u))
        {
            out[num] = in + index_in + 1u;
            in[index_in] = '\0'; // terminate previous element
            index_in++;
            i++;
            // strip leading div characters for the next element
            while ((i < length) && (in[index_in] != '\0') && (in[index_in] == div_char))
            {
                index_in++;
                out[num]++;
                i++;
            }
            if (in[index_in] != '\0')
            {
                num++; // only increase num if there is something!
            }
        }
        else
        {
            index_in++;
            i++;
        }
    }

    in[index_in] = '\0'; // terminate the last segment
    return (int32_t)num;
}

void command_deinit(void)
{
    console_deinit();
}

void command_reinit(void)
{
    unlocked_flag = false; /* unlock must be false at startup */
    console_reinit();
}

void command_init(void)
{
    (void)memset(command_buffer, 0, sizeof(command_buffer)); /* no command yet */
    unlocked_flag = false;                                   /* unlock must be false at startup */
    cmd_set_replay(0u);                                      /* No command replay by default */

    printf("Hydroponics Controller Console\r\n# ");
}

void command_execute(void)
{
    char console_buffer[CONSOLE_RX_BUF_LEN];
    bool cmd_execute_flag = false; /* Will a command be executed */
    static int32_t argc;
    static char* argv[CONSOLE_RX_BUF_LEN / 2u]; /* Amount of arguments can never be more than roughly the command buffer length divided by two */

    if (console_read_line(console_buffer, CONSOLE_RX_BUF_LEN)) /* is a complete line received? */
    {
        /* Yes, execute the command */
        cmd_set_replay(0u);            /* Stop replaying any commands */
        if (console_buffer[0u] == '!') /* repeat last command ?? */
        {
            printf("#");
            argc = nsplit(command_buffer, ' ', argv, CONSOLE_RX_BUF_LEN / 2u);

            for (uint32_t i = 0u; i < (uint32_t)argc; i++)
            {
                printf("%s ", argv[i]);
            }

            printf("\r\n"); /* print the last command for user convenience */
        }
        else
        {
            (void)memset(argv, 0, sizeof(argv));                                  /* clear array */
            (void)memcpy(command_buffer, console_buffer, sizeof(console_buffer)); /* Copy new command to command_buffer */
            argc = nsplit(command_buffer, ' ', argv, sizeof(argv) / sizeof(argv[0]));
        } /* received line is processed, clear flag for next line */
        cmd_execute_flag = true;
    }
    else if (replay.period != 0u) /* Is a command set to replay ?*/
    {
        if (timer_get_elapsed_module_timer(replay.timer) >= replay.period)
        {
            cmd_execute_flag = true;
            cmd_incr_replay_count();
        }
    }
    else
    {
        // Do nothing
    }

    if (cmd_execute_flag)
    {
        uint32_t i = 0u;
        for (; i < command_num; i++)
        {
            if (command_table[i].CMD_FUNC != NULL)
            {
                if (strcmp(argv[0], command_table[i].command) == 0)
                {
                    command_table[i].CMD_FUNC(argc, (const char* const*)argv); //lint !e9087 execute the command
                    break;
                }
            }
        }
        if (i >= (command_num - 1u))
        {
            if (strlen(argv[0]) > 0u)
            {
                printf("Command not found!");
            }
        }

        if (!replay.disable_newline)
        {
            printf("\r\n#"); /* new command line */
        }
    }
}

void commands_proc(void)
{
    (void)console_background_print(CONSOLE_TIMEOUT);
#ifndef DISABLE_LOGGING
    log_background_print();
#endif
    command_execute();
}

void commands_go_setup_mode(bool reset)
{
    rtc_set_cmd_setup_flag();

    if (reset)
    {
        nvic_safe_system_reset();
    }
}

void cmd_help(int32_t argc, const char* const* argv)
{
    static char ch = 'a';
    static uint32_t index = 0u;
    static uint32_t last_index = 0u;

    UNUSED(argc);
    UNUSED(argv);

    if (cmd_get_replay_count() == 0u)
    {
        cmd_set_replay(1); /* replay help command every cycle until it print all data*/
        prvcmd_disable_newline_in_replay();
        printf("\r\n");
        ch = 'a';
        index = 0u;
        last_index = 0u;
    }

    const uint32_t print_buffer_space = console_get_print_buffer_space();
    const uint32_t command_length = strlen(command_table[index].help_text) + 25u;
    /* Check buffer has space for "command -- help_text\r\n" */
    if (print_buffer_space > command_length)
    {
        // This will display the commands in alphabetical order.
        // It's sorted on the first character only.
        // Characters '\x00' and '\xFF' are ignored
        bool did_print = false;
        do
        {
            for (uint32_t idx = last_index; !did_print && (idx < command_num); ++idx)
            {
                if (ch == command_table[idx].command[0])
                {
                    printf("%-15s -- %s", command_table[idx].command, command_table[idx].help_text);
                    index++;

                    did_print = true;
                    last_index = idx + 1u;
                    if (last_index >= (command_num - 1u))
                    {
                        last_index = 0u;
                        ++ch;
                        if (ch == (char)0xFF)
                        {
                            ch = '\x01';
                        }
                    }
                }
            }
            if (!did_print)
            {
                last_index = 0u;
                ++ch;
                if (ch == (char)0xFF)
                {
                    ch = '\x01';
                }
            }
        } while (!did_print);

        if (index >= (command_num - 1u))
        {
            cmd_set_replay(0); /* printed all help, stop replay */
        }
        else
        {
            printf("\r\n");
        }
    }
}

void cmd_version(int32_t argc, const char* const* argv)
{
    UNUSED(argc);
    UNUSED(argv);
    printf(" HW-ID: 0x%x\r\n", 0u);
}

void cmd_clear(int32_t argc, const char* const* argv)
{
    UNUSED(argc);
    UNUSED(argv);
    printf("%cc", 0x1B);
}

void cmd_uptime(int32_t argc, const char* const* argv)
{
    UNUSED(argc);
    UNUSED(argv);
    printf("Uptime: %ld\r\n", (uint32_t)timer_get_uptime());
}

void cmd_clock(int32_t argc, const char* const* argv)
{
    RTC_TimeTypeDef rtc_time;
    RTC_DateTypeDef rtc_date;
    rtc_read(&rtc_time, &rtc_date);

    if ((argc == 7) && unlocked_flag)
    {
        uint32_t year = strtoul(argv[1u], NULL, 10u);
        if (year > 2000u)
        {
            year -= 2000u;
        }

        rtc_date.Year = (uint8_t)year;
        rtc_date.Month = (uint8_t)strtoul(argv[2u], NULL, 10u);
        rtc_date.Date = (uint8_t)strtoul(argv[3u], NULL, 10u);
        rtc_time.Hours = (uint8_t)strtoul(argv[4u], NULL, 10u);
        rtc_time.Minutes = (uint8_t)strtoul(argv[5u], NULL, 10u);
        rtc_time.Seconds = (uint8_t)strtoul(argv[6u], NULL, 10u);

        (void)rtc_validate_and_correct(&rtc_time, &rtc_date);

        rtc_write(&rtc_time, &rtc_date);
    }

    printf("OK, 20%02d %02d %02d  %02d %02d %02d\r\n", rtc_date.Year, rtc_date.Month, rtc_date.Date, rtc_time.Hours, rtc_time.Minutes, rtc_time.Seconds);
}

void cmd_temp_status(int32_t argc, const char* const* argv)
{
#ifndef DISABLE_TEMPERATURE
    printf("OK\r\n");

    if (argc == 2) /* Print with improved readability and optional replay */
    {
        cmd_set_replay(strtoul(argv[1u], NULL, 10));
        printf("CELLS | %3d %3d |", temp_get_cell_temp_max(), temp_get_cell_temp_min());

        for (uint32_t i = 0u; i < TEMP_NUM_CELL_NTCS; i++)
        {
            printf(" %3d", temp_get_cell_temp(i));
        }

        printf("\r\n");
        printf("BMS   | %3d %3d |", temp_get_bms_temp_max(), temp_get_bms_temp_min());

        for (uint32_t i = 0u; i < TEMP_NUM_BMS_NTCS; i++)
        {
            printf(" %3d", temp_get_bms_temp(i));
        }

        printf("\r\n");
        printf("MCU     %3d", temp_get_mcu_temp());

#ifndef DISABLE_GAUGE
        printf("\r\n");
        printf("GGE I   %3d", gauge_get_temp_int());
#endif
    }
    else /* Classic print */
    {
        for (uint32_t i = 0u; i < TEMP_NUM_CELL_NTCS; i++)
        {
            printf(" %d", temp_get_cell_temp(i));
        }
        printf("   %d", temp_get_bms_temp_max());
        printf(" %d", temp_get_cell_temp_min());
        printf(" %d", temp_get_cell_temp_max());
    }

    printf("\r\n");
#else
    UNUSED(argc);
    UNUSED(argv);
    printf("no values\r\n");
#endif
}

void cmd_reset(int32_t argc, const char* const* argv)
{
    UNUSED(argv);
    if ((argc == 1) && unlocked_flag)
    {
        // TODO
    }
    else
    {
        printf("Error\r\n");
    }
}

void cmd_off(int32_t argc, const char* const* argv)
{
    UNUSED(argv);
    if ((argc == 1) && unlocked_flag)
    {
        // TODO
    }
    else
    {
        printf("Error\r\n");
    }
}

void cmd_load(int32_t argc, const char* const* argv)
{
    UNUSED(argv);
    if ((argc == 1) && unlocked_flag)
    {
        bootloader_start(); /* we will not return form this...... */
    }
    else
    {
        printf("Error\r\n");
    }
}

void cmd_setup_mode(int32_t argc, const char* const* argv)
{
    UNUSED(argc);
    UNUSED(argv);
    commands_go_setup_mode(true);
}

#ifndef DISABLE_LED
void cmd_led(int32_t argc, const char* const* argv)
{

    bool ret = false;

    uint32_t leds[LED_NUMBER_ON_BMS];
    uint32_t li;
    uint32_t leds_to_set;

    if ((argc <= (int32_t)(LED_NUMBER_ON_BMS + 1u)) && unlocked_flag)
    {
        leds_to_set = (uint32_t)argc - 1u;

        if (leds_to_set == LED_NUMBER_ON_BMS) /* Extra check to appease PC Lint no OOB access is happening */
        {
            for (li = 0; li < leds_to_set; li++)
            {
                leds[li] = strtoul(argv[li + 1u], NULL, 10);
            }

            ret = led_set(li, leds);
        }

        if (ret)
        {
            printf("OK\r\n");
        }
        else
        {
            printf("Error\r\n");
        }
    }
    else
    {
        printf("Error\r\n");
    }
}
#endif

#ifndef DISABLE_LOGGING
void cmd_log(int32_t argc, const char* const* argv)
{
    int32_t count;
    LOG_HEADER header;
    (void)log_read_header(&header);

    if (argc == 2)
    {
        count = strtol_(argv[1u], NULL, 10);
    }
    else
    {
        count = 20;
    }

    if ((count == (int32_t)header.counter) || (count >= (int32_t)header.max_entries))
    {
        (void)rtc_set_general_flag(LOG_PRINT_FLAG);
        (void)log_write_record_std(LOG_LOG_RESET); /* Logged with special state */
        nvic_safe_system_reset();
    }
    else
    {
        (void)log_data_print(count);
    }
}
#endif

STATIC CERTIFY certify = {0, 0};

void cmd_password(int32_t argc, const char* const* argv)
{
    unlocked_flag = false; // locked

    if (argc < 2)
    {
        auth_renew_values(&certify);
        (void)auth_unlock(&certify, 0u);
        printf("OK %lu %lu\r\n", certify.z, certify.w); // send challenge
    }
    else
    {
        const uint32_t response = strtoul(argv[1], NULL, 10);
        bool unlocked = auth_unlock(&certify, response);
        if (!unlocked)
        {
            unlocked = 0 == strcmp("N3k0c0", argv[1u]);
        }
        if (unlocked)
        {
            printf("OK\r\n");
            unlocked_flag = true; // unlocked
        }
        else
        {
            printf("ERROR\r\n");
        }
    }
}
