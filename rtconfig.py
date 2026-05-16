import os

# 文件说明：
# RT-Thread SCons 构建使用的 Python 配置文件。
# 这里集中描述工具链、CPU 编译选项、链接脚本、输出文件和外部 SDK 路径。

BSP_ROOT = os.path.dirname(os.path.abspath(__file__))
RTT_ROOT = os.environ.get('RTT_ROOT', os.path.join(BSP_ROOT, 'external', 'rt-thread-lts-v4.1.x'))
NS800_SDK_ROOT = os.environ.get(
    'NS800_SDK_ROOT',
    '/Volumes/1TBWDBlue/Embedded_Dev/ns800rt7p65-rtthread-adc-demo/v0.4.0'
)

ARCH = 'arm'
CPU = 'cortex-m7'
CROSS_TOOL = 'gcc'
PLATFORM = 'gcc'
EXEC_PATH = os.environ.get('RTT_EXEC_PATH', '/Applications/ArmGNUToolchain/14.2.rel1/arm-none-eabi/bin')

BUILD = 'debug'

PREFIX = 'arm-none-eabi-'
CC = PREFIX + 'gcc'
CXX = PREFIX + 'g++'
AS = PREFIX + 'gcc'
AR = PREFIX + 'ar'
LINK = PREFIX + 'gcc'
TARGET_EXT = 'elf'
SIZE = PREFIX + 'size'
OBJDUMP = PREFIX + 'objdump'
OBJCPY = PREFIX + 'objcopy'
STRIP = PREFIX + 'strip'

CPU_FLAGS = ' -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard'
COMMON_FLAGS = CPU_FLAGS + ' -ffunction-sections -fdata-sections -fno-common -funsigned-char -funsigned-bitfields -Wall -Wextra'

CFLAGS = COMMON_FLAGS + ' -std=c11'
AFLAGS = CPU_FLAGS + ' -x assembler-with-cpp -D__ASSEMBLY__'
CXXFLAGS = COMMON_FLAGS + ' -std=c++11 -fno-exceptions -fno-rtti'

if BUILD == 'debug':
    CFLAGS += ' -Og -g3'
    AFLAGS += ' -g3'
    CXXFLAGS += ' -Og -g3'
else:
    CFLAGS += ' -O2'
    CXXFLAGS += ' -O2'

LINK_SCRIPT = os.path.join(BSP_ROOT, 'linker', 'ns800rt7xxx_eflash_cpu1_rtthread.ld')

LFLAGS = CPU_FLAGS
LFLAGS += ' -T%s' % LINK_SCRIPT
LFLAGS += ' -Wl,-Map=build/ns800rt7p65_rtthread_adc.map'
LFLAGS += ' -Wl,--gc-sections'
LFLAGS += ' -Wl,-e,entry'
LFLAGS += ' -nostartfiles --specs=nosys.specs'

POST_ACTION = OBJCPY + ' -O binary $TARGET build/ns800rt7p65_rtthread_adc.bin\n' + SIZE + ' $TARGET\n'

CPATH = ''
LPATH = ''
