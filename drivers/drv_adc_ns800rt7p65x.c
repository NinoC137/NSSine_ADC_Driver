/**
 * @file drv_adc_ns800rt7p65x.c
 * @brief NS800RT7P65x ADC 的 RT-Thread ADC device 适配层。
 *
 * 文件说明：
 * 本文件负责把 NOVOSENSE SDK 提供的 ADC 底层函数封装为
 * struct rt_adc_ops，并注册为 RT-Thread ADC 设备 adc0。
 * 应用层通过 rt_adc_enable/read/disable/voltage 使用 ADC，
 * 不需要了解 ADCA/ADCC、SOC 编号或厂商寄存器接口。
 */

#include "drv_adc_ns800rt7p65x.h"
#include "ns_adc_irq.h"

#ifdef RT_USING_FINSH
#include "finsh.h"
#endif

static struct rt_adc_device ns_adc_device;
static rt_uint32_t ns_adc_enabled_mask = 0;

static rt_err_t ns_adc_enabled(struct rt_adc_device *device, rt_uint32_t channel, rt_bool_t enabled)
{
    RT_UNUSED(device);

    /*
     * RT-Thread ADC framework 会先 enable 通道再 read。
     * 对当前软件触发 demo 来说，enable 不需要额外配置 SOC，只记录通道状态。
     */
    if (channel >= NS_ADC_CHANNEL_COUNT)
    {
        return -RT_EINVAL;
    }

    ns_adc_hw_init();

    if (enabled == RT_TRUE)
    {
        ns_adc_enabled_mask |= (1UL << channel);
    }
    else
    {
        ns_adc_enabled_mask &= ~(1UL << channel);
    }

    return RT_EOK;
}

static rt_err_t ns_adc_convert(struct rt_adc_device *device, rt_uint32_t channel, rt_uint32_t *value)
{
    RT_UNUSED(device);

    if ((value == RT_NULL) || (channel >= NS_ADC_CHANNEL_COUNT))
    {
        return -RT_EINVAL;
    }

    if ((ns_adc_enabled_mask & (1UL << channel)) == 0)
    {
        return -RT_ERROR;
    }

    if (ns_adc_read_channel_once(channel, value) != RT_EOK)
    {
        return -RT_ETIMEOUT;
    }

    return RT_EOK;
}

static rt_uint8_t ns_adc_get_resolution(struct rt_adc_device *device)
{
    /* 当前按 12 bit ADC raw 值处理。 */
    RT_UNUSED(device);
    return NS_ADC_RESOLUTION_BITS;
}

static rt_int16_t ns_adc_get_vref(struct rt_adc_device *device)
{
    /* 当前默认内部参考电压为 3300 mV。 */
    RT_UNUSED(device);
    return NS_ADC_REFERENCE_MV;
}

static const struct rt_adc_ops ns_adc_ops =
{
    ns_adc_enabled,
    ns_adc_convert,
    ns_adc_get_resolution,
    ns_adc_get_vref,
};

int rt_hw_ns_adc_init(void)
{
    /*
     * RT-Thread 组件初始化入口。
     * 这里先完成硬件初始化，再把 ns_adc_device 注册为 adc0。
     */
    rt_memset(&ns_adc_device, 0, sizeof(ns_adc_device));
    ns_adc_hw_init();

    return rt_hw_adc_register(&ns_adc_device,
                              NS_ADC_DEVICE_NAME,
                              &ns_adc_ops,
                              RT_NULL);
}

INIT_DEVICE_EXPORT(rt_hw_ns_adc_init);

static ns_adc_unit_id ns_adc_regs_unit_id(const char *name)
{
    if ((name != RT_NULL) && ((name[0] == 'b') || (name[0] == 'B') || (name[0] == '1')))
    {
        return NS_ADC_UNIT_B;
    }
    if ((name != RT_NULL) && ((name[0] == 'c') || (name[0] == 'C') || (name[0] == '2')))
    {
        return NS_ADC_UNIT_C;
    }

    return NS_ADC_UNIT_A;
}

static int adc_regs(int argc, char **argv)
{
    /*
     * ADC 基础诊断命令：
     * 直接打印指定 ADC unit 的关键控制、SOC、INT、RESULT 状态，用于定位
     * 软件触发后到底是 SOC 未触发、ADCINT 未置位，还是结果读取异常。
     */
    ns_adc_unit_id id;
    ns_adc_unit *unit;

    ns_adc_hw_init();
    id = (argc >= 2) ? ns_adc_regs_unit_id(argv[1]) : NS_ADC_UNIT_A;
    unit = ns_adc_get(id);
    if ((unit == RT_NULL) || (unit->cfg == RT_NULL))
    {
        rt_kprintf("adc unit not found\r\n");
        return -RT_ERROR;
    }

    rt_kprintf("ADC%s CTL1=0x%08x INTFLG=0x%08x INTOVF=0x%08x SOC0=0x%08x SOC1=0x%08x RES0=%u RES1=%u\r\n",
               unit->cfg->name,
               unit->cfg->base->CTL1.WORDVAL,
               unit->cfg->base->INTFLG.WORDVAL,
               unit->cfg->base->INTOVF.WORDVAL,
               unit->cfg->base->SOC0CTL.WORDVAL,
               unit->cfg->base->SOC1CTL.WORDVAL,
               ADC_readResult(unit->cfg->result, ADC_SOC_NUMBER0),
               ADC_readResult(unit->cfg->result, ADC_SOC_NUMBER1));

    ns_adc_trigger(unit, ADC_FORCE_SOC0);
    rt_thread_mdelay(2);
    rt_kprintf("ADC%s after force SOC0: INTFLG=0x%08x INTOVF=0x%08x RES0=%u\r\n",
               unit->cfg->name,
               unit->cfg->base->INTFLG.WORDVAL,
               unit->cfg->base->INTOVF.WORDVAL,
               ADC_readResult(unit->cfg->result, ADC_SOC_NUMBER0));

    ns_adc_clear(unit, ADC_INT_NUMBER1);
    return 0;
}

#ifdef RT_USING_FINSH
MSH_CMD_EXPORT(adc_regs, dump adc registers);
#endif
