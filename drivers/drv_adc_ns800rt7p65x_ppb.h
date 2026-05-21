/*
 * 文件说明：
 * NS800RT7P65x ADC PPB 后处理模块适配接口。
 * 本文件封装 PPB limit/event/delay 相关逻辑，供应用或调试命令直接调用。
 */

#ifndef __DRV_ADC_NS800RT7P65X_PPB_H__
#define __DRV_ADC_NS800RT7P65X_PPB_H__

#include "ns_adc_unit.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NS_ADC_EVT_TRIPHI 0x0001U
#define NS_ADC_EVT_TRIPLO 0x0002U
#define NS_ADC_EVT_ZERO   0x0004U

typedef struct
{
    ADC_PPBNumber ppb;
    ADC_SOCNumber soc;
} ns_adc_ppb_bind_t;

typedef struct
{
    ns_adc_unit_id unit_id;
    ns_adc_ppb_bind_t bind;
    rt_uint32_t high_limit;
    rt_uint32_t low_limit;
    rt_uint32_t event_mask;
    rt_bool_t event_irq;
} ns_adc_ppb_config_t;

typedef struct
{
    rt_uint32_t event_flags;
    rt_uint32_t delay_stamp;
} ns_adc_ppb_status_t;

typedef struct
{
    ns_adc_unit_id unit_id;
    ns_adc_ppb_bind_t bind;
    ADC_Channel adc_channel;
    rt_uint32_t sample_count;
    rt_uint32_t sample_window;
    rt_uint32_t shift;
    rt_bool_t drop_min_max;
    rt_bool_t enable_osint_irq;
} ns_adc_ppb_oversampling_config_t;

typedef struct
{
    rt_uint32_t count;
    rt_int32_t sum;
    rt_int32_t min;
    rt_int32_t max;
    rt_uint32_t min_index;
    rt_uint32_t max_index;
    rt_int32_t average;
    rt_int32_t trimmed_average;
    rt_int32_t value;
} ns_adc_ppb_oversampling_result_t;

rt_err_t ns_adc_ppb_limit(const ns_adc_ppb_config_t *cfg);
rt_err_t ns_adc_ppb_oversampling_config(const ns_adc_ppb_oversampling_config_t *cfg);
rt_err_t ns_adc_ppb_oversampling_start(const ns_adc_ppb_oversampling_config_t *cfg,
                                       ns_adc_ppb_oversampling_result_t *result);
void ns_adc_ppb_event(ns_adc_unit_id id, ns_adc_ppb_status_t *status);

#ifdef __cplusplus
}
#endif

#endif
