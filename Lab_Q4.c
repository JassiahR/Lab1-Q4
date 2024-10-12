#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/times.h> 

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "driver/hw_timer.h"
#include "driver/uart.h"

#define NUM_SLOTS               4
#define NUM_CYCLES              5
#define TIME_SLOT_DURATION_US   5000
#define UART_BUFFER_SIZE        1024

#define ONE_SHOT_MODE           false       // testing will be done without auto reload (one-shot)
#define RELOAD_MODE             true        // testing will be done with auto reload

static int g_timer_count = 0;
static int g_ticks_per_second;
static int g_cycle_counter = 0;
static int g_slot_counter = 0;
static int g_current_time, g_previous_time, g_burn_start_time;

// Structure to store time in minutes, seconds, and milliseconds
struct TimeStore {
    int minutes;
    int seconds;
    int milliseconds;
};

static struct TimeStore g_current_timestamp;

void timer_callback(void *p_arg)
{
    g_timer_count++;
    if (g_timer_count == 10) {
        g_current_timestamp.milliseconds++;
        if (g_current_timestamp.milliseconds == 1000) {
            g_current_timestamp.seconds++;
            g_current_timestamp.milliseconds = 0;
            if (g_current_timestamp.seconds == 60) {
                g_current_timestamp.minutes++;
                g_current_timestamp.seconds = 0;
            }
        }
        g_timer_count = 0;
    }
}

void configure_uart(void)
{
    uint8_t p_uart_data = (uint8_t)malloc(UART_BUFFER_SIZE);

    uart_config_t uart_config;
    uart_config.baud_rate = 9600;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;

    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, UART_BUFFER_SIZE * 2, 0, 0, NULL, 0);
}

void enter_sleep(uint32_t sleep_duration_us)
{
    esp_sleep_enable_timer_wakeup(sleep_duration_us);
    esp_light_sleep_start();
}

void task_one(void)
{
    const char *task_message = "Task one running \n";
    uart_write_bytes(UART_NUM_0, task_message, strlen(task_message));
    enter_sleep(1000000);
}

void task_two(void)
{
    const char *task_message = "Task two running \n";
    uart_write_bytes(UART_NUM_0, task_message, strlen(task_message));
    enter_sleep(1000000);
}

void task_three(void)
{
    const char *task_message = "Task three running \n";
    uart_write_bytes(UART_NUM_0, task_message, strlen(task_message));
    enter_sleep(1000000);
}

void task_four(void)
{
    const char *task_message = "Task four running \n";
    uart_write_bytes(UART_NUM_0, task_message, strlen(task_message));
    enter_sleep(1000000);
}

int get_time_in_seconds(const struct TimeStore *p_time_store)
{
    return p_time_store->seconds;
}

void burn_time(void)
{
    const char *burn_message = "I am stuck in burn \n";
    uart_write_bytes(UART_NUM_0, burn_message, strlen(burn_message));

    g_current_time = get_time_in_seconds(&g_current_timestamp);
    while ((g_current_time = get_time_in_seconds(&g_current_timestamp) - g_previous_time) < 5) {
        /* Burn time here */
    }
    printf("Burn time = %2.2ds\n\n", (get_time_in_seconds(&g_current_timestamp) - g_current_time));
    g_previous_time = g_current_time;
    g_cycle_counter = NUM_CYCLES;
}

void (*task_table[NUM_SLOTS][NUM_CYCLES])(void) = {
    {task_one, task_two, burn_time, burn_time, burn_time},
    {task_one, task_three, burn_time, burn_time, burn_time},
    {task_one, task_four, burn_time, burn_time, burn_time},
    {burn_time, burn_time, burn_time, burn_time, burn_time}
};

void app_main(void)
{
    g_ticks_per_second = 1000;
    g_previous_time = 0;
    g_current_timestamp.milliseconds = 0;
    g_current_timestamp.seconds = 0;
    g_current_timestamp.minutes = 0;

    hw_timer_init(timer_callback, NULL);
    hw_timer_alarm_us(100, RELOAD_MODE);

    configure_uart();

    g_ticks_per_second = 10000;  // number of clock ticks per second 
    printf("Clock ticks/sec = %d\n\n", g_ticks_per_second);

    while (1) {
        for (g_slot_counter = 0; g_slot_counter < NUM_SLOTS; g_slot_counter++) {
            for (g_cycle_counter = 0; g_cycle_counter < NUM_CYCLES; g_cycle_counter++) {
                (*task_table[g_slot_counter][g_cycle_counter])();
            }
        }
    }
}