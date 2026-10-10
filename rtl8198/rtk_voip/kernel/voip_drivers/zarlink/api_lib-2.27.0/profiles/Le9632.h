/*
 * ZLR963222H_135VIB_Profile.h --
 *
 * This header file exports the Profile data types
 *
 * Project Info --
 *   File:   D:\data\Desktop\ZLR963222H_IB_SM2_FULL_Rev2_9.vpw
 *   Type:   Design for ZLR963222H SM2 Line Module Featuring the Le9632, Lite Coefficients
 *   Date:   Thursday, August 10, 2017 12:06:38
 *   Device: ZL880 ZL88702
 *
 *   This file was generated with Profile Wizard Version: P2.7.0
 *
 * Project Comments --
 *  ----------------------------------------------------------------------------------------------------------------------------------
 *  Profile Wizard Coefficient Revision 2.9 Release Notes:
 *  ----------------------------------------------------------------------------------------------------------------------------------
 *  I. General:
 *  1. This is the second release of the ZL88702 worldwide coefficients.
 *  2. This release includes support for the following 44 countries:
 *  Argentina (AR), Austria (AT), Australia (AU), Belgium (BE), Brazil (BR), Bulgaria (BG), Canada (CA), Switzerland (CH),
 *  Chile (CL), China (CN), Czech Republic (CZ), Germany (DE), Denmark (DK), Ecuador (EC), Spain (ES), Finland (FI),
 *  France (FR), UK (GB), Greece (GR), Hong Kong SAR China (HK), Hungary (HU), Indonesia (ID), Ireland (IE), Israel (IL),
 *  India (IN), Iceland (IS), Italy (IT), Japan (JP), S. Korea (KR), Mexico (MX), Malaysia (MY), Netherlands (NL),
 *  Norway (NO), New Zealand (NZ), Poland (PL), Portugal (PT), Russian Federation (RU), Sweden (SE), Singapore (SG),
 *  Thailand (TH), Turkey (TK), Taiwan (TW), USA (US), and South Africa (ZA).
 *  2. The coefficients in this and all releases are provided for use only with the Microsemi VoicePath API-II (VP-API-II). Please refer 
 *  to the terms and conditions for licensing the software regarding terms and conditions of usage. These profiles are provided for 
 *  reference only with no guarantee whatsoever by Microsemi Corporation.
 *  3. This release is for the ZLR88721H SM2 Line Module based on the ZL88702.
 *  
 *  II. Device Profile:
 *  1. The default settings for the Device Profile are:
 *         PCLK = 8192 kHz
 *         PCM Transmit Edge = Positive
 *         Transmit Time Slot = 0
 *         Receive Time Slot = 0
 *         Interrupt Mode = Open Drain
 *         Switcher = Flyback (12V in / 135V out)
 *         IO21 Pin Mode = Analog Voltage Sense
 *         IO22 Pin Mode = Digital
 *      
 *  2. The settings may be changed by the user as necessary.  Please refer to the ZL880 and VP-API-II documentation for information 
 *  about the supported settings.
 *  
 *  II. DC Profiles:
 *  1. The DC_FXS_ZL880_FB150V_DEF Profile is the default unless a country specific profile is selected. Example DC profile settings are
 *  provided for China, ETSI and the USA.
 *  
 *  III. AC Profiles:
 *  1. FXS Coefficients assume -6dBr RX (Output from chipset) and 0dB TX relative gain levels.
 *  2. Supported countries not individually listed should use the default 600R profile AC_FXS_RF14_600R_DEF.
 *  4. AC FXS Coefficients assume the use of two 7 ohm series resistors or PTCs. Customers using other PTC resistance values (such as 
 *  25ohms or 50 ohms) should not use these AC coefficients and can request alternate ones from Microsemi.
 *  5. This release includes both Narrowband and Wideband coefficients. Note that the ZL880 Series devices support per channel Narrowband or 
 *  Wideband audio selection.
 *  
 *  
 *  IV. Ring Profiles:
 *  1. RING_ZL880_FB150V_DEF is the default ringing profile and should be used for all countries which do not have a listed ringing profile.  
 *  The default ringing profile is set for a sine wave ringing with an amplitude of 62Vrms (87.7Vpk)with 48V DC bias and a frequency of 25 Hz
 *  tracking supply.
 *  2. Most ringing profiles on the list are sinusoidal with an amplitude of 62Vrms with 48V DC bias. 
 *  3. The ringing definitions may be changed based on the requirements of the target market as long as the total amplitude (AC + DC 
 *  components) does not exceed the limits set forth in the ZL88701/702 data sheet.
 *  
 *  
 *  V. Tone Profiles:
 *  1. These profiles are available only in the full version of the VP-API-II.
 *  2. The shown levels assume a 6dB attenuation in the chipset before being outputed to line.
 *  3. Call progress tone levels may be arbitrary as they are not always specified in national standards, or the standards may not be available 
 *  to Microsemi.
 *  4. ITU-T Recommendation E.180 (03/1998) revised on 02/2003 and ETSI TR 101 041-2 V.1.1.1 (05/1997) were used if national standards were not
 *  available.
 *  5. Recommended ETSI ES 201 970 call progress tones are provided for reference.
 *  6. Modulated tones f1 x f2 are approximated as the sum of f1 + (f1+f2)/2 + (f1-f2)/2.
 *  7. The data in these profiles may be changed by the user as necessary.
 *   8. T_CAS_DEF is not a country-specific tone and is used by several national Caller ID profiles.
 *  
 *  VI. Cadence Profiles:
 *  1. These profiles are available only in the full version of the VP-API-II.
 *  2.  ITU-T Recommendation E.180 (03/1998) revised on 02/2003 and ETSI TR 101 041-2 V.1.1.1 (05/1997) were used if national standards were not
 *  available.
 *  3. Recommended ETSI ES 201 970 call progress cadences are provided for reference.
 *  4. Some countries support multiple call progress tone cadences.  The ones used are believed to be representative and most common.  The user may
 *  wish to edit some of the cadence definitions or add additional cadences.
 *  5. Ringing signal cadences include the alerting signal(s) and necessary delays for Type 1 Caller ID, if it is supported below in the Caller ID 
 *  Profiles.
 *  
 *  VII. Caller ID Profiles:
 *  1. These profiles are available only in the full version of the VP-API-II.
 *  2. The option to calculate the checksum in the VP-API-II is selected for all countries except Japan, which requires that the CRC checksum be 
 *  calculated by host application.
 *  
 *  VIII. Metering Profiles:
 *  1. These profiles are available only in the full version of the VP-API-II.
 *  2. Metering profiles are provided for 12kHz and 16kHz signaling. They are not currently listed by the countries that use them. 
 *  
 *  (c) Copyright 2012 Microsemi Corporation. All rights reserved.
 *  
 *  -----------------------------------------------------------------------------------------------------------------------------------------------------
 */

#ifndef ZLR963222H_135VIB_PROFILE_H
#define ZLR963222H_135VIB_PROFILE_H

#ifdef VP_API_TYPES_H
#include "vp_api_types.h"
#else
typedef unsigned char VpProfileDataType;
#endif


/************** Device_Parameters **************/
extern const VpProfileDataType ZLR963222H_IB135V_DEVICE[];/* Device Configuration Data - Le9632 Tracker 135V IB */
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_MICROSEMI_LE9632_IB
extern const VpProfileDataType ZLR963222H_IB135V_DEVICE[];
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_MICROSEMI_LE9632_FB)
extern const VpProfileDataType ZLR963221_FB150V_DEVICE[];
#endif	
/************** DC_Coefficients **************/
extern const VpProfileDataType DC_FXS_ZL880_FB150V_DEF[];/* DC FXS Default - Use for all countries unless country file exists - 25mA Current Feed */

/************** AC_Coefficients **************/
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

/************** Ring_Parameters **************/
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_MICROSEMI_LE9632_IB
extern const VpProfileDataType RING_ZL880_IB150V_DEF[];
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_MICROSEMI_LE9632_FB)
extern const VpProfileDataType RING_ZL880_FB150V_DEF[];
#endif	
extern const VpProfileDataType RING_ZL880_IB150V_DEF[];/* Default Ringing 25Hz 62Vrms + 48VDC Tracking, DC Trip - Use for all countries unless country profile exists */
extern const VpProfileDataType RING_ZL880_FB150V_AT[];/* Austria Ringing 50Hz 62Vrms + 48VDC Tracking, DC Trip */
extern const VpProfileDataType RING_ZL880_FB150V_CA[];/* Canada Ringing 20Hz 62Vrms + 48VDC Tracking, DC Trip */
extern const VpProfileDataType RING_ZL880_FB150V_FI[];/* Finland Ringing 50Hz 62Vrms + 48VDC Tracking, DC Trip */
extern const VpProfileDataType RING_ZL880_FB150V_FR[];/* France Ringing 50Hz 62Vrms + 48VDC Tracking, DC Trip */
extern const VpProfileDataType RING_ZL880_FB150V_HK[];/* Hong Kong Ringing 20Hz 62Vrms + 48VDC Tracking, DC Trip */
extern const VpProfileDataType RING_ZL880_FB150V_JP[];/* Japan Ringing 16Hz 62Vrms + 48VDC Tracking, DC Trip */
extern const VpProfileDataType RING_ZL880_FB150V_KR[];/* Korea Ringing 20Hz 62Vrms + 48VDC Tracking, DC Trip */
extern const VpProfileDataType RING_ZL880_FB150V_SG[];/* Singapore 24Hz 62Vrms + 48VDC Tracking, DC Trip */
extern const VpProfileDataType RING_ZL880_FB150V_TW[];/* Taiwan Ringing 20Hz 62Vrms + 48VDC Tracking, DC Trip */
extern const VpProfileDataType RING_ZL880_FB150V_US[];/* USA Ringing 20Hz 62Vrms + 48VDC Tracking, DC Trip */
extern const VpProfileDataType RING_ZL880_IB135V_US[];/* USA Ringing 20Hz 62Vrms + 20VDC Tracking, DC Trip */
extern const VpProfileDataType RING_ZL880_IB135V_70VRMS_50HZ_US[];/* USA Ringing 20Hz 52Vrms */
extern const VpProfileDataType RING_ZL880_IB135V_70VRMS_25HZ_US[];/* USA Ringing 20Hz 52Vrms */
extern const VpProfileDataType RING_ZL880_IB135V_70VRMS_20HZ_US[];/* USA Ringing 20Hz 52Vrms */
extern const VpProfileDataType RING_ZL880_IB135V_63VRMS_50HZ_US[];/* USA Ringing 20Hz 52Vrms */
extern const VpProfileDataType RING_ZL880_IB135V_63VRMS_25HZ_US[];/* USA Ringing 20Hz 52Vrms */
extern const VpProfileDataType RING_ZL880_IB135V_63VRMS_20HZ_US[];/* USA Ringing 20Hz 52Vrms */
extern const VpProfileDataType RING_ZL880_IB135V_52VRMS_50HZ_US[];/* USA Ringing 20Hz 52Vrms */
extern const VpProfileDataType RING_ZL880_IB135V_52VRMS_25HZ_US[];/* USA Ringing 20Hz 52Vrms */
extern const VpProfileDataType RING_ZL880_IB135V_52VRMS_20HZ_US[];/* USA Ringing 20Hz 52Vrms */

/************** Call_Progress_Tones **************/
extern const VpProfileDataType T_CAS_DEF[];          /* Caller ID Alert Tone (CAS) - Used in Many Country Caller ID Profiles */
extern const VpProfileDataType T_DL_ETSI[];          /* ETSI ES201 970 Dial Tone  */
extern const VpProfileDataType T_RB_ETSI[];          /* ETSI ES201 970 Ringback Tone  */
extern const VpProfileDataType T_BS_ETSI[];          /* ETSI ES201 970 Busy Tone  */
extern const VpProfileDataType T_CW_ETSI[];          /* ETSI ES201 970 Call Waiting Tone */
extern const VpProfileDataType T_CG_ETSI[];          /* ETSI ES201 970 Congestion Tone */
extern const VpProfileDataType T_DL_AR[];            /* Argentina Dial Tone  */
extern const VpProfileDataType T_RB_AR[];            /* Argentina Ringback Tone  */
extern const VpProfileDataType T_BS_AR[];            /* Argentina Busy Tone  */
extern const VpProfileDataType T_CW_AR[];            /* Argentina Call Waiting Tone */
extern const VpProfileDataType T_CG_AR[];            /* Argentina Congestion Tone */
extern const VpProfileDataType T_DL_AT[];            /* Austria Dial Tone  */
extern const VpProfileDataType T_RB_AT[];            /* Austria Ringback Tone  */
extern const VpProfileDataType T_BS_AT[];            /* Austria Busy Tone  */
extern const VpProfileDataType T_CW_AT[];            /* Austria Call Waiting Tone */
extern const VpProfileDataType T_CG_AT[];            /* Austria Congestion Tone */
extern const VpProfileDataType T_DL_AU[];            /* Australia Dial Tone  */
extern const VpProfileDataType T_RB_AU[];            /* Australia Ringback Tone  */
extern const VpProfileDataType T_BS_AU[];            /* Australia Busy Tone  */
extern const VpProfileDataType T_CW_AU[];            /* Australia Call Waiting Tone */
extern const VpProfileDataType T_CG_AU[];            /* Australia Congestion Tone  */
extern const VpProfileDataType T_DL_BE[];            /* Belgium Dial Tone  */
extern const VpProfileDataType T_RB_BE[];            /* Belgium Ringback Tone  */
extern const VpProfileDataType T_BS_BE[];            /* Belgium Busy Tone  */
extern const VpProfileDataType T_CW_BE[];            /* Belgium Call Waiting Tone */
extern const VpProfileDataType T_CG_BE[];            /* Belgium Congestion Tone */
extern const VpProfileDataType T_DL_BG[];            /* Bulgaria Dial Tone  */
extern const VpProfileDataType T_RG_BG[];            /* Bulgaria Ringback Tone  */
extern const VpProfileDataType T_BS_BG[];            /* Bulgaria Busy Tone  */
extern const VpProfileDataType T_CW_BG[];            /* Bulgaria Call Waiting Tone  */
extern const VpProfileDataType T_CG_BG[];            /* Bulgaria Congestion Tone */
extern const VpProfileDataType T_DL_BR[];            /* Brazil Dial Tone  */
extern const VpProfileDataType T_RB_BR[];            /* Brazil Ringback Tone  */
extern const VpProfileDataType T_BS_BR[];            /* Brazil Busy Tone  */
extern const VpProfileDataType T_CW_BR[];            /* Brazil Call Waiting Tone */
extern const VpProfileDataType T_DL_CA[];            /* Canada Dial Tone  */
extern const VpProfileDataType T_RB_CA[];            /* Canada Ringback Tone  */
extern const VpProfileDataType T_BS_CA[];            /* Canada Busy Tone  */
extern const VpProfileDataType T_CW_CA[];            /* Canada Call Waiting Tone */
extern const VpProfileDataType T_CG_CA[];            /* Canada Congestion Tone */
extern const VpProfileDataType T_DL_CH[];            /* Switzerland Dial Tone  */
extern const VpProfileDataType T_RB_CH[];            /* Switzerland Ringback Tone  */
extern const VpProfileDataType T_BS_CH[];            /* Switzerland Busy Tone  */
extern const VpProfileDataType T_CW_CH[];            /* Switzerland Call Waiting Tone */
extern const VpProfileDataType T_CG_CH[];            /* Switzerland Congestion Tone */
extern const VpProfileDataType T_DL_CL[];            /* Chile Dial Tone  */
extern const VpProfileDataType T_RG_CL[];            /* Chile Ringback Tone  */
extern const VpProfileDataType T_BS_CL[];            /* Chile Busy Tone  */
extern const VpProfileDataType T_CW_CL[];            /* Chile Call Waiting Tone  */
extern const VpProfileDataType T_CG_CL[];            /* Chile Congestion Tone */
extern const VpProfileDataType T_DL_CN[];            /* China Dial Tone  */
extern const VpProfileDataType T_RB_CN[];            /* China Ringback Tone  */
extern const VpProfileDataType T_BS_CN[];            /* China Busy Tone  */
extern const VpProfileDataType T_CW_CN[];            /* China Call Waiting Tone */
extern const VpProfileDataType T_CG_CN[];            /* China Congestion Tone */
extern const VpProfileDataType T_DL_CZ[];            /* Czech Republic Dial Tone  */
extern const VpProfileDataType T_RG_CZ[];            /* Czech Republic Ringback Tone  */
extern const VpProfileDataType T_BS_CZ[];            /* Czech Republic Busy Tone  */
extern const VpProfileDataType T_CW_CZ[];            /* Czech Republic Call Waiting Tone  */
extern const VpProfileDataType T_CG_CZ[];            /* Czech Republic Congestion Tone */
extern const VpProfileDataType T_DL_DE[];            /* Germany Dial Tone  */
extern const VpProfileDataType T_RB_DE[];            /* Germany Ringback Tone */
extern const VpProfileDataType T_BS_DE[];            /* Germany Busy Tone */
extern const VpProfileDataType T_CW_DE[];            /* Germany Call Waiting Tone */
extern const VpProfileDataType T_CG_DE[];            /* Germany Congestion Tone  */
extern const VpProfileDataType T_DL_DK[];            /* Denmark Dial Tone  */
extern const VpProfileDataType T_RG_DK[];            /* Denmark Ringback Tone  */
extern const VpProfileDataType T_BS_DK[];            /* Denmark Busy Tone  */
extern const VpProfileDataType T_CW_DK[];            /* Denmark Call Waiting Tone  */
extern const VpProfileDataType T_CG_DK[];            /* Denmark Congestion Tone */
extern const VpProfileDataType T_DL_EC[];            /* Ecuador Dial Tone  */
extern const VpProfileDataType T_RG_EC[];            /* Ecuador Ringback Tone  */
extern const VpProfileDataType T_BS_EC[];            /* Ecuador Busy Tone  */
extern const VpProfileDataType T_CW_EC[];            /* Ecuador Call Waiting Tone  */
extern const VpProfileDataType T_CG_EC[];            /* Ecuador Congestion Tone */
extern const VpProfileDataType T_DL_ES[];            /* Spain Dial Tone  */
extern const VpProfileDataType T_RB_ES[];            /* Spain Ringback Tone  */
extern const VpProfileDataType T_BS_ES[];            /* Spain Busy Tone  */
extern const VpProfileDataType T_CW_ES[];            /* Spain Call Waiting Tone */
extern const VpProfileDataType T_CG_ES[];            /* Spain Congestion Tone */
extern const VpProfileDataType T_DL_FI[];            /* Finland Dial Tone  */
extern const VpProfileDataType T_RG_FI[];            /* Finland Ringback Tone  */
extern const VpProfileDataType T_BS_FI[];            /* Finland Busy Tone  */
extern const VpProfileDataType T_CW_FI[];            /* Finland Call Waiting Tone  */
extern const VpProfileDataType T_CG_FI[];            /* Finland Congestion Tone */
extern const VpProfileDataType T_DL_FR[];            /* France Dial Tone  */
extern const VpProfileDataType T_RG_FR[];            /* France Ringback Tone  */
extern const VpProfileDataType T_BS_FR[];            /* France Busy Tone  */
extern const VpProfileDataType T_CW_FR[];            /* France Call Waiting Tone  */
extern const VpProfileDataType T_DL_GB[];            /* UK Dial Tone  */
extern const VpProfileDataType T_RB_GB[];            /* UK Ringback Tone  */
extern const VpProfileDataType T_BS_GB[];            /* UK Busy Tone  */
extern const VpProfileDataType T_CW_GB[];            /* UK Call Waiting Tone */
extern const VpProfileDataType T_CG_GB[];            /* UK Congestion Tone  */
extern const VpProfileDataType T_DL_GR[];            /* Greece Dial Tone  */
extern const VpProfileDataType T_RG_GR[];            /* Greece Ringback Tone  */
extern const VpProfileDataType T_BS_GR[];            /* Greece Busy Tone  */
extern const VpProfileDataType T_CW_GR[];            /* Greece Call Waiting Tone  */
extern const VpProfileDataType T_CG_GR[];            /* Greece Congestion Tone */
extern const VpProfileDataType T_DL_HK[];            /* Hong Kong SAR Dial Tone  */
extern const VpProfileDataType T_RB_HK[];            /* Hong Kong SAR Ringback Tone  */
extern const VpProfileDataType T_BS_HK[];            /* Hong Kong SAR Busy Tone  */
extern const VpProfileDataType T_CW_HK[];            /* Hong Kong SAR Call Waiting Tone */
extern const VpProfileDataType T_CG_HK[];            /* Hong Kong SAR Congestion Tone */
extern const VpProfileDataType T_DL_HU[];            /* Hungary Dial Tone  */
extern const VpProfileDataType T_RG_HU[];            /* Hungary Ringback Tone  */
extern const VpProfileDataType T_BS_HU[];            /* Hungary Busy Tone  */
extern const VpProfileDataType T_CW_HU[];            /* Hungary Call Waiting Tone  */
extern const VpProfileDataType T_CG_HU[];            /* Hungary Congestion Tone */
extern const VpProfileDataType T_DL_ID[];            /* Indonesia Dial Tone  */
extern const VpProfileDataType T_RG_ID[];            /* Indonesia Ringback Tone  */
extern const VpProfileDataType T_BS_ID[];            /* Indonesia Busy Tone  */
extern const VpProfileDataType T_CW_ID[];            /* Indonesia Call Waiting Tone  */
extern const VpProfileDataType T_CG_ID[];            /* Indonesia Congestion Tone */
extern const VpProfileDataType T_DL_IE[];            /* Ireland Dial Tone  */
extern const VpProfileDataType T_RB_IE[];            /* Ireland Ringback Tone  */
extern const VpProfileDataType T_BS_IE[];            /* Ireland Busy Tone  */
extern const VpProfileDataType T_CW_IE[];            /* Ireland Call Waiting Tone */
extern const VpProfileDataType T_DL_IL[];            /* Israel Dial Tone  */
extern const VpProfileDataType T_RB_IL[];            /* Israel Ringback Tone  */
extern const VpProfileDataType T_BS_IL[];            /* Israel Busy Tone  */
extern const VpProfileDataType T_CW_IL[];            /* Israel Call Waiting Tone */
extern const VpProfileDataType T_CG_IL[];            /* Israel Congestion Tone (SIT) */
extern const VpProfileDataType T_DL_IN[];            /* India Dial Tone  */
extern const VpProfileDataType T_RB_IN[];            /* India Ringback Tone  */
extern const VpProfileDataType T_BS_IN[];            /* India Busy Tone  */
extern const VpProfileDataType T_CW_IN[];            /* India Call Waiting Tone */
extern const VpProfileDataType T_CG_IN[];            /* India Congestion Tone  */
extern const VpProfileDataType T_DL_IS[];            /* Iceland Dial Tone  */
extern const VpProfileDataType T_RG_IS[];            /* Iceland Ringback Tone  */
extern const VpProfileDataType T_BS_IS[];            /* Iceland Busy Tone  */
extern const VpProfileDataType T_CW_IS[];            /* Iceland Call Waiting Tone  */
extern const VpProfileDataType T_CG_IS[];            /* Iceland Congestion Tone */
extern const VpProfileDataType T_DL_IT[];            /* Italy Dial Tone  */
extern const VpProfileDataType T_RB_IT[];            /* Italy Ringback Tone */
extern const VpProfileDataType T_BS_IT[];            /* Italy Busy Tone */
extern const VpProfileDataType T_CW_IT[];            /* Italy Call Waiting Tone */
extern const VpProfileDataType T_CG_IT[];            /* Italy Congestion Tone  */
extern const VpProfileDataType T_DL_JP[];            /* Japan Dial Tone  */
extern const VpProfileDataType T_RB_JP[];            /* Japan Ringback Tone  */
extern const VpProfileDataType T_BS_JP[];            /* Japan Busy Tone  */
extern const VpProfileDataType T_CW_JP[];            /* Japan Call Waiting Tone */
extern const VpProfileDataType T_AC_JP[];            /* Japan Caller ID Alert Tone - Digit C */
extern const VpProfileDataType T_AD_JP[];            /* Japan Caller ID Alert Tone - Digit D */
extern const VpProfileDataType T_DL_KR[];            /* S. Korea Dial Tone  */
extern const VpProfileDataType T_RB_KR[];            /* S. Korea Ringback Tone  */
extern const VpProfileDataType T_BS_KR[];            /* S. Korea Busy Tone  */
extern const VpProfileDataType T_CW_KR[];            /* S. Korea Call Waiting Tone */
extern const VpProfileDataType T_CG_KR[];            /* S. Korea Congestion Tone */
extern const VpProfileDataType T_DL_MX[];            /* Mexico Dial Tone  */
extern const VpProfileDataType T_RG_MX[];            /* Mexico Ringback Tone  */
extern const VpProfileDataType T_BS_MX[];            /* Mexico Busy Tone  */
extern const VpProfileDataType T_CG_MX[];            /* Mexico Congestion Tone */
extern const VpProfileDataType T_DL_MY[];            /* Malaysia Dial Tone  */
extern const VpProfileDataType T_RB_MY[];            /* Malaysia Ringback Tone */
extern const VpProfileDataType T_BS_MY[];            /* Malaysia Busy Tone */
extern const VpProfileDataType T_CG_MY[];            /* Malaysia Congestion Tone  */
extern const VpProfileDataType T_DL_NL[];            /* Netherlands Dial Tone  */
extern const VpProfileDataType T_RB_NL[];            /* Netherlands Ringback Tone  */
extern const VpProfileDataType T_BS_NL[];            /* Netherlands Busy Tone  */
extern const VpProfileDataType T_CW_NL[];            /* Netherlands Call Waiting Tone */
extern const VpProfileDataType T_CG_NL[];            /* Netherlands Congestion Tone */
extern const VpProfileDataType T_DL_NO[];            /* Norway Dial Tone  */
extern const VpProfileDataType T_RB_NO[];            /* Norway Ringback Tone  */
extern const VpProfileDataType T_BS_NO[];            /* Norway Busy Tone  */
extern const VpProfileDataType T_CW_NO[];            /* Norway Call Waiting Tone */
extern const VpProfileDataType T_CG_NO[];            /* Norway Congestion Tone */
extern const VpProfileDataType T_DL_NZ[];            /* New Zealand Dial Tone  */
extern const VpProfileDataType T_RB_NZ[];            /* New Zealand Ringback Tone  */
extern const VpProfileDataType T_BS_NZ[];            /* New Zealand Busy Tone  */
extern const VpProfileDataType T_CW_NZ[];            /* New Zealand Call Waiting Tone */
extern const VpProfileDataType T_CG_NZ[];            /* New Zealand Congestion Tone  */
extern const VpProfileDataType T_DL_PL[];            /* Poland Dial Tone  */
extern const VpProfileDataType T_RG_PL[];            /* Poland Ringback Tone  */
extern const VpProfileDataType T_BS_PL[];            /* Poland Busy Tone  */
extern const VpProfileDataType T_CW_PL[];            /* Poland Call Waiting Tone  */
extern const VpProfileDataType T_CG_PL[];            /* Poland Congestion Tone */
extern const VpProfileDataType T_DL_PT[];            /* Portugal Dial Tone  */
extern const VpProfileDataType T_RB_PT[];            /* Portugal Ringback Tone  */
extern const VpProfileDataType T_BS_PT[];            /* Portugal Busy Tone  */
extern const VpProfileDataType T_CW_PT[];            /* Portugal Call Waiting Tone */
extern const VpProfileDataType T_CG_PT[];            /* Portugal Congestion Tone */
extern const VpProfileDataType T_DL_RU[];            /* Russia Dial Tone  */
extern const VpProfileDataType T_RG_RU[];            /* Russia Ringback Tone  */
extern const VpProfileDataType T_BS_RU[];            /* Russia Busy Tone  */
extern const VpProfileDataType T_CW_RU[];            /* Russia Call Waiting Tone  */
extern const VpProfileDataType T_CG_RU[];            /* Russia Congestion Tone */
extern const VpProfileDataType T_DL_SE[];            /* Sweden Dial Tone  */
extern const VpProfileDataType T_RB_SE[];            /* Sweden Ringback Tone  */
extern const VpProfileDataType T_BS_SE[];            /* Sweden Busy Tone  */
extern const VpProfileDataType T_CW_SE[];            /* Sweden Call Waiting Tone */
extern const VpProfileDataType T_CG_SE[];            /* Sweden Congestion Tone */
extern const VpProfileDataType T_DL_SG[];            /* Singapore Dial Tone  */
extern const VpProfileDataType T_RB_SG[];            /* Singapore Ringback Tone */
extern const VpProfileDataType T_BS_SG[];            /* Singapore Busy Tone */
extern const VpProfileDataType T_CW_SG[];            /* Singapore Call Waiting Tone */
extern const VpProfileDataType T_CG_SG[];            /* Singapore Congestion Tone  */
extern const VpProfileDataType T_DL_TH[];            /* Thailand Dial Tone  */
extern const VpProfileDataType T_RG_TH[];            /* Thailand Ringback Tone  */
extern const VpProfileDataType T_BS_TH[];            /* Thailand Busy Tone  */
extern const VpProfileDataType T_CW_TH[];            /* Thailand Call Waiting Tone  */
extern const VpProfileDataType T_CG_TH[];            /* Thailand Congestion Tone */
extern const VpProfileDataType T_DL_TK[];            /* Turkey Dial Tone  */
extern const VpProfileDataType T_RG_TK[];            /* Turkey Ringback Tone  */
extern const VpProfileDataType T_BS_TK[];            /* Turkey Busy Tone  */
extern const VpProfileDataType T_CW_TK[];            /* Turkey Call Waiting Tone  */
extern const VpProfileDataType T_CG_TK[];            /* Turkey Congestion Tone */
extern const VpProfileDataType T_DL_TW[];            /* Taiwan Dial Tone  */
extern const VpProfileDataType T_RB_TW[];            /* Taiwan Ringback Tone  */
extern const VpProfileDataType T_BS_TW[];            /* Taiwan Busy Tone  */
extern const VpProfileDataType T_CW_TW[];            /* Taiwan Call Waiting Tone */
extern const VpProfileDataType T_CG_TW[];            /* Taiwan Congestion Tone */
extern const VpProfileDataType T_DL_US[];            /* USA Dial Tone  */
extern const VpProfileDataType T_RB_US[];            /* USA Ringback Tone  */
extern const VpProfileDataType T_BS_US[];            /* USA Busy Tone  */
extern const VpProfileDataType T_CW_US[];            /* USA Call Waiting Tone */
extern const VpProfileDataType T_CG_US[];            /* USA Congestion Tone */
extern const VpProfileDataType T_DL_ZA[];            /* South Africa Dial Tone  */
extern const VpProfileDataType T_RB_ZA[];            /* South Africa Ringback Tone  */
extern const VpProfileDataType T_BS_ZA[];            /* South Africa Busy Tone  */
extern const VpProfileDataType T_CW_ZA[];            /* South Africa Call Waiting */
extern const VpProfileDataType T_CG_ZA[];            /* South Africa Congestion Tone  */

/************** Cadence_Definitions **************/
extern const VpProfileDataType LE880_RING_CAD_STD[];
extern const VpProfileDataType LE880_RING_CAD_SHORT[];
extern const VpProfileDataType LE886_RING_CAD_STD[];
extern const VpProfileDataType LE886_RING_CAD_SHORT[];
extern const VpProfileDataType CT_CONT[];            /* Continuous ON Tone Cadence  */
extern const VpProfileDataType CR_ETSI[];            /* ETSI ES201 970 Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_ETSI[];         /* ETSI ES201 970 Dial Tone Cadence */
extern const VpProfileDataType CT_RB_ETSI[];         /* ETSI ES201 970 Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_ETSI[];         /* ETSI ES201 970 Busy Tone Cadence */
extern const VpProfileDataType CT_CW_ETSI[];         /* ETSI ES201 970 Call Waiting Cadence */
extern const VpProfileDataType CT_CG_ETSI[];         /* ETSI ES201 970 Congestion Cadence */
extern const VpProfileDataType CR_AR[];              /* Argentina Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_AR[];           /* Argentina Dial Tone Cadence */
extern const VpProfileDataType CT_RB_AR[];           /* Argentina Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_AR[];           /* Argentina Busy Tone Cadence */
extern const VpProfileDataType CT_CW_AR[];           /* Argentina Call Waiting Cadence */
extern const VpProfileDataType CT_CG_AR[];           /* Argentina Congestion Cadence */
extern const VpProfileDataType CR_AT[];              /* Austria Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_AT[];           /* Austria Dial Tone Cadence */
extern const VpProfileDataType CT_RB_AT[];           /* Austria Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_AT[];           /* Austria Busy Tone Cadence */
extern const VpProfileDataType CT_CW_AT[];           /* Austria Call Waiting Cadence */
extern const VpProfileDataType CT_CG_AT[];           /* Austria Congestion Cadence */
extern const VpProfileDataType CR_AU[];              /* Australia Ringing Signal Cadence  */
extern const VpProfileDataType CT_DL_AU[];           /* Australia Dial Tone Cadence */
extern const VpProfileDataType CT_RB_AU[];           /* Australia Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_AU[];           /* Australia Busy Tone Cadence */
extern const VpProfileDataType CT_CW_AU[];           /* Australia Call Waiting Tone Cadence */
extern const VpProfileDataType CT_CG_AU[];           /* Australia Congestion Tone Cadence */
extern const VpProfileDataType CR_BE[];              /* Belgium Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_BE[];           /* Belgium Dial Tone Cadence */
extern const VpProfileDataType CT_RB_BE[];           /* Belgium Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_BE[];           /* Belgium Busy Tone Cadence */
extern const VpProfileDataType CT_CW_BE[];           /* Belgium Call Waiting Cadence */
extern const VpProfileDataType CT_CG_BE[];           /* Belgium Congestion Cadence */
extern const VpProfileDataType CR_BG[];              /* Bulgaria Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_BG[];           /* Bulgaria Dial Tone Cadence */
extern const VpProfileDataType CT_RB_BG[];           /* Bulgaria Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_BG[];           /* Bulgaria Busy Tone Cadence */
extern const VpProfileDataType CT_CW_BG[];           /* Bulgaria Call Waiting Cadence */
extern const VpProfileDataType CT_CG_BG[];           /* Bulgaria Congestion Cadence */
extern const VpProfileDataType CR_BR[];              /* Brazil Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_BR[];           /* Brazil Dial Tone Cadence */
extern const VpProfileDataType CT_RB_BR[];           /* Brazil Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_BR[];           /* Brazil Busy Tone Cadence */
extern const VpProfileDataType CT_CW_BR[];           /* Brazil Call Waiting Cadence */
extern const VpProfileDataType CR_CA[];              /* Canada Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_CA[];           /* Canada Dial Tone Cadence */
extern const VpProfileDataType CT_RB_CA[];           /* Canada Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_CA[];           /* Canada Busy Tone Cadence */
extern const VpProfileDataType CT_CW_CA[];           /* Canada Call Waiting Cadence */
extern const VpProfileDataType CT_CG_CA[];           /* Canada Congestion Cadence */
extern const VpProfileDataType CR_CH[];              /* Switzerland Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_CH[];           /* Switzerland Dial Tone Cadence */
extern const VpProfileDataType CT_RB_CH[];           /* Switzerland Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_CH[];           /* Switzerland Busy Tone Cadence */
extern const VpProfileDataType CT_CW_CH[];           /* Switzerland Call Waiting Cadence */
extern const VpProfileDataType CT_CG_CH[];           /* Switzerland Congestion Cadence */
extern const VpProfileDataType CR_CL[];              /* Chile Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_CL[];           /* Chile Dial Tone Cadence */
extern const VpProfileDataType CT_RB_CL[];           /* Chile Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_CL[];           /* Chile Busy Tone Cadence */
extern const VpProfileDataType CT_CW_CL[];           /* Chile Call Waiting Cadence */
extern const VpProfileDataType CT_CG_CL[];           /* Chile Congestion Cadence */
extern const VpProfileDataType CR_CN[];              /* China Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_CN[];           /* China Dial Tone Cadence */
extern const VpProfileDataType CT_RB_CN[];           /* China Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_CN[];           /* China Busy Tone Cadence */
extern const VpProfileDataType CT_CW_CN[];           /* China Call Waiting Cadence */
extern const VpProfileDataType CT_CG_CN[];           /* China Congestion Cadence */
extern const VpProfileDataType CR_CZ[];              /* Czech Republic Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_CZ[];           /* Czech Republic Dial Tone Cadence */
extern const VpProfileDataType CT_RB_CZ[];           /* Czech Republic Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_CZ[];           /* Czech Republic Busy Tone Cadence */
extern const VpProfileDataType CT_CW_CZ[];           /* Czech Republic Call Waiting Cadence */
extern const VpProfileDataType CT_CG_CZ[];           /* Czech Republic Congestion Cadence */
extern const VpProfileDataType CR_DE[];              /* Germany Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_DE[];           /* Germany Dial Tone Cadence */
extern const VpProfileDataType CT_RB_DE[];           /* Germany Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_DE[];           /* Germany Busy Tone Cadence */
extern const VpProfileDataType CT_CW_DE[];           /* Germany Call Waiting Cadence */
extern const VpProfileDataType CT_CG_DE[];           /* Germany Congestion Cadence */
extern const VpProfileDataType CR_DK[];              /* Denmark Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_DK[];           /* Denmark Dial Tone Cadence */
extern const VpProfileDataType CT_RB_DK[];           /* Denmark Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_DK[];           /* Denmark Busy Tone Cadence */
extern const VpProfileDataType CT_CW_DK[];           /* Denmark Call Waiting Cadence */
extern const VpProfileDataType CT_CG_DK[];           /* Denmark Congestion Cadence */
extern const VpProfileDataType CR_EC[];              /* Ecuador Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_EC[];           /* Ecuador Dial Tone Cadence */
extern const VpProfileDataType CT_RB_EC[];           /* Ecuador Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_EC[];           /* Ecuador Busy Tone Cadence */
extern const VpProfileDataType CT_CW_EC[];           /* Ecuador Call Waiting Cadence */
extern const VpProfileDataType CT_CG_EC[];           /* Ecuador Congestion Cadence */
extern const VpProfileDataType CR_ES[];              /* Spain Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_ES[];           /* Spain Dial Tone Cadence */
extern const VpProfileDataType CT_RB_ES[];           /* Spain Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_ES[];           /* Spain Busy Tone Cadence */
extern const VpProfileDataType CT_CW_ES[];           /* Spain Call Waiting Cadence */
extern const VpProfileDataType CT_CG_ES[];           /* Spain Congestion Cadence */
extern const VpProfileDataType CR_FI[];              /* Finland Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_FI[];           /* Finland Dial Tone Cadence */
extern const VpProfileDataType CT_RB_FI[];           /* Finland Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_FI[];           /* Finland Busy Tone Cadence */
extern const VpProfileDataType CT_CW_FI[];           /* Finland Call Waiting Cadence */
extern const VpProfileDataType CT_CG_FI[];           /* Finland Congestion Cadence */
extern const VpProfileDataType CR_FR[];              /* France Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_FR[];           /* France Dial Tone Cadence */
extern const VpProfileDataType CT_RB_FR[];           /* France Ringback Tone  Cadence */
extern const VpProfileDataType CT_BS_FR[];           /* France Busy Tone Cadence */
extern const VpProfileDataType CT_CW_FR[];           /* France Call Waiting Tone Cadence */
extern const VpProfileDataType CR_GB[];              /* UK Ringing Signal Cadence  */
extern const VpProfileDataType CT_DL_GB[];           /* UK Dial Tone Cadence */
extern const VpProfileDataType CT_RB_GB[];           /* UK Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_GB[];           /* UK Busy Tone Cadence */
extern const VpProfileDataType CT_CW_GB[];           /* UK Call Waiting Tone Cadence */
extern const VpProfileDataType CT_CG_GB[];           /* UK Congestion Tone Cadence */
extern const VpProfileDataType CR_GR[];              /* Greece Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_GR[];           /* Greece Dial Tone Cadence */
extern const VpProfileDataType CT_RB_GR[];           /* Greece Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_GR[];           /* Greece Busy Tone Cadence */
extern const VpProfileDataType CT_CW_GR[];           /* Greece Call Waiting Cadence */
extern const VpProfileDataType CT_CG_GR[];           /* Greece Congestion Cadence */
extern const VpProfileDataType CR_HK[];              /* Hong Kong SAR Ringing Signal Cadence  */
extern const VpProfileDataType CT_DL_HK[];           /* Hong Kong SAR Dial Tone Cadence */
extern const VpProfileDataType CT_RB_HK[];           /* Hong Kong SAR Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_HK[];           /* Hong Kong SAR Busy Tone Cadence */
extern const VpProfileDataType CT_CW_HK[];           /* Hong Kong SAR Call Waiting Cadence */
extern const VpProfileDataType CT_CG_HK[];           /* Hong Kong SAR Congestion Cadence */
extern const VpProfileDataType CR_HU[];              /* Hungary Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_HU[];           /* Hungary Dial Tone Cadence */
extern const VpProfileDataType CT_RB_HU[];           /* Hungary Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_HU[];           /* Hungary Busy Tone Cadence */
extern const VpProfileDataType CT_CW_HU[];           /* Hungary Call Waiting Cadence */
extern const VpProfileDataType CT_CG_HU[];           /* Hungary Congestion Cadence */
extern const VpProfileDataType CR_ID[];              /* Indonesia Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_ID[];           /* Indonesia Dial Tone Cadence */
extern const VpProfileDataType CT_RB_ID[];           /* Indonesia Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_ID[];           /* Indonesia Busy Tone Cadence */
extern const VpProfileDataType CT_CW_ID[];           /* Indonesia Call Waiting Cadence */
extern const VpProfileDataType CT_CG_ID[];           /* Indonesia Congestion Cadence */
extern const VpProfileDataType CR_IE[];              /* Ireland Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_IE[];           /* Ireland Dial Tone Cadence */
extern const VpProfileDataType CT_RB_IE[];           /* Ireland Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_IE[];           /* Ireland Busy Tone Cadence */
extern const VpProfileDataType CT_CW_IE[];           /* Ireland Call Waiting Cadence */
extern const VpProfileDataType CR_IL[];              /* Israel Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_IL[];           /* Israel Dial Tone Cadence */
extern const VpProfileDataType CT_RB_IL[];           /* Israel Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_IL[];           /* Israel Busy Tone Cadence */
extern const VpProfileDataType CT_CW_IL[];           /* Israel Call Waiting Cadence */
extern const VpProfileDataType CT_CG_IL[];           /* Israel Congestion Cadence (SIT) */
extern const VpProfileDataType CR_IN[];              /* India Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_IN[];           /* India Dial Tone Cadence */
extern const VpProfileDataType CT_RB_IN[];           /* India Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_IN[];           /* India Busy Tone Cadence */
extern const VpProfileDataType CT_CW_IN[];           /* India Call Waiting Tone Cadence */
extern const VpProfileDataType CT_CG_IN[];           /* India Congestion Tone Cadence */
extern const VpProfileDataType CR_IS[];              /* Iceland Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_IS[];           /* Iceland Dial Tone Cadence */
extern const VpProfileDataType CT_RB_IS[];           /* Iceland Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_IS[];           /* Iceland Busy Tone Cadence */
extern const VpProfileDataType CT_CW_IS[];           /* Iceland Call Waiting Cadence */
extern const VpProfileDataType CT_CG_IS[];           /* Iceland Congestion Cadence */
extern const VpProfileDataType CR_IT[];              /* Italy Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_IT[];           /* Italy Dial Tone Cadence */
extern const VpProfileDataType CT_RB_IT[];           /* Italy Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_IT[];           /* Italy Busy Tone Cadence */
extern const VpProfileDataType CT_CW_IT[];           /* Italy Call Waiting Cadence */
extern const VpProfileDataType CT_CG_IT[];           /* Italy Congestion Cadence */
extern const VpProfileDataType CR_JP1[];             /* Japan Ringing Cadence (NTT TIR and CAR) Before CID */
extern const VpProfileDataType CR_JP2[];             /* Japan Normal Ringing Signal Cadence (NTT IR) - Follow-On Rings */
extern const VpProfileDataType CT_DL_JP[];           /* Japan Dial Tone Cadence */
extern const VpProfileDataType CT_RB_JP[];           /* Japan Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_JP[];           /* Japan Busy Tone Cadence */
extern const VpProfileDataType CT_CW_JP[];           /* Japan Call Waiting Tone Cadence */
extern const VpProfileDataType CR_KR[];              /* S. Korea Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_KR[];           /* S. Korea Dial Tone Cadence */
extern const VpProfileDataType CT_RB_KR[];           /* S. Korea Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_KR[];           /* S. Korea Busy Tone Cadence */
extern const VpProfileDataType CT_CW_KR[];           /* S. Korea Call Waiting Cadence */
extern const VpProfileDataType CT_CG_KR[];           /* S. Korea Congestion Cadence */
extern const VpProfileDataType CR_MX[];              /* Mexico Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_MX[];           /* Mexico Dial Tone Cadence */
extern const VpProfileDataType CT_RB_MX[];           /* Mexico Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_MX[];           /* Mexico Busy Tone Cadence */
extern const VpProfileDataType CT_CG_MX[];           /* Mexico Congestion Cadence */
extern const VpProfileDataType CR_MY[];              /* Malaysia Ringing Signal Cadence  */
extern const VpProfileDataType CT_DL_MY[];           /* Malaysia Dial Tone Cadence */
extern const VpProfileDataType CT_RB_MY[];           /* Malaysia Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_MY[];           /* Malaysia Busy Tone Cadence */
extern const VpProfileDataType CT_CG_MY[];           /* Malaysia Congestion Cadence */
extern const VpProfileDataType CR_NL[];              /* Netherlands Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_NL[];           /* Netherlands Dial Tone Cadence */
extern const VpProfileDataType CT_RB_NL[];           /* Netherlands Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_NL[];           /* Netherlands Busy Tone Cadence */
extern const VpProfileDataType CT_CW_NL[];           /* Netherlands Call Waiting Cadence */
extern const VpProfileDataType CT_CG_NL[];           /* Netherlands Congestion Cadence */
extern const VpProfileDataType CR_NO[];              /* Norway Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_NO[];           /* Norway Dial Tone Cadence */
extern const VpProfileDataType CT_RB_NO[];           /* Norway Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_NO[];           /* Norway Busy Tone Cadence */
extern const VpProfileDataType CT_CW_NO[];           /* Norway Call Waiting Cadence */
extern const VpProfileDataType CT_CG_NO[];           /* Norway Congestion Cadence */
extern const VpProfileDataType CR_NZ[];              /* New Zealand Ringing Signal Cadence  */
extern const VpProfileDataType CT_DL_NZ[];           /* New Zealand Dial Tone Cadence */
extern const VpProfileDataType CT_RB_NZ[];           /* New Zealand Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_NZ[];           /* New Zealand Busy Tone Cadence */
extern const VpProfileDataType CT_CW_NZ[];           /* New Zealand Call Waiting Tone Cadence */
extern const VpProfileDataType CT_CG_NZ[];           /* New Zealand Congestion Tone Cadence */
extern const VpProfileDataType CR_PL[];              /* Poland Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_PL[];           /* Poland Dial Tone Cadence */
extern const VpProfileDataType CT_RB_PL[];           /* Poland Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_PL[];           /* Poland Busy Tone Cadence */
extern const VpProfileDataType CT_CW_PL[];           /* Poland Call Waiting Cadence */
extern const VpProfileDataType CT_CG_PL[];           /* Poland Congestion Cadence */
extern const VpProfileDataType CR_PT[];              /* Portugal Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_PT[];           /* Portugal Dial Tone Cadence */
extern const VpProfileDataType CT_RB_PT[];           /* Portugal Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_PT[];           /* Portugal Busy Tone Cadence */
extern const VpProfileDataType CT_CW_PT[];           /* Portugal Call Waiting Cadence */
extern const VpProfileDataType CT_CG_PT[];           /* Portugal Congestion Cadence */
extern const VpProfileDataType CR_RU[];              /* Russia Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_RU[];           /* Russia Dial Tone Cadence */
extern const VpProfileDataType CT_RB_RU[];           /* Russia Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_RU[];           /* Russia Busy Tone Cadence */
extern const VpProfileDataType CT_CW_RU[];           /* Russia Call Waiting Cadence */
extern const VpProfileDataType CT_CG_RU[];           /* Russia Congestion Cadence */
extern const VpProfileDataType CR_SE[];              /* Sweden Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_SE[];           /* Sweden Dial Tone Cadence */
extern const VpProfileDataType CT_RB_SE[];           /* Sweden Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_SE[];           /* Sweden Busy Tone Cadence */
extern const VpProfileDataType CT_CW_SE[];           /* Sweden Call Waiting Cadence */
extern const VpProfileDataType CT_CG_SE[];           /* Sweden Congestion Cadence */
extern const VpProfileDataType CR_SG[];              /* Singapore Ringing Signal Cadence  */
extern const VpProfileDataType CT_DL_SG[];           /* Singapore Dial Tone Cadence */
extern const VpProfileDataType CT_RB_SG[];           /* Singapore Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_SG[];           /* Singapore Busy Tone Cadence */
extern const VpProfileDataType CT_CW_SG[];           /* Singapore Call Waiting Cadence */
extern const VpProfileDataType CT_CG_SG[];           /* Singapore Congestion Tone Cadence */
extern const VpProfileDataType CR_TH[];              /* Thailand Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_TH[];           /* Thailand Dial Tone Cadence */
extern const VpProfileDataType CT_RB_TH[];           /* Thailand Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_TH[];           /* Thailand Busy Tone Cadence */
extern const VpProfileDataType CT_CW_TH[];           /* Thailand Call Waiting Cadence */
extern const VpProfileDataType CT_CG_TH[];           /* Thailand Congestion Cadence */
extern const VpProfileDataType CR_TK[];              /* Turkey Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_TK[];           /* Turkey Dial Tone Cadence */
extern const VpProfileDataType CT_RB_TK[];           /* Turkey Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_TK[];           /* Turkey Busy Tone Cadence */
extern const VpProfileDataType CT_CW_TK[];           /* Turkey Call Waiting Cadence */
extern const VpProfileDataType CT_CG_TK[];           /* Turkey Congestion Cadence */
extern const VpProfileDataType CR_TW[];              /* Taiwan Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_TW[];           /* Taiwan Dial Tone Cadence */
extern const VpProfileDataType CT_RB_TW[];           /* Taiwan Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_TW[];           /* Taiwan Busy Tone Cadence */
extern const VpProfileDataType CT_CW_TW[];           /* Taiwan Call Waiting Cadence */
extern const VpProfileDataType CT_CG_TW[];           /* Taiwan Congestion Cadence */
extern const VpProfileDataType CR_US[];              /* US Ringing Signal Cadence */
extern const VpProfileDataType CT_DL_US[];           /* USA Dial Tone Cadence */
extern const VpProfileDataType CT_RB_US[];           /* USA Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_US[];           /* USA Busy Tone Cadence */
extern const VpProfileDataType CT_CW_US[];           /* USA Call Waiting Cadence */
extern const VpProfileDataType CT_CG_US[];           /* USA Congestion Cadence */
extern const VpProfileDataType CR_ZA[];              /* South Africa Ringing Signal Cadence  */
extern const VpProfileDataType CT_DL_ZA[];           /* South Africa Dial Tone Cadence */
extern const VpProfileDataType CT_RB_ZA[];           /* South Africa Ringback Tone Cadence */
extern const VpProfileDataType CT_BS_ZA[];           /* South Africa Busy Tone Cadence */
extern const VpProfileDataType CT_CW_ZA[];           /* South Africa Call Waiting Tone Cadence */
extern const VpProfileDataType CT_CG_ZA[];           /* South Africa Congestion Tone Cadence */

/************** Caller_ID **************/
extern const VpProfileDataType CID_TYPE1_AR[];       /* Argentina Caller ID (Type 1 - On-Hook) - ETSI FSK */
extern const VpProfileDataType CID_TYPE1_AT[];       /* Austria Caller ID (Type 1 - On-Hook) - ETSI FSK */
extern const VpProfileDataType CID_TYPE1_AU[];       /* Australia Caller ID (Type 1 - On-Hook) - Telcordia FSK */
extern const VpProfileDataType CID_TYPE2_AU[];       /* Australia Caller ID (Type 2 - Off-Hook) - Telcordia FSK */
extern const VpProfileDataType CID_TYPE1_BE[];       /* Belgium Caller ID (Type 1 - On-Hook) - ETSI DTMF */
extern const VpProfileDataType CID_TYPE1_BR[];       /* Brazil Caller ID (Type 1 - On-Hook) - ETSI DTMF */
extern const VpProfileDataType CID_TYPE1_CA[];       /* Canada Caller ID (Type 1 - On-Hook) - Telcordia FSK */
extern const VpProfileDataType CID_TYPE2_CA[];       /* Canada Caller ID (Type 2 - Off-Hook) - Telcordia FSK */
extern const VpProfileDataType CID_TYPE1_CH[];       /* Switzerland Caller ID (Type 1 - On-Hook) - ETSI FSK */
extern const VpProfileDataType CID_TYPE2_CH[];       /* Switzerland Caller ID (Type 2 - Off-Hook) - ETSI FSK */
extern const VpProfileDataType CID_TYPE1_CL[];       /* Chile Caller ID (Type 1 - On-Hook) - ETSI FSK */
extern const VpProfileDataType CID_TYPE1_CN[];       /* China Caller ID (Type 1 - On-Hook) - Telcordia FSK */
extern const VpProfileDataType CID_TYPE2_CN[];       /* China Caller ID (Type 2 - Off-Hook) - Telcordia FSK */
extern const VpProfileDataType CID_TYPE1_CZ[];       /* Czech Republic Caller ID (Type 1 - On-Hook) - ETSI FSK */
extern const VpProfileDataType CID_TYPE1_DE[];       /* Germany Caller ID (Type 1 - On-Hook) - ETSI FSK */
extern const VpProfileDataType CID_TYPE2_DE[];       /* Germany Caller ID (Type 2 - Off-Hook) - ETSI FSK */
extern const VpProfileDataType CID_TYPE1_DK[];       /* Denmark Caller ID (Type 1 - On-Hook) - ETSI DTMF */
extern const VpProfileDataType CID_TYPE1_EC[];       /* Ecuador Caller ID (Type 1 - On-Hook) - ETSI FSK */
extern const VpProfileDataType CID_TYPE1_ES[];       /* Spain Caller ID (Type 1 - On-Hook) - ETSI FSK */
extern const VpProfileDataType CID_TYPE2_ES[];       /* Spain Caller ID (Type 2 - Off-Hook) - ETSI FSK */
extern const VpProfileDataType CID_TYPE1_FI[];       /* Finland Caller ID (Type 1 - On-Hook) - ETSI DTMF */
extern const VpProfileDataType CID_TYPE1_FR[];       /* France Caller ID (Type 1 - On-Hook) - ETSI FSK */
extern const VpProfileDataType CID_TYPE2_FR[];       /* France Caller ID (Type 2 - Off-Hook) - ETSI FSK */
extern const VpProfileDataType CID_TYPE1_GB[];       /* UK Caller ID (Type 1 - On-Hook) - ETSI FSK */
extern const VpProfileDataType CID_TYPE2_GB[];       /* UK Caller ID (Type 2 - Off-Hook) - ETSI FSK */
extern const VpProfileDataType CID_TYPE1_GR[];       /* Greece Caller ID (Type 1 - On-Hook) - ETSI FSK */
extern const VpProfileDataType CID_TYPE1_HK[];       /* Hong Kong SAR Caller ID (Type 1 - On-Hook) - Telcordia FSK */
extern const VpProfileDataType CID_TYPE2_HK[];       /* Hong Kong SAR Caller ID (Type 2 - Off-Hook) - Telcordia FSK */
extern const VpProfileDataType CID_TYPE1_IE[];       /* Ireland Caller ID (Type 1 - On-Hook) - ETSI FSK */
extern const VpProfileDataType CID_TYPE2_IE[];       /* Ireland Caller ID (Type 2 - Off-Hook) - ETSI FSK */
extern const VpProfileDataType CID_TYPE1_IL[];       /* Israel Caller ID (Type 1 - On-Hook) - Telcordia FSK */
extern const VpProfileDataType CID_TYPE1_IN[];       /* India Caller ID (Type 1 - On-Hook) - ETSI DTMF */
extern const VpProfileDataType CID_TYPE1_IS[];       /* Iceland Caller ID (Type 1 - On-Hook) - ETSI DTMF */
extern const VpProfileDataType CID_TYPE1_IT[];       /* Italy Caller ID (Type 1 - On-Hook) - ETSI FSK */
extern const VpProfileDataType CID_TYPE1_JP[];       /* Japan Caller ID (Type 1 - On-Hook) - NTT FSK */
extern const VpProfileDataType CID_TYPE2_JP[];       /* Japan Caller ID (Type 2 - Off-Hook / Phone Catch) - NTT FSK */
extern const VpProfileDataType CID_TYPE1_KR[];       /* S. Korea Caller ID (Type 1 - On-Hook) - Telcordia FSK */
extern const VpProfileDataType CID_TYPE2_KR[];       /* S. Korea Caller ID (Type 2 - Off-Hook) - Telcordia FSK */
extern const VpProfileDataType CID_TYPE1_MX[];       /* Mexico Caller ID (Type 1 - On-Hook) - Telcordia FSK */
extern const VpProfileDataType CID_TYPE1_NL[];       /* Netherlands Caller ID (Type 1 - On-Hook) - ETSI DTMF */
extern const VpProfileDataType CID_TYPE1_NO[];       /* Norway Caller ID (Type 1 - On-Hook) - ETSI FSK */
extern const VpProfileDataType CID_TYPE2_NO[];       /* Norway Caller ID (Type 2 - Hook) - ETSI FSK */
extern const VpProfileDataType CID_TYPE1_NZ[];       /* New Zealand Caller ID (Type 1 - On-Hook) - Telcordia FSK */
extern const VpProfileDataType CID_TYPE2_NZ[];       /* New Zealand Caller ID (Type 2 - Off-Hook) - Telcordia FSK */
extern const VpProfileDataType CID_TYPE1_PT[];       /* Portugal Caller ID (Type 1 - On-Hook) - ETSI FSK */
extern const VpProfileDataType CID_TYPE2_PT[];       /* Portugal Caller ID (Type 2 - Off-Hook) - ETSI FSK */
extern const VpProfileDataType CID_TYPE1_SE[];       /* Sweden Caller ID (Type 1 - On-Hook) - ETSI DTMF */
extern const VpProfileDataType CID_TYPE1_SG[];       /* Singapore Caller ID (Type 1 - On-Hook) - Telcordia FSK */
extern const VpProfileDataType CID_TYPE2_SG[];       /* Singapore Caller ID (Type 2 - Off-Hook) - Telcordia FSK */
extern const VpProfileDataType CID_TYPE1_TK[];       /* Turkey Caller ID (Type 1 - On-Hook) - ETSI FSK */
extern const VpProfileDataType CID_TYPE1_TW[];       /* Taiwan Caller ID (Type 1 - On-Hook) - Telcordia FSK */
extern const VpProfileDataType CID_TYPE1_US[];       /* USA Caller ID (Type 1 - On-Hook) - Telcordia FSK */
extern const VpProfileDataType CID_TYPE2_US[];       /* USA Caller ID (Type 2 - Off-Hook) - Telcordia FSK */
extern const VpProfileDataType CID_TYPE1_ZA[];       /* South Africa Caller ID (Type 1 - On-Hook) - ETSI FSK */

/************** Metering_Profile **************/
extern const VpProfileDataType METER_12KHZ_RF14[];   /* Metering, 12kHz 0.5Vrms */
extern const VpProfileDataType METER_16KHZ_RF14[];   /* Metering, 16kHz 0.5Vrms */

#endif /* ZLR963222H_135VIB_PROFILE_H */

