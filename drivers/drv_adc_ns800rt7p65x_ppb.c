/**
 * @file drv_adc_ns800rt7p65x_ppb.c
 * @brief NS800RT7P65x ADC PPB 后处理模块适配实现。
 *
 * 文件说明：
 * PPB(Post Processing Block) 用于对指定 SOC 的结果进行后处理，
 * 例如高低限比较、事件标志记录、转换延迟时间戳读取和硬件过采样。
 * 本文件只处理 PPB 寄存器配置和事件状态读写，不持有 nsadc0 设备状态。
 */

#include "drv_adc_ns800rt7p65x_ppb.h"
#include "ns_adc_irq.h"

#if defined(BSP_USING_ADC0_ADVANCED) && defined(BSP_USING_ADC0_PPB)

#define NS_ADC_PPB_REP_INSTANCE        ADC_REPINST1
#define NS_ADC_PPB_REP_SOC_TRIGGER     ADC_TRIGGER_REP1_TRIG
#define NS_ADC_PPB_REP_SOURCE_TRIGGER  ADC_TRIGGER_SW_ONLY
#define NS_ADC_PPB_REP_SPREAD          32U

static ADC_PPBNumber ns_adc_ppb_active[NS_ADC_UNIT_MAX];

static rt_bool_t ns_adc_ppb_is_power_of_two(rt_uint32_t value)
{
    return (value != 0U) && ((value & (value - 1U)) == 0U);
}

static rt_uint32_t ns_adc_ppb_log2(rt_uint32_t value)
{
    rt_uint32_t shift = 0U;

    while (value > 1U)
    {
        value >>= 1U;
        shift++;
    }

    return shift;
}

static rt_uint32_t ns_adc_ppb_effective_shift(const ns_adc_ppb_oversampling_config_t *cfg)
{
    if ((cfg->shift == 0U) && ns_adc_ppb_is_power_of_two(cfg->sample_count))
    {
        return ns_adc_ppb_log2(cfg->sample_count);
    }

    return cfg->shift;
}

static void ns_adc_ppb_repeater_clear_overflow(ns_adc_unit *unit)
{
    SET_BIT(unit->cfg->base->REP1CTL.WORDVAL, ADC_REP1CTL_PHASEOVF_M | ADC_REP1CTL_TRIGGEROVF_M);
}

static rt_err_t ns_adc_ppb_repeater_reset(ns_adc_unit *unit, ADC_PPBNumber ppb)
{
    ADC_triggerRepeaterCount(unit->cfg->base, NS_ADC_PPB_REP_INSTANCE, 0U);
    ADC_forceRepeaterTriggerSync(unit->cfg->base, NS_ADC_PPB_REP_INSTANCE);
    
    while ((unit->cfg->base->REP1CTL.WORDVAL & ADC_REP1CTL_MODULEBUSY_M) != 0U);

    ADC_forcePPBSync(unit->cfg->base, ppb);
    ns_adc_ppb_repeater_clear_overflow(unit);

    return RT_EOK;
}

static rt_err_t ns_adc_ppb_configure(ns_adc_unit *unit, const ns_adc_ppb_config_t *cfg)
{
    /* 配置 PPB limit/event 能力，供高级设备读取越限和延迟状态。 */
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL) || (cfg == RT_NULL))
    {
        return -RT_EINVAL;
    }

    ns_adc_ppb_active[unit->cfg->id] = cfg->bind.ppb;
    ADC_setupPPB(unit->cfg->base, cfg->bind.ppb, cfg->bind.soc);
    ADC_disablePPBEvent(unit->cfg->base, cfg->bind.ppb, cfg->event_mask);
    ADC_setPPBCalibrationOffset(unit->cfg->base, cfg->bind.ppb, 0U);
    ADC_setPPBReferenceOffset(unit->cfg->base, cfg->bind.ppb, 0U);
    ADC_disablePPBTwosComplement(unit->cfg->base, cfg->bind.ppb);
    ADC_setPPBTripLimits(unit->cfg->base, cfg->bind.ppb, ADC_PPB_PosSyb, cfg->high_limit, cfg->low_limit);
    ADC_disablePPBEventCBCClear(unit->cfg->base, cfg->bind.ppb);

    if (cfg->event_irq == RT_TRUE)
    {
        ADC_enablePPBEventInterrupt(unit->cfg->base, cfg->bind.ppb, cfg->event_mask);
        NVIC_ClearPendingIRQ(unit->cfg->event_irq);
        NVIC_EnableIRQ(unit->cfg->event_irq);
    }

    return RT_EOK;
}

rt_err_t ns_adc_ppb_limit(const ns_adc_ppb_config_t *cfg)
{
    ns_adc_unit *unit;
    /*
     * 将用户显式给出的 ADC unit/SOC/PPB 组合写入硬件。
     * PPB 是 unit 内部资源，不再固定为 PPB1。
     */
    if ((cfg == RT_NULL) ||
        (cfg->unit_id >= NS_ADC_UNIT_MAX) ||
        (cfg->bind.ppb > ADC_PPB_NUMBER4) ||
        (cfg->bind.soc > ADC_SOC_NUMBER31))
    {
        return -RT_EINVAL;
    }

    unit = ns_adc_get(cfg->unit_id);
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_EINVAL;
    }

    if (ns_adc_ppb_configure(unit, cfg) != RT_EOK)
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}

rt_err_t ns_adc_ppb_oversampling_config(const ns_adc_ppb_oversampling_config_t *cfg)
{
    ns_adc_unit *unit;
    ns_adc_slot_config_t slot;
    ns_adc_ppb_config_t ppb_cfg;
    ADC_RepeaterConfig rep_cfg;
    rt_uint32_t sample_window;
    rt_uint32_t shift;

    /*
     * 配置 PPB 硬件过采样：
     * PPB 绑定到指定 SOC，SOC 每完成一次转换，PPB 的 partial sum/count/min/max
     * 就更新一次；同步信号负责把 partial 寄存器锁存到 final SUM/COUNT/MIN/MAX
     * 并清空 partial 状态。
     */
    if ((cfg == RT_NULL) ||
        (cfg->unit_id >= NS_ADC_UNIT_MAX) ||
        (cfg->bind.ppb > ADC_PPB_NUMBER4) ||
        (cfg->bind.soc > ADC_SOC_NUMBER31) ||
        (cfg->adc_channel > ADC_CH_ADCIN31) ||
        (cfg->sample_count == 0U) ||
        (cfg->sample_count > 1024U) ||
        (cfg->shift > 10U))
    {
        return -RT_EINVAL;
    }

    unit = ns_adc_get(cfg->unit_id);
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_EINVAL;
    }

    if (ns_adc_ppb_repeater_reset(unit, cfg->bind.ppb) != RT_EOK)
    {
        return -RT_ETIMEOUT;
    }

    sample_window = cfg->sample_window;
    if (sample_window == 0U)
    {
        sample_window = NS_ADC_SAMPLE_WINDOW;
    }

    rt_memset(&slot, 0, sizeof(slot));
    slot.soc = cfg->bind.soc;
    slot.trigger = NS_ADC_PPB_REP_SOC_TRIGGER;
    slot.adc_channel = cfg->adc_channel;
    slot.sample_window = sample_window;
    slot.int_trigger = ADC_INT_SOC_TRIGGER_NONE;
    if (ns_adc_slot_cfg(unit, &slot) != RT_EOK)
    {
        return -RT_ERROR;
    }

    rt_memset(&ppb_cfg, 0, sizeof(ppb_cfg));
    ppb_cfg.bind = cfg->bind;
    ppb_cfg.high_limit = 0x0fffU;
    ppb_cfg.low_limit = 0U;
    ppb_cfg.event_mask = NS_ADC_EVT_TRIPHI | NS_ADC_EVT_TRIPLO | NS_ADC_EVT_ZERO;
    ppb_cfg.event_irq = RT_FALSE;
    if (ns_adc_ppb_configure(unit, &ppb_cfg) != RT_EOK)
    {
        return -RT_ERROR;
    }

    rt_memset(&rep_cfg, 0, sizeof(rep_cfg));
    rep_cfg.repMode = ADC_REPMODE_OVERSAMPLING;
    rep_cfg.repTrigger = NS_ADC_PPB_REP_SOURCE_TRIGGER;
    rep_cfg.repSyncin = ADC_SYNCIN_DISABLE;
    rep_cfg.repCount = (uint16_t)cfg->sample_count;
    rep_cfg.repPhase = 0U;
    rep_cfg.repSpread = NS_ADC_PPB_REP_SPREAD;
    ADC_configureRepeater(unit->cfg->base, NS_ADC_PPB_REP_INSTANCE, &rep_cfg);

    ADC_setPPBCountLimit(unit->cfg->base, cfg->bind.ppb, (uint16_t)cfg->sample_count);
    ADC_selectPPBCompareSource(unit->cfg->base, cfg->bind.ppb, ADC_PPB_COMPSOURCE_RESULT);
    ADC_selectPPBOSINTSource(unit->cfg->base,
                             cfg->bind.ppb,
                             cfg->enable_osint_irq ? ADC_PPB_OS_INT_2 : ADC_PPB_OS_INT_1);

    /*
     * SHIFT 会在 partial sum 锁存到 final SUM 时右移。调用方未显式指定
     * SHIFT 时，2^n 个样本自动使用 SHIFT=n，让 final SUM 成为硬件平均值；
     * 非 2^n 个样本则保持 SHIFT=0，后续用 SUM/COUNT 兜底。
     */
    shift = ns_adc_ppb_effective_shift(cfg);
    ADC_setPPBShiftValue(unit->cfg->base, cfg->bind.ppb, (uint16_t)shift);
    ADC_selectPPBSyncInput(unit->cfg->base, cfg->bind.ppb, ADC_SYNCIN_DISABLE);

    ADC_forceRepeaterTriggerSync(unit->cfg->base, NS_ADC_PPB_REP_INSTANCE);

    while ((unit->cfg->base->REP1CTL.WORDVAL & ADC_REP1CTL_MODULEBUSY_M) != 0U);

    ADC_forcePPBSync(unit->cfg->base, cfg->bind.ppb);
    return RT_EOK;
}

rt_err_t ns_adc_ppb_oversampling_start(const ns_adc_ppb_oversampling_config_t *cfg,
                                       ns_adc_ppb_oversampling_result_t *result)
{
    ns_adc_unit *unit;
    rt_uint32_t shift;
    rt_int32_t scaled_sum;

    /*
     * 软件只触发一次 REP1，REP1 负责连续产生 cfg->sample_count 次 SOC，
     * PPB 在硬件中完成累加、极值记录和 2^n 移位平均。
     */
    if ((cfg == RT_NULL) || (result == RT_NULL))
    {
        return -RT_EINVAL;
    }

    if (ns_adc_ppb_oversampling_config(cfg) != RT_EOK)
    {
        return -RT_ERROR;
    }

    unit = ns_adc_get(cfg->unit_id);
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_EINVAL;
    }

    ns_adc_ppb_repeater_clear_overflow(unit);
    ADC_forceRepeaterTrigger(unit->cfg->base, NS_ADC_PPB_REP_INSTANCE);
    
    while (ADC_isBusy(unit->cfg->base) != 0U);

    result->count = ADC_readPPBCount(unit->cfg->result, cfg->bind.ppb);
    result->sum = ADC_readPPBSum(unit->cfg->result, cfg->bind.ppb);
    result->min = ADC_readPPBMin(unit->cfg->result, cfg->bind.ppb);
    result->max = ADC_readPPBMax(unit->cfg->result, cfg->bind.ppb);
    result->min_index = ADC_readPPBMinIndex(unit->cfg->result, cfg->bind.ppb);
    result->max_index = ADC_readPPBMaxIndex(unit->cfg->result, cfg->bind.ppb);

    shift = ns_adc_ppb_effective_shift(cfg);
    scaled_sum = result->sum * (rt_int32_t)(1UL << shift);
    if (result->count > 0U)
    {
        if ((shift > 0U) && ((1UL << shift) == result->count))
        {
            result->average = result->sum;
        }
        else
        {
            result->average = scaled_sum / (rt_int32_t)result->count;
        }
    }
    if ((cfg->drop_min_max == RT_TRUE) && (shift == 0U) && (result->count > 2U))
    {
        result->trimmed_average = (scaled_sum - result->min - result->max) / (rt_int32_t)(result->count - 2U);
        result->value = result->trimmed_average;
    }
    else
    {
        result->trimmed_average = result->average;
        result->value = result->average;
    }

    return RT_EOK;
}

void ns_adc_ppb_event(ns_adc_unit_id id, ns_adc_ppb_status_t *status)
{
    ns_adc_unit *unit = ns_adc_get(id);
    ADC_PPBNumber ppb = ADC_PPB_NUMBER1;

    /* 读取并清除当前 unit 配置的 PPB 事件状态；事件累计策略由调用方决定。 */
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return;
    }

    ppb = ns_adc_ppb_active[id];
    if (status != RT_NULL)
    {
        status->event_flags |= ADC_getPPBEventStatus(unit->cfg->base, ppb);
        status->delay_stamp = ADC_getPPBDelayTimeStamp(unit->cfg->base, ppb);
    }

    ADC_clearPPBEventStatus(unit->cfg->base,
                            ppb,
                            NS_ADC_EVT_TRIPHI | NS_ADC_EVT_TRIPLO | NS_ADC_EVT_ZERO);
}

#endif
