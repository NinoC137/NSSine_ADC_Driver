/**
 * @file drv_adc_ns800rt7p65x_ppb.c
 * @brief NS800RT7P65x ADC PPB 后处理模块适配实现。
 *
 * 文件说明：
 * PPB(Post Processing Block) 用于对指定 SOC 的结果进行后处理，
 * 例如高低限比较、事件标志记录和转换延迟时间戳读取。
 * 本文件只处理 PPB 寄存器配置和事件状态读写，不持有 nsadc0 设备状态。
 */

#include "drv_adc_ns800rt7p65x_ppb.h"

#if defined(BSP_USING_ADC0_ADVANCED) && defined(BSP_USING_ADC0_PPB)

void ADCA_EVENT_IRQHandler(void);
void ADCB_EVENT_IRQHandler(void);
void ADCC_EVENT_IRQHandler(void);

static void ns_adc_ppb_irq(IRQn_Type irqn, void (*handler)(void))
{
    /*
     * 仅挂接 PPB event IRQ 自身，不重新初始化整张向量表，
     * 避免影响 RT-Thread 已经建立好的异常/中断入口。
     */
    Interrupt_register(irqn, handler);
    NVIC_ClearPendingIRQ(irqn);
    NVIC_EnableIRQ(irqn);
}

static void ns_adc_ppb_event_irq(ns_adc_unit *unit)
{
    /* 根据 ADC unit 选择对应 PPB EVENT IRQ handler。 */
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return;
    }

    if (unit->cfg->event_irq == ADCA_EVENT_IRQn)
    {
        ns_adc_ppb_irq(ADCA_EVENT_IRQn, ADCA_EVENT_IRQHandler);
    }
    else if (unit->cfg->event_irq == ADCB_EVENT_IRQn)
    {
        ns_adc_ppb_irq(ADCB_EVENT_IRQn, ADCB_EVENT_IRQHandler);
    }
    else if (unit->cfg->event_irq == ADCC_EVENT_IRQn)
    {
        ns_adc_ppb_irq(ADCC_EVENT_IRQn, ADCC_EVENT_IRQHandler);
    }
}

rt_err_t ns_adc_ppb_limit(const ns_adc_ppb_limit_config_t *cfg)
{
    ns_adc_unit *unit;
    ns_adc_ppb_cfg ppb_cfg;

    /*
     * 将用户显式给出的 ADC unit/SOC/PPB 组合写入硬件。
     * PPB 是 unit 内部资源，不再固定为 PPB1。
     */
    if ((cfg == RT_NULL) ||
        (cfg->unit_id >= NS_ADC_UNIT_MAX) ||
        (cfg->ppb > ADC_PPB_NUMBER4) ||
        (cfg->soc > ADC_SOC_NUMBER31))
    {
        return -RT_EINVAL;
    }

    unit = ns_adc_get(cfg->unit_id);
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_EINVAL;
    }

    rt_memset(&ppb_cfg, 0, sizeof(ppb_cfg));
    ppb_cfg.ppb = cfg->ppb;
    ppb_cfg.soc = cfg->soc;
    ppb_cfg.high_limit = cfg->high_limit;
    ppb_cfg.low_limit = cfg->low_limit;
    ppb_cfg.event_mask = NS_ADC_EVT_TRIPHI | NS_ADC_EVT_TRIPLO | NS_ADC_EVT_ZERO;
    ppb_cfg.event_irq = cfg->enable_event_irq;

    if (ns_adc_ppb(unit, &ppb_cfg) != RT_EOK)
    {
        return -RT_ERROR;
    }

    if (cfg->enable_event_irq == RT_TRUE)
    {
        ns_adc_ppb_event_irq(unit);
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

    ppb = unit->ppb.ppb;
    if (status != RT_NULL)
    {
        status->event_flags |= ADC_getPPBEventStatus(unit->cfg->base, ppb);
        status->delay_stamp = ADC_getPPBDelayTimeStamp(unit->cfg->base, ppb);
    }

    ADC_clearPPBEventStatus(unit->cfg->base,
                            ppb,
                            NS_ADC_EVT_TRIPHI | NS_ADC_EVT_TRIPLO | NS_ADC_EVT_ZERO);
}

void ns_adc_ppb_status_init(ns_adc_ppb_status_t *status)
{
    /* 清空 PPB 事件累计状态，通常在重新配置 PPB 前调用。 */
    if (status == RT_NULL)
    {
        return;
    }

    status->event_flags = 0U;
    status->delay_stamp = 0U;
}

#endif
