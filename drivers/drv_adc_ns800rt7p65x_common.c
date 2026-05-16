/**
 * @file drv_adc_ns800rt7p65x_common.c
 * @brief NS800RT7P65x ADC 标准驱动与高级驱动共用的硬件工具层。
 *
 * 文件说明：
 * 本文件只处理芯片 ADC 的基础能力：通道映射、ADC unit 初始化、
 * SOC 软件触发、EOC 等待、结果读取和中断标志清理。标准 rt_adc_ops
 * 与高级 nsadc0 设备都通过这里访问底层硬件，从而降低耦合。
 */

#include "drv_adc_ns800rt7p65x_common.h"

static rt_bool_t ns_adc_inited = RT_FALSE;

static const ns_adc_channel_map_t ns_adc_channels[NS_ADC_CHANNEL_COUNT] =
{
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER0, ADC_CH_ADCIN0},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER1, ADC_CH_ADCIN1},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER2, ADC_CH_ADCIN2},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER3, ADC_CH_ADCIN3},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER4, ADC_CH_ADCIN4},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER5, ADC_CH_ADCIN5},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER6, ADC_CH_ADCIN6},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER7, ADC_CH_ADCIN7},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER8, ADC_CH_ADCIN8},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER9, ADC_CH_ADCIN9},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER10, ADC_CH_ADCIN10},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER11, ADC_CH_ADCIN11},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER12, ADC_CH_ADCIN12},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER13, ADC_CH_ADCIN13},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER14, ADC_CH_ADCIN14},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER15, ADC_CH_ADCIN15},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER16, ADC_CH_ADCIN16},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER17, ADC_CH_ADCIN17},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER18, ADC_CH_ADCIN18},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER19, ADC_CH_ADCIN19},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER20, ADC_CH_ADCIN20},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER21, ADC_CH_ADCIN21},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER22, ADC_CH_ADCIN22},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER23, ADC_CH_ADCIN23},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER24, ADC_CH_ADCIN24},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER25, ADC_CH_ADCIN25},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER26, ADC_CH_ADCIN26},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER27, ADC_CH_ADCIN27},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER28, ADC_CH_ADCIN28},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER29, ADC_CH_ADCIN29},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER30, ADC_CH_ADCIN30},
    {NS_ADC_UNIT_A, ADC_SOC_NUMBER31, ADC_CH_ADCIN31},
};

rt_err_t ns_adc_check_channel(rt_int8_t channel)
{
    /* 统一检查通道号，避免访问映射表越界。 */
    if ((channel < 0) || (channel >= (rt_int8_t)NS_ADC_CHANNEL_COUNT))
    {
        return -RT_EINVAL;
    }

    return RT_EOK;
}

const ns_adc_channel_map_t *ns_adc_get_channel(rt_uint32_t channel)
{
    /* 返回指定 RT-Thread channel 对应的 ADC unit/SOC/ADCIN 映射。 */
    if (channel >= NS_ADC_CHANNEL_COUNT)
    {
        return RT_NULL;
    }

    return &ns_adc_channels[channel];
}

ns_adc_unit *ns_adc_get_channel_unit(const ns_adc_channel_map_t *map)
{
    /* 将静态映射中的 unit id 转换为可操作的 ADC unit 实例。 */
    if (map == RT_NULL)
    {
        return RT_NULL;
    }

    return ns_adc_get(map->unit_id);
}

static rt_err_t ns_adc_config_default_map(void)
{
    ns_adc_slot_cfg slots[NS_ADC_CHANNEL_COUNT];
    ns_adc_seq_cfg seq_cfg;
    rt_uint32_t i;

    /*
     * adc0 的默认 32 通道视图：
     * RT channel N -> ADCA SOCN -> ADCINN。
     * 这只是 demo 默认映射，开发者仍可在高级设备中显式选择 unit/SOC/ADCIN。
     */
    for (i = 0U; i < NS_ADC_CHANNEL_COUNT; i++)
    {
        slots[i].soc = ns_adc_channels[i].soc;
        slots[i].trigger = ADC_TRIGGER_SW_ONLY;
        slots[i].adc_channel = ns_adc_channels[i].adc_channel;
        slots[i].sample_window = NS_ADC_SAMPLE_WINDOW;
        slots[i].int_trigger = ADC_INT_SOC_TRIGGER_NONE;
    }

    seq_cfg.unit_id = NS_ADC_UNIT_A;
    seq_cfg.slots = slots;
    seq_cfg.slot_count = NS_ADC_CHANNEL_COUNT;
    seq_cfg.eoc_int = ADC_INT_NUMBER1;
    return ns_adc_seq(&seq_cfg);
}

rt_err_t ns_adc_hw_init(void)
{
    /*
     * ADC 基础硬件初始化只执行一次。
     * 默认将 adc0 的 32 个 RT channel 映射到 ADCA 的 32 个 SOC。
     */
    if (ns_adc_inited == RT_TRUE)
    {
        return RT_EOK;
    }

    if (ns_adc_config_default_map() != RT_EOK)
    {
        return -RT_ERROR;
    }
    ns_adc_inited = RT_TRUE;
    return RT_EOK;
}

rt_err_t ns_adc_hw_reset(void)
{
    /*
     * 高级模式停止后显式恢复 adc0 的基础 SOC 配置。
     * 与 ns_adc_hw_init() 不同，本函数会重新写入 ADC 单元寄存器。
     */
    ns_adc_unit *unit;

    for (rt_uint32_t i = 0U; i < NS_ADC_UNIT_MAX; i++)
    {
        unit = ns_adc_get((ns_adc_unit_id)i);
        if ((unit != RT_NULL) && (unit->cfg != RT_NULL))
        {
            ns_adc_init(unit, unit->cfg);
        }
    }

    ns_adc_inited = RT_FALSE;
    return ns_adc_hw_init();
}

uint32_t ns_adc_force_mask(ADC_SOCNumber soc)
{
    /*
     * SDK 已提供 ADC_FORCE_SOCx 位定义，SOC 编号本身连续。
     * 这里保留范围检查，避免错误 SOC 形成未定义移位。
     */
    if (soc > ADC_SOC_NUMBER31)
    {
        return 0U;
    }

    return ns_adc_mask(soc);
}

rt_err_t ns_adc_wait_eoc(ADC_TypeDef *base, ADC_IntNumber int_number)
{
    rt_uint32_t timeout = NS_ADC_EOC_TIMEOUT_MS;

    /* 轮询等待指定 ADCINT 标志置位，随后清除标志。 */
    while (ADC_getInterruptStatus(base, int_number) == false)
    {
        if (timeout == 0U)
        {
            return -RT_ETIMEOUT;
        }

        timeout--;
        rt_thread_mdelay(1);
    }

    ns_adc_clear_interrupt(base, int_number);
    return RT_EOK;
}

rt_uint32_t ns_adc_read_channel_raw(rt_uint32_t channel)
{
    const ns_adc_channel_map_t *map;

    /* 读取已完成转换的 raw 结果；调用者负责保证转换已经完成。 */
    map = ns_adc_get_channel(channel);
    if (map == RT_NULL)
    {
        return 0U;
    }

    return ns_adc_read(ns_adc_get_channel_unit(map), map->soc);
}

void ns_adc_clear_interrupt(ADC_TypeDef *base, ADC_IntNumber int_number)
{
    /* 清除 ADCINT 标志；如果发生 overflow，同步清除 overflow 后再清一次标志。 */
    ADC_clearInterruptStatus(base, int_number);

    if (ADC_getInterruptOverflowStatus(base, int_number) == true)
    {
        ADC_clearInterruptOverflowStatus(base, int_number);
        ADC_clearInterruptStatus(base, int_number);
    }
}

rt_err_t ns_adc_channel_to_slot(rt_uint32_t channel, ns_adc_slot_cfg *slot)
{
    const ns_adc_channel_map_t *map;

    if (slot == RT_NULL)
    {
        return -RT_EINVAL;
    }

    map = ns_adc_get_channel(channel);
    if (map == RT_NULL)
    {
        return -RT_EINVAL;
    }

    slot->soc = map->soc;
    slot->trigger = ADC_TRIGGER_SW_ONLY;
    slot->adc_channel = map->adc_channel;
    slot->sample_window = NS_ADC_SAMPLE_WINDOW;
    slot->int_trigger = ADC_INT_SOC_TRIGGER_NONE;
    return RT_EOK;
}
