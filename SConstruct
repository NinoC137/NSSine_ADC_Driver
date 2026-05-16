import os
import sys
import rtconfig

# 文件说明：
# NS800RT7P65 独立 BSP 的 SCons 总入口。
# 它加载 RT-Thread 官方 tools/building.py，并把本 BSP 交给官方
# PrepareBuilding()/DoBuilding() 流程管理。

BSP_ROOT = os.path.abspath(os.getcwd())
RTT_ROOT = os.environ.get('RTT_ROOT', os.path.join(BSP_ROOT, 'external', 'rt-thread-lts-v4.1.x'))

sys.path = sys.path + [os.path.join(RTT_ROOT, 'tools')]
from building import *

TARGET = 'build/ns800rt7p65_rtthread_adc.' + rtconfig.TARGET_EXT

DefaultEnvironment(tools=[])
env = Environment(tools=['mingw'],
                  AS=rtconfig.AS,
                  ASFLAGS=rtconfig.AFLAGS,
                  CC=rtconfig.CC,
                  CFLAGS=rtconfig.CFLAGS,
                  CXX=rtconfig.CXX,
                  CXXFLAGS=rtconfig.CXXFLAGS,
                  AR=rtconfig.AR,
                  ARFLAGS='-rc',
                  LINK=rtconfig.LINK,
                  LINKFLAGS=rtconfig.LFLAGS)
env.PrependENVPath('PATH', rtconfig.EXEC_PATH)
env['ASCOM'] = env['ASPPCOM']

Export('RTT_ROOT')
Export('rtconfig')

objs = PrepareBuilding(env, RTT_ROOT)
DoBuilding(TARGET, objs)
