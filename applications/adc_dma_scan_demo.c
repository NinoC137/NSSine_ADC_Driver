/**
 * @file adc_dma_scan_demo.c
 * @brief ADCA ADCIN0~8 software-trigger scan with EDMA buffer transfer.
 */

#include <rtthread.h>
#include "ns_adc_unit.h"
#include "ns_adc_irq.h"

#if defined(BSP_USING_ADC0) && defined(BSP_USING_ADC0_DMA)

#include "ns_adc_dma.h"

#ifdef RT_USING_FINSH
#include "finsh.h"
#endif

#define ADC_DMA_SCAN_CHANNELS      9U
#define ADC_DMA_SCAN_FRAMES        8U
#define ADC_DMA_SCAN_DMA_CHANNEL   1U
#define ADC_DMA_SCAN_TIMEOUT_MS    1000U

static rt_uint16_t adc_dma_scan_buffer[ADC_DMA_SCAN_FRAMES][ADC_DMA_SCAN_CHANNELS]
    __attribute__((aligned(32)));

static rt_err_t adc_dma_scan_config_soc(ns_adc_unit *unit)
{
    rt_uint32_t i;

    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_EINVAL;
    }

    for (i = 0U; i < ADC_DMA_SCAN_CHANNELS; i++)
    {
        ns_adc_slot_config_t slot;

        rt_memset(&slot, 0, sizeof(slot));
        slot.soc = (ADC_SOCNumber)i;
        slot.trigger = ADC_TRIGGER_SW_ONLY;
        slot.adc_channel = (ADC_Channel)i;
        slot.sample_window = NS_ADC_SAMPLE_WINDOW;
        slot.int_trigger = ADC_INT_SOC_TRIGGER_NONE;

        if (ns_adc_slot_cfg(unit, &slot) != RT_EOK)
        {
            return -RT_ERROR;
        }
    }

    return RT_EOK;
}

static rt_err_t adc_dma_scan_config_dma(ns_adc_unit *unit, rt_uint32_t frame)
{
    ns_adc_dma_cfg dma_cfg;

    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        return -RT_EINVAL;
    }

    rt_memset(&dma_cfg, 0, sizeof(dma_cfg));
    dma_cfg.channel.edma_channel = ADC_DMA_SCAN_DMA_CHANNEL;
    dma_cfg.channel.req_index = 0U;
    dma_cfg.channel.dma_int = ADC_INT_NUMBER1;
    dma_cfg.channel.major_int = RT_TRUE;
    dma_cfg.channel.error_int = RT_TRUE;
    dma_cfg.channel.continuous_int = RT_TRUE;
    dma_cfg.buffer = adc_dma_scan_buffer[frame];
    dma_cfg.frame_count = 1U;
    dma_cfg.results_per_frame = ADC_DMA_SCAN_CHANNELS;
    dma_cfg.first_soc = ADC_SOC_NUMBER0;
    dma_cfg.trans_trigger_soc = ADC_SOC_NUMBER8;

    return ns_adc_dma(unit, &dma_cfg);
}

static void adc_dma_scan_dump(void)
{
    rt_uint32_t frame;
    rt_uint32_t ch;

    rt_kprintf("adc dma scan buffer: %u frames x %u channels\r\n",
               (unsigned int)ADC_DMA_SCAN_FRAMES,
               (unsigned int)ADC_DMA_SCAN_CHANNELS);

    for (frame = 0U; frame < ADC_DMA_SCAN_FRAMES; frame++)
    {
        rt_kprintf("frame%u:", (unsigned int)frame);
        for (ch = 0U; ch < ADC_DMA_SCAN_CHANNELS; ch++)
        {
            rt_kprintf(" ch%u=%u",
                       (unsigned int)ch,
                       (unsigned int)(adc_dma_scan_buffer[frame][ch] & 0x0fffU));
        }
        rt_kprintf("\r\n");
    }
}

static rt_err_t adc_dma_scan_wait_frame(ns_adc_unit *unit)
{
    rt_tick_t start = rt_tick_get();
    rt_tick_t timeout = rt_tick_from_millisecond(ADC_DMA_SCAN_TIMEOUT_MS);

    while (ns_adc_dma_done(unit) == RT_FALSE)
    {
        if (ns_adc_dma_error(unit) == RT_TRUE)
        {
            return -RT_ERROR;
        }

        if ((rt_tick_get() - start) >= timeout)
        {
            return -RT_ETIMEOUT;
        }

        rt_thread_mdelay(1);
    }

    if (ns_adc_dma_error(unit) == RT_TRUE)
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}

int adc_dma_scan(void)
{
    ns_adc_unit *unit;
    rt_err_t ret = RT_EOK;
    rt_uint32_t frame;

    if (ns_adc_hw_init() != RT_EOK)
    {
        rt_kprintf("adc dma scan hw init failed\r\n");
        return -RT_ERROR;
    }

    unit = ns_adc_get(NS_ADC_UNIT_A);
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        rt_kprintf("adc dma scan unit invalid\r\n");
        return -RT_ERROR;
    }

    ns_adc_stop(unit);

    if (adc_dma_scan_config_soc(unit) != RT_EOK)
    {
        rt_kprintf("adc dma scan soc config failed\r\n");
        return -RT_ERROR;
    }

    rt_memset(adc_dma_scan_buffer, 0, sizeof(adc_dma_scan_buffer));

    for (frame = 0U; frame < ADC_DMA_SCAN_FRAMES; frame++)
    {
        if (adc_dma_scan_config_dma(unit, frame) != RT_EOK)
        {
            rt_kprintf("adc dma scan dma config failed at frame%u\r\n", (unsigned int)frame);
            ret = -RT_ERROR;
            break;
        }

        if (ns_adc_dma_start(unit) != RT_EOK)
        {
            rt_kprintf("adc dma scan dma start failed at frame%u\r\n", (unsigned int)frame);
            ret = -RT_ERROR;
            break;
        }

        ns_adc_trigger(unit, ns_adc_soc_mask(ADC_SOC_NUMBER0, ADC_DMA_SCAN_CHANNELS));
        ret = adc_dma_scan_wait_frame(unit);
        if (ret != RT_EOK)
        {
            rt_kprintf("adc dma scan failed at frame%u: %d\r\n",
                       (unsigned int)frame,
                       ret);
            break;
        }

        SCB_InvalidateDCache_by_Addr(adc_dma_scan_buffer[frame],
                                     ADC_DMA_SCAN_CHANNELS * sizeof(rt_uint16_t));
        ns_adc_dma_stop(unit);
    }

    if (ret == RT_EOK)
    {
        adc_dma_scan_dump();
    }

    ns_adc_dma_stop(unit);
    ns_adc_hw_reset();
    return (ret == RT_EOK) ? 0 : -RT_ERROR;
}

#ifdef RT_USING_FINSH
// MSH_CMD_EXPORT(adc_dma_scan, scan ADCIN0-8 by EDMA into a fixed buffer);
#endif

#endif
