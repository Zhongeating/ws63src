
#include "pinctrl.h"
#include "watchdog.h"
#include "gpio.h"
#include "tcxo.h"
#include "uart.h"

#include "lfs.h"
#include "littlefs_adapt.h"
#include "fcntl.h"

#include "soc_osal.h"
#include "app_init.h"

#include "pin_map.h"
#include "lfs_example.h"

#define UART_BUFF_SIZE  32
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

    strncpy(g_uart_rx_buff, (char*)buffer, length);

    uint8_t cpy_flag = 0;
    for (int i = 0; i < UART_BUFF_SIZE; i++)
    {
        if (cpy_flag == 1) {
            g_uart_rx_buff[i] = '\0';
        }
        else {
            uint8_t char_temp = ((uint8_t*)buffer)[i];
            if (char_temp <= 32 || char_temp > 126) {
                cpy_flag = 1;
                g_uart_rx_buff[i] = '\0';
            }
            else {
                g_uart_rx_buff[i] = (char)char_temp;
            }
        }
    }

    printf(g_uart_rx_buff);
    printf("\r\n");
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

static int lfs_task(void)
{
    pin_init();
    config_init();

    uint8_t fls_state = 0;
    int fp;
    int ret;
    char file_name[UART_BUFF_SIZE] = {0};
    char file_context[UART_BUFF_SIZE] = {0};
    while (1) {
        uapi_watchdog_kick();
        if (state_flag == 1) {
            switch (state)
            {
                case 0:
                    if (strncmp(g_uart_rx_buff, "lfs", 3) == 0) {
                        state = 1;
                    }
                    if (strncmp(g_uart_rx_buff, "foc", 3) == 0) {
                        state = 2;
                    }
                    break;
                case 1:
                    if (strncmp(g_uart_rx_buff, "cancel", 6) == 0) {
                        fls_state = 0;
                        state = 0;
                        break;
                    }
                    switch (fls_state)
                    {
                        case 0:
                            if (strncmp(g_uart_rx_buff, "read", 4) == 0) {
                                fls_state = 1;
                            }
                            if (strncmp(g_uart_rx_buff, "write", 5) == 0) {
                                fls_state = 2;
                            }
                            break;
                        case 1:
                            fp = fs_adapt_open(g_uart_rx_buff, O_RDWR);
                            if (fp < 0) {
                                printf("[FLS] open error!!\r\n");
                                fls_state = 1;
                                break;
                            }
                            ret = fs_adapt_read(fp, file_context, sizeof(file_context));
                            if (ret < 0) {
                                printf("[FLS] read error!!\r\n");
                                fs_adapt_close(fp);
                                fls_state = 1;
                                break;
                            }
                            printf("======\r\n");
                            printf(file_context);
                            printf("\r\n");
                            printf("======\r\n");
                            fs_adapt_close(fp);
                            fls_state = 0;
                            state = 0;
                            break;
                        case 2:
                            strncpy(file_name, g_uart_rx_buff, UART_BUFF_SIZE);
                            fls_state = 3;
                            break;
                        case 3:
                            printf("======\r\n");
                            printf("test");
                            printf("======\r\n");
                            printf(file_name);
                            printf("======\r\n");
                            fp = fs_adapt_open(file_name, O_RDWR | O_CREAT);
                            if (fp < 0) {
                                printf("[FLS] open error!!\r\n");
                                fls_state = 0;
                                break;
                            }
                            ret = fs_adapt_seek(fp, 0, LFS_SEEK_SET);
                            if (ret < 0) {
                                printf("[FLS] seek error!!\r\n");
                                fs_adapt_close(fp);
                                fls_state = 0;
                                break;
                            }
                            ret = fs_adapt_write(fp, g_uart_rx_buff, sizeof(g_uart_rx_buff));
                            if (ret < 0) {
                                printf("[FLS] write error!!\r\n");
                                fs_adapt_close(fp);
                                fls_state = 0;
                                break;
                            }
                            fs_adapt_close(fp);
                            fls_state = 0;
                            state = 0;
                            break;
                        default:
                            break;
                    }
                    break;
                case 2:
                    if (strncmp(g_uart_rx_buff, "cancel", 6) == 0) {
                        state = 0;
                        break;
                    }
                default:
                    break;
            }
            state_flag = 2;
        }
        if (state_flag == 2) {
            switch (state)
            {
                case 0:
                    printf("[OPT] You can input CMD: [lfs / foc]\r\n");
                    break;
                case 1:
                    switch (fls_state)
                    {
                        case 0:
                            printf("[FLS] opt: [ read / write / cancel]\r\n");
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
                    case 2:
                        printf("[FOC] opt: [ on / off / cancel]\r\n");
                        break;
                default:
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
    task_handle = osal_kthread_create((osal_kthread_handler)lfs_task, NULL, "LfsTask", TASK_STACK_SIZE);
    if (task_handle != NULL) {
        osal_kthread_set_priority(task_handle, TASK_PRIO);
        osal_kfree(task_handle);
    }
    osal_kthread_unlock();
}

/* Run the task_entry. */
app_run(task_entry);