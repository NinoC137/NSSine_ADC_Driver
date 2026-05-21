/*
 * 文件说明：
 * NS800RT7P65x ADC RT-Thread 适配层的公共配置头文件。
 * 这里声明设备名、通道数量、默认采样参数以及驱动初始化入口。
 * 应用层不应包含厂商 ADC 细节，只需要使用 RT-Thread 官方 rtdevice.h
 * 暴露的标准 ADC API。
 */

#ifndef __DRV_ADC_NS800RT7P65X_H__
#define __DRV_ADC_NS800RT7P65X_H__

#include "rtdevice.h"
#include "ns_adc_unit.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NS_ADC_DEVICE_NAME "adc0"

int rt_hw_ns_adc_init(void);

#ifdef __cplusplus
}
#endif

#endif
