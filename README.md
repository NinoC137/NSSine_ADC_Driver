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

## 目录结构

- `SConstruct`：BSP 的 SCons 总入口，加载 RT-Thread 官方构建脚本。
- `SConscript`：本 BSP 的源码分组，引用 SDK 启动文件、板级文件、外设驱动和应用。
- `Kconfig`：NS800RT7P65 BSP 配置菜单，并继续 source RT-Thread 官方 Kconfig。
- `rtconfig.h`：由 `.config` 生成的 RT-Thread 配置头，需要跟踪，保证 clone 后可直接构建。
- `rtconfig.py`：GCC Cortex-M7 hard-float 工具链、链接脚本和 SDK 路径配置。
- `Makefile`：SCons wrapper，仅提供 `make`、`make menuconfig`、`make genconfig` 等便捷命令。
- `user/`：BSP 入口、console/FinSH 对接、LED 线程。
- `drivers/`：NS800RT7P65x ADC 外设单元抽象、标准 `rt_adc_ops` 适配层和高级伴生设备。
- `applications/`：周期 ADC 采样线程和 FinSH 手动采样命令。
- `external/rt-thread-lts-v4.1.x/`：RT-Thread 官方 `lts-v4.1.x` 本地依赖，不跟踪。

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

进入 RT-Thread menuconfig：

```sh
make menuconfig
```

## 构建

```sh
make clean
make
```

也可以直接使用 SCons：

```sh
scons -j4
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

## 自动化联调 CLI

工程提供 `tools/bringup/nsbringup.py`，用于把编译、烧录、串口日志、FinSH 命令
和 GDB 快照串成可重复的 bring-up 流程。

安装串口依赖：

```sh
.venv/bin/pip install -r requirements-bringup.txt
```

常用命令：

```sh
python3 tools/bringup/nsbringup.py list-serial
python3 tools/bringup/nsbringup.py build --jobs 4
python3 tools/bringup/nsbringup.py flash
python3 tools/bringup/nsbringup.py serial-log --port /dev/cu.xxx
python3 tools/bringup/nsbringup.py msh --port /dev/cu.xxx --cmd adc_sample --cmd adc_regs
python3 tools/bringup/nsbringup.py gdb-snapshot --mem 0x40030000:16 --mem 0x40032000:16
python3 tools/bringup/nsbringup.py smoke --port /dev/cu.xxx
```

每次运行都会在 `build/bringup/YYYYMMDD-HHMMSS/` 下生成日志和 `summary.md`。

## ADC PPB 过采样验证

PPB 过采样使用 ADC 后处理模块的 `PPBnPSUM/PPBnPMIN/PPBnPMAX/PPBnPCOUNT`
和 final `PPBnSUM/PPBnMIN/PPBnMAX/PPBnCOUNT` 寄存器完成硬件累加与最大/最小值
记录。当前 demo 提供 FinSH 命令：

```sh
adc_ppb_os
adc_ppb_os a 1 0 0 16 1
```

第二条命令含义为：`ADCA`、`PPB1`、`SOC0`、`ADCIN0`、采样 `16` 次，并计算剔除
最大/最小值后的平均值。当前第一版使用软件重复触发指定 SOC 来驱动 PPB 过采样，
后续可替换为 repeater、burst 或 ePWM 触发源；PPB 结果读取接口保持不变。
