/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Compatibility layer for building the OPLUS charger on mainline Linux.
 * Force-included into every file of this module tree (build-mainline.sh).
 */
#ifndef __OPLUS_MAINLINE_COMPAT_H
#define __OPLUS_MAINLINE_COMPAT_H

/*
 * The Android 6.6 kernel the charger runs on has neither the upstream MSM DRM
 * driver nor fbdev (CONFIG_DRM_MSM and CONFIG_FB are off there), so it uses
 * none of the msm_drm_notify/fb notifier paths. Mainline enables both; hide
 * them so the same code paths are built as on the phone's Android kernel.
 */
#undef CONFIG_DRM_MSM
#undef CONFIG_DRM_MSM_MODULE
#undef CONFIG_FB
#undef CONFIG_FB_MODULE

#include <linux/hrtimer.h>
#include <linux/string.h>
#include <linux/timer.h>

/* Kernel APIs removed since 6.6, with their old semantics. */

/* strncpy() is gone from the kernel; keep its exact (zero padding) behaviour. */
static inline char *oplus_compat_strncpy(char *dest, const char *src, size_t n)
{
	size_t i;

	for (i = 0; i < n && src[i]; i++)
		dest[i] = src[i];
	for (; i < n; i++)
		dest[i] = '\0';
	return dest;
}
#define strncpy oplus_compat_strncpy

#define del_timer(t)		timer_delete(t)
#define del_timer_sync(t)	timer_delete_sync(t)

/*
 * hrtimer_init() was replaced by hrtimer_setup(), which wants the callback;
 * the charger assigns ->function right after init.
 */
static inline enum hrtimer_restart oplus_compat_hrtimer_nop(struct hrtimer *t)
{
	return HRTIMER_NORESTART;
}
#define hrtimer_init(timer, clock, mode) \
	hrtimer_setup(timer, oplus_compat_hrtimer_nop, clock, mode)

/*
 * sched_setscheduler() is no longer exported; sched_setattr_nocheck() keeps
 * the exact policy and RT priority the charger asks for (sched_set_fifo()
 * would force MAX_RT_PRIO / 2).
 */
#include <linux/sched.h>
#include <uapi/linux/sched/types.h>
static inline int oplus_compat_sched_setscheduler(struct task_struct *p, int policy,
						  const struct sched_param *param)
{
	struct sched_attr attr = {
		.size = sizeof(attr),
		.sched_policy = policy,
		.sched_priority = param->sched_priority,
	};

	return sched_setattr_nocheck(p, &attr);
}
#define sched_setscheduler oplus_compat_sched_setscheduler

#endif
