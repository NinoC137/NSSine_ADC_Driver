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
#include "ns_adc_irq.h"
#ifdef BSP_USING_ADC0_DMA
#include "ns_adc_dma.h"
#endif

static rt_bool_t ns_adc_hw_inited = RT_FALSE;

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

static rt_err_t ns_adc_once(ns_adc_unit *unit, const ns_adc_slot_config_t *slot, ADC_IntNumber int_no, rt_uint32_t *value);

ns_adc_unit *ns_adc_get(ns_adc_unit_id id)
{
    /* 获取一个静态 ADC 单元对象，调用方无需管理其生命周期。 */
    if (id >= NS_ADC_UNIT_MAX)
    {
        return RT_NULL;
    }

    if (ns_adc_units[id].cfg == RT_NULL)
    {
        ns_adc_cfg(&ns_adc_units[id], &ns_adc_unit_cfgs[id]);
    }

    return &ns_adc_units[id];
}

rt_err_t ns_adc_cfg(ns_adc_unit *unit, const ns_adc_unit_cfg *cfg)
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

    return RT_EOK;
}

static rt_err_t ns_adc_default_slot(rt_uint32_t channel, ns_adc_slot_config_t *slot)
{
    if ((slot == RT_NULL) || (channel >= NS_ADC_CHANNEL_COUNT))
    {
        return -RT_EINVAL;
    }

    slot->soc = (ADC_SOCNumber)channel;
    slot->trigger = ADC_TRIGGER_SW_ONLY;
    slot->adc_channel = (ADC_Channel)channel;
    slot->sample_window = NS_ADC_SAMPLE_WINDOW;
    slot->int_trigger = ADC_INT_SOC_TRIGGER_NONE;
    return RT_EOK;
}

rt_err_t ns_adc_hw_init(void)
{
    ns_adc_unit *unit;

    if (ns_adc_hw_inited == RT_TRUE)
    {
        return RT_EOK;
    }

    unit = ns_adc_get(NS_ADC_UNIT_A);
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_ERROR;
    }

    ns_adc_hw_inited = RT_TRUE;
    return RT_EOK;
}

rt_err_t ns_adc_hw_reset(void)
{
    ns_adc_unit *unit;
    rt_uint32_t i;

    for (i = 0U; i < NS_ADC_UNIT_MAX; i++)
    {
        unit = ns_adc_get((ns_adc_unit_id)i);
        if ((unit != RT_NULL) && (unit->cfg != RT_NULL))
        {
#ifdef BSP_USING_ADC0_DMA
            ns_adc_dma_stop(unit);
#endif
            ns_adc_cfg(unit, unit->cfg);
        }
    }

    ns_adc_hw_inited = RT_FALSE;
    return ns_adc_hw_init();
}

rt_err_t ns_adc_read_channel_once(rt_uint32_t channel, rt_uint32_t *value)
{
    ns_adc_unit *unit;
    ns_adc_slot_config_t slot;

    if ((value == RT_NULL) || (channel >= NS_ADC_CHANNEL_COUNT))
    {
        return -RT_EINVAL;
    }

    if (ns_adc_hw_init() != RT_EOK)
    {
        return -RT_ERROR;
    }

    unit = ns_adc_get(NS_ADC_UNIT_A);
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_EINVAL;
    }

    if (ns_adc_default_slot(channel, &slot) != RT_EOK)
    {
        return -RT_EINVAL;
    }

    return ns_adc_once(unit, &slot, ADC_INT_NUMBER1, value);
}

/**
 * ns_adc_slot_cfg() : 配置一个 SOC 槽位
 * 启动一个channel的ADC采样
 * 配置流程：
 * 1. 选定一个adc channel，与硬件外部io唯一对应
 * 2. 为这个channel指定一个硬件内部SOC
 * 3. 开启采样流程、配置中断
 */
rt_err_t ns_adc_slot_cfg(ns_adc_unit *unit, const ns_adc_slot_config_t *cfg)
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
    return RT_EOK;
}

rt_err_t ns_adc_burst_cfg(ns_adc_unit *unit, const ns_adc_burst_config_t *cfg)
{
    /* 配置 ADC Burst 模式，具体 SOC 序列仍由 ns_adc_slot_cfg() 提前设置。 */
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL) || (cfg == RT_NULL) || (cfg->burst_size == 0U))
    {
        return -RT_EINVAL;
    }

    ADC_enableBurstMode(unit->cfg->base);
    ADC_setBurstModeConfig(unit->cfg->base, cfg->trigger, cfg->burst_size);
    ADC_setSOCPriority(unit->cfg->base, cfg->priority);
    return RT_EOK;
}

rt_err_t ns_adc_stop(ns_adc_unit *unit)
{
    /* 停止该 ADC 单元使用的 IRQ 和 Burst 模式。 */
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_EINVAL;
    }

    NVIC_DisableIRQ(unit->cfg->conv_irq);
    NVIC_ClearPendingIRQ(unit->cfg->conv_irq);
    NVIC_DisableIRQ(unit->cfg->event_irq);
    NVIC_ClearPendingIRQ(unit->cfg->event_irq);

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

/**
 * ADC Channel单次读取api
 * 实现“配置+触发+等待+读取一个slot”的完整流程
 */
static rt_err_t ns_adc_once(ns_adc_unit *unit, const ns_adc_slot_config_t *slot, ADC_IntNumber int_no, rt_uint32_t *value)
{
    /*
     * 这是标准 adc0 单通道读取和 FinSH 调试命令的基础路径。
     */
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL) || (slot == RT_NULL) || (value == RT_NULL))
    {
        return -RT_EINVAL;
    }

    if (ns_adc_slot_cfg(unit, slot) != RT_EOK)
    {
        return -RT_EINVAL;
    }

    if (ns_adc_irq_on(unit->cfg->id, int_no, slot->soc) != RT_EOK)
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

rt_uint32_t ns_adc_mask(ADC_SOCNumber soc)
{
    /* 将 SOC 编号转换为 ADC_forceMultipleSOC 使用的 bit mask。 */
    if (soc > ADC_SOC_NUMBER31)
    {
        return 0U;
    }

    return (1UL << (rt_uint32_t)soc);
}

rt_uint32_t ns_adc_soc_mask(ADC_SOCNumber first_soc, rt_uint32_t count)
{
    rt_uint32_t i;
    rt_uint32_t first = (rt_uint32_t)first_soc;
    rt_uint32_t mask = 0U;

    /* 生成从 first_soc 开始、连续 count 个 SOC 的触发 mask。 */
    if ((first_soc > ADC_SOC_NUMBER31) || (count == 0U) || ((first + count) > NS_ADC_UNIT_SOC_MAX))
    {
        return 0U;
    }

    for (i = 0U; i < count; i++)
    {
        mask |= ns_adc_mask((ADC_SOCNumber)(first + i));
    }

    return mask;
}
