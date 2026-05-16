/**
 * @file main.c
 * @brief NS800RT7P65 RT-Thread ADC device demo 的应用入口。
 *
 * 文件说明：
 * main.c 保留一个简单 LED 线程和主循环 LED 翻转，用于确认 RT-Thread
 * 调度、系统 tick 和 GPIO 初始化正常。ADC 采样逻辑放在 applications/adc_demo.c，
 * 避免把外设 demo 逻辑堆进 main。
 */

#include "main.h"
#include "rtthread.h"

/*******************************************************************************
 * Variables
 ******************************************************************************/
static rt_thread_t led1_thread = RT_NULL;

/*******************************************************************************
 * Functions
 ******************************************************************************/
static void led1_thread_entry(void *parameter)
{
    /* LED1 线程：每 500 ms 翻转一次，用于观察线程调度是否正常。 */
    RT_UNUSED(parameter);

    while (1)
    {
        GPIO_togglePin(BOARD_LED1_PIN);
        rt_thread_mdelay(500);
    }
}

int main(void)
{
    /*
     * main 线程创建 LED1 子线程后，自己周期翻转 LED2。
     * 如果 LED1/LED2 都在闪烁，说明基本调度链路正常。
     */
    rt_kprintf("NS800RT7P65 RT-Thread ADC device demo\r\n");

    led1_thread = rt_thread_create("led1",
                                   led1_thread_entry,
                                   RT_NULL,
                                   512,
                                   3,
                                   20);

    if (led1_thread != RT_NULL)
    {
        rt_thread_startup(led1_thread);
    }

    while (1)
    {
        GPIO_togglePin(BOARD_LED2_PIN);
        rt_thread_mdelay(1000);
    }
}
