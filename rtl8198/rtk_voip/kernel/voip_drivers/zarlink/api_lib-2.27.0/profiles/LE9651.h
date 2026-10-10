/*
 * LE9653_default.h --
 *
 * This header file exports the Profile data types
 *
 * Project Info --
 *   File:   C:\Users\ben_lin\Desktop\LE9653_default.vpw
 *   Type:   Design for ZLR965324H SM2 Line Module Featuring the  Le9643 and Le9653, Lite Coefficients
 *   Date:   Monday, August 26, 2019 17:47:09
 *   Device: miSLIC Le9653
 *
 *   This file was generated with Profile Wizard Version: P2.9.0
 *
 * Project Comments --
 *  ----------------------------------------------------------------------------------------------------------------------------------
 *  Profile Wizard Coefficient Revision 2.9 Release Notes:
 *  ----------------------------------------------------------------------------------------------------------------------------------
 *  I. General:
 *  1. This release includes support for the following 44 countries:
 *  Argentina (AR), Austria (AT), Australia (AU), Belgium (BE), Brazil (BR), Bulgaria (BG), Canada (CA), Switzerland (CH),
 *  Chile (CL), China (CN), Czech Republic (CZ), Germany (DE), Denmark (DK), Ecuador (EC), Spain (ES), Finland (FI),
 *  France (FR), UK (GB), Greece (GR), Hong Kong SAR China (HK), Hungary (HU), Indonesia (ID), Ireland (IE), Israel (IL),
 *  India (IN), Iceland (IS), Italy (IT), Japan (JP), S. Korea (KR), Mexico (MX), Malaysia (MY), Netherlands (NL),
 *  Norway (NO), New Zealand (NZ), Poland (PL), Portugal (PT), Russian Federation (RU), Sweden (SE), Singapore (SG),
 *  Thailand (TH), Turkey (TK), Taiwan (TW), USA (US), and South Africa (ZA).
 *  2. The coefficients in this and all releases are provided for use only with the Microsemi VoicePath API-II (VP-API-II). Please refer 
 *  to the terms and conditions for licensing the software regarding terms and conditions of usage. These profiles are provided for 
 *  reference only with no guarantee whatsoever by Microsemi Corporation.
 *  3. This release is for the ZLR965222H SM2 Line Module based on the Le9652.
 *  
 *  II. Device Profile:
 *  1. The default settings for the Device Profile are:
 *         PCLK = 8192 kHz
 *         PCM Transmit Edge = Positive
 *         Transmit Time Slot = 6 (for ZSI)
 *         Receive Time Slot = 0
 *         Interrupt Mode = Open Drain
 *         Switcher = Inverting Boost(12V in / 90V out)
 *         IO21 Pin Mode = Analog Voltage Sense
 *         IO22 Pin Mode = Digital
 *      
 *  2. The settings may be changed by the user as necessary.  Please refer to the VE960 and VP-API-II documentation for information 
 *  about the supported settings.
 *  
 *  II. DC Profiles:
 *  1. The DC_FXS_miSLIC_IB90V_DEF Profile is the default unless a country specific profile is selected. Example DC profile settings are
 *  provided for China, ETSI and the USA.
 *  
 *  III. AC Profiles:
 *  1. FXS Coefficients assume -6dBr RX (Output from chipset) and 0dB TX relative gain levels.
 *  2. Supported countries not individually listed should use the default 600R profile AC_FXS_RF14_600R_DEF.
 *  4. AC FXS Coefficients assume the use of two 7 ohm series resistors or PTCs. Customers using other PTC resistance values (such as 
 *  25ohms or 50 ohms) should not use these AC coefficients and can request alternate ones from Microsemi.
 *  5. This release includes both Narrowband and Wideband coefficients. Note that the VE960 Series devices support per channel Narrowband or
 *  Wideband audio selection.
 *  
 *  
 *  IV. Ring Profiles:
 *  1. RING_miSLIC_FB100V_DEF is the default ringing profile and should be used for all countries which do not have a listed ringing profile.  
 *  The default ringing profile is set for a sine wave ringing with an amplitude of 50Vrms (70.7Vpk)with no DC bias and a frequency of 25 Hz 
 *  tracking supply.
 *  2. Most ringing profiles on the list are sinusoidal with an amplitude of 50Vrms with no DC bias. 
 *  3. The ringing definitions may be changed based on the requirements of the target market as long as the total amplitude (AC + DC 
 *  components) does not exceed the limits set forth in the Le9652 data sheet.
 *  
 *  
 *  V. Tone Profiles:
 *  1. These profiles are available only in the full version of the VP-API-II.
 *  
 *  VI. Cadence Profiles:
 *  1. These profiles are available only in the full version of the VP-API-II.
 *  
 *  VII. Caller ID Profiles:
 *  1. These profiles are available only in the full version of the VP-API-II.
 *  
 *  VIII. Metering Profiles:
 *  1. These profiles are available only in the full version of the VP-API-II.
 *  
 *  (c) Copyright 2013 Microsemi Corporation. All rights reserved.
 *  
 *  -----------------------------------------------------------------------------------------------------------------------------------------------------
 */

#ifndef LE9651_H
#define LE9651_H

#ifdef VP_API_TYPES_H
#include "vp_api_types.h"
#else
typedef unsigned char VpProfileDataType;
#endif


/************** Device Profile **************/
extern const VpProfileDataType DEV_PROFILE_Le9653_135V_IB[];/* Device Configuration Data - Tracker 135V Inverting Boost */
extern const VpProfileDataType DEV_PROFILE_Le9653_135V_IB_ZSI[];/* Device Configuration Data - Tracker 135V Inverting Boost ZSI */

/************** DC Profile **************/
extern const VpProfileDataType DC_FXS_miSLIC_IB90V_DEF[];/* DC FXS Default - Use for all countries unless country file exists - 25mA Current Feed */
extern const VpProfileDataType DC_FXS_miSLIC_IB90V_CN[];/* China DC FXS Parameters - 20mA Current Feed */
extern const VpProfileDataType DC_FXS_miSLIC_IB90V_ETSI[];/* ETSI DC FXS Parameters - 25mA Current Feed */
extern const VpProfileDataType DC_FXS_miSLIC_IB90V_US[];/* USA DC FXS Parameters - 25mA Current Feed */

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
extern const VpProfileDataType RING_miSLIC_IB90V_DEF[];/* Default Ringing 25Hz 50Vrms Tracking, AC Trip - Use for all countries unless country profile exists */
extern const VpProfileDataType RING_miSLIC_IB90V_AT[];/* Austria Ringing 50Hz 50Vrms Tracking, AC Trip */
extern const VpProfileDataType RING_miSLIC_IB90V_CA[];/* Canada Ringing 20Hz 50Vrms Tracking, AC Trip */
extern const VpProfileDataType RING_miSLIC_IB90V_FI[];/* Finland Ringing 50Hz 50Vrms Tracking, AC Trip */
extern const VpProfileDataType RING_miSLIC_IB90V_FR[];/* France Ringing 50Hz 50Vrms Tracking, AC Trip */
extern const VpProfileDataType RING_miSLIC_IB90V_HK[];/* Hong Kong Ringing 20Hz 50Vrms Tracking, AC Trip */
extern const VpProfileDataType RING_miSLIC_IB90V_JP[];/* Japan Ringing 16Hz 50Vrms Tracking, AC Trip */
extern const VpProfileDataType RING_miSLIC_IB90V_KR[];/* Korea Ringing 20Hz 50Vrms Tracking, AC Trip */
extern const VpProfileDataType RING_miSLIC_IB90V_SG[];/* Singapore Ringing 24Hz 50Vrms Tracking, AC Trip */
extern const VpProfileDataType RING_miSLIC_IB90V_TW[];/* Taiwan Ringing 20 Hz 50vrms Tracking, AC Trip */
extern const VpProfileDataType RING_miSLIC_IB90V_US[];/* USA Ringing 20Hz 50Vrms Tracking, AC Trip */
extern const VpProfileDataType RING_MISLIC_IB135V_US[];/* USA AT&T Ringing 20Hz 62Vrms + 20VDC Tracking, ACTRIP%0D%0A */
extern const VpProfileDataType RING_Le9643_IB90V_60Vrms[];/* Le9643 Ringing 25Hz 60Vrms Tracking 3REN, AC Trip */

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

#endif /* LE9651_H */

