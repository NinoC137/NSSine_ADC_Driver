# 文件说明：
# 本 Makefile 只作为 RT-Thread SCons 构建流程的便捷封装。
# 真正的源码分组、编译选项和 menuconfig 配置由 SConstruct、SConscript、
# Kconfig、.config、rtconfig.py 和生成的 rtconfig.h 管理。

PROJECT := ns800rt7p65_rtthread_adc
BUILD_DIR := build
SCONS ?= $(shell [ -x .venv/bin/scons ] && printf ".venv/bin/scons" || printf "scons")
JLINK_EXE ?= JLinkExe
JLINK_DEVICE ?= NS800RT7P65
JLINK_IF ?= SWD
JLINK_SPEED ?= 4000
JLINK_SCRIPT := $(BUILD_DIR)/flash.jlink
JLINK_DIR := $(BUILD_DIR)/jlink
JLINK_DEVICES_XML := $(JLINK_DIR)/JLinkDevices.xml
JLINK_FLASH_ALGO := $(JLINK_DIR)/NS800RT7xxx_FlashBank1.FLM
# JLINK_USER_DEVICES_DIR ?= $(HOME)/Library/Application Support/SEGGER/JLinkDevices/NOVOSENSE/NS800RT7P65

.PHONY: all clean menuconfig genconfig prepare-jlink install-jlink-device flash flash-jlink

all:
	$(SCONS) -j4

menuconfig:
	$(SCONS) --menuconfig

genconfig:
	$(SCONS) --pyconfig-silent

clean:
	$(SCONS) -c
	rm -rf $(BUILD_DIR)

flash: flash-jlink

prepare-jlink: $(JLINK_DEVICES_XML) $(JLINK_FLASH_ALGO) #install-jlink-device

$(JLINK_DEVICES_XML): jlink/JLinkDevices.xml
	@mkdir -p $(JLINK_DIR)
	cp $< $@

$(JLINK_FLASH_ALGO): jlink/NS800RT7xxx_FlashBank1.FLM
	@mkdir -p $(JLINK_DIR)
	cp $< $@

# install-jlink-device:
# 	@mkdir -p "$(JLINK_USER_DEVICES_DIR)"
# 	cp jlink/JLinkDevices.xml "$(JLINK_USER_DEVICES_DIR)/Devices.xml"
# 	cp jlink/NS800RT7xxx_FlashBank1.FLM "$(JLINK_USER_DEVICES_DIR)/NS800RT7xxx_FlashBank1.FLM"

flash-jlink: $(BUILD_DIR)/$(PROJECT).elf prepare-jlink
	# J-Link 烧录：使用 J-Link Commander 下载 ELF 到目标板并复位运行。
	@mkdir -p $(BUILD_DIR)
	@printf "ExitOnError 1\nr\nhalt\nloadfile %s\nr\ng\nqc\n" "$<" > $(JLINK_SCRIPT)
	$(JLINK_EXE) -device $(JLINK_DEVICE) -if $(JLINK_IF) -speed $(JLINK_SPEED) -CommanderScript $(JLINK_SCRIPT)
