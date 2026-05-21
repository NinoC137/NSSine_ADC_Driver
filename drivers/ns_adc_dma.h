/*
 * 文件说明：
 * NS800RT7P65x ADC DMA 薄封装。
 * 本文件只处理 ADCINT -> DMAMUX -> EDMA -> memory 的搬运链路。
 */

#ifndef __NS_ADC_DMA_H__
#define __NS_ADC_DMA_H__

#include "ns_adc_unit.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    EDMA_TypeDef *edma;
    DMAMUX_TypeDef *dmamux;
    rt_uint32_t edma_channel;
    rt_uint32_t req_index;
    ADC_IntNumber dma_int;
    rt_bool_t major_int;
    rt_bool_t error_int;
    rt_bool_t continuous_int;
} ns_adc_dma_channel_config_t;

typedef struct
{
    ns_adc_dma_channel_config_t channel;
    void *buffer;
    rt_uint32_t frame_count;
    rt_uint32_t results_per_frame;
    ADC_SOCNumber first_soc;
    ADC_SOCNumber trans_trigger_soc;
} ns_adc_dma_cfg;

rt_err_t ns_adc_dma(ns_adc_unit *unit, const ns_adc_dma_cfg *cfg);
rt_err_t ns_adc_dma_start(ns_adc_unit *unit);
rt_bool_t ns_adc_dma_done(ns_adc_unit *unit);
rt_bool_t ns_adc_dma_error(ns_adc_unit *unit);
void ns_adc_dma_clear(ns_adc_unit *unit);
void ns_adc_dma_stop(ns_adc_unit *unit);

#ifdef __cplusplus
}
#endif

#endif
