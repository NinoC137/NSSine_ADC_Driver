/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-05-24                  the first version
 */

/*
 * 文件说明：
 * 本文件来自 SDK RT-Thread demo 的 BSP 层，负责系统堆、SysTick、
 * 板级初始化和 console/FinSH 字符输入输出对接。本 demo 只在副本内使用，
 * 原始 SDK 文件不做修改。
 */

#include <rthw.h>
#include <rtthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "main.h"

#define NS_UART_DEVICE_NAME RT_CONSOLE_DEVICE_NAME

static struct rt_device ns_uart_device;

#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
/*
 * Please modify RT_HEAP_SIZE if you enable RT_USING_HEAP
 * the RT_HEAP_SIZE max value = (sram size - ZI size), 1024 means 1024 bytes
 */
#define RT_HEAP_SIZE (56*1024)
static rt_uint8_t rt_heap[RT_HEAP_SIZE];

RT_WEAK void *rt_heap_begin_get(void)
{
    /* 返回 RT-Thread heap 起始地址。 */
    return rt_heap;
}

RT_WEAK void *rt_heap_end_get(void)
{
    /* 返回 RT-Thread heap 结束地址。 */
    return rt_heap + RT_HEAP_SIZE;
}
#endif

void SysTick_Handler(void)
{
    /* 系统 tick 中断入口：通知 RT-Thread 进入中断并推进系统节拍。 */
    rt_interrupt_enter();
    
    rt_tick_increase();

    rt_interrupt_leave();
}

static rt_err_t ns_uart_init(rt_device_t dev)
{
    RT_UNUSED(dev);
    return RT_EOK;
}

static rt_err_t ns_uart_open(rt_device_t dev, rt_uint16_t oflag)
{
    RT_UNUSED(dev);
    RT_UNUSED(oflag);

    UART_enableRxFifo(BOARD_SERIALCOM);
    UART_resetRxFifo(BOARD_SERIALCOM);
    UART_setRxFifoWatermark(BOARD_SERIALCOM, UART_FIFO_RX1);
    UART_enableRxModule(BOARD_SERIALCOM);

    return RT_EOK;
}

static rt_err_t ns_uart_close(rt_device_t dev)
{
    RT_UNUSED(dev);
    return RT_EOK;
}

static rt_size_t ns_uart_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    rt_uint8_t *data = (rt_uint8_t *)buffer;
    rt_size_t count = 0;

    RT_UNUSED(dev);
    RT_UNUSED(pos);

    while (count < size)
    {
        if (UART_isDataAvailable(BOARD_SERIALCOM) ||
            (UART_getRxFifoStatus(BOARD_SERIALCOM) != UART_FIFO_RX0))
        {
            data[count++] = (rt_uint8_t)UART_readChar(BOARD_SERIALCOM);
        }
        else if (count == 0)
        {
            rt_thread_mdelay(10);
        }
        else
        {
            break;
        }
    }

    return count;
}

static rt_size_t ns_uart_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    const char *data = (const char *)buffer;
    rt_size_t i = 0;

    RT_UNUSED(dev);
    RT_UNUSED(pos);

    for (i = 0; i < size; i++)
    {
        if (data[i] == '\n')
        {
            board_putchar('\r');
        }
        board_putchar(data[i]);
    }

    board_wait_putchar_done();
    return size;
}

static rt_err_t ns_uart_control(rt_device_t dev, int cmd, void *args)
{
    RT_UNUSED(dev);
    RT_UNUSED(cmd);
    RT_UNUSED(args);
    return -RT_ENOSYS;
}

static int ns_uart_register(void)
{
#ifdef RT_USING_DEVICE_OPS
    static const struct rt_device_ops ns_uart_ops =
    {
        ns_uart_init,
        ns_uart_open,
        ns_uart_close,
        ns_uart_read,
        ns_uart_write,
        ns_uart_control,
    };

    ns_uart_device.ops = &ns_uart_ops;
#else
    ns_uart_device.init = ns_uart_init;
    ns_uart_device.open = ns_uart_open;
    ns_uart_device.close = ns_uart_close;
    ns_uart_device.read = ns_uart_read;
    ns_uart_device.write = ns_uart_write;
    ns_uart_device.control = ns_uart_control;
#endif

    ns_uart_device.type = RT_Device_Class_Char;
    ns_uart_device.rx_indicate = RT_NULL;
    ns_uart_device.tx_complete = RT_NULL;

    return rt_device_register(&ns_uart_device,
                              NS_UART_DEVICE_NAME,
                              RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_STREAM);
}

/**
 * This function will initial your board.
 */
void rt_hw_board_init(void)
{
#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
    /*
     * RT-Thread 官方启动流程要求 heap 在 board init 阶段尽早可用。
     * Device_init()、Board_init() 或 INIT_BOARD_EXPORT 组件都可能间接
     * 注册对象、打印日志或申请内存，因此系统堆必须作为第一步初始化。
     */
    rt_system_heap_init(rt_heap_begin_get(), rt_heap_end_get());
#endif

    /* 初始化芯片时钟和基础外设。 */
    Device_init();

    /* 解锁外设寄存器，允许后续板级初始化配置 GPIO/UART 等外设。 */
    Device_unlockPeriphReg();

    /* 初始化板级资源，包括 LED、按键和串口。 */
    Board_init();

    /*
     * SDK 默认只显式配置 TX FIFO。FinSH 采用轮询方式读取字符，
     * 这里补齐 RX FIFO 与低水位，保证收到 1 个字符即可被 getchar 读到。
     */
    UART_enableRxFifo(BOARD_SERIALCOM);
    UART_resetRxFifo(BOARD_SERIALCOM);
    UART_setRxFifoWatermark(BOARD_SERIALCOM, UART_FIFO_RX1);
    UART_enableRxModule(BOARD_SERIALCOM);

    /*
     * FinSH 在启用 RT_USING_DEVICE 后只通过 shell device 读取输入。
     * 注册 uart 字符设备并设置为 console，使 msh 的 rt_device_read()
     * 能进入本文件的 UART RX 轮询路径。
     */
    ns_uart_register();
    rt_console_set_device(NS_UART_DEVICE_NAME);
    
    /* 调用 INIT_BOARD_EXPORT 注册的板级组件初始化函数。 */
#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif
}

#ifdef RT_USING_CONSOLE

void rt_hw_console_output(const char *str)
{
    /* console 输出适配：直接写 SDK 板级 UART，避免 newlib FILE/fputc 路径。 */
#if defined(__ICCARM__)
    rt_size_t size = rt_strlen(str);
    
    __write(0, str, size);
#else
    rt_size_t i = 0, size = 0;
    char a = '\r';

    size = rt_strlen(str);

    for (i = 0; i < size; i++)
    {
        if (*(str + i) == '\n')
        {
            board_putchar(a);
        }
        board_putchar(str[i]);
    }

    board_wait_putchar_done();
#endif
}
#endif

#ifdef RT_USING_FINSH
char rt_hw_console_getchar(void)
{
    /* 非 RT_USING_DEVICE 路径的 FinSH 输入适配。当前默认由 uart 设备承载输入。 */
    int ch = -1;

    if (UART_isDataAvailable(BOARD_SERIALCOM) ||
        (UART_getRxFifoStatus(BOARD_SERIALCOM) != UART_FIFO_RX0))
    {
        ch = (uint8_t)UART_readChar(BOARD_SERIALCOM);
    }
    else
    {
        rt_thread_mdelay(10);
    }

    return ch;
}
#endif
