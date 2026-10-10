/*
 * Driver for Texas Instruments INA60 power monitor chips
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/jiffies.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/util_macros.h>
#include <linux/regmap.h>

/* register definitions */
#define INA260_CONFIG			0x00
#define INA260_CURRENT			0x01 /* readonly */
#define INA260_BUS_VOLTAGE		0x02 /* readonly */
#define INA260_POWER			0x03 /* readonly */
#define INA260_MASK_ENABLE		0x06
#define INA260_ALERT_LIMIT		0x07

/* register count */
#define INA260_REGISTERS		8

/* settings - depend on use case */
#define INA260_CONFIG_DEFAULT		0x6127

/* bit mask for reading the averaging setting in the configuration register */
#define INA260_AVG_RD_MASK		0x0E00

#define INA260_READ_AVG(reg)		(((reg) & INA260_AVG_RD_MASK) >> 9)
#define INA260_SHIFT_AVG(val)		((val) << 9)

/* common attrs, ina226 attrs and NULL */
#define INA260_MAX_ATTRIBUTE_GROUPS	3

/*
 * Both bus voltage and shunt voltage conversion times for ina260 are set
 * to 0b0100 on POR, which translates to 2200 microseconds in total.
 */
#define INA260_TOTAL_CONV_TIME_DEFAULT	2200

static struct regmap_config ina260_regmap_config = {
	.reg_bits = 8,
	.val_bits = 16,
};

enum ina260_ids { ina260 };

struct ina260_config {
	u16 config_default;
	int registers;
	int current_lsb;	/* mA */
	int bus_voltage_lsb;	/* mV */
	int power_lsb;	/* mW */
};

struct ina260_data {
	const struct ina260_config *config;

	struct regmap *regmap;

	const struct attribute_group *groups[INA260_MAX_ATTRIBUTE_GROUPS];
};

static const struct ina260_config ina260_config[] = {
	[ina260] = {
		.config_default = INA260_CONFIG_DEFAULT,
		.registers = INA260_REGISTERS,
		.current_lsb = 1250,
		.bus_voltage_lsb = 1250,
		.power_lsb = 10,
	},
};

/*
 * Available averaging rates for ina260. The indices correspond with
 * the bit values expected by the chip (according to the ina260 datasheet,
 * table 3 AVG bit settings, found at
 * http://www.ti.com/lit/ds/symlink/ina260.pdf.
 */
static const int ina260_avg_tab[] = { 1, 4, 16, 64, 128, 256, 512, 1024 };

static int ina260_reg_to_interval(u16 config)
{
	int avg = ina260_avg_tab[INA260_READ_AVG(config)];

	/*
	 * Multiply the total conversion time by the number of averages.
	 * Return the result in milliseconds.
	 */
	return DIV_ROUND_CLOSEST(avg * INA260_TOTAL_CONV_TIME_DEFAULT, 1000);
}

/*
 * Return the new, shifted AVG field value of CONFIG register,
 * to use with regmap_update_bits
 */
static u16 ina260_interval_to_reg(int interval)
{
	int avg, avg_bits;

	avg = DIV_ROUND_CLOSEST(interval * 1000,
				INA260_TOTAL_CONV_TIME_DEFAULT);
	avg_bits = find_closest(avg, ina260_avg_tab,
				ARRAY_SIZE(ina260_avg_tab));

	return INA260_SHIFT_AVG(avg_bits);
}

/*
 * Initialize the configuration and calibration registers.
 */
static int ina260_init(struct ina260_data *data)
{
	int ret = regmap_write(data->regmap, INA260_CONFIG,
			       data->config->config_default);
	int time = ina260_reg_to_interval(INA260_CONFIG);

	if (ret < 0)
		return ret;

	usleep_range(time, time + 100);
	return 0;
}

static int ina260_read_reg(struct device *dev, int reg, unsigned int *regval)
{
	struct ina260_data *data = dev_get_drvdata(dev);
	int ret;

	dev_dbg(dev, "Starting register %d read\n", reg);

	ret = regmap_read(data->regmap, reg, regval);
	if (ret < 0)
		return ret;

	dev_dbg(dev, "read %d, val = 0x%04x\n", reg, *regval);
	return 0;
}

static int ina260_get_value(struct ina260_data *data, u8 reg,
			    unsigned int regval)
{
	int val;

	switch (reg) {
	case INA260_BUS_VOLTAGE:
		val = regval * data->config->bus_voltage_lsb / 1000;
		break;
	case INA260_POWER:
		val = regval * data->config->power_lsb;
		break;
	case INA260_CURRENT:
		val = (s16)regval * data->config->current_lsb / 1000;
		break;
	default:
		/* programmer goofed */
		WARN_ON_ONCE(1);
		val = 0;
		break;
	}

	return val;
}

static ssize_t ina260_show_value(struct device *dev,
				 struct device_attribute *da, char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct ina260_data *data = dev_get_drvdata(dev);
	unsigned int regval;

	int err = ina260_read_reg(dev, attr->index, &regval);

	if (err < 0)
		return err;

	return snprintf(buf, PAGE_SIZE, "%d\n",
			ina260_get_value(data, attr->index, regval));
}

static ssize_t ina260_set_interval(struct device *dev,
				   struct device_attribute *da,
				   const char *buf, size_t count)
{
	struct ina260_data *data = dev_get_drvdata(dev);
	unsigned long val;
	int status;

	status = kstrtoul(buf, 10, &val);
	if (status < 0)
		return status;

	if (val > INT_MAX || val == 0)
		return -EINVAL;

	status = regmap_update_bits(data->regmap, INA260_CONFIG,
				    INA260_AVG_RD_MASK,
				    ina260_interval_to_reg(val));
	if (status < 0)
		return status;

	return count;
}

static ssize_t ina260_show_interval(struct device *dev,
				    struct device_attribute *da, char *buf)
{
	struct ina260_data *data = dev_get_drvdata(dev);
	int status;
	unsigned int regval;

	status = regmap_read(data->regmap, INA260_CONFIG, &regval);
	if (status)
		return status;

	return snprintf(buf, PAGE_SIZE, "%d\n", ina260_reg_to_interval(regval));
}

/* bus voltage */
static SENSOR_DEVICE_ATTR(volt_input, S_IRUGO, ina260_show_value, NULL,
			  INA260_BUS_VOLTAGE);

/* calculated current */
static SENSOR_DEVICE_ATTR(curr_input, S_IRUGO, ina260_show_value, NULL,
			  INA260_CURRENT);

/* calculated power */
static SENSOR_DEVICE_ATTR(power_input, S_IRUGO, ina260_show_value, NULL,
			  INA260_POWER);

static SENSOR_DEVICE_ATTR(update_interval, S_IRUGO | S_IWUSR,
			  ina260_show_interval, ina260_set_interval, 0);

/* pointers to created device attributes */
static struct attribute *ina260_attrs[] = {
	&sensor_dev_attr_volt_input.dev_attr.attr,
	&sensor_dev_attr_curr_input.dev_attr.attr,
	&sensor_dev_attr_power_input.dev_attr.attr,
	&sensor_dev_attr_update_interval.dev_attr.attr,
	NULL,
};

static const struct attribute_group ina260_group = {
	.attrs = ina260_attrs,
};

static int ina260_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct ina260_data *data;
	struct device *hwmon_dev;
	int ret, group = 0;
	enum ina260_ids chip;

	if (client->dev.of_node)
		chip = (enum ina260_ids)of_device_get_match_data(&client->dev);
	else
		chip = id->driver_data;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	/* set the device type */
	data->config = &ina260_config[chip];

	ina260_regmap_config.max_register = data->config->registers;

	data->regmap = devm_regmap_init_i2c(client, &ina260_regmap_config);
	if (IS_ERR(data->regmap)) {
		dev_err(dev, "failed to allocate register map\n");
		return PTR_ERR(data->regmap);
	}

	ret = ina260_init(data);
	if (ret < 0) {
		dev_err(dev, "error configuring the device: %d\n", ret);
		return -ENODEV;
	}

	data->groups[group++] = &ina260_group;

	hwmon_dev = devm_hwmon_device_register_with_groups(dev, client->name,
							   data, data->groups);
	if (IS_ERR(hwmon_dev))
		return PTR_ERR(hwmon_dev);

	return 0;
}

static const struct i2c_device_id ina260_id[] = {
	{ "ina260", ina260 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ina260_id);

static const struct of_device_id __maybe_unused ina260_of_match[] = {
	{
		.compatible = "ti,ina260",
		.data = (void *)ina260
	},
	{ },
};
MODULE_DEVICE_TABLE(of, ina260_of_match);

static struct i2c_driver ina260_driver = {
	.driver = {
		.name	= "ina260",
		.of_match_table = of_match_ptr(ina260_of_match),
	},
	.probe		= ina260_probe,
	.id_table	= ina260_id,
};

module_i2c_driver(ina260_driver);

MODULE_DESCRIPTION("ina260 driver");
MODULE_LICENSE("GPL");
