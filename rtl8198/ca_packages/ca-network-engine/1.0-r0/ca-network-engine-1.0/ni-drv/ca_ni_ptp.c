#include <linux/device.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/of_irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/ptp_clock_kernel.h>

#include "ptp.h"
#include "aal_ptp.h"
#include "aal_port.h"
#include "port.h"
#include "special_packet.h"

#include "ca_ni_ptp.h"
#include "aal_common.h"
#include "aal_psds.h"

#define DRIVER		"ca_ptp"

typedef struct ca_ptp {
	struct ptp_clock *ptp_clock;
	struct ptp_clock_info caps;
	int clock_index;

	unsigned int irq_tx_timestamp;	/* IRQ number of Tx timestamp interrupt */
	unsigned int irq_rollover;	/* IRQ number of rollover interrupt */
	unsigned int irq_psds_prbs; 	/* IRQ number of PSDS PRBS interrupt */
} ca_ptp_t;

static ca_ptp_t ca_ptp;

DEFINE_SPINLOCK(register_lock);

int ca_ptp_get_clock_index(void)
{
	return ca_ptp.clock_index;
}

/* enable this then PTP packets are forwarded to CPU ports */
static ca_status_t ca_ptp_special_packet_enable(ca_boolean_t enable)
{
	ca_port_id_t source_port_id, cpu_port_id;
	ca_special_packet_option_t options = {0};
	int i;
	ca_status_t ret;

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	for (i = AAL_LPORT_ETH_NI3; i <= AAL_LPORT_ETH_NI6; i++) {
		/* L2 PTP packet from LAN */
		source_port_id = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, i);
		cpu_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, CA_LAN_TYPE_CPU_PORT_DEFAULT_ID);
		ret = ca_special_packet_set(0, source_port_id, CA_PKT_TYPE_L2_PTP, enable, cpu_port_id, 6, &options);
		if (CA_E_OK != ret) {
			printk(KERN_ERR "Fail to setup special packet for L2 PTP from port %d. (enable %d, ret %d)\n", i, enable, ret);
			return ret;
		}

		/* L4 PTP packet from LAN */
		source_port_id = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, i);
		cpu_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, CA_LAN_TYPE_CPU_PORT_DEFAULT_ID);
		ret = ca_special_packet_set(0, source_port_id, CA_PKT_TYPE_L4_PTP, enable, cpu_port_id, 6, &options);
		if (CA_E_OK != ret) {
			printk(KERN_ERR "Fail to setup special packet for L4 PTP from port %d. (enable %d, ret %d)\n", i, enable, ret);
			return ret;
		}
	}
#elif defined(CONFIG_ARCH_CORTINA_G3LITE)
	/* port#4 is WAN port */
	for (i = AAL_LPORT_ETH_NI0; i <= AAL_LPORT_ETH_NI4; i++) {
		/* L2 PTP packet */
		source_port_id = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, i);
		cpu_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, CA_LAN_TYPE_CPU_PORT_DEFAULT_ID);
		ret = ca_special_packet_set(0, source_port_id, CA_PKT_TYPE_L2_PTP, enable, cpu_port_id, 6, &options);
		if (CA_E_OK != ret) {
			printk(KERN_ERR "Fail to setup special packet for L2 PTP from port %d. (enable %d, ret %d)\n", i, enable, ret);
			return ret;
		}

		/* L4 PTP packet */
		source_port_id = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, i);
		cpu_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, CA_LAN_TYPE_CPU_PORT_DEFAULT_ID);
		ret = ca_special_packet_set(0, source_port_id, CA_PKT_TYPE_L4_PTP, enable, cpu_port_id, 6, &options);
		if (CA_E_OK != ret) {
			printk(KERN_ERR "Fail to setup special packet for L4 PTP from port %d. (enable %d, ret %d)\n", i, enable, ret);
			return ret;
		}
	}
#else
	for (i = AAL_LPORT_ETH_NI0; i <= AAL_LPORT_ETH_NI6; i++) {
		/* L2 PTP packet from LAN */
		source_port_id = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, i);
		cpu_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, CA_LAN_TYPE_CPU_PORT_DEFAULT_ID);
		ret = ca_special_packet_set(0, source_port_id, CA_PKT_TYPE_L2_PTP, enable, cpu_port_id, 6, &options);
		if (CA_E_OK != ret) {
			printk(KERN_ERR "Fail to setup special packet for L2 PTP from port %d. (enable %d, ret %d)\n", i, enable, ret);
			return ret;
		}

		/* L4 PTP packet from LAN */
		source_port_id = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, i);
		cpu_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, CA_LAN_TYPE_CPU_PORT_DEFAULT_ID);
		ret = ca_special_packet_set(0, source_port_id, CA_PKT_TYPE_L4_PTP, enable, cpu_port_id, 6, &options);
		if (CA_E_OK != ret) {
			printk(KERN_ERR "Fail to setup special packet for L4 PTP from port %d. (enable %d, ret %d)\n", i, enable, ret);
			return ret;
		}
	}

	/* L2 PTP packet from WAN */
	source_port_id = CA_PORT_ID(CA_PORT_TYPE_L3, AAL_LPORT_ETH_NI7);
	cpu_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, CA_WAN_TYPE_CPU_PORT_DEFAULT_ID);
	ret = ca_special_packet_set(0, source_port_id, CA_PKT_TYPE_L2_PTP, enable, cpu_port_id, 6, &options);
	if (CA_E_OK != ret) {
		printk(KERN_ERR "Fail to setup special packet for L2 PTP from port 7. (enable %d, ret %d)\n", enable, ret);
		return ret;
	}

	/* L4 PTP packet from WAN */
	source_port_id = CA_PORT_ID(CA_PORT_TYPE_L3, AAL_LPORT_ETH_NI7);
	cpu_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, CA_WAN_TYPE_CPU_PORT_DEFAULT_ID);
	ret = ca_special_packet_set(0, source_port_id, CA_PKT_TYPE_L4_PTP, enable, cpu_port_id, 6, &options);
	if (CA_E_OK != ret) {
		printk(KERN_ERR "Fail to setup special packet for L4 PTP from port 7. (enable %d, ret %d)\n", enable, ret);
		return ret;
	}
#endif

	return CA_E_OK;
}

/*
 * Interrupt service routine
 */

static irqreturn_t ca_ptp_isr_rollover(int irq, void *priv)
{
	aal_ptp_timer_rollover_intr_enable(0);

	if (aal_ptp_timer_rollover_intr_status_get()) {
		aal_ptp_timer_rollover_triggered();
	}
	aal_ptp_timer_rollover_intr_status_clear();

	aal_ptp_timer_rollover_intr_enable(1);

	return IRQ_HANDLED;
}

static irqreturn_t ca_ptp_isr_tx_timestamp(int irq, void *priv)
{
	ca_uint32_t orig_status, status;
	ca_uint32_t port;

	aal_ptp_tx_timestamp_intr_enable(0);

	orig_status = status = aal_ptp_tx_timestamp_intr_status_get();
	while (status != 0) {
		port = ffs(status) - 1;
		aal_ptp_tx_timestamp_record(port);
		status &= ~(BIT(port));
	}
	aal_ptp_tx_timestamp_intr_status_clear(orig_status);

	aal_ptp_tx_timestamp_intr_enable(1);

	return IRQ_HANDLED;
}

static irqreturn_t ca_ptp_isr_psds_prbs(int irq, void *priv)
{
	bool prbs_enable;
	/* Update sec part with the mpcp TOD */
	prbs_enable = aal_psds_prbs_intr_enable_get();
	if (prbs_enable) {
		aal_psds_prbs_intr_enable(0);
		aal_ptp_timer_sync_mpcp_time_triggered();
	}
#if 0
	aal_psds_prbs_intr_enable(0);

	if (aal_psds_prbs_intr_status_get()) {
		sec = atomic_read(&tod_second);
		atomic_set(&ptp_second, (int) sec);
	}
	aal_psds_prbs_intr_status_clear();

#endif
	return IRQ_HANDLED;
}

/*
 * PTP clock operations
 */

static int ca_ptp_adjfreq(struct ptp_clock_info *ptp, s32 ppb)
{
	ca_ptp_local_clock_freq_adj_set(0, ppb);
	return 0;
}

static int ca_ptp_adjtime(struct ptp_clock_info *ptp, s64 delta)
{
	ca_status_t ret;
	ca_ptp_local_clock_t local_clock;
	ca_ptp_set_trigger_t trigger = {0};
	uint64_t tmp_time;

	ret = ca_ptp_local_clock_get(0, &local_clock);
	if (ret != CA_E_OK)
		return -EINVAL;

	tmp_time = local_clock.sec * 1000000000 + local_clock.nsec;
	tmp_time += delta;
	local_clock.sec = (u32) div_u64_rem(tmp_time, 1000000000, &local_clock.nsec);

	ret = ca_ptp_local_clock_set(0, &trigger, &local_clock);
	if (ret != CA_E_OK)
		return -EINVAL;

	return 0;
}

static int ca_ptp_gettime(struct ptp_clock_info *ptp, struct timespec64 *ts)
{
	ca_status_t ret;
	ca_ptp_local_clock_t local_clock;

	ret = ca_ptp_local_clock_get(0, &local_clock);
	if (ret != CA_E_OK)
		return -EINVAL;

	ts->tv_sec = local_clock.sec;
	ts->tv_nsec = local_clock.nsec;

	return 0;
}

static int ca_ptp_settime(struct ptp_clock_info *ptp,
			   const struct timespec64 *ts)
{
	ca_status_t ret;
	ca_ptp_local_clock_t local_clock;
	ca_ptp_set_trigger_t trigger = {0};

	local_clock.sec = ts->tv_sec;
	local_clock.nsec = ts->tv_nsec;

	ret = ca_ptp_local_clock_set(0, &trigger, &local_clock);
	if (ret != CA_E_OK)
		return -EINVAL;

	return 0;
}

static int ca_ptp_enable(struct ptp_clock_info *ptp,
			  struct ptp_clock_request *rq, int on)
{
	/* API is not supported because there is not any external PPS and external clock. */
	return -EOPNOTSUPP;
}

static struct ptp_clock_info ca_ptp_caps = {
	.owner		= THIS_MODULE,
	.name		= "CA PTP timer",
	.max_adj	= 999999999,
	.n_ext_ts	= 0,
	.n_pins		= 0,
	.pps		= 0,
	.adjfreq	= ca_ptp_adjfreq,
	.adjtime	= ca_ptp_adjtime,
	.gettime64	= ca_ptp_gettime,
	.settime64	= ca_ptp_settime,
	.enable		= ca_ptp_enable,
};

/* module operations */

void __exit ca_ni_ptp_exit(struct platform_device *pdev)
{
	/* stop forwarding PTP packets to CPU port */
	ca_ptp_special_packet_enable(CA_FALSE);

	ca_ptp_init(0, CA_FALSE);
	free_irq(ca_ptp.irq_rollover, &(pdev->dev));
	free_irq(ca_ptp.irq_tx_timestamp, &(pdev->dev));
	ptp_clock_unregister(ca_ptp.ptp_clock);
}

int __init ca_ni_ptp_init(struct platform_device *pdev)
{
	int ret;
	struct device_node *parent_np = pdev->dev.of_node;
	struct device_node *np;

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	np = of_get_child_by_name(parent_np, "ni_ptp_ictl");
	ca_ptp.irq_tx_timestamp = irq_of_parse_and_map(np, 0);
	ca_ptp.irq_rollover = irq_of_parse_and_map(np, 0);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	np = of_get_child_by_name(parent_np, "ni_hv_ictl");
	ca_ptp.irq_tx_timestamp = irq_of_parse_and_map(np, 0);
	np = of_get_child_by_name(parent_np, "ni_ptp_ictl");
	ca_ptp.irq_rollover = irq_of_parse_and_map(np, 0);
	np = of_get_child_by_name(parent_np, "ni_psds_prbs");
	ca_ptp.irq_psds_prbs = irq_of_parse_and_map(np, 0);
#endif

	ca_ptp.caps = ca_ptp_caps;
	ca_ptp.ptp_clock = ptp_clock_register(&ca_ptp.caps, NULL);
	if (IS_ERR(ca_ptp.ptp_clock))
		return PTR_ERR(ca_ptp.ptp_clock);

	ca_ptp.clock_index = ptp_clock_index(ca_ptp.ptp_clock);

	ret = request_irq(ca_ptp.irq_rollover, ca_ptp_isr_rollover, IRQF_SHARED | IRQF_TIMER, "CA PTP rollover", &(pdev->dev));
	if (ret) {
		printk(KERN_ERR "%s: Unable to request IRQ %d. (errno %d)\n", __func__, ca_ptp.irq_rollover, ret);
		ptp_clock_unregister(ca_ptp.ptp_clock);
		return ret;
	}

	ret = request_irq(ca_ptp.irq_tx_timestamp, ca_ptp_isr_tx_timestamp, IRQF_SHARED | IRQF_TIMER, "CA PTP Tx timestamp", &(pdev->dev));
	if (ret) {
		printk(KERN_ERR "%s: Unable to request IRQ %d. (errno %d)\n", __func__, ca_ptp.irq_tx_timestamp, ret);
		ptp_clock_unregister(ca_ptp.ptp_clock);
		return ret;
	}
#if 0
//#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	ret = request_irq(ca_ptp.irq_psds_prbs, ca_ptp_isr_psds_prbs, 0, "CA PSDS PRBS", &(pdev->dev));
	if (ret) {
		printk(KERN_ERR "%s: Unable to request IRQ %d. (errno %d)\n", __func__, ca_ptp.irq_tx_timestamp, ret);
		ptp_clock_unregister(ca_ptp.ptp_clock);
		return ret;
	}
#endif
	/* enable rollover interrupt */
	ca_ptp_init(0, CA_TRUE);

	/* forward PTP packets to CPU port */
	ca_ptp_special_packet_enable(CA_TRUE);

	/* NOTE:
	 *   Do not update ca_ptp.sec from system at this moment.
	 *   System timer is not updated yet; it still contains zero.
	 */

	printk(KERN_INFO "%s: Cortina PTP driver is ready.\n", __func__);

	return 0;
}

