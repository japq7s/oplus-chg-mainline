// SPDX-License-Identifier: GPL-2.0-only
/*
 * The OPLUS fork of dwc3-msm exports oplus_dwc3_config_usbphy_pfunc(): the
 * charger registers a callback (oplus_is_pd_svooc) through it and dwc3-msm
 * uses it to retune the USB PHY while SVOOC runs over PD. The mainline
 * dwc3-qcom driver has no such hook, so the callback is only stored.
 *
 * TODO: wire it to the USB PHY if SVOOC over PD needs the retuning (stage 3).
 */
#include <linux/export.h>
#include <linux/types.h>

static bool (*oplus_dwc3_pd_svooc)(void);

void oplus_dwc3_config_usbphy_pfunc(bool (*pfunc)(void))
{
	oplus_dwc3_pd_svooc = pfunc;
}
EXPORT_SYMBOL(oplus_dwc3_config_usbphy_pfunc);
