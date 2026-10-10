#ifndef __CA_NI_PTP_H__
#define __CA_NI_PTP_H__

#include <linux/platform_device.h>

#define DEFAULT_ADDEND	0x80000000 /* 125MHz clock which MSB is aligned to 8ns */

int ca_ptp_get_clock_index(void);

int ca_ni_ptp_init(struct platform_device *pdev);
void ca_ni_ptp_exit(struct platform_device *pdev);

#endif /* __CA_NI_PTP_H__ */
