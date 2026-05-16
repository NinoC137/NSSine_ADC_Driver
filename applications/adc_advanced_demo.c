/**
 * @file adc_advanced_demo.c
 * @brief nsadc0 高级 ADC 伴生设备的 FinSH 验证命令。
 *
 * 文件说明：
 * 本文件只作为应用层验证入口，不直接操作厂商寄存器。
 * 它通过 rt_device_find/control/read 访问 nsadc0，用来验证中断、DMA、
 * Burst、Burst+DMA 和 PPB 配置链路是否已经接入 RT-Thread device API。
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "drv_adc_ns800rt7p65x_adv.h"
#include "drv_adc_ns800rt7p65x_ppb.h"

#ifdef BSP_USING_ADC0_ADVANCED

#ifdef RT_USING_FINSH
#include "finsh.h"
#endif

static rt_device_t adc_adv_get_device(void)
{
    /* 统一查找 nsadc0，减少每个命令里的重复错误处理。 */
    rt_device_t dev = rt_device_find(NS_ADC_ADV_DEVICE_NAME);

    if (dev == RT_NULL)
    {
        rt_kprintf("nsadc0 not found\r\n");
        return RT_NULL;
    }

    if (rt_device_open(dev, RT_DEVICE_OFLAG_RDWR) != RT_EOK)
    {
        rt_kprintf("nsadc0 open failed\r\n");
        return RT_NULL;
    }

    return dev;
}

static void adc_adv_dump_samples(rt_device_t dev)
{
    ns_adc_adv_sample_t samples[NS_ADC_ADV_BUFFER_MAX];
    rt_size_t count;
    rt_size_t i;

    /* 读取并打印 nsadc0 最近一次高级采样结果。 */
    count = rt_device_read(dev, 0, samples, NS_ADC_ADV_BUFFER_MAX);
    rt_kprintf("nsadc0 samples: %u\r\n", (unsigned int)count);

    for (i = 0; i < count; i++)
    {
        rt_kprintf("  [%u] unit=%u soc=%u adcin=%u raw=%u\r\n",
                   (unsigned int)i,
                   (unsigned int)samples[i].unit_id,
                   (unsigned int)samples[i].soc,
                   (unsigned int)samples[i].adc_channel,
                   (unsigned int)samples[i].raw);
    }
}

static ns_adc_unit_id adc_demo_unit_id(int unit)
{
    if ((unit == 'b') || (unit == 'B') || (unit == '1'))
    {
        return NS_ADC_UNIT_B;
    }
    if ((unit == 'c') || (unit == 'C') || (unit == '2'))
    {
        return NS_ADC_UNIT_C;
    }

    return NS_ADC_UNIT_A;
}

static int adc_demo_atoi(const char *s)
{
    int sign = 1;
    int value = 0;

    if (s == RT_NULL)
    {
        return 0;
    }

    if (*s == '-')
    {
        sign = -1;
        s++;
    }

    while ((*s >= '0') && (*s <= '9'))
    {
        value = value * 10 + (*s - '0');
        s++;
    }

    return value * sign;
}

static void adc_adv_dump_status(rt_device_t dev)
{
    ns_adc_adv_status_t status;

    /* 打印高级设备通用状态；可选模块状态由各自命令单独查询。 */
    if (rt_device_control(dev, NS_ADC_ADV_CMD_GET_STATUS, &status) != RT_EOK)
    {
        rt_kprintf("nsadc0 get status failed\r\n");
        return;
    }

    rt_kprintf("nsadc0 status: mode=%d running=%d ready=%u\r\n",
               status.mode,
               status.running,
               status.ready_count);
}

static void adc_adv_dump_ppb(rt_device_t dev)
{
    ns_adc_ppb_status_t status;

    /* PPB 是可选高级模块，状态通过独立 PPB 命令查询。 */
    if (rt_device_control(dev, NS_ADC_ADV_CMD_GET_PPB_STATUS, &status) != RT_EOK)
    {
        rt_kprintf("nsadc0 get ppb status failed\r\n");
        return;
    }

    rt_kprintf("nsadc0 ppb: evt=0x%x delay=%u\r\n",
               status.event_flags,
               status.delay_stamp);
}

static rt_err_t adc_adv_control(rt_device_t dev, int cmd, void *arg, const char *step)
{
    rt_err_t ret;

    /* 统一检查 nsadc0 control 返回值，避免把配置失败误判为无样本。 */
    ret = rt_device_control(dev, cmd, arg);
    if (ret != RT_EOK)
    {
        rt_kprintf("nsadc0 %s failed: %d\r\n", step, ret);
    }

    return ret;
}

int adc_adv_irq(void)
{
    rt_device_t dev;
    ns_adc_adv_irq_config_t cfg =
    {
        .sequence =
        {
            .unit_id = NS_ADC_UNIT_A,
            .slots =
            {
                {ADC_SOC_NUMBER0, ADC_TRIGGER_SW_ONLY, ADC_CH_ADCIN0, 8U, ADC_INT_SOC_TRIGGER_NONE},
                {ADC_SOC_NUMBER1, ADC_TRIGGER_SW_ONLY, ADC_CH_ADCIN1, 8U, ADC_INT_SOC_TRIGGER_NONE},
                {ADC_SOC_NUMBER2, ADC_TRIGGER_SW_ONLY, ADC_CH_ADCIN2, 8U, ADC_INT_SOC_TRIGGER_NONE},
                {ADC_SOC_NUMBER3, ADC_TRIGGER_SW_ONLY, ADC_CH_ADCIN3, 8U, ADC_INT_SOC_TRIGGER_NONE},
            },
            .slot_count = 4U,
            .sample_count = 4U,
            .eoc_int = ADC_INT_NUMBER1,
        },
    };

    /* 软件触发 4 通道中断采样，用于验证 ADC EOC IRQ 链路。 */
    dev = adc_adv_get_device();
    if (dev == RT_NULL)
    {
        return -RT_ERROR;
    }

    if (adc_adv_control(dev, NS_ADC_ADV_CMD_CONFIG_IRQ, &cfg, "config irq") != RT_EOK)
    {
        return -RT_ERROR;
    }
    if (adc_adv_control(dev, NS_ADC_ADV_CMD_START_IRQ, RT_NULL, "start irq") != RT_EOK)
    {
        rt_device_close(dev);
        return -RT_ERROR;
    }

    adc_adv_dump_samples(dev);
    rt_device_close(dev);
    return 0;
}

int adc_adv_dma(void)
{
    rt_device_t dev;
    ns_adc_adv_dma_config_t cfg =
    {
        .unit_id = NS_ADC_UNIT_A,
        .soc = ADC_SOC_NUMBER0,
        .adc_channel = ADC_CH_ADCIN0,
        .sample_count = 8U,
        .edma_channel = 0U,
    };

    /* 使用 EDMA1 CH0 搬运 ADCA channel0 的 8 个 raw 样本。 */
    dev = adc_adv_get_device();
    if (dev == RT_NULL)
    {
        return -RT_ERROR;
    }

    if (adc_adv_control(dev, NS_ADC_ADV_CMD_CONFIG_DMA, &cfg, "config dma") != RT_EOK)
    {
        return -RT_ERROR;
    }
    if (adc_adv_control(dev, NS_ADC_ADV_CMD_START_DMA, RT_NULL, "start dma") != RT_EOK)
    {
        rt_device_close(dev);
        return -RT_ERROR;
    }

    adc_adv_dump_samples(dev);
    rt_device_close(dev);
    return 0;
}

int adc_adv_burst(void)
{
    rt_device_t dev;
    ns_adc_adv_burst_config_t cfg =
    {
        .unit_id = NS_ADC_UNIT_A,
        .first_soc = ADC_SOC_NUMBER7,
        .adc_channels = {ADC_CH_ADCIN0, ADC_CH_ADCIN1, ADC_CH_ADCIN2},
        .channel_count = 3U,
        .burst_size = 3U,
        .trigger = ADC_BURSTTRIG5_EPWM1_SOCA,
        .epwm_period = 2599U,
        .epwm_compare = 1300U,
    };

    /* 使用 EPWM1 SOCA 触发 ADCA Burst，默认一次 burst 采 3 个 SOC。 */
    dev = adc_adv_get_device();
    if (dev == RT_NULL)
    {
        return -RT_ERROR;
    }

    if (adc_adv_control(dev, NS_ADC_ADV_CMD_CONFIG_BURST, &cfg, "config burst") != RT_EOK)
    {
        return -RT_ERROR;
    }
    if (adc_adv_control(dev, NS_ADC_ADV_CMD_START_BURST, RT_NULL, "start burst") != RT_EOK)
    {
        rt_device_close(dev);
        return -RT_ERROR;
    }

    adc_adv_dump_samples(dev);
    rt_device_close(dev);
    return 0;
}

int adc_adv_burst_dma(void)
{
    rt_device_t dev;
    ns_adc_adv_burst_dma_config_t cfg =
    {
        .burst =
        {
            .unit_id = NS_ADC_UNIT_A,
            .first_soc = ADC_SOC_NUMBER7,
            .adc_channels = {ADC_CH_ADCIN0, ADC_CH_ADCIN1, ADC_CH_ADCIN2},
            .channel_count = 3U,
            .burst_size = 3U,
            .trigger = ADC_BURSTTRIG5_EPWM1_SOCA,
            .epwm_period = 2599U,
            .epwm_compare = 1300U,
        },
        .edma_channel = 0U,
    };

    /*
     * Burst+DMA 当前先完成配置链路验证。
     * 后续可在驱动中把 START_BURST_DMA 拆成独立命令并加入 DMA TCD 链式搬运。
     */
    dev = adc_adv_get_device();
    if (dev == RT_NULL)
    {
        return -RT_ERROR;
    }

    if (adc_adv_control(dev, NS_ADC_ADV_CMD_CONFIG_BURST_DMA, &cfg, "config burst dma") != RT_EOK)
    {
        rt_device_close(dev);
        return -RT_ERROR;
    }

    adc_adv_dump_status(dev);
    rt_device_close(dev);
    return 0;
}

int adc_adv_ppb(void)
{
    rt_device_t dev;
    ns_adc_ppb_limit_config_t cfg =
    {
        .unit_id = NS_ADC_UNIT_A,
        .ppb = ADC_PPB_NUMBER1,
        .soc = ADC_SOC_NUMBER0,
        .high_limit = 3000U,
        .low_limit = 1000U,
        .enable_event_irq = RT_TRUE,
    };

    /* 将 PPB1 绑定到 channel0，配置 high/low 事件并读取状态。 */
    dev = adc_adv_get_device();
    if (dev == RT_NULL)
    {
        return -RT_ERROR;
    }

    if (adc_adv_control(dev, NS_ADC_ADV_CMD_CONFIG_PPB_LIMIT, &cfg, "config ppb") != RT_EOK)
    {
        rt_device_close(dev);
        return -RT_ERROR;
    }

    adc_adv_dump_ppb(dev);
    rt_device_close(dev);
    return 0;
}

int adc_unit_read(int argc, char **argv)
{
    ns_adc_unit *adc;
    ns_adc_slot_cfg slot;
    rt_uint32_t raw = 0U;
    int unit;
    int soc;
    int adcin;
    ns_adc_unit_id id;

    if (argc != 4)
    {
        rt_kprintf("usage: adc_unit_read <a|b|c> <soc 0-31> <adcin 0-31>\r\n");
        return -RT_EINVAL;
    }

    unit = argv[1][0];
    soc = adc_demo_atoi(argv[2]);
    adcin = adc_demo_atoi(argv[3]);
    id = adc_demo_unit_id(unit);

    if ((soc < 0) || (soc > 31) || (adcin < 0) || (adcin > 31))
    {
        rt_kprintf("usage: adc_unit_read <a|b|c> <soc 0-31> <adcin 0-31>\r\n");
        return -RT_EINVAL;
    }

    adc = ns_adc_get(id);
    if (adc == RT_NULL)
    {
        return -RT_ERROR;
    }

    slot.soc = (ADC_SOCNumber)soc;
    slot.trigger = ADC_TRIGGER_SW_ONLY;
    slot.adc_channel = (ADC_Channel)adcin;
    slot.sample_window = 8U;
    slot.int_trigger = ADC_INT_SOC_TRIGGER_NONE;

    if (ns_adc_once(adc, &slot, ADC_INT_NUMBER1, &raw) != RT_EOK)
    {
        rt_kprintf("adc_unit_read failed\r\n");
        return -RT_ERROR;
    }

    rt_kprintf("adc unit=%u soc=%d adcin=%d raw=%u\r\n", id, soc, adcin, raw);
    return 0;
}

int adc_unit_scan(int argc, char **argv)
{
    ns_adc_unit_id id;
    ns_adc_unit *adc;
    ns_adc_slot_cfg slot;
    rt_uint32_t raw;
    int unit;
    int first_soc;
    int adcin_start;
    int count;
    int i;

    if (argc != 5)
    {
        rt_kprintf("usage: adc_unit_scan <a|b|c> <first_soc 0-31> <adcin_start 0-31> <count>\r\n");
        return -RT_EINVAL;
    }

    unit = argv[1][0];
    first_soc = adc_demo_atoi(argv[2]);
    adcin_start = adc_demo_atoi(argv[3]);
    count = adc_demo_atoi(argv[4]);
    id = adc_demo_unit_id(unit);

    if ((first_soc < 0) || (first_soc > 31) ||
        (adcin_start < 0) || (adcin_start > 31) ||
        (count <= 0) || ((first_soc + count) > 32) || ((adcin_start + count) > 32))
    {
        rt_kprintf("usage: adc_unit_scan <a|b|c> <first_soc 0-31> <adcin_start 0-31> <count>\r\n");
        return -RT_EINVAL;
    }

    adc = ns_adc_get(id);
    if (adc == RT_NULL)
    {
        return -RT_ERROR;
    }

    for (i = 0; i < count; i++)
    {
        slot.soc = (ADC_SOCNumber)(first_soc + i);
        slot.trigger = ADC_TRIGGER_SW_ONLY;
        slot.adc_channel = (ADC_Channel)(adcin_start + i);
        slot.sample_window = 8U;
        slot.int_trigger = ADC_INT_SOC_TRIGGER_NONE;
        raw = 0U;

        if (ns_adc_once(adc, &slot, ADC_INT_NUMBER1, &raw) != RT_EOK)
        {
            rt_kprintf("scan failed at soc=%d adcin=%d\r\n", first_soc + i, adcin_start + i);
            return -RT_ERROR;
        }

        rt_kprintf("adc unit=%u soc=%d adcin=%d raw=%u\r\n", id, first_soc + i, adcin_start + i, raw);
    }

    return 0;
}

int adc_ppb_cfg(int argc, char **argv)
{
    rt_device_t dev;
    ns_adc_ppb_limit_config_t cfg;
    int unit;
    int ppb;
    int soc;
    int low;
    int high;

    if (argc != 6)
    {
        rt_kprintf("usage: adc_ppb_cfg <a|b|c> <ppb 1-4> <soc 0-31> <low> <high>\r\n");
        return -RT_EINVAL;
    }

    unit = argv[1][0];
    ppb = adc_demo_atoi(argv[2]);
    soc = adc_demo_atoi(argv[3]);
    low = adc_demo_atoi(argv[4]);
    high = adc_demo_atoi(argv[5]);

    if ((ppb < 1) || (ppb > 4) || (soc < 0) || (soc > 31))
    {
        rt_kprintf("usage: adc_ppb_cfg <a|b|c> <ppb 1-4> <soc 0-31> <low> <high>\r\n");
        return -RT_EINVAL;
    }

    dev = adc_adv_get_device();
    if (dev == RT_NULL)
    {
        return -RT_ERROR;
    }

    cfg.unit_id = adc_demo_unit_id(unit);
    cfg.ppb = (ADC_PPBNumber)(ppb - 1);
    cfg.soc = (ADC_SOCNumber)soc;
    cfg.low_limit = (rt_uint32_t)low;
    cfg.high_limit = (rt_uint32_t)high;
    cfg.enable_event_irq = RT_TRUE;

    if (adc_adv_control(dev, NS_ADC_ADV_CMD_CONFIG_PPB_LIMIT, &cfg, "config explicit ppb") != RT_EOK)
    {
        rt_device_close(dev);
        return -RT_ERROR;
    }

    adc_adv_dump_ppb(dev);
    rt_device_close(dev);
    return 0;
}

int adc_adv_stop(void)
{
    rt_device_t dev;

    /* 停止高级模式，关闭本驱动启用的 IRQ/EPWM 触发，并恢复基础 ADC 配置。 */
    dev = adc_adv_get_device();
    if (dev == RT_NULL)
    {
        return -RT_ERROR;
    }

    if (adc_adv_control(dev, NS_ADC_ADV_CMD_STOP, RT_NULL, "stop") != RT_EOK)
    {
        rt_device_close(dev);
        return -RT_ERROR;
    }

    rt_kprintf("nsadc0 stopped\r\n");
    rt_device_close(dev);
    return 0;
}

#ifdef RT_USING_FINSH
MSH_CMD_EXPORT(adc_adv_irq, nsadc0 interrupt sampling demo);
MSH_CMD_EXPORT(adc_adv_dma, nsadc0 dma sampling demo);
MSH_CMD_EXPORT(adc_adv_burst, nsadc0 burst sampling demo);
MSH_CMD_EXPORT(adc_adv_burst_dma, nsadc0 burst dma config demo);
MSH_CMD_EXPORT(adc_adv_ppb, nsadc0 ppb limit demo);
MSH_CMD_EXPORT(adc_adv_stop, stop nsadc0 advanced mode);
MSH_CMD_EXPORT(adc_unit_read, read explicit adc unit soc adcin);
MSH_CMD_EXPORT(adc_unit_scan, scan explicit adc unit soc/adcin range);
MSH_CMD_EXPORT(adc_ppb_cfg, config explicit adc ppb limit);
#endif

#endif
