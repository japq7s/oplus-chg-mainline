/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Downstream (QTI) PMIC GLINK client API, as used by the OPLUS charger.
 * On mainline it is provided by compat/oplus_pmic_glink.c on top of the
 * upstream pmic_glink driver (<linux/soc/qcom/pmic_glink.h>).
 */
#ifndef __QTI_PMIC_GLINK_H
#define __QTI_PMIC_GLINK_H

#include <linux/types.h>

struct pmic_glink_client;
struct device;

enum pmic_glink_state {
	PMIC_GLINK_STATE_DOWN,
	PMIC_GLINK_STATE_UP,
};

struct pmic_glink_client_data {
	const char	*name;
	u32		id;
	void		*priv;
	int		(*msg_cb)(void *priv, void *data, size_t len);
	void		(*state_cb)(void *priv, enum pmic_glink_state state);
};

struct pmic_glink_hdr {
	u32 owner;
	u32 type;
	u32 opcode;
};

struct pmic_glink_client *pmic_glink_register_client(struct device *dev,
			const struct pmic_glink_client_data *client_data);
int pmic_glink_unregister_client(struct pmic_glink_client *client);
int pmic_glink_write(struct pmic_glink_client *client, void *data,
			size_t len);

#endif
