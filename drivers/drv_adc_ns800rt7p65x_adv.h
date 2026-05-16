/*
 * 文件说明：
 * NS800RT7P65x ADC 高级伴生设备 nsadc0 的公共接口。
 * adc0 保持 RT-Thread 官方 ADC API 兼容；本文件定义的命令和结构体
 * 只用于 rt_device_control/read(nsadc0, ...) 访问中断、DMA 和 Burst。
 * PPB 是可选高级功能，其配置和状态类型定义在独立 PPB 头文件中。
 */

#ifndef __DRV_ADC_NS800RT7P65X_ADV_H__
#define __DRV_ADC_NS800RT7P65X_ADV_H__

#include <rtthread.h>
#include "ns_adc_unit.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NS_ADC_ADV_DEVICE_NAME "nsadc0"
#define NS_ADC_ADV_BUFFER_MAX 32U

typedef enum
{
    NS_ADC_ADV_MODE_IDLE = 0,
    NS_ADC_ADV_MODE_IRQ,
    NS_ADC_ADV_MODE_DMA,
    NS_ADC_ADV_MODE_BURST,
    NS_ADC_ADV_MODE_BURST_DMA,
} ns_adc_adv_mode_t;

typedef enum
{
    NS_ADC_ADV_CMD_CONFIG_IRQ = 0x200,
    NS_ADC_ADV_CMD_START_IRQ,
    NS_ADC_ADV_CMD_CONFIG_DMA,
    NS_ADC_ADV_CMD_START_DMA,
    NS_ADC_ADV_CMD_CONFIG_BURST,
    NS_ADC_ADV_CMD_START_BURST,
    NS_ADC_ADV_CMD_CONFIG_BURST_DMA,
    NS_ADC_ADV_CMD_STOP = 0x209,
    NS_ADC_ADV_CMD_GET_STATUS,
} ns_adc_adv_cmd_t;

typedef struct
{
    ns_adc_unit_id unit_id;
    ns_adc_slot_cfg slots[NS_ADC_ADV_BUFFER_MAX];
    rt_uint32_t slot_count;
    rt_uint32_t sample_count;
    ADC_IntNumber eoc_int;
} ns_adc_adv_sequence_config_t;

typedef struct
{
    ns_adc_adv_sequence_config_t sequence;
} ns_adc_adv_irq_config_t;

typedef struct
{
    ns_adc_unit_id unit_id;
    ADC_SOCNumber soc;
    ADC_Channel adc_channel;
    rt_uint32_t sample_count;
    rt_uint32_t edma_channel;
} ns_adc_adv_dma_config_t;

typedef struct
{
    ns_adc_unit_id unit_id;
    ADC_SOCNumber first_soc;
    ADC_Channel adc_channels[NS_ADC_ADV_BUFFER_MAX];
    rt_uint32_t channel_count;
    rt_uint32_t burst_size;
    ADC_BurstTrigger trigger;
    rt_uint32_t epwm_period;
    rt_uint32_t epwm_compare;
} ns_adc_adv_burst_config_t;

typedef struct
{
    ns_adc_adv_burst_config_t burst;
    rt_uint32_t edma_channel;
} ns_adc_adv_burst_dma_config_t;

typedef struct
{
    ns_adc_adv_mode_t mode;
    rt_bool_t running;
    rt_uint32_t sample_count;
    rt_uint32_t ready_count;
} ns_adc_adv_status_t;

typedef struct
{
    ns_adc_unit_id unit_id;
    ADC_SOCNumber soc;
    ADC_Channel adc_channel;
    rt_uint32_t raw;
} ns_adc_adv_sample_t;

int rt_hw_ns_adc_adv_init(void);

#ifdef __cplusplus
}
#endif

#endif
