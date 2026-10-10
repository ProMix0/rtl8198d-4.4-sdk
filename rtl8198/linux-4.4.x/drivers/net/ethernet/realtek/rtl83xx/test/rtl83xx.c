#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>

#include "rtl83xx_ioctl.h"
#include "rtl8367m_vb.h"
main(int argc, char *argv[])
{
	int fd;
	int ret;
	int reg;
	int port;
	int i;
	RTK_CMD_T	rtk_cmd;
	int ioctl_num;
	rtk_data_t SignalDetect, Sync;
	rtk_port_linkStatus_t Link;

	if (argc < 2) {
		printf("rtl83xx_ioctl %d - RTK_PORT_PHY_STATUS_GET\n", RTK_STAT_PORT_SGMII_LINK_STATUS_GET);
		printf("rtl83xx_ioctl %d - RTK_PORT_PHYREG_GET\n", RTK_PORT_PHYREG_GET);
		printf("rtl83xx_ioctl %d - Reset EXT_PORT0\n", RTK_STAT_PORT_SDS_RESET);
		printf("rtl83xx_ioctl %d - Get SGMII Link status EXT_PORT0\n", RTK_STAT_PORT_SGMII_LINK_STATUS_GET);
	}

	printf("argv[1]=%s\n", argv[1]);
	ioctl_num = atoi(argv[1]);
	printf("ioctl_num=%d\n", ioctl_num);

	fd = open("/dev/switch", O_RDWR);
	printf("open /dev/switch fd=%d\n", __func__, fd);

	switch (ioctl_num) {
		case RTK_PORT_PHY_STATUS_GET:
			memset(&rtk_cmd, 0, sizeof(RTK_CMD_T));
			rtk_cmd.para.port_phy_status.port = 1;
			rtk_cmd.cmd = RTK_PORT_PHY_STATUS_GET;
			ret = ioctl(fd, SIOCDEVPRIVATE, &rtk_cmd);
			printf("ioctl RTK_PORT_PHY_STATUS_GET return=%d\n", ret);
			printf("port=%d, linkStatus=%d, speed=%d, duplex=%d\n", 
					rtk_cmd.para.port_phy_status.port, rtk_cmd.para.port_phy_status.linkStatus, rtk_cmd.para.port_phy_status.speed, rtk_cmd.para.port_phy_status.duplex);
			break;

		case RTK_PORT_PHYREG_GET:
			for (port = 0; port < 4; port++) {

				for (reg = 0; reg <= 5; reg++) {
					memset(&rtk_cmd, 0, sizeof(RTK_CMD_T));
					rtk_cmd.para.port_phy_reg.port = port;
					rtk_cmd.para.port_phy_reg.reg = reg;
					rtk_cmd.cmd = RTK_PORT_PHYREG_GET;
					ret = ioctl(fd, SIOCDEVPRIVATE, &rtk_cmd);
					printf("ioctl RTK_PORT_PHYREG_GET reg=%d return=%d\n", reg, ret);
					printf("port=%d, reg=%d, data=0x%x\n", rtk_cmd.para.port_phy_reg.port, rtk_cmd.para.port_phy_reg.reg, rtk_cmd.para.port_phy_reg.data);

				}
			}
			break;
		case RTK_STAT_PORT_SDS_RESET:
			memset(&rtk_cmd, 0, sizeof(RTK_CMD_T));
			rtk_cmd.para.sds_reset.port = EXT_PORT0;
			rtk_cmd.cmd = RTK_STAT_PORT_SDS_RESET;
			ret = ioctl(fd, SIOCDEVPRIVATE, &rtk_cmd);
			printf("ioctl RTK_STAT_PORT_SDS_RESET, port=%d, ret=%d\n", EXT_PORT0, ret);
			break;
		case RTK_STAT_PORT_SGMII_LINK_STATUS_GET:
			memset(&rtk_cmd, 0, sizeof(RTK_CMD_T));
                        rtk_cmd.para.sgmii_link_status.port = EXT_PORT0;
                        rtk_cmd.cmd = RTK_STAT_PORT_SGMII_LINK_STATUS_GET;
                        ret = ioctl(fd, SIOCDEVPRIVATE, &rtk_cmd);
			printf("ioctl RTK_STAT_PORT_SGMII_LINK_STATUS_GET, port=%d, ret=%d\n", EXT_PORT0, ret);
			printf("SignalDetect=%d, Sync=%d, Link=%d\n", rtk_cmd.para.sgmii_link_status.SignalDetect,
				rtk_cmd.para.sgmii_link_status.Sync, rtk_cmd.para.sgmii_link_status.Link);	
			break;
		default:
			printf("Not support ioctl number!!\n");
			break;
	}
	
	close(fd);
}
