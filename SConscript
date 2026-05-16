from building import *
import os
import rtconfig

# 文件说明：
# NS800RT7P65 BSP 的源码分组文件。
# 这里只声明本 BSP 自己的应用、驱动、SDK 启动文件和板级依赖；
# RT-Thread kernel/libcpu/components 由官方 building.py 自动加入。

cwd = GetCurrentDir()
sdk_root = rtconfig.NS800_SDK_ROOT
rtt_demo = os.path.join(sdk_root, 'examples/application_demo/ns800rt7p65x/rt-thread')
rtt_root = rtconfig.RTT_ROOT

common_defines = [
    'DUAL_CORE_ENABLE=0',
    'SYSCLK_SOURCE_USE_HXTL',
    'SYSCLK_USE_PLL',
    'PLLCLK_SOURCE_USE_HXTL',
    '_GCC_C_ALIOTHXX_',
    'UART_PRINTF',
]

sdk_includes = [
    cwd,
    os.path.join(cwd, 'user'),
    os.path.join(cwd, 'drivers'),
    os.path.join(cwd, 'applications'),
    os.path.join(sdk_root, 'cmsis/core/include'),
    os.path.join(sdk_root, 'cmsis/core/include/m-profile'),
    os.path.join(sdk_root, 'common'),
    os.path.join(sdk_root, 'common/inc'),
    os.path.join(sdk_root, 'drivers/ns800rt7p65x/inc'),
    os.path.join(sdk_root, 'drivers/ns800rt7p65x/inc/ti'),
    os.path.join(sdk_root, 'boards/ns800rt7p65x/inc'),
    os.path.join(rtt_demo, 'common/include'),
    os.path.join(rtt_demo, 'common/startup_system/system'),
    os.path.join(rtt_root, 'components/drivers/include'),
    os.path.join(rtt_root, 'components/finsh'),
    os.path.join(rtt_root, 'libcpu/arm/cortex-m7'),
]

bsp_sources = [
    'user/bsp.c',
    'user/main.c',
    'drivers/ns_adc_unit.c',
    'drivers/drv_adc_ns800rt7p65x_common.c',
    'drivers/drv_adc_ns800rt7p65x.c',
    'applications/adc_demo.c',
    'user/startup_NS800RT7xxx.S',
]

if GetDepend('BSP_USING_ADC0_ADVANCED'):
    bsp_sources += [
        'drivers/drv_adc_ns800rt7p65x_adv.c',
        'applications/adc_advanced_demo.c',
    ]

if GetDepend('BSP_USING_ADC0_PPB'):
    bsp_sources += [
        'drivers/drv_adc_ns800rt7p65x_ppb.c',
    ]

sdk_sources = [
    os.path.join(sdk_root, 'boards/ns800rt7p65x/src/board.c'),
    os.path.join(sdk_root, 'common/src/common.c'),
    os.path.join(sdk_root, 'drivers/ns800rt7p65x/src/adc.c'),
    os.path.join(sdk_root, 'drivers/ns800rt7p65x/src/gpio.c'),
    os.path.join(sdk_root, 'drivers/ns800rt7p65x/src/rcc.c'),
    os.path.join(sdk_root, 'drivers/ns800rt7p65x/src/uart.c'),
    os.path.join(rtt_demo, 'common/source/device.c'),
    os.path.join(rtt_demo, 'common/startup_system/system/system_NS800RT7xxx.c'),
    os.path.join(rtt_demo, 'common/startup_system/system/system_it_NS800RT7xxx.c'),
]

if GetDepend('BSP_USING_ADC0_ADVANCED'):
    sdk_sources += [
        os.path.join(sdk_root, 'drivers/ns800rt7p65x/src/interrupt.c'),
    ]

if GetDepend('BSP_USING_ADC0_DMA'):
    sdk_sources += [
        os.path.join(sdk_root, 'drivers/ns800rt7p65x/src/dmamux.c'),
        os.path.join(sdk_root, 'drivers/ns800rt7p65x/src/edma.c'),
    ]

if GetDepend('BSP_USING_ADC0_BURST'):
    sdk_sources += [
        os.path.join(sdk_root, 'drivers/ns800rt7p65x/src/epwm.c'),
    ]

sdk_objects = []
for source in sdk_sources:
    rel_path = os.path.relpath(source, sdk_root)
    obj_path = os.path.join(cwd, 'build', 'sdk_objects', rel_path)
    obj_path = os.path.splitext(obj_path)[0] + '.o'
    sdk_objects.extend(Env.Object(target=obj_path, source=source))

src = bsp_sources + sdk_objects

group = DefineGroup('NS800RT7P65 BSP',
                    src,
                    depend=['BSP_USING_NS800RT7P65'],
                    CPPPATH=sdk_includes,
                    CPPDEFINES=common_defines)

Return('group')
