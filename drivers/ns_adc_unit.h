/*
 * 文件说明：
 * NS800RT7P65x ADC 外设单元抽象层。
 * 一个 ns_adc_unit 表示一个硬件 ADC 单元，例如 ADCA、ADCB 或 ADCC。
 * 上层驱动通过该结构体配置基础采样、中断、DMA、Burst 和 PPB，
 * 避免在 adc0/nsadc0 设备层散落寄存器基地址和 IRQ 细节。
 */

#ifndef __NS_ADC_UNIT_H__
#define __NS_ADC_UNIT_H__

#include <rtthread.h>

/*
 * NS800RTxxxx.h 会包含厂商 adc.h/dmamux.h/edma.h/epwm.h 等外设定义。
 * 厂商 adc.h 与 RT-Thread 官方 drivers/adc.h 共用 __ADC_H__ guard，
 * 因此这里在包含前后都释放 guard，减少与 rtdevice.h 的相互影响。
 */
#ifdef __ADC_H__
#undef __ADC_H__
#endif
#include "NS800RTxxxx.h"
#ifdef __ADC_H__
#undef __ADC_H__
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define NS_ADC_UNIT_SOC_MAX 32U
#define NS_ADC_UNIT_PPB_MAX 4U
#define NS_ADC_UNIT_DMA_REQ_MAX 5U

typedef enum
{
    NS_ADC_UNIT_A = 0,
    NS_ADC_UNIT_B,
    NS_ADC_UNIT_C,
    NS_ADC_UNIT_MAX,
} ns_adc_unit_id;

struct ns_adc_unit;

typedef void (*ns_adc_irq_handler_t)(void);

typedef struct
{
    const char *name;
    ns_adc_unit_id id;
    ADC_TypeDef *base;
    ADCRESULT_TypeDef *result;
    IRQn_Type conv_irq;
    IRQn_Type event_irq;
    DMAMUX_Req dma_req[NS_ADC_UNIT_DMA_REQ_MAX];
    ADC_ReferenceMode ref_mode;
    ADC_ReferenceVoltage ref_voltage;
    ADC_ClkPrescale prescaler;
    ADC_PulsePosMode pulse_mode;
    rt_uint32_t sample_window;
} ns_adc_unit_cfg;

typedef struct
{
    ADC_SOCNumber soc;
    ADC_Trigger trigger;
    ADC_Channel adc_channel;
    rt_uint32_t sample_window;
    ADC_IntSOCTrigger int_trigger;
} ns_adc_slot_cfg;

typedef struct
{
    ADC_SOCNumber soc;
    ADC_Trigger trigger;
    ADC_Channel channel;
    rt_uint32_t sample_window;
    ADC_IntSOCTrigger int_trigger;
} ns_adc_soc_cfg;

typedef struct
{
    ns_adc_unit_id unit_id;
    const ns_adc_slot_cfg *slots;
    rt_uint32_t slot_count;
    ADC_IntNumber eoc_int;
} ns_adc_seq_cfg;

typedef struct
{
    ADC_IntNumber int_no;
    ADC_SOCNumber source_soc;
    IRQn_Type irqn;
    ns_adc_irq_handler_t handler;
    void *user_data;
} ns_adc_irq_cfg;

typedef struct
{
    EDMA_TypeDef *edma;
    DMAMUX_TypeDef *dmamux;
    rt_uint32_t edma_channel;
    rt_uint32_t req_index;
    void *buffer;
    rt_uint32_t sample_count;
    rt_bool_t major_int;
} ns_adc_dma_cfg;

typedef struct
{
    ADC_BurstTrigger trigger;
    rt_uint32_t burst_size;
    ADC_PriorityMode priority;
} ns_adc_burst_cfg;

typedef struct
{
    ADC_PPBNumber ppb;
    ADC_SOCNumber soc;
    rt_uint32_t high_limit;
    rt_uint32_t low_limit;
    rt_uint32_t event_mask;
    rt_bool_t event_irq;
} ns_adc_ppb_cfg;

typedef ns_adc_ppb_cfg ns_adc_ppb_slot_cfg;

typedef struct ns_adc_unit
{
    const ns_adc_unit_cfg *cfg;
    rt_bool_t inited;
    rt_bool_t running;
    rt_uint32_t enabled_soc_mask;
    ns_adc_irq_cfg irq;
    ns_adc_dma_cfg dma;
    ns_adc_burst_cfg burst;
    ns_adc_ppb_cfg ppb;
} ns_adc_unit;

extern ns_adc_unit ns_adc_units[NS_ADC_UNIT_MAX];

ns_adc_unit *ns_adc_get(ns_adc_unit_id id);
rt_err_t ns_adc_init(ns_adc_unit *unit, const ns_adc_unit_cfg *cfg);
rt_err_t ns_adc_deinit(ns_adc_unit *unit);
rt_err_t ns_adc_slot(ns_adc_unit *unit, const ns_adc_slot_cfg *cfg);
rt_err_t ns_adc_soc(ns_adc_unit *unit, const ns_adc_soc_cfg *cfg);
rt_err_t ns_adc_seq(const ns_adc_seq_cfg *cfg);
rt_err_t ns_adc_irq(ns_adc_unit *unit, const ns_adc_irq_cfg *cfg, rt_bool_t enable);
rt_err_t ns_adc_dma(ns_adc_unit *unit, const ns_adc_dma_cfg *cfg);
rt_err_t ns_adc_burst(ns_adc_unit *unit, const ns_adc_burst_cfg *cfg);
rt_err_t ns_adc_ppb(ns_adc_unit *unit, const ns_adc_ppb_cfg *cfg);
rt_err_t ns_adc_start(ns_adc_unit *unit);
rt_err_t ns_adc_stop(ns_adc_unit *unit);
rt_err_t ns_adc_trigger(ns_adc_unit *unit, rt_uint32_t soc_mask);
rt_err_t ns_adc_wait(ns_adc_unit *unit, ADC_IntNumber int_no, rt_int32_t timeout_ms);
rt_err_t ns_adc_once(ns_adc_unit *unit, const ns_adc_slot_cfg *slot, ADC_IntNumber int_no, rt_uint32_t *value);
rt_uint32_t ns_adc_read(ns_adc_unit *unit, ADC_SOCNumber soc);
void ns_adc_clear(ns_adc_unit *unit, ADC_IntNumber int_no);
rt_uint32_t ns_adc_mask(ADC_SOCNumber soc);

#ifdef __cplusplus
}
#endif

#endif
