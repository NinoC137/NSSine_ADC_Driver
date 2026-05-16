/**
 * @file ns_adc_unit.c
 * @brief NS800RT7P65x ADC 外设单元抽象层实现。
 *
 * 文件说明：
 * 本文件以 ns_adc_unit 为核心，对 ADCA/ADCB/ADCC 的寄存器基地址、
 * 结果寄存器、IRQ、DMAMUX 请求源和常用功能进行统一封装。
 * 函数名保持简短，贴近 RT-Thread BSP 常见命名风格。
 */

#include "ns_adc_unit.h"

static const ns_adc_unit_cfg ns_adc_unit_cfgs[NS_ADC_UNIT_MAX] =
{
    {
        .name = "adca",
        .id = NS_ADC_UNIT_A,
        .base = ADCA,
        .result = ADCARESULT,
        .conv_irq = ADCA_CONV_IRQn,
        .event_irq = ADCA_EVENT_IRQn,
        .dma_req = {DMAMUX_ADCA1_REQ, DMAMUX_ADCA2_REQ, DMAMUX_ADCA3_REQ, DMAMUX_ADCA4_REQ, DMAMUX_ADCA5_REQ},
        .ref_mode = ADC_REFERENCE_INTERNAL,
        .ref_voltage = ADC_REFERENCE_3_3V,
        .prescaler = ADC_CLK_DIV_4,
        .pulse_mode = ADC_PULSE_END_OF_CONV,
        .sample_window = 8U,
    },
    {
        .name = "adcb",
        .id = NS_ADC_UNIT_B,
        .base = ADCB,
        .result = ADCBRESULT,
        .conv_irq = ADCB_CONV_IRQn,
        .event_irq = ADCB_EVENT_IRQn,
        .dma_req = {DMAMUX_ADCB1_REQ, DMAMUX_ADCB2_REQ, DMAMUX_ADCB3_REQ, DMAMUX_ADCB4_REQ, DMAMUX_ADCB5_REQ},
        .ref_mode = ADC_REFERENCE_INTERNAL,
        .ref_voltage = ADC_REFERENCE_3_3V,
        .prescaler = ADC_CLK_DIV_4,
        .pulse_mode = ADC_PULSE_END_OF_CONV,
        .sample_window = 8U,
    },
    {
        .name = "adcc",
        .id = NS_ADC_UNIT_C,
        .base = ADCC,
        .result = ADCCRESULT,
        .conv_irq = ADCC_CONV_IRQn,
        .event_irq = ADCC_EVENT_IRQn,
        .dma_req = {DMAMUX_ADCC1_REQ, DMAMUX_ADCC2_REQ, DMAMUX_ADCC3_REQ, DMAMUX_ADCC4_REQ, DMAMUX_ADCC5_REQ},
        .ref_mode = ADC_REFERENCE_INTERNAL,
        .ref_voltage = ADC_REFERENCE_3_3V,
        .prescaler = ADC_CLK_DIV_4,
        .pulse_mode = ADC_PULSE_END_OF_CONV,
        .sample_window = 8U,
    },
};

ns_adc_unit ns_adc_units[NS_ADC_UNIT_MAX];

ns_adc_unit *ns_adc_get(ns_adc_unit_id id)
{
    /* 获取一个静态 ADC 单元对象，调用方无需管理其生命周期。 */
    if (id >= NS_ADC_UNIT_MAX)
    {
        return RT_NULL;
    }

    if (ns_adc_units[id].cfg == RT_NULL)
    {
        ns_adc_init(&ns_adc_units[id], &ns_adc_unit_cfgs[id]);
    }

    return &ns_adc_units[id];
}

rt_err_t ns_adc_init(ns_adc_unit *unit, const ns_adc_unit_cfg *cfg)
{
    /* 初始化 ADC 单元的基础模拟参考、电源、时钟分频和中断脉冲位置。 */
    if ((unit == RT_NULL) || (cfg == RT_NULL) || (cfg->base == RT_NULL) || (cfg->result == RT_NULL))
    {
        return -RT_EINVAL;
    }

    rt_memset(unit, 0, sizeof(*unit));
    unit->cfg = cfg;

    ADC_setVREF(cfg->base, cfg->ref_mode, cfg->ref_voltage);
    ADC_setPrescaler(cfg->base, cfg->prescaler);
    ADC_setInterruptPulsePosMode(cfg->base, cfg->pulse_mode);
    ADC_enableConverter(cfg->base);
    Delay_us(1000);
    ADC_disableBurstMode(cfg->base);
    ADC_setSOCPriority(cfg->base, ADC_PRI_ALL_ROUND_ROBIN);

    unit->inited = RT_TRUE;
    return RT_EOK;
}

rt_err_t ns_adc_deinit(ns_adc_unit *unit)
{
    /* 当前 SDK 没有对称的 ADC 关闭流程，这里只清理本地状态并停止高级触发。 */
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_EINVAL;
    }

    ns_adc_stop(unit);
    unit->inited = RT_FALSE;
    unit->enabled_soc_mask = 0U;
    return RT_EOK;
}

rt_err_t ns_adc_slot(ns_adc_unit *unit, const ns_adc_slot_cfg *cfg)
{
    rt_uint32_t sample_window;

    /* 配置单个 SOC 的触发源、ADCIN 通道、采样窗口和中断联动。 */
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL) || (cfg == RT_NULL))
    {
        return -RT_EINVAL;
    }

    if ((cfg->soc > ADC_SOC_NUMBER31) || (cfg->adc_channel > ADC_CH_ADCIN31))
    {
        return -RT_EINVAL;
    }

    sample_window = cfg->sample_window;
    if (sample_window == 0U)
    {
        sample_window = unit->cfg->sample_window;
    }

    ADC_setupSOC(unit->cfg->base, cfg->soc, cfg->trigger, cfg->adc_channel, sample_window);
    ADC_setInterruptSOCTrigger(unit->cfg->base, cfg->soc, cfg->int_trigger);
    unit->enabled_soc_mask |= ns_adc_mask(cfg->soc);
    return RT_EOK;
}

rt_err_t ns_adc_soc(ns_adc_unit *unit, const ns_adc_soc_cfg *cfg)
{
    ns_adc_slot_cfg slot;

    /* 兼容旧的 SOC 配置入口；新代码优先使用 ns_adc_slot()。 */
    if (cfg == RT_NULL)
    {
        return -RT_EINVAL;
    }

    slot.soc = cfg->soc;
    slot.trigger = cfg->trigger;
    slot.adc_channel = cfg->channel;
    slot.sample_window = cfg->sample_window;
    slot.int_trigger = cfg->int_trigger;
    return ns_adc_slot(unit, &slot);
}

rt_err_t ns_adc_seq(const ns_adc_seq_cfg *cfg)
{
    ns_adc_unit *unit;
    ns_adc_irq_cfg irq_cfg;
    rt_uint32_t i;

    /*
     * 配置一个 ADC unit 内部的 SOC 序列。
     * 序列只描述 SOC 与 ADCIN 的绑定，是否触发由调用者决定。
     */
    if ((cfg == RT_NULL) || (cfg->slots == RT_NULL) ||
        (cfg->unit_id >= NS_ADC_UNIT_MAX) ||
        (cfg->slot_count == 0U) ||
        (cfg->slot_count > NS_ADC_UNIT_SOC_MAX))
    {
        return -RT_EINVAL;
    }

    unit = ns_adc_get(cfg->unit_id);
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_EINVAL;
    }

    for (i = 0U; i < cfg->slot_count; i++)
    {
        if (ns_adc_slot(unit, &cfg->slots[i]) != RT_EOK)
        {
            return -RT_EINVAL;
        }
    }

    rt_memset(&irq_cfg, 0, sizeof(irq_cfg));
    irq_cfg.int_no = cfg->eoc_int;
    irq_cfg.source_soc = cfg->slots[cfg->slot_count - 1U].soc;
    irq_cfg.irqn = unit->cfg->conv_irq;
    return ns_adc_irq(unit, &irq_cfg, RT_TRUE);
}

rt_err_t ns_adc_irq(ns_adc_unit *unit, const ns_adc_irq_cfg *cfg, rt_bool_t enable)
{
    /* 绑定 ADCINT 到 SOC，并按需挂接/使能转换完成 IRQ。 */
    IRQn_Type irqn;

    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_EINVAL;
    }

    if (enable == RT_FALSE)
    {
        NVIC_DisableIRQ(unit->cfg->conv_irq);
        NVIC_ClearPendingIRQ(unit->cfg->conv_irq);
        return RT_EOK;
    }

    if (cfg == RT_NULL)
    {
        return -RT_EINVAL;
    }

    unit->irq = *cfg;
    irqn = (cfg->irqn >= 0) ? cfg->irqn : unit->cfg->conv_irq;

    ADC_setInterruptSource(unit->cfg->base, cfg->int_no, cfg->source_soc);
    ADC_clearInterruptStatus(unit->cfg->base, cfg->int_no);
    ADC_disableContinuousMode(unit->cfg->base, cfg->int_no);
    ADC_enableInterrupt(unit->cfg->base, cfg->int_no);

    if (cfg->handler == RT_NULL)
    {
        return RT_EOK;
    }

    NVIC_SetVector(irqn, (uint32_t)cfg->handler);
    NVIC_ClearPendingIRQ(irqn);
    NVIC_EnableIRQ(irqn);
    return RT_EOK;
}

rt_err_t ns_adc_dma(ns_adc_unit *unit, const ns_adc_dma_cfg *cfg)
{
    /* 配置 ADC 到内存的 EDMA/DMAMUX 搬运通道。 */
    EDMA_CommonConfig common = {0};
    EDMA_TransferConfig transfer = {0};
    rt_uint32_t req_index;

    if ((unit == RT_NULL) || (unit->cfg == RT_NULL) || (cfg == RT_NULL) || (cfg->buffer == RT_NULL))
    {
        return -RT_EINVAL;
    }

    req_index = cfg->req_index;
    if (req_index >= NS_ADC_UNIT_DMA_REQ_MAX)
    {
        req_index = 0U;
    }

    unit->dma = *cfg;
    common.enableHaltOnError = true;

    transfer.channel = cfg->edma_channel;
    transfer.srcAddr = (uint32_t)unit->cfg->result;
    transfer.destAddr = EDMA_getAbsAddrForMultiCore((uint32_t)cfg->buffer);
    transfer.srcTransferSize = EDMA_TRANSFERSIZE2BYTES;
    transfer.destTransferSize = EDMA_TRANSFERSIZE2BYTES;
    transfer.destOffset = 2;
    transfer.minorLoopBytes = 2;
    transfer.majorLoopCounts = cfg->sample_count;
    transfer.dlast = -((int32_t)cfg->sample_count * 2);
    transfer.enMajorInt = cfg->major_int;
    transfer.startMode = true;

    DMAMUX_configModule(cfg->dmamux);
    DMAMUX_setSource(cfg->dmamux, (uint8_t)cfg->edma_channel, unit->cfg->dma_req[req_index]);
    DMAMUX_enableChannel(cfg->dmamux, (uint8_t)cfg->edma_channel);

    EDMA_configModule(cfg->edma, &common);
    EDMA_configChannel(cfg->edma, &transfer);
    return RT_EOK;
}

rt_err_t ns_adc_burst(ns_adc_unit *unit, const ns_adc_burst_cfg *cfg)
{
    /* 配置 ADC Burst 模式，具体 SOC 序列仍由 ns_adc_soc() 提前设置。 */
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL) || (cfg == RT_NULL) || (cfg->burst_size == 0U))
    {
        return -RT_EINVAL;
    }

    unit->burst = *cfg;
    ADC_enableBurstMode(unit->cfg->base);
    ADC_setBurstModeConfig(unit->cfg->base, cfg->trigger, cfg->burst_size);
    ADC_setSOCPriority(unit->cfg->base, cfg->priority);
    return RT_EOK;
}

rt_err_t ns_adc_ppb(ns_adc_unit *unit, const ns_adc_ppb_cfg *cfg)
{
    /* 配置 PPB limit/event 能力，供高级设备读取越限和延迟状态。 */
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL) || (cfg == RT_NULL))
    {
        return -RT_EINVAL;
    }

    unit->ppb = *cfg;
    ADC_setupPPB(unit->cfg->base, cfg->ppb, cfg->soc);
    ADC_disablePPBEvent(unit->cfg->base, cfg->ppb, cfg->event_mask);
    ADC_setPPBCalibrationOffset(unit->cfg->base, cfg->ppb, 0U);
    ADC_setPPBReferenceOffset(unit->cfg->base, cfg->ppb, 0U);
    ADC_disablePPBTwosComplement(unit->cfg->base, cfg->ppb);
    ADC_setPPBTripLimits(unit->cfg->base, cfg->ppb, ADC_PPB_PosSyb, cfg->high_limit, cfg->low_limit);
    ADC_disablePPBEventCBCClear(unit->cfg->base, cfg->ppb);

    if (cfg->event_irq == RT_TRUE)
    {
        ADC_enablePPBEventInterrupt(unit->cfg->base, cfg->ppb, cfg->event_mask);
        NVIC_ClearPendingIRQ(unit->cfg->event_irq);
        NVIC_EnableIRQ(unit->cfg->event_irq);
    }

    return RT_EOK;
}

rt_err_t ns_adc_start(ns_adc_unit *unit)
{
    /* 启动与该 ADC 单元绑定的 DMA 请求。普通软件触发采样无需显式 start。 */
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_EINVAL;
    }

    unit->running = RT_TRUE;
    if ((unit->dma.edma != RT_NULL) && (unit->dma.sample_count > 0U))
    {
        EDMA_startTransfer(unit->dma.edma, (uint8_t)unit->dma.edma_channel);
    }

    return RT_EOK;
}

rt_err_t ns_adc_stop(ns_adc_unit *unit)
{
    /* 停止该 ADC 单元使用的 IRQ、DMA 请求和 Burst 模式。 */
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_EINVAL;
    }

    unit->running = RT_FALSE;
    NVIC_DisableIRQ(unit->cfg->conv_irq);
    NVIC_ClearPendingIRQ(unit->cfg->conv_irq);
    NVIC_DisableIRQ(unit->cfg->event_irq);
    NVIC_ClearPendingIRQ(unit->cfg->event_irq);

    if ((unit->dma.dmamux != RT_NULL) && (unit->dma.edma != RT_NULL))
    {
        DMAMUX_disableChannel(unit->dma.dmamux, (uint8_t)unit->dma.edma_channel);
        EDMA_disableChannelRequest(unit->dma.edma, (uint8_t)unit->dma.edma_channel);
    }

    ADC_disableBurstMode(unit->cfg->base);
    return RT_EOK;
}

rt_err_t ns_adc_trigger(ns_adc_unit *unit, rt_uint32_t soc_mask)
{
    /* 软件触发一个或多个 SOC。 */
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL) || (soc_mask == 0U))
    {
        return -RT_EINVAL;
    }

    ADC_forceMultipleSOC(unit->cfg->base, soc_mask);
    return RT_EOK;
}

rt_err_t ns_adc_wait(ns_adc_unit *unit, ADC_IntNumber int_no, rt_int32_t timeout_ms)
{
    rt_tick_t start;
    rt_tick_t timeout;
    rt_bool_t forever;

    /* 轮询等待指定 ADCINT 标志，timeout_ms < 0 表示永久等待。 */
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_EINVAL;
    }

    if (timeout_ms >= 0)
    {
        timeout = rt_tick_from_millisecond(timeout_ms);
        start = rt_tick_get();
        forever = RT_FALSE;
    }
    else
    {
        timeout = 0U;
        start = 0U;
        forever = RT_TRUE;
    }

    while (ADC_getInterruptStatus(unit->cfg->base, int_no) == false)
    {
        if ((forever == RT_FALSE) && ((rt_tick_get() - start) >= timeout))
        {
            return -RT_ETIMEOUT;
        }

        rt_thread_mdelay(1);
    }

    ns_adc_clear(unit, int_no);
    return RT_EOK;
}

rt_err_t ns_adc_once(ns_adc_unit *unit, const ns_adc_slot_cfg *slot, ADC_IntNumber int_no, rt_uint32_t *value)
{
    ns_adc_irq_cfg irq_cfg;

    /*
     * 配置并软件触发一个 SOC，然后等待该 SOC 对应的 ADCINT 完成。
     * 这是标准 adc0 单通道读取和 FinSH 调试命令的基础路径。
     */
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL) || (slot == RT_NULL) || (value == RT_NULL))
    {
        return -RT_EINVAL;
    }

    if (ns_adc_slot(unit, slot) != RT_EOK)
    {
        return -RT_EINVAL;
    }

    rt_memset(&irq_cfg, 0, sizeof(irq_cfg));
    irq_cfg.int_no = int_no;
    irq_cfg.source_soc = slot->soc;
    irq_cfg.irqn = unit->cfg->conv_irq;
    if (ns_adc_irq(unit, &irq_cfg, RT_TRUE) != RT_EOK)
    {
        return -RT_ERROR;
    }

    ns_adc_clear(unit, int_no);
    if (ns_adc_trigger(unit, ns_adc_mask(slot->soc)) != RT_EOK)
    {
        return -RT_ERROR;
    }

    if (ns_adc_wait(unit, int_no, 50) != RT_EOK)
    {
        return -RT_ETIMEOUT;
    }

    *value = ns_adc_read(unit, slot->soc);
    return RT_EOK;
}

rt_uint32_t ns_adc_read(ns_adc_unit *unit, ADC_SOCNumber soc)
{
    /* 读取指定 SOC 的 raw 结果。 */
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return 0U;
    }

    return ADC_readResult(unit->cfg->result, soc);
}

void ns_adc_clear(ns_adc_unit *unit, ADC_IntNumber int_no)
{
    /* 清理 ADCINT 标志和 overflow 标志。 */
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return;
    }

    ADC_clearInterruptStatus(unit->cfg->base, int_no);
    if (ADC_getInterruptOverflowStatus(unit->cfg->base, int_no) == true)
    {
        ADC_clearInterruptOverflowStatus(unit->cfg->base, int_no);
        ADC_clearInterruptStatus(unit->cfg->base, int_no);
    }
}

rt_uint32_t ns_adc_mask(ADC_SOCNumber soc)
{
    /* 将 SOC 编号转换为 ADC_forceMultipleSOC 使用的 bit mask。 */
    if (soc > ADC_SOC_NUMBER31)
    {
        return 0U;
    }

    return (1UL << (rt_uint32_t)soc);
}
