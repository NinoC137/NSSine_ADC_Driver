/**
 * @file drv_adc_ns800rt7p65x_adv.c
 * @brief NS800RT7P65x ADC 高级伴生设备 nsadc0。
 *
 * 文件说明：
 * RT-Thread v4.1.x 官方 ADC device framework 只定义 enable/convert/vref
 * 等同步采样接口，不适合直接承载中断、DMA、Burst 和 PPB 控制。
 * 本文件额外注册一个 Misc 设备 nsadc0：标准应用继续使用 adc0；
 * 高级实验通过 nsadc0 的 rt_device_control/read 接口完成。
 */

#include "drv_adc_ns800rt7p65x_adv.h"

#ifdef BSP_USING_ADC0_ADVANCED

#include "drv_adc_ns800rt7p65x_common.h"
#include "drv_adc_ns800rt7p65x_ppb.h"
#include "v0.4.0/drivers/ns800rt7p65x/inc/interrupt.h"

#define NS_ADC_ADV_WAIT_MS 1000U

#ifdef BSP_USING_ADC0_DMA
#include "v0.4.0/drivers/ns800rt7p65x/inc/dmamux.h"
#include "v0.4.0/drivers/ns800rt7p65x/inc/edma.h"
#endif

#ifdef BSP_USING_ADC0_BURST
#include "v0.4.0/drivers/ns800rt7p65x/inc/epwm.h"
#include "v0.4.0/drivers/ns800rt7p65x/inc/syscon.h"
#endif

#undef __ADC_H__

void ADCA_CONV_IRQHandler(void);
void ADCB_CONV_IRQHandler(void);
void ADCC_CONV_IRQHandler(void);
void ADCA_EVENT_IRQHandler(void);
void ADCB_EVENT_IRQHandler(void);
void ADCC_EVENT_IRQHandler(void);
void EDMA1_CH0_IRQHandler(void);

typedef struct
{
    struct rt_device parent;
    struct rt_mutex lock;
    struct rt_semaphore done;
    ns_adc_adv_mode_t mode;
    rt_bool_t running;
    rt_uint32_t ready_count;
    rt_uint32_t write_index;
    ns_adc_adv_sample_t samples[NS_ADC_ADV_BUFFER_MAX];
    ns_adc_adv_irq_config_t irq_cfg;
    ns_adc_adv_dma_config_t dma_cfg;
    ns_adc_adv_burst_config_t burst_cfg;
    ns_adc_adv_burst_dma_config_t burst_dma_cfg;
#ifdef BSP_USING_ADC0_PPB
    ns_adc_ppb_ctx_t ppb;
#endif
} ns_adc_adv_device_t;

static ns_adc_adv_device_t ns_adc_adv;
#define ctx (&ns_adc_adv)

#ifdef BSP_USING_ADC0_DMA
static volatile rt_uint16_t ns_adc_dma_buffer[NS_ADC_ADV_BUFFER_MAX] __attribute__((aligned(32)));
#endif

static rt_uint32_t ns_adc_adv_clamp_count(rt_uint32_t count)
{
    /* 将用户配置的样本数量限制在本 demo 固定缓冲区容量内。 */
    if (count == 0U)
    {
        return 1U;
    }

    if (count > NS_ADC_ADV_BUFFER_MAX)
    {
        return NS_ADC_ADV_BUFFER_MAX;
    }

    return count;
}

static ns_adc_adv_device_t *ns_adc_adv_get(rt_device_t dev)
{
    /* 从 RT-Thread device 取回高级设备上下文；ISR 路径则使用当前单实例。 */
    if ((dev == RT_NULL) || (dev->user_data == RT_NULL))
    {
        return ctx;
    }

    return (ns_adc_adv_device_t *)dev->user_data;
}

static void ns_adc_adv_reset_samples(void)
{
    /* 清空采样缓冲区和状态计数，通常在启动某个高级模式前调用。 */
    rt_memset(ctx->samples, 0, sizeof(ctx->samples));
    ctx->ready_count = 0U;
    ctx->write_index = 0U;
    rt_sem_control(&ctx->done, RT_IPC_CMD_RESET, RT_NULL);
}

static void ns_adc_adv_push_sample(ns_adc_unit_id unit_id, ADC_SOCNumber soc, ADC_Channel adc_channel, rt_uint32_t raw)
{
    /* ISR/DMA 完成路径写入固定环形缓冲，并通过 rx_indicate 通知上层有数据。 */
    ctx->samples[ctx->write_index].unit_id = unit_id;
    ctx->samples[ctx->write_index].soc = soc;
    ctx->samples[ctx->write_index].adc_channel = adc_channel;
    ctx->samples[ctx->write_index].raw = raw;
    ctx->write_index = (ctx->write_index + 1U) % NS_ADC_ADV_BUFFER_MAX;

    if (ctx->ready_count < NS_ADC_ADV_BUFFER_MAX)
    {
        ctx->ready_count++;
    }

    if (ctx->parent.rx_indicate != RT_NULL)
    {
        ctx->parent.rx_indicate(&ctx->parent, ctx->ready_count);
    }
}

static rt_err_t ns_adc_adv_wait_done(rt_uint32_t timeout_ms)
{
    /* 高级模式等待完成事件，超时后返回错误，避免 FinSH 命令静默失败或永久阻塞。 */
    rt_err_t ret = rt_sem_take(&ctx->done, rt_tick_from_millisecond(timeout_ms));

    if (ret != RT_EOK)
    {
        ctx->running = RT_FALSE;
        return -RT_ETIMEOUT;
    }

    return RT_EOK;
}

static rt_err_t ns_adc_adv_check_slot(const ns_adc_slot_cfg *slot)
{
    if ((slot == RT_NULL) || (slot->soc > ADC_SOC_NUMBER31) || (slot->adc_channel > ADC_CH_ADCIN31))
    {
        return -RT_EINVAL;
    }

    return RT_EOK;
}

static rt_err_t ns_adc_adv_copy_sequence(ns_adc_adv_sequence_config_t *dst,
                                         const ns_adc_adv_sequence_config_t *src)
{
    rt_uint32_t i;

    /* 校验并保存一个通道序列；序列用于中断模式和软件触发批量采样。 */
    if ((dst == RT_NULL) || (src == RT_NULL))
    {
        return -RT_EINVAL;
    }

    if ((src->unit_id >= NS_ADC_UNIT_MAX) ||
        (src->slot_count == 0U) ||
        (src->slot_count > NS_ADC_ADV_BUFFER_MAX))
    {
        return -RT_EINVAL;
    }

    for (i = 0U; i < src->slot_count; i++)
    {
        if (ns_adc_adv_check_slot(&src->slots[i]) != RT_EOK)
        {
            return -RT_EINVAL;
        }
    }

    rt_memcpy(dst, src, sizeof(*dst));
    dst->sample_count = ns_adc_adv_clamp_count(src->sample_count);
    if (dst->eoc_int == 0U)
    {
        dst->eoc_int = ADC_INT_NUMBER1;
    }
    return RT_EOK;
}

static void ns_adc_adv_enable_irq(IRQn_Type irqn, void (*handler)(void))
{
    /*
     * RT-Thread 启动文件已经建立向量表。
     * 这里仅使用 SDK 的 Interrupt_register/NVIC_EnableIRQ 挂接单个 IRQ，
     * 不调用 Interrupt_initModule/Interrupt_initVectorTable，避免覆盖内核向量。
     */
    Interrupt_register(irqn, handler);
    NVIC_ClearPendingIRQ(irqn);
    NVIC_EnableIRQ(irqn);
}

static void ns_adc_adv_enable_unit_irq(ns_adc_unit *unit)
{
    /* 根据 ADC unit 选择对应的转换完成 IRQ。 */
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return;
    }

    if (unit->cfg->conv_irq == ADCA_CONV_IRQn)
    {
        ns_adc_adv_enable_irq(ADCA_CONV_IRQn, ADCA_CONV_IRQHandler);
    }
    else if (unit->cfg->conv_irq == ADCB_CONV_IRQn)
    {
        ns_adc_adv_enable_irq(ADCB_CONV_IRQn, ADCB_CONV_IRQHandler);
    }
    else if (unit->cfg->conv_irq == ADCC_CONV_IRQn)
    {
        ns_adc_adv_enable_irq(ADCC_CONV_IRQn, ADCC_CONV_IRQHandler);
    }
}

static void ns_adc_adv_disable_irq(IRQn_Type irqn)
{
    /* 停止高级模式时只关闭本驱动使用过的外设 IRQ。 */
    NVIC_DisableIRQ(irqn);
    NVIC_ClearPendingIRQ(irqn);
}

#ifdef BSP_USING_ADC0_BURST
static void ns_adc_adv_epwm_start(rt_uint32_t period, rt_uint32_t compare)
{
    /*
     * 配置 EPWM1 SOCA 作为 Burst/PPB 的默认触发源。
     * period/compare 使用 SDK 示例中的语义，0 值则退回示例默认参数。
     */
    if (period == 0U)
    {
        period = 2599U;
    }
    if (compare == 0U)
    {
        compare = period / 2U;
    }

    SYSCON_UNLOCK;
    SYSCON_setTbClkSync(SYSCON, false);
    SYSCON_LOCK;

    EPWM_disableADCTrigger(EPWM1, EPWM_SOC_A);
    EPWM_setADCTriggerSource(EPWM1, EPWM_SOC_A, EPWM_SOC_TBCTR_U_CMPA);
    EPWM_setADCTriggerEventPrescale(EPWM1, EPWM_SOC_A, 1U);
    EPWM_setTimeBasePeriod(EPWM1, (rt_uint16_t)period);
    EPWM_setCounterCompareValue(EPWM1, EPWM_COUNTER_COMPARE_A, (rt_uint16_t)compare);
    EPWM_setClockPrescaler(EPWM1, EPWM_CLOCK_DIVIDER_2, EPWM_HSCLOCK_DIVIDER_1);
    EPWM_setTimeBaseCounterMode(EPWM1, EPWM_COUNTER_MODE_UP_DOWN);
    EPWM_enableADCTrigger(EPWM1, EPWM_SOC_A);

    SYSCON_UNLOCK;
    SYSCON_setEpwmSocAEn(SYSCON, SYSCON_EPWM1SOCAEN, true);
    SYSCON_setTbClkSync(SYSCON, true);
    SYSCON_LOCK;
}

static void ns_adc_adv_epwm_stop(void)
{
    /* 关闭 EPWM1 SOCA 触发，并冻结计数器，防止高级模式停止后继续触发 ADC。 */
    EPWM_disableADCTrigger(EPWM1, EPWM_SOC_A);
    EPWM_setTimeBaseCounterMode(EPWM1, EPWM_COUNTER_MODE_STOP_FREEZE);
}
#endif

static rt_err_t ns_adc_adv_config_irq(const ns_adc_adv_irq_config_t *cfg)
{
#ifndef BSP_USING_ADC0_IRQ
    RT_UNUSED(cfg);
    return -RT_ENOSYS;
#else
    rt_err_t ret;

    ret = ns_adc_adv_copy_sequence(&ctx->irq_cfg.sequence, &cfg->sequence);
    if (ret != RT_EOK)
    {
        return ret;
    }

    ctx->mode = NS_ADC_ADV_MODE_IRQ;
    return RT_EOK;
#endif
}

static rt_err_t ns_adc_adv_start_irq(void)
{
#ifndef BSP_USING_ADC0_IRQ
    return -RT_ENOSYS;
#else
    const ns_adc_adv_sequence_config_t *seq = &ctx->irq_cfg.sequence;
    ns_adc_unit *unit;
    ns_adc_seq_cfg seq_cfg;
    rt_uint32_t i;

    /*
     * 中断模式采用软件触发批量 SOC：
     * 逐通道 force SOC，ADCINT1 由硬件置位后进入 IRQ handler，
     * handler 读取当前通道结果并释放完成信号。
     */
    ns_adc_adv_reset_samples();
    ctx->running = RT_TRUE;
    ctx->mode = NS_ADC_ADV_MODE_IRQ;
    ns_adc_hw_init();

    unit = ns_adc_get(seq->unit_id);
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_EINVAL;
    }

    seq_cfg.unit_id = seq->unit_id;
    seq_cfg.slots = seq->slots;
    seq_cfg.slot_count = seq->slot_count;
    seq_cfg.eoc_int = seq->eoc_int;
    if (ns_adc_seq(&seq_cfg) != RT_EOK)
    {
        return -RT_EINVAL;
    }

    ns_adc_adv_enable_unit_irq(unit);
    for (i = 0U; i < seq->sample_count; i++)
    {
        const ns_adc_slot_cfg *slot = &seq->slots[i % seq->slot_count];

        ADC_setInterruptSource(unit->cfg->base, seq->eoc_int, slot->soc);
        ns_adc_clear(unit, seq->eoc_int);
        ns_adc_trigger(unit, ns_adc_force_mask(slot->soc));
        if (ns_adc_adv_wait_done(NS_ADC_ADV_WAIT_MS) != RT_EOK)
        {
            return -RT_ETIMEOUT;
        }
    }

    ctx->running = RT_FALSE;
    return RT_EOK;
#endif
}

static rt_err_t ns_adc_adv_config_dma(const ns_adc_adv_dma_config_t *cfg)
{
#ifndef BSP_USING_ADC0_DMA
    RT_UNUSED(cfg);
    return -RT_ENOSYS;
#else
    if ((cfg == RT_NULL) ||
        (cfg->unit_id >= NS_ADC_UNIT_MAX) ||
        (cfg->soc > ADC_SOC_NUMBER31) ||
        (cfg->adc_channel > ADC_CH_ADCIN31))
    {
        return -RT_EINVAL;
    }

    ctx->dma_cfg = *cfg;
    ctx->dma_cfg.sample_count = ns_adc_adv_clamp_count(cfg->sample_count);
    if (ctx->dma_cfg.edma_channel == 0U)
    {
        ctx->dma_cfg.edma_channel = 0U;
    }

    ctx->mode = NS_ADC_ADV_MODE_DMA;
    return RT_EOK;
#endif
}

static rt_err_t ns_adc_adv_start_dma(void)
{
#ifndef BSP_USING_ADC0_DMA
    return -RT_ENOSYS;
#else
    ns_adc_unit *unit;
    ns_adc_slot_cfg slot;
    EDMA_CommonConfig edma_config = {0};
    EDMA_TransferConfig transfer = {0};
    rt_uint32_t count;

    /*
     * DMA 模式参考 SDK adc_ex6：ADC 结果寄存器作为源地址，EDMA1 CH0
     * 将半字结果搬入本地对齐缓冲，major loop 完成后由 EDMA ISR 汇总。
     */
    unit = ns_adc_get(ctx->dma_cfg.unit_id);
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_EINVAL;
    }

    rt_memset(&slot, 0, sizeof(slot));
    slot.soc = ctx->dma_cfg.soc;
    slot.trigger = ADC_TRIGGER_SW_ONLY;
    slot.adc_channel = ctx->dma_cfg.adc_channel;
    slot.sample_window = NS_ADC_SAMPLE_WINDOW;
    if (ns_adc_slot(unit, &slot) != RT_EOK)
    {
        return -RT_EINVAL;
    }

    count = ns_adc_adv_clamp_count(ctx->dma_cfg.sample_count);
    ns_adc_adv_reset_samples();
    rt_memset((void *)ns_adc_dma_buffer, 0, sizeof(ns_adc_dma_buffer));
    ctx->running = RT_TRUE;
    ctx->mode = NS_ADC_ADV_MODE_DMA;

    ns_adc_hw_init();
    EDMA_initialize(EDMA1);

    edma_config.enableHaltOnError = true;
    transfer.channel = ctx->dma_cfg.edma_channel;
    transfer.srcAddr = (uint32_t)unit->cfg->result;
    transfer.destAddr = EDMA_getAbsAddrForMultiCore((uint32_t)(void *)ns_adc_dma_buffer);
    transfer.srcTransferSize = EDMA_TRANSFERSIZE2BYTES;
    transfer.destTransferSize = EDMA_TRANSFERSIZE2BYTES;
    transfer.destOffset = 2;
    transfer.minorLoopBytes = 2;
    transfer.majorLoopCounts = count;
    transfer.dlast = -((int32_t)count * 2);
    transfer.enMajorInt = true;
    transfer.startMode = true;

    DMAMUX_configModule(DMAMUX1);
    DMAMUX_setSource(DMAMUX1, (uint8_t)transfer.channel, unit->cfg->dma_req[0]);
    DMAMUX_enableChannel(DMAMUX1, (uint8_t)transfer.channel);

    ADC_setInterruptSource(unit->cfg->base, ADC_INT_NUMBER1, ctx->dma_cfg.soc);
    ns_adc_clear(unit, ADC_INT_NUMBER1);
    ns_adc_adv_enable_unit_irq(unit);
    ns_adc_adv_enable_irq(EDMA1_CH0_IRQn, EDMA1_CH0_IRQHandler);

    EDMA_configModule(EDMA1, &edma_config);
    EDMA_configChannel(EDMA1, &transfer);
    EDMA_startTransfer(EDMA1, (uint8_t)transfer.channel);

    for (rt_uint32_t i = 0U; i < count; i++)
    {
        ns_adc_trigger(unit, ns_adc_force_mask(ctx->dma_cfg.soc));
    }

    if (ns_adc_adv_wait_done(NS_ADC_ADV_WAIT_MS) != RT_EOK)
    {
        return -RT_ETIMEOUT;
    }

    ctx->running = RT_FALSE;
    return RT_EOK;
#endif
}

static rt_err_t ns_adc_adv_config_burst(const ns_adc_adv_burst_config_t *cfg)
{
#ifndef BSP_USING_ADC0_BURST
    RT_UNUSED(cfg);
    return -RT_ENOSYS;
#else
    rt_uint32_t i;

    if ((cfg == RT_NULL) ||
        (cfg->unit_id >= NS_ADC_UNIT_MAX) ||
        (cfg->first_soc > ADC_SOC_NUMBER31) ||
        (cfg->channel_count == 0U) ||
        (cfg->channel_count > NS_ADC_ADV_BUFFER_MAX))
    {
        return -RT_EINVAL;
    }

    for (i = 0U; i < cfg->channel_count; i++)
    {
        if (cfg->adc_channels[i] > ADC_CH_ADCIN31)
        {
            return -RT_EINVAL;
        }
    }

    ctx->burst_cfg = *cfg;
    ctx->burst_cfg.burst_size = ns_adc_adv_clamp_count(cfg->burst_size);
    if (((rt_uint32_t)ctx->burst_cfg.first_soc + ctx->burst_cfg.burst_size) > NS_ADC_UNIT_SOC_MAX)
    {
        return -RT_EINVAL;
    }
    if (ctx->burst_cfg.trigger == 0U)
    {
        ctx->burst_cfg.trigger = ADC_BURSTTRIG5_EPWM1_SOCA;
    }
    ctx->mode = NS_ADC_ADV_MODE_BURST;
    return RT_EOK;
#endif
}

static rt_err_t ns_adc_adv_start_burst(void)
{
#ifndef BSP_USING_ADC0_BURST
    return -RT_ENOSYS;
#else
    const ns_adc_adv_burst_config_t *cfg = &ctx->burst_cfg;
    ns_adc_unit *unit;
    ns_adc_burst_cfg burst_cfg;
    ns_adc_slot_cfg slot;
    rt_uint32_t i;

    /*
     * Burst 模式使用显式 unit + first_soc。
     * 将用户配置的 ADCIN 展开到 first_soc 起始的一组 SOC，ADCINT1 绑定到
     * 本次 burst 的最后一个 SOC，ISR 中批量读取结果。
     */
    if ((cfg->channel_count == 0U) || (cfg->burst_size == 0U))
    {
        return -RT_EINVAL;
    }

    ns_adc_adv_reset_samples();
    ctx->running = RT_TRUE;
    ctx->mode = NS_ADC_ADV_MODE_BURST;

    unit = ns_adc_get(cfg->unit_id);
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_EINVAL;
    }

    for (i = 0U; i < cfg->burst_size; i++)
    {
        rt_memset(&slot, 0, sizeof(slot));
        slot.soc = (ADC_SOCNumber)((rt_uint32_t)cfg->first_soc + i);
        slot.trigger = ADC_TRIGGER_SW_ONLY;
        slot.adc_channel = cfg->adc_channels[i % cfg->channel_count];
        slot.sample_window = NS_ADC_SAMPLE_WINDOW;
        if (ns_adc_slot(unit, &slot) != RT_EOK)
        {
            return -RT_EINVAL;
        }
    }

    burst_cfg.trigger = cfg->trigger;
    burst_cfg.burst_size = cfg->burst_size;
    burst_cfg.priority = ADC_PRI_THRU_SOC6_HIPRI;
    if (ns_adc_burst(unit, &burst_cfg) != RT_EOK)
    {
        return -RT_ERROR;
    }

    ADC_setInterruptSource(unit->cfg->base,
                           ADC_INT_NUMBER1,
                           (ADC_SOCNumber)((rt_uint32_t)cfg->first_soc + cfg->burst_size - 1U));
    ADC_clearInterruptStatus(unit->cfg->base, ADC_INT_NUMBER1);
    ADC_enableInterrupt(unit->cfg->base, ADC_INT_NUMBER1);

    ns_adc_adv_enable_unit_irq(unit);
    ns_adc_adv_epwm_start(cfg->epwm_period, cfg->epwm_compare);
    if (ns_adc_adv_wait_done(NS_ADC_ADV_WAIT_MS) != RT_EOK)
    {
        return -RT_ETIMEOUT;
    }

    ctx->running = RT_FALSE;
    return RT_EOK;
#endif
}

static rt_err_t ns_adc_adv_config_burst_dma(const ns_adc_adv_burst_dma_config_t *cfg)
{
#if !defined(BSP_USING_ADC0_BURST) || !defined(BSP_USING_ADC0_DMA)
    RT_UNUSED(cfg);
    return -RT_ENOSYS;
#else
    rt_err_t ret;

    if (cfg == RT_NULL)
    {
        return -RT_EINVAL;
    }

    ret = ns_adc_adv_config_burst(&cfg->burst);
    if (ret != RT_EOK)
    {
        return ret;
    }

    ctx->burst_dma_cfg = *cfg;
    ctx->mode = NS_ADC_ADV_MODE_BURST_DMA;
    return RT_EOK;
#endif
}

static rt_err_t ns_adc_adv_config_ppb_limit(const ns_adc_ppb_limit_config_t *cfg)
{
#ifndef BSP_USING_ADC0_PPB
    RT_UNUSED(cfg);
    return -RT_ENOSYS;
#else
    rt_err_t ret;

    /* PPB 具体寄存器配置由独立 PPB 模块完成，设备层只保存状态。 */
    ret = ns_adc_ppb_limit(cfg);
    if (ret != RT_EOK)
    {
        return ret;
    }

    ctx->ppb.limit = *cfg;
    ns_adc_ppb_status_init(&ctx->ppb.status);
    ctx->mode = NS_ADC_ADV_MODE_PPB_LIMIT;
    return RT_EOK;
#endif
}

static rt_err_t ns_adc_adv_get_ppb_status(ns_adc_ppb_status_t *status)
{
#ifndef BSP_USING_ADC0_PPB
    RT_UNUSED(status);
    return -RT_ENOSYS;
#else
    /* 返回 PPB 事件和 delay timestamp。 */
    if (status == RT_NULL)
    {
        return -RT_EINVAL;
    }

    *status = ctx->ppb.status;
    return RT_EOK;
#endif
}

static void ns_adc_adv_fill_status(ns_adc_adv_status_t *status)
{
    /* 填充 nsadc0 通用状态；可选模块状态由各自模块单独查询。 */
    status->mode = ctx->mode;
    status->running = ctx->running;
    status->sample_count = NS_ADC_ADV_BUFFER_MAX;
    status->ready_count = ctx->ready_count;
}

static rt_err_t ns_adc_adv_stop(void)
{
    /*
     * 停止所有高级模式并恢复 ADC 基础配置。
     * 注意这里只关闭本驱动显式启用的 IRQ/触发源，不影响 RT-Thread 内核。
     */
    ctx->running = RT_FALSE;
    ctx->mode = NS_ADC_ADV_MODE_IDLE;

    ns_adc_adv_disable_irq(ADCA_CONV_IRQn);
    ns_adc_adv_disable_irq(ADCB_CONV_IRQn);
    ns_adc_adv_disable_irq(ADCC_CONV_IRQn);
    ns_adc_adv_disable_irq(ADCA_EVENT_IRQn);
    ns_adc_adv_disable_irq(ADCB_EVENT_IRQn);
    ns_adc_adv_disable_irq(ADCC_EVENT_IRQn);

#ifdef BSP_USING_ADC0_DMA
    ns_adc_adv_disable_irq(EDMA1_CH0_IRQn);
    DMAMUX_disableChannel(DMAMUX1, (uint8_t)ctx->dma_cfg.edma_channel);
    EDMA_disableChannelRequest(EDMA1, (uint8_t)ctx->dma_cfg.edma_channel);
#endif

#ifdef BSP_USING_ADC0_BURST
    ns_adc_adv_epwm_stop();
#endif

    ns_adc_hw_reset();
    return RT_EOK;
}

static rt_err_t ns_adc_adv_init(rt_device_t dev)
{
    RT_UNUSED(ns_adc_adv_get(dev));
    ns_adc_hw_init();
    return RT_EOK;
}

static rt_err_t ns_adc_adv_open(rt_device_t dev, rt_uint16_t oflag)
{
    RT_UNUSED(ns_adc_adv_get(dev));
    RT_UNUSED(oflag);
    return RT_EOK;
}

static rt_err_t ns_adc_adv_close(rt_device_t dev)
{
    RT_UNUSED(ns_adc_adv_get(dev));
    return ns_adc_adv_stop();
}

static rt_size_t ns_adc_adv_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    rt_size_t count;

    /*
     * 从 nsadc0 读取最近采样结果。
     * 当前实现为固定快照读取，不消费环形缓冲；便于 FinSH 和调试器重复查看。
     */
    RT_UNUSED(ns_adc_adv_get(dev));
    RT_UNUSED(pos);

    if ((buffer == RT_NULL) || (size == 0U))
    {
        return 0U;
    }

    count = ctx->ready_count;
    if (count > size)
    {
        count = size;
    }

    rt_memcpy(buffer, ctx->samples, count * sizeof(ns_adc_adv_sample_t));
    return count;
}

static rt_size_t ns_adc_adv_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    RT_UNUSED(dev);
    RT_UNUSED(pos);
    RT_UNUSED(buffer);
    RT_UNUSED(size);
    return 0U;
}

static rt_err_t ns_adc_adv_control(rt_device_t dev, int cmd, void *args)
{
    rt_err_t ret;

    RT_UNUSED(ns_adc_adv_get(dev));
    rt_mutex_take(&ctx->lock, RT_WAITING_FOREVER);

    switch (cmd)
    {
    case NS_ADC_ADV_CMD_CONFIG_IRQ:
        ret = ns_adc_adv_config_irq((const ns_adc_adv_irq_config_t *)args);
        break;
    case NS_ADC_ADV_CMD_START_IRQ:
        ret = ns_adc_adv_start_irq();
        break;
    case NS_ADC_ADV_CMD_CONFIG_DMA:
        ret = ns_adc_adv_config_dma((const ns_adc_adv_dma_config_t *)args);
        break;
    case NS_ADC_ADV_CMD_START_DMA:
        ret = ns_adc_adv_start_dma();
        break;
    case NS_ADC_ADV_CMD_CONFIG_BURST:
        ret = ns_adc_adv_config_burst((const ns_adc_adv_burst_config_t *)args);
        break;
    case NS_ADC_ADV_CMD_START_BURST:
        ret = ns_adc_adv_start_burst();
        break;
    case NS_ADC_ADV_CMD_CONFIG_BURST_DMA:
        ret = ns_adc_adv_config_burst_dma((const ns_adc_adv_burst_dma_config_t *)args);
        break;
    case NS_ADC_ADV_CMD_CONFIG_PPB_LIMIT:
        ret = ns_adc_adv_config_ppb_limit((const ns_adc_ppb_limit_config_t *)args);
        break;
    case NS_ADC_ADV_CMD_GET_PPB_STATUS:
        ret = ns_adc_adv_get_ppb_status((ns_adc_ppb_status_t *)args);
        break;
    case NS_ADC_ADV_CMD_GET_STATUS:
        if (args == RT_NULL)
        {
            ret = -RT_EINVAL;
        }
        else
        {
            ns_adc_adv_fill_status((ns_adc_adv_status_t *)args);
            ret = RT_EOK;
        }
        break;
    case NS_ADC_ADV_CMD_STOP:
        ret = ns_adc_adv_stop();
        break;
    default:
        ret = -RT_EINVAL;
        break;
    }

    rt_mutex_release(&ctx->lock);
    return ret;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops ns_adc_adv_ops =
{
    ns_adc_adv_init,
    ns_adc_adv_open,
    ns_adc_adv_close,
    ns_adc_adv_read,
    ns_adc_adv_write,
    ns_adc_adv_control,
};
#endif

int rt_hw_ns_adc_adv_init(void)
{
    /*
     * 注册高级伴生设备 nsadc0。
     * 它使用 Miscellaneous 类型，避免和官方 ADC class 的 rt_adc_ops 混用。
     */
    rt_memset(ctx, 0, sizeof(*ctx));
    rt_mutex_init(&ctx->lock, "nsadclk", RT_IPC_FLAG_PRIO);
    rt_sem_init(&ctx->done, "nsadcdn", 0, RT_IPC_FLAG_PRIO);

    ctx->parent.type = RT_Device_Class_Miscellaneous;
    ctx->parent.user_data = ctx;

#ifdef RT_USING_DEVICE_OPS
    ctx->parent.ops = &ns_adc_adv_ops;
#else
    ctx->parent.init = ns_adc_adv_init;
    ctx->parent.open = ns_adc_adv_open;
    ctx->parent.close = ns_adc_adv_close;
    ctx->parent.read = ns_adc_adv_read;
    ctx->parent.write = ns_adc_adv_write;
    ctx->parent.control = ns_adc_adv_control;
#endif

    return rt_device_register(&ctx->parent,
                              NS_ADC_ADV_DEVICE_NAME,
                              RT_DEVICE_FLAG_RDWR);
}
INIT_DEVICE_EXPORT(rt_hw_ns_adc_adv_init);

void ADCA_CONV_IRQHandler(void)
{
    ns_adc_unit *unit = ns_adc_get(NS_ADC_UNIT_A);

    /*
     * ADCA 转换完成中断：
     * IRQ 模式读取当前配置序列通道；Burst 模式读取 SOC7 起始的 burst 结果。
     */
    if (ctx->mode == NS_ADC_ADV_MODE_BURST)
    {
#ifdef BSP_USING_ADC0_BURST
        if (ctx->burst_cfg.unit_id == NS_ADC_UNIT_A)
        {
            rt_uint32_t i;
            rt_uint32_t count = ctx->burst_cfg.burst_size;

            for (i = 0U; i < count; i++)
            {
                ADC_SOCNumber soc = (ADC_SOCNumber)((rt_uint32_t)ctx->burst_cfg.first_soc + i);
                ADC_Channel adc_channel = ctx->burst_cfg.adc_channels[i % ctx->burst_cfg.channel_count];
                ns_adc_adv_push_sample(NS_ADC_UNIT_A, soc, adc_channel, ns_adc_read(unit, soc));
            }
        }
#endif
    }
    else if (((ctx->mode == NS_ADC_ADV_MODE_IRQ) || (ctx->mode == NS_ADC_ADV_MODE_DMA)) &&
             (ctx->irq_cfg.sequence.slot_count > 0U) &&
             (ctx->irq_cfg.sequence.unit_id == NS_ADC_UNIT_A))
    {
        rt_uint32_t index = ctx->write_index % ctx->irq_cfg.sequence.slot_count;
        const ns_adc_slot_cfg *slot = &ctx->irq_cfg.sequence.slots[index];
        ns_adc_adv_push_sample(NS_ADC_UNIT_A, slot->soc, slot->adc_channel, ns_adc_read(unit, slot->soc));
    }

    ns_adc_clear(unit, ADC_INT_NUMBER1);
    rt_sem_release(&ctx->done);
}

void ADCB_CONV_IRQHandler(void)
{
    ns_adc_unit *unit = ns_adc_get(NS_ADC_UNIT_B);

    /* ADCB 转换完成中断，供后续 adc1/adc2 或扩展通道映射复用。 */
    if ((ctx->mode == NS_ADC_ADV_MODE_IRQ) &&
        (ctx->irq_cfg.sequence.slot_count > 0U) &&
        (ctx->irq_cfg.sequence.unit_id == NS_ADC_UNIT_B))
    {
        rt_uint32_t index = ctx->write_index % ctx->irq_cfg.sequence.slot_count;
        const ns_adc_slot_cfg *slot = &ctx->irq_cfg.sequence.slots[index];
        ns_adc_adv_push_sample(NS_ADC_UNIT_B, slot->soc, slot->adc_channel, ns_adc_read(unit, slot->soc));
    }

    ns_adc_clear(unit, ADC_INT_NUMBER1);
    rt_sem_release(&ctx->done);
}

void ADCC_CONV_IRQHandler(void)
{
    ns_adc_unit *unit = ns_adc_get(NS_ADC_UNIT_C);

    /* ADCC 转换完成中断，主要服务 IRQ 模式下的 channel 2/3。 */
    if ((ctx->mode == NS_ADC_ADV_MODE_IRQ) &&
        (ctx->irq_cfg.sequence.slot_count > 0U) &&
        (ctx->irq_cfg.sequence.unit_id == NS_ADC_UNIT_C))
    {
        rt_uint32_t index = ctx->write_index % ctx->irq_cfg.sequence.slot_count;
        const ns_adc_slot_cfg *slot = &ctx->irq_cfg.sequence.slots[index];
        ns_adc_adv_push_sample(NS_ADC_UNIT_C, slot->soc, slot->adc_channel, ns_adc_read(unit, slot->soc));
    }

    ns_adc_clear(unit, ADC_INT_NUMBER1);
    rt_sem_release(&ctx->done);
}

static void ns_adc_adv_event_isr(ns_adc_unit_id id)
{
    /* PPB 事件状态读取和清除交给独立 PPB 模块处理。 */
#ifdef BSP_USING_ADC0_PPB
    ns_adc_ppb_event(id, &ctx->ppb.status);
#else
    RT_UNUSED(id);
#endif
}

void ADCA_EVENT_IRQHandler(void)
{
    ns_adc_adv_event_isr(NS_ADC_UNIT_A);
}

void ADCB_EVENT_IRQHandler(void)
{
    ns_adc_adv_event_isr(NS_ADC_UNIT_B);
}

void ADCC_EVENT_IRQHandler(void)
{
    ns_adc_adv_event_isr(NS_ADC_UNIT_C);
}

#ifdef BSP_USING_ADC0_DMA
void EDMA1_CH0_IRQHandler(void)
{
    rt_uint32_t i;
    rt_uint32_t count = ns_adc_adv_clamp_count(ctx->dma_cfg.sample_count);

    /*
     * EDMA major loop 完成中断：
     * 失效 DCache 后把 DMA 缓冲转成 nsadc0 的标准 sample 快照。
     */
    SCB_InvalidateDCache_by_Addr((void *)ns_adc_dma_buffer, (int32_t)sizeof(ns_adc_dma_buffer));
    EDMA_clearChannelStatusIntFlags(EDMA1, (uint8_t)ctx->dma_cfg.edma_channel);
    EDMA_clearChannelStatusDoneFlags(EDMA1, (uint8_t)ctx->dma_cfg.edma_channel);

    for (i = 0U; i < count; i++)
    {
        ns_adc_adv_push_sample(ctx->dma_cfg.unit_id,
                               ctx->dma_cfg.soc,
                               ctx->dma_cfg.adc_channel,
                               ns_adc_dma_buffer[i]);
    }

    rt_sem_release(&ctx->done);
}
#endif

#endif
