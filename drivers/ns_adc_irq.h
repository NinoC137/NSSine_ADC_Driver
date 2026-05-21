/*
 * 文件说明：
 * NS800RT7P65x ADC 中断薄封装。
 * ADC 的 NVIC 入口以 ADCA/ADCB/ADCC 为单位，但每个 ADC unit 内部有
 * INT1~INT4 四个可配置中断块。这里提供最小 API：用户显式指定
 * unit、INT 编号和对应 EOC SOC，并可注册 unit 级回调。
 */

#ifndef __NS_ADC_IRQ_H__
#define __NS_ADC_IRQ_H__

#include "ns_adc_unit.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ns_adc_irq_cb_t)(ns_adc_unit_id unit_id);

rt_err_t ns_adc_irq_attach(ns_adc_unit_id unit_id, ns_adc_irq_cb_t callback);
rt_err_t ns_adc_irq_detach(ns_adc_unit_id unit_id);
rt_err_t ns_adc_event_attach(ns_adc_unit_id unit_id, ns_adc_irq_cb_t callback);
rt_err_t ns_adc_event_detach(ns_adc_unit_id unit_id);
rt_err_t ns_adc_irq_on(ns_adc_unit_id unit_id, ADC_IntNumber int_no, ADC_SOCNumber eoc_soc);
rt_err_t ns_adc_irq_off(ns_adc_unit_id unit_id, ADC_IntNumber int_no);
rt_bool_t ns_adc_irq_pending(ns_adc_unit_id unit_id, ADC_IntNumber int_no);
void ns_adc_clear(ns_adc_unit *unit, ADC_IntNumber int_no);
rt_err_t ns_adc_wait(ns_adc_unit *unit, ADC_IntNumber int_no, rt_int32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
