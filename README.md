# NS800RT7P65 RT-Thread ADC BSP 示例工程

本工程是面向 NS800RT7P65 的 RT-Thread ADC device 示例 BSP。当前版本已经从
“自定义 Makefile + 手写 `user/rtconfig.h`”迁移到 RT-Thread 官方生态中的
`SCons + Kconfig + menuconfig` 构建方式。

启动流程:

先进入虚拟环境，再进行编译等操作。
不在这个虚拟环境中的话，scons等工具所需的依赖不一定全面

```bash
source .venv/bin/activate
# 退出虚拟环境：
# deactivate
```


核心原则：

- 不修改原始 NOVOSENSE SDK。
- 不修改 `external/rt-thread-lts-v4.1.x` 中的 RT-Thread 官方源码。
- `external/` 仅作为本地依赖目录存在，不纳入本 demo 仓库跟踪。
- 应用层使用 RT-Thread 官方 ADC API：`rt_adc_enable()`、`rt_adc_read()`、
  `rt_adc_disable()`、`rt_adc_voltage()`。
- NS800RT7P65x 硬件差异只封装在 BSP/驱动层，不下沉到应用层。

## 目录结构

- `SConstruct`：BSP 的 SCons 总入口，加载 RT-Thread 官方构建脚本。
- `SConscript`：本 BSP 的源码分组，引用 SDK 启动文件、板级文件、外设驱动和应用。
- `Kconfig`：NS800RT7P65 BSP 配置菜单，并继续 source RT-Thread 官方 Kconfig。
- `.config`：menuconfig 的当前配置快照，需要跟踪。
- `rtconfig.h`：由 `.config` 生成的 RT-Thread 配置头，需要跟踪，保证 clone 后可直接构建。
- `rtconfig.py`：GCC Cortex-M7 hard-float 工具链、链接脚本和 SDK 路径配置。
- `Makefile`：SCons wrapper，仅提供 `make`、`make menuconfig`、`make genconfig` 等便捷命令。
- `user/`：BSP 入口、console/FinSH 对接、LED 线程和历史 include 路径兼容头。
- `drivers/`：NS800RT7P65x ADC 外设单元抽象、标准 `rt_adc_ops` 适配层和高级伴生设备。
- `applications/`：周期 ADC 采样线程和 FinSH 手动采样命令。
- `external/rt-thread-lts-v4.1.x/`：RT-Thread 官方 `lts-v4.1.x` 本地依赖，不跟踪。
- `_deprecated_local_adc_framework/`：早期本地 ADC framework 备份，不参与构建。

## 环境变量

默认配置已经适配当前机器路径，也可以通过环境变量覆盖：

```sh
export RTT_ROOT=/path/to/rt-thread-lts-v4.1.x
export NS800_SDK_ROOT=/path/to/NS800RT7P65/SDK/v0.4.0
export RTT_EXEC_PATH=/Applications/ArmGNUToolchain/14.2.rel1/arm-none-eabi/bin
```

未设置时的默认值：

- `RTT_ROOT`：`external/rt-thread-lts-v4.1.x`
- `NS800_SDK_ROOT`：
  `/Volumes/1TBWDBlue/文稿/EmbeddedDevelop/NS800RT7P65_DevKit_260422/NS800RT7P65/SDK/v0.4.0`
- `RTT_EXEC_PATH`：
  `/Applications/ArmGNUToolchain/14.2.rel1/arm-none-eabi/bin`

## 配置

生成或刷新 `rtconfig.h`：

```sh
make genconfig
```

进入 RT-Thread menuconfig：

```sh
make menuconfig
```

当前 BSP 默认启用：

- `BSP_USING_NS800RT7P65`
- `BSP_USING_ADC0`
- `BSP_USING_JLINK_FLASH_HELPER`
- `RT_USING_COMPONENTS_INIT`
- `RT_USING_USER_MAIN`
- `RT_USING_HEAP`
- `RT_USING_DEVICE`
- `RT_USING_ADC`
- `RT_USING_MUTEX`
- `RT_USING_FINSH`

## 构建

```sh
make clean
make
```

也可以直接使用 SCons：

```sh
scons --pyconfig-silent
scons -j4
```

预期生成文件：

- `build/ns800rt7p65_rtthread_adc.elf`
- `build/ns800rt7p65_rtthread_adc.bin`
- `build/ns800rt7p65_rtthread_adc.map`

## ADC 行为

设备名：`adc0`

驱动分层：

- `ns_adc_unit`：描述一个硬件 ADC 单元，例如 ADCA、ADCB、ADCC，集中保存基地址、结果寄存器、转换 IRQ、事件 IRQ、DMAMUX 请求源和默认采样参数。
- `drv_adc_ns800rt7p65x_common.c`：维护 RT-Thread channel 到 `ns_adc_unit + SOC + ADCIN` 的静态映射，并提供基础初始化、软件触发、EOC 等待和 raw 读取。
- `drv_adc_ns800rt7p65x.c`：只实现官方 `struct rt_adc_ops`，注册 `adc0`，应用层继续使用 `rt_adc_enable/read/disable/voltage`。
- `drv_adc_ns800rt7p65x_adv.c`：注册 `nsadc0` 高级伴生设备，用 `rt_device_control/read` 承载 IRQ、DMA、Burst 和 PPB。
- `drv_adc_ns800rt7p65x_ppb.c/.h`：单独封装 PPB limit/event/delay 处理，以及 PPB 专属配置结构体、状态结构体和命令号，避免 PPB 细节散落在高级设备层。

固定通道映射：

- `0`：ADCA SOC0，ADCIN0
- `1`：ADCA SOC1，ADCIN1
- `2`：ADCC SOC0，ADCIN2
- `3`：ADCC SOC1，ADCIN3

默认配置：

- 内部 3.3 V 参考电压
- `ADC_CLK_DIV_4`
- 采样窗口：8 个 SYSCLK 周期
- 软件触发
- 12 bit raw 结果

## 运行验证

- UART1，115200 baud。
- LED 线程周期翻转板载 LED，用于确认 RT-Thread 调度正常。
- `adc_demo` 线程每秒打印 4 路 ADC raw 值。
- FinSH 命令：

```text
adc_sample
adc probe adc0
adc enable 0
adc read 0
adc voltage 0
adc disable 0
```

高级功能 FinSH 命令：

```text
adc_adv_irq
adc_adv_dma
adc_adv_burst
adc_adv_burst_dma
adc_adv_ppb
adc_adv_stop
```

## J-Link 烧录

当前 Makefile 默认使用 J-Link Commander 烧录 SCons 生成的 ELF 文件。工程在
`jlink/` 目录中保存了 `JLinkDevices.xml` 和 `NS800RT7xxx_FlashBank1.FLM`，
构建时会复制到 `build/jlink/` 便于检查，同时安装到 macOS 用户级 SEGGER
设备目录：

```text
~/Library/Application Support/SEGGER/JLinkDevices/NOVOSENSE/NS800RT7P65
```

这样 `JLinkExe -device NS800RT7P65` 就可以加载片内 Flash 编程算法。

```sh
make flash
```

等价于：

```sh
make flash-jlink
```

可按需覆盖 J-Link 参数：

```sh
make flash JLINK_EXE=JLinkExe JLINK_DEVICE=NS800RT7P65 JLINK_IF=SWD JLINK_SPEED=4000
```

Makefile 会生成临时脚本 `build/flash.jlink`，执行
`ExitOnError/reset/halt/loadfile/reset/go/quit`。如果出现
`Writing target memory failed`，通常说明 J-Link 没有加载 NS800 的 Flash
算法，重点检查：

- `build/jlink/NS800RT7xxx_FlashBank1.FLM` 是否已生成。
- `~/Library/Application Support/SEGGER/JLinkDevices/NOVOSENSE/NS800RT7P65` 下是否存在 `Devices.xml` 和 `NS800RT7xxx_FlashBank1.FLM`。
- `JLINK_DEVICE` 是否为 `NS800RT7P65`，而不是裸核名 `Cortex-M7`。

串口调试仍使用板卡对应的 UART 转串口设备，波特率设置为 115200。

## 后续迁移方向

当前工程已经使用 RT-Thread 官方 `lts-v4.1.x` 的 ADC device framework，
NS800RT7P65x 驱动只实现 `rt_adc_ops` 并通过 `rt_hw_adc_register()` 注册
`adc0`；高级能力通过 `nsadc0` 伴生设备解耦承载。后续如果要把 BSP 合入
RT-Thread 官方源码树，可以继续做这些工作：

- 将 `drivers/drv_adc_ns800rt7p65x.c/.h` 按官方 BSP 目录规范移动。
- 将 SDK 引用逐步替换为可发布的 BSP 外设库依赖。
- 补齐更多外设的 Kconfig 选项和 SConscript 分组。
- 增加板级 flash/debug 配置，完善 J-Link 调试脚本。
