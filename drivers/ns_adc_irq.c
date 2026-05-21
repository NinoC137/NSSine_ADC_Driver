/**
 * @file ns_adc_irq.c
 * @brief NS800RT7P65x ADC INT1~INT4 中断薄封装。
 *
 * 文件说明：
 * 本文件只封装 ADC unit 内部 INT1~INT4 的最小配置，以及 ADCA/B/C
 * 转换完成中断入口的回调分发。业务层仍负责决定如何读取结果、释放信号量
 * 或处理 DMA/Burst/PPB 状态。
 */

#include "ns_adc_irq.h"
#include "interrupt.h"

typedef struct
{
    ns_adc_irq_cb_t callback;
} ns_adc_irq_entry_t;

static ns_adc_irq_entry_t ns_adc_irq_entries[NS_ADC_UNIT_MAX];
static ns_adc_irq_entry_t ns_adc_event_entries[NS_ADC_UNIT_MAX];

void ADCA_CONV_IRQHandler(void);
void ADCB_CONV_IRQHandler(void);
void ADCC_CONV_IRQHandler(void);

void ADCA_EVENT_IRQHandler(void);
void ADCB_EVENT_IRQHandler(void);
void ADCC_EVENT_IRQHandler(void);

static rt_bool_t ns_adc_irq_valid_unit(ns_adc_unit_id unit_id)
{
    return (unit_id < NS_ADC_UNIT_MAX) ? RT_TRUE : RT_FALSE;
}

static rt_bool_t ns_adc_irq_valid_int(ADC_IntNumber int_no)
{
    return ((rt_uint32_t)int_no < NS_ADC_IRQ_COUNT) ? RT_TRUE : RT_FALSE;
}

static void ns_adc_irq_dispatch(ns_adc_unit_id unit_id)
{
    if ((ns_adc_irq_valid_unit(unit_id) == RT_FALSE) ||
        (ns_adc_irq_entries[unit_id].callback == RT_NULL))
    {
        return;
    }

    /*
     * 回调中可能释放 semaphore 等 RT-Thread IPC 对象，因此硬件 ISR
     * 进入业务回调前必须通知内核进入中断上下文。
     */
    rt_interrupt_enter();
    ns_adc_irq_entries[unit_id].callback(unit_id);
    rt_interrupt_leave();
}

static void ns_adc_event_dispatch(ns_adc_unit_id unit_id)
{
    if ((ns_adc_irq_valid_unit(unit_id) == RT_FALSE) ||
        (ns_adc_event_entries[unit_id].callback == RT_NULL))
    {
        return;
    }

    /*
     * Event IRQ 同样可能触发 RT-Thread 同步对象，和转换完成中断保持
     * 一致的内核中断上下文管理。
     */
    rt_interrupt_enter();
    ns_adc_event_entries[unit_id].callback(unit_id);
    rt_interrupt_leave();
}

rt_err_t ns_adc_irq_attach(ns_adc_unit_id unit_id, ns_adc_irq_cb_t callback)
{
    /*
     * 注册某个 ADC unit 的转换完成回调，并将启动文件中的 ADCA/B/C
     * IRQ handler 挂到当前模块，供用户层配置和复用。
     */
    ns_adc_unit *unit;

    if ((ns_adc_irq_valid_unit(unit_id) == RT_FALSE) || (callback == RT_NULL))
    {
        return -RT_EINVAL;
    }

    unit = ns_adc_get(unit_id);
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_EINVAL;
    }

    ns_adc_irq_entries[unit_id].callback = callback;

    if (unit_id == NS_ADC_UNIT_A)
    {
        Interrupt_register(unit->cfg->conv_irq, ADCA_CONV_IRQHandler);
    }
    else if (unit_id == NS_ADC_UNIT_B)
    {
        Interrupt_register(unit->cfg->conv_irq, ADCB_CONV_IRQHandler);
    }
    else
    {
        Interrupt_register(unit->cfg->conv_irq, ADCC_CONV_IRQHandler);
    }

    NVIC_ClearPendingIRQ(unit->cfg->conv_irq);
    NVIC_EnableIRQ(unit->cfg->conv_irq);
    return RT_EOK;
}

rt_err_t ns_adc_irq_detach(ns_adc_unit_id unit_id)
{
    ns_adc_unit *unit;

    if (ns_adc_irq_valid_unit(unit_id) == RT_FALSE)
    {
        return -RT_EINVAL;
    }

    unit = ns_adc_get(unit_id);
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_EINVAL;
    }

    NVIC_DisableIRQ(unit->cfg->conv_irq);
    NVIC_ClearPendingIRQ(unit->cfg->conv_irq);
    ns_adc_irq_entries[unit_id].callback = RT_NULL;
    return RT_EOK;
}

rt_err_t ns_adc_event_attach(ns_adc_unit_id unit_id, ns_adc_irq_cb_t callback)
{
    /*
     * 注册 ADC event IRQ 回调。该入口通常服务 PPB trip/zero/OSINT 等事件，
     * 仍然属于 ADC 的中断入口，因此和 CONV handler 一起集中在本模块。
     */
    ns_adc_unit *unit;

    if ((ns_adc_irq_valid_unit(unit_id) == RT_FALSE) || (callback == RT_NULL))
    {
        return -RT_EINVAL;
    }

    unit = ns_adc_get(unit_id);
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_EINVAL;
    }

    ns_adc_event_entries[unit_id].callback = callback;

    if (unit_id == NS_ADC_UNIT_A)
    {
        Interrupt_register(unit->cfg->event_irq, ADCA_EVENT_IRQHandler);
    }
    else if (unit_id == NS_ADC_UNIT_B)
    {
        Interrupt_register(unit->cfg->event_irq, ADCB_EVENT_IRQHandler);
    }
    else
    {
        Interrupt_register(unit->cfg->event_irq, ADCC_EVENT_IRQHandler);
    }

    NVIC_ClearPendingIRQ(unit->cfg->event_irq);
    NVIC_EnableIRQ(unit->cfg->event_irq);
    return RT_EOK;
}

rt_err_t ns_adc_event_detach(ns_adc_unit_id unit_id)
{
    ns_adc_unit *unit;

    if (ns_adc_irq_valid_unit(unit_id) == RT_FALSE)
    {
        return -RT_EINVAL;
    }

    unit = ns_adc_get(unit_id);
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_EINVAL;
    }

    NVIC_DisableIRQ(unit->cfg->event_irq);
    NVIC_ClearPendingIRQ(unit->cfg->event_irq);
    ns_adc_event_entries[unit_id].callback = RT_NULL;
    return RT_EOK;
}

rt_err_t ns_adc_irq_on(ns_adc_unit_id unit_id, ADC_IntNumber int_no, ADC_SOCNumber eoc_soc)
{
    /*
     * 使能一个 ADCINTx：
     * 用户显式给出 ADCA/B/C、INT1~4 和触发该 INTx 的 EOC SOC。
     */
    ns_adc_unit *unit;

    if ((ns_adc_irq_valid_unit(unit_id) == RT_FALSE) ||
        (ns_adc_irq_valid_int(int_no) == RT_FALSE) ||
        (eoc_soc > ADC_SOC_NUMBER31))
    {
        return -RT_EINVAL;
    }

    unit = ns_adc_get(unit_id);
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_EINVAL;
    }

    ADC_setInterruptSource(unit->cfg->base, int_no, eoc_soc);
    ns_adc_clear(unit, int_no);
    ADC_disableContinuousMode(unit->cfg->base, int_no);
    ADC_enableInterrupt(unit->cfg->base, int_no);
    return RT_EOK;
}

rt_err_t ns_adc_irq_off(ns_adc_unit_id unit_id, ADC_IntNumber int_no)
{
    ns_adc_unit *unit;

    if ((ns_adc_irq_valid_unit(unit_id) == RT_FALSE) ||
        (ns_adc_irq_valid_int(int_no) == RT_FALSE))
    {
        return -RT_EINVAL;
    }

    unit = ns_adc_get(unit_id);
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_EINVAL;
    }

    ADC_disableInterrupt(unit->cfg->base, int_no);
    ns_adc_clear(unit, int_no);
    return RT_EOK;
}

rt_bool_t ns_adc_irq_pending(ns_adc_unit_id unit_id, ADC_IntNumber int_no)
{
    ns_adc_unit *unit;

    if ((ns_adc_irq_valid_unit(unit_id) == RT_FALSE) ||
        (ns_adc_irq_valid_int(int_no) == RT_FALSE))
    {
        return RT_FALSE;
    }

    unit = ns_adc_get(unit_id);
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return RT_FALSE;
    }

    return ADC_getInterruptStatus(unit->cfg->base, int_no) ? RT_TRUE : RT_FALSE;
}

void ns_adc_clear(ns_adc_unit *unit, ADC_IntNumber int_no)
{
    /* 清理指定 ADCINTx 的 flag 和 overflow flag。 */
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL) ||
        (ns_adc_irq_valid_int(int_no) == RT_FALSE))
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

rt_err_t ns_adc_wait(ns_adc_unit *unit, ADC_IntNumber int_no, rt_int32_t timeout_ms)
{
    rt_tick_t start;
    rt_tick_t timeout;
    rt_bool_t forever;

    /* 轮询等待指定 ADCINTx 标志，timeout_ms < 0 表示永久等待。 */
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL) ||
        (ns_adc_irq_valid_int(int_no) == RT_FALSE))
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

void ADCA_CONV_IRQHandler(void)
{
    ns_adc_irq_dispatch(NS_ADC_UNIT_A);
}

void ADCB_CONV_IRQHandler(void)
{
    ns_adc_irq_dispatch(NS_ADC_UNIT_B);
}

void ADCC_CONV_IRQHandler(void)
{
    ns_adc_irq_dispatch(NS_ADC_UNIT_C);
}

void ADCA_EVENT_IRQHandler(void)
{
    ns_adc_event_dispatch(NS_ADC_UNIT_A);
}

void ADCB_EVENT_IRQHandler(void)
{
    ns_adc_event_dispatch(NS_ADC_UNIT_B);
}

void ADCC_EVENT_IRQHandler(void)
{
    ns_adc_event_dispatch(NS_ADC_UNIT_C);
}
