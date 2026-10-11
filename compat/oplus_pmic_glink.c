// SPDX-License-Identifier: GPL-2.0-only
/*
 * Downstream PMIC GLINK client API (pmic_glink_register_client() & co.) on
 * top of the mainline pmic_glink driver, for the OPLUS charger.
 *
 * Mainline creates one auxiliary device per pmic_glink service; the battery
 * manager one ("pmic_glink.power-supply") is normally bound by qcom_battmgr.
 * On OPLUS phones the ADSP battery firmware keeps charging disabled until the
 * OPLUS charger driver enables it, so that driver has to own the battery
 * manager channel instead: this module binds the auxiliary device (qcom_battmgr
 * must not be loaded) and hands out clients allocated on it, since
 * devm_pmic_glink_client_alloc() takes the pmic_glink instance from the
 * parent of the device it is given.
 */
#include <linux/auxiliary_bus.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/soc/qcom/pdr.h>
#include <linux/soc/qcom/pmic_glink.h>

/* The downstream API types (see include/linux/soc/qcom/qti_pmic_glink.h) */
enum oplus_glink_state {
	OPLUS_GLINK_STATE_DOWN,
	OPLUS_GLINK_STATE_UP,
};

struct oplus_glink_client_data {
	const char	*name;
	u32		id;
	void		*priv;
	int		(*msg_cb)(void *priv, void *data, size_t len);
	void		(*state_cb)(void *priv, enum oplus_glink_state state);
};

struct oplus_glink_client {
	struct list_head node;
	struct pmic_glink_client *client;
	struct oplus_glink_client_data data;
	bool active;
};

static DEFINE_MUTEX(oplus_glink_lock);
static struct auxiliary_device *oplus_glink_adev;
static LIST_HEAD(oplus_glink_clients);

static void oplus_glink_cb(const void *data, size_t len, void *priv)
{
	struct oplus_glink_client *c = priv;

	if (READ_ONCE(c->active) && c->data.msg_cb)
		c->data.msg_cb(c->data.priv, (void *)data, len);
}

static void oplus_glink_pdr(void *priv, int state)
{
	struct oplus_glink_client *c = priv;

	if (!READ_ONCE(c->active) || !c->data.state_cb)
		return;

	c->data.state_cb(c->data.priv, state == SERVREG_SERVICE_STATE_UP ?
			 OPLUS_GLINK_STATE_UP : OPLUS_GLINK_STATE_DOWN);
}

struct pmic_glink_client *
pmic_glink_register_client(struct device *dev,
			   const struct oplus_glink_client_data *data)
{
	struct oplus_glink_client *c;

	if (!data || !data->msg_cb)
		return ERR_PTR(-EINVAL);

	guard(mutex)(&oplus_glink_lock);

	if (!oplus_glink_adev)
		return ERR_PTR(-EPROBE_DEFER);

	/*
	 * Mainline clients cannot be removed before their device goes away.
	 * Reuse an inactive one of the same owner (e.g. after a deferred probe
	 * of the charger) instead of adding another listener.
	 */
	list_for_each_entry(c, &oplus_glink_clients, node) {
		if (!c->active && c->data.id == data->id) {
			c->data = *data;
			WRITE_ONCE(c->active, true);
			if (c->data.state_cb)
				c->data.state_cb(c->data.priv, OPLUS_GLINK_STATE_UP);
			return (struct pmic_glink_client *)c;
		}
	}

	c = devm_kzalloc(&oplus_glink_adev->dev, sizeof(*c), GFP_KERNEL);
	if (!c)
		return ERR_PTR(-ENOMEM);

	c->data = *data;
	c->active = true;
	c->client = devm_pmic_glink_client_alloc(&oplus_glink_adev->dev, data->id,
						 oplus_glink_cb, oplus_glink_pdr, c);
	if (IS_ERR(c->client))
		return ERR_CAST(c->client);

	list_add_tail(&c->node, &oplus_glink_clients);
	/* reports the current service state through oplus_glink_pdr() */
	pmic_glink_client_register(c->client);

	dev_dbg(dev, "pmic_glink client %s (owner %u) registered\n",
		data->name, data->id);

	return (struct pmic_glink_client *)c;
}
EXPORT_SYMBOL_GPL(pmic_glink_register_client);

int pmic_glink_unregister_client(struct pmic_glink_client *client)
{
	struct oplus_glink_client *c = (struct oplus_glink_client *)client;

	if (IS_ERR_OR_NULL(client))
		return -EINVAL;

	WRITE_ONCE(c->active, false);
	return 0;
}
EXPORT_SYMBOL_GPL(pmic_glink_unregister_client);

int pmic_glink_write(struct pmic_glink_client *client, void *data, size_t len)
{
	struct oplus_glink_client *c = (struct oplus_glink_client *)client;

	if (IS_ERR_OR_NULL(client) || !READ_ONCE(c->active))
		return -ENODEV;

	return pmic_glink_send(c->client, data, len);
}
EXPORT_SYMBOL_GPL(pmic_glink_write);

static int oplus_glink_probe(struct auxiliary_device *adev,
			     const struct auxiliary_device_id *id)
{
	guard(mutex)(&oplus_glink_lock);
	oplus_glink_adev = adev;
	return 0;
}

static void oplus_glink_remove(struct auxiliary_device *adev)
{
	struct oplus_glink_client *c;

	guard(mutex)(&oplus_glink_lock);
	/* the clients are devres of adev and go away with it */
	list_for_each_entry(c, &oplus_glink_clients, node)
		WRITE_ONCE(c->active, false);
	INIT_LIST_HEAD(&oplus_glink_clients);
	oplus_glink_adev = NULL;
}

static const struct auxiliary_device_id oplus_glink_id_table[] = {
	{ .name = "pmic_glink.power-supply", },
	{},
};
MODULE_DEVICE_TABLE(auxiliary, oplus_glink_id_table);

static struct auxiliary_driver oplus_glink_driver = {
	.name = "oplus_pmic_glink",
	.probe = oplus_glink_probe,
	.remove = oplus_glink_remove,
	.id_table = oplus_glink_id_table,
};
module_auxiliary_driver(oplus_glink_driver);

MODULE_DESCRIPTION("Downstream PMIC GLINK client API for the OPLUS charger");
MODULE_LICENSE("GPL");
