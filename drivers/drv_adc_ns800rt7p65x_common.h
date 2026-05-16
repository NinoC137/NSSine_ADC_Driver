/*
 * 文件说明：
 * NS800RT7P65x ADC 驱动公共硬件抽象头文件。
 * 这里集中声明标准 adc0 与高级 nsadc0 共用的通道映射、基础初始化、
 * SOC 软件触发和结果读取接口，避免两个驱动层直接复制厂商寄存器细节。
 */

#ifndef __DRV_ADC_NS800RT7P65X_COMMON_H__
#define __DRV_ADC_NS800RT7P65X_COMMON_H__

#include <rtthread.h>
#include "ns_adc_unit.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NS_ADC_CHANNEL_COUNT 32U
#define NS_ADC_SAMPLE_WINDOW 8U
#define NS_ADC_REFERENCE_MV 3300U
#define NS_ADC_RESOLUTION_BITS 12U
#define NS_ADC_EOC_TIMEOUT_MS 50U

typedef struct
{
    ns_adc_unit_id unit_id;
    ADC_SOCNumber soc;
    ADC_Channel adc_channel;
} ns_adc_channel_map_t;

rt_err_t ns_adc_check_channel(rt_int8_t channel);
const ns_adc_channel_map_t *ns_adc_get_channel(rt_uint32_t channel);
ns_adc_unit *ns_adc_get_channel_unit(const ns_adc_channel_map_t *map);
rt_err_t ns_adc_hw_init(void);
rt_err_t ns_adc_hw_reset(void);
uint32_t ns_adc_force_mask(ADC_SOCNumber soc);
rt_err_t ns_adc_wait_eoc(ADC_TypeDef *base, ADC_IntNumber int_number);
rt_uint32_t ns_adc_read_channel_raw(rt_uint32_t channel);
void ns_adc_clear_interrupt(ADC_TypeDef *base, ADC_IntNumber int_number);
rt_err_t ns_adc_channel_to_slot(rt_uint32_t channel, ns_adc_slot_cfg *slot);

#ifdef __cplusplus
}
#endif

#endif
