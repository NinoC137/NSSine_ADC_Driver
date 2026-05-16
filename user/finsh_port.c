/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 */

/*
 * 文件说明：
 * FinSH console 输入弱实现模板。当前 Makefile 未编译本文件，实际使用的是
 * user/bsp.c 中的 rt_hw_console_getchar()。保留此文件是为了说明 SDK
 * demo 原始移植结构，后续如果调整 console 入口，可在本 demo 副本内完善。
 */

#include <rthw.h>
#include <rtconfig.h>

#ifndef RT_USING_FINSH
#error Please uncomment the line <#include "finsh_config.h"> in the rtconfig.h 
#endif

#ifdef RT_USING_FINSH

RT_WEAK char rt_hw_console_getchar(void)
{
    /* 弱实现模板：ch 初值必须小于 0，表示当前没有读到字符。 */
    int ch = -1;

#error "TODO 4: Read a char from the uart and assign it to 'ch'."

    return ch;
}

#endif /* RT_USING_FINSH */

