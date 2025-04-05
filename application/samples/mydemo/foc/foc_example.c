
#include "pinctrl.h"
#include "watchdog.h"
#include "gpio.h"
#include "tcxo.h"
#include "uart.h"
#include "littlefs_adapt.h"
#include "lfs.h"

#include "soc_osal.h"
#include "app_init.h"

#include "pin_map.h"
#include "foc_example.h"

#define UART_BUFF_SIZE  64
#define UART_BUS_ID     0

static char g_uart_rx_buff[UART_BUFF_SIZE] = {0};
static uint8_t state = 0;
static uint8_t state_flag = 1;

static void uart_read_handler(const void *buffer, uint16_t length, bool error)
{
    unused(error);
    if (buffer == NULL || length == 0) {
        osal_printk("uart%d int mode transfer illegal data!\r\n", UART_BUS_ID);
        return;
    }

    char *ptr = strchr((char*)buffer, ' ');
    if (ptr != NULL) {
        length = ptr - (char*)buffer + 2;
        strncpy(ptr, "\r\n", 2);
    }
    strncpy(g_uart_rx_buff, (char*)buffer, length);

    printf(g_uart_rx_buff);
    state_flag = 1;
}

static void pin_init(void)
{
    uapi_pin_set_mode(LED_PIN, PIN_FUNC_IO_02_GPIO_02);
    uapi_gpio_set_dir(LED_PIN, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_val(LED_PIN, GPIO_LEVEL_LOW);
}

static void config_init(void)
{
    uapi_uart_register_rx_callback(UART_BUS_ID, UART_RX_CONDITION_FULL_OR_SUFFICIENT_DATA_OR_IDLE,
                                    UART_BUFF_SIZE, uart_read_handler);
}

static int foc_task(void)
{
    pin_init();
    config_init();


    uint8_t fls_state = 0;
    while (1) {
        uapi_watchdog_kick();
        if (state_flag == 1) {
            switch (state)
            {
            case 0:
                if (strncmp(g_uart_rx_buff, "lfs", 3) == 0) {
                    state = 1;
                }
                break;
            case 1:
                switch (fls_state)
                {
                case 0:
                    if (strncmp(g_uart_rx_buff, "read", 4) == 0) {
                        state = 1;
                    }
                    if (strncmp(g_uart_rx_buff, "write", 5) == 0) {
                        state = 2;
                    }
                    break;
                case 1:
                    break;
                case 2:
                    break;
                case 3:
                    break;
                default:
                    break;
                }
                break;
            }
            state_flag = 2;
        }
        if (state_flag == 2) {
            switch (state)
            {
            case 0:
                printf("[OPT] You can input CMD:\r\n");
                break;
            case 1:
                switch (fls_state)
                {
                case 0:
                    printf("[FLS] opt: [ read / write ]\r\n");
                    break;
                case 1:
                    printf("[FLS] file name:\r\n");
                    break;
                case 2:
                    printf("[FLS] file name:\r\n");
                    break;
                case 3:
                    printf("[FLS] new context:\r\n");
                    break;
                default:
                    break;
                }
                break;
            }
            state_flag = 0;
        }
        // uapi_gpio_set_val(LED_PIN, GPIO_LEVEL_HIGH);
        // uapi_tcxo_delay_ms(500);
        // uapi_gpio_set_val(LED_PIN, GPIO_LEVEL_LOW);
    }

    return 0;
}

static void task_entry(void)
{
    osal_task *task_handle = NULL;
    osal_kthread_lock();
    task_handle = osal_kthread_create((osal_kthread_handler)foc_task, NULL, "FocTask", TASK_STACK_SIZE);
    if (task_handle != NULL) {
        osal_kthread_set_priority(task_handle, TASK_PRIO);
        osal_kfree(task_handle);
    }
    osal_kthread_unlock();
}

/* Run the task_entry. */
app_run(task_entry);