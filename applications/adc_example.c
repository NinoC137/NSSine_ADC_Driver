/**
 * @file adc_example.c
 * @brief ADC 最小应用流程示例。
 *
 * 文件说明：
 * 这些示例只展示关键初始化和采样 API 的调用顺序，方便 bring-up 时对照。
 * 为了保持代码干净，示例固定使用 ADCA、ADCIN0~ADCIN3、SOC0~SOC3。
 */

#include <rtthread.h>
#include "ns_adc_unit.h"
#include "ns_adc_irq.h"

#if defined(BSP_USING_ADC0)

#ifdef BSP_USING_ADC0_PPB
#include "drv_adc_ns800rt7p65x_ppb.h"
#endif

#ifdef BSP_USING_ADC0_DMA
#include "ns_adc_dma.h"
#endif

#ifdef RT_USING_FINSH
#include "finsh.h"
#endif

#define ADC_EXAMPLE_CHANNELS     4U
#define ADC_EXAMPLE_FRAMES       4U
#define ADC_EXAMPLE_DMA_CHANNEL  1U
#define ADC_EXAMPLE_TIMEOUT_MS   1000U
#define ADC_EXAMPLE_EPWM_PERIOD  2599U
#define ADC_EXAMPLE_EPWM_COMPARE 1300U
#define ADC_EXAMPLE_PPB_SAMPLES  8U

#ifdef BSP_USING_ADC0_DMA
static rt_uint16_t adc_example_dma_buffer[ADC_EXAMPLE_FRAMES][ADC_EXAMPLE_CHANNELS]
    __attribute__((aligned(32)));
#endif

#ifdef BSP_USING_ADC0_PPB
static void adc_example_print_ppb_status(const ns_adc_ppb_status_t *status)
{
    if (status == RT_NULL)
    {
        return;
    }

    rt_kprintf("ppb status: evt=0x%x delay=%u\r\n",
               (unsigned int)status->event_flags,
               (unsigned int)status->delay_stamp);
}

static void adc_example_print_ppb_oversampling(const ns_adc_ppb_oversampling_result_t *result)
{
    if (result == RT_NULL)
    {
        return;
    }

    rt_kprintf("ppb os: count=%u sum=%d min=%d max=%d min_i=%u max_i=%u avg=%d trim_avg=%d value=%d\r\n",
               (unsigned int)result->count,
               (int)result->sum,
               (int)result->min,
               (int)result->max,
               (unsigned int)result->min_index,
               (unsigned int)result->max_index,
               (int)result->average,
               (int)result->trimmed_average,
               (int)result->value);
}
#endif

static rt_err_t adc_example_config_channels(ns_adc_unit *unit)
{
    rt_uint32_t i;

    for (i = 0U; i < ADC_EXAMPLE_CHANNELS; i++)
    {
        ns_adc_slot_config_t slot;

        rt_memset(&slot, 0, sizeof(slot));
        slot.soc = (ADC_SOCNumber)i;
        slot.trigger = ADC_TRIGGER_SW_ONLY;
        slot.adc_channel = (ADC_Channel)i;
        slot.sample_window = NS_ADC_SAMPLE_WINDOW;
        slot.int_trigger = ADC_INT_SOC_TRIGGER_NONE;

        /* ns_adc_slot_cfg(): 配置一个 SOC 的触发源、ADCIN 通道和采样窗口。 */
        if (ns_adc_slot_cfg(unit, &slot) != RT_EOK)
        {
            return -RT_ERROR;
        }
    }

    return RT_EOK;
}

static rt_err_t adc_example_wait_eoc(ns_adc_unit *unit)
{
    /* ns_adc_wait(): 等待 ADCINT1 置位，确认最后一个 SOC 已完成转换。 */
    return ns_adc_wait(unit, ADC_INT_NUMBER1, 50);
}

static void adc_example_print_frame(const char *name, rt_uint32_t frame, const rt_uint16_t *raw)
{
    rt_uint32_t i;

    rt_kprintf("%s frame%u:", name, (unsigned int)frame);
    for (i = 0U; i < ADC_EXAMPLE_CHANNELS; i++)
    {
        rt_kprintf(" ch%u=%u", (unsigned int)i, (unsigned int)(raw[i] & 0x0fffU));
    }
    rt_kprintf("\r\n");
}

#ifdef BSP_USING_ADC0_BURST
static void adc_example_epwm1_soca_start(void)
{
    /* SYSCON_UNLOCK: 解锁系统控制寄存器，允许修改 ePWM 时钟同步配置。 */
    SYSCON_UNLOCK;

    /* SYSCON_setTbClkSync(false): 暂停 ePWM TBCLK 同步，避免配置过程中产生错误触发。 */
    SYSCON_setTbClkSync(SYSCON, false);

    /* SYSCON_LOCK: 重新锁定系统控制寄存器。 */
    SYSCON_LOCK;

    /* EPWM_disableADCTrigger(): 先关闭 EPWM1 SOCA，确保重新配置前没有旧触发输出。 */
    EPWM_disableADCTrigger(EPWM1, EPWM_SOC_A);

    /* EPWM_setADCTriggerSource(): 选择计数器上数到 CMPA 时产生 SOCA 触发。 */
    EPWM_setADCTriggerSource(EPWM1, EPWM_SOC_A, EPWM_SOC_TBCTR_U_CMPA);

    /* EPWM_setADCTriggerEventPrescale(): 每次满足触发条件都输出一次 SOCA。 */
    EPWM_setADCTriggerEventPrescale(EPWM1, EPWM_SOC_A, 1U);

    /* EPWM_setTimeBasePeriod(): 设置 EPWM1 计数周期。 */
    EPWM_setTimeBasePeriod(EPWM1, ADC_EXAMPLE_EPWM_PERIOD);

    /* EPWM_setCounterCompareValue(): 设置 CMPA，决定 SOCA 在周期中的触发位置。 */
    EPWM_setCounterCompareValue(EPWM1, EPWM_COUNTER_COMPARE_A, ADC_EXAMPLE_EPWM_COMPARE);

    /* EPWM_setClockPrescaler(): 设置 EPWM1 时钟分频。 */
    EPWM_setClockPrescaler(EPWM1, EPWM_CLOCK_DIVIDER_2, EPWM_HSCLOCK_DIVIDER_1);

    /* EPWM_setTimeBaseCounterMode(): 配置上下计数模式，让 EPWM1 开始计数。 */
    EPWM_setTimeBaseCounterMode(EPWM1, EPWM_COUNTER_MODE_UP_DOWN);

    /* EPWM_enableADCTrigger(): 使能 EPWM1 SOCA 输出到 ADC Burst 触发链路。 */
    EPWM_enableADCTrigger(EPWM1, EPWM_SOC_A);

    /* SYSCON_UNLOCK: 解锁系统控制寄存器，打开 EPWM1 SOCA 到 ADC 的门控。 */
    SYSCON_UNLOCK;

    /* SYSCON_setEpwmSocAEn(): 允许 EPWM1 SOCA 作为 ADC 触发源。 */
    SYSCON_setEpwmSocAEn(SYSCON, SYSCON_EPWM1SOCAEN, true);

    /* SYSCON_setTbClkSync(true): 恢复 ePWM TBCLK 同步，配置后的 EPWM1 正式运行。 */
    SYSCON_setTbClkSync(SYSCON, true);

    /* SYSCON_LOCK: 锁定系统控制寄存器。 */
    SYSCON_LOCK;
}

static void adc_example_epwm1_soca_stop(void)
{
    /* EPWM_disableADCTrigger(): 关闭 EPWM1 SOCA，停止继续触发 ADC Burst。 */
    EPWM_disableADCTrigger(EPWM1, EPWM_SOC_A);

    /* EPWM_setTimeBaseCounterMode(): 冻结 EPWM1 计数器。 */
    EPWM_setTimeBaseCounterMode(EPWM1, EPWM_COUNTER_MODE_STOP_FREEZE);
}
#endif

int adc_once(void)
{
    rt_uint32_t i;
    rt_uint32_t raw32;
    rt_uint16_t raw[ADC_EXAMPLE_CHANNELS];

    /* ns_adc_hw_init(): 初始化 ADC 硬件基础配置；单次读取会按需配置对应 SOC。 */
    if (ns_adc_hw_init() != RT_EOK)
    {
        return -RT_ERROR;
    }

    for (i = 0U; i < ADC_EXAMPLE_CHANNELS; i++)
    {
        /* ns_adc_read_channel_once(): 对指定 ADCIN 做一次同步软件触发采样。 */
        if (ns_adc_read_channel_once(i, &raw32) != RT_EOK)
        {
            return -RT_ERROR;
        }
        raw[i] = (rt_uint16_t)raw32;
    }

    adc_example_print_frame("once", 0U, raw);
    return RT_EOK;
}

int adc_loop(void)
{
    ns_adc_unit *unit;
    rt_uint32_t frame;
    rt_uint32_t ch;
    rt_uint16_t raw[ADC_EXAMPLE_CHANNELS];

    /* ns_adc_hw_init(): 初始化 ADC 基础硬件，确保 ADCA 已可用。 */
    if (ns_adc_hw_init() != RT_EOK)
    {
        return -RT_ERROR;
    }

    /* ns_adc_get(): 获取 ADCA 的静态 unit 对象，后续配置都作用在这个 ADC 单元。 */
    unit = ns_adc_get(NS_ADC_UNIT_A);
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_ERROR;
    }

    if (adc_example_config_channels(unit) != RT_EOK)
    {
        return -RT_ERROR;
    }

    /* ns_adc_irq_on(): 让 ADCINT1 在 SOC3 转换完成时置位，作为一帧完成标志。 */
    if (ns_adc_irq_on(NS_ADC_UNIT_A, ADC_INT_NUMBER1, ADC_SOC_NUMBER3) != RT_EOK)
    {
        return -RT_ERROR;
    }

    for (frame = 0U; frame < ADC_EXAMPLE_FRAMES; frame++)
    {
        /* ns_adc_clear(): 清除上一帧留下的 ADCINT1/overflow 状态。 */
        ns_adc_clear(unit, ADC_INT_NUMBER1);

        /* ns_adc_trigger(): 软件触发 SOC0~SOC3，启动一次多通道扫描。 */
        if (ns_adc_trigger(unit, ns_adc_soc_mask(ADC_SOC_NUMBER0, ADC_EXAMPLE_CHANNELS)) != RT_EOK)
        {
            return -RT_ERROR;
        }

        if (adc_example_wait_eoc(unit) != RT_EOK)
        {
            return -RT_ETIMEOUT;
        }

        for (ch = 0U; ch < ADC_EXAMPLE_CHANNELS; ch++)
        {
            /* ns_adc_read(): 从 RESULTx 读取对应 SOC 的 raw 转换结果。 */
            raw[ch] = (rt_uint16_t)ns_adc_read(unit, (ADC_SOCNumber)ch);
        }

        adc_example_print_frame("loop", frame, raw);
    }

    return RT_EOK;
}

int adc_burst_loop(void)
{
#ifndef BSP_USING_ADC0_BURST
    return -RT_ENOSYS;
#else
    ns_adc_unit *unit;
    ns_adc_burst_config_t burst;
    rt_uint32_t frame;
    rt_uint32_t ch;
    rt_uint16_t raw[ADC_EXAMPLE_CHANNELS];

    /* ns_adc_hw_init(): 初始化 ADC 基础硬件，Burst 配置建立在基础 ADC 之上。 */
    if (ns_adc_hw_init() != RT_EOK)
    {
        return -RT_ERROR;
    }

    /* ns_adc_get(): 获取 ADCA 的 unit 对象。 */
    unit = ns_adc_get(NS_ADC_UNIT_A);
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_ERROR;
    }

    if (adc_example_config_channels(unit) != RT_EOK)
    {
        return -RT_ERROR;
    }

    rt_memset(&burst, 0, sizeof(burst));
    burst.trigger = ADC_BURSTTRIG5_EPWM1_SOCA;
    burst.burst_size = ADC_EXAMPLE_CHANNELS;
    burst.priority = ADC_PRI_ALL_ROUND_ROBIN;

    /* ns_adc_burst_cfg(): 开启 Burst 模式，设置硬件 Burst 的触发源和每组 SOC 数量。 */
    if (ns_adc_burst_cfg(unit, &burst) != RT_EOK)
    {
        return -RT_ERROR;
    }

    /* ns_adc_irq_on(): 设置 SOC3 转换完成后置位 ADCINT1，作为一帧完成标志。 */
    if (ns_adc_irq_on(NS_ADC_UNIT_A, ADC_INT_NUMBER1, ADC_SOC_NUMBER3) != RT_EOK)
    {
        return -RT_ERROR;
    }

    /* adc_example_epwm1_soca_start(): 显式配置并启动 EPWM1 SOCA，作为 ADC Burst 必要触发源。 */
    adc_example_epwm1_soca_start();

    for (frame = 0U; frame < ADC_EXAMPLE_FRAMES; frame++)
    {
        /* ns_adc_clear(): 清除上一帧 ADCINT1 状态。 */
        ns_adc_clear(unit, ADC_INT_NUMBER1);

        if (adc_example_wait_eoc(unit) != RT_EOK)
        {
            adc_example_epwm1_soca_stop();
            ns_adc_stop(unit);
            return -RT_ETIMEOUT;
        }

        for (ch = 0U; ch < ADC_EXAMPLE_CHANNELS; ch++)
        {
            /* ns_adc_read(): 读取本帧每个 SOC 的 raw 结果。 */
            raw[ch] = (rt_uint16_t)ns_adc_read(unit, (ADC_SOCNumber)ch);
        }

        adc_example_print_frame("burst", frame, raw);
    }

    /* adc_example_epwm1_soca_stop(): 关闭 EPWM1 SOCA，避免示例结束后继续触发 ADC。 */
    adc_example_epwm1_soca_stop();

    /* ns_adc_stop(): 停止本 unit 的 Burst 和 IRQ 状态，便于后续示例继续运行。 */
    ns_adc_stop(unit);
    return RT_EOK;
#endif
}

int adc_dma_loop(void)
{
#ifndef BSP_USING_ADC0_DMA
    return -RT_ENOSYS;
#else
    ns_adc_unit *unit;
    ns_adc_dma_cfg dma_cfg;
    rt_uint32_t frame;
    rt_tick_t start;
    rt_tick_t timeout;

    /* ns_adc_hw_init(): 初始化 ADC 基础硬件，DMA 只负责搬运 ADC 结果。 */
    if (ns_adc_hw_init() != RT_EOK)
    {
        return -RT_ERROR;
    }

    /* ns_adc_get(): 获取 ADCA 的 unit 对象。 */
    unit = ns_adc_get(NS_ADC_UNIT_A);
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_ERROR;
    }

    if (adc_example_config_channels(unit) != RT_EOK)
    {
        return -RT_ERROR;
    }

    rt_memset(adc_example_dma_buffer, 0, sizeof(adc_example_dma_buffer));

    for (frame = 0U; frame < ADC_EXAMPLE_FRAMES; frame++)
    {
        rt_memset(&dma_cfg, 0, sizeof(dma_cfg));
        dma_cfg.channel.edma_channel = ADC_EXAMPLE_DMA_CHANNEL;
        dma_cfg.channel.req_index = 0U;
        dma_cfg.channel.dma_int = ADC_INT_NUMBER1;
        dma_cfg.channel.major_int = RT_TRUE;
        dma_cfg.channel.error_int = RT_TRUE;
        dma_cfg.channel.continuous_int = RT_TRUE;
        dma_cfg.buffer = adc_example_dma_buffer[frame];
        dma_cfg.frame_count = 1U;
        dma_cfg.results_per_frame = ADC_EXAMPLE_CHANNELS;
        dma_cfg.first_soc = ADC_SOC_NUMBER0;
        dma_cfg.trans_trigger_soc = ADC_SOC_NUMBER3;

        /* ns_adc_dma(): 配置 ADCINT1 -> EDMA -> buffer 的搬运链路。 */
        if (ns_adc_dma(unit, &dma_cfg) != RT_EOK)
        {
            return -RT_ERROR;
        }

        /* ns_adc_dma_start(): 允许 EDMA channel 响应 ADCINT DMA 请求。 */
        if (ns_adc_dma_start(unit) != RT_EOK)
        {
            return -RT_ERROR;
        }

        /* ns_adc_trigger(): 软件触发 SOC0~SOC3；SOC3 EOC 后产生 DMA 请求。 */
        if (ns_adc_trigger(unit, ns_adc_soc_mask(ADC_SOC_NUMBER0, ADC_EXAMPLE_CHANNELS)) != RT_EOK)
        {
            return -RT_ERROR;
        }

        start = rt_tick_get();
        timeout = rt_tick_from_millisecond(ADC_EXAMPLE_TIMEOUT_MS);
        while (ns_adc_dma_done(unit) == RT_FALSE)
        {
            /* ns_adc_dma_error(): 查询 EDMA channel 是否出现总线或传输错误。 */
            if (ns_adc_dma_error(unit) == RT_TRUE)
            {
                ns_adc_dma_stop(unit);
                return -RT_ERROR;
            }

            if ((rt_tick_get() - start) >= timeout)
            {
                ns_adc_dma_stop(unit);
                return -RT_ETIMEOUT;
            }

            rt_thread_mdelay(1);
        }

        /* DCache invalidate: DMA 写内存后，CPU 读取前需要失效对应 cache line。 */
        SCB_InvalidateDCache_by_Addr(adc_example_dma_buffer[frame],
                                     ADC_EXAMPLE_CHANNELS * sizeof(rt_uint16_t));

        adc_example_print_frame("dma", frame, adc_example_dma_buffer[frame]);

        /* ns_adc_dma_stop(): 关闭 DMA 请求并清除 EDMA/ADCINT 状态，避免影响下一帧。 */
        ns_adc_dma_stop(unit);
    }

    return RT_EOK;
#endif
}

#ifdef BSP_USING_ADC0_PPB
int adc_ppb(void)
{
    ns_adc_unit *unit;
    ns_adc_slot_config_t slot;
    ns_adc_ppb_config_t cfg =
    {
        .unit_id = NS_ADC_UNIT_A,
        .bind =
        {
            .ppb = ADC_PPB_NUMBER1,
            .soc = ADC_SOC_NUMBER0,
        },
        .high_limit = 3000U,
        .low_limit = 0U,
        .event_mask = NS_ADC_EVT_TRIPHI | NS_ADC_EVT_TRIPLO | NS_ADC_EVT_ZERO,
        .event_irq = RT_TRUE,
    };
    ns_adc_ppb_status_t status = {0};

    if (ns_adc_hw_init() != RT_EOK)
    {
        return -RT_ERROR;
    }

    unit = ns_adc_get(NS_ADC_UNIT_A);
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_ERROR;
    }

    rt_memset(&slot, 0, sizeof(slot));
    slot.soc = cfg.bind.soc;
    slot.trigger = ADC_TRIGGER_SW_ONLY;
    slot.adc_channel = ADC_CH_ADCIN0;
    slot.sample_window = NS_ADC_SAMPLE_WINDOW;
    slot.int_trigger = ADC_INT_SOC_TRIGGER_NONE;
    if (ns_adc_slot_cfg(unit, &slot) != RT_EOK)
    {
        return -RT_ERROR;
    }

    if (ns_adc_ppb_limit(&cfg) != RT_EOK)
    {
        return -RT_ERROR;
    }

    ns_adc_ppb_event(cfg.unit_id, &status);
    adc_example_print_ppb_status(&status);
    return 0;
}

int adc_ppb_os(void)
{
    ns_adc_ppb_oversampling_config_t cfg =
    {
        .unit_id = NS_ADC_UNIT_A,
        .bind =
        {
            .ppb = ADC_PPB_NUMBER1,
            .soc = ADC_SOC_NUMBER0,
        },
        .adc_channel = ADC_CH_ADCIN0,
        .sample_count = ADC_EXAMPLE_PPB_SAMPLES,
        .sample_window = NS_ADC_SAMPLE_WINDOW,
        .shift = 0U,
        .drop_min_max = RT_TRUE,
        .enable_osint_irq = RT_FALSE,
    };
    ns_adc_ppb_oversampling_result_t result = {0};
    rt_err_t ret;

    if (ns_adc_hw_init() != RT_EOK)
    {
        return -RT_ERROR;
    }

    ret = ns_adc_ppb_oversampling_start(&cfg, &result);
    if (ret != RT_EOK)
    {
        rt_kprintf("ppb os failed: %d\r\n", ret);
        return ret;
    }

    adc_example_print_ppb_oversampling(&result);
    return 0;
}
#endif

#ifdef RT_USING_FINSH
MSH_CMD_EXPORT(adc_once, adc example: multi-channel one-shot);
MSH_CMD_EXPORT(adc_loop, adc example: multi-channel loop);
MSH_CMD_EXPORT(adc_burst_loop, adc example: multi-channel burst loop);
MSH_CMD_EXPORT(adc_dma_loop, adc example: multi-channel dma loop);
#ifdef BSP_USING_ADC0_PPB
MSH_CMD_EXPORT(adc_ppb, adc example: ppb limit event);
MSH_CMD_EXPORT(adc_ppb_os, adc example: ppb hardware oversampling);
#endif
#endif

#endif
