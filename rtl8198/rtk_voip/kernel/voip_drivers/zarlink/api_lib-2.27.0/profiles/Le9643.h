/*
 * Le9643_SM2_LVIBSS_HVIB_Realtek.h --
 *
 * This header file exports the Profile data types
 *
 * Project Info --
 *   File:   D:\data\Desktop\ZLR965324H_SM2_LVIBSS_HVIB_Realtek.vpw
 *   Type:   miSLIC- Le9643/53 - Inverting Boost Tracker, 90V CH1, 135V Ch2
 *   Date:   Thursday, August 24, 2017 10:25:07
 *   Device: miSLIC Le9651
 *
 *   This file was generated with Profile Wizard Version: P2.7.0
 *
 * Project Comments --
 *  Profile:
 *  1) 135V HV Inverting Boost, 12Vin, L=4.7uHy
 *  2) 90V LV Inverting Boost, 12Vin, L=10uhy with soft start
 *  3) 90V LV Inverting Boost, 5Vin, L=3.3uHy with soft start
 */

#ifndef LE9643_SM2_LVIBSS_HVIB_REALTEK_H
#define LE9643_SM2_LVIBSS_HVIB_REALTEK_H

#ifdef VP_API_TYPES_H
#include "vp_api_types.h"
#else
typedef unsigned char VpProfileDataType;
#endif


/************** Device Profile **************/
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9643_INVERT_DC_DC
extern const VpProfileDataType DEV_PROFILE_Le9643_90V_IB_10uH_SS[];/* Device Configuration Data - Le9643 90V 300Khz 10uHy IB SS */
#else
extern const VpProfileDataType DEV_PROFILE_Buck_Boost[];
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ZSI
extern const VpProfileDataType ZLR964122_BB100V_DEVICE[];
#else
extern const VpProfileDataType ZLR966122_BB100V_DEVICE[];	
#endif	
#endif

/************** DC Profile **************/
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9643_INVERT_DC_DC
extern const VpProfileDataType DC_FXS_miSLIC_IB90V_DEF[];
#else
extern const VpProfileDataType DC_FXS_MISLIC_BB_DEF[];/* DC FXS Default - Use for all countries unless country file exists - 25mA Current Feed */
#endif
	
/************** AC Profile **************/
extern const VpProfileDataType AC_FXS_RF14_600R_DEF[];/* AC FXS RF14 600R Normal Coefficients (Default)  */
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
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9643_INVERT_DC_DC
extern const VpProfileDataType RING_miSLIC_IB90V_DEF[];
#else
extern const VpProfileDataType RING_MISLIC_BB_DEF[]; /* Default Ringing 25Hz 50Vrms Fixed, AC Trip - Use for all countries unless country profile exists */
#endif

/************** Tone Profile **************/
extern const VpProfileDataType TONE_CLI[];           /* Caller ID Alert Tone */
extern const VpProfileDataType TONE_CALLWAIT[];      /* Call Waiting Tone for Type II Caller ID */
extern const VpProfileDataType T_US_DIAL[];          /* US Dial Tone (C_CONT) */
extern const VpProfileDataType T_US_RING[];          /* US Ringback Tone (C_00) */
extern const VpProfileDataType T_US_BUSY[];          /* US Busy Tone (C_01) */
extern const VpProfileDataType T_US_CONG[];          /* US Congestion Tone (C_02) */
extern const VpProfileDataType T_US_RECALL[];        /* US Recall Dial Tone (C_03) */
extern const VpProfileDataType T_US_WAITING[];       /* US Call Waiting (on Hold) (C_04) */

/************** Cadence Profile **************/
extern const VpProfileDataType LE880_RING_CAD_STD[];
extern const VpProfileDataType LE880_RING_CAD_SHORT[];

extern const VpProfileDataType LE886_RING_CAD_STD[];
extern const VpProfileDataType LE886_RING_CAD_SHORT[];
extern const VpProfileDataType C_CONT[];             /* Continuous ON Cadence */
extern const VpProfileDataType CR_CN[];              /* China Ringing Signal Cadence */

/************** Caller ID Profile **************/
extern const VpProfileDataType CID_TYPE2_US[];       /* US Caller ID Type II */
extern const VpProfileDataType CID_TYPE1_UK[];       /* UK Caller ID Type I */
extern const VpProfileDataType CLI_TYPE1_US[];       /* US Caller ID (Type 1 - On-Hook) */

/************** Metering_Profile **************/
extern const VpProfileDataType METER_12KHZ_RF14[];   /* Metering, 12kHz 0.5Vrms */
extern const VpProfileDataType METER_16KHZ_RF14[];   /* Metering, 16kHz 0.5Vrms */
extern const VpProfileDataType METER_1[];            /* Metering Index 1 */

#endif /* LE9643_SM2_LVIBSS_HVIB_REALTEK_H */

