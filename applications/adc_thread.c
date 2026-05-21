/**
 * @file adc_thread.c
 * @brief 后台线程持续执行 8 通道 ADC+DMA 采样示例。
 *
 * 文件说明：
 * 本示例展示一种硬件触发后台采样模型：
 * 1. EPWM1 SOCA 周期性触发 ADCA SOC0~SOC7。
 * 2. ADCINT1 在 SOC7 EOC 后触发 EDMA。
 * 3. EDMA 每次 request 搬运 RESULT0~RESULT7 到 ring buffer 中的一帧。
 * 4. CPU 不参与逐帧触发，也不打开逐帧 EDMA/ADC 中断。
 * 5. FinSH dump 根据 EDMA 当前写指针读取最近完成的帧。
 */

#include <rtthread.h>
#include "ns_adc_unit.h"
#include "ns_adc_irq.h"
#include "interrupt.h"

#if defined(BSP_USING_ADC0) && defined(BSP_USING_ADC0_DMA) && defined(BSP_USING_ADC0_BURST)

#include "ns_adc_dma.h"

#ifdef RT_USING_FINSH
#include "finsh.h"
#endif

#define ADC_THREAD_CHANNELS      8U
#define ADC_THREAD_FRAME_COUNT   32U
#define ADC_THREAD_DMA_CHANNEL   1U
#define ADC_THREAD_CACHE_BYTES    32U
// 配置ADC触发采样频率为10khz
#define ADC_THREAD_EPWM_PERIOD   155U
#define ADC_THREAD_EPWM_COMPARE  78U

static rt_uint16_t adc_thread_frames[ADC_THREAD_FRAME_COUNT][ADC_THREAD_CHANNELS]
    __attribute__((aligned(32)));
static rt_uint16_t adc_thread_zero_frame[ADC_THREAD_CHANNELS]
    __attribute__((aligned(32)));

static volatile rt_bool_t adc_thread_running = RT_FALSE;

static rt_err_t adc_thread_config_channels(ns_adc_unit *unit)
{
    rt_uint32_t i;

    for (i = 0U; i < ADC_THREAD_CHANNELS; i++)
    {
        ns_adc_slot_config_t slot;

        rt_memset(&slot, 0, sizeof(slot));
        slot.soc = (ADC_SOCNumber)i;
        slot.trigger = ADC_TRIGGER_EPWM1_SOCA;
        slot.adc_channel = (ADC_Channel)i;
        slot.sample_window = NS_ADC_SAMPLE_WINDOW;
        slot.int_trigger = ADC_INT_SOC_TRIGGER_NONE;

        /* ns_adc_slot_cfg(): 配置 SOCx 绑定 ADCINx，并设置软件触发和采样窗口。 */
        if (ns_adc_slot_cfg(unit, &slot) != RT_EOK)
        {
            return -RT_ERROR;
        }
    }

    return RT_EOK;
}

static rt_err_t adc_thread_config_dma(ns_adc_unit *unit)
{
    ns_adc_dma_cfg dma;

    rt_memset(&dma, 0, sizeof(dma));
    dma.channel.edma_channel = ADC_THREAD_DMA_CHANNEL;
    dma.channel.req_index = 0U;
    dma.channel.dma_int = ADC_INT_NUMBER1;
    dma.channel.major_int = RT_FALSE;
    dma.channel.error_int = RT_TRUE;
    dma.channel.continuous_int = RT_TRUE;
    dma.buffer = adc_thread_frames[0];
    dma.frame_count = ADC_THREAD_FRAME_COUNT;
    dma.results_per_frame = ADC_THREAD_CHANNELS;
    dma.first_soc = ADC_SOC_NUMBER0;
    dma.trans_trigger_soc = ADC_SOC_NUMBER7;

    /* 配置 ADCINT1 -> EDMA -> DMA 工作 buffer 的搬运链路。 */
    return ns_adc_dma(unit, &dma);
}

static void adc_thread_epwm1_soca_start(void)
{
    SYSCON_UNLOCK;
    SYSCON_setTbClkSync(SYSCON, false);
    SYSCON_LOCK;

    EPWM_disableADCTrigger(EPWM1, EPWM_SOC_A);
    EPWM_setADCTriggerSource(EPWM1, EPWM_SOC_A, EPWM_SOC_TBCTR_U_CMPA);
    EPWM_setADCTriggerEventPrescale(EPWM1, EPWM_SOC_A, 1U);
    EPWM_setTimeBasePeriod(EPWM1, ADC_THREAD_EPWM_PERIOD);
    EPWM_setCounterCompareValue(EPWM1, EPWM_COUNTER_COMPARE_A, ADC_THREAD_EPWM_COMPARE);
    EPWM_setClockPrescaler(EPWM1, EPWM_CLOCK_DIVIDER_64, EPWM_HSCLOCK_DIVIDER_1);
    EPWM_setTimeBaseCounterMode(EPWM1, EPWM_COUNTER_MODE_UP_DOWN);
    EPWM_enableADCTrigger(EPWM1, EPWM_SOC_A);

    SYSCON_UNLOCK;
    SYSCON_setEpwmSocAEn(SYSCON, SYSCON_EPWM1SOCAEN, true);
    SYSCON_setTbClkSync(SYSCON, true);
    SYSCON_LOCK;
}

static void adc_thread_epwm1_soca_stop(void)
{
    EPWM_disableADCTrigger(EPWM1, EPWM_SOC_A);
    EPWM_setTimeBaseCounterMode(EPWM1, EPWM_COUNTER_MODE_STOP_FREEZE);
}

static rt_uint32_t adc_thread_next_write_frame(void)
{
    uintptr_t base = (uintptr_t)EDMA_getAbsAddrForMultiCore((uint32_t)adc_thread_frames[0]);
    uintptr_t daddr = (uintptr_t)EDMA1->TCD[ADC_THREAD_DMA_CHANNEL].DADDR.WORDVAL;
    uintptr_t frame_bytes = ADC_THREAD_CHANNELS * sizeof(rt_uint16_t);
    uintptr_t ring_bytes = ADC_THREAD_FRAME_COUNT * frame_bytes;
    uintptr_t offset;

    if ((daddr < base) || (daddr >= (base + ring_bytes)))
    {
        return 0U;
    }

    offset = daddr - base;
    return (rt_uint32_t)((offset / frame_bytes) % ADC_THREAD_FRAME_COUNT);
}

const rt_uint16_t *adc_thread_latest(rt_uint32_t *seq)
{
    rt_uint32_t next;
    rt_uint32_t latest;

    if (adc_thread_running == RT_FALSE)
    {
        if (seq != RT_NULL)
        {
            *seq = 0U;
        }
        return adc_thread_zero_frame;
    }

    next = adc_thread_next_write_frame();
    latest = (next + ADC_THREAD_FRAME_COUNT - 1U) % ADC_THREAD_FRAME_COUNT;

    SCB_InvalidateDCache_by_Addr(adc_thread_frames[latest], ADC_THREAD_CACHE_BYTES);
    if (seq != RT_NULL)
    {
        rt_uint32_t citer = EDMA1->TCD[ADC_THREAD_DMA_CHANNEL].CITER_ELINKNO.BIT.CITER;
        rt_uint32_t completed_in_cycle = (ADC_THREAD_FRAME_COUNT - citer) % ADC_THREAD_FRAME_COUNT;
        *seq = completed_in_cycle;
    }

    return adc_thread_frames[latest];
}

int adc_start(void)
{
    ns_adc_unit *unit;

    if (adc_thread_running == RT_TRUE)
    {
        return 0;
    }

    if (ns_adc_hw_init() != RT_EOK)
    {
        return -RT_ERROR;
    }

    unit = ns_adc_get(NS_ADC_UNIT_A);
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_ERROR;
    }

    adc_thread_epwm1_soca_stop();
    ns_adc_dma_stop(unit);

    if (adc_thread_config_channels(unit) != RT_EOK)
    {
        return -RT_ERROR;
    }

    rt_memset((void *)adc_thread_frames, 0, sizeof(adc_thread_frames));
    rt_memset((void *)adc_thread_zero_frame, 0, sizeof(adc_thread_zero_frame));
    SCB_CleanDCache_by_Addr(adc_thread_frames, sizeof(adc_thread_frames));

    if (adc_thread_config_dma(unit) != RT_EOK)
    {
        return -RT_ERROR;
    }

    ns_adc_dma_clear(unit);

    if (ns_adc_dma_start(unit) != RT_EOK)
    {
        ns_adc_dma_stop(unit);
        return -RT_ERROR;
    }

    adc_thread_running = RT_TRUE;
    adc_thread_epwm1_soca_start();
    return 0;
}

int adc_stop(void)
{
    ns_adc_unit *unit = ns_adc_get(NS_ADC_UNIT_A);

    adc_thread_running = RT_FALSE;
    adc_thread_epwm1_soca_stop();
    if (unit != RT_NULL)
    {
        ns_adc_dma_stop(unit);
    }
    return 0;
}

int adc_dump(void)
{
    rt_uint32_t i;
    rt_uint32_t seq;
    const rt_uint16_t *frame = adc_thread_latest(&seq);

    rt_kprintf("adc_thread seq=%u:", (unsigned int)seq);
    for (i = 0U; i < ADC_THREAD_CHANNELS; i++)
    {
        rt_kprintf(" ch%u=%u", (unsigned int)i, (unsigned int)(frame[i] & 0x0fffU));
    }
    rt_kprintf("\r\n");

    return 0;
}

int adc_status(void)
{
    ns_adc_unit *unit = ns_adc_get(NS_ADC_UNIT_A);
    rt_uint32_t citer = EDMA1->TCD[ADC_THREAD_DMA_CHANNEL].CITER_ELINKNO.BIT.CITER;
    rt_uint32_t daddr = EDMA1->TCD[ADC_THREAD_DMA_CHANNEL].DADDR.WORDVAL;
    rt_uint32_t int_status = EDMA_getChannelIntStatus(EDMA1);
    rt_uint32_t err_status = EDMA_getAllChannelErrorFlags(EDMA1);
    rt_uint32_t adc_int = 0U;
    rt_uint32_t adc_ovf = 0U;

    if ((unit != RT_NULL) && (unit->cfg != RT_NULL))
    {
        adc_int = ADC_getInterruptStatus(unit->cfg->base, ADC_INT_NUMBER1);
        adc_ovf = ADC_getInterruptOverflowStatus(unit->cfg->base, ADC_INT_NUMBER1);
    }

    rt_kprintf("adc_thread running=%u next=%u citer=%u daddr=0x%08x edma_int=0x%08x edma_err=0x%08x adcint=%u ovf=%u\r\n",
               (unsigned int)adc_thread_running,
               (unsigned int)adc_thread_next_write_frame(),
               (unsigned int)citer,
               (unsigned int)daddr,
               (unsigned int)int_status,
               (unsigned int)err_status,
               (unsigned int)adc_int,
               (unsigned int)adc_ovf);
    return 0;
}

#ifdef RT_USING_FINSH
MSH_CMD_EXPORT(adc_start, start background ADC DMA sampling);
MSH_CMD_EXPORT(adc_stop, stop background ADC DMA sampling);
MSH_CMD_EXPORT(adc_dump, dump latest background ADC frame);
MSH_CMD_EXPORT(adc_status, dump background ADC DMA status);
#endif

#endif
