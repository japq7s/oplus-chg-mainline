/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * of_get_named_gpio() for the OPLUS charger: mainline removed <linux/of_gpio.h>
 * together with the legacy OF GPIO lookup, while the charger still requests
 * GPIOs by global number (gpio_request(), gpio_get_value(), ...).
 *
 * The number is resolved the way gpiolib-of did it: the controller's of_xlate
 * turns the specifier into an offset (needed for PMIC GPIOs, which are
 * 1-based in DT), and the offset is added to the controller's base.
 */
#ifndef __OPLUS_COMPAT_OF_GPIO_H
#define __OPLUS_COMPAT_OF_GPIO_H

#include <linux/errno.h>
#include <linux/of.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio/driver.h>

enum of_gpio_flags {
	OF_GPIO_ACTIVE_LOW = 0x1,
	OF_GPIO_SINGLE_ENDED = 0x2,
	OF_GPIO_OPEN_DRAIN = 0x4,
	OF_GPIO_TRANSITORY = 0x8,
	OF_GPIO_PULL_UP = 0x10,
	OF_GPIO_PULL_DOWN = 0x20,
};

static inline int of_get_named_gpio_flags(const struct device_node *np,
					  const char *propname, int index,
					  enum of_gpio_flags *flags)
{
	struct of_phandle_args args;
	struct gpio_device *gdev;
	struct gpio_chip *gc;
	u32 xflags = 0;
	int ret, base;

	if (flags)
		*flags = 0;

	ret = of_parse_phandle_with_args_map(np, propname, "gpio", index, &args);
	if (ret)
		return ret;

	gdev = gpio_device_find_by_fwnode(of_fwnode_handle(args.np));
	of_node_put(args.np);
	if (!gdev)
		return -EPROBE_DEFER;

	gc = gpio_device_get_chip(gdev);
	if (gc && gc->of_xlate)
		ret = gc->of_xlate(gc, &args, &xflags);
	else
		ret = args.args_count ? args.args[0] : -EINVAL;
	base = gpio_device_get_base(gdev);
	gpio_device_put(gdev);
	if (ret < 0)
		return ret;

	if (flags && (xflags & OF_GPIO_ACTIVE_LOW))
		*flags = OF_GPIO_ACTIVE_LOW;

	return base + ret;
}

static inline int of_get_named_gpio(const struct device_node *np,
				    const char *propname, int index)
{
	return of_get_named_gpio_flags(np, propname, index, NULL);
}

#endif
