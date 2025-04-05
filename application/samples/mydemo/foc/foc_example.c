
#include "pinctrl.h"
#include "watchdog.h"
#include "gpio.h"
#include "tcxo.h"
#include "uart.h"

#include "soc_osal.h"
#include "app_init.h"

#include "pin_map.h"
#include "foc_example.h"
#include "wifi/wifi_connect.h"

static char g_uart_rx_buff[UART_BUFF_SIZE] = {0};

static void uart_read_handler(const void *buffer, uint16_t length, bool error)
{
    unused(error);
    if (buffer == NULL || length == 0) {
        osal_printk("uart%d int mode transfer illegal data!\r\n", UART_BUS_ID);
        return;
    }
    strncpy(g_uart_rx_buff, (char*)buffer, length);

    printf(g_uart_rx_buff);
    printf("\r\n");
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
    wifi_connect(WIFI_SSID, WIFI_PWD);
}

static int foc_task(void)
{
    pin_init();
    config_init();

    while (1) {
        uapi_watchdog_kick();
        uapi_gpio_set_val(LED_PIN, GPIO_LEVEL_HIGH);
        uapi_tcxo_delay_ms(500);
        uapi_gpio_set_val(LED_PIN, GPIO_LEVEL_LOW);
        uapi_tcxo_delay_ms(500);
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