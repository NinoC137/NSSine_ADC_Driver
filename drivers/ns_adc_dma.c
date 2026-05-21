/**
 * @file ns_adc_dma.c
 * @brief NS800RT7P65x ADC DMA 薄封装实现。
 */

#include "ns_adc_dma.h"
#include "ns_adc_irq.h"

#ifdef BSP_USING_ADC0_DMA

static ns_adc_dma_cfg ns_adc_dma_state[NS_ADC_UNIT_MAX];

static ns_adc_dma_cfg *ns_adc_dma_get_state(ns_adc_unit *unit)
{
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL) || (unit->cfg->id >= NS_ADC_UNIT_MAX))
    {
        return RT_NULL;
    }

    return &ns_adc_dma_state[unit->cfg->id];
}

static void ns_adc_dma_set_adc_request(ns_adc_unit *unit, ADC_IntNumber int_no, rt_bool_t enabled)
{
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return;
    }

    if (int_no == ADC_INT_NUMBER1)
    {
        unit->cfg->base->DMA.BIT.INT1DMAEN = enabled ? 1U : 0U;
    }
    else if (int_no == ADC_INT_NUMBER2)
    {
        unit->cfg->base->DMA.BIT.INT2DMAEN = enabled ? 1U : 0U;
    }
    else if (int_no == ADC_INT_NUMBER3)
    {
        unit->cfg->base->DMA.BIT.INT3DMAEN = enabled ? 1U : 0U;
    }
    else if (int_no == ADC_INT_NUMBER4)
    {
        unit->cfg->base->DMA.BIT.INT4DMAEN = enabled ? 1U : 0U;
    }
}

rt_err_t ns_adc_dma(ns_adc_unit *unit, const ns_adc_dma_cfg *cfg)
{
    EDMA_CommonConfig common = {0};
    EDMA_TransferConfig transfer = {0};
    EDMA_MinorOffsetConfig minor = {0};
    ns_adc_dma_cfg *state;
    rt_uint32_t req_index;
    rt_uint32_t results_per_frame;
    rt_uint32_t frame_count;
    rt_uint32_t first_soc;
    EDMA_TypeDef *edma;
    DMAMUX_TypeDef *dmamux;

    if ((unit == RT_NULL) || (unit->cfg == RT_NULL) || (cfg == RT_NULL) || (cfg->buffer == RT_NULL))
    {
        return -RT_EINVAL;
    }

    state = ns_adc_dma_get_state(unit);
    if (state == RT_NULL)
    {
        return -RT_EINVAL;
    }

    req_index = cfg->channel.req_index;
    if (req_index >= NS_ADC_UNIT_DMA_REQ_MAX)
    {
        req_index = 0U;
    }

    edma = (cfg->channel.edma != RT_NULL) ? cfg->channel.edma : EDMA1;
    dmamux = (cfg->channel.dmamux != RT_NULL) ? cfg->channel.dmamux : DMAMUX1;

    *state = *cfg;
    state->channel.edma = edma;
    state->channel.dmamux = dmamux;
    common.enableHaltOnError = true;
    results_per_frame = (cfg->results_per_frame == 0U) ? 1U : cfg->results_per_frame;
    frame_count = (cfg->frame_count == 0U) ? 1U : cfg->frame_count;
    if ((results_per_frame > 1U) && (frame_count > 1U))
    {
        common.enableEmlm = true;
    }
    first_soc = (rt_uint32_t)cfg->first_soc;
    if (first_soc >= NS_ADC_CHANNEL_COUNT)
    {
        first_soc = 0U;
    }

    transfer.channel = cfg->channel.edma_channel;
    transfer.srcAddr = (uint32_t)&unit->cfg->result->RESULT0.WORDVAL + first_soc * 4U;
    transfer.destAddr = EDMA_getAbsAddrForMultiCore((uint32_t)cfg->buffer);
    transfer.srcTransferSize = EDMA_TRANSFERSIZE2BYTES;
    transfer.destTransferSize = EDMA_TRANSFERSIZE2BYTES;
    transfer.srcOffset = (results_per_frame > 1U) ? 4 : 0;
    transfer.destOffset = 2;
    transfer.minorLoopBytes = results_per_frame * 2U;
    transfer.majorLoopCounts = frame_count;
    transfer.slast = (results_per_frame > 1U) ? -((int32_t)results_per_frame * 4) : 0;
    transfer.dlast = -((int32_t)results_per_frame * (int32_t)frame_count * 2);
    transfer.enMajorInt = cfg->channel.major_int;
    transfer.enErrInt = cfg->channel.error_int;
    transfer.startMode = true;

    EDMA_initialize(edma);
    DMAMUX_configModule(dmamux);
    DMAMUX_setSource(dmamux, (uint8_t)cfg->channel.edma_channel, unit->cfg->dma_req[req_index]);
    DMAMUX_enableChannel(dmamux, (uint8_t)cfg->channel.edma_channel);

    EDMA_configModule(edma, &common);
    EDMA_configChannel(edma, &transfer);
    if ((results_per_frame > 1U) && (frame_count > 1U))
    {
        /*
         * One ADC frame uses RESULTn..RESULTn+results_per_frame-1 as the source.
         * Without source minor-loop offset, the next DMA request would keep
         * reading RESULTn+results_per_frame and eventually run past the ADC result
         * register block. Rewind the source pointer after each frame.
         */
        minor.enableSrcMinorOffset = true;
        minor.enableDestMinorOffset = false;
        minor.minorOffset = -((int32_t)results_per_frame * 4);
        EDMA_setMinorOffsetConfig(edma, (uint8_t)cfg->channel.edma_channel, &minor);
    }

    ns_adc_clear(unit, cfg->channel.dma_int);
    ADC_setInterruptSource(unit->cfg->base, cfg->channel.dma_int, cfg->trans_trigger_soc);
    if (cfg->channel.continuous_int == RT_TRUE)
    {
        ADC_enableContinuousMode(unit->cfg->base, cfg->channel.dma_int);
    }
    else
    {
        ADC_disableContinuousMode(unit->cfg->base, cfg->channel.dma_int);
    }
    ADC_enableInterrupt(unit->cfg->base, cfg->channel.dma_int);
    ns_adc_dma_set_adc_request(unit, cfg->channel.dma_int, RT_TRUE);

    return RT_EOK;
}

rt_err_t ns_adc_dma_start(ns_adc_unit *unit)
{
    ns_adc_dma_cfg *state = ns_adc_dma_get_state(unit);

    if ((unit == RT_NULL) || (unit->cfg == RT_NULL) || (state == RT_NULL))
    {
        return -RT_EINVAL;
    }

    if ((state->channel.edma != RT_NULL) && (state->frame_count > 0U))
    {
        EDMA_startTransfer(state->channel.edma, (uint8_t)state->channel.edma_channel);
    }

    return RT_EOK;
}

rt_bool_t ns_adc_dma_done(ns_adc_unit *unit)
{
    ns_adc_dma_cfg *state = ns_adc_dma_get_state(unit);

    if ((state == RT_NULL) || (state->channel.edma == RT_NULL))
    {
        return RT_FALSE;
    }

    return ((EDMA_getChannelIntStatus(state->channel.edma) & (1UL << state->channel.edma_channel)) != 0U) ? RT_TRUE : RT_FALSE;
}

rt_bool_t ns_adc_dma_error(ns_adc_unit *unit)
{
    ns_adc_dma_cfg *state = ns_adc_dma_get_state(unit);

    if ((state == RT_NULL) || (state->channel.edma == RT_NULL))
    {
        return RT_FALSE;
    }

    return ((EDMA_getAllChannelErrorFlags(state->channel.edma) & (1UL << state->channel.edma_channel)) != 0U) ? RT_TRUE : RT_FALSE;
}

void ns_adc_dma_clear(ns_adc_unit *unit)
{
    ns_adc_dma_cfg *state = ns_adc_dma_get_state(unit);

    if ((state == RT_NULL) || (state->channel.edma == RT_NULL))
    {
        return;
    }

    EDMA_clearChannelStatusIntFlags(state->channel.edma, (uint8_t)state->channel.edma_channel);
    EDMA_clearChannelStatusDoneFlags(state->channel.edma, (uint8_t)state->channel.edma_channel);
    EDMA_clearChannelStatusErrorFlags(state->channel.edma, (uint8_t)state->channel.edma_channel);
}

void ns_adc_dma_stop(ns_adc_unit *unit)
{
    ns_adc_dma_cfg *state = ns_adc_dma_get_state(unit);

    if ((unit == RT_NULL) || (unit->cfg == RT_NULL) || (state == RT_NULL))
    {
        return;
    }

    ns_adc_dma_set_adc_request(unit, state->channel.dma_int, RT_FALSE);
    ADC_disableInterrupt(unit->cfg->base, state->channel.dma_int);
    ADC_disableContinuousMode(unit->cfg->base, state->channel.dma_int);
    ns_adc_clear(unit, state->channel.dma_int);

    if ((state->channel.dmamux != RT_NULL) && (state->channel.edma != RT_NULL))
    {
        DMAMUX_disableChannel(state->channel.dmamux, (uint8_t)state->channel.edma_channel);
        EDMA_disableChannelRequest(state->channel.edma, (uint8_t)state->channel.edma_channel);
        EDMA_disableChannelInterrupts(state->channel.edma,
                                      (uint8_t)state->channel.edma_channel,
                                      EDMA_MAJORINTERRUPTENABLE | EDMA_ERRORINTERRUPTENABLE);
        ns_adc_dma_clear(unit);
    }

    rt_memset(state, 0, sizeof(*state));
}

#endif
