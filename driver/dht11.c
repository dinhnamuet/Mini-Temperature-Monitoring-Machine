#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <linux/of.h>
/* time ms */
#define TIME_START		            19
/* time ns */
#define TIME_RESPONSE_MAX	        90000
#define TIM_WAITING_DATA_MAX		60000
#define TIM_DATA_BIT0_MAX	        40000
#define TIM_DATA_BIT1_MAX	        90000
#define TIMEOUT_WAIT_RESPONSE_MAX   60000
/* GPIO Logic Voltage */
#define LOW                         0x00
#define HIGH                        0x01
/* Bit access */
#define set(var, bit) var |= (1UL << bit)
#define reset(var, bit) var &= ~(1UL << bit)
/* Private data */
#define ROUDING_FACTOR				100U

typedef union {
    u8 array[4];
    struct {
        u8 integer_hum;
        u8 decimal_hum; 
        u8 integer_temp;
        u8 decimal_temp;
    } bytes_data;
} dht_data_t;

struct dht_sensor {
    struct iio_dev *iio;
    struct gpio_desc *gpio;
    struct mutex lock;
    struct device *dev;
};

static int dht_read(struct iio_dev *indio_dev, struct iio_chan_spec const *chan, int *val, int *val2, long mask)
{
    int ret, i, j;
    u8 checksum = 0, data[5];
    s64 start_time;
    dht_data_t res;
    struct dht_sensor *dht = iio_priv(indio_dev);
    if (IS_ERR(dht)) {
        dev_err(indio_dev->dev.parent, "%s - Cannot get Device Instance!\n", __func__);
        return -ENODEV;
    }
    ret = mutex_lock_interruptible(&dht->lock);
    if (ret < 0) {
        dev_err(indio_dev->dev.parent, "%s - Cannot Take Mutex!\n", __func__);
        goto err;
    }
    /* Start Condition */
    ret = gpiod_direction_output(dht->gpio, HIGH);
    if (ret) {
        dev_err(indio_dev->dev.parent, "%s - Cannot set gpio %d as output!\n", __func__, desc_to_gpio(dht->gpio));
        goto proc_err;
    }
    gpiod_set_value(dht->gpio, LOW);
    mdelay(TIME_START);
    gpiod_set_value(dht->gpio, HIGH);
    /* Wait DHT Response */
    ret = gpiod_direction_input(dht->gpio);
    if (ret) {
        dev_err(indio_dev->dev.parent, "%s - Cannot set gpio %d as input!\n", __func__, desc_to_gpio(dht->gpio));
        goto proc_err;
    }
    start_time = iio_get_time_ns(indio_dev);
    while (gpiod_get_value(dht->gpio)) {
        if (iio_get_time_ns(indio_dev) - start_time > TIMEOUT_WAIT_RESPONSE_MAX) {
            dev_err(dht->dev, "%s - timeout, line %d\n", __func__, __LINE__);
            ret = -EIO;
            goto proc_err;
    	};
    }
    /* Response */
    start_time = iio_get_time_ns(indio_dev);
    while (!gpiod_get_value(dht->gpio)) {
        if (iio_get_time_ns(indio_dev) - start_time >= TIME_RESPONSE_MAX) {
            dev_err(dht->dev, "%s - timeout, line %d\n", __func__, __LINE__);
            ret = -EIO;
            goto proc_err;
    	};
    }
    start_time = iio_get_time_ns(indio_dev);
    while (gpiod_get_value(dht->gpio)) {
        if (iio_get_time_ns(indio_dev) - start_time >= TIME_RESPONSE_MAX) {
            dev_err(dht->dev, "%s - timeout, line %d\n", __func__, __LINE__);
            ret = -EIO;
            goto proc_err;
    	};
    }
    /* Data Reading */
    for (i = 0; i < 5; i ++) {
        for (j = 7; j >= 0; j--) {
            start_time = iio_get_time_ns(indio_dev);
            while (!gpiod_get_value(dht->gpio)) {
                if(iio_get_time_ns(indio_dev) - start_time > TIM_WAITING_DATA_MAX) {
                    ret = -EIO;
                    goto proc_err;
                }
            }
            start_time = iio_get_time_ns(indio_dev);
            while (gpiod_get_value(dht->gpio)) {
                if(iio_get_time_ns(indio_dev) - start_time > TIM_DATA_BIT1_MAX) {
                    ret = -EIO;
                    goto proc_err;
                }
            }
            if (iio_get_time_ns(indio_dev) - start_time < TIM_DATA_BIT0_MAX)
                reset(data[i], j);
            else
                set(data[i], j);
        }
    }
    /* CheckSum */
    for (int i = 0; i < 4; i++) {
        checksum += data[i];
    }
    if ((checksum & 0xFF) != data[4]) {
        ret = -EFAULT;
        dev_err(dht->dev, "%s - CheckSum failed!\n", __func__);
        goto proc_err;
    }
    /* Data Decoding */
	memcpy(res.array, data, 4);
    ret = IIO_VAL_FRACTIONAL;
	if (chan->type == IIO_TEMP) {
		*val = (u32)((u32)res.bytes_data.integer_temp * ROUDING_FACTOR) + (u32)res.bytes_data.decimal_temp;
	    *val2 = ROUDING_FACTOR;
    } else if (chan->type == IIO_HUMIDITYRELATIVE) {
		*val = (u32)((u32)res.bytes_data.integer_hum * ROUDING_FACTOR) + (u32)res.bytes_data.decimal_hum;
	    *val2 = ROUDING_FACTOR;
    } else {
		ret = -EINVAL;
    }

proc_err:
	mutex_unlock(&dht->lock);
err:
	return ret;
}

static struct iio_chan_spec dht_channels[] = {
    {	.type = IIO_TEMP,
        .info_mask_separate = BIT(IIO_CHAN_INFO_PROCESSED), 
		.extend_name = "hnam02", },
    { 	.type = IIO_HUMIDITYRELATIVE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_PROCESSED), 
		.extend_name = "hnam02", },
    { }
};

static struct iio_info dht_info = {
    .read_raw = dht_read,
};

static int dht_probe(struct platform_device *pdev)
{
    struct dht_sensor *dht;
    struct iio_dev *iio;
    int ret;

    iio = devm_iio_device_alloc(&pdev->dev, sizeof(*dht));
    if (IS_ERR(iio)) 
        return -ENOMEM;
    dht = iio_priv(iio);
    dht->dev = &pdev->dev;
    dht->gpio = devm_gpiod_get(dht->dev, "dht", GPIOD_OUT_HIGH_OPEN_DRAIN);
    if (IS_ERR(dht->gpio)) {
        return PTR_ERR(dht->gpio);
    }
    mutex_init(&dht->lock);
    
    iio->name = "hnam02-sensor";
    iio->dev.parent = &pdev->dev;
    iio->modes = INDIO_DIRECT_MODE;
    iio->channels = dht_channels;
    iio->num_channels = ARRAY_SIZE(dht_channels);
    iio->info = &dht_info;
    platform_set_drvdata(pdev, iio);
    ret = iio_device_register(iio);
    if (ret < 0)
        goto reg_err;

    dev_info(dht->dev, "%s - dinhnamuet!\n", __func__);
    return ret;
reg_err:
    mutex_destroy(&dht->lock);
    return ret;
}

static int dht_remove(struct platform_device *pdev)
{
    struct iio_dev *iio = platform_get_drvdata(pdev);
    struct dht_sensor *dht = iio_priv(iio);
    if (dht) {
        iio_device_unregister(iio);
        mutex_destroy(&dht->lock);
		return 0;
    } else {
		return -ENODEV;
	}
}

static const struct of_device_id dht_dev_ids[] = {
    { .compatible = "dinhnam-uet,dht", },
    { }
};
MODULE_DEVICE_TABLE(of, dht_dev_ids);

static struct platform_driver dht_drv = {
    .driver = {
        .owner = THIS_MODULE,
        .name = "dht_sensor",
        .of_match_table = of_match_ptr(dht_dev_ids),
    },
    .probe = dht_probe,
    .remove = dht_remove,
};

module_platform_driver(dht_drv);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("dinhnamuet <dinhnamuet@gmail.com>");
MODULE_DESCRIPTION("IIO Driver for DHT Sensor");
MODULE_VERSION("GPL");