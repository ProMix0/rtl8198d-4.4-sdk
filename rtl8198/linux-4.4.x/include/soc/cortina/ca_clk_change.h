/*
 * Copyright (c) Cortina-Systems Limited 2014.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __CA_CLK_CHANGE_H__
#define __CA_CLK_CHANGE_H__

#include "ca_types.h"

/* Peripheral frequencies in use */
typedef enum {
	CA_PERIPHERAL_FREQUENCY_100,
	CA_PERIPHERAL_FREQUENCY_150,
	CA_PERIPHERAL_FREQUENCY_170,
} ca_peripheral_freq_t;

/* AXI bus frequencies in use */
typedef enum {
	CA_AXI_FREQUENCY_133,
	CA_AXI_FREQUENCY_140,
	CA_AXI_FREQUENCY_142,
	CA_AXI_FREQUENCY_150,
	CA_AXI_FREQUENCY_160,
} ca_axi_freq_t;

/* Notification events sent by the framework  */
typedef enum {
	CA_PM_FREQ_PRECHANGE,
	CA_PM_FREQ_POSTCHANGE,
	CA_PM_FREQ_RESUMECHANGE,
} ca_pm_freq_notifier_event_t;

/* Callback notification data structure */
typedef struct {
	ca_pm_freq_notifier_event_t event;
	void *data;
	ca_peripheral_freq_t old_peripheral_clk;
	ca_peripheral_freq_t new_peripheral_clk;
	ca_axi_freq_t old_axi_clk;
	ca_axi_freq_t new_axi_clk;
} ca_pm_freq_notifier_data_t;

/* Notification callback function */
typedef ca_status_t (ca_pm_freq_notifier_callback_t)(ca_pm_freq_notifier_data_t *);

/* Callback registration data structure */
typedef struct {
	ca_pm_freq_notifier_callback_t *notifier;
	void *data;
} ca_pm_freq_notifier_t;

/*
 * Register Clock Change Notification Callback
 * n		Structure containing a callback function and callback data
 * priority	Callback priority
 */
ca_status_t ca_pm_freq_register_notifier(CA_IN ca_pm_freq_notifier_t *n,
					 CA_IN ca_uint32_t priority);

/*
 * Unregister Clock Change Notification Callback
 * n		Structure containing a callback function and callback data
 */
ca_status_t ca_pm_freq_unregister_notifier(CA_IN ca_pm_freq_notifier_t *n);

#endif /* __CA_CLK_CHANGE_H__ */
