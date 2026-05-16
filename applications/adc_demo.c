/**
 * @file adc_demo.c
 * @brief ADC 周期采样线程和 FinSH 手动采样命令。
 *
 * 文件说明：
 * 本文件是应用层示例，只依赖 rtthread.h 和 RT-Thread 官方 rtdevice.h。
 * 它不直接访问 NOVOSENSE ADC 底层驱动，而是通过标准 rt_adc_* API
 * 访问 adc0，用于验证本地 ADC device framework 和芯片驱动适配层。
 */

#include "rtthread.h"
#include "rtdevice.h"

#ifdef RT_USING_FINSH
#include "finsh.h"
#endif

#define ADC_DEMO_DEVICE_NAME "adc0"
#define ADC_DEMO_CHANNEL_COUNT 4

static void adc_print_sample(const rt_uint32_t raw[ADC_DEMO_CHANNEL_COUNT])
{
    /* 统一打印 4 路 raw 结果，便于线程周期采样和 FinSH 手动采样复用。 */
    rt_kprintf("adc0 raw: ch0=%u ch1=%u ch2=%u ch3=%u\r\n",
               raw[0],
               raw[1],
               raw[2],
               raw[3]);
}

static rt_err_t adc_sample_once(void)
{
    rt_adc_device_t dev;
    rt_uint32_t raw[ADC_DEMO_CHANNEL_COUNT];
    rt_size_t read_size;
    rt_int8_t channel;

    /*
     * 手动采样一次：
     * 每个通道临时 enable -> read -> disable，证明单次 API 调用链完整可用。
     */
    dev = (rt_adc_device_t)rt_device_find(ADC_DEMO_DEVICE_NAME);
    if (dev == RT_NULL)
    {
        rt_kprintf("adc0 not found\r\n");
        return -RT_ERROR;
    }

    if (rt_device_open((rt_device_t)dev, RT_DEVICE_OFLAG_RDONLY) != RT_EOK)
    {
        rt_kprintf("adc0 open failed\r\n");
        return -RT_ERROR;
    }

    for (channel = 0; channel < ADC_DEMO_CHANNEL_COUNT; channel++)
    {
        if (rt_adc_enable(dev, channel) != RT_EOK)
        {
            rt_device_close((rt_device_t)dev);
            rt_kprintf("adc0 channel %d enable failed\r\n", channel);
            return -RT_ERROR;
        }

        /*
         * RT-Thread v4.1.x 的 rt_adc_read() 不返回错误码，convert 失败时
         * 可能把未初始化 value 当作 raw 返回。这里用 rt_device_read()
         * 检查实际读取长度，避免把超时或硬件错误打印成看似有效的 ADC 值。
         */
        raw[channel] = 0U;
        read_size = rt_device_read((rt_device_t)dev,
                                   channel,
                                   &raw[channel],
                                   sizeof(raw[channel]));
        if (read_size != sizeof(raw[channel]))
        {
            rt_adc_disable(dev, channel);
            rt_device_close((rt_device_t)dev);
            rt_kprintf("adc0 channel %d read failed\r\n", channel);
            return -RT_ETIMEOUT;
        }

        rt_adc_disable(dev, channel);
    }

    rt_device_close((rt_device_t)dev);
    adc_print_sample(raw);
    return RT_EOK;
}

int adc_sample(void)
{
    /* FinSH 命令入口：手动触发一次 4 通道采样。 */
    return (int)adc_sample_once();
}

#ifdef RT_USING_FINSH
MSH_CMD_EXPORT(adc_sample, sample adc0 once);
#endif
