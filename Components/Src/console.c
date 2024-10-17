/**
 * @file console.c
 * @author PL
 * @brief Implementation of console i/o module
 * @copyright (c) Copyright Nekoco 2024
 * @ingroup COMMAND
 *
 * This module is split out from command module.
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "console.h"
#include "timer.h"
#include "nvic.h"
#include "rtc.h"

#ifdef __GNUC__
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int character)
#else
#define PUTCHAR_PROTOTYPE int fputc(int char, FILE* f)
#endif /* __GNUC__ */

/**
 * @brief Struct for circular buffer
 */
typedef struct
{
    char* buffer;                /**< pointer to the buffer array */
    volatile uint16_t index_in;  /**< in index buffer */
    volatile uint16_t index_out; /**< out index buffer */
    const uint16_t length;       /**< length of the buffer */
} CONSOLE_BUFFER;

static char rx_buf_inst[CONSOLE_RX_BUF_LEN];
static CONSOLE_BUFFER rx_buffer = {rx_buf_inst, 0, 0, CONSOLE_RX_BUF_LEN};

static char tx_buf_inst[CONSOLE_TX_BUF_LEN];
static CONSOLE_BUFFER tx_buffer = {tx_buf_inst, 0, 0, CONSOLE_TX_BUF_LEN};

static UART_HandleTypeDef* console; /* UART handler for the console port */
STATIC uint8_t rx_data[4];          /* temporary receive buffer */

#ifdef UART_USE_TRANSMIT_DMA
static uint8_t tx_dma_buffer[CONSOLE_TX_DMA_BUF_LEN]; /* temporary transmit buffer */

#endif /* UART_USE_TRANSMIT_DMA */

/********
 * @brief Struct for console flags
 *********/
typedef struct
{
    bool read_echo_delay; /**< true: echo back when a received line is completed */
    bool blocking_printf; /**< true: print immediately otherwise use buffer */
    bool silent_printf;   /**< true: print immediately otherwise use buffer */
} CONSOLE_FLAGS;
static CONSOLE_FLAGS console_flags;

/********
 * @brief Struct for timers used in console module
 *********/
typedef struct
{
    uint32_t console_active_timer;   /* save time stamp when the console module is used in the BMS cycle */
    uint32_t console_disabled_timer; /* Timer used to keep track of console disable time */
    uint32_t console_disabled_time;  /* Length of time for which to disable console */
} CONSOLE_TIMERS;
static CONSOLE_TIMERS console_timers;

/***************************************************
 * @brief Enqueue a character to a console buffer
 * @param buf pointer to a CONSOLE_BUFFER
 * @param ch enqueued character
 * @return true if the data is buffered otherwise false (buffer full)
 ***************************************************/
static bool console_enq_buffer(CONSOLE_BUFFER* buf, const char ch);

/***************************************************
 * @brief Callback when uart port receive a character
 ***************************************************/
STATIC void console_rx_cplt_callback(void)
{
    (void)console_enq_buffer(&rx_buffer, (char)rx_data[0]);
    (void)uart_receive_it(console, rx_data, 1); /* activate UART receive interrupt every time */
}

/***************************************************
 * @brief Callback when uart port get an error
 ***************************************************/
STATIC void console_error_callback(void)
{
    (void)uart_receive_it(console, rx_data, 1); /* activate UART receive interrupt every time */
}

/***************************************************
 * @brief Init console buffer
 * @param buf pointer to a CONSOLE_BUFFER
 ***************************************************/
static void console_init_buffer(CONSOLE_BUFFER* buf);

/***************************************************
 * @brief Dequeue a character to a console buffer
 * @param buf pointer to a CONSOLE_BUFFER
 * @param ch pointer to store the dequeued data
 * @return true if data is dequeued otherwise false (buffer empty)
 ***************************************************/
static bool console_deq_buffer(CONSOLE_BUFFER* buf, char* ch);

/***************************************************
 * @brief Check if the buffer is full
 * @param buf pointer to a CONSOLE_BUFFER
 * @return true if the buffer is full otherwise false
 ***************************************************/
static bool console_is_buffer_full(const CONSOLE_BUFFER* buf);

/***************************************************
 * @brief get length of buffered data
 * @param buf pointer to a CONSOLE_BUFFER
 * @return length of buffered data
 ***************************************************/
static uint16_t console_get_buffered_length(const CONSOLE_BUFFER* buf);

/***************************************************
 * @brief Output character to the designated UART.
 * @param character character to be sent via uart
 ***************************************************/
PUTCHAR_PROTOTYPE
{
    if (console_flags.silent_printf)
    {
        return 0;
    }

    if (console_flags.blocking_printf)
    {
        (void)uart_transmit(console, (uint8_t*)&character, 1, CONSOLE_TIMEOUT);
    }
    else
    {
        (void)console_enq_buffer(&tx_buffer, character);
    }

    return 0;
}

void console_init(UART_HandleTypeDef* huart)
{
    console = huart;
    console_timers.console_active_timer = 0;
    console_timers.console_disabled_timer = 0;
    console_timers.console_disabled_time = 0;
    console_init_buffer(&rx_buffer);

    console_echo_delay(false);            /* default echo back immediately */
    console_enable_blocking_printf(true); /* default use blocking function */
    if (rtc_check_any_general_flag())
    {
        console_enable_silent_printf(true); /* Stop printing if any special wakeup process is required */
    }
    else
    {
        console_enable_silent_printf(false);
    }
    uart_set_rx_cplt_callback(console, console_rx_cplt_callback);
    uart_set_error_callback(console, console_error_callback);
    (void)uart_receive_it(console, rx_data, 1); /* activate UART receive interrupt first time */
}

void console_deinit(void)
{
    uart_sleep(console);
}

void console_reinit(void)
{
    uart_wakeup(console);
}

void console_echo_delay(bool enable)
{
    console_flags.read_echo_delay = enable;
}

void console_enable_blocking_printf(bool enable)
{
    console_flags.blocking_printf = enable;
}

void console_enable_silent_printf(bool enable)
{
    console_flags.silent_printf = enable;
}

bool console_read_line(char* buf, uint16_t max_len)
{
    bool rx_line_flag;         /* true if receive a complete line. */
    uint16_t rx_index_in_copy; /* copy of rx_index_in */
    char chartemp;
    static uint16_t read_line_index = 0;
    static uint16_t rx_index_in_prev = 0;             /* rx_index in last bms cycle */
    static char read_line_buffer[CONSOLE_RX_BUF_LEN]; /* uart_read_line buffer */
    uint32_t irq_cfgr;

    rx_line_flag = false;
    irq_cfgr = nvic_disable_IRQs();        /* critical section disable IRQ */
    rx_index_in_copy = rx_buffer.index_in; /* duplicate index pointers */
    nvic_enable_IRQs(irq_cfgr);            /* enable IRQ again */

    if (rx_index_in_copy != rx_index_in_prev) /* busy receiving ?, do nothing yet (HAL_UART might be corrupted! */
    {
        rx_index_in_prev = rx_index_in_copy;
    }
    else
    {
        while (console_deq_buffer(&rx_buffer, &chartemp) && (rx_line_flag == false)) /*  is new data in the buffer and previous line executed ? */
        {
            if (chartemp != '\r') /* if not a CR, then */
            {
                if (read_line_index < CONSOLE_RX_BUF_LEN) /* don't go past the buffer */
                {
                    if ((chartemp >= ' ') && (chartemp <= '~'))
                    {
                        read_line_buffer[read_line_index] = chartemp; /* add printable character to buffer */
                        read_line_index++;
                        if (console_flags.read_echo_delay == false)
                        {
                            printf("%c", chartemp);
                        }
                    }
                    if ((chartemp == (char)0x7Fu) || (chartemp == '\b')) /* backspace or Del, so delete previous character */
                    {
                        if (read_line_index > 0u)
                        {
                            --read_line_index;
                            read_line_buffer[read_line_index] = '\0'; /* remove previous character (if not last character) */
                            if (console_flags.read_echo_delay == false)
                            {
                                printf("%c", chartemp);
                            }
                        }
                    }
                }
                else /* if buffer is full, do nothing */
                {
                    printf("Console buffer overrun %d\r\n", CONSOLE_RX_BUF_LEN);
                    read_line_index = 0;
                }
            }
            else /* it was a CR, execute the command */
            {
                rx_line_flag = true;
                if (read_line_index >= CONSOLE_RX_BUF_LEN)
                {
                    read_line_index = CONSOLE_RX_BUF_LEN - 1u;
                }
                read_line_buffer[read_line_index] = '\0'; /* last character is 0 */
                if (console_flags.read_echo_delay == false)
                {
                    printf("\r\n");
                }
            }
        }
    }

    if (rx_line_flag == true)
    {
        if (console_flags.read_echo_delay == true)
        {
            printf("%s\r\n", read_line_buffer); /* echo complete line */
        }
        if (max_len > CONSOLE_RX_BUF_LEN) /* max_len can't be longer than read_line_buffer length */
        {
            (void)memcpy(buf, read_line_buffer, CONSOLE_RX_BUF_LEN);
        }
        else
        {
            (void)memcpy(buf, read_line_buffer, max_len);
        }
        read_line_index = 0;
    }

    return (rx_line_flag);
}

bool console_background_print(uint32_t timeout)
{
    uint32_t print_timer;
    char ch;
    bool ret = false;
    bool data_in_tx_buffer = console_get_buffered_length(&tx_buffer) > 0u;
    bool console_disabled = false;

    if (console_timers.console_disabled_time != 0u)
    {
        if (timer_get_elapsed_module_timer(console_timers.console_disabled_timer) > console_timers.console_disabled_time)
        {
            console_timers.console_disabled_time = 0u;
        }
        else
        {
            console_disabled = true;
        }
    }

    timer_reset_module_timer(&print_timer);
    if (data_in_tx_buffer && !console_disabled)
    {
        ret = true;
#ifdef UART_USE_TRANSMIT_DMA
        (void)timeout; /* timeout is not used if it's DMA mode */
        if (!uart_is_transmit_dma_busy(console))
        {
            uint16_t buffed_size = 0u;
            while (buffed_size < CONSOLE_TX_DMA_BUF_LEN)
            {
                if (console_deq_buffer(&tx_buffer, &ch))
                {
                    tx_dma_buffer[buffed_size] = (uint8_t)ch;
                    buffed_size++;
                }
                else
                {
                    break;
                }
            }
            (void)uart_transmit_dma(console, tx_dma_buffer, buffed_size);
        }
#else
        while (timer_get_elapsed_module_timer(print_timer) < timeout)
        {
            if (console_deq_buffer(&tx_buffer, &ch))
            {
                (void)uart_transmit(console, (uint8_t*)&ch, 1, CONSOLE_TIMEOUT);
            }
            else
            {
                break;
            }
        }
#endif /* UART_USE_TRANSMIT_DMA */
    }
    else
    {
        ret = false;
    }

    // Check if console is busy
    bool reset_timer = console_get_rx_buffer_space() != CONSOLE_RX_BUF_LEN;
    if (!reset_timer)
    {
        reset_timer = console_get_buffered_length(&tx_buffer) > 0u;
    }
    if (reset_timer)
    {
        timer_reset_module_timer(&console_timers.console_active_timer);
    }

    return ret;
}

void console_disable(uint32_t disable_time)
{
    console_timers.console_disabled_time = disable_time;
    timer_reset_module_timer(&console_timers.console_disabled_timer);
}

uint32_t console_get_active_timer(void)
{
    return console_timers.console_active_timer;
}

uint16_t console_get_print_buffer_space(void)
{
    return (CONSOLE_TX_BUF_LEN - console_get_buffered_length(&tx_buffer));
}

uint16_t console_get_rx_buffer_space(void)
{
    return (CONSOLE_RX_BUF_LEN - console_get_buffered_length(&rx_buffer));
}

static void console_init_buffer(CONSOLE_BUFFER* buf)
{
    buf->index_in = 0;
    buf->index_out = 0;
}

static bool console_enq_buffer(CONSOLE_BUFFER* buf, const char ch)
{
    uint16_t i;
    if (console_is_buffer_full(buf) == false)
    {
        i = buf->index_in % buf->length;
        buf->buffer[i] = ch;
        buf->index_in++;
        return true;
    }
    return false; /* buffer full */
}

static bool console_deq_buffer(CONSOLE_BUFFER* buf, char* ch)
{
    uint32_t irq_cfgr;
    uint16_t i, in, out;

    irq_cfgr = nvic_disable_IRQs();
    in = buf->index_in;
    nvic_enable_IRQs(irq_cfgr);
    out = buf->index_out;
    if (in != out)
    {
        i = out % buf->length;
        *ch = buf->buffer[i];
        i++;
        buf->index_out++;
        irq_cfgr = nvic_disable_IRQs();
        if (buf->index_in == buf->index_out) //lint !e931
        {
            buf->index_in = i;
            buf->index_out = i;
        }
        nvic_enable_IRQs(irq_cfgr);
        return true;
    }
    else
    {
        return false; /* no data */
    }
}

static bool console_is_buffer_full(const CONSOLE_BUFFER* buf)
{
    if ((uint16_t)(buf->index_in - buf->index_out) >= buf->length) //lint !e931
    {
        return true;
    }
    return false;
}

static uint16_t console_get_buffered_length(const CONSOLE_BUFFER* buf)
{
    return ((uint16_t)(buf->index_in - buf->index_out)); //lint !e931
}

void console_diag_pre_process(void)
{
#ifdef UART_USE_TRANSMIT_DMA
    uart_transmit_dma_pause(console);
#endif /* UART_USE_TRANSMIT_DMA */
}

void console_diag_post_process(void)
{
#ifdef UART_USE_TRANSMIT_DMA
    uart_transmit_dma_resume(console);
#endif /* UART_USE_TRANSMIT_DMA */
}

#if !defined(NDEBUG) && !defined(TEST)

void uart_transmit_assert(const char* tx, int32_t length, uint32_t timeout)
{
    (void)uart_transmit(console, (uint8_t*)tx, (uint16_t)length, timeout);
}

#endif /* _DEBUG && !TEST */
