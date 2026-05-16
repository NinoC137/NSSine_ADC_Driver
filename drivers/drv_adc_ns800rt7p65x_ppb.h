/*
 * 文件说明：
 * NS800RT7P65x ADC PPB 后处理模块适配接口。
 * 本文件把 PPB limit/event/delay 相关逻辑从 nsadc0 高级设备中拆出，
 * 让高级设备只负责命令分发和状态保存。
 */

#ifndef __DRV_ADC_NS800RT7P65X_PPB_H__
#define __DRV_ADC_NS800RT7P65X_PPB_H__

#include "drv_adc_ns800rt7p65x_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NS_ADC_EVT_TRIPHI 0x0001U
#define NS_ADC_EVT_TRIPLO 0x0002U
#define NS_ADC_EVT_ZERO   0x0004U

#define NS_ADC_ADV_CMD_CONFIG_PPB_LIMIT 0x207
#define NS_ADC_ADV_CMD_GET_PPB_STATUS   0x208

#define NS_ADC_ADV_MODE_PPB_LIMIT 5

typedef struct
{
    ns_adc_unit_id unit_id;
    ADC_PPBNumber ppb;
    ADC_SOCNumber soc;
    rt_uint32_t high_limit;
    rt_uint32_t low_limit;
    rt_bool_t enable_event_irq;
} ns_adc_ppb_limit_config_t;

typedef struct
{
    rt_uint32_t event_flags;
    rt_uint32_t delay_stamp;
} ns_adc_ppb_status_t;

typedef struct
{
    ns_adc_ppb_limit_config_t limit;
    ns_adc_ppb_status_t status;
} ns_adc_ppb_ctx_t;

rt_err_t ns_adc_ppb_limit(const ns_adc_ppb_limit_config_t *cfg);
void ns_adc_ppb_event(ns_adc_unit_id id, ns_adc_ppb_status_t *status);
void ns_adc_ppb_status_init(ns_adc_ppb_status_t *status);

#ifdef __cplusplus
}
#endif

#endif
