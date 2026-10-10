/*
 * ZLR962282L_25_76_Realtek_Profile.h --
 *
 * This header file exports the Profile data types
 *
 * Project Info --
 *   File:   P:\SysApps\HardwareApps\LineModules_VOB\miSLIC line modules\ZLR962282H_Le9622_8FXS_MLIB\software\ZLR962282L_25_76_Realtek.vpw
 *   Type:   Microsemi miSLIC Le9622 Multiline IB ABS - See Comments>>>>
 *   Date:   Thursday, September 14, 2017 19:57:47
 *   Device: ZL880 ZL88602
 *
 *   This file was generated with Profile Wizard Version: P2.7.0
 *
 * Project Comments --
 *  MICROSEMI ZL88602 Multiline Inverting Boost 8 FXS 
 *  Dec 9, 2016 JLR
 *  
 *  This profile defines the switcher parameters for the 8FXS design.
 *  In this design Device one switcher Y is the master for the VBL supply
 *  but switcher Z is a slave.  Device 2 swicher Z is the master for the
 *  VBH supply and switcher Y is a slave.  The control of the VBL and VBH
 *  switchers are spread across two devices.  This is to minimize loading
 *  on each devices VDDSW supply which is powered from an internal
 *  charge pump. 
 *  
 *  Device profiles also include softstart timing parameters.
 *  This loads limited switcher timing for a specified period (50ms in this case)
 *  to limit inrush current during start up. This can be modified if startup
 *  current is too high.
 *  
 *  VBAT supplies are configured as follows:
 *  
 *  VBL device 1 SWY(master)- 25V, FSW= 300kHz, VDDSW=4.7V (device mode reg)
 *               SWZ(slave) - 76V
 *    I/O21_VS1 Analog- Open
 *    I/O22_VS2 Analog - VSW sense
 *    Softstart Enabled - 50ms
 *    Overcurrent shutdown sense on VS1
 *  
 *  VBH device 2 SWY (slave) - 25V
 *               SWZ (master) - 76V, Fsw 192kHz, VDDSW=5V (device mode reg)
 *    I/O21_VS1 Analog - Open
 *    I/O22_VS2 Digital - uncommitted 
 *    Softstart Enabled - 50ms 
 *    Overcurrent shutdown sense on VS1
 *  
 *  All other devices: SWY (slave) - 25V
 *                     SWZ (slave) - 76V
 *    All I/Os digital and uncommitted
 *  
 *  
 *  Device 1 I/O22_VS2 can be used to measure the VSW input supply.  This may
 *  be important to support wide input supply voltages where alternate switcher
 *  profiles may need to be loaded.
 */

#ifndef ZLR962282L_25_76_REALTEK_PROFILE_H
#define ZLR962282L_25_76_REALTEK_PROFILE_H

#ifdef VP_API_TYPES_H
#include "vp_api_types.h"
#else
typedef unsigned char VpProfileDataType;
#endif


/************** Device Profile **************/
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_MICROSEMI_LE9622_BBABS
extern const VpProfileDataType DEV_PROFILE_47UH_PL_400MA[];
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_MICROSEMI_LE9622_IBABS)
extern const VpProfileDataType DEV_PROFILE[];        /* Device Configuration Data */
#endif
extern const VpProfileDataType DEV_PROFILE_47UH[];
extern const VpProfileDataType DEV_Master_PROFILE_25V_VBL[];/* Device Configuration Data - Dev 1 */
extern const VpProfileDataType DEV_Master_PROFILE_76V_VBH[];/* Device Configuration Data  - Dev 2 */
extern const VpProfileDataType DEV_Slave_PROFILE_25V_76V[];/* Device Configuration Data - Dev 3 and up */

/************** DC Profile **************/
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_MICROSEMI_LE9622_BBABS
extern const VpProfileDataType DC_FXS_ZL880_ABS100V_DEF[];
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_MICROSEMI_LE9622_IBABS)
extern const VpProfileDataType DC_PROFILE[];         /* DC Parameters */
#endif
extern const VpProfileDataType DC_PROFILE_25ma[];    /* DC Parameters */
/************** AC Profile **************/
extern const VpProfileDataType AC_FXS_RF14_600R_DEF[];         /* 600 ohms profile */
extern const VpProfileDataType AC_FXS_RF14_ETSI[];   /* AC FXS RF14 ETSI ES201 970 270R+750R//150nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_GR57[];   /* AC FXS RF14 Telcordia GR-57 900R+2.16uF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_AT[];     /* AC FXS RF14 Austria 220R+820R//115nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_AU[];     /* AC FXS RF14 Australia 220R+820R//120nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_BE[];     /* AC FXS RF14 Belgium 270R+750R//150nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_BG[];     /* AC FXS RF14 Bulgaria 220R+820R//115nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_BR[];     /* AC FXS RF14 Brazil 900R Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_CH[];     /* AC FXS RF14 Switzerland 270R+750R//150nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_CN[];     /* AC FXS RF14 China 200R+680R//100nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_DE[];     /* AC FXS RF14 Germany 220R+820R//115nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_DK[];     /* AC FXS RF14 Denmark 270R+750R//150nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_ES[];     /* AC FXS RF14 Spain 270R+750R//150nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_FI[];     /* AC FXS RF14 Finland 270R+910R//120nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_FR[];     /* AC FXS RF14 France 270R+750R//150nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_GB[];     /* AC FXS RF14 UK 370R+620R//310nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_GR[];     /* AC FXS RF14 Greece 270R+750R//150nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_HU[];     /* AC FXS RF14 Hungary 270R+750R//150nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_IE[];     /* AC FXS RF14 Ireland 270R+750R//150nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_IL[];     /* AC FXS RF14 Israel 270R+750R//150nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_IS[];     /* AC FXS RF14 Iceland 270R+750R//150nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_IT[];     /* AC FXS RF14 Italy 270R+750R//150nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_JP[];     /* AC FXS RF14 Japan 600R+1uF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_NL[];     /* AC FXS RF14 Netherlands 270R+750R//150nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_NO[];     /* AC FXS RF14 Norway 270R+750R//150nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_NZ[];     /* AC FXS RF14 New Zealand 370R+620R//310nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_PT[];     /* AC FXS RF14 Portugal 270R+750R//150nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_RU[];     /* AC FXS RF14 Russia 150R+510R//47nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_SE[];     /* AC FXS RF14 Sweden 270R+750R//150nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_TK[];     /* AC FXS RF14 Turkey 270R+750R//150nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_ZA[];     /* AC FXS RF14 South Africa 220R+820R//115nF Normal Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_600R_DEF[];/* AC FXS RF14 600R Wideband Coefficients (Default)  */
extern const VpProfileDataType AC_FXS_RF14_WB_ETSI[];/* AC FXS RF14 ETSI ES201 970 270R+750R//150nF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_GR57[];/* AC FXS RF14 Telcordia GR-57 900R+2.16uF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_AT[];  /* AC FXS RF14 Austria 220R+820R//115nF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_AU[];  /* AC FXS RF14 Australia 220R+820R//120nF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_BE[];  /* AC FXS RF14 Belgium 270R+750R//150nF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_BG[];  /* AC FXS RF14 Bulgaria 220R+820R//115nF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_BR[];  /* AC FXS RF14 Brazil 900R Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_CH[];  /* AC FXS RF14 Switzerland 270R+750R//150nF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_CN[];  /* AC FXS RF14 China 200R+680R//100nF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_DE[];  /* AC FXS RF14 Germany 220R+820R//115nF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_DK[];  /* AC FXS RF14 Denmark 270R+750R//150nF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_ES[];  /* AC FXS RF14 Spain 270R+750R//150nF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_FI[];  /* AC FXS RF14 Finland 270R+910R//120nF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_FR[];  /* AC FXS RF14 France 270R+750R//150nF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_GB[];  /* AC FXS RF14 UK 370R+620R//310nF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_GR[];  /* AC FXS RF14 Greece 270R+750R//150nF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_HU[];  /* AC FXS RF14 Hungary 270R+750R//150nF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_IE[];  /* AC FXS RF14 Ireland 270R+750R//150nF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_IL[];  /* AC FXS RF14 Israel 270R+750R//150nF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_IS[];  /* AC FXS RF14 Iceland 270R+750R//150nF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_IT[];  /* AC FXS RF14 Italy 270R+750R//150nF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_JP[];  /* AC FXS RF14 Japan 600R+1uF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_NL[];  /* AC FXS RF14 Netherlands 270R+750R//150nF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_NO[];  /* AC FXS RF14 Norway 270R+750R//150nF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_NZ[];  /* AC FXS RF14 New Zealand 370R+620R//310nF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_PT[];  /* AC FXS RF14 Portugal 270R+750R//150nF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_RU[];  /* AC FXS RF14 Russia 150R+510R//47nF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_SE[];  /* AC FXS RF14 Sweden 270R+750R//150nF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_TK[];  /* AC FXS RF14 Turkey 270R+750R//150nF Wideband Coefficients */
extern const VpProfileDataType AC_FXS_RF14_WB_ZA[];  /* AC FXS RF14 South Africa 220R+820R//115nF Wideband Coefficients */

/************** Ringing Profile **************/
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_MICROSEMI_LE9622_BBABS
extern const VpProfileDataType RING_VE960_ABS100V_DEF[];/* Default Ringing 25Hz 50Vrms Fixed, AC Trip - Use for all countries unless country profile exists */
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_MICROSEMI_LE9622_IBABS)
extern const VpProfileDataType RING_25Hz_70Vpk[];    /* Default Ringing 25Hz 50Vrms Fixed, AC Trip - Use for all countries unless country profile exists */
extern const VpProfileDataType RING_25HZ_85VPK[];    /* 60vrms 25Hz Ringing, AC trip */
#endif
/************** Tone Profile **************/

/************** Cadence Profile **************/
extern const VpProfileDataType LE880_RING_CAD_STD[];
extern const VpProfileDataType LE880_RING_CAD_SHORT[];
extern const VpProfileDataType LE886_RING_CAD_STD[];
extern const VpProfileDataType LE886_RING_CAD_SHORT[];

/************** Caller ID Profile **************/

/************** Metering_Profile **************/
extern const VpProfileDataType METER_12KHZ_RF14[];   /* Metering, 12kHz 0.5Vrms */
extern const VpProfileDataType METER_16KHZ_RF14[];   /* Metering, 16kHz 0.5Vrms */

#endif /* ZLR962282L_25_76_REALTEK_PROFILE_H */

