/*
 * ZLR963222H_135VIB_Profile.c --
 *
 * This file contains profile data in byte format
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

#include "ZLR963222H_135VIB_Profile.h"


/************** Device_Parameters **************/

/* Device Configuration Data - Le9632 Tracker 135V IB */
const VpProfileDataType ZLR963222H_IB135V_DEVICE[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0xFF, 0x00,       /* Device Profile for ZL88702 device */
   0x28,                   /* Profile length = 40 + 4 = 44 bytes */
   0x04,                   /* Profile format version = 4 */
   0x14,                   /* MPI section length = 20 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0x46, 0x82,             /* PCLK = 8.192 MHz; INTM = Open Drain */
   0x44, 0x00,             /* PCM Clock Slot = 0 TX / 0 RX; XE = Neg. */
   0x5E, 0x34, 0x0A,       /* Device Mode Register */
   0xF6, 0x96, 0x00, 0xB0, /* Switching Regulator Timing Params */
         0x52, 0xFC, 0x52,
   0xE4, 0x04, 0xB2, 0x0A, /* Switching Regulator Params */
   0xE6, 0x00,             /* Switching Regulator Control */
/* Formatted Parameters Section -------------------------------------------- */
   0x02,                   /* IO21 Mode: Analog Voltage Sense */
                           /* IO22 Mode: Digital */
   0x04,                   /* Dial pulse correction: 0 ms */
                           /* Switcher Configuration =
                              Inverting Boost (12 V in, 135 V out) */
   0x00, 0x00,             /* Over-current event threshold = Off */
   0x01,                   /* Leading edge blanking window = 81 ns */
   0x65, 0x00, 0x64, 0x52, /* FreeRun Timing Parameters */
         0x64, 0x52,
   0xFF,
   0x97, 0xFF,             /* Low-Power Timing Parameters */
   0x87, 0x87,             /* Switcher Limit Voltages 135 V, 135 V */
   0x03,                   /* Charge pump enabled */
                           /* Charge Pump Overload Protection: Switcher Cycle Skip */
   0x3C                    /* Switcher Input 12 V */
};

/************** DC_Coefficients **************/

/* DC FXS Default - Use for all countries unless country file exists - 25mA Current Feed */
const VpProfileDataType DC_FXS_ZL880_FB150V_DEF[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x01, 0x00,       /* DC Profile for ZL88702 device */
   0x0C,                   /* Profile length = 12 + 4 = 16 bytes */
   0x02,                   /* Profile format version = 2 */
   0x03,                   /* MPI section length = 3 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC6, 0x92, 0x27,       /* DC Feed Parameters: ir_overhead = 100 ohm; */
                           /* VOC = 48 V; LI = 50 ohm; VAS = 8.78 V; ILA = 25 mA */
                           /* Maximum Voice Amplitude = 2.93 V */
/* Formatted Parameters Section -------------------------------------------- */
   0x1C,                   /* Switch Hook Threshold = 11 mA */
                           /* Ground-Key Threshold = 18 mA */
   0x84,                   /* Switch Hook Debounce = 8 ms */
                           /* Ground-Key Debounce = 16 ms */
   0x58,                   /* Low-Power Switch Hook Hysteresis = 2 V */
                           /* Ground-Key Hysteresis = 6 mA */
                           /* Switch Hook Hysteresis = 2 mA */
   0x80,                   /* Low-Power Switch Hook Threshold = 22 V */
   0x02,                   /* Floor Voltage = -15 V */
   0x00,                   /* R_OSP = 0 ohms */
   0x07                    /* R_ISP = 7 ohms */
};

/************** AC_Coefficients **************/

/* AC FXS RF14 600R Normal Coefficients (Default)  */
const VpProfileDataType AC_FXS_RF14_600R_DEF[] =
{
  /* AC Profile */
 0xA4, 0x00, 0xF4, 0x4C, 0x01, 0x49, 0xCA, 0xF5, 0x98, 0xAA, 0x7B, 0xAB,
 0x2C, 0xA3, 0x25, 0xA5, 0x24, 0xB2, 0x3D, 0x9A, 0x2A, 0xAA, 0xA6, 0x9F,
 0x01, 0x8A, 0x1D, 0x01, 0xA3, 0xA0, 0x2E, 0xB2, 0xB2, 0xBA, 0xAC, 0xA2,
 0xA6, 0xCB, 0x3B, 0x45, 0x88, 0x2A, 0x20, 0x3C, 0xBC, 0x4E, 0xA6, 0x2B,
 0xA5, 0x2B, 0x3E, 0xBA, 0x8F, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0x2A, 0x42, 0xA1, 0xCB, 0x1B, 0xA3, 0xA8, 0xFB, 0x87, 0xAA,
 0xFB, 0x9F, 0xA9, 0xF0, 0x96, 0x2E, 0x01, 0x00
};

/* AC FXS RF14 ETSI ES201 970 270R+750R//150nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_ETSI[] =
{
  /* AC Profile */
 0xD5, 0x00, 0xEC, 0x4C, 0x01, 0x49, 0xCA, 0xE8, 0x98, 0x4A, 0xA1, 0x34,
 0xA2, 0xA2, 0xAC, 0x2C, 0xBB, 0x2B, 0xA4, 0x9A, 0x98, 0x92, 0x97, 0x9F,
 0x01, 0x8A, 0x9C, 0xD0, 0x2B, 0x20, 0x8F, 0x39, 0x2A, 0xBB, 0xA4, 0x24,
 0xB2, 0xBC, 0x4B, 0x26, 0x88, 0xA3, 0xC0, 0xC2, 0x27, 0xBA, 0x3E, 0xAC,
 0xAF, 0xB4, 0x26, 0x24, 0x3F, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0xC3, 0x5F, 0x32, 0x98, 0x03, 0xA1, 0xAA, 0x23, 0xB3, 0x2C,
 0x4B, 0x55, 0x25, 0x60, 0x96, 0xF2, 0xC1, 0x00
};

/* AC FXS RF14 Telcordia GR-57 900R+2.16uF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_GR57[] =
{
  /* AC Profile */
 0x01, 0x00, 0x90, 0x4C, 0x01, 0x49, 0xCA, 0x0E, 0x98, 0x32, 0x2B, 0x24,
 0x7C, 0x4A, 0x24, 0x55, 0xA3, 0x3A, 0xBC, 0x9A, 0xBA, 0xB3, 0xF4, 0x2D,
 0x01, 0x8A, 0xEB, 0xC0, 0x26, 0x21, 0xB2, 0x35, 0xB3, 0xA6, 0x2A, 0xAD,
 0xD4, 0xA6, 0x52, 0x4F, 0x88, 0xA3, 0xD0, 0x23, 0xAA, 0xAF, 0x62, 0xEA,
 0xBA, 0x2A, 0xB3, 0x4A, 0x3D, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0xBD, 0x52, 0xA1, 0x62, 0x1B, 0x22, 0x3A, 0x4B, 0xB5, 0x2A,
 0x65, 0xA7, 0xB6, 0x70, 0x96, 0x23, 0xC0, 0x00
};

/* AC FXS RF14 Austria 220R+820R//115nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_AT[] =
{
  /* AC Profile */
 0x6D, 0x00, 0xA7, 0x4C, 0x01, 0x49, 0xCA, 0xEA, 0x98, 0xFA, 0xE1, 0x23,
 0xA2, 0xEA, 0xCB, 0xB3, 0xAA, 0xA3, 0xB3, 0x9A, 0x98, 0x92, 0x97, 0x9F,
 0x01, 0x8A, 0x2E, 0x01, 0xB3, 0x20, 0xAB, 0xA8, 0xCC, 0x24, 0x22, 0x97,
 0xAF, 0x8F, 0xD3, 0xBF, 0x88, 0x3B, 0xB0, 0x33, 0x46, 0x2A, 0x2E, 0x33,
 0x4F, 0xAE, 0xA5, 0xB3, 0xBE, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0x2A, 0x42, 0x22, 0x98, 0x02, 0x42, 0xA2, 0x43, 0xA6, 0xD8,
 0xFA, 0x8F, 0x29, 0xF0, 0x96, 0x2E, 0x01, 0x00
};

/* AC FXS RF14 Australia 220R+820R//120nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_AU[] =
{
  /* AC Profile */
 0xBF, 0x00, 0xAF, 0x4C, 0x01, 0x49, 0xCA, 0xE8, 0x98, 0xBA, 0x41, 0x54,
 0xA2, 0x2A, 0x5B, 0xA4, 0xAA, 0x3A, 0xB3, 0x9A, 0x87, 0xA2, 0x97, 0x9F,
 0x01, 0x8A, 0x2E, 0x01, 0x23, 0x20, 0x3E, 0xA8, 0x2A, 0x57, 0xCB, 0x34,
 0xBA, 0xAC, 0xBB, 0xA6, 0x88, 0xAA, 0xB0, 0x49, 0xF6, 0x3B, 0x2E, 0x49,
 0xFF, 0xBC, 0xA5, 0x3A, 0xBE, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0x34, 0x5C, 0x32, 0x25, 0x1A, 0xA1, 0x3B, 0x23, 0xB3, 0x5B,
 0x4D, 0xB5, 0xDC, 0x60, 0x96, 0x3B, 0xA1, 0x00
};

/* AC FXS RF14 Belgium 270R+750R//150nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_BE[] =
{
  /* AC Profile */
 0xD5, 0x00, 0xEC, 0x4C, 0x01, 0x49, 0xCA, 0xE8, 0x98, 0x4A, 0xA1, 0x34,
 0xA2, 0xA2, 0xAC, 0x2C, 0xBB, 0x2B, 0xA4, 0x9A, 0x98, 0x92, 0x97, 0x9F,
 0x01, 0x8A, 0x9C, 0xD0, 0x2B, 0x20, 0x8F, 0x39, 0x2A, 0xBB, 0xA4, 0x24,
 0xB2, 0xBC, 0x4B, 0x26, 0x88, 0xA3, 0xC0, 0xC2, 0x27, 0xBA, 0x3E, 0xAC,
 0xAF, 0xB4, 0x26, 0x24, 0x3F, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0xC3, 0x5F, 0x32, 0x98, 0x03, 0xA1, 0xAA, 0x23, 0xB3, 0x2C,
 0x4B, 0x55, 0x25, 0x60, 0x96, 0xF2, 0xC1, 0x00
};

/* AC FXS RF14 Bulgaria 220R+820R//115nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_BG[] =
{
  /* AC Profile */
 0x6D, 0x00, 0xA7, 0x4C, 0x01, 0x49, 0xCA, 0xEA, 0x98, 0xFA, 0xE1, 0x23,
 0xA2, 0xEA, 0xCB, 0xB3, 0xAA, 0xA3, 0xB3, 0x9A, 0x98, 0x92, 0x97, 0x9F,
 0x01, 0x8A, 0x2E, 0x01, 0xB3, 0x20, 0xAB, 0xA8, 0xCC, 0x24, 0x22, 0x97,
 0xAF, 0x8F, 0xD3, 0xBF, 0x88, 0x3B, 0xB0, 0x33, 0x46, 0x2A, 0x2E, 0x33,
 0x4F, 0xAE, 0xA5, 0xB3, 0xBE, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0x2A, 0x42, 0x22, 0x98, 0x02, 0x42, 0xA2, 0x43, 0xA6, 0xD8,
 0xFA, 0x8F, 0x29, 0xF0, 0x96, 0x2E, 0x01, 0x00
};

/* AC FXS RF14 Brazil 900R Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_BR[] =
{
  /* AC Profile */
 0xE4, 0x00, 0xF7, 0x4C, 0x01, 0x49, 0xCA, 0x0B, 0x98, 0x34, 0x3B, 0xB2,
 0x2C, 0x9F, 0xA4, 0xD2, 0xA3, 0x22, 0xAC, 0x9A, 0xA4, 0xAA, 0x67, 0x9F,
 0x01, 0x8A, 0xB3, 0xD0, 0xA6, 0x21, 0xAA, 0x34, 0x2B, 0x2F, 0x4A, 0xA6,
 0xAB, 0x2F, 0x4A, 0x37, 0x88, 0x2A, 0xC0, 0x23, 0xAC, 0x2A, 0x26, 0x32,
 0xA5, 0x3C, 0xAD, 0xA4, 0xA7, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0xAB, 0x49, 0xF1, 0x2A, 0x0A, 0x43, 0xC9, 0xF3, 0x27, 0xBC,
 0xE2, 0xAF, 0xAD, 0xF0, 0x96, 0x2E, 0x01, 0x00
};

/* AC FXS RF14 Switzerland 270R+750R//150nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_CH[] =
{
  /* AC Profile */
 0xD5, 0x00, 0xEC, 0x4C, 0x01, 0x49, 0xCA, 0xE8, 0x98, 0x4A, 0xA1, 0x34,
 0xA2, 0xA2, 0xAC, 0x2C, 0xBB, 0x2B, 0xA4, 0x9A, 0x98, 0x92, 0x97, 0x9F,
 0x01, 0x8A, 0x9C, 0xD0, 0x2B, 0x20, 0x8F, 0x39, 0x2A, 0xBB, 0xA4, 0x24,
 0xB2, 0xBC, 0x4B, 0x26, 0x88, 0xA3, 0xC0, 0xC2, 0x27, 0xBA, 0x3E, 0xAC,
 0xAF, 0xB4, 0x26, 0x24, 0x3F, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0xC3, 0x5F, 0x32, 0x98, 0x03, 0xA1, 0xAA, 0x23, 0xB3, 0x2C,
 0x4B, 0x55, 0x25, 0x60, 0x96, 0xF2, 0xC1, 0x00
};

/* AC FXS RF14 China 200R+680R//100nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_CN[] =
{
  /* AC Profile */
 0xB9, 0x00, 0x1F, 0x4C, 0x01, 0x49, 0xCA, 0xE9, 0x98, 0x5C, 0xB1, 0xAA,
 0x23, 0x43, 0xEB, 0x32, 0xAA, 0x23, 0xB3, 0x9A, 0x98, 0x94, 0x97, 0x9F,
 0x01, 0x8A, 0x2D, 0x01, 0x22, 0x30, 0x6A, 0x59, 0x2A, 0xCB, 0xB4, 0xB2,
 0xBF, 0x3B, 0xA3, 0x25, 0x88, 0x43, 0xC0, 0x3A, 0x45, 0x2B, 0xBD, 0x4C,
 0x2E, 0x4C, 0x25, 0x42, 0xAD, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0xA4, 0x5A, 0x32, 0xB5, 0x12, 0xA1, 0xA2, 0x3A, 0xA4, 0xB5,
 0x64, 0x97, 0xAA, 0xF0, 0x96, 0x2E, 0x01, 0x00
};

/* AC FXS RF14 Germany 220R+820R//115nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_DE[] =
{
  /* AC Profile */
 0xEB, 0x00, 0xDC, 0x4C, 0x01, 0x49, 0xCA, 0xEA, 0x98, 0xA2, 0xE1, 0x24,
 0xA2, 0x2A, 0xCB, 0xB3, 0xAA, 0xD3, 0xB3, 0x9A, 0x98, 0x92, 0x97, 0x9F,
 0x01, 0x8A, 0x2D, 0x01, 0x24, 0x20, 0x2C, 0xA8, 0xAB, 0x24, 0x22, 0x97,
 0xAF, 0x8F, 0x23, 0xBF, 0x88, 0x3B, 0xB0, 0x33, 0x46, 0x2A, 0x2E, 0x33,
 0x4F, 0xAE, 0xA5, 0xB3, 0xBE, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0x53, 0x54, 0x32, 0xC4, 0x1A, 0xA1, 0x2B, 0x2A, 0xA3, 0xAA,
 0x4A, 0xA5, 0xBA, 0x60, 0x96, 0xD2, 0xA1, 0x00
};

/* AC FXS RF14 Denmark 270R+750R//150nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_DK[] =
{
  /* AC Profile */
 0x7D, 0x00, 0x9A, 0x4C, 0x01, 0x49, 0xCA, 0xE6, 0x98, 0x22, 0xA1, 0x52,
 0xD2, 0xE7, 0x24, 0xF4, 0xCD, 0xAA, 0x97, 0x9A, 0xCA, 0xB3, 0xA5, 0x6E,
 0x01, 0x8A, 0x32, 0xD0, 0xAB, 0x20, 0xAA, 0x49, 0x29, 0x7A, 0x32, 0xD6,
 0xA2, 0xED, 0xC5, 0x97, 0x88, 0xBA, 0xC0, 0x22, 0xA7, 0x2C, 0xFE, 0x42,
 0xDE, 0x22, 0xB6, 0xB3, 0x9F, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0x23, 0x53, 0x32, 0x2C, 0x1B, 0x22, 0xCA, 0x23, 0x63, 0x2A,
 0x33, 0xD4, 0x2A, 0x40, 0x96, 0xA2, 0xA0, 0x00
};

/* AC FXS RF14 Spain 270R+750R//150nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_ES[] =
{
  /* AC Profile */
 0xD5, 0x00, 0xEC, 0x4C, 0x01, 0x49, 0xCA, 0xE8, 0x98, 0x4A, 0xA1, 0x34,
 0xA2, 0xA2, 0xAC, 0x2C, 0xBB, 0x2B, 0xA4, 0x9A, 0x98, 0x92, 0x97, 0x9F,
 0x01, 0x8A, 0x9C, 0xD0, 0x2B, 0x20, 0x8F, 0x39, 0x2A, 0xBB, 0xA4, 0x24,
 0xB2, 0xBC, 0x4B, 0x26, 0x88, 0xA3, 0xC0, 0xC2, 0x27, 0xBA, 0x3E, 0xAC,
 0xAF, 0xB4, 0x26, 0x24, 0x3F, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0xC3, 0x5F, 0x32, 0x98, 0x03, 0xA1, 0xAA, 0x23, 0xB3, 0x2C,
 0x4B, 0x55, 0x25, 0x60, 0x96, 0xF2, 0xC1, 0x00
};

/* AC FXS RF14 Finland 270R+910R//120nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_FI[] =
{
  /* AC Profile */
 0xFF, 0x00, 0x8D, 0x4C, 0x01, 0x49, 0xCA, 0xEF, 0x98, 0x2A, 0xB1, 0x23,
 0x33, 0xB2, 0xAB, 0x44, 0xEB, 0xA4, 0x44, 0x9A, 0x98, 0x92, 0x97, 0x9F,
 0x01, 0x8A, 0x2D, 0x01, 0x24, 0x20, 0x25, 0xB8, 0x2D, 0x42, 0xDB, 0xBA,
 0xE3, 0x43, 0xF2, 0xAC, 0x88, 0xC4, 0xB0, 0xA2, 0xA7, 0x42, 0x3E, 0xA2,
 0x9F, 0x32, 0x26, 0xBE, 0x2F, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0xA2, 0x5C, 0x22, 0xA2, 0x13, 0xC1, 0xB3, 0x22, 0x23, 0x7A,
 0x3D, 0xB4, 0xD4, 0x50, 0x96, 0x3C, 0x41, 0x00
};

/* AC FXS RF14 France 270R+750R//150nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_FR[] =
{
  /* AC Profile */
 0xFC, 0x00, 0xA4, 0x4C, 0x01, 0x49, 0xCA, 0xE7, 0x98, 0xBA, 0x31, 0xA5,
 0x62, 0x35, 0xAB, 0xAA, 0xAA, 0xAC, 0xA3, 0x9A, 0xAA, 0xA2, 0xC7, 0x6F,
 0x01, 0x8A, 0x2D, 0x01, 0xBB, 0x10, 0xC2, 0x38, 0x2A, 0xA1, 0xC2, 0xA9,
 0x23, 0xA2, 0x22, 0x2D, 0x88, 0xA3, 0xB0, 0x53, 0x55, 0x4A, 0xAD, 0xB2,
 0xAD, 0x5A, 0x25, 0x72, 0x5E, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0x2A, 0x43, 0x22, 0x9F, 0x1A, 0x22, 0x4B, 0x33, 0xD5, 0x2C,
 0x7A, 0x87, 0xA8, 0xF0, 0x96, 0x2E, 0x01, 0x00
};

/* AC FXS RF14 UK 370R+620R//310nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_GB[] =
{
  /* AC Profile */
 0x24, 0x00, 0x0A, 0x4C, 0x01, 0x49, 0xCA, 0xE6, 0x98, 0x22, 0xA1, 0x52,
 0xD2, 0xE7, 0x24, 0xF4, 0xCD, 0xAA, 0x97, 0x9A, 0x2B, 0xA2, 0x27, 0x6E,
 0x01, 0x8A, 0x32, 0xD0, 0xCB, 0x20, 0xBB, 0x59, 0x5A, 0x4A, 0xAA, 0xB5,
 0x2C, 0x2D, 0x42, 0xC7, 0x88, 0xAA, 0xC0, 0xA3, 0xA7, 0xBA, 0xDE, 0x5C,
 0xCE, 0xAA, 0xA6, 0xA2, 0x9F, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0x43, 0x5B, 0x32, 0x2C, 0x1D, 0x22, 0xAA, 0x2B, 0x33, 0xBA,
 0x3C, 0x34, 0xBA, 0x40, 0x96, 0xA2, 0xA0, 0x00
};

/* AC FXS RF14 Greece 270R+750R//150nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_GR[] =
{
  /* AC Profile */
 0x63, 0x00, 0x7D, 0x4C, 0x01, 0x49, 0xCA, 0x03, 0x98, 0x22, 0xA6, 0x2C,
 0x3E, 0x2B, 0x87, 0x44, 0x36, 0x2A, 0x9F, 0x9A, 0x22, 0xBA, 0xA7, 0x9F,
 0x01, 0x8A, 0x7D, 0xD0, 0xAA, 0xA0, 0x22, 0xAA, 0x98, 0xB4, 0x23, 0xBE,
 0xAB, 0xBF, 0xAA, 0x97, 0x88, 0x3C, 0xC0, 0x3A, 0xBD, 0xED, 0x8F, 0xEB,
 0xA6, 0x3A, 0x8F, 0x2B, 0x6F, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0xD2, 0x52, 0x22, 0xA2, 0x13, 0xA1, 0x97, 0x32, 0x25, 0xCA,
 0x6B, 0x87, 0x2B, 0xF0, 0x96, 0x2E, 0x01, 0x00
};

/* AC FXS RF14 Hungary 270R+750R//150nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_HU[] =
{
  /* AC Profile */
 0xD5, 0x00, 0xEC, 0x4C, 0x01, 0x49, 0xCA, 0xE8, 0x98, 0x4A, 0xA1, 0x34,
 0xA2, 0xA2, 0xAC, 0x2C, 0xBB, 0x2B, 0xA4, 0x9A, 0x98, 0x92, 0x97, 0x9F,
 0x01, 0x8A, 0x9C, 0xD0, 0x2B, 0x20, 0x8F, 0x39, 0x2A, 0xBB, 0xA4, 0x24,
 0xB2, 0xBC, 0x4B, 0x26, 0x88, 0xA3, 0xC0, 0xC2, 0x27, 0xBA, 0x3E, 0xAC,
 0xAF, 0xB4, 0x26, 0x24, 0x3F, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0xC3, 0x5F, 0x32, 0x98, 0x03, 0xA1, 0xAA, 0x23, 0xB3, 0x2C,
 0x4B, 0x55, 0x25, 0x60, 0x96, 0xF2, 0xC1, 0x00
};

/* AC FXS RF14 Ireland 270R+750R//150nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_IE[] =
{
  /* AC Profile */
 0xD5, 0x00, 0xEC, 0x4C, 0x01, 0x49, 0xCA, 0xE8, 0x98, 0x4A, 0xA1, 0x34,
 0xA2, 0xA2, 0xAC, 0x2C, 0xBB, 0x2B, 0xA4, 0x9A, 0x98, 0x92, 0x97, 0x9F,
 0x01, 0x8A, 0x9C, 0xD0, 0x2B, 0x20, 0x8F, 0x39, 0x2A, 0xBB, 0xA4, 0x24,
 0xB2, 0xBC, 0x4B, 0x26, 0x88, 0xA3, 0xC0, 0xC2, 0x27, 0xBA, 0x3E, 0xAC,
 0xAF, 0xB4, 0x26, 0x24, 0x3F, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0xC3, 0x5F, 0x32, 0x98, 0x03, 0xA1, 0xAA, 0x23, 0xB3, 0x2C,
 0x4B, 0x55, 0x25, 0x60, 0x96, 0xF2, 0xC1, 0x00
};

/* AC FXS RF14 Israel 270R+750R//150nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_IL[] =
{
  /* AC Profile */
 0xD5, 0x00, 0xEC, 0x4C, 0x01, 0x49, 0xCA, 0xE8, 0x98, 0x4A, 0xA1, 0x34,
 0xA2, 0xA2, 0xAC, 0x2C, 0xBB, 0x2B, 0xA4, 0x9A, 0x98, 0x92, 0x97, 0x9F,
 0x01, 0x8A, 0x9C, 0xD0, 0x2B, 0x20, 0x8F, 0x39, 0x2A, 0xBB, 0xA4, 0x24,
 0xB2, 0xBC, 0x4B, 0x26, 0x88, 0xA3, 0xC0, 0xC2, 0x27, 0xBA, 0x3E, 0xAC,
 0xAF, 0xB4, 0x26, 0x24, 0x3F, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0xC3, 0x5F, 0x32, 0x98, 0x03, 0xA1, 0xAA, 0x23, 0xB3, 0x2C,
 0x4B, 0x55, 0x25, 0x60, 0x96, 0xF2, 0xC1, 0x00
};

/* AC FXS RF14 Iceland 270R+750R//150nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_IS[] =
{
  /* AC Profile */
 0xD5, 0x00, 0xEC, 0x4C, 0x01, 0x49, 0xCA, 0xE8, 0x98, 0x4A, 0xA1, 0x34,
 0xA2, 0xA2, 0xAC, 0x2C, 0xBB, 0x2B, 0xA4, 0x9A, 0x98, 0x92, 0x97, 0x9F,
 0x01, 0x8A, 0x9C, 0xD0, 0x2B, 0x20, 0x8F, 0x39, 0x2A, 0xBB, 0xA4, 0x24,
 0xB2, 0xBC, 0x4B, 0x26, 0x88, 0xA3, 0xC0, 0xC2, 0x27, 0xBA, 0x3E, 0xAC,
 0xAF, 0xB4, 0x26, 0x24, 0x3F, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0xC3, 0x5F, 0x32, 0x98, 0x03, 0xA1, 0xAA, 0x23, 0xB3, 0x2C,
 0x4B, 0x55, 0x25, 0x60, 0x96, 0xF2, 0xC1, 0x00
};

/* AC FXS RF14 Italy 270R+750R//150nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_IT[] =
{
  /* AC Profile */
 0xEE, 0x00, 0xBD, 0x4C, 0x01, 0x49, 0xCA, 0xF3, 0x98, 0x22, 0xD2, 0xBA,
 0x34, 0x22, 0x3C, 0x53, 0xAB, 0xAA, 0xB4, 0x9A, 0x98, 0x1B, 0x97, 0x9F,
 0x01, 0x8A, 0x9C, 0xD0, 0xBC, 0xD0, 0x2A, 0x2A, 0x3C, 0x4C, 0x4E, 0x23,
 0xD2, 0xBB, 0xDB, 0x55, 0x88, 0xAA, 0xC0, 0x2C, 0xB5, 0x23, 0xAD, 0x4A,
 0x5E, 0xAD, 0xB5, 0x2B, 0x2E, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0xA2, 0x52, 0xA1, 0xA3, 0x1A, 0x32, 0xAA, 0x4A, 0xAF, 0x3B,
 0xF4, 0x9F, 0x3A, 0xF0, 0x96, 0x2E, 0x01, 0x00
};

/* AC FXS RF14 Japan 600R+1uF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_JP[] =
{
  /* AC Profile */
 0xDA, 0x00, 0x5B, 0x4C, 0x01, 0x49, 0xCA, 0xF7, 0x98, 0xCA, 0x4D, 0xCD,
 0x2C, 0x45, 0x36, 0x23, 0x44, 0x3A, 0xCC, 0x9A, 0xC2, 0x61, 0xA3, 0xA2,
 0xC0, 0x8A, 0x7B, 0xA0, 0x5A, 0xA0, 0x23, 0xBB, 0x26, 0x23, 0x3A, 0x4B,
 0x5A, 0x24, 0xAA, 0xBD, 0x88, 0xAA, 0x20, 0x4A, 0xCA, 0x34, 0xA1, 0x34,
 0xA9, 0x3B, 0xB2, 0xAA, 0xAB, 0x82, 0x29, 0x71, 0x80, 0x22, 0xF0, 0x50,
 0x00, 0x86, 0xF2, 0x62, 0xA1, 0x36, 0x12, 0xA2, 0x24, 0x4A, 0x34, 0x5F,
 0x4C, 0x64, 0xB3, 0x40, 0x96, 0x2A, 0xC0, 0x00
};

/* AC FXS RF14 Netherlands 270R+750R//150nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_NL[] =
{
  /* AC Profile */
 0x7D, 0x00, 0x38, 0x4C, 0x01, 0x49, 0xCA, 0xF5, 0x98, 0xAA, 0x7B, 0xAB,
 0x2C, 0xA3, 0x25, 0xA5, 0x24, 0xB2, 0x3D, 0x9A, 0x22, 0xBA, 0xA6, 0x9F,
 0x01, 0x8A, 0x34, 0xD0, 0xCD, 0xA0, 0xAB, 0xD3, 0x22, 0xAB, 0x4A, 0xA3,
 0xA2, 0x2C, 0xAB, 0x45, 0x88, 0x2A, 0x20, 0x6C, 0xBC, 0xCE, 0xA6, 0x2B,
 0xA5, 0xAA, 0x3E, 0xBA, 0x8F, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0x62, 0x52, 0x22, 0xCD, 0x12, 0x23, 0x44, 0x7A, 0x87, 0x3B,
 0xFC, 0xBF, 0xBA, 0xF0, 0x96, 0x2E, 0x01, 0x00
};

/* AC FXS RF14 Norway 270R+750R//150nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_NO[] =
{
  /* AC Profile */
 0x04, 0x00, 0xA8, 0x4C, 0x01, 0x49, 0xCA, 0xE4, 0x98, 0xAA, 0x41, 0x22,
 0xA1, 0xAA, 0x3B, 0x23, 0x2A, 0xD4, 0x23, 0x9A, 0x98, 0x92, 0x97, 0x9F,
 0x01, 0x8A, 0x2D, 0x01, 0xEA, 0x10, 0x22, 0xB8, 0x22, 0x3B, 0x23, 0xA1,
 0xAA, 0xBA, 0x32, 0xC4, 0x88, 0x25, 0xB0, 0x3B, 0x24, 0x2C, 0x2D, 0xA3,
 0xAC, 0xF4, 0xC4, 0xAB, 0xAD, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0xBB, 0x52, 0xE2, 0xBD, 0x1B, 0xA1, 0x3B, 0x2A, 0xA3, 0x3A,
 0x4A, 0x26, 0x29, 0xF0, 0x96, 0x2E, 0x01, 0x00
};

/* AC FXS RF14 New Zealand 370R+620R//310nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_NZ[] =
{
  /* AC Profile */
 0xFF, 0x00, 0x0D, 0x4C, 0x01, 0x49, 0xCA, 0xFA, 0x98, 0xAB, 0x2A, 0xAB,
 0x22, 0x22, 0xA1, 0xD2, 0xB4, 0x4B, 0xBB, 0x9A, 0x2A, 0xB3, 0x46, 0x1F,
 0x01, 0x8A, 0x1D, 0x01, 0xBA, 0xD0, 0xA3, 0x4B, 0xBC, 0x3A, 0x3A, 0xAC,
 0x2C, 0xB6, 0x2B, 0xAE, 0x88, 0x4C, 0xC0, 0xCB, 0xA3, 0xAA, 0x2B, 0x2A,
 0xA3, 0x3A, 0x2C, 0x98, 0x15, 0x82, 0x29, 0xF1, 0x80, 0xCA, 0xF0, 0x50,
 0x00, 0x86, 0xB2, 0x5C, 0x22, 0x5C, 0x12, 0x42, 0xBA, 0x2B, 0xD3, 0x2A,
 0x36, 0xC4, 0x42, 0x50, 0x96, 0xD2, 0xA0, 0x00
};

/* AC FXS RF14 Portugal 270R+750R//150nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_PT[] =
{
  /* AC Profile */
 0xD5, 0x00, 0xEC, 0x4C, 0x01, 0x49, 0xCA, 0xE8, 0x98, 0x4A, 0xA1, 0x34,
 0xA2, 0xA2, 0xAC, 0x2C, 0xBB, 0x2B, 0xA4, 0x9A, 0x98, 0x92, 0x97, 0x9F,
 0x01, 0x8A, 0x9C, 0xD0, 0x2B, 0x20, 0x8F, 0x39, 0x2A, 0xBB, 0xA4, 0x24,
 0xB2, 0xBC, 0x4B, 0x26, 0x88, 0xA3, 0xC0, 0xC2, 0x27, 0xBA, 0x3E, 0xAC,
 0xAF, 0xB4, 0x26, 0x24, 0x3F, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0xC3, 0x5F, 0x32, 0x98, 0x03, 0xA1, 0xAA, 0x23, 0xB3, 0x2C,
 0x4B, 0x55, 0x25, 0x60, 0x96, 0xF2, 0xC1, 0x00
};

/* AC FXS RF14 Russia 150R+510R//47nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_RU[] =
{
  /* AC Profile */
 0x00, 0x00, 0x70, 0x4C, 0x01, 0x49, 0xCA, 0xED, 0x98, 0x98, 0x13, 0xB3,
 0xA4, 0xAB, 0xAC, 0xBB, 0xEC, 0x2B, 0x25, 0x9A, 0x3E, 0xAA, 0xA7, 0x9F,
 0x01, 0x8A, 0x2D, 0x01, 0xA8, 0x70, 0x2C, 0x2A, 0x7A, 0xC3, 0x23, 0x2D,
 0x22, 0xA6, 0xAB, 0x87, 0x88, 0x65, 0x40, 0xBC, 0xB6, 0x22, 0xAD, 0x2A,
 0xAF, 0x22, 0xA5, 0x32, 0xBE, 0x82, 0xF8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0xE3, 0x5A, 0x22, 0xAC, 0x14, 0xB2, 0x34, 0x53, 0xAF, 0xDA,
 0xFA, 0x9F, 0xA9, 0xF0, 0x96, 0x2E, 0x01, 0x00
};

/* AC FXS RF14 Sweden 270R+750R//150nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_SE[] =
{
  /* AC Profile */
 0x0E, 0x00, 0x42, 0x4C, 0x01, 0x49, 0xCA, 0xF0, 0x98, 0x24, 0xF1, 0xB4,
 0xA2, 0xAB, 0x4B, 0x4B, 0xAA, 0xA7, 0xC3, 0x9A, 0x98, 0x92, 0x97, 0x9F,
 0x01, 0x8A, 0x2D, 0x01, 0xB3, 0x20, 0x5B, 0xB8, 0x24, 0xA2, 0x3C, 0xAC,
 0xAA, 0x27, 0x32, 0x9F, 0x88, 0x3A, 0xA0, 0x3A, 0x25, 0x3A, 0xAD, 0x22,
 0xAD, 0xBE, 0x25, 0xA5, 0x2E, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0x73, 0x52, 0x22, 0xD2, 0x12, 0xB1, 0x4A, 0x23, 0xC4, 0xC3,
 0x6A, 0x97, 0xA8, 0xF0, 0x96, 0x2E, 0x01, 0x00
};

/* AC FXS RF14 Turkey 270R+750R//150nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_TK[] =
{
  /* AC Profile */
 0xD5, 0x00, 0xEC, 0x4C, 0x01, 0x49, 0xCA, 0xE8, 0x98, 0x4A, 0xA1, 0x34,
 0xA2, 0xA2, 0xAC, 0x2C, 0xBB, 0x2B, 0xA4, 0x9A, 0x98, 0x92, 0x97, 0x9F,
 0x01, 0x8A, 0x9C, 0xD0, 0x2B, 0x20, 0x8F, 0x39, 0x2A, 0xBB, 0xA4, 0x24,
 0xB2, 0xBC, 0x4B, 0x26, 0x88, 0xA3, 0xC0, 0xC2, 0x27, 0xBA, 0x3E, 0xAC,
 0xAF, 0xB4, 0x26, 0x24, 0x3F, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0xC3, 0x5F, 0x32, 0x98, 0x03, 0xA1, 0xAA, 0x23, 0xB3, 0x2C,
 0x4B, 0x55, 0x25, 0x60, 0x96, 0xF2, 0xC1, 0x00
};

/* AC FXS RF14 South Africa 220R+820R//115nF Normal Coefficients */
const VpProfileDataType AC_FXS_RF14_ZA[] =
{
  /* AC Profile */
 0x6D, 0x00, 0xA7, 0x4C, 0x01, 0x49, 0xCA, 0xEA, 0x98, 0xFA, 0xE1, 0x23,
 0xA2, 0xEA, 0xCB, 0xB3, 0xAA, 0xA3, 0xB3, 0x9A, 0x98, 0x92, 0x97, 0x9F,
 0x01, 0x8A, 0x2E, 0x01, 0xB3, 0x20, 0xAB, 0xA8, 0xCC, 0x24, 0x22, 0x97,
 0xAF, 0x8F, 0xD3, 0xBF, 0x88, 0x3B, 0xB0, 0x33, 0x46, 0x2A, 0x2E, 0x33,
 0x4F, 0xAE, 0xA5, 0xB3, 0xBE, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0x2A, 0x42, 0x22, 0x98, 0x02, 0x42, 0xA2, 0x43, 0xA6, 0xD8,
 0xFA, 0x8F, 0x29, 0xF0, 0x96, 0x2E, 0x01, 0x00
};

/* AC FXS RF14 600R Wideband Coefficients (Default)  */
const VpProfileDataType AC_FXS_RF14_WB_600R_DEF[] =
{
  /* AC Profile */
 0xED, 0x00, 0xA9, 0x4C, 0x01, 0x49, 0xCA, 0xFC, 0x98, 0xCC, 0x2A, 0xA2,
 0x3B, 0x98, 0xB3, 0x43, 0xA2, 0x3A, 0xAB, 0x9A, 0x2D, 0xAA, 0xB6, 0x9F,
 0x01, 0x8A, 0x2E, 0x01, 0xC4, 0xA0, 0x2E, 0x23, 0x3A, 0x2E, 0x9F, 0x87,
 0x4C, 0xCE, 0xB2, 0x27, 0x88, 0x4A, 0x20, 0xBA, 0xAC, 0x22, 0xB5, 0x5A,
 0xB5, 0x3B, 0x4D, 0xA2, 0x27, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0xCB, 0x5E, 0x22, 0xBC, 0x12, 0xA3, 0xA9, 0xFA, 0xC6, 0x34,
 0xE2, 0x9F, 0xBA, 0xF0, 0x96, 0x2E, 0x01, 0x00
};

/* AC FXS RF14 ETSI ES201 970 270R+750R//150nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_ETSI[] =
{
  /* AC Profile */
 0xAA, 0x00, 0x9C, 0x4C, 0x01, 0x49, 0xCA, 0xE0, 0x98, 0xAA, 0xA3, 0x6B,
 0x22, 0x43, 0xA2, 0xAC, 0xBF, 0x22, 0xA6, 0x9A, 0x01, 0x90, 0x01, 0x01,
 0x90, 0x8A, 0x2E, 0x01, 0xBA, 0x10, 0xA2, 0x3A, 0xBA, 0xE9, 0xAA, 0xB5,
 0x2A, 0xBC, 0xAF, 0x97, 0x88, 0xAB, 0xC0, 0x4A, 0xBE, 0xBA, 0x9F, 0xC3,
 0xAF, 0x3A, 0x3F, 0xCD, 0xA7, 0x82, 0xC8, 0x71, 0x80, 0x69, 0xF0, 0x50,
 0x00, 0x86, 0xBA, 0x52, 0xB2, 0xBA, 0x12, 0x42, 0x4A, 0x2A, 0x33, 0xBA,
 0x33, 0x34, 0x4B, 0x40, 0x96, 0xB2, 0xA0, 0x00
};

/* AC FXS RF14 Telcordia GR-57 900R+2.16uF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_GR57[] =
{
  /* AC Profile */
 0x1D, 0x00, 0x10, 0x4C, 0x01, 0x49, 0xCA, 0x0B, 0x98, 0xC2, 0xBB, 0x2A,
 0x2B, 0x2B, 0x8F, 0x6A, 0xB3, 0x2A, 0xA7, 0x9A, 0x7B, 0xA2, 0x36, 0x6E,
 0x01, 0x8A, 0x2D, 0xD0, 0x2C, 0xA0, 0x2B, 0xAA, 0x2C, 0x62, 0x22, 0xAA,
 0xBC, 0xA4, 0x23, 0xAE, 0x88, 0xA2, 0x30, 0xAD, 0xC9, 0x2C, 0x41, 0xB3,
 0xA9, 0xB2, 0x53, 0xAB, 0x3D, 0x82, 0xB8, 0x71, 0x80, 0x7A, 0xF0, 0x50,
 0x00, 0x86, 0x2B, 0x55, 0xB1, 0x3A, 0x0A, 0xA2, 0x98, 0xCA, 0xA4, 0xAA,
 0x5A, 0x87, 0x6A, 0x60, 0x96, 0x1E, 0x01, 0x00
};

/* AC FXS RF14 Austria 220R+820R//115nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_AT[] =
{
  /* AC Profile */
 0x30, 0x00, 0xC0, 0x4C, 0x01, 0x49, 0xCA, 0xDB, 0x98, 0xBA, 0xA1, 0xBA,
 0x22, 0x2A, 0xF4, 0xAA, 0xAB, 0x22, 0x65, 0x9A, 0x01, 0x90, 0x01, 0x01,
 0x90, 0x8A, 0x01, 0x11, 0xA3, 0x10, 0x2B, 0x29, 0x53, 0xB9, 0xBD, 0xB3,
 0x7B, 0xCC, 0xDB, 0xA6, 0x88, 0x2A, 0xB0, 0x2B, 0x9F, 0x2E, 0x3E, 0x22,
 0xBE, 0xC3, 0x26, 0x35, 0x2F, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0x2B, 0x53, 0x52, 0x4B, 0x12, 0x52, 0xDF, 0x32, 0xB4, 0x3A,
 0x5A, 0xA7, 0xB9, 0xF0, 0x96, 0x2E, 0x01, 0x00
};

/* AC FXS RF14 Australia 220R+820R//120nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_AU[] =
{
  /* AC Profile */
 0xC2, 0x00, 0x2E, 0x4C, 0x01, 0x49, 0xCA, 0xF0, 0x98, 0xBA, 0xA1, 0xAD,
 0xA3, 0xA2, 0x2C, 0x2D, 0xAB, 0x22, 0xD4, 0x9A, 0xCC, 0xA2, 0x97, 0x9F,
 0x01, 0x8A, 0x2E, 0x01, 0x7A, 0x10, 0xBA, 0x29, 0x2A, 0x3A, 0xA2, 0x32,
 0xB2, 0xAA, 0xF2, 0xA4, 0x88, 0xBA, 0xA0, 0x5C, 0x3F, 0x2C, 0xCE, 0xAA,
 0x87, 0xC2, 0x66, 0xDB, 0x7E, 0x82, 0xA8, 0x71, 0x80, 0x39, 0x70, 0x50,
 0x00, 0x86, 0xAA, 0x59, 0x81, 0x25, 0x1A, 0xA1, 0xBB, 0x2B, 0xB3, 0x2C,
 0x46, 0xD5, 0xBC, 0x60, 0x96, 0xAA, 0xB1, 0x00
};

/* AC FXS RF14 Belgium 270R+750R//150nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_BE[] =
{
  /* AC Profile */
 0xAA, 0x00, 0x9C, 0x4C, 0x01, 0x49, 0xCA, 0xE0, 0x98, 0xAA, 0xA3, 0x6B,
 0x22, 0x43, 0xA2, 0xAC, 0xBF, 0x22, 0xA6, 0x9A, 0x01, 0x90, 0x01, 0x01,
 0x90, 0x8A, 0x2E, 0x01, 0xBA, 0x10, 0xA2, 0x3A, 0xBA, 0xE9, 0xAA, 0xB5,
 0x2A, 0xBC, 0xAF, 0x97, 0x88, 0xAB, 0xC0, 0x4A, 0xBE, 0xBA, 0x9F, 0xC3,
 0xAF, 0x3A, 0x3F, 0xCD, 0xA7, 0x82, 0xC8, 0x71, 0x80, 0x69, 0xF0, 0x50,
 0x00, 0x86, 0xBA, 0x52, 0xB2, 0xBA, 0x12, 0x42, 0x4A, 0x2A, 0x33, 0xBA,
 0x33, 0x34, 0x4B, 0x40, 0x96, 0xB2, 0xA0, 0x00
};

/* AC FXS RF14 Bulgaria 220R+820R//115nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_BG[] =
{
  /* AC Profile */
 0x30, 0x00, 0xC0, 0x4C, 0x01, 0x49, 0xCA, 0xDB, 0x98, 0xBA, 0xA1, 0xBA,
 0x22, 0x2A, 0xF4, 0xAA, 0xAB, 0x22, 0x65, 0x9A, 0x01, 0x90, 0x01, 0x01,
 0x90, 0x8A, 0x01, 0x11, 0xA3, 0x10, 0x2B, 0x29, 0x53, 0xB9, 0xBD, 0xB3,
 0x7B, 0xCC, 0xDB, 0xA6, 0x88, 0x2A, 0xB0, 0x2B, 0x9F, 0x2E, 0x3E, 0x22,
 0xBE, 0xC3, 0x26, 0x35, 0x2F, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0x2B, 0x53, 0x52, 0x4B, 0x12, 0x52, 0xDF, 0x32, 0xB4, 0x3A,
 0x5A, 0xA7, 0xB9, 0xF0, 0x96, 0x2E, 0x01, 0x00
};

/* AC FXS RF14 Brazil 900R Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_BR[] =
{
  /* AC Profile */
 0xF3, 0x00, 0x33, 0x4C, 0x01, 0x49, 0xCA, 0x0B, 0x98, 0xAA, 0xAB, 0xAC,
 0xAA, 0x42, 0xAD, 0xB8, 0xF3, 0x34, 0xA5, 0x9A, 0xAC, 0xAA, 0xC7, 0x9F,
 0x01, 0x8A, 0x2E, 0x01, 0xAA, 0xA0, 0xCB, 0xDA, 0xAB, 0x22, 0xBC, 0xAA,
 0x3B, 0xA4, 0x42, 0x2F, 0x88, 0xAA, 0x40, 0x4A, 0x4A, 0xB3, 0x22, 0x3D,
 0x2B, 0xB2, 0xAF, 0x4A, 0x37, 0x82, 0xA9, 0xF1, 0x80, 0xB9, 0xF0, 0x50,
 0x00, 0x86, 0x2F, 0x5A, 0xA1, 0x3A, 0x03, 0xA2, 0xA3, 0xCC, 0x25, 0x3A,
 0x54, 0x3E, 0xCB, 0xF0, 0x96, 0x2E, 0x01, 0x00
};

/* AC FXS RF14 Switzerland 270R+750R//150nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_CH[] =
{
  /* AC Profile */
 0xAA, 0x00, 0x9C, 0x4C, 0x01, 0x49, 0xCA, 0xE0, 0x98, 0xAA, 0xA3, 0x6B,
 0x22, 0x43, 0xA2, 0xAC, 0xBF, 0x22, 0xA6, 0x9A, 0x01, 0x90, 0x01, 0x01,
 0x90, 0x8A, 0x2E, 0x01, 0xBA, 0x10, 0xA2, 0x3A, 0xBA, 0xE9, 0xAA, 0xB5,
 0x2A, 0xBC, 0xAF, 0x97, 0x88, 0xAB, 0xC0, 0x4A, 0xBE, 0xBA, 0x9F, 0xC3,
 0xAF, 0x3A, 0x3F, 0xCD, 0xA7, 0x82, 0xC8, 0x71, 0x80, 0x69, 0xF0, 0x50,
 0x00, 0x86, 0xBA, 0x52, 0xB2, 0xBA, 0x12, 0x42, 0x4A, 0x2A, 0x33, 0xBA,
 0x33, 0x34, 0x4B, 0x40, 0x96, 0xB2, 0xA0, 0x00
};

/* AC FXS RF14 China 200R+680R//100nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_CN[] =
{
  /* AC Profile */
 0xEF, 0x00, 0x46, 0x4C, 0x01, 0x49, 0xCA, 0xDA, 0x98, 0xB3, 0xB1, 0x23,
 0x32, 0x64, 0x97, 0xAA, 0x2B, 0x4A, 0xA4, 0x9A, 0x01, 0x90, 0x01, 0x01,
 0x90, 0x8A, 0x2E, 0x01, 0xCD, 0x10, 0x2B, 0x29, 0x22, 0x2A, 0xA2, 0x33,
 0x4B, 0x4C, 0xBD, 0xF6, 0x88, 0x2A, 0xC0, 0x52, 0x97, 0xF2, 0x2E, 0xAA,
 0x4E, 0xAC, 0xB5, 0x5B, 0xBE, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0x2A, 0x5A, 0xA2, 0xBA, 0x1B, 0x22, 0xCA, 0x24, 0xC3, 0x43,
 0x42, 0xA4, 0x4B, 0x50, 0x96, 0x3C, 0x41, 0x00
};

/* AC FXS RF14 Germany 220R+820R//115nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_DE[] =
{
  /* AC Profile */
 0x27, 0x00, 0xD7, 0x4C, 0x01, 0x49, 0xCA, 0xDB, 0x98, 0xBA, 0xA1, 0xBA,
 0x22, 0x2A, 0xF4, 0xAA, 0xAB, 0x22, 0x65, 0x9A, 0x01, 0x90, 0x01, 0x01,
 0x90, 0x8A, 0x2E, 0x01, 0xA3, 0x10, 0x2B, 0x29, 0x53, 0xB9, 0xBD, 0xB3,
 0x7B, 0xCC, 0xDB, 0xA6, 0x88, 0x2A, 0xB0, 0x2B, 0x9F, 0x2E, 0x3E, 0x22,
 0xBE, 0xC3, 0x26, 0x35, 0x2F, 0x82, 0xB8, 0x71, 0x80, 0xAB, 0xF0, 0x50,
 0x00, 0x86, 0xCA, 0x5A, 0xA2, 0xBA, 0x1F, 0x22, 0x3B, 0x2A, 0x23, 0x2B,
 0x3A, 0x24, 0x4B, 0x40, 0x96, 0x22, 0x21, 0x00
};

/* AC FXS RF14 Denmark 270R+750R//150nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_DK[] =
{
  /* AC Profile */
 0xAA, 0x00, 0x9C, 0x4C, 0x01, 0x49, 0xCA, 0xE0, 0x98, 0xAA, 0xA3, 0x6B,
 0x22, 0x43, 0xA2, 0xAC, 0xBF, 0x22, 0xA6, 0x9A, 0x01, 0x90, 0x01, 0x01,
 0x90, 0x8A, 0x2E, 0x01, 0xBA, 0x10, 0xA2, 0x3A, 0xBA, 0xE9, 0xAA, 0xB5,
 0x2A, 0xBC, 0xAF, 0x97, 0x88, 0xAB, 0xC0, 0x4A, 0xBE, 0xBA, 0x9F, 0xC3,
 0xAF, 0x3A, 0x3F, 0xCD, 0xA7, 0x82, 0xC8, 0x71, 0x80, 0x69, 0xF0, 0x50,
 0x00, 0x86, 0xBA, 0x52, 0xB2, 0xBA, 0x12, 0x42, 0x4A, 0x2A, 0x33, 0xBA,
 0x33, 0x34, 0x4B, 0x40, 0x96, 0xB2, 0xA0, 0x00
};

/* AC FXS RF14 Spain 270R+750R//150nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_ES[] =
{
  /* AC Profile */
 0xAA, 0x00, 0x9C, 0x4C, 0x01, 0x49, 0xCA, 0xE0, 0x98, 0xAA, 0xA3, 0x6B,
 0x22, 0x43, 0xA2, 0xAC, 0xBF, 0x22, 0xA6, 0x9A, 0x01, 0x90, 0x01, 0x01,
 0x90, 0x8A, 0x2E, 0x01, 0xBA, 0x10, 0xA2, 0x3A, 0xBA, 0xE9, 0xAA, 0xB5,
 0x2A, 0xBC, 0xAF, 0x97, 0x88, 0xAB, 0xC0, 0x4A, 0xBE, 0xBA, 0x9F, 0xC3,
 0xAF, 0x3A, 0x3F, 0xCD, 0xA7, 0x82, 0xC8, 0x71, 0x80, 0x69, 0xF0, 0x50,
 0x00, 0x86, 0xBA, 0x52, 0xB2, 0xBA, 0x12, 0x42, 0x4A, 0x2A, 0x33, 0xBA,
 0x33, 0x34, 0x4B, 0x40, 0x96, 0xB2, 0xA0, 0x00
};

/* AC FXS RF14 Finland 270R+910R//120nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_FI[] =
{
  /* AC Profile */
 0xF1, 0x00, 0xB1, 0x4C, 0x01, 0x49, 0xCA, 0xE5, 0x98, 0x2E, 0x52, 0x2A,
 0xB2, 0xBA, 0xB3, 0xA2, 0xAE, 0x22, 0x37, 0x9A, 0x01, 0x90, 0x01, 0x01,
 0x90, 0x8A, 0x01, 0x11, 0x2B, 0x10, 0xA3, 0xB9, 0xA2, 0xC9, 0xAA, 0xB4,
 0x3A, 0xBC, 0x22, 0x37, 0x88, 0xEC, 0xB0, 0xB3, 0x2E, 0xE2, 0x3F, 0xAA,
 0x8F, 0x25, 0x97, 0xDB, 0xAF, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0x7A, 0x5A, 0xB2, 0x3B, 0x12, 0xA1, 0xAC, 0x24, 0xA2, 0x73,
 0x32, 0xB3, 0x22, 0x40, 0x96, 0x24, 0xA0, 0x00
};

/* AC FXS RF14 France 270R+750R//150nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_FR[] =
{
  /* AC Profile */
 0x98, 0x00, 0x26, 0x4C, 0x01, 0x49, 0xCA, 0xD7, 0x98, 0x4F, 0x22, 0xB5,
 0xB1, 0xC6, 0x23, 0x3A, 0xAB, 0x24, 0xB5, 0x9A, 0x01, 0x90, 0x01, 0x01,
 0x90, 0x8A, 0x01, 0x11, 0x22, 0x10, 0x3A, 0x29, 0x3C, 0x29, 0x32, 0xB3,
 0x4A, 0x2C, 0x2E, 0xB6, 0x88, 0xAC, 0xB0, 0x43, 0x37, 0xAB, 0x3E, 0xB4,
 0x3E, 0x22, 0x37, 0x22, 0xAF, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
 0x00, 0x86, 0xAA, 0x52, 0xE2, 0x2B, 0x12, 0x62, 0x63, 0x3A, 0x24, 0xA2,
 0x5C, 0x36, 0x39, 0xF0, 0x96, 0x2E, 0x01, 0x00
};

/* AC FXS RF14 UK 370R+620R//310nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_GB[] =
{
  /* AC Profile */
 0xDB, 0x00, 0x85, 0x4C, 0x01, 0x49, 0xCA, 0xE6, 0x98, 0x2B, 0x2A, 0x3B,
 0xD1, 0xB4, 0x22, 0x23, 0x6C, 0xA6, 0x43, 0x9A, 0xBA, 0x23, 0x25, 0x2D,
 0x01, 0x8A, 0x32, 0xC0, 0x2A, 0x10, 0xA5, 0x4A, 0x62, 0xBA, 0xA6, 0xA9,
 0xAA, 0xC3, 0xA2, 0xAB, 0x88, 0x32, 0xC0, 0x3D, 0xB3, 0xA3, 0xBA, 0x53,
 0xC2, 0xAA, 0xAA, 0xC4, 0xA3, 0x82, 0xE8, 0x71, 0x80, 0xAA, 0xF0, 0x50,
 0x00, 0x86, 0x3A, 0x5B, 0xC2, 0xFA, 0x12, 0xB2, 0xE2, 0x33, 0xE3, 0xBA,
 0x33, 0x24, 0xA4, 0x40, 0x96, 0x23, 0xB0, 0x00
};

/* AC FXS RF14 Greece 270R+750R//150nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_GR[] =
{
  /* AC Profile */
 0xAA, 0x00, 0x9C, 0x4C, 0x01, 0x49, 0xCA, 0xE0, 0x98, 0xAA, 0xA3, 0x6B,
 0x22, 0x43, 0xA2, 0xAC, 0xBF, 0x22, 0xA6, 0x9A, 0x01, 0x90, 0x01, 0x01,
 0x90, 0x8A, 0x2E, 0x01, 0xBA, 0x10, 0xA2, 0x3A, 0xBA, 0xE9, 0xAA, 0xB5,
 0x2A, 0xBC, 0xAF, 0x97, 0x88, 0xAB, 0xC0, 0x4A, 0xBE, 0xBA, 0x9F, 0xC3,
 0xAF, 0x3A, 0x3F, 0xCD, 0xA7, 0x82, 0xC8, 0x71, 0x80, 0x69, 0xF0, 0x50,
 0x00, 0x86, 0xBA, 0x52, 0xB2, 0xBA, 0x12, 0x42, 0x4A, 0x2A, 0x33, 0xBA,
 0x33, 0x34, 0x4B, 0x40, 0x96, 0xB2, 0xA0, 0x00
};

/* AC FXS RF14 Hungary 270R+750R//150nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_HU[] =
{
  /* AC Profile */
 0xAA, 0x00, 0x9C, 0x4C, 0x01, 0x49, 0xCA, 0xE0, 0x98, 0xAA, 0xA3, 0x6B,
 0x22, 0x43, 0xA2, 0xAC, 0xBF, 0x22, 0xA6, 0x9A, 0x01, 0x90, 0x01, 0x01,
 0x90, 0x8A, 0x2E, 0x01, 0xBA, 0x10, 0xA2, 0x3A, 0xBA, 0xE9, 0xAA, 0xB5,
 0x2A, 0xBC, 0xAF, 0x97, 0x88, 0xAB, 0xC0, 0x4A, 0xBE, 0xBA, 0x9F, 0xC3,
 0xAF, 0x3A, 0x3F, 0xCD, 0xA7, 0x82, 0xC8, 0x71, 0x80, 0x69, 0xF0, 0x50,
 0x00, 0x86, 0xBA, 0x52, 0xB2, 0xBA, 0x12, 0x42, 0x4A, 0x2A, 0x33, 0xBA,
 0x33, 0x34, 0x4B, 0x40, 0x96, 0xB2, 0xA0, 0x00
};

/* AC FXS RF14 Ireland 270R+750R//150nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_IE[] =
{
  /* AC Profile */
 0xAA, 0x00, 0x9C, 0x4C, 0x01, 0x49, 0xCA, 0xE0, 0x98, 0xAA, 0xA3, 0x6B,
 0x22, 0x43, 0xA2, 0xAC, 0xBF, 0x22, 0xA6, 0x9A, 0x01, 0x90, 0x01, 0x01,
 0x90, 0x8A, 0x2E, 0x01, 0xBA, 0x10, 0xA2, 0x3A, 0xBA, 0xE9, 0xAA, 0xB5,
 0x2A, 0xBC, 0xAF, 0x97, 0x88, 0xAB, 0xC0, 0x4A, 0xBE, 0xBA, 0x9F, 0xC3,
 0xAF, 0x3A, 0x3F, 0xCD, 0xA7, 0x82, 0xC8, 0x71, 0x80, 0x69, 0xF0, 0x50,
 0x00, 0x86, 0xBA, 0x52, 0xB2, 0xBA, 0x12, 0x42, 0x4A, 0x2A, 0x33, 0xBA,
 0x33, 0x34, 0x4B, 0x40, 0x96, 0xB2, 0xA0, 0x00
};

/* AC FXS RF14 Israel 270R+750R//150nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_IL[] =
{
  /* AC Profile */
 0xAA, 0x00, 0x9C, 0x4C, 0x01, 0x49, 0xCA, 0xE0, 0x98, 0xAA, 0xA3, 0x6B,
 0x22, 0x43, 0xA2, 0xAC, 0xBF, 0x22, 0xA6, 0x9A, 0x01, 0x90, 0x01, 0x01,
 0x90, 0x8A, 0x2E, 0x01, 0xBA, 0x10, 0xA2, 0x3A, 0xBA, 0xE9, 0xAA, 0xB5,
 0x2A, 0xBC, 0xAF, 0x97, 0x88, 0xAB, 0xC0, 0x4A, 0xBE, 0xBA, 0x9F, 0xC3,
 0xAF, 0x3A, 0x3F, 0xCD, 0xA7, 0x82, 0xC8, 0x71, 0x80, 0x69, 0xF0, 0x50,
 0x00, 0x86, 0xBA, 0x52, 0xB2, 0xBA, 0x12, 0x42, 0x4A, 0x2A, 0x33, 0xBA,
 0x33, 0x34, 0x4B, 0x40, 0x96, 0xB2, 0xA0, 0x00
};

/* AC FXS RF14 Iceland 270R+750R//150nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_IS[] =
{
  /* AC Profile */
 0xAA, 0x00, 0x9C, 0x4C, 0x01, 0x49, 0xCA, 0xE0, 0x98, 0xAA, 0xA3, 0x6B,
 0x22, 0x43, 0xA2, 0xAC, 0xBF, 0x22, 0xA6, 0x9A, 0x01, 0x90, 0x01, 0x01,
 0x90, 0x8A, 0x2E, 0x01, 0xBA, 0x10, 0xA2, 0x3A, 0xBA, 0xE9, 0xAA, 0xB5,
 0x2A, 0xBC, 0xAF, 0x97, 0x88, 0xAB, 0xC0, 0x4A, 0xBE, 0xBA, 0x9F, 0xC3,
 0xAF, 0x3A, 0x3F, 0xCD, 0xA7, 0x82, 0xC8, 0x71, 0x80, 0x69, 0xF0, 0x50,
 0x00, 0x86, 0xBA, 0x52, 0xB2, 0xBA, 0x12, 0x42, 0x4A, 0x2A, 0x33, 0xBA,
 0x33, 0x34, 0x4B, 0x40, 0x96, 0xB2, 0xA0, 0x00
};

/* AC FXS RF14 Italy 270R+750R//150nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_IT[] =
{
  /* AC Profile */
 0xAA, 0x00, 0x9C, 0x4C, 0x01, 0x49, 0xCA, 0xE0, 0x98, 0xAA, 0xA3, 0x6B,
 0x22, 0x43, 0xA2, 0xAC, 0xBF, 0x22, 0xA6, 0x9A, 0x01, 0x90, 0x01, 0x01,
 0x90, 0x8A, 0x2E, 0x01, 0xBA, 0x10, 0xA2, 0x3A, 0xBA, 0xE9, 0xAA, 0xB5,
 0x2A, 0xBC, 0xAF, 0x97, 0x88, 0xAB, 0xC0, 0x4A, 0xBE, 0xBA, 0x9F, 0xC3,
 0xAF, 0x3A, 0x3F, 0xCD, 0xA7, 0x82, 0xC8, 0x71, 0x80, 0x69, 0xF0, 0x50,
 0x00, 0x86, 0xBA, 0x52, 0xB2, 0xBA, 0x12, 0x42, 0x4A, 0x2A, 0x33, 0xBA,
 0x33, 0x34, 0x4B, 0x40, 0x96, 0xB2, 0xA0, 0x00
};

/* AC FXS RF14 Japan 600R+1uF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_JP[] =
{
  /* AC Profile */
 0x83, 0x00, 0xF8, 0x4C, 0x01, 0x49, 0xCA, 0xFF, 0x98, 0xAB, 0x3A, 0x35,
 0x2C, 0xA2, 0x43, 0xAA, 0x23, 0x26, 0xAB, 0x9A, 0xAA, 0xA2, 0x24, 0x1D,
 0x01, 0x8A, 0xAA, 0xB0, 0xA3, 0xA0, 0x22, 0xA3, 0x2A, 0xCE, 0x2A, 0xBD,
 0x62, 0xA5, 0x54, 0xBE, 0x88, 0x23, 0x30, 0x2E, 0xAC, 0xB2, 0xA6, 0xBA,
 0x26, 0xA2, 0xED, 0x22, 0x27, 0x82, 0xAA, 0x61, 0x80, 0x2A, 0x60, 0x50,
 0x00, 0x86, 0x23, 0x62, 0x22, 0x98, 0x0C, 0x43, 0x2A, 0x44, 0xA4, 0xBD,
 0x62, 0xB5, 0x4F, 0x50, 0x96, 0xBC, 0x01, 0x00
};

/* AC FXS RF14 Netherlands 270R+750R//150nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_NL[] =
{
  /* AC Profile */
 0xAA, 0x00, 0x9C, 0x4C, 0x01, 0x49, 0xCA, 0xE0, 0x98, 0xAA, 0xA3, 0x6B,
 0x22, 0x43, 0xA2, 0xAC, 0xBF, 0x22, 0xA6, 0x9A, 0x01, 0x90, 0x01, 0x01,
 0x90, 0x8A, 0x2E, 0x01, 0xBA, 0x10, 0xA2, 0x3A, 0xBA, 0xE9, 0xAA, 0xB5,
 0x2A, 0xBC, 0xAF, 0x97, 0x88, 0xAB, 0xC0, 0x4A, 0xBE, 0xBA, 0x9F, 0xC3,
 0xAF, 0x3A, 0x3F, 0xCD, 0xA7, 0x82, 0xC8, 0x71, 0x80, 0x69, 0xF0, 0x50,
 0x00, 0x86, 0xBA, 0x52, 0xB2, 0xBA, 0x12, 0x42, 0x4A, 0x2A, 0x33, 0xBA,
 0x33, 0x34, 0x4B, 0x40, 0x96, 0xB2, 0xA0, 0x00
};

/* AC FXS RF14 Norway 270R+750R//150nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_NO[] =
{
  /* AC Profile */
 0xAA, 0x00, 0x9C, 0x4C, 0x01, 0x49, 0xCA, 0xE0, 0x98, 0xAA, 0xA3, 0x6B,
 0x22, 0x43, 0xA2, 0xAC, 0xBF, 0x22, 0xA6, 0x9A, 0x01, 0x90, 0x01, 0x01,
 0x90, 0x8A, 0x2E, 0x01, 0xBA, 0x10, 0xA2, 0x3A, 0xBA, 0xE9, 0xAA, 0xB5,
 0x2A, 0xBC, 0xAF, 0x97, 0x88, 0xAB, 0xC0, 0x4A, 0xBE, 0xBA, 0x9F, 0xC3,
 0xAF, 0x3A, 0x3F, 0xCD, 0xA7, 0x82, 0xC8, 0x71, 0x80, 0x69, 0xF0, 0x50,
 0x00, 0x86, 0xBA, 0x52, 0xB2, 0xBA, 0x12, 0x42, 0x4A, 0x2A, 0x33, 0xBA,
 0x33, 0x34, 0x4B, 0x40, 0x96, 0xB2, 0xA0, 0x00
};

/* AC FXS RF14 New Zealand 370R+620R//310nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_NZ[] =
{
  /* AC Profile */
 0x11, 0x00, 0xE2, 0x4C, 0x01, 0x49, 0xCA, 0xF2, 0x98, 0x5A, 0x29, 0xCB,
 0xA1, 0xA3, 0xA1, 0xB4, 0x3D, 0x22, 0x24, 0x9A, 0x22, 0xA2, 0xE5, 0x2E,
 0x01, 0x8A, 0x3A, 0xB0, 0xDA, 0x30, 0xBD, 0xAB, 0x42, 0xAF, 0xCA, 0xA9,
 0x32, 0x23, 0xBA, 0xAB, 0x88, 0x2A, 0xB0, 0xAD, 0x42, 0x4C, 0xA9, 0xA6,
 0xA1, 0x2A, 0x2A, 0xA2, 0x43, 0x82, 0x23, 0x71, 0x80, 0x4A, 0x60, 0x50,
 0x00, 0x86, 0xCA, 0x59, 0x81, 0xBA, 0x12, 0xA2, 0x44, 0x3B, 0xA3, 0x23,
 0x4A, 0xC4, 0x3A, 0x40, 0x96, 0x22, 0xB0, 0x00
};

/* AC FXS RF14 Portugal 270R+750R//150nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_PT[] =
{
  /* AC Profile */
 0xAA, 0x00, 0x9C, 0x4C, 0x01, 0x49, 0xCA, 0xE0, 0x98, 0xAA, 0xA3, 0x6B,
 0x22, 0x43, 0xA2, 0xAC, 0xBF, 0x22, 0xA6, 0x9A, 0x01, 0x90, 0x01, 0x01,
 0x90, 0x8A, 0x2E, 0x01, 0xBA, 0x10, 0xA2, 0x3A, 0xBA, 0xE9, 0xAA, 0xB5,
 0x2A, 0xBC, 0xAF, 0x97, 0x88, 0xAB, 0xC0, 0x4A, 0xBE, 0xBA, 0x9F, 0xC3,
 0xAF, 0x3A, 0x3F, 0xCD, 0xA7, 0x82, 0xC8, 0x71, 0x80, 0x69, 0xF0, 0x50,
 0x00, 0x86, 0xBA, 0x52, 0xB2, 0xBA, 0x12, 0x42, 0x4A, 0x2A, 0x33, 0xBA,
 0x33, 0x34, 0x4B, 0x40, 0x96, 0xB2, 0xA0, 0x00
};

/* AC FXS RF14 Russia 150R+510R//47nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_RU[] =
{
  /* AC Profile */
 0x2E, 0x00, 0x60, 0x4C, 0x01, 0x49, 0xCA, 0xE6, 0x98, 0x2D, 0xC2, 0x62,
 0x43, 0xA2, 0xBC, 0x24, 0xEB, 0xBA, 0xB4, 0x9A, 0x24, 0xB5, 0x97, 0x9F,
 0x01, 0x8A, 0x2E, 0x01, 0x2C, 0x30, 0x62, 0x3A, 0x22, 0x2B, 0x3B, 0xA2,
 0xA2, 0xCB, 0x2A, 0xE5, 0x88, 0xDA, 0x40, 0xBA, 0x35, 0x24, 0xDD, 0x4A,
 0xAD, 0xD4, 0xA4, 0xAF, 0xAD, 0x82, 0xA8, 0x71, 0x80, 0xB9, 0xF0, 0x50,
 0x00, 0x86, 0x2A, 0x5A, 0xB2, 0x3B, 0x1A, 0x32, 0x2A, 0x32, 0xA5, 0xA8,
 0x72, 0x9F, 0x29, 0xF0, 0x96, 0x2E, 0x01, 0x00
};

/* AC FXS RF14 Sweden 270R+750R//150nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_SE[] =
{
  /* AC Profile */
 0xAA, 0x00, 0x9C, 0x4C, 0x01, 0x49, 0xCA, 0xE0, 0x98, 0xAA, 0xA3, 0x6B,
 0x22, 0x43, 0xA2, 0xAC, 0xBF, 0x22, 0xA6, 0x9A, 0x01, 0x90, 0x01, 0x01,
 0x90, 0x8A, 0x2E, 0x01, 0xBA, 0x10, 0xA2, 0x3A, 0xBA, 0xE9, 0xAA, 0xB5,
 0x2A, 0xBC, 0xAF, 0x97, 0x88, 0xAB, 0xC0, 0x4A, 0xBE, 0xBA, 0x9F, 0xC3,
 0xAF, 0x3A, 0x3F, 0xCD, 0xA7, 0x82, 0xC8, 0x71, 0x80, 0x69, 0xF0, 0x50,
 0x00, 0x86, 0xBA, 0x52, 0xB2, 0xBA, 0x12, 0x42, 0x4A, 0x2A, 0x33, 0xBA,
 0x33, 0x34, 0x4B, 0x40, 0x96, 0xB2, 0xA0, 0x00
};

/* AC FXS RF14 Turkey 270R+750R//150nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_TK[] =
{
  /* AC Profile */
 0xAA, 0x00, 0x9C, 0x4C, 0x01, 0x49, 0xCA, 0xE0, 0x98, 0xAA, 0xA3, 0x6B,
 0x22, 0x43, 0xA2, 0xAC, 0xBF, 0x22, 0xA6, 0x9A, 0x01, 0x90, 0x01, 0x01,
 0x90, 0x8A, 0x2E, 0x01, 0xBA, 0x10, 0xA2, 0x3A, 0xBA, 0xE9, 0xAA, 0xB5,
 0x2A, 0xBC, 0xAF, 0x97, 0x88, 0xAB, 0xC0, 0x4A, 0xBE, 0xBA, 0x9F, 0xC3,
 0xAF, 0x3A, 0x3F, 0xCD, 0xA7, 0x82, 0xC8, 0x71, 0x80, 0x69, 0xF0, 0x50,
 0x00, 0x86, 0xBA, 0x52, 0xB2, 0xBA, 0x12, 0x42, 0x4A, 0x2A, 0x33, 0xBA,
 0x33, 0x34, 0x4B, 0x40, 0x96, 0xB2, 0xA0, 0x00
};

/* AC FXS RF14 South Africa 220R+820R//115nF Wideband Coefficients */
const VpProfileDataType AC_FXS_RF14_WB_ZA[] =
{
  /* AC Profile */
 0x27, 0x00, 0xD7, 0x4C, 0x01, 0x49, 0xCA, 0xDB, 0x98, 0xBA, 0xA1, 0xBA,
 0x22, 0x2A, 0xF4, 0xAA, 0xAB, 0x22, 0x65, 0x9A, 0x01, 0x90, 0x01, 0x01,
 0x90, 0x8A, 0x2E, 0x01, 0xA3, 0x10, 0x2B, 0x29, 0x53, 0xB9, 0xBD, 0xB3,
 0x7B, 0xCC, 0xDB, 0xA6, 0x88, 0x2A, 0xB0, 0x2B, 0x9F, 0x2E, 0x3E, 0x22,
 0xBE, 0xC3, 0x26, 0x35, 0x2F, 0x82, 0xB8, 0x71, 0x80, 0xAB, 0xF0, 0x50,
 0x00, 0x86, 0xCA, 0x5A, 0xA2, 0xBA, 0x1F, 0x22, 0x3B, 0x2A, 0x23, 0x2B,
 0x3A, 0x24, 0x4B, 0x40, 0x96, 0x22, 0x21, 0x00
};

/************** Ring_Parameters **************/

/* Default Ringing 25Hz 62Vrms + 48VDC Tracking, DC Trip - Use for all countries unless country profile exists */
const VpProfileDataType RING_ZL880_IB150V_DEF[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x04, 0x00,       /* Ringing Profile for ZL88702 device */
   0x12,                   /* Profile length = 18 + 4 = 22 bytes */
   0x01,                   /* Profile format version = 1 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC0, 0x18, 0x00, 0x00, /* Ringing Generator Parameters: */
         0x00, 0x44, 0x57, /* 24.9 Hz Sine; 1.41 CF; 106.00 Vpk; 0.00 V bias */
         0xE0, 0x00, 0x00, /* Ring trip cycles = 1; RTHALFCYC = 1 */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0x3E,                   /* Ring Trip Method = DC; Threshold = 31.0 mA */
   0x0F,                   /* Ringing Current Limit = 80 mA */
   0x15,                   /* Tracked; Max Ringing Supply = 110 V */
   0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/* Austria Ringing 50Hz 62Vrms + 48VDC Tracking, DC Trip */
const VpProfileDataType RING_ZL880_FB150V_AT[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x04, 0x00,       /* Ringing Profile for ZL88702 device */
   0x12,                   /* Profile length = 18 + 4 = 22 bytes */
   0x01,                   /* Profile format version = 1 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC0, 0x10, 0x27, 0xCB, /* Ringing Generator Parameters: */
         0x00, 0x89, 0x48, /* 50.2 Hz Sine; 1.41 CF; 87.70 Vpk; 48.00 V bias */
         0xB4, 0x00, 0x00, /* Ring trip cycles = 2; RTHALFCYC = 1 */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0x1E,                   /* Ring Trip Method = DC; Threshold = 15.0 mA */
   0x09,                   /* Ringing Current Limit = 68 mA */
   0x1B,                   /* Tracked; Max Ringing Supply = 140 V */
   0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/* Canada Ringing 20Hz 62Vrms + 48VDC Tracking, DC Trip */
const VpProfileDataType RING_ZL880_FB150V_CA[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x04, 0x00,       /* Ringing Profile for ZL88702 device */
   0x12,                   /* Profile length = 18 + 4 = 22 bytes */
   0x01,                   /* Profile format version = 1 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC0, 0x10, 0x27, 0xCB, /* Ringing Generator Parameters: */
         0x00, 0x37, 0x48, /* 20.1 Hz Sine; 1.41 CF; 87.70 Vpk; 48.00 V bias */
         0xB4, 0x00, 0x00, /* Ring trip cycles = 2; RTHALFCYC = 1 */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0x1E,                   /* Ring Trip Method = DC; Threshold = 15.0 mA */
   0x09,                   /* Ringing Current Limit = 68 mA */
   0x1B,                   /* Tracked; Max Ringing Supply = 140 V */
   0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/* Finland Ringing 50Hz 62Vrms + 48VDC Tracking, DC Trip */
const VpProfileDataType RING_ZL880_FB150V_FI[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x04, 0x00,       /* Ringing Profile for ZL88702 device */
   0x12,                   /* Profile length = 18 + 4 = 22 bytes */
   0x01,                   /* Profile format version = 1 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC0, 0x10, 0x27, 0xCB, /* Ringing Generator Parameters: */
         0x00, 0x89, 0x48, /* 50.2 Hz Sine; 1.41 CF; 87.70 Vpk; 48.00 V bias */
         0xB4, 0x00, 0x00, /* Ring trip cycles = 2; RTHALFCYC = 1 */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0x1E,                   /* Ring Trip Method = DC; Threshold = 15.0 mA */
   0x09,                   /* Ringing Current Limit = 68 mA */
   0x1B,                   /* Tracked; Max Ringing Supply = 140 V */
   0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/* France Ringing 50Hz 62Vrms + 48VDC Tracking, DC Trip */
const VpProfileDataType RING_ZL880_FB150V_FR[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x04, 0x00,       /* Ringing Profile for ZL88702 device */
   0x12,                   /* Profile length = 18 + 4 = 22 bytes */
   0x01,                   /* Profile format version = 1 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC0, 0x10, 0x27, 0xCB, /* Ringing Generator Parameters: */
         0x00, 0x89, 0x48, /* 50.2 Hz Sine; 1.41 CF; 87.70 Vpk; 48.00 V bias */
         0xB4, 0x00, 0x00, /* Ring trip cycles = 2; RTHALFCYC = 1 */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0x1E,                   /* Ring Trip Method = DC; Threshold = 15.0 mA */
   0x09,                   /* Ringing Current Limit = 68 mA */
   0x1B,                   /* Tracked; Max Ringing Supply = 140 V */
   0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/* Hong Kong Ringing 20Hz 62Vrms + 48VDC Tracking, DC Trip */
const VpProfileDataType RING_ZL880_FB150V_HK[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x04, 0x00,       /* Ringing Profile for ZL88702 device */
   0x12,                   /* Profile length = 18 + 4 = 22 bytes */
   0x01,                   /* Profile format version = 1 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC0, 0x10, 0x27, 0xCB, /* Ringing Generator Parameters: */
         0x00, 0x37, 0x48, /* 20.1 Hz Sine; 1.41 CF; 87.70 Vpk; 48.00 V bias */
         0xB4, 0x00, 0x00, /* Ring trip cycles = 2; RTHALFCYC = 1 */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0x1E,                   /* Ring Trip Method = DC; Threshold = 15.0 mA */
   0x09,                   /* Ringing Current Limit = 68 mA */
   0x1B,                   /* Tracked; Max Ringing Supply = 140 V */
   0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/* Japan Ringing 16Hz 62Vrms + 48VDC Tracking, DC Trip */
const VpProfileDataType RING_ZL880_FB150V_JP[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x04, 0x00,       /* Ringing Profile for ZL88702 device */
   0x12,                   /* Profile length = 18 + 4 = 22 bytes */
   0x01,                   /* Profile format version = 1 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC0, 0x18, 0x27, 0xCB, /* Ringing Generator Parameters: */
         0x00, 0x2C, 0x48, /* 16.1 Hz Sine; 1.41 CF; 87.70 Vpk; 48.00 V bias */
         0xB4, 0x00, 0x00, /* Ring trip cycles = 1; RTHALFCYC = 1 */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0x1E,                   /* Ring Trip Method = DC; Threshold = 15.0 mA */
   0x09,                   /* Ringing Current Limit = 68 mA */
   0x1B,                   /* Tracked; Max Ringing Supply = 140 V */
   0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/* Korea Ringing 20Hz 62Vrms + 48VDC Tracking, DC Trip */
const VpProfileDataType RING_ZL880_FB150V_KR[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x04, 0x00,       /* Ringing Profile for ZL88702 device */
   0x12,                   /* Profile length = 18 + 4 = 22 bytes */
   0x01,                   /* Profile format version = 1 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC0, 0x10, 0x27, 0xCB, /* Ringing Generator Parameters: */
         0x00, 0x37, 0x48, /* 20.1 Hz Sine; 1.41 CF; 87.80 Vpk; 48.00 V bias */
         0xCA, 0x00, 0x00, /* Ring trip cycles = 2; RTHALFCYC = 1 */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0x1E,                   /* Ring Trip Method = DC; Threshold = 15.0 mA */
   0x09,                   /* Ringing Current Limit = 68 mA */
   0x1B,                   /* Tracked; Max Ringing Supply = 140 V */
   0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/* Singapore 24Hz 62Vrms + 48VDC Tracking, DC Trip */
const VpProfileDataType RING_ZL880_FB150V_SG[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x04, 0x00,       /* Ringing Profile for ZL88702 device */
   0x12,                   /* Profile length = 18 + 4 = 22 bytes */
   0x01,                   /* Profile format version = 1 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC0, 0x18, 0x27, 0xCB, /* Ringing Generator Parameters: */
         0x00, 0x42, 0x48, /* 24.2 Hz Sine; 1.41 CF; 87.70 Vpk; 48.00 V bias */
         0xB4, 0x00, 0x00, /* Ring trip cycles = 1; RTHALFCYC = 1 */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0x1E,                   /* Ring Trip Method = DC; Threshold = 15.0 mA */
   0x09,                   /* Ringing Current Limit = 68 mA */
   0x1B,                   /* Tracked; Max Ringing Supply = 140 V */
   0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/* Taiwan Ringing 20Hz 62Vrms + 48VDC Tracking, DC Trip */
const VpProfileDataType RING_ZL880_FB150V_TW[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x04, 0x00,       /* Ringing Profile for ZL88702 device */
   0x12,                   /* Profile length = 18 + 4 = 22 bytes */
   0x01,                   /* Profile format version = 1 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC0, 0x10, 0x27, 0xCB, /* Ringing Generator Parameters: */
         0x00, 0x37, 0x48, /* 20.1 Hz Sine; 1.41 CF; 87.70 Vpk; 48.00 V bias */
         0xB4, 0x00, 0x00, /* Ring trip cycles = 2; RTHALFCYC = 1 */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0x1E,                   /* Ring Trip Method = DC; Threshold = 15.0 mA */
   0x09,                   /* Ringing Current Limit = 68 mA */
   0x1B,                   /* Tracked; Max Ringing Supply = 140 V */
   0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/* USA Ringing 20Hz 62Vrms + 48VDC Tracking, DC Trip */
const VpProfileDataType RING_ZL880_FB150V_US[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x04, 0x00,       /* Ringing Profile for ZL88702 device */
   0x12,                   /* Profile length = 18 + 4 = 22 bytes */
   0x01,                   /* Profile format version = 1 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC0, 0x10, 0x27, 0xCB, /* Ringing Generator Parameters: */
         0x00, 0x37, 0x48, /* 20.1 Hz Sine; 1.41 CF; 87.70 Vpk; 48.00 V bias */
         0xB4, 0x00, 0x00, /* Ring trip cycles = 2; RTHALFCYC = 1 */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0x1E,                   /* Ring Trip Method = DC; Threshold = 15.0 mA */
   0x09,                   /* Ringing Current Limit = 68 mA */
   0x1B,                   /* Tracked; Max Ringing Supply = 140 V */
   0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/* USA Ringing 20Hz 62Vrms + 20VDC Tracking, DC Trip */
const VpProfileDataType RING_ZL880_IB135V_US[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x04, 0x00,       /* Ringing Profile for ZL88702 device */
   0x12,                   /* Profile length = 18 + 4 = 22 bytes */
   0x01,                   /* Profile format version = 1 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC0, 0x10, 0x27, 0xCB, /* Ringing Generator Parameters: */
         0x00, 0x37, 0x48, /* 20.1 Hz Sine; 1.41 CF; 87.70 Vpk; 48.00 V bias */
         0xB4, 0x00, 0x00, /* Ring trip cycles = 2; RTHALFCYC = 1 */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0x1E,                   /* Ring Trip Method = DC; Threshold = 15.0 mA */
   0x09,                   /* Ringing Current Limit = 68 mA */
   0x1B,                   /* Tracked; Max Ringing Supply = 140 V */
   0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/* USA Ringing 20Hz 52Vrms */
const VpProfileDataType RING_ZL880_IB135V_70VRMS_50HZ_US[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x04, 0x00,       /* Ringing Profile for ZL88702 device */
   0x12,                   /* Profile length = 18 + 4 = 22 bytes */
   0x01,                   /* Profile format version = 1 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC0, 0x10, 0x00, 0x00, /* Ringing Generator Parameters: */
         0x00, 0x89, 0x52, /* 50.2 Hz Sine; 1.41 CF; 100.00 Vpk; 0.00 V bias */
         0xE7, 0x00, 0x00, /* Ring trip cycles = 2; RTHALFCYC = 1 */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0x1E,                   /* Ring Trip Method = DC; Threshold = 15.0 mA */
   0x09,                   /* Ringing Current Limit = 68 mA */
   0x13,                   /* Tracked; Max Ringing Supply = 100 V */
   0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/* USA Ringing 20Hz 52Vrms */
const VpProfileDataType RING_ZL880_IB135V_70VRMS_25HZ_US[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x04, 0x00,       /* Ringing Profile for ZL88702 device */
   0x12,                   /* Profile length = 18 + 4 = 22 bytes */
   0x01,                   /* Profile format version = 1 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC0, 0x10, 0x00, 0x00, /* Ringing Generator Parameters: */
         0x00, 0x45, 0x52, /* 25.3 Hz Sine; 1.41 CF; 100.00 Vpk; 0.00 V bias */
         0xE7, 0x00, 0x00, /* Ring trip cycles = 2; RTHALFCYC = 1 */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0x1E,                   /* Ring Trip Method = DC; Threshold = 15.0 mA */
   0x09,                   /* Ringing Current Limit = 68 mA */
   0x13,                   /* Tracked; Max Ringing Supply = 100 V */
   0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/* USA Ringing 20Hz 52Vrms */
const VpProfileDataType RING_ZL880_IB135V_70VRMS_20HZ_US[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x04, 0x00,       /* Ringing Profile for ZL88702 device */
   0x12,                   /* Profile length = 18 + 4 = 22 bytes */
   0x01,                   /* Profile format version = 1 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC0, 0x10, 0x00, 0x00, /* Ringing Generator Parameters: */
         0x00, 0x37, 0x52, /* 20.1 Hz Sine; 1.41 CF; 100.00 Vpk; 0.00 V bias */
         0xE7, 0x00, 0x00, /* Ring trip cycles = 2; RTHALFCYC = 1 */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0x1E,                   /* Ring Trip Method = DC; Threshold = 15.0 mA */
   0x09,                   /* Ringing Current Limit = 68 mA */
   0x13,                   /* Tracked; Max Ringing Supply = 100 V */
   0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/* USA Ringing 20Hz 52Vrms */
const VpProfileDataType RING_ZL880_IB135V_63VRMS_50HZ_US[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x04, 0x00,       /* Ringing Profile for ZL88702 device */
   0x12,                   /* Profile length = 18 + 4 = 22 bytes */
   0x01,                   /* Profile format version = 1 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC0, 0x10, 0x00, 0x00, /* Ringing Generator Parameters: */
         0x00, 0x89, 0x4A, /* 50.2 Hz Sine; 1.41 CF; 90.00 Vpk; 0.00 V bias */
         0x9D, 0x00, 0x00, /* Ring trip cycles = 2; RTHALFCYC = 1 */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0x1E,                   /* Ring Trip Method = DC; Threshold = 15.0 mA */
   0x09,                   /* Ringing Current Limit = 68 mA */
   0x11,                   /* Tracked; Max Ringing Supply = 90 V */
   0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/* USA Ringing 20Hz 52Vrms */
const VpProfileDataType RING_ZL880_IB135V_63VRMS_25HZ_US[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x04, 0x00,       /* Ringing Profile for ZL88702 device */
   0x12,                   /* Profile length = 18 + 4 = 22 bytes */
   0x01,                   /* Profile format version = 1 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC0, 0x10, 0x00, 0x00, /* Ringing Generator Parameters: */
         0x00, 0x45, 0x4A, /* 25.3 Hz Sine; 1.41 CF; 90.00 Vpk; 0.00 V bias */
         0x9D, 0x00, 0x00, /* Ring trip cycles = 2; RTHALFCYC = 1 */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0x1E,                   /* Ring Trip Method = DC; Threshold = 15.0 mA */
   0x09,                   /* Ringing Current Limit = 68 mA */
   0x11,                   /* Tracked; Max Ringing Supply = 90 V */
   0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/* USA Ringing 20Hz 52Vrms */
const VpProfileDataType RING_ZL880_IB135V_63VRMS_20HZ_US[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x04, 0x00,       /* Ringing Profile for ZL88702 device */
   0x12,                   /* Profile length = 18 + 4 = 22 bytes */
   0x01,                   /* Profile format version = 1 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC0, 0x10, 0x00, 0x00, /* Ringing Generator Parameters: */
         0x00, 0x37, 0x4A, /* 20.1 Hz Sine; 1.41 CF; 90.00 Vpk; 0.00 V bias */
         0x9D, 0x00, 0x00, /* Ring trip cycles = 2; RTHALFCYC = 1 */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0x1E,                   /* Ring Trip Method = DC; Threshold = 15.0 mA */
   0x09,                   /* Ringing Current Limit = 68 mA */
   0x11,                   /* Tracked; Max Ringing Supply = 90 V */
   0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/* USA Ringing 20Hz 52Vrms */
const VpProfileDataType RING_ZL880_IB135V_52VRMS_50HZ_US[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x04, 0x00,       /* Ringing Profile for ZL88702 device */
   0x12,                   /* Profile length = 18 + 4 = 22 bytes */
   0x01,                   /* Profile format version = 1 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC0, 0x10, 0x00, 0x00, /* Ringing Generator Parameters: */
         0x00, 0x89, 0x3E, /* 50.2 Hz Sine; 1.41 CF; 75.00 Vpk; 0.00 V bias */
         0x2D, 0x00, 0x00, /* Ring trip cycles = 2; RTHALFCYC = 1 */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0x1E,                   /* Ring Trip Method = DC; Threshold = 15.0 mA */
   0x09,                   /* Ringing Current Limit = 68 mA */
   0x0E,                   /* Tracked; Max Ringing Supply = 75 V */
   0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/* USA Ringing 20Hz 52Vrms */
const VpProfileDataType RING_ZL880_IB135V_52VRMS_25HZ_US[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x04, 0x00,       /* Ringing Profile for ZL88702 device */
   0x12,                   /* Profile length = 18 + 4 = 22 bytes */
   0x01,                   /* Profile format version = 1 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC0, 0x10, 0x00, 0x00, /* Ringing Generator Parameters: */
         0x00, 0x44, 0x3E, /* 24.9 Hz Sine; 1.41 CF; 75.00 Vpk; 0.00 V bias */
         0x2D, 0x00, 0x00, /* Ring trip cycles = 2; RTHALFCYC = 1 */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0x1E,                   /* Ring Trip Method = DC; Threshold = 15.0 mA */
   0x09,                   /* Ringing Current Limit = 68 mA */
   0x0E,                   /* Tracked; Max Ringing Supply = 75 V */
   0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/* USA Ringing 20Hz 52Vrms */
const VpProfileDataType RING_ZL880_IB135V_52VRMS_20HZ_US[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x04, 0x00,       /* Ringing Profile for ZL88702 device */
   0x12,                   /* Profile length = 18 + 4 = 22 bytes */
   0x01,                   /* Profile format version = 1 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xC0, 0x10, 0x00, 0x00, /* Ringing Generator Parameters: */
         0x00, 0x37, 0x3E, /* 20.1 Hz Sine; 1.41 CF; 75.00 Vpk; 0.00 V bias */
         0x2D, 0x00, 0x00, /* Ring trip cycles = 2; RTHALFCYC = 1 */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0x1E,                   /* Ring Trip Method = DC; Threshold = 15.0 mA */
   0x09,                   /* Ringing Current Limit = 68 mA */
   0x0E,                   /* Tracked; Max Ringing Supply = 75 V */
   0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/************** Call_Progress_Tones **************/

/* Caller ID Alert Tone (CAS) - Used in Many Country Caller ID Profiles */
const VpProfileDataType T_CAS_DEF[] =
{
/* Sig gen A = 2129.88 Hz at -8.00 dBm0 */
/* Sig gen B = 2750.24 Hz at -8.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x16, 0xB8, 0x23,
         0x80, 0x1D, 0x56,
         0x23, 0x80,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* ETSI ES201 970 Dial Tone  */
const VpProfileDataType T_DL_ETSI[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* ETSI ES201 970 Ringback Tone  */
const VpProfileDataType T_RB_ETSI[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* ETSI ES201 970 Busy Tone  */
const VpProfileDataType T_BS_ETSI[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* ETSI ES201 970 Call Waiting Tone */
const VpProfileDataType T_CW_ETSI[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* ETSI ES201 970 Congestion Tone */
const VpProfileDataType T_CG_ETSI[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Argentina Dial Tone  */
const VpProfileDataType T_DL_AR[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Argentina Ringback Tone  */
const VpProfileDataType T_RB_AR[] =
{
/* Sig gen A = 425.17 Hz at -10.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x1C,
         0x33, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Argentina Busy Tone  */
const VpProfileDataType T_BS_AR[] =
{
/* Sig gen A = 425.17 Hz at -10.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x1C,
         0x33, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Argentina Call Waiting Tone */
const VpProfileDataType T_CW_AR[] =
{
/* Sig gen A = 425.17 Hz at -7.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x27,
         0xD5, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Argentina Congestion Tone */
const VpProfileDataType T_CG_AR[] =
{
/* Sig gen A = 425.17 Hz at -10.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x1C,
         0x33, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Austria Dial Tone  */
const VpProfileDataType T_DL_AT[] =
{
/* Sig gen A = 420.04 Hz at -10.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x7B, 0x1C,
         0x33, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Austria Ringback Tone  */
const VpProfileDataType T_RB_AT[] =
{
/* Sig gen A = 420.04 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x7B, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Austria Busy Tone  */
const VpProfileDataType T_BS_AT[] =
{
/* Sig gen A = 420.04 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x7B, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Austria Call Waiting Tone */
const VpProfileDataType T_CW_AT[] =
{
/* Sig gen A = 420.04 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x7B, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Austria Congestion Tone */
const VpProfileDataType T_CG_AT[] =
{
/* Sig gen A = 420.04 Hz at -10.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x7B, 0x1C,
         0x33, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Australia Dial Tone  */
const VpProfileDataType T_DL_AU[] =
{
/* Sig gen A = 425.17 Hz at -10.00 dBm0 */
/* Sig gen B = 450.07 Hz at -16.00 dBm0 */
/* Sig gen C = 399.90 Hz at -16.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x18,                   /* Profile length = 24 + 4 = 28 bytes */
   0x02,                   /* Profile format version = 2 */
   0x15,                   /* MPI section length = 21 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x1C,
         0x33, 0x04, 0xCD,
         0x0E, 0x22,
   0xD4, 0x04, 0x44, 0x0E, /* Signal Generator C, D Parameters */
         0x22, 0x00, 0x00,
         0x59, 0x2B,
/* Formatted Parameters Section -------------------------------------------- */
   0x07                    /* FM_EN = 0; WB = 0; Active Generators: A B C */
};

/* Australia Ringback Tone  */
const VpProfileDataType T_RB_AU[] =
{
/* Sig gen A = 425.17 Hz at -10.00 dBm0 */
/* Sig gen B = 450.07 Hz at -16.00 dBm0 */
/* Sig gen C = 399.90 Hz at -16.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x18,                   /* Profile length = 24 + 4 = 28 bytes */
   0x02,                   /* Profile format version = 2 */
   0x15,                   /* MPI section length = 21 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x1C,
         0x33, 0x04, 0xCD,
         0x0E, 0x22,
   0xD4, 0x04, 0x44, 0x0E, /* Signal Generator C, D Parameters */
         0x22, 0x00, 0x00,
         0x59, 0x2B,
/* Formatted Parameters Section -------------------------------------------- */
   0x07                    /* FM_EN = 0; WB = 0; Active Generators: A B C */
};

/* Australia Busy Tone  */
const VpProfileDataType T_BS_AU[] =
{
/* Sig gen A = 425.17 Hz at -13.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x13,
         0xF7, 0x00, 0x00,
         0x59, 0x2B,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Australia Call Waiting Tone */
const VpProfileDataType T_CW_AU[] =
{
/* Sig gen A = 425.17 Hz at -23.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x06,
         0x50, 0x00, 0x00,
         0x59, 0x2B,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Australia Congestion Tone  */
const VpProfileDataType T_CG_AU[] =
{
/* Sig gen A = 425.17 Hz at -13.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x13,
         0xF7, 0x00, 0x00,
         0x05, 0x04,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Belgium Dial Tone  */
const VpProfileDataType T_DL_BE[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Belgium Ringback Tone  */
const VpProfileDataType T_RB_BE[] =
{
/* Sig gen A = 425.17 Hz at -10.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x1C,
         0x33, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Belgium Busy Tone  */
const VpProfileDataType T_BS_BE[] =
{
/* Sig gen A = 425.17 Hz at -12.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x16,
         0x66, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Belgium Call Waiting Tone */
const VpProfileDataType T_CW_BE[] =
{
/* Sig gen A = 1400.02 Hz at -7.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x0E, 0xEF, 0x27,
         0xD5, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Belgium Congestion Tone */
const VpProfileDataType T_CG_BE[] =
{
/* Sig gen A = 425.17 Hz at -15.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x0F,
         0xDC, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Bulgaria Dial Tone  */
const VpProfileDataType T_DL_BG[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Bulgaria Ringback Tone  */
const VpProfileDataType T_RG_BG[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Bulgaria Busy Tone  */
const VpProfileDataType T_BS_BG[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Bulgaria Call Waiting Tone  */
const VpProfileDataType T_CW_BG[] =
{
/* Sig gen A = 440.19 Hz at -7.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xB2, 0x27,
         0xD5, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Bulgaria Congestion Tone */
const VpProfileDataType T_CG_BG[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Brazil Dial Tone  */
const VpProfileDataType T_DL_BR[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Brazil Ringback Tone  */
const VpProfileDataType T_RB_BR[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Brazil Busy Tone  */
const VpProfileDataType T_BS_BR[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Brazil Call Waiting Tone */
const VpProfileDataType T_CW_BR[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Canada Dial Tone  */
const VpProfileDataType T_DL_CA[] =
{
/* Sig gen A = 350.10 Hz at -7.00 dBm0 */
/* Sig gen B = 440.19 Hz at -7.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x03, 0xBC, 0x27,
         0xD5, 0x04, 0xB2,
         0x27, 0xD5,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* Canada Ringback Tone  */
const VpProfileDataType T_RB_CA[] =
{
/* Sig gen A = 440.19 Hz at -13.00 dBm0 */
/* Sig gen B = 480.10 Hz at -13.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xB2, 0x13,
         0xF7, 0x05, 0x1F,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* Canada Busy Tone  */
const VpProfileDataType T_BS_CA[] =
{
/* Sig gen A = 480.10 Hz at -18.00 dBm0 */
/* Sig gen B = 620.00 Hz at -18.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x05, 0x1F, 0x0B,
         0x3A, 0x06, 0x9D,
         0x0B, 0x3A,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* Canada Call Waiting Tone */
const VpProfileDataType T_CW_CA[] =
{
/* Sig gen A = 440.19 Hz at -7.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xB2, 0x27,
         0xD5, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Canada Congestion Tone */
const VpProfileDataType T_CG_CA[] =
{
/* Sig gen A = 480.10 Hz at -18.00 dBm0 */
/* Sig gen B = 620.00 Hz at -18.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x05, 0x1F, 0x0B,
         0x3A, 0x06, 0x9D,
         0x0B, 0x3A,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* Switzerland Dial Tone  */
const VpProfileDataType T_DL_CH[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Switzerland Ringback Tone  */
const VpProfileDataType T_RB_CH[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Switzerland Busy Tone  */
const VpProfileDataType T_BS_CH[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Switzerland Call Waiting Tone */
const VpProfileDataType T_CW_CH[] =
{
/* Sig gen A = 425.17 Hz at -12.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x16,
         0x66, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Switzerland Congestion Tone */
const VpProfileDataType T_CG_CH[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Chile Dial Tone  */
const VpProfileDataType T_DL_CL[] =
{
/* Sig gen A = 399.90 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Chile Ringback Tone  */
const VpProfileDataType T_RG_CL[] =
{
/* Sig gen A = 399.90 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Chile Busy Tone  */
const VpProfileDataType T_BS_CL[] =
{
/* Sig gen A = 399.90 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Chile Call Waiting Tone  */
const VpProfileDataType T_CW_CL[] =
{
/* Sig gen A = 900.15 Hz at -8.00 dBm0 */
/* Sig gen B = 1300.05 Hz at -8.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x09, 0x9A, 0x23,
         0x80, 0x0D, 0xDE,
         0x23, 0x80,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* Chile Congestion Tone */
const VpProfileDataType T_CG_CL[] =
{
/* Sig gen A = 399.90 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* China Dial Tone  */
const VpProfileDataType T_DL_CN[] =
{
/* Sig gen A = 450.07 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xCD, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* China Ringback Tone  */
const VpProfileDataType T_RB_CN[] =
{
/* Sig gen A = 450.07 Hz at -10.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xCD, 0x1C,
         0x33, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* China Busy Tone  */
const VpProfileDataType T_BS_CN[] =
{
/* Sig gen A = 450.07 Hz at -12.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xCD, 0x16,
         0x66, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* China Call Waiting Tone */
const VpProfileDataType T_CW_CN[] =
{
/* Sig gen A = 450.07 Hz at -7.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xCD, 0x27,
         0xD5, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* China Congestion Tone */
const VpProfileDataType T_CG_CN[] =
{
/* Sig gen A = 450.07 Hz at -15.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xCD, 0x0F,
         0xDC, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Czech Republic Dial Tone  */
const VpProfileDataType T_DL_CZ[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Czech Republic Ringback Tone  */
const VpProfileDataType T_RG_CZ[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Czech Republic Busy Tone  */
const VpProfileDataType T_BS_CZ[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Czech Republic Call Waiting Tone  */
const VpProfileDataType T_CW_CZ[] =
{
/* Sig gen A = 425.17 Hz at -7.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x27,
         0xD5, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Czech Republic Congestion Tone */
const VpProfileDataType T_CG_CZ[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Germany Dial Tone  */
const VpProfileDataType T_DL_DE[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Germany Ringback Tone */
const VpProfileDataType T_RB_DE[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Germany Busy Tone */
const VpProfileDataType T_BS_DE[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Germany Call Waiting Tone */
const VpProfileDataType T_CW_DE[] =
{
/* Sig gen A = 425.17 Hz at -7.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x27,
         0xD5, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Germany Congestion Tone  */
const VpProfileDataType T_CG_DE[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Denmark Dial Tone  */
const VpProfileDataType T_DL_DK[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Denmark Ringback Tone  */
const VpProfileDataType T_RG_DK[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Denmark Busy Tone  */
const VpProfileDataType T_BS_DK[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Denmark Call Waiting Tone  */
const VpProfileDataType T_CW_DK[] =
{
/* Sig gen A = 440.19 Hz at -7.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xB2, 0x27,
         0xD5, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Denmark Congestion Tone */
const VpProfileDataType T_CG_DK[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Ecuador Dial Tone  */
const VpProfileDataType T_DL_EC[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Ecuador Ringback Tone  */
const VpProfileDataType T_RG_EC[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Ecuador Busy Tone  */
const VpProfileDataType T_BS_EC[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Ecuador Call Waiting Tone  */
const VpProfileDataType T_CW_EC[] =
{
/* Sig gen A = 425.17 Hz at -7.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x27,
         0xD5, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Ecuador Congestion Tone */
const VpProfileDataType T_CG_EC[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Spain Dial Tone  */
const VpProfileDataType T_DL_ES[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Spain Ringback Tone  */
const VpProfileDataType T_RB_ES[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Spain Busy Tone  */
const VpProfileDataType T_BS_ES[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Spain Call Waiting Tone */
const VpProfileDataType T_CW_ES[] =
{
/* Sig gen A = 425.17 Hz at -14.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x11,
         0xCB, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Spain Congestion Tone */
const VpProfileDataType T_CG_ES[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Finland Dial Tone  */
const VpProfileDataType T_DL_FI[] =
{
/* Sig gen A = 425.17 Hz at -8.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x23,
         0x80, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Finland Ringback Tone  */
const VpProfileDataType T_RG_FI[] =
{
/* Sig gen A = 425.17 Hz at -8.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x23,
         0x80, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Finland Busy Tone  */
const VpProfileDataType T_BS_FI[] =
{
/* Sig gen A = 425.17 Hz at -8.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x23,
         0x80, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Finland Call Waiting Tone  */
const VpProfileDataType T_CW_FI[] =
{
/* Sig gen A = 425.17 Hz at -15.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x0F,
         0xDC, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Finland Congestion Tone */
const VpProfileDataType T_CG_FI[] =
{
/* Sig gen A = 425.17 Hz at -8.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x23,
         0x80, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* France Dial Tone  */
const VpProfileDataType T_DL_FR[] =
{
/* Sig gen A = 440.19 Hz at -4.50 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xB2, 0x35,
         0x1D, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* France Ringback Tone  */
const VpProfileDataType T_RG_FR[] =
{
/* Sig gen A = 440.19 Hz at -9.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xB2, 0x1F,
         0xA4, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* France Busy Tone  */
const VpProfileDataType T_BS_FR[] =
{
/* Sig gen A = 440.19 Hz at -9.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xB2, 0x1F,
         0xA4, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* France Call Waiting Tone  */
const VpProfileDataType T_CW_FR[] =
{
/* Sig gen A = 440.19 Hz at -9.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xB2, 0x1F,
         0xA4, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* UK Dial Tone  */
const VpProfileDataType T_DL_GB[] =
{
/* Sig gen A = 350.10 Hz at -7.00 dBm0 */
/* Sig gen B = 440.19 Hz at -7.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x03, 0xBC, 0x27,
         0xD5, 0x04, 0xB2,
         0x27, 0xD5,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* UK Ringback Tone  */
const VpProfileDataType T_RB_GB[] =
{
/* Sig gen A = 440.19 Hz at -12.00 dBm0 */
/* Sig gen B = 450.07 Hz at -12.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xB2, 0x16,
         0x66, 0x04, 0xCD,
         0x16, 0x66,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* UK Busy Tone  */
const VpProfileDataType T_BS_GB[] =
{
/* Sig gen A = 399.90 Hz at -12.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x16,
         0x66, 0x00, 0x00,
         0x59, 0x2B,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* UK Call Waiting Tone */
const VpProfileDataType T_CW_GB[] =
{
/* Sig gen A = 399.90 Hz at -8.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x23,
         0x80, 0x00, 0x00,
         0x59, 0x2B,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* UK Congestion Tone  */
const VpProfileDataType T_CG_GB[] =
{
/* Sig gen A = 399.90 Hz at -15.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x0F,
         0xDC, 0x00, 0x00,
         0x59, 0x2B,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Greece Dial Tone  */
const VpProfileDataType T_DL_GR[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Greece Ringback Tone  */
const VpProfileDataType T_RG_GR[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Greece Busy Tone  */
const VpProfileDataType T_BS_GR[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Greece Call Waiting Tone  */
const VpProfileDataType T_CW_GR[] =
{
/* Sig gen A = 425.17 Hz at -7.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x27,
         0xD5, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Greece Congestion Tone */
const VpProfileDataType T_CG_GR[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Hong Kong SAR Dial Tone  */
const VpProfileDataType T_DL_HK[] =
{
/* Sig gen A = 350.10 Hz at -10.00 dBm0 */
/* Sig gen B = 440.19 Hz at -10.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x03, 0xBC, 0x1C,
         0x33, 0x04, 0xB2,
         0x1C, 0x33,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* Hong Kong SAR Ringback Tone  */
const VpProfileDataType T_RB_HK[] =
{
/* Sig gen A = 440.19 Hz at -10.00 dBm0 */
/* Sig gen B = 480.10 Hz at -10.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xB2, 0x1C,
         0x33, 0x05, 0x1F,
         0x1C, 0x33,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* Hong Kong SAR Busy Tone  */
const VpProfileDataType T_BS_HK[] =
{
/* Sig gen A = 480.10 Hz at -10.00 dBm0 */
/* Sig gen B = 620.00 Hz at -10.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x05, 0x1F, 0x1C,
         0x33, 0x06, 0x9D,
         0x1C, 0x33,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* Hong Kong SAR Call Waiting Tone */
const VpProfileDataType T_CW_HK[] =
{
/* Sig gen A = 440.19 Hz at -10.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xB2, 0x1C,
         0x33, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Hong Kong SAR Congestion Tone */
const VpProfileDataType T_CG_HK[] =
{
/* Sig gen A = 480.10 Hz at -10.00 dBm0 */
/* Sig gen B = 620.00 Hz at -10.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x05, 0x1F, 0x1C,
         0x33, 0x06, 0x9D,
         0x1C, 0x33,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* Hungary Dial Tone  */
const VpProfileDataType T_DL_HU[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Hungary Ringback Tone  */
const VpProfileDataType T_RG_HU[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Hungary Busy Tone  */
const VpProfileDataType T_BS_HU[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Hungary Call Waiting Tone  */
const VpProfileDataType T_CW_HU[] =
{
/* Sig gen A = 425.17 Hz at -7.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x27,
         0xD5, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Hungary Congestion Tone */
const VpProfileDataType T_CG_HU[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Indonesia Dial Tone  */
const VpProfileDataType T_DL_ID[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Indonesia Ringback Tone  */
const VpProfileDataType T_RG_ID[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Indonesia Busy Tone  */
const VpProfileDataType T_BS_ID[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Indonesia Call Waiting Tone  */
const VpProfileDataType T_CW_ID[] =
{
/* Sig gen A = 425.17 Hz at -7.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x27,
         0xD5, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Indonesia Congestion Tone */
const VpProfileDataType T_CG_ID[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Ireland Dial Tone  */
const VpProfileDataType T_DL_IE[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Ireland Ringback Tone  */
const VpProfileDataType T_RB_IE[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Ireland Busy Tone  */
const VpProfileDataType T_BS_IE[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Ireland Call Waiting Tone */
const VpProfileDataType T_CW_IE[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Israel Dial Tone  */
const VpProfileDataType T_DL_IL[] =
{
/* Sig gen A = 399.90 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Israel Ringback Tone  */
const VpProfileDataType T_RB_IL[] =
{
/* Sig gen A = 399.90 Hz at -10.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x1C,
         0x33, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Israel Busy Tone  */
const VpProfileDataType T_BS_IL[] =
{
/* Sig gen A = 399.90 Hz at -12.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x16,
         0x66, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Israel Call Waiting Tone */
const VpProfileDataType T_CW_IL[] =
{
/* Sig gen A = 440.19 Hz at -7.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xB2, 0x27,
         0xD5, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Israel Congestion Tone (SIT) */
const VpProfileDataType T_CG_IL[] =
{
/* Sig gen A = 1000.12 Hz at -15.00 dBm0 */
/* Sig gen B = 1400.02 Hz at -15.00 dBm0 */
/* Sig gen C = 1799.93 Hz at -15.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x18,                   /* Profile length = 24 + 4 = 28 bytes */
   0x02,                   /* Profile format version = 2 */
   0x15,                   /* MPI section length = 21 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x0A, 0xAB, 0x0F,
         0xDC, 0x0E, 0xEF,
         0x0F, 0xDC,
   0xD4, 0x13, 0x33, 0x0F, /* Signal Generator C, D Parameters */
         0xDC, 0x00, 0x00,
         0x59, 0x2B,
/* Formatted Parameters Section -------------------------------------------- */
   0x07                    /* FM_EN = 0; WB = 0; Active Generators: A B C */
};

/* India Dial Tone  */
const VpProfileDataType T_DL_IN[] =
{
/* Sig gen A = 425.17 Hz at -6.00 dBm0 */
/* Sig gen B = 450.07 Hz at -12.00 dBm0 */
/* Sig gen C = 399.90 Hz at -12.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x18,                   /* Profile length = 24 + 4 = 28 bytes */
   0x02,                   /* Profile format version = 2 */
   0x15,                   /* MPI section length = 21 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x2C,
         0xB1, 0x04, 0xCD,
         0x16, 0x66,
   0xD4, 0x04, 0x44, 0x16, /* Signal Generator C, D Parameters */
         0x66, 0x00, 0x00,
         0x59, 0x2B,
/* Formatted Parameters Section -------------------------------------------- */
   0x07                    /* FM_EN = 0; WB = 0; Active Generators: A B C */
};

/* India Ringback Tone  */
const VpProfileDataType T_RB_IN[] =
{
/* Sig gen A = 425.17 Hz at -14.00 dBm0 */
/* Sig gen B = 450.07 Hz at -20.00 dBm0 */
/* Sig gen C = 399.90 Hz at -20.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x18,                   /* Profile length = 24 + 4 = 28 bytes */
   0x02,                   /* Profile format version = 2 */
   0x15,                   /* MPI section length = 21 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x11,
         0xCB, 0x04, 0xCD,
         0x08, 0xEB,
   0xD4, 0x04, 0x44, 0x08, /* Signal Generator C, D Parameters */
         0xEB, 0x00, 0x00,
         0x59, 0x2B,
/* Formatted Parameters Section -------------------------------------------- */
   0x07                    /* FM_EN = 0; WB = 0; Active Generators: A B C */
};

/* India Busy Tone  */
const VpProfileDataType T_BS_IN[] =
{
/* Sig gen A = 399.90 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x38,
         0x43, 0x00, 0x00,
         0x59, 0x2B,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* India Call Waiting Tone */
const VpProfileDataType T_CW_IN[] =
{
/* Sig gen A = 399.90 Hz at -7.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x27,
         0xD5, 0x00, 0x00,
         0x59, 0x2B,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* India Congestion Tone  */
const VpProfileDataType T_CG_IN[] =
{
/* Sig gen A = 399.90 Hz at -9.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x1F,
         0xA4, 0x00, 0x00,
         0x05, 0x04,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Iceland Dial Tone  */
const VpProfileDataType T_DL_IS[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Iceland Ringback Tone  */
const VpProfileDataType T_RG_IS[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Iceland Busy Tone  */
const VpProfileDataType T_BS_IS[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Iceland Call Waiting Tone  */
const VpProfileDataType T_CW_IS[] =
{
/* Sig gen A = 425.17 Hz at -7.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x27,
         0xD5, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Iceland Congestion Tone */
const VpProfileDataType T_CG_IS[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Italy Dial Tone  */
const VpProfileDataType T_DL_IT[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Italy Ringback Tone */
const VpProfileDataType T_RB_IT[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Italy Busy Tone */
const VpProfileDataType T_BS_IT[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Italy Call Waiting Tone */
const VpProfileDataType T_CW_IT[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Italy Congestion Tone  */
const VpProfileDataType T_CG_IT[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Japan Dial Tone  */
const VpProfileDataType T_DL_JP[] =
{
/* Sig gen A = 399.90 Hz at -9.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x1F,
         0xA4, 0x00, 0x00,
         0x59, 0x2B,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Japan Ringback Tone  */
const VpProfileDataType T_RB_JP[] =
{
/* Sig gen A = 440.19 Hz at -9.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xB2, 0x1F,
         0xA4, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Japan Busy Tone  */
const VpProfileDataType T_BS_JP[] =
{
/* Sig gen A = 399.90 Hz at -9.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x1F,
         0xA4, 0x00, 0x00,
         0x59, 0x2B,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Japan Call Waiting Tone */
const VpProfileDataType T_CW_JP[] =
{
/* Sig gen A = 399.90 Hz at -8.00 dBm0 */
/* Sig gen B = 416.02 Hz at -14.00 dBm0 */
/* Sig gen C = 384.16 Hz at -14.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x18,                   /* Profile length = 24 + 4 = 28 bytes */
   0x02,                   /* Profile format version = 2 */
   0x15,                   /* MPI section length = 21 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x23,
         0x80, 0x04, 0x70,
         0x11, 0xCB,
   0xD4, 0x04, 0x19, 0x11, /* Signal Generator C, D Parameters */
         0xCB, 0x00, 0x00,
         0x59, 0x2B,
/* Formatted Parameters Section -------------------------------------------- */
   0x07                    /* FM_EN = 0; WB = 0; Active Generators: A B C */
};

/* Japan Caller ID Alert Tone - Digit C */
const VpProfileDataType T_AC_JP[] =
{
/* Sig gen A = 852.17 Hz at -8.00 dBm0 */
/* Sig gen B = 1632.93 Hz at -6.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x09, 0x17, 0x23,
         0x80, 0x11, 0x6B,
         0x2C, 0xB1,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* Japan Caller ID Alert Tone - Digit D */
const VpProfileDataType T_AD_JP[] =
{
/* Sig gen A = 941.16 Hz at -8.00 dBm0 */
/* Sig gen B = 1632.93 Hz at -6.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x0A, 0x0A, 0x23,
         0x80, 0x11, 0x6B,
         0x2C, 0xB1,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* S. Korea Dial Tone  */
const VpProfileDataType T_DL_KR[] =
{
/* Sig gen A = 350.10 Hz at -7.00 dBm0 */
/* Sig gen B = 440.19 Hz at -7.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x03, 0xBC, 0x27,
         0xD5, 0x04, 0xB2,
         0x27, 0xD5,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* S. Korea Ringback Tone  */
const VpProfileDataType T_RB_KR[] =
{
/* Sig gen A = 440.19 Hz at -13.00 dBm0 */
/* Sig gen B = 480.10 Hz at -13.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xB2, 0x13,
         0xF7, 0x05, 0x1F,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* S. Korea Busy Tone  */
const VpProfileDataType T_BS_KR[] =
{
/* Sig gen A = 480.10 Hz at -15.00 dBm0 */
/* Sig gen B = 620.00 Hz at -15.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x05, 0x1F, 0x0F,
         0xDC, 0x06, 0x9D,
         0x0F, 0xDC,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* S. Korea Call Waiting Tone */
const VpProfileDataType T_CW_KR[] =
{
/* Sig gen A = 350.10 Hz at -10.00 dBm0 */
/* Sig gen B = 440.19 Hz at -10.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x03, 0xBC, 0x1C,
         0x33, 0x04, 0xB2,
         0x1C, 0x33,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* S. Korea Congestion Tone */
const VpProfileDataType T_CG_KR[] =
{
/* Sig gen A = 480.10 Hz at -17.00 dBm0 */
/* Sig gen B = 620.00 Hz at -17.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x05, 0x1F, 0x0C,
         0x99, 0x06, 0x9D,
         0x0C, 0x99,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* Mexico Dial Tone  */
const VpProfileDataType T_DL_MX[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Mexico Ringback Tone  */
const VpProfileDataType T_RG_MX[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Mexico Busy Tone  */
const VpProfileDataType T_BS_MX[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Mexico Congestion Tone */
const VpProfileDataType T_CG_MX[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Malaysia Dial Tone  */
const VpProfileDataType T_DL_MY[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Malaysia Ringback Tone */
const VpProfileDataType T_RB_MY[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Malaysia Busy Tone */
const VpProfileDataType T_BS_MY[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Malaysia Congestion Tone  */
const VpProfileDataType T_CG_MY[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Netherlands Dial Tone  */
const VpProfileDataType T_DL_NL[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Netherlands Ringback Tone  */
const VpProfileDataType T_RB_NL[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Netherlands Busy Tone  */
const VpProfileDataType T_BS_NL[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Netherlands Call Waiting Tone */
const VpProfileDataType T_CW_NL[] =
{
/* Sig gen A = 425.17 Hz at -9.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x1F,
         0xA4, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Netherlands Congestion Tone */
const VpProfileDataType T_CG_NL[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Norway Dial Tone  */
const VpProfileDataType T_DL_NO[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Norway Ringback Tone  */
const VpProfileDataType T_RB_NO[] =
{
/* Sig gen A = 425.17 Hz at -9.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x1F,
         0xA4, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Norway Busy Tone  */
const VpProfileDataType T_BS_NO[] =
{
/* Sig gen A = 425.17 Hz at -9.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x1F,
         0xA4, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Norway Call Waiting Tone */
const VpProfileDataType T_CW_NO[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Norway Congestion Tone */
const VpProfileDataType T_CG_NO[] =
{
/* Sig gen A = 425.17 Hz at -9.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x1F,
         0xA4, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* New Zealand Dial Tone  */
const VpProfileDataType T_DL_NZ[] =
{
/* Sig gen A = 399.90 Hz at -9.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x1F,
         0xA4, 0x00, 0x00,
         0x1C, 0x33,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* New Zealand Ringback Tone  */
const VpProfileDataType T_RB_NZ[] =
{
/* Sig gen A = 399.90 Hz at -12.00 dBm0 */
/* Sig gen B = 450.07 Hz at -12.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x16,
         0x66, 0x04, 0xCD,
         0x16, 0x66,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* New Zealand Busy Tone  */
const VpProfileDataType T_BS_NZ[] =
{
/* Sig gen A = 399.90 Hz at -9.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x1F,
         0xA4, 0x00, 0x00,
         0x59, 0x2B,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* New Zealand Call Waiting Tone */
const VpProfileDataType T_CW_NZ[] =
{
/* Sig gen A = 399.90 Hz at -12.00 dBm0 */
/* Sig gen B = 450.07 Hz at -12.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x16,
         0x66, 0x04, 0xCD,
         0x16, 0x66,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* New Zealand Congestion Tone  */
const VpProfileDataType T_CG_NZ[] =
{
/* Sig gen A = 399.90 Hz at -9.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x1F,
         0xA4, 0x00, 0x00,
         0x59, 0x2B,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Poland Dial Tone  */
const VpProfileDataType T_DL_PL[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Poland Ringback Tone  */
const VpProfileDataType T_RG_PL[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Poland Busy Tone  */
const VpProfileDataType T_BS_PL[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Poland Call Waiting Tone  */
const VpProfileDataType T_CW_PL[] =
{
/* Sig gen A = 425.17 Hz at -7.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x27,
         0xD5, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Poland Congestion Tone */
const VpProfileDataType T_CG_PL[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Portugal Dial Tone  */
const VpProfileDataType T_DL_PT[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Portugal Ringback Tone  */
const VpProfileDataType T_RB_PT[] =
{
/* Sig gen A = 425.17 Hz at -8.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x23,
         0x80, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Portugal Busy Tone  */
const VpProfileDataType T_BS_PT[] =
{
/* Sig gen A = 425.17 Hz at -8.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x23,
         0x80, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Portugal Call Waiting Tone */
const VpProfileDataType T_CW_PT[] =
{
/* Sig gen A = 425.17 Hz at -12.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x16,
         0x66, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Portugal Congestion Tone */
const VpProfileDataType T_CG_PT[] =
{
/* Sig gen A = 425.17 Hz at -8.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x23,
         0x80, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Russia Dial Tone  */
const VpProfileDataType T_DL_RU[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Russia Ringback Tone  */
const VpProfileDataType T_RG_RU[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Russia Busy Tone  */
const VpProfileDataType T_BS_RU[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Russia Call Waiting Tone  */
const VpProfileDataType T_CW_RU[] =
{
/* Sig gen A = 425.17 Hz at -9.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x1F,
         0xA4, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Russia Congestion Tone */
const VpProfileDataType T_CG_RU[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Sweden Dial Tone  */
const VpProfileDataType T_DL_SE[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Sweden Ringback Tone  */
const VpProfileDataType T_RB_SE[] =
{
/* Sig gen A = 425.17 Hz at -9.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x1F,
         0xA4, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Sweden Busy Tone  */
const VpProfileDataType T_BS_SE[] =
{
/* Sig gen A = 425.17 Hz at -9.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x1F,
         0xA4, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Sweden Call Waiting Tone */
const VpProfileDataType T_CW_SE[] =
{
/* Sig gen A = 425.17 Hz at -7.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x27,
         0xD5, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Sweden Congestion Tone */
const VpProfileDataType T_CG_SE[] =
{
/* Sig gen A = 425.17 Hz at -9.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x1F,
         0xA4, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Singapore Dial Tone  */
const VpProfileDataType T_DL_SG[] =
{
/* Sig gen A = 425.17 Hz at -9.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x1F,
         0xA4, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Singapore Ringback Tone */
const VpProfileDataType T_RB_SG[] =
{
/* Sig gen A = 425.17 Hz at -7.00 dBm0 */
/* Sig gen B = 448.97 Hz at -13.00 dBm0 */
/* Sig gen C = 401.00 Hz at -13.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x18,                   /* Profile length = 24 + 4 = 28 bytes */
   0x02,                   /* Profile format version = 2 */
   0x15,                   /* MPI section length = 21 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x27,
         0xD5, 0x04, 0xCA,
         0x13, 0xF7,
   0xD4, 0x04, 0x47, 0x13, /* Signal Generator C, D Parameters */
         0xF7, 0x00, 0x00,
         0x59, 0x2B,
/* Formatted Parameters Section -------------------------------------------- */
   0x07                    /* FM_EN = 0; WB = 0; Active Generators: A B C */
};

/* Singapore Busy Tone */
const VpProfileDataType T_BS_SG[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Singapore Call Waiting Tone */
const VpProfileDataType T_CW_SG[] =
{
/* Sig gen A = 425.17 Hz at -9.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x1F,
         0xA4, 0x00, 0x00,
         0x0F, 0xDC,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Singapore Congestion Tone  */
const VpProfileDataType T_CG_SG[] =
{
/* Sig gen A = 425.17 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x89, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Thailand Dial Tone  */
const VpProfileDataType T_DL_TH[] =
{
/* Sig gen A = 399.90 Hz at -8.00 dBm0 */
/* Sig gen B = 450.07 Hz at -14.00 dBm0 */
/* Sig gen C = 350.10 Hz at -14.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x18,                   /* Profile length = 24 + 4 = 28 bytes */
   0x02,                   /* Profile format version = 2 */
   0x15,                   /* MPI section length = 21 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x23,
         0x80, 0x04, 0xCD,
         0x11, 0xCB,
   0xD4, 0x03, 0xBC, 0x11, /* Signal Generator C, D Parameters */
         0xCB, 0x00, 0x00,
         0x59, 0x2B,
/* Formatted Parameters Section -------------------------------------------- */
   0x07                    /* FM_EN = 0; WB = 0; Active Generators: A B C */
};

/* Thailand Ringback Tone  */
const VpProfileDataType T_RG_TH[] =
{
/* Sig gen A = 399.90 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Thailand Busy Tone  */
const VpProfileDataType T_BS_TH[] =
{
/* Sig gen A = 399.90 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Thailand Call Waiting Tone  */
const VpProfileDataType T_CW_TH[] =
{
/* Sig gen A = 399.90 Hz at -7.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x27,
         0xD5, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Thailand Congestion Tone */
const VpProfileDataType T_CG_TH[] =
{
/* Sig gen A = 399.90 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Turkey Dial Tone  */
const VpProfileDataType T_DL_TK[] =
{
/* Sig gen A = 450.07 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xCD, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Turkey Ringback Tone  */
const VpProfileDataType T_RG_TK[] =
{
/* Sig gen A = 450.07 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xCD, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Turkey Busy Tone  */
const VpProfileDataType T_BS_TK[] =
{
/* Sig gen A = 450.07 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xCD, 0x38,
         0x43, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Turkey Call Waiting Tone  */
const VpProfileDataType T_CW_TK[] =
{
/* Sig gen A = 450.07 Hz at -7.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xCD, 0x27,
         0xD5, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Turkey Congestion Tone */
const VpProfileDataType T_CG_TK[] =
{
/* Sig gen A = 450.07 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xCD, 0x38,
         0x43, 0x00, 0x00,
         0x0E, 0x22,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* Taiwan Dial Tone  */
const VpProfileDataType T_DL_TW[] =
{
/* Sig gen A = 350.10 Hz at -7.00 dBm0 */
/* Sig gen B = 440.19 Hz at -7.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x03, 0xBC, 0x27,
         0xD5, 0x04, 0xB2,
         0x27, 0xD5,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* Taiwan Ringback Tone  */
const VpProfileDataType T_RB_TW[] =
{
/* Sig gen A = 440.19 Hz at -13.00 dBm0 */
/* Sig gen B = 480.10 Hz at -13.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xB2, 0x13,
         0xF7, 0x05, 0x1F,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* Taiwan Busy Tone  */
const VpProfileDataType T_BS_TW[] =
{
/* Sig gen A = 480.10 Hz at -15.00 dBm0 */
/* Sig gen B = 620.00 Hz at -15.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x05, 0x1F, 0x0F,
         0xDC, 0x06, 0x9D,
         0x0F, 0xDC,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* Taiwan Call Waiting Tone */
const VpProfileDataType T_CW_TW[] =
{
/* Sig gen A = 440.19 Hz at -10.00 dBm0 */
/* Sig gen B = 480.10 Hz at -10.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xB2, 0x1C,
         0x33, 0x05, 0x1F,
         0x1C, 0x33,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* Taiwan Congestion Tone */
const VpProfileDataType T_CG_TW[] =
{
/* Sig gen A = 440.19 Hz at -17.00 dBm0 */
/* Sig gen B = 480.10 Hz at -17.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xB2, 0x0C,
         0x99, 0x05, 0x1F,
         0x0C, 0x99,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* USA Dial Tone  */
const VpProfileDataType T_DL_US[] =
{
/* Sig gen A = 350.10 Hz at -7.00 dBm0 */
/* Sig gen B = 440.19 Hz at -7.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x03, 0xBC, 0x27,
         0xD5, 0x04, 0xB2,
         0x27, 0xD5,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* USA Ringback Tone  */
const VpProfileDataType T_RB_US[] =
{
/* Sig gen A = 440.19 Hz at -13.00 dBm0 */
/* Sig gen B = 480.10 Hz at -13.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xB2, 0x13,
         0xF7, 0x05, 0x1F,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* USA Busy Tone  */
const VpProfileDataType T_BS_US[] =
{
/* Sig gen A = 480.10 Hz at -18.00 dBm0 */
/* Sig gen B = 620.00 Hz at -18.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x05, 0x1F, 0x0B,
         0x3A, 0x06, 0x9D,
         0x0B, 0x3A,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* USA Call Waiting Tone */
const VpProfileDataType T_CW_US[] =
{
/* Sig gen A = 440.19 Hz at -7.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0xB2, 0x27,
         0xD5, 0x00, 0x00,
         0x13, 0xF7,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* USA Congestion Tone */
const VpProfileDataType T_CG_US[] =
{
/* Sig gen A = 480.10 Hz at -18.00 dBm0 */
/* Sig gen B = 620.00 Hz at -18.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x05, 0x1F, 0x0B,
         0x3A, 0x06, 0x9D,
         0x0B, 0x3A,
/* Formatted Parameters Section -------------------------------------------- */
   0x03                    /* FM_EN = 0; WB = 0; Active Generators: A B */
};

/* South Africa Dial Tone  */
const VpProfileDataType T_DL_ZA[] =
{
/* Sig gen A = 399.90 Hz at -6.00 dBm0 */
/* Sig gen B = 432.86 Hz at -12.00 dBm0 */
/* Sig gen C = 366.94 Hz at -12.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x18,                   /* Profile length = 24 + 4 = 28 bytes */
   0x02,                   /* Profile format version = 2 */
   0x15,                   /* MPI section length = 21 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x2C,
         0xB1, 0x04, 0x9E,
         0x16, 0x66,
   0xD4, 0x03, 0xEA, 0x16, /* Signal Generator C, D Parameters */
         0x66, 0x00, 0x00,
         0x59, 0x2B,
/* Formatted Parameters Section -------------------------------------------- */
   0x07                    /* FM_EN = 0; WB = 0; Active Generators: A B C */
};

/* South Africa Ringback Tone  */
const VpProfileDataType T_RB_ZA[] =
{
/* Sig gen A = 399.90 Hz at -6.00 dBm0 */
/* Sig gen B = 432.86 Hz at -12.00 dBm0 */
/* Sig gen C = 366.94 Hz at -12.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x18,                   /* Profile length = 24 + 4 = 28 bytes */
   0x02,                   /* Profile format version = 2 */
   0x15,                   /* MPI section length = 21 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x2C,
         0xB1, 0x04, 0x9E,
         0x16, 0x66,
   0xD4, 0x03, 0xEA, 0x16, /* Signal Generator C, D Parameters */
         0x66, 0x00, 0x00,
         0x59, 0x2B,
/* Formatted Parameters Section -------------------------------------------- */
   0x07                    /* FM_EN = 0; WB = 0; Active Generators: A B C */
};

/* South Africa Busy Tone  */
const VpProfileDataType T_BS_ZA[] =
{
/* Sig gen A = 399.90 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x38,
         0x43, 0x00, 0x00,
         0x59, 0x2B,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/* South Africa Call Waiting */
const VpProfileDataType T_CW_ZA[] =
{
/* Sig gen A = 399.90 Hz at -6.00 dBm0 */
/* Sig gen B = 432.86 Hz at -12.00 dBm0 */
/* Sig gen C = 366.94 Hz at -12.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x18,                   /* Profile length = 24 + 4 = 28 bytes */
   0x02,                   /* Profile format version = 2 */
   0x15,                   /* MPI section length = 21 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x2C,
         0xB1, 0x04, 0x9E,
         0x16, 0x66,
   0xD4, 0x03, 0xEA, 0x16, /* Signal Generator C, D Parameters */
         0x66, 0x00, 0x00,
         0x59, 0x2B,
/* Formatted Parameters Section -------------------------------------------- */
   0x07                    /* FM_EN = 0; WB = 0; Active Generators: A B C */
};

/* South Africa Congestion Tone  */
const VpProfileDataType T_CG_ZA[] =
{
/* Sig gen A = 399.90 Hz at -4.00 dBm0 */
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x02, 0x00,       /* Tone Profile for ZL88702 device */
   0x0F,                   /* Profile length = 15 + 4 = 19 bytes */
   0x02,                   /* Profile format version = 2 */
   0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD2, 0x00, 0x00, 0x00, /* Signal Generator A, B, Bias Parameters */
         0x04, 0x44, 0x38,
         0x43, 0x00, 0x00,
         0x59, 0x2B,
/* Formatted Parameters Section -------------------------------------------- */
   0x01                    /* FM_EN = 0; WB = 0; Active Generators: A */
};

/************** Cadence_Definitions **************/
/* Short Ringing Cadence (2 sec on 4 sec off) */
const VpProfileDataType LE880_RING_CAD_STD[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x01, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x01, 0x07, 0x21, 0x90,
 0x01, 0x05, 0x23, 0x20, 0x40, 0x00
};

/* Short Ringing Cadence (0.5 sec on 0.5 sec off) */
const VpProfileDataType LE880_RING_CAD_SHORT[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x01, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x01, 0x07, 0x20, 0x64,
 0x01, 0x05, 0x20, 0x64, 0x40, 0x00
};

/* Short Ringing Cadence (2 sec on 4 sec off) */
const VpProfileDataType LE886_RING_CAD_STD[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x01, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x01, 0x07, 0x21, 0x90,
 0x01, 0x05, 0x23, 0x20, 0x40, 0x00
};

/* Short Ringing Cadence (0.5 sec on 0.5 sec off) */
const VpProfileDataType LE886_RING_CAD_SHORT[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x01, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x01, 0x07, 0x20, 0x64,
 0x01, 0x05, 0x20, 0x64, 0x40, 0x00
};

/* Continuous ON Tone Cadence  */
const VpProfileDataType CT_CONT[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* ETSI ES201 970 Ringing Signal Cadence */
const VpProfileDataType CR_ETSI[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x12, 0x00, 0x00, 0x00, 0x0E, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x22, 0xF8, 0x40, 0x00
};

/* ETSI ES201 970 Dial Tone Cadence */
const VpProfileDataType CT_DL_ETSI[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* ETSI ES201 970 Ringback Tone Cadence */
const VpProfileDataType CT_RB_ETSI[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x23, 0x20, 0x40, 0x00
};

/* ETSI ES201 970 Busy Tone Cadence */
const VpProfileDataType CT_BS_ETSI[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x40, 0x00
};

/* ETSI ES201 970 Call Waiting Cadence */
const VpProfileDataType CT_CW_ETSI[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x18, 0x00, 0x00, 0x00, 0x14, 0x00, 0x8F, 0x20, 0x28,
 0x00, 0x80, 0x20, 0x28, 0x00, 0x8F, 0x20, 0x28, 0x00, 0x80, 0x23, 0xE8,
 0x23, 0x20, 0x40, 0x01
};

/* ETSI ES201 970 Congestion Cadence */
const VpProfileDataType CT_CG_ETSI[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x32,
 0x00, 0x80, 0x20, 0x32, 0x40, 0x00
};

/* Argentina Ringing Signal Cadence */
const VpProfileDataType CR_AR[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x1A, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x05, 0x20, 0x28, 0x02, 0x01, 0x22, 0xF8, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x22, 0xF8, 0x46, 0x00
};

/* Argentina Dial Tone Cadence */
const VpProfileDataType CT_DL_AR[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Argentina Ringback Tone Cadence */
const VpProfileDataType CT_RB_AR[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x23, 0x20, 0x40, 0x00
};

/* Argentina Busy Tone Cadence */
const VpProfileDataType CT_BS_AR[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x3C,
 0x00, 0x80, 0x20, 0x28, 0x40, 0x00
};

/* Argentina Call Waiting Cadence */
const VpProfileDataType CT_CW_AR[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x8F, 0x20, 0x3C,
 0x00, 0x80, 0x23, 0xE8, 0x23, 0xE8, 0x40, 0x01
};

/* Argentina Congestion Cadence */
const VpProfileDataType CT_CG_AR[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x3C,
 0x00, 0x8F, 0x20, 0x50, 0x40, 0x00
};

/* Austria Ringing Signal Cadence */
const VpProfileDataType CR_AT[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x1A, 0x01, 0x07, 0x20, 0x78,
 0x01, 0x05, 0x20, 0x28, 0x02, 0x01, 0x23, 0xC0, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x23, 0xC0, 0x46, 0x00
};

/* Austria Dial Tone Cadence */
const VpProfileDataType CT_DL_AT[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Austria Ringback Tone Cadence */
const VpProfileDataType CT_RB_AT[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x23, 0xE8, 0x40, 0x00
};

/* Austria Busy Tone Cadence */
const VpProfileDataType CT_BS_AT[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x50,
 0x00, 0x80, 0x20, 0x50, 0x40, 0x00
};

/* Austria Call Waiting Cadence */
const VpProfileDataType CT_CW_AT[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x08,
 0x00, 0x80, 0x21, 0x86, 0x40, 0x01
};

/* Austria Congestion Cadence */
const VpProfileDataType CT_CG_AT[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x28,
 0x00, 0x8F, 0x20, 0x28, 0x40, 0x00
};

/* Australia Ringing Signal Cadence  */
const VpProfileDataType CR_AU[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x26, 0x00, 0x00, 0x00, 0x22, 0x01, 0x07, 0x20, 0x50,
 0x01, 0x05, 0x20, 0x28, 0x02, 0x01, 0x21, 0x68, 0x01, 0x07, 0x20, 0x50,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x07, 0x20, 0x50, 0x01, 0x00, 0x20, 0x28,
 0x01, 0x05, 0x21, 0x68, 0x46, 0x00
};

/* Australia Dial Tone Cadence */
const VpProfileDataType CT_DL_AU[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Australia Ringback Tone Cadence */
const VpProfileDataType CT_RB_AU[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x50,
 0x00, 0x80, 0x20, 0x28, 0x00, 0x8F, 0x20, 0x50, 0x00, 0x80, 0x21, 0x90,
 0x40, 0x00
};

/* Australia Busy Tone Cadence */
const VpProfileDataType CT_BS_AU[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x4B,
 0x00, 0x80, 0x20, 0x4B, 0x40, 0x00
};

/* Australia Call Waiting Tone Cadence */
const VpProfileDataType CT_CW_AU[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x28,
 0x00, 0x80, 0x20, 0x28, 0x00, 0x8F, 0x20, 0x28, 0x00, 0x80, 0x23, 0x70,
 0x40, 0x01
};

/* Australia Congestion Tone Cadence */
const VpProfileDataType CT_CG_AU[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x81, 0x20, 0x4B,
 0x00, 0x80, 0x20, 0x4B, 0x00, 0x82, 0x20, 0x4B, 0x00, 0x80, 0x20, 0x4B,
 0x40, 0x00
};

/* Belgium Ringing Signal Cadence */
const VpProfileDataType CR_BE[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x18, 0x01, 0x05, 0x20, 0x14,
 0x07, 0x00, 0x22, 0x58, 0x20, 0x3C, 0x01, 0x07, 0x20, 0xC8, 0x01, 0x00,
 0x20, 0x28, 0x01, 0x05, 0x22, 0x30, 0x45, 0x00
};

/* Belgium Dial Tone Cadence */
const VpProfileDataType CT_DL_BE[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Belgium Ringback Tone Cadence */
const VpProfileDataType CT_RB_BE[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x22, 0x58, 0x40, 0x00
};

/* Belgium Busy Tone Cadence */
const VpProfileDataType CT_BS_BE[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x40, 0x00
};

/* Belgium Call Waiting Cadence */
const VpProfileDataType CT_CW_BE[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x23,
 0x00, 0x80, 0x20, 0x23, 0x00, 0x8F, 0x20, 0x23, 0x00, 0x80, 0x22, 0xBC,
 0x40, 0x01
};

/* Belgium Congestion Cadence */
const VpProfileDataType CT_CG_BE[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x21,
 0x00, 0x80, 0x20, 0x21, 0x40, 0x00
};

/* Bulgaria Ringing Signal Cadence */
const VpProfileDataType CR_BG[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x12, 0x00, 0x00, 0x00, 0x0E, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x22, 0xF8, 0x40, 0x00
};

/* Bulgaria Dial Tone Cadence */
const VpProfileDataType CT_DL_BG[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Bulgaria Ringback Tone Cadence */
const VpProfileDataType CT_RB_BG[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x23, 0x20, 0x40, 0x00
};

/* Bulgaria Busy Tone Cadence */
const VpProfileDataType CT_BS_BG[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x40, 0x00
};

/* Bulgaria Call Waiting Cadence */
const VpProfileDataType CT_CW_BG[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x1E,
 0x00, 0x80, 0x20, 0x1E, 0x00, 0x8F, 0x20, 0x1E, 0x00, 0x80, 0x23, 0x20,
 0x40, 0x01
};

/* Bulgaria Congestion Cadence */
const VpProfileDataType CT_CG_BG[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x32,
 0x00, 0x8F, 0x20, 0x32, 0x40, 0x00
};

/* Brazil Ringing Signal Cadence */
const VpProfileDataType CR_BR[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x18, 0x01, 0x05, 0x20, 0x14,
 0x07, 0x00, 0x22, 0x58, 0x20, 0x3C, 0x01, 0x07, 0x20, 0xC8, 0x01, 0x00,
 0x20, 0x28, 0x01, 0x05, 0x22, 0xF8, 0x45, 0x00
};

/* Brazil Dial Tone Cadence */
const VpProfileDataType CT_DL_BR[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Brazil Ringback Tone Cadence */
const VpProfileDataType CT_RB_BR[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x23, 0x20, 0x40, 0x00
};

/* Brazil Busy Tone Cadence */
const VpProfileDataType CT_BS_BR[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x32,
 0x00, 0x80, 0x20, 0x32, 0x40, 0x00
};

/* Brazil Call Waiting Cadence */
const VpProfileDataType CT_CW_BR[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x0C,
 0x00, 0x80, 0x20, 0x32, 0x00, 0x8F, 0x20, 0x0C, 0x00, 0x80, 0x23, 0xE8,
 0x40, 0x01
};

/* Canada Ringing Signal Cadence */
const VpProfileDataType CR_CA[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x1A, 0x01, 0x07, 0x21, 0x90,
 0x01, 0x05, 0x20, 0x28, 0x02, 0x01, 0x22, 0xF8, 0x01, 0x07, 0x21, 0x90,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x22, 0xF8, 0x46, 0x00
};

/* Canada Dial Tone Cadence */
const VpProfileDataType CT_DL_CA[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Canada Ringback Tone Cadence */
const VpProfileDataType CT_RB_CA[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x21, 0x90,
 0x00, 0x80, 0x23, 0x20, 0x40, 0x00
};

/* Canada Busy Tone Cadence */
const VpProfileDataType CT_BS_CA[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x40, 0x00
};

/* Canada Call Waiting Cadence */
const VpProfileDataType CT_CW_CA[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x8F, 0x20, 0x3C,
 0x00, 0x80, 0x23, 0xE8, 0x23, 0xE8, 0x40, 0x01
};

/* Canada Congestion Cadence */
const VpProfileDataType CT_CG_CA[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x32,
 0x00, 0x80, 0x20, 0x32, 0x40, 0x00
};

/* Switzerland Ringing Signal Cadence */
const VpProfileDataType CR_CH[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1A, 0x00, 0x00, 0x00, 0x16, 0x01, 0x05, 0x20, 0x0A,
 0x07, 0x00, 0x21, 0xF4, 0x01, 0x07, 0x20, 0xC8, 0x01, 0x00, 0x20, 0x28,
 0x01, 0x05, 0x22, 0xF8, 0x46, 0x00
};

/* Switzerland Dial Tone Cadence */
const VpProfileDataType CT_DL_CH[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Switzerland Ringback Tone Cadence */
const VpProfileDataType CT_RB_CH[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x23, 0x20, 0x40, 0x00
};

/* Switzerland Busy Tone Cadence */
const VpProfileDataType CT_BS_CH[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x40, 0x00
};

/* Switzerland Call Waiting Cadence */
const VpProfileDataType CT_CW_CH[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x28,
 0x00, 0x80, 0x20, 0x28, 0x00, 0x8F, 0x20, 0x28, 0x00, 0x80, 0x23, 0x20,
 0x40, 0x01
};

/* Switzerland Congestion Cadence */
const VpProfileDataType CT_CG_CH[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x28,
 0x00, 0x8F, 0x20, 0x28, 0x40, 0x00
};

/* Chile Ringing Signal Cadence */
const VpProfileDataType CR_CL[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x1A, 0x01, 0x07, 0x20, 0x78,
 0x01, 0x05, 0x20, 0x28, 0x02, 0x01, 0x22, 0x30, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x22, 0x30, 0x46, 0x00
};

/* Chile Dial Tone Cadence */
const VpProfileDataType CT_DL_CL[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x40, 0x00
};

/* Chile Ringback Tone Cadence */
const VpProfileDataType CT_RB_CL[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x22, 0x58, 0x40, 0x00
};

/* Chile Busy Tone Cadence */
const VpProfileDataType CT_BS_CL[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x40, 0x00
};

/* Chile Call Waiting Cadence */
const VpProfileDataType CT_CW_CL[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x23, 0xE8, 0x40, 0x01
};

/* Chile Congestion Cadence */
const VpProfileDataType CT_CG_CL[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x28,
 0x00, 0x8F, 0x20, 0x28, 0x40, 0x00
};

/* China Ringing Signal Cadence */
const VpProfileDataType CR_CN[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x1A, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x05, 0x20, 0x28, 0x02, 0x01, 0x22, 0xF8, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x22, 0xF8, 0x46, 0x00
};

/* China Dial Tone Cadence */
const VpProfileDataType CT_DL_CN[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* China Ringback Tone Cadence */
const VpProfileDataType CT_RB_CN[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x23, 0x20, 0x40, 0x00
};

/* China Busy Tone Cadence */
const VpProfileDataType CT_BS_CN[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x46,
 0x00, 0x80, 0x20, 0x46, 0x40, 0x00
};

/* China Call Waiting Cadence */
const VpProfileDataType CT_CW_CN[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x50,
 0x00, 0x80, 0x23, 0x20, 0x40, 0x01
};

/* China Congestion Cadence */
const VpProfileDataType CT_CG_CN[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x8C,
 0x00, 0x80, 0x20, 0x8C, 0x40, 0x00
};

/* Czech Republic Ringing Signal Cadence */
const VpProfileDataType CR_CZ[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x1A, 0x01, 0x07, 0x20, 0x78,
 0x01, 0x05, 0x20, 0x28, 0x02, 0x01, 0x22, 0xF8, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x22, 0xF8, 0x46, 0x00
};

/* Czech Republic Dial Tone Cadence */
const VpProfileDataType CT_DL_CZ[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x42,
 0x00, 0x80, 0x20, 0x42, 0x00, 0x8F, 0x20, 0x84, 0x00, 0x80, 0x20, 0x84,
 0x40, 0x00
};

/* Czech Republic Ringback Tone Cadence */
const VpProfileDataType CT_RB_CZ[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x23, 0x20, 0x40, 0x00
};

/* Czech Republic Busy Tone Cadence */
const VpProfileDataType CT_BS_CZ[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x42,
 0x00, 0x80, 0x20, 0x42, 0x40, 0x00
};

/* Czech Republic Call Waiting Cadence */
const VpProfileDataType CT_CW_CZ[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x8F, 0x20, 0x42,
 0x00, 0x80, 0x23, 0xE8, 0x23, 0x20, 0x40, 0x01
};

/* Czech Republic Congestion Cadence */
const VpProfileDataType CT_CG_CZ[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x21,
 0x00, 0x8F, 0x20, 0x21, 0x40, 0x00
};

/* Germany Ringing Signal Cadence */
const VpProfileDataType CR_DE[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x1A, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x05, 0x20, 0xA0, 0x02, 0x01, 0x22, 0x80, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x22, 0xF8, 0x46, 0x00
};

/* Germany Dial Tone Cadence */
const VpProfileDataType CT_DL_DE[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Germany Ringback Tone Cadence */
const VpProfileDataType CT_RB_DE[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x23, 0x20, 0x40, 0x00
};

/* Germany Busy Tone Cadence */
const VpProfileDataType CT_BS_DE[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x60,
 0x00, 0x80, 0x20, 0x60, 0x40, 0x00
};

/* Germany Call Waiting Cadence */
const VpProfileDataType CT_CW_DE[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x28,
 0x00, 0x80, 0x20, 0x28, 0x00, 0x8F, 0x20, 0x28, 0x00, 0x80, 0x23, 0xE8,
 0x40, 0x01
};

/* Germany Congestion Cadence */
const VpProfileDataType CT_CG_DE[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x30,
 0x00, 0x8F, 0x20, 0x30, 0x40, 0x00
};

/* Denmark Ringing Signal Cadence */
const VpProfileDataType CR_DK[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x18, 0x01, 0x05, 0x20, 0x14,
 0x07, 0x00, 0x22, 0x58, 0x20, 0x3C, 0x01, 0x07, 0x20, 0xC8, 0x01, 0x00,
 0x20, 0x28, 0x01, 0x05, 0x22, 0xF8, 0x45, 0x00
};

/* Denmark Dial Tone Cadence */
const VpProfileDataType CT_DL_DK[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Denmark Ringback Tone Cadence */
const VpProfileDataType CT_RB_DK[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x23, 0x20, 0x40, 0x00
};

/* Denmark Busy Tone Cadence */
const VpProfileDataType CT_BS_DK[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x32,
 0x00, 0x80, 0x20, 0x32, 0x40, 0x00
};

/* Denmark Call Waiting Cadence */
const VpProfileDataType CT_CW_DK[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x8F, 0x20, 0x10,
 0x00, 0x80, 0x23, 0xE8, 0x23, 0xE8, 0x40, 0x01
};

/* Denmark Congestion Cadence */
const VpProfileDataType CT_CG_DK[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x32,
 0x00, 0x8F, 0x20, 0x32, 0x40, 0x00
};

/* Ecuador Ringing Signal Cadence */
const VpProfileDataType CR_EC[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x1A, 0x01, 0x07, 0x20, 0xF0,
 0x01, 0x05, 0x20, 0x28, 0x02, 0x01, 0x23, 0x7A, 0x01, 0x07, 0x20, 0xF0,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x23, 0x7A, 0x46, 0x00
};

/* Ecuador Dial Tone Cadence */
const VpProfileDataType CT_DL_EC[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Ecuador Ringback Tone Cadence */
const VpProfileDataType CT_RB_EC[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xF0,
 0x00, 0x80, 0x23, 0xA2, 0x40, 0x00
};

/* Ecuador Busy Tone Cadence */
const VpProfileDataType CT_BS_EC[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x42,
 0x00, 0x80, 0x20, 0x42, 0x40, 0x00
};

/* Ecuador Call Waiting Cadence */
const VpProfileDataType CT_CW_EC[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x28,
 0x00, 0x80, 0x20, 0x78, 0x40, 0x01
};

/* Ecuador Congestion Cadence */
const VpProfileDataType CT_CG_EC[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x42,
 0x00, 0x80, 0x20, 0x42, 0x40, 0x00
};

/* Spain Ringing Signal Cadence */
const VpProfileDataType CR_ES[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x20, 0x00, 0x00, 0x00, 0x1C, 0x01, 0x07, 0x20, 0x32,
 0x01, 0x05, 0x20, 0x14, 0x07, 0x00, 0x22, 0x30, 0x20, 0x46, 0x01, 0x07,
 0x21, 0x2C, 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x22, 0x30, 0x47, 0x00
};

/* Spain Dial Tone Cadence */
const VpProfileDataType CT_DL_ES[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Spain Ringback Tone Cadence */
const VpProfileDataType CT_RB_ES[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x21, 0x2C,
 0x00, 0x80, 0x22, 0x58, 0x40, 0x00
};

/* Spain Busy Tone Cadence */
const VpProfileDataType CT_BS_ES[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x28,
 0x00, 0x80, 0x20, 0x28, 0x40, 0x00
};

/* Spain Call Waiting Cadence */
const VpProfileDataType CT_CW_ES[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x23,
 0x00, 0x80, 0x20, 0x23, 0x00, 0x8F, 0x20, 0x23, 0x00, 0x80, 0x22, 0xBC,
 0x40, 0x01
};

/* Spain Congestion Cadence */
const VpProfileDataType CT_CG_ES[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x8F, 0x20, 0x28,
 0x00, 0x80, 0x20, 0x28, 0x00, 0x8F, 0x20, 0x28, 0x00, 0x80, 0x20, 0x28,
 0x00, 0x8F, 0x20, 0x28, 0x00, 0x80, 0x20, 0x78, 0x40, 0x00
};

/* Finland Ringing Signal Cadence */
const VpProfileDataType CR_FI[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x18, 0x01, 0x05, 0x20, 0x0A,
 0x07, 0x00, 0x22, 0x58, 0x20, 0x3C, 0x01, 0x07, 0x20, 0xC8, 0x01, 0x00,
 0x20, 0x28, 0x01, 0x05, 0x22, 0xF8, 0x45, 0x00
};

/* Finland Dial Tone Cadence */
const VpProfileDataType CT_DL_FI[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Finland Ringback Tone Cadence */
const VpProfileDataType CT_RB_FI[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x23, 0x20, 0x40, 0x00
};

/* Finland Busy Tone Cadence */
const VpProfileDataType CT_BS_FI[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x3C,
 0x00, 0x80, 0x20, 0x3C, 0x40, 0x00
};

/* Finland Call Waiting Cadence */
const VpProfileDataType CT_CW_FI[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x18, 0x00, 0x00, 0x00, 0x14, 0x00, 0x8F, 0x20, 0x1E,
 0x00, 0x80, 0x20, 0x1E, 0x00, 0x8F, 0x20, 0x1E, 0x00, 0x80, 0x23, 0xE8,
 0x22, 0x58, 0x40, 0x01
};

/* Finland Congestion Cadence */
const VpProfileDataType CT_CG_FI[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x28,
 0x00, 0x8F, 0x20, 0x28, 0x40, 0x00
};

/* France Ringing Signal Cadence */
const VpProfileDataType CR_FR[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x20, 0x00, 0x00, 0x00, 0x1C, 0x01, 0x07, 0x20, 0x32,
 0x01, 0x05, 0x20, 0x28, 0x07, 0x00, 0x21, 0xF4, 0x20, 0x46, 0x01, 0x07,
 0x21, 0x2C, 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x22, 0x94, 0x47, 0x00
};

/* France Dial Tone Cadence */
const VpProfileDataType CT_DL_FR[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* France Ringback Tone  Cadence */
const VpProfileDataType CT_RB_FR[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x21, 0x2C,
 0x00, 0x80, 0x22, 0xBC, 0x40, 0x00
};

/* France Busy Tone Cadence */
const VpProfileDataType CT_BS_FR[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x40, 0x00
};

/* France Call Waiting Tone Cadence */
const VpProfileDataType CT_CW_FR[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x8F, 0x20, 0x3C,
 0x00, 0x80, 0x23, 0xE8, 0x23, 0xE8, 0x40, 0x01
};

/* UK Ringing Signal Cadence  */
const VpProfileDataType CR_GB[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x22, 0x00, 0x00, 0x00, 0x1E, 0x01, 0x0D, 0x20, 0x0A,
 0x07, 0x00, 0x22, 0x58, 0x01, 0x07, 0x20, 0x50, 0x01, 0x00, 0x20, 0x28,
 0x01, 0x07, 0x20, 0x50, 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x21, 0x68,
 0x44, 0x00
};

/* UK Dial Tone Cadence */
const VpProfileDataType CT_DL_GB[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* UK Ringback Tone Cadence */
const VpProfileDataType CT_RB_GB[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x50,
 0x00, 0x80, 0x20, 0x28, 0x00, 0x8F, 0x20, 0x50, 0x00, 0x80, 0x21, 0x90,
 0x40, 0x00
};

/* UK Busy Tone Cadence */
const VpProfileDataType CT_BS_GB[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x4B,
 0x00, 0x80, 0x20, 0x4B, 0x40, 0x00
};

/* UK Call Waiting Tone Cadence */
const VpProfileDataType CT_CW_GB[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x23, 0xE8, 0x23, 0xE8, 0x40, 0x01
};

/* UK Congestion Tone Cadence */
const VpProfileDataType CT_CG_GB[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x50,
 0x00, 0x80, 0x20, 0x46, 0x00, 0x8F, 0x20, 0x2D, 0x00, 0x80, 0x20, 0x69,
 0x40, 0x00
};

/* Greece Ringing Signal Cadence */
const VpProfileDataType CR_GR[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x1A, 0x01, 0x07, 0x20, 0x78,
 0x01, 0x05, 0x20, 0x28, 0x02, 0x01, 0x22, 0xF8, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x22, 0xF8, 0x46, 0x00
};

/* Greece Dial Tone Cadence */
const VpProfileDataType CT_DL_GR[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x28,
 0x00, 0x80, 0x20, 0x3C, 0x00, 0x8F, 0x20, 0x8C, 0x00, 0x80, 0x20, 0xA0,
 0x40, 0x00
};

/* Greece Ringback Tone Cadence */
const VpProfileDataType CT_RB_GR[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x23, 0x20, 0x40, 0x00
};

/* Greece Busy Tone Cadence */
const VpProfileDataType CT_BS_GR[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x3C,
 0x00, 0x80, 0x20, 0x3C, 0x40, 0x00
};

/* Greece Call Waiting Cadence */
const VpProfileDataType CT_CW_GR[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x8F, 0x20, 0x3C,
 0x00, 0x80, 0x23, 0xE8, 0x23, 0xE8, 0x40, 0x01
};

/* Greece Congestion Cadence */
const VpProfileDataType CT_CG_GR[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x1E,
 0x00, 0x8F, 0x20, 0x1E, 0x40, 0x00
};

/* Hong Kong SAR Ringing Signal Cadence  */
const VpProfileDataType CR_HK[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x26, 0x00, 0x00, 0x00, 0x22, 0x01, 0x07, 0x20, 0xF0,
 0x01, 0x05, 0x20, 0x28, 0x02, 0x01, 0x22, 0x30, 0x01, 0x07, 0x20, 0x50,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x07, 0x20, 0x50, 0x01, 0x00, 0x20, 0x28,
 0x01, 0x05, 0x22, 0x30, 0x46, 0x00
};

/* Hong Kong SAR Dial Tone Cadence */
const VpProfileDataType CT_DL_HK[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Hong Kong SAR Ringback Tone Cadence */
const VpProfileDataType CT_RB_HK[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x50,
 0x00, 0x80, 0x20, 0x28, 0x00, 0x8F, 0x20, 0x50, 0x00, 0x80, 0x22, 0x58,
 0x40, 0x00
};

/* Hong Kong SAR Busy Tone Cadence */
const VpProfileDataType CT_BS_HK[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x40, 0x00
};

/* Hong Kong SAR Call Waiting Cadence */
const VpProfileDataType CT_CW_HK[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x8F, 0x20, 0x3C,
 0x00, 0x80, 0x23, 0xE8, 0x23, 0xE8, 0x40, 0x01
};

/* Hong Kong SAR Congestion Cadence */
const VpProfileDataType CT_CG_HK[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x32,
 0x00, 0x80, 0x20, 0x32, 0x40, 0x00
};

/* Hungary Ringing Signal Cadence */
const VpProfileDataType CR_HU[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x12, 0x00, 0x00, 0x00, 0x0E, 0x01, 0x07, 0x20, 0xFA,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x22, 0xC6, 0x40, 0x00
};

/* Hungary Dial Tone Cadence */
const VpProfileDataType CT_DL_HU[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Hungary Ringback Tone Cadence */
const VpProfileDataType CT_RB_HU[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xFA,
 0x00, 0x80, 0x22, 0xEE, 0x40, 0x00
};

/* Hungary Busy Tone Cadence */
const VpProfileDataType CT_BS_HU[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x40, 0x00
};

/* Hungary Call Waiting Cadence */
const VpProfileDataType CT_CW_HU[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x08,
 0x00, 0x80, 0x21, 0x88, 0x40, 0x01
};

/* Hungary Congestion Cadence */
const VpProfileDataType CT_CG_HU[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x3C,
 0x00, 0x8F, 0x20, 0x3C, 0x40, 0x00
};

/* Indonesia Ringing Signal Cadence */
const VpProfileDataType CR_ID[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x12, 0x00, 0x00, 0x00, 0x0E, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x22, 0xF8, 0x40, 0x00
};

/* Indonesia Dial Tone Cadence */
const VpProfileDataType CT_DL_ID[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Indonesia Ringback Tone Cadence */
const VpProfileDataType CT_RB_ID[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x23, 0x20, 0x40, 0x00
};

/* Indonesia Busy Tone Cadence */
const VpProfileDataType CT_BS_ID[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x40, 0x00
};

/* Indonesia Call Waiting Cadence */
const VpProfileDataType CT_CW_ID[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x18, 0x00, 0x00, 0x00, 0x14, 0x00, 0x8F, 0x20, 0x1E,
 0x00, 0x80, 0x20, 0x1E, 0x00, 0x8F, 0x20, 0x1E, 0x00, 0x80, 0x23, 0xE8,
 0x23, 0xE8, 0x40, 0x01
};

/* Indonesia Congestion Cadence */
const VpProfileDataType CT_CG_ID[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x32,
 0x00, 0x8F, 0x20, 0x32, 0x40, 0x00
};

/* Ireland Ringing Signal Cadence */
const VpProfileDataType CR_IE[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x28, 0x00, 0x00, 0x00, 0x24, 0x01, 0x07, 0x20, 0x3C,
 0x01, 0x05, 0x20, 0x28, 0x07, 0x00, 0x21, 0xF4, 0x20, 0x46, 0x01, 0x07,
 0x20, 0x50, 0x01, 0x00, 0x20, 0x28, 0x01, 0x07, 0x20, 0x50, 0x01, 0x00,
 0x20, 0x28, 0x01, 0x05, 0x21, 0x68, 0x47, 0x00
};

/* Ireland Dial Tone Cadence */
const VpProfileDataType CT_DL_IE[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Ireland Ringback Tone Cadence */
const VpProfileDataType CT_RB_IE[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x50,
 0x00, 0x80, 0x20, 0x28, 0x00, 0x8F, 0x20, 0x50, 0x00, 0x80, 0x21, 0x90,
 0x40, 0x00
};

/* Ireland Busy Tone Cadence */
const VpProfileDataType CT_BS_IE[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x40, 0x00
};

/* Ireland Call Waiting Cadence */
const VpProfileDataType CT_CW_IE[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x24,
 0x00, 0x80, 0x20, 0x28, 0x00, 0x8F, 0x20, 0x28, 0x00, 0x80, 0x23, 0x84,
 0x40, 0x01
};

/* Israel Ringing Signal Cadence */
const VpProfileDataType CR_IL[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x1A, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x05, 0x20, 0x28, 0x02, 0x01, 0x22, 0x30, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x22, 0x30, 0x46, 0x00
};

/* Israel Dial Tone Cadence */
const VpProfileDataType CT_DL_IL[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Israel Ringback Tone Cadence */
const VpProfileDataType CT_RB_IL[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x22, 0x58, 0x40, 0x00
};

/* Israel Busy Tone Cadence */
const VpProfileDataType CT_BS_IL[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x40, 0x00
};

/* Israel Call Waiting Cadence */
const VpProfileDataType CT_CW_IL[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x23, 0xE8, 0x23, 0xE8, 0x40, 0x01
};

/* Israel Congestion Cadence (SIT) */
const VpProfileDataType CT_CG_IL[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x81, 0x20, 0x42,
 0x00, 0x80, 0x20, 0x06, 0x00, 0x82, 0x20, 0x42, 0x00, 0x80, 0x20, 0x06,
 0x00, 0x84, 0x20, 0x42, 0x00, 0x80, 0x23, 0x20, 0x40, 0x00
};

/* India Ringing Signal Cadence */
const VpProfileDataType CR_IN[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x24, 0x00, 0x00, 0x00, 0x20, 0x01, 0x05, 0x20, 0x14,
 0x07, 0x00, 0x22, 0x58, 0x20, 0x3C, 0x01, 0x07, 0x20, 0x50, 0x01, 0x00,
 0x20, 0x28, 0x01, 0x07, 0x20, 0x50, 0x01, 0x00, 0x20, 0x28, 0x01, 0x05,
 0x21, 0x68, 0x45, 0x00
};

/* India Dial Tone Cadence */
const VpProfileDataType CT_DL_IN[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* India Ringback Tone Cadence */
const VpProfileDataType CT_RB_IN[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x50,
 0x00, 0x80, 0x20, 0x28, 0x00, 0x8F, 0x20, 0x50, 0x00, 0x80, 0x21, 0x90,
 0x40, 0x00
};

/* India Busy Tone Cadence */
const VpProfileDataType CT_BS_IN[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x4B,
 0x00, 0x80, 0x20, 0x4B, 0x40, 0x00
};

/* India Call Waiting Tone Cadence */
const VpProfileDataType CT_CW_IN[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x28,
 0x00, 0x80, 0x20, 0x14, 0x00, 0x8F, 0x20, 0x28, 0x00, 0x80, 0x25, 0xDC,
 0x40, 0x01
};

/* India Congestion Tone Cadence */
const VpProfileDataType CT_CG_IN[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x81, 0x20, 0x32,
 0x00, 0x80, 0x20, 0x32, 0x40, 0x00
};

/* Iceland Ringing Signal Cadence */
const VpProfileDataType CR_IS[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x18, 0x01, 0x05, 0x20, 0x14,
 0x07, 0x00, 0x22, 0x58, 0x20, 0x3C, 0x01, 0x07, 0x20, 0xF0, 0x01, 0x00,
 0x20, 0x28, 0x01, 0x05, 0x23, 0x84, 0x45, 0x00
};

/* Iceland Dial Tone Cadence */
const VpProfileDataType CT_DL_IS[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Iceland Ringback Tone Cadence */
const VpProfileDataType CT_RB_IS[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xF0,
 0x00, 0x80, 0x23, 0xAC, 0x40, 0x00
};

/* Iceland Busy Tone Cadence */
const VpProfileDataType CT_BS_IS[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x32,
 0x00, 0x80, 0x20, 0x32, 0x40, 0x00
};

/* Iceland Call Waiting Cadence */
const VpProfileDataType CT_CW_IS[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x28,
 0x00, 0x80, 0x20, 0x78, 0x00, 0x8F, 0x20, 0x28, 0x00, 0x80, 0x23, 0xE8,
 0x40, 0x01
};

/* Iceland Congestion Cadence */
const VpProfileDataType CT_CG_IS[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x32,
 0x00, 0x8F, 0x20, 0x32, 0x40, 0x00
};

/* Italy Ringing Signal Cadence */
const VpProfileDataType CR_IT[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x1A, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x05, 0x20, 0x28, 0x02, 0x01, 0x22, 0xF8, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x22, 0xF8, 0x46, 0x00
};

/* Italy Dial Tone Cadence */
const VpProfileDataType CT_DL_IT[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x28,
 0x00, 0x80, 0x20, 0x28, 0x00, 0x8F, 0x20, 0x78, 0x00, 0x80, 0x20, 0xC8,
 0x40, 0x00
};

/* Italy Ringback Tone Cadence */
const VpProfileDataType CT_RB_IT[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x23, 0x20, 0x40, 0x00
};

/* Italy Busy Tone Cadence */
const VpProfileDataType CT_BS_IT[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x40, 0x00
};

/* Italy Call Waiting Cadence */
const VpProfileDataType CT_CW_IT[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x20, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x8F, 0x20, 0x50,
 0x00, 0x80, 0x20, 0x14, 0x00, 0x8F, 0x20, 0x32, 0x00, 0x80, 0x20, 0x14,
 0x00, 0x8F, 0x20, 0x1E, 0x00, 0x80, 0x25, 0x78, 0x25, 0x78, 0x40, 0x01
};

/* Italy Congestion Cadence */
const VpProfileDataType CT_CG_IT[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x28,
 0x00, 0x80, 0x20, 0x28, 0x40, 0x00
};

/* Japan Ringing Cadence (NTT TIR and CAR) Before CID */
const VpProfileDataType CR_JP1[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x12, 0x00, 0x00, 0x00, 0x0E, 0x01, 0x0B, 0x20, 0x14,
 0x01, 0x0F, 0x20, 0x64, 0x01, 0x0D, 0x20, 0x64, 0x42, 0x05
};

/* Japan Normal Ringing Signal Cadence (NTT IR) - Follow-On Rings */
const VpProfileDataType CR_JP2[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x12, 0x00, 0x00, 0x00, 0x0E, 0x01, 0x0F, 0x20, 0xC8,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x0D, 0x21, 0x68, 0x40, 0x00
};

/* Japan Dial Tone Cadence */
const VpProfileDataType CT_DL_JP[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Japan Ringback Tone Cadence */
const VpProfileDataType CT_RB_JP[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x21, 0x90, 0x40, 0x00
};

/* Japan Busy Tone Cadence */
const VpProfileDataType CT_BS_JP[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x40, 0x00
};

/* Japan Call Waiting Tone Cadence */
const VpProfileDataType CT_CW_JP[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x00, 0x81, 0x20, 0x64, 0x00, 0x80, 0x22, 0xB2,
 0x40, 0x01
};

/* S. Korea Ringing Signal Cadence */
const VpProfileDataType CR_KR[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x1A, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x05, 0x20, 0x28, 0x02, 0x01, 0x21, 0x90, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x21, 0x68, 0x46, 0x00
};

/* S. Korea Dial Tone Cadence */
const VpProfileDataType CT_DL_KR[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* S. Korea Ringback Tone Cadence */
const VpProfileDataType CT_RB_KR[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x21, 0x90, 0x40, 0x00
};

/* S. Korea Busy Tone Cadence */
const VpProfileDataType CT_BS_KR[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x40, 0x00
};

/* S. Korea Call Waiting Cadence */
const VpProfileDataType CT_CW_KR[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x32,
 0x00, 0x80, 0x20, 0x32, 0x00, 0x8F, 0x20, 0x32, 0x00, 0x80, 0x22, 0x8A,
 0x40, 0x01
};

/* S. Korea Congestion Cadence */
const VpProfileDataType CT_CG_KR[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x3C,
 0x00, 0x80, 0x20, 0x28, 0x40, 0x00
};

/* Mexico Ringing Signal Cadence */
const VpProfileDataType CR_MX[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x1A, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x05, 0x20, 0x28, 0x02, 0x01, 0x22, 0xF8, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x22, 0xF8, 0x46, 0x00
};

/* Mexico Dial Tone Cadence */
const VpProfileDataType CT_DL_MX[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Mexico Ringback Tone Cadence */
const VpProfileDataType CT_RB_MX[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x23, 0x20, 0x40, 0x00
};

/* Mexico Busy Tone Cadence */
const VpProfileDataType CT_BS_MX[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x32,
 0x00, 0x80, 0x20, 0x32, 0x40, 0x00
};

/* Mexico Congestion Cadence */
const VpProfileDataType CT_CG_MX[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x32,
 0x00, 0x8F, 0x20, 0x32, 0x40, 0x00
};

/* Malaysia Ringing Signal Cadence  */
const VpProfileDataType CR_MY[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1A, 0x00, 0x00, 0x00, 0x16, 0x01, 0x07, 0x20, 0x50,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x07, 0x20, 0x50, 0x01, 0x00, 0x20, 0x28,
 0x01, 0x05, 0x21, 0x68, 0x40, 0x00
};

/* Malaysia Dial Tone Cadence */
const VpProfileDataType CT_DL_MY[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Malaysia Ringback Tone Cadence */
const VpProfileDataType CT_RB_MY[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x50,
 0x00, 0x80, 0x20, 0x28, 0x00, 0x8F, 0x20, 0x50, 0x00, 0x80, 0x21, 0x90,
 0x40, 0x00
};

/* Malaysia Busy Tone Cadence */
const VpProfileDataType CT_BS_MY[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x40, 0x00
};

/* Malaysia Congestion Cadence */
const VpProfileDataType CT_CG_MY[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x8F, 0x20, 0x32, 0x40, 0x00
};

/* Netherlands Ringing Signal Cadence */
const VpProfileDataType CR_NL[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x18, 0x01, 0x05, 0x20, 0x14,
 0x07, 0x00, 0x22, 0x58, 0x20, 0x3C, 0x01, 0x07, 0x20, 0xC8, 0x01, 0x00,
 0x20, 0x28, 0x01, 0x05, 0x22, 0xF8, 0x45, 0x00
};

/* Netherlands Dial Tone Cadence */
const VpProfileDataType CT_DL_NL[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Netherlands Ringback Tone Cadence */
const VpProfileDataType CT_RB_NL[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x23, 0x20, 0x40, 0x00
};

/* Netherlands Busy Tone Cadence */
const VpProfileDataType CT_BS_NL[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x40, 0x00
};

/* Netherlands Call Waiting Cadence */
const VpProfileDataType CT_CW_NL[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x23, 0xE8, 0x23, 0x84, 0x40, 0x01
};

/* Netherlands Congestion Cadence */
const VpProfileDataType CT_CG_NL[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x32,
 0x00, 0x8F, 0x20, 0x32, 0x40, 0x00
};

/* Norway Ringing Signal Cadence */
const VpProfileDataType CR_NO[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x1A, 0x01, 0x07, 0x20, 0x78,
 0x01, 0x05, 0x20, 0x28, 0x02, 0x01, 0x22, 0xF8, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x22, 0xF8, 0x46, 0x00
};

/* Norway Dial Tone Cadence */
const VpProfileDataType CT_DL_NO[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Norway Ringback Tone Cadence */
const VpProfileDataType CT_RB_NO[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x23, 0x20, 0x40, 0x00
};

/* Norway Busy Tone Cadence */
const VpProfileDataType CT_BS_NO[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x40, 0x00
};

/* Norway Call Waiting Cadence */
const VpProfileDataType CT_CW_NO[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x18, 0x00, 0x00, 0x00, 0x14, 0x00, 0x8F, 0x20, 0x28,
 0x00, 0x80, 0x20, 0x78, 0x00, 0x8F, 0x20, 0x28, 0x00, 0x80, 0x23, 0xE8,
 0x23, 0xE8, 0x40, 0x01
};

/* Norway Congestion Cadence */
const VpProfileDataType CT_CG_NO[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x28,
 0x00, 0x8F, 0x20, 0x28, 0x40, 0x00
};

/* New Zealand Ringing Signal Cadence  */
const VpProfileDataType CR_NZ[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x22, 0x00, 0x00, 0x00, 0x1E, 0x01, 0x07, 0x20, 0x50,
 0x01, 0x05, 0x20, 0x28, 0x02, 0x01, 0x21, 0x68, 0x01, 0x0F, 0x20, 0x50,
 0x01, 0x0D, 0x20, 0x28, 0x01, 0x0F, 0x20, 0x50, 0x01, 0x0D, 0x21, 0x90,
 0x46, 0x00
};

/* New Zealand Dial Tone Cadence */
const VpProfileDataType CT_DL_NZ[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* New Zealand Ringback Tone Cadence */
const VpProfileDataType CT_RB_NZ[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x50,
 0x00, 0x80, 0x20, 0x28, 0x00, 0x8F, 0x20, 0x50, 0x00, 0x80, 0x21, 0x90,
 0x40, 0x00
};

/* New Zealand Busy Tone Cadence */
const VpProfileDataType CT_BS_NZ[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x40, 0x00
};

/* New Zealand Call Waiting Tone Cadence */
const VpProfileDataType CT_CW_NZ[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x12, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x8F, 0x20, 0x28,
 0x00, 0x80, 0x22, 0x58, 0x40, 0x03, 0x00, 0x8F, 0x20, 0x28
};

/* New Zealand Congestion Tone Cadence */
const VpProfileDataType CT_CG_NZ[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x32,
 0x00, 0x80, 0x20, 0x32, 0x40, 0x00
};

/* Poland Ringing Signal Cadence */
const VpProfileDataType CR_PL[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x12, 0x00, 0x00, 0x00, 0x0E, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x22, 0xF8, 0x40, 0x00
};

/* Poland Dial Tone Cadence */
const VpProfileDataType CT_DL_PL[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Poland Ringback Tone Cadence */
const VpProfileDataType CT_RB_PL[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x23, 0x20, 0x40, 0x00
};

/* Poland Busy Tone Cadence */
const VpProfileDataType CT_BS_PL[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x40, 0x00
};

/* Poland Call Waiting Cadence */
const VpProfileDataType CT_CW_PL[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x1E,
 0x00, 0x80, 0x20, 0x1E, 0x00, 0x8F, 0x20, 0x1E, 0x00, 0x80, 0x23, 0x20,
 0x40, 0x01
};

/* Poland Congestion Cadence */
const VpProfileDataType CT_CG_PL[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x8F, 0x20, 0x64, 0x40, 0x00
};

/* Portugal Ringing Signal Cadence */
const VpProfileDataType CR_PT[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x1A, 0x01, 0x07, 0x20, 0x78,
 0x01, 0x05, 0x20, 0x28, 0x02, 0x01, 0x23, 0xC0, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x23, 0xC0, 0x46, 0x00
};

/* Portugal Dial Tone Cadence */
const VpProfileDataType CT_DL_PT[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Portugal Ringback Tone Cadence */
const VpProfileDataType CT_RB_PT[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x23, 0xE8, 0x40, 0x00
};

/* Portugal Busy Tone Cadence */
const VpProfileDataType CT_BS_PT[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x40, 0x00
};

/* Portugal Call Waiting Cadence */
const VpProfileDataType CT_CW_PT[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x28,
 0x00, 0x80, 0x20, 0x28, 0x00, 0x8F, 0x20, 0x28, 0x00, 0x80, 0x23, 0xE8,
 0x40, 0x01
};

/* Portugal Congestion Cadence */
const VpProfileDataType CT_CG_PT[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x28,
 0x00, 0x8F, 0x20, 0x28, 0x40, 0x00
};

/* Russia Ringing Signal Cadence */
const VpProfileDataType CR_RU[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x12, 0x00, 0x00, 0x00, 0x0E, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x22, 0x30, 0x40, 0x00
};

/* Russia Dial Tone Cadence */
const VpProfileDataType CT_DL_RU[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Russia Ringback Tone Cadence */
const VpProfileDataType CT_RB_RU[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xA0,
 0x00, 0x80, 0x22, 0x80, 0x40, 0x00
};

/* Russia Busy Tone Cadence */
const VpProfileDataType CT_BS_RU[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x50,
 0x00, 0x80, 0x20, 0x50, 0x40, 0x00
};

/* Russia Call Waiting Cadence */
const VpProfileDataType CT_CW_RU[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x28,
 0x00, 0x80, 0x23, 0xE8, 0x40, 0x01
};

/* Russia Congestion Cadence */
const VpProfileDataType CT_CG_RU[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x28,
 0x00, 0x8F, 0x20, 0x28, 0x40, 0x00
};

/* Sweden Ringing Signal Cadence */
const VpProfileDataType CR_SE[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x18, 0x01, 0x05, 0x20, 0x14,
 0x07, 0x00, 0x22, 0x58, 0x20, 0x28, 0x01, 0x07, 0x20, 0xC8, 0x01, 0x00,
 0x20, 0x28, 0x01, 0x05, 0x23, 0xC0, 0x45, 0x00
};

/* Sweden Dial Tone Cadence */
const VpProfileDataType CT_DL_SE[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Sweden Ringback Tone Cadence */
const VpProfileDataType CT_RB_SE[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x23, 0xE8, 0x40, 0x00
};

/* Sweden Busy Tone Cadence */
const VpProfileDataType CT_BS_SE[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x32,
 0x00, 0x80, 0x20, 0x32, 0x40, 0x00
};

/* Sweden Call Waiting Cadence */
const VpProfileDataType CT_CW_SE[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x28,
 0x00, 0x80, 0x20, 0x64, 0x00, 0x8F, 0x20, 0x28, 0x00, 0x80, 0x23, 0xE8,
 0x40, 0x01
};

/* Sweden Congestion Cadence */
const VpProfileDataType CT_CG_SE[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x32,
 0x00, 0x8F, 0x20, 0x96, 0x40, 0x00
};

/* Singapore Ringing Signal Cadence  */
const VpProfileDataType CR_SG[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x24, 0x00, 0x00, 0x00, 0x20, 0x01, 0x0F, 0x20, 0xC8,
 0x01, 0x05, 0x20, 0x28, 0x07, 0x00, 0x21, 0xF4, 0x20, 0x64, 0x01, 0x0F,
 0x20, 0x50, 0x01, 0x00, 0x20, 0x28, 0x01, 0x0F, 0x20, 0x50, 0x01, 0x05,
 0x21, 0x90, 0x47, 0x00
};

/* Singapore Dial Tone Cadence */
const VpProfileDataType CT_DL_SG[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Singapore Ringback Tone Cadence */
const VpProfileDataType CT_RB_SG[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x50,
 0x00, 0x80, 0x20, 0x28, 0x00, 0x8F, 0x20, 0x50, 0x00, 0x80, 0x21, 0x90,
 0x40, 0x00
};

/* Singapore Busy Tone Cadence */
const VpProfileDataType CT_BS_SG[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x96,
 0x00, 0x80, 0x20, 0x96, 0x40, 0x00
};

/* Singapore Call Waiting Cadence */
const VpProfileDataType CT_CW_SG[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x3C,
 0x00, 0x80, 0x20, 0x28, 0x00, 0x8F, 0x20, 0x3C, 0x00, 0x80, 0x22, 0x80,
 0x40, 0x01
};

/* Singapore Congestion Tone Cadence */
const VpProfileDataType CT_CG_SG[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x32,
 0x00, 0x80, 0x20, 0x32, 0x40, 0x00
};

/* Thailand Ringing Signal Cadence */
const VpProfileDataType CR_TH[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x12, 0x00, 0x00, 0x00, 0x0E, 0x01, 0x07, 0x20, 0xC8,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x22, 0xF8, 0x40, 0x00
};

/* Thailand Dial Tone Cadence */
const VpProfileDataType CT_DL_TH[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Thailand Ringback Tone Cadence */
const VpProfileDataType CT_RB_TH[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x23, 0x20, 0x40, 0x00
};

/* Thailand Busy Tone Cadence */
const VpProfileDataType CT_BS_TH[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x40, 0x00
};

/* Thailand Call Waiting Cadence */
const VpProfileDataType CT_CW_TH[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x23, 0xE8, 0x23, 0xE8, 0x40, 0x01
};

/* Thailand Congestion Cadence */
const VpProfileDataType CT_CG_TH[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x3C,
 0x00, 0x80, 0x20, 0x3C, 0x40, 0x00
};

/* Turkey Ringing Signal Cadence */
const VpProfileDataType CR_TK[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x1A, 0x01, 0x07, 0x21, 0x90,
 0x01, 0x05, 0x20, 0x28, 0x02, 0x01, 0x22, 0xF8, 0x01, 0x07, 0x21, 0x90,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x22, 0xF8, 0x46, 0x00
};

/* Turkey Dial Tone Cadence */
const VpProfileDataType CT_DL_TK[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Turkey Ringback Tone Cadence */
const VpProfileDataType CT_RB_TK[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x21, 0x90,
 0x00, 0x80, 0x23, 0x20, 0x40, 0x00
};

/* Turkey Busy Tone Cadence */
const VpProfileDataType CT_BS_TK[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x40, 0x00
};

/* Turkey Call Waiting Cadence */
const VpProfileDataType CT_CW_TK[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x28,
 0x00, 0x80, 0x20, 0x78, 0x00, 0x8F, 0x20, 0x28, 0x00, 0x80, 0x26, 0x40,
 0x40, 0x01
};

/* Turkey Congestion Cadence */
const VpProfileDataType CT_CG_TK[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x28,
 0x00, 0x80, 0x20, 0x28, 0x40, 0x03, 0x00, 0x8F, 0x20, 0x78, 0x00, 0x80,
 0x40, 0x00
};

/* Taiwan Ringing Signal Cadence */
const VpProfileDataType CR_TW[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1A, 0x00, 0x00, 0x00, 0x16, 0x01, 0x05, 0x20, 0x14,
 0x07, 0x00, 0x21, 0x90, 0x01, 0x07, 0x20, 0xC8, 0x01, 0x00, 0x20, 0x28,
 0x01, 0x05, 0x21, 0x68, 0x44, 0x00
};

/* Taiwan Dial Tone Cadence */
const VpProfileDataType CT_DL_TW[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* Taiwan Ringback Tone Cadence */
const VpProfileDataType CT_RB_TW[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0xC8,
 0x00, 0x80, 0x21, 0x90, 0x40, 0x00
};

/* Taiwan Busy Tone Cadence */
const VpProfileDataType CT_BS_TW[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x40, 0x00
};

/* Taiwan Call Waiting Cadence */
const VpProfileDataType CT_CW_TW[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x32,
 0x00, 0x80, 0x20, 0x32, 0x00, 0x8F, 0x20, 0x32, 0x00, 0x80, 0x22, 0x8A,
 0x40, 0x01
};

/* Taiwan Congestion Cadence */
const VpProfileDataType CT_CG_TW[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x32,
 0x00, 0x80, 0x20, 0x32, 0x40, 0x00
};

/* US Ringing Signal Cadence */
const VpProfileDataType CR_US[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x1A, 0x01, 0x07, 0x21, 0x90,
 0x01, 0x05, 0x20, 0x28, 0x02, 0x01, 0x22, 0xF8, 0x01, 0x07, 0x21, 0x90,
 0x01, 0x00, 0x20, 0x28, 0x01, 0x05, 0x22, 0xF8, 0x46, 0x00
};

/* USA Dial Tone Cadence */
const VpProfileDataType CT_DL_US[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* USA Ringback Tone Cadence */
const VpProfileDataType CT_RB_US[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x21, 0x90,
 0x00, 0x80, 0x23, 0x20, 0x40, 0x00
};

/* USA Busy Tone Cadence */
const VpProfileDataType CT_BS_US[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x40, 0x00
};

/* USA Call Waiting Cadence */
const VpProfileDataType CT_CW_US[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x8F, 0x20, 0x3C,
 0x00, 0x80, 0x23, 0xE8, 0x23, 0xE8, 0x40, 0x01
};

/* USA Congestion Cadence */
const VpProfileDataType CT_CG_US[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x32,
 0x00, 0x80, 0x20, 0x32, 0x40, 0x00
};

/* South Africa Ringing Signal Cadence  */
const VpProfileDataType CR_ZA[] =
{
  /* Cadence Profile */
 0x00, 0x08, 0x00, 0x28, 0x00, 0x00, 0x00, 0x24, 0x01, 0x07, 0x20, 0x32,
 0x01, 0x05, 0x20, 0x28, 0x07, 0x00, 0x21, 0x90, 0x20, 0x46, 0x01, 0x07,
 0x20, 0x50, 0x01, 0x00, 0x20, 0x28, 0x01, 0x07, 0x20, 0x50, 0x01, 0x00,
 0x20, 0x28, 0x01, 0x05, 0x21, 0x68, 0x47, 0x00
};

/* South Africa Dial Tone Cadence */
const VpProfileDataType CT_DL_ZA[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x8F, 0x20, 0x00
};

/* South Africa Ringback Tone Cadence */
const VpProfileDataType CT_RB_ZA[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x8F, 0x20, 0x50,
 0x00, 0x80, 0x20, 0x28, 0x00, 0x8F, 0x20, 0x50, 0x00, 0x80, 0x21, 0x90,
 0x40, 0x00
};

/* South Africa Busy Tone Cadence */
const VpProfileDataType CT_BS_ZA[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x64,
 0x00, 0x80, 0x20, 0x64, 0x40, 0x00
};

/* South Africa Call Waiting Tone Cadence */
const VpProfileDataType CT_CW_ZA[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x26, 0x00, 0x00, 0x00, 0x22, 0x00, 0x8F, 0x20, 0x32,
 0x00, 0x80, 0x20, 0x32, 0x00, 0x8F, 0x20, 0x32, 0x00, 0x80, 0x20, 0x32,
 0x00, 0x8F, 0x20, 0x32, 0x00, 0x80, 0x20, 0x32, 0x00, 0x8F, 0x20, 0x32,
 0x00, 0x80, 0x25, 0xAA, 0x40, 0x01
};

/* South Africa Congestion Tone Cadence */
const VpProfileDataType CT_CG_ZA[] =
{
  /* Cadence Profile */
 0x00, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x8F, 0x20, 0x32,
 0x00, 0x80, 0x20, 0x32, 0x40, 0x00
};

/************** Caller_ID **************/

/* Argentina Caller ID (Type 1 - On-Hook) - ETSI FSK */
const VpProfileDataType CID_TYPE1_AR[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-6.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x2A, 0x30, 0x0D, 0xDD, 0x2A, 0x30, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x02, 0x58, /* Silence Interval for 600ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* Austria Caller ID (Type 1 - On-Hook) - ETSI FSK */
const VpProfileDataType CID_TYPE1_AT[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-6.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x2A, 0x30, 0x0D, 0xDD, 0x2A, 0x30, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x02, 0x58, /* Silence Interval for 600ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* Australia Caller ID (Type 1 - On-Hook) - Telcordia FSK */
const VpProfileDataType CID_TYPE1_AU[] =
{
  /* Space=2200Hz, Mark=1200Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x17, 0x77, 0x25, 0x99, 0x0C, 0xCC, 0x25, 0x99, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x01, 0x90, /* Silence Interval for 400ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* Australia Caller ID (Type 2 - Off-Hook) - Telcordia FSK */
const VpProfileDataType CID_TYPE2_AU[] =
{
  /* Space=2200Hz, Mark=1200Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x68, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x17, 0x77, 0x25, 0x99, 0x0C, 0xCC, 0x25, 0x99, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x58, /* Length of Elements Data */
  0x00, 0x02, /* Mute the far end */
  0x00, 0x06, 0x00, 0x14, /* Silence Interval for 20ms */
  0x00, 0x04, /* Alert Tone */
  /* Australia Call Waiting Tone */
  /* 425.17 Hz, -23.00 dBm0, 0.00 Hz, -100.00 dBm0 */
  0x09, 0xD4, 0x04, 0x89, 0x06, 0x50, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x00, 0xC8, /* Alert Tone Part 2 - Tone+Cadence = 200ms */
  0x00, 0x06, 0x00, 0xC8, /* Silence Interval for 200ms */
  0x00, 0x04, /* Alert Tone */
  /* Australia Call Waiting Tone */
  /* 425.17 Hz, -23.00 dBm0, 0.00 Hz, -100.00 dBm0 */
  0x09, 0xD4, 0x04, 0x89, 0x06, 0x50, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x00, 0xC8, /* Alert Tone Part 2 - Tone+Cadence = 200ms */
  0x00, 0x06, 0x00, 0x32, /* Silence Interval for 50ms */
  0x00, 0x04, /* Alert Tone */
  /* Caller ID Alert Tone (CAS) - Used in Many Country Caller ID Profiles */
  /* 2129.88 Hz, -8.00 dBm0, 2750.24 Hz, -8.00 dBm0 */
  0x09, 0xD4, 0x16, 0xB8, 0x23, 0x80, 0x1D, 0x56, 0x23, 0x80,
  0x00, 0x05, 0x00, 0x50, /* Alert Tone Part 2 - Tone+Cadence = 80ms */
  0x00, 0x08, 0x00, 0x82, 0x00, 0xD0, 0x00, 0x00, /* Detect Tone A | D, Timeout = 130ms */
  0x00, 0x06, 0x00, 0x46, /* Silence Interval for 70ms */
  0x00, 0x0A, 0x00, 0x41, /* Mark Signal for 65ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x06, 0x00, 0x14, /* Silence Interval for 20ms */
  0x00, 0x03, /* Unmute the far end */
  0x00, 0x0D  /* End of Transmission */
};

/* Belgium Caller ID (Type 1 - On-Hook) - ETSI DTMF */
const VpProfileDataType CID_TYPE1_BE[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-4.00dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x38, 0x42, 0x0D, 0xDD, 0x38, 0x42, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x01, /* Line Reversal */
  0x00, 0x06, 0x01, 0x2C, /* Silence Interval for 300ms */
  0x00, 0x0E, /* Message Data (DTMF Format) */
  0x00, 0x06, 0x00, 0x14, /* Silence Interval for 20ms */
  0x00, 0x01, /* Line Reversal */
  0x00, 0x0D  /* End of Transmission */
};

/* Brazil Caller ID (Type 1 - On-Hook) - ETSI DTMF */
const VpProfileDataType CID_TYPE1_BR[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-4.00dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x38, 0x42, 0x0D, 0xDD, 0x38, 0x42, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x01, /* Line Reversal */
  0x00, 0x06, 0x01, 0x2C, /* Silence Interval for 300ms */
  0x00, 0x0E, /* Message Data (DTMF Format) */
  0x00, 0x06, 0x00, 0x14, /* Silence Interval for 20ms */
  0x00, 0x01, /* Line Reversal */
  0x00, 0x0D  /* End of Transmission */
};

/* Canada Caller ID (Type 1 - On-Hook) - Telcordia FSK */
const VpProfileDataType CID_TYPE1_CA[] =
{
  /* Space=2200Hz, Mark=1200Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x17, 0x77, 0x25, 0x99, 0x0C, 0xCC, 0x25, 0x99, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x01, 0x2C, /* Silence Interval for 300ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* Canada Caller ID (Type 2 - Off-Hook) - Telcordia FSK */
const VpProfileDataType CID_TYPE2_CA[] =
{
  /* Space=2200Hz, Mark=1200Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x54, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x17, 0x77, 0x25, 0x99, 0x0C, 0xCC, 0x25, 0x99, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x44, /* Length of Elements Data */
  0x00, 0x02, /* Mute the far end */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x04, /* Alert Tone */
  /* Canada Call Waiting Tone */
  /* 440.19 Hz, -7.00 dBm0, 0.00 Hz, -100.00 dBm0 */
  0x09, 0xD4, 0x04, 0xB2, 0x27, 0xD5, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x01, 0x2C, /* Alert Tone Part 2 - Tone+Cadence = 300ms */
  0x00, 0x06, 0x00, 0x32, /* Silence Interval for 50ms */
  0x00, 0x04, /* Alert Tone */
  /* Caller ID Alert Tone (CAS) - Used in Many Country Caller ID Profiles */
  /* 2129.88 Hz, -8.00 dBm0, 2750.24 Hz, -8.00 dBm0 */
  0x09, 0xD4, 0x16, 0xB8, 0x23, 0x80, 0x1D, 0x56, 0x23, 0x80,
  0x00, 0x05, 0x00, 0x50, /* Alert Tone Part 2 - Tone+Cadence = 80ms */
  0x00, 0x08, 0x00, 0xA0, 0x00, 0xD0, 0x00, 0x00, /* Detect Tone A | D, Timeout = 160ms */
  0x00, 0x06, 0x00, 0x32, /* Silence Interval for 50ms */
  0x00, 0x0A, 0x00, 0x46, /* Mark Signal for 70ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x03, /* Unmute the far end */
  0x00, 0x0D  /* End of Transmission */
};

/* Switzerland Caller ID (Type 1 - On-Hook) - ETSI FSK */
const VpProfileDataType CID_TYPE1_CH[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-6.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x34, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x2A, 0x30, 0x0D, 0xDD, 0x2A, 0x30, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x24, /* Length of Elements Data */
  0x00, 0x04, /* Alert Tone */
  /* Caller ID Alert Tone (CAS) - Used in Many Country Caller ID Profiles */
  /* 2129.88 Hz, -8.00 dBm0, 2750.24 Hz, -8.00 dBm0 */
  0x09, 0xD4, 0x16, 0xB8, 0x23, 0x80, 0x1D, 0x56, 0x23, 0x80,
  0x00, 0x05, 0x00, 0x64, /* Alert Tone Part 2 - Tone+Cadence = 100ms */
  0x00, 0x06, 0x01, 0x2C, /* Silence Interval for 300ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x06, 0x01, 0x5E, /* Silence Interval for 350ms */
  0x00, 0x0D  /* End of Transmission */
};

/* Switzerland Caller ID (Type 2 - Off-Hook) - ETSI FSK */
const VpProfileDataType CID_TYPE2_CH[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-8.00dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x54, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x23, 0x7F, 0x0D, 0xDD, 0x23, 0x7F, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x44, /* Length of Elements Data */
  0x00, 0x02, /* Mute the far end */
  0x00, 0x06, 0x00, 0x0A, /* Silence Interval for 10ms */
  0x00, 0x04, /* Alert Tone */
  /* Spain Call Waiting Tone */
  /* 425.17 Hz, -14.00 dBm0, 0.00 Hz, -100.00 dBm0 */
  0x09, 0xD4, 0x04, 0x89, 0x11, 0xCB, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x00, 0xAF, /* Alert Tone Part 2 - Tone+Cadence = 175ms */
  0x00, 0x06, 0x00, 0x4B, /* Silence Interval for 75ms */
  0x00, 0x04, /* Alert Tone */
  /* Caller ID Alert Tone (CAS) - Used in Many Country Caller ID Profiles */
  /* 2129.88 Hz, -8.00 dBm0, 2750.24 Hz, -8.00 dBm0 */
  0x09, 0xD4, 0x16, 0xB8, 0x23, 0x80, 0x1D, 0x56, 0x23, 0x80,
  0x00, 0x05, 0x00, 0x50, /* Alert Tone Part 2 - Tone+Cadence = 80ms */
  0x00, 0x08, 0x00, 0xA5, 0x00, 0xD0, 0x00, 0x00, /* Detect Tone A | D, Timeout = 165ms */
  0x00, 0x06, 0x00, 0x50, /* Silence Interval for 80ms */
  0x00, 0x0A, 0x00, 0x41, /* Mark Signal for 65ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x06, 0x00, 0x3C, /* Silence Interval for 60ms */
  0x00, 0x03, /* Unmute the far end */
  0x00, 0x0D  /* End of Transmission */
};

/* Chile Caller ID (Type 1 - On-Hook) - ETSI FSK */
const VpProfileDataType CID_TYPE1_CL[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-6.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x2A, 0x30, 0x0D, 0xDD, 0x2A, 0x30, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x02, 0x58, /* Silence Interval for 600ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* China Caller ID (Type 1 - On-Hook) - Telcordia FSK */
const VpProfileDataType CID_TYPE1_CN[] =
{
  /* Space=2200Hz, Mark=1200Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x17, 0x77, 0x25, 0x99, 0x0C, 0xCC, 0x25, 0x99, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x01, 0x2C, /* Silence Interval for 300ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* China Caller ID (Type 2 - Off-Hook) - Telcordia FSK */
const VpProfileDataType CID_TYPE2_CN[] =
{
  /* Space=2200Hz, Mark=1200Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x54, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x17, 0x77, 0x25, 0x99, 0x0C, 0xCC, 0x25, 0x99, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x44, /* Length of Elements Data */
  0x00, 0x02, /* Mute the far end */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x04, /* Alert Tone */
  /* China Call Waiting Tone */
  /* 450.07 Hz, -7.00 dBm0, 0.00 Hz, -100.00 dBm0 */
  0x09, 0xD4, 0x04, 0xCD, 0x27, 0xD5, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x01, 0x90, /* Alert Tone Part 2 - Tone+Cadence = 400ms */
  0x00, 0x06, 0x00, 0x64, /* Silence Interval for 100ms */
  0x00, 0x04, /* Alert Tone */
  /* Caller ID Alert Tone (CAS) - Used in Many Country Caller ID Profiles */
  /* 2129.88 Hz, -8.00 dBm0, 2750.24 Hz, -8.00 dBm0 */
  0x09, 0xD4, 0x16, 0xB8, 0x23, 0x80, 0x1D, 0x56, 0x23, 0x80,
  0x00, 0x05, 0x00, 0x50, /* Alert Tone Part 2 - Tone+Cadence = 80ms */
  0x00, 0x08, 0x00, 0xA0, 0x00, 0xD0, 0x00, 0x00, /* Detect Tone A | D, Timeout = 160ms */
  0x00, 0x06, 0x00, 0x32, /* Silence Interval for 50ms */
  0x00, 0x0A, 0x00, 0x41, /* Mark Signal for 65ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x03, /* Unmute the far end */
  0x00, 0x0D  /* End of Transmission */
};

/* Czech Republic Caller ID (Type 1 - On-Hook) - ETSI FSK */
const VpProfileDataType CID_TYPE1_CZ[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-6.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x2A, 0x30, 0x0D, 0xDD, 0x2A, 0x30, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x02, 0x58, /* Silence Interval for 600ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* Germany Caller ID (Type 1 - On-Hook) - ETSI FSK */
const VpProfileDataType CID_TYPE1_DE[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-6.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x2A, 0x30, 0x0D, 0xDD, 0x2A, 0x30, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x00, 0x64, /* Silence Interval for 100ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* Germany Caller ID (Type 2 - Off-Hook) - ETSI FSK */
const VpProfileDataType CID_TYPE2_DE[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-6.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x40, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x2A, 0x30, 0x0D, 0xDD, 0x2A, 0x30, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x30, /* Length of Elements Data */
  0x00, 0x02, /* Mute the far end */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x04, /* Alert Tone */
  /* Caller ID Alert Tone (CAS) - Used in Many Country Caller ID Profiles */
  /* 2129.88 Hz, -8.00 dBm0, 2750.24 Hz, -8.00 dBm0 */
  0x09, 0xD4, 0x16, 0xB8, 0x23, 0x80, 0x1D, 0x56, 0x23, 0x80,
  0x00, 0x05, 0x00, 0x50, /* Alert Tone Part 2 - Tone+Cadence = 80ms */
  0x00, 0x08, 0x00, 0xA0, 0x00, 0xD0, 0x00, 0x00, /* Detect Tone A | D, Timeout = 160ms */
  0x00, 0x06, 0x00, 0x32, /* Silence Interval for 50ms */
  0x00, 0x0A, 0x00, 0x41, /* Mark Signal for 65ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x03, /* Unmute the far end */
  0x00, 0x0D  /* End of Transmission */
};

/* Denmark Caller ID (Type 1 - On-Hook) - ETSI DTMF */
const VpProfileDataType CID_TYPE1_DK[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-4.00dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x38, 0x42, 0x0D, 0xDD, 0x38, 0x42, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x01, /* Line Reversal */
  0x00, 0x06, 0x01, 0x2C, /* Silence Interval for 300ms */
  0x00, 0x0E, /* Message Data (DTMF Format) */
  0x00, 0x06, 0x00, 0x14, /* Silence Interval for 20ms */
  0x00, 0x01, /* Line Reversal */
  0x00, 0x0D  /* End of Transmission */
};

/* Ecuador Caller ID (Type 1 - On-Hook) - ETSI FSK */
const VpProfileDataType CID_TYPE1_EC[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-6.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x2A, 0x30, 0x0D, 0xDD, 0x2A, 0x30, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x01, 0x2C, /* Silence Interval for 300ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* Spain Caller ID (Type 1 - On-Hook) - ETSI FSK */
const VpProfileDataType CID_TYPE1_ES[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x25, 0x99, 0x0D, 0xDD, 0x25, 0x99, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x02, 0x26, /* Silence Interval for 550ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* Spain Caller ID (Type 2 - Off-Hook) - ETSI FSK */
const VpProfileDataType CID_TYPE2_ES[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x54, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x25, 0x99, 0x0D, 0xDD, 0x25, 0x99, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x44, /* Length of Elements Data */
  0x00, 0x02, /* Mute the far end */
  0x00, 0x06, 0x00, 0x0A, /* Silence Interval for 10ms */
  0x00, 0x04, /* Alert Tone */
  /* Spain Call Waiting Tone */
  /* 425.17 Hz, -14.00 dBm0, 0.00 Hz, -100.00 dBm0 */
  0x09, 0xD4, 0x04, 0x89, 0x11, 0xCB, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x00, 0xAF, /* Alert Tone Part 2 - Tone+Cadence = 175ms */
  0x00, 0x06, 0x00, 0x4B, /* Silence Interval for 75ms */
  0x00, 0x04, /* Alert Tone */
  /* Caller ID Alert Tone (CAS) - Used in Many Country Caller ID Profiles */
  /* 2129.88 Hz, -8.00 dBm0, 2750.24 Hz, -8.00 dBm0 */
  0x09, 0xD4, 0x16, 0xB8, 0x23, 0x80, 0x1D, 0x56, 0x23, 0x80,
  0x00, 0x05, 0x00, 0x50, /* Alert Tone Part 2 - Tone+Cadence = 80ms */
  0x00, 0x08, 0x00, 0xA5, 0x00, 0xD0, 0x00, 0x00, /* Detect Tone A | D, Timeout = 165ms */
  0x00, 0x06, 0x00, 0x50, /* Silence Interval for 80ms */
  0x00, 0x0A, 0x00, 0x41, /* Mark Signal for 65ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x06, 0x00, 0x3C, /* Silence Interval for 60ms */
  0x00, 0x03, /* Unmute the far end */
  0x00, 0x0D  /* End of Transmission */
};

/* Finland Caller ID (Type 1 - On-Hook) - ETSI DTMF */
const VpProfileDataType CID_TYPE1_FI[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-4.00dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x38, 0x42, 0x0D, 0xDD, 0x38, 0x42, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x01, /* Line Reversal */
  0x00, 0x06, 0x01, 0x2C, /* Silence Interval for 300ms */
  0x00, 0x0E, /* Message Data (DTMF Format) */
  0x00, 0x06, 0x00, 0x14, /* Silence Interval for 20ms */
  0x00, 0x01, /* Line Reversal */
  0x00, 0x0D  /* End of Transmission */
};

/* France Caller ID (Type 1 - On-Hook) - ETSI FSK */
const VpProfileDataType CID_TYPE1_FR[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-6.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x2A, 0x30, 0x0D, 0xDD, 0x2A, 0x30, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x01, 0xC2, /* Silence Interval for 450ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* France Caller ID (Type 2 - Off-Hook) - ETSI FSK */
const VpProfileDataType CID_TYPE2_FR[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-6.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x60, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x2A, 0x30, 0x0D, 0xDD, 0x2A, 0x30, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x50, /* Length of Elements Data */
  0x00, 0x02, /* Mute the far end */
  0x00, 0x06, 0x00, 0x0A, /* Silence Interval for 10ms */
  0x00, 0x04, /* Alert Tone */
  /* France Call Waiting Tone  */
  /* 440.19 Hz, -9.00 dBm0, 0.00 Hz, -100.00 dBm0 */
  0x09, 0xD4, 0x04, 0xB2, 0x1F, 0xA4, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x01, 0x2C, /* Alert Tone Part 2 - Tone+Cadence = 300ms */
  0x00, 0x06, 0x00, 0x0A, /* Silence Interval for 10ms */
  0x00, 0x03, /* Unmute the far end */
  0x00, 0x06, 0x01, 0x2C, /* Silence Interval for 300ms */
  0x00, 0x02, /* Mute the far end */
  0x00, 0x06, 0x00, 0x41, /* Silence Interval for 65ms */
  0x00, 0x04, /* Alert Tone */
  /* Caller ID Alert Tone (CAS) - Used in Many Country Caller ID Profiles */
  /* 2129.88 Hz, -8.00 dBm0, 2750.24 Hz, -8.00 dBm0 */
  0x09, 0xD4, 0x16, 0xB8, 0x23, 0x80, 0x1D, 0x56, 0x23, 0x80,
  0x00, 0x05, 0x00, 0x50, /* Alert Tone Part 2 - Tone+Cadence = 80ms */
  0x00, 0x08, 0x00, 0xA5, 0x00, 0xD0, 0x00, 0x00, /* Detect Tone A | D, Timeout = 165ms */
  0x00, 0x06, 0x00, 0x78, /* Silence Interval for 120ms */
  0x00, 0x0A, 0x00, 0x41, /* Mark Signal for 65ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x06, 0x00, 0x50, /* Silence Interval for 80ms */
  0x00, 0x03, /* Unmute the far end */
  0x00, 0x0D  /* End of Transmission */
};

/* UK Caller ID (Type 1 - On-Hook) - ETSI FSK */
const VpProfileDataType CID_TYPE1_GB[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-6.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x3A, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x2A, 0x30, 0x0D, 0xDD, 0x2A, 0x30, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x2A, /* Length of Elements Data */
  0x00, 0x06, 0x00, 0x96, /* Silence Interval for 150ms */
  0x00, 0x04, /* Alert Tone */
  /* Caller ID Alert Tone (CAS) - Used in Many Country Caller ID Profiles */
  /* 2129.88 Hz, -8.00 dBm0, 2750.24 Hz, -8.00 dBm0 */
  0x09, 0xD4, 0x16, 0xB8, 0x23, 0x80, 0x1D, 0x56, 0x23, 0x80,
  0x00, 0x05, 0x00, 0x64, /* Alert Tone Part 2 - Tone+Cadence = 100ms */
  0x00, 0x07, 0x00, 0xFA, 0x00, 0x14, /* Silence Interval for 250ms, Masked-hook Interval for 20ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x41, /* Mark Signal for 65ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x06, 0x00, 0xFA, /* Silence Interval for 250ms */
  0x00, 0x0D  /* End of Transmission */
};

/* UK Caller ID (Type 2 - Off-Hook) - ETSI FSK */
const VpProfileDataType CID_TYPE2_GB[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-6.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x40, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x2A, 0x30, 0x0D, 0xDD, 0x2A, 0x30, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x30, /* Length of Elements Data */
  0x00, 0x02, /* Mute the far end */
  0x00, 0x06, 0x00, 0x2D, /* Silence Interval for 45ms */
  0x00, 0x04, /* Alert Tone */
  /* Caller ID Alert Tone (CAS) - Used in Many Country Caller ID Profiles */
  /* 2129.88 Hz, -8.00 dBm0, 2750.24 Hz, -8.00 dBm0 */
  0x09, 0xD4, 0x16, 0xB8, 0x23, 0x80, 0x1D, 0x56, 0x23, 0x80,
  0x00, 0x05, 0x00, 0x55, /* Alert Tone Part 2 - Tone+Cadence = 85ms */
  0x00, 0x08, 0x00, 0xA0, 0x00, 0xD0, 0x00, 0x00, /* Detect Tone A | D, Timeout = 160ms */
  0x00, 0x06, 0x00, 0x64, /* Silence Interval for 100ms */
  0x00, 0x0A, 0x00, 0x41, /* Mark Signal for 65ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x06, 0x00, 0x14, /* Silence Interval for 20ms */
  0x00, 0x03, /* Unmute the far end */
  0x00, 0x0D  /* End of Transmission */
};

/* Greece Caller ID (Type 1 - On-Hook) - ETSI FSK */
const VpProfileDataType CID_TYPE1_GR[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-6.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x2A, 0x30, 0x0D, 0xDD, 0x2A, 0x30, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x02, 0x58, /* Silence Interval for 600ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* Hong Kong SAR Caller ID (Type 1 - On-Hook) - Telcordia FSK */
const VpProfileDataType CID_TYPE1_HK[] =
{
  /* Space=2200Hz, Mark=1200Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x17, 0x77, 0x25, 0x99, 0x0C, 0xCC, 0x25, 0x99, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x01, 0x2C, /* Silence Interval for 300ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* Hong Kong SAR Caller ID (Type 2 - Off-Hook) - Telcordia FSK */
const VpProfileDataType CID_TYPE2_HK[] =
{
  /* Space=2200Hz, Mark=1200Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x54, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x17, 0x77, 0x25, 0x99, 0x0C, 0xCC, 0x25, 0x99, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x44, /* Length of Elements Data */
  0x00, 0x02, /* Mute the far end */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x04, /* Alert Tone */
  /* Hong Kong SAR Call Waiting Tone */
  /* 440.19 Hz, -10.00 dBm0, 0.00 Hz, -100.00 dBm0 */
  0x09, 0xD4, 0x04, 0xB2, 0x1C, 0x33, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x01, 0x2C, /* Alert Tone Part 2 - Tone+Cadence = 300ms */
  0x00, 0x06, 0x00, 0x32, /* Silence Interval for 50ms */
  0x00, 0x04, /* Alert Tone */
  /* Caller ID Alert Tone (CAS) - Used in Many Country Caller ID Profiles */
  /* 2129.88 Hz, -8.00 dBm0, 2750.24 Hz, -8.00 dBm0 */
  0x09, 0xD4, 0x16, 0xB8, 0x23, 0x80, 0x1D, 0x56, 0x23, 0x80,
  0x00, 0x05, 0x00, 0x50, /* Alert Tone Part 2 - Tone+Cadence = 80ms */
  0x00, 0x08, 0x00, 0xA0, 0x00, 0xD0, 0x00, 0x00, /* Detect Tone A | D, Timeout = 160ms */
  0x00, 0x06, 0x00, 0x32, /* Silence Interval for 50ms */
  0x00, 0x0A, 0x00, 0x41, /* Mark Signal for 65ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x03, /* Unmute the far end */
  0x00, 0x0D  /* End of Transmission */
};

/* Ireland Caller ID (Type 1 - On-Hook) - ETSI FSK */
const VpProfileDataType CID_TYPE1_IE[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-6.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x2A, 0x30, 0x0D, 0xDD, 0x2A, 0x30, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x01, 0xC2, /* Silence Interval for 450ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* Ireland Caller ID (Type 2 - Off-Hook) - ETSI FSK */
const VpProfileDataType CID_TYPE2_IE[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-6.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x54, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x2A, 0x30, 0x0D, 0xDD, 0x2A, 0x30, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x44, /* Length of Elements Data */
  0x00, 0x02, /* Mute the far end */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x04, /* Alert Tone */
  /* Ireland Call Waiting Tone */
  /* 425.17 Hz, -4.00 dBm0, 0.00 Hz, -100.00 dBm0 */
  0x09, 0xD4, 0x04, 0x89, 0x38, 0x43, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x01, 0x2C, /* Alert Tone Part 2 - Tone+Cadence = 300ms */
  0x00, 0x06, 0x00, 0xC8, /* Silence Interval for 200ms */
  0x00, 0x04, /* Alert Tone */
  /* Caller ID Alert Tone (CAS) - Used in Many Country Caller ID Profiles */
  /* 2129.88 Hz, -8.00 dBm0, 2750.24 Hz, -8.00 dBm0 */
  0x09, 0xD4, 0x16, 0xB8, 0x23, 0x80, 0x1D, 0x56, 0x23, 0x80,
  0x00, 0x05, 0x00, 0x50, /* Alert Tone Part 2 - Tone+Cadence = 80ms */
  0x00, 0x08, 0x00, 0xA0, 0x00, 0xD0, 0x00, 0x00, /* Detect Tone A | D, Timeout = 160ms */
  0x00, 0x06, 0x00, 0x32, /* Silence Interval for 50ms */
  0x00, 0x0A, 0x00, 0x41, /* Mark Signal for 65ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x03, /* Unmute the far end */
  0x00, 0x0D  /* End of Transmission */
};

/* Israel Caller ID (Type 1 - On-Hook) - Telcordia FSK */
const VpProfileDataType CID_TYPE1_IL[] =
{
  /* Space=2200Hz, Mark=1200Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x17, 0x77, 0x25, 0x99, 0x0C, 0xCC, 0x25, 0x99, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x01, 0x2C, /* Silence Interval for 300ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* India Caller ID (Type 1 - On-Hook) - ETSI DTMF */
const VpProfileDataType CID_TYPE1_IN[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-4.00dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x38, 0x42, 0x0D, 0xDD, 0x38, 0x42, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x01, /* Line Reversal */
  0x00, 0x06, 0x01, 0x2C, /* Silence Interval for 300ms */
  0x00, 0x0E, /* Message Data (DTMF Format) */
  0x00, 0x06, 0x00, 0x14, /* Silence Interval for 20ms */
  0x00, 0x01, /* Line Reversal */
  0x00, 0x0D  /* End of Transmission */
};

/* Iceland Caller ID (Type 1 - On-Hook) - ETSI DTMF */
const VpProfileDataType CID_TYPE1_IS[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-4.00dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x38, 0x42, 0x0D, 0xDD, 0x38, 0x42, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x01, /* Line Reversal */
  0x00, 0x06, 0x01, 0x2C, /* Silence Interval for 300ms */
  0x00, 0x0E, /* Message Data (DTMF Format) */
  0x00, 0x06, 0x00, 0x14, /* Silence Interval for 20ms */
  0x00, 0x01, /* Line Reversal */
  0x00, 0x0D  /* End of Transmission */
};

/* Italy Caller ID (Type 1 - On-Hook) - ETSI FSK */
const VpProfileDataType CID_TYPE1_IT[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-6.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x2A, 0x30, 0x0D, 0xDD, 0x2A, 0x30, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x02, 0x58, /* Silence Interval for 600ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* Japan Caller ID (Type 1 - On-Hook) - NTT FSK */
const VpProfileDataType CID_TYPE1_JP[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-8.00dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x23, 0x7F, 0x0D, 0xDD, 0x23, 0x7F, /* MPI Data */
  0x00, 0x00, /* Checksum Part of Message Data */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x01, 0xF4, /* Silence Interval for 500ms */
  0x00, 0x02, /* Mute the far end */
  0x00, 0x0A, 0x00, 0x32, /* Mark Signal for 50ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x03, /* Unmute the far end */
  0x00, 0x0D  /* End of Transmission */
};

/* Japan Caller ID (Type 2 - Off-Hook / Phone Catch) - NTT FSK */
const VpProfileDataType CID_TYPE2_JP[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-6.00dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x60, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x2C, 0xB0, 0x0D, 0xDD, 0x2C, 0xB0, /* MPI Data */
  0x00, 0x00, /* Checksum Part of Message Data */
  0x00, 0x50, /* Length of Elements Data */
  0x00, 0x02, /* Mute the far end */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x04, /* Alert Tone */
  /* Japan Call Waiting Tone */
  /* 399.90 Hz, -8.00 dBm0, 416.02 Hz, -14.00 dBm0 */
  0x09, 0xD4, 0x04, 0x44, 0x23, 0x80, 0x04, 0x70, 0x11, 0xCB,
  0x00, 0x05, 0x01, 0xF4, /* Alert Tone Part 2 - Tone+Cadence = 500ms */
  0x00, 0x06, 0x01, 0x2C, /* Silence Interval for 300ms */
  0x00, 0x04, /* Alert Tone */
  /* Japan Caller ID Alert Tone - Digit C */
  /* 852.17 Hz, -8.00 dBm0, 1632.93 Hz, -6.00 dBm0 */
  0x09, 0xD4, 0x09, 0x17, 0x23, 0x80, 0x11, 0x6B, 0x2C, 0xB1,
  0x00, 0x05, 0x00, 0x5A, /* Alert Tone Part 2 - Tone+Cadence = 90ms */
  0x00, 0x06, 0x00, 0x5A, /* Silence Interval for 90ms */
  0x00, 0x04, /* Alert Tone */
  /* Japan Caller ID Alert Tone - Digit D */
  /* 941.16 Hz, -8.00 dBm0, 1632.93 Hz, -6.00 dBm0 */
  0x09, 0xD4, 0x0A, 0x0A, 0x23, 0x80, 0x11, 0x6B, 0x2C, 0xB1,
  0x00, 0x05, 0x00, 0x5A, /* Alert Tone Part 2 - Tone+Cadence = 90ms */
  0x00, 0x06, 0x00, 0xC8, /* Silence Interval for 200ms */
  0x00, 0x0A, 0x00, 0x32, /* Mark Signal for 50ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x03, /* Unmute the far end */
  0x00, 0x0D  /* End of Transmission */
};

/* S. Korea Caller ID (Type 1 - On-Hook) - Telcordia FSK */
const VpProfileDataType CID_TYPE1_KR[] =
{
  /* Space=2200Hz, Mark=1200Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x17, 0x77, 0x25, 0x99, 0x0C, 0xCC, 0x25, 0x99, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x01, 0x2C, /* Silence Interval for 300ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* S. Korea Caller ID (Type 2 - Off-Hook) - Telcordia FSK */
const VpProfileDataType CID_TYPE2_KR[] =
{
  /* Space=2200Hz, Mark=1200Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x54, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x17, 0x77, 0x25, 0x99, 0x0C, 0xCC, 0x25, 0x99, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x44, /* Length of Elements Data */
  0x00, 0x02, /* Mute the far end */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x04, /* Alert Tone */
  /* S. Korea Call Waiting Tone */
  /* 350.10 Hz, -10.00 dBm0, 440.19 Hz, -10.00 dBm0 */
  0x09, 0xD4, 0x03, 0xBC, 0x1C, 0x33, 0x04, 0xB2, 0x1C, 0x33,
  0x00, 0x05, 0x01, 0x2C, /* Alert Tone Part 2 - Tone+Cadence = 300ms */
  0x00, 0x06, 0x00, 0x4B, /* Silence Interval for 75ms */
  0x00, 0x04, /* Alert Tone */
  /* Caller ID Alert Tone (CAS) - Used in Many Country Caller ID Profiles */
  /* 2129.88 Hz, -8.00 dBm0, 2750.24 Hz, -8.00 dBm0 */
  0x09, 0xD4, 0x16, 0xB8, 0x23, 0x80, 0x1D, 0x56, 0x23, 0x80,
  0x00, 0x05, 0x00, 0x50, /* Alert Tone Part 2 - Tone+Cadence = 80ms */
  0x00, 0x08, 0x00, 0xA5, 0x00, 0xD0, 0x00, 0x00, /* Detect Tone A | D, Timeout = 165ms */
  0x00, 0x06, 0x00, 0x64, /* Silence Interval for 100ms */
  0x00, 0x0A, 0x00, 0x41, /* Mark Signal for 65ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x03, /* Unmute the far end */
  0x00, 0x0D  /* End of Transmission */
};

/* Mexico Caller ID (Type 1 - On-Hook) - Telcordia FSK */
const VpProfileDataType CID_TYPE1_MX[] =
{
  /* Space=2200Hz, Mark=1200Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x17, 0x77, 0x25, 0x99, 0x0C, 0xCC, 0x25, 0x99, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x01, 0x2C, /* Silence Interval for 300ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* Netherlands Caller ID (Type 1 - On-Hook) - ETSI DTMF */
const VpProfileDataType CID_TYPE1_NL[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-4.00dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x38, 0x42, 0x0D, 0xDD, 0x38, 0x42, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x01, /* Line Reversal */
  0x00, 0x06, 0x01, 0xF4, /* Silence Interval for 500ms */
  0x00, 0x0E, /* Message Data (DTMF Format) */
  0x00, 0x06, 0x00, 0x14, /* Silence Interval for 20ms */
  0x00, 0x01, /* Line Reversal */
  0x00, 0x0D  /* End of Transmission */
};

/* Norway Caller ID (Type 1 - On-Hook) - ETSI FSK */
const VpProfileDataType CID_TYPE1_NO[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-6.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x2A, 0x30, 0x0D, 0xDD, 0x2A, 0x30, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x02, 0x58, /* Silence Interval for 600ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* Norway Caller ID (Type 2 - Hook) - ETSI FSK */
const VpProfileDataType CID_TYPE2_NO[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-6.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x54, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x2A, 0x30, 0x0D, 0xDD, 0x2A, 0x30, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x44, /* Length of Elements Data */
  0x00, 0x02, /* Mute the far end */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x04, /* Alert Tone */
  /* Norway Call Waiting Tone */
  /* 425.17 Hz, -4.00 dBm0, 0.00 Hz, -100.00 dBm0 */
  0x09, 0xD4, 0x04, 0x89, 0x38, 0x43, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x01, 0x2C, /* Alert Tone Part 2 - Tone+Cadence = 300ms */
  0x00, 0x06, 0x00, 0xC8, /* Silence Interval for 200ms */
  0x00, 0x04, /* Alert Tone */
  /* Caller ID Alert Tone (CAS) - Used in Many Country Caller ID Profiles */
  /* 2129.88 Hz, -8.00 dBm0, 2750.24 Hz, -8.00 dBm0 */
  0x09, 0xD4, 0x16, 0xB8, 0x23, 0x80, 0x1D, 0x56, 0x23, 0x80,
  0x00, 0x05, 0x00, 0x50, /* Alert Tone Part 2 - Tone+Cadence = 80ms */
  0x00, 0x08, 0x00, 0xA0, 0x00, 0xD0, 0x00, 0x00, /* Detect Tone A | D, Timeout = 160ms */
  0x00, 0x06, 0x00, 0x32, /* Silence Interval for 50ms */
  0x00, 0x0A, 0x00, 0x41, /* Mark Signal for 65ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x03, /* Unmute the far end */
  0x00, 0x0D  /* End of Transmission */
};

/* New Zealand Caller ID (Type 1 - On-Hook) - Telcordia FSK */
const VpProfileDataType CID_TYPE1_NZ[] =
{
  /* Space=2200Hz, Mark=1200Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x17, 0x77, 0x25, 0x99, 0x0C, 0xCC, 0x25, 0x99, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x01, 0x90, /* Silence Interval for 400ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* New Zealand Caller ID (Type 2 - Off-Hook) - Telcordia FSK */
const VpProfileDataType CID_TYPE2_NZ[] =
{
  /* Space=2200Hz, Mark=1200Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x54, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x17, 0x77, 0x25, 0x99, 0x0C, 0xCC, 0x25, 0x99, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x44, /* Length of Elements Data */
  0x00, 0x02, /* Mute the far end */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x04, /* Alert Tone */
  /* New Zealand Call Waiting Tone */
  /* 399.90 Hz, -12.00 dBm0, 450.07 Hz, -12.00 dBm0 */
  0x09, 0xD4, 0x04, 0x44, 0x16, 0x66, 0x04, 0xCD, 0x16, 0x66,
  0x00, 0x05, 0x01, 0xF4, /* Alert Tone Part 2 - Tone+Cadence = 500ms */
  0x00, 0x06, 0x00, 0x64, /* Silence Interval for 100ms */
  0x00, 0x04, /* Alert Tone */
  /* Caller ID Alert Tone (CAS) - Used in Many Country Caller ID Profiles */
  /* 2129.88 Hz, -8.00 dBm0, 2750.24 Hz, -8.00 dBm0 */
  0x09, 0xD4, 0x16, 0xB8, 0x23, 0x80, 0x1D, 0x56, 0x23, 0x80,
  0x00, 0x05, 0x00, 0x50, /* Alert Tone Part 2 - Tone+Cadence = 80ms */
  0x00, 0x08, 0x00, 0xA0, 0x00, 0xD0, 0x00, 0x00, /* Detect Tone A | D, Timeout = 160ms */
  0x00, 0x06, 0x00, 0x32, /* Silence Interval for 50ms */
  0x00, 0x0A, 0x00, 0x41, /* Mark Signal for 65ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x03, /* Unmute the far end */
  0x00, 0x0D  /* End of Transmission */
};

/* Portugal Caller ID (Type 1 - On-Hook) - ETSI FSK */
const VpProfileDataType CID_TYPE1_PT[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-6.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x2A, 0x30, 0x0D, 0xDD, 0x2A, 0x30, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x02, 0x58, /* Silence Interval for 600ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* Portugal Caller ID (Type 2 - Off-Hook) - ETSI FSK */
const VpProfileDataType CID_TYPE2_PT[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-6.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x54, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x2A, 0x30, 0x0D, 0xDD, 0x2A, 0x30, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x44, /* Length of Elements Data */
  0x00, 0x02, /* Mute the far end */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x04, /* Alert Tone */
  /* Portugal Call Waiting Tone */
  /* 425.17 Hz, -12.00 dBm0, 0.00 Hz, -100.00 dBm0 */
  0x09, 0xD4, 0x04, 0x89, 0x16, 0x66, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x01, 0x2C, /* Alert Tone Part 2 - Tone+Cadence = 300ms */
  0x00, 0x06, 0x00, 0xC8, /* Silence Interval for 200ms */
  0x00, 0x04, /* Alert Tone */
  /* Caller ID Alert Tone (CAS) - Used in Many Country Caller ID Profiles */
  /* 2129.88 Hz, -8.00 dBm0, 2750.24 Hz, -8.00 dBm0 */
  0x09, 0xD4, 0x16, 0xB8, 0x23, 0x80, 0x1D, 0x56, 0x23, 0x80,
  0x00, 0x05, 0x00, 0x50, /* Alert Tone Part 2 - Tone+Cadence = 80ms */
  0x00, 0x08, 0x00, 0xA0, 0x00, 0xD0, 0x00, 0x00, /* Detect Tone A | D, Timeout = 160ms */
  0x00, 0x06, 0x00, 0x32, /* Silence Interval for 50ms */
  0x00, 0x0A, 0x00, 0x41, /* Mark Signal for 65ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x03, /* Unmute the far end */
  0x00, 0x0D  /* End of Transmission */
};

/* Sweden Caller ID (Type 1 - On-Hook) - ETSI DTMF */
const VpProfileDataType CID_TYPE1_SE[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-4.00dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x38, 0x42, 0x0D, 0xDD, 0x38, 0x42, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x01, /* Line Reversal */
  0x00, 0x06, 0x01, 0x2C, /* Silence Interval for 300ms */
  0x00, 0x0E, /* Message Data (DTMF Format) */
  0x00, 0x06, 0x00, 0x14, /* Silence Interval for 20ms */
  0x00, 0x01, /* Line Reversal */
  0x00, 0x0D  /* End of Transmission */
};

/* Singapore Caller ID (Type 1 - On-Hook) - Telcordia FSK */
const VpProfileDataType CID_TYPE1_SG[] =
{
  /* Space=2200Hz, Mark=1200Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x17, 0x77, 0x25, 0x99, 0x0C, 0xCC, 0x25, 0x99, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x01, 0xF4, /* Silence Interval for 500ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* Singapore Caller ID (Type 2 - Off-Hook) - Telcordia FSK */
const VpProfileDataType CID_TYPE2_SG[] =
{
  /* Space=2200Hz, Mark=1200Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x68, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x17, 0x77, 0x25, 0x99, 0x0C, 0xCC, 0x25, 0x99, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x58, /* Length of Elements Data */
  0x00, 0x02, /* Mute the far end */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x04, /* Alert Tone */
  /* Singapore Call Waiting Tone */
  /* 425.17 Hz, -9.00 dBm0, 0.00 Hz, -100.00 dBm0 */
  0x09, 0xD4, 0x04, 0x89, 0x1F, 0xA4, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x01, 0x2C, /* Alert Tone Part 2 - Tone+Cadence = 300ms */
  0x00, 0x06, 0x00, 0xC8, /* Silence Interval for 200ms */
  0x00, 0x04, /* Alert Tone */
  /* Singapore Call Waiting Tone */
  /* 425.17 Hz, -9.00 dBm0, 0.00 Hz, -100.00 dBm0 */
  0x09, 0xD4, 0x04, 0x89, 0x1F, 0xA4, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x01, 0x2C, /* Alert Tone Part 2 - Tone+Cadence = 300ms */
  0x00, 0x06, 0x01, 0x2C, /* Silence Interval for 300ms */
  0x00, 0x04, /* Alert Tone */
  /* Caller ID Alert Tone (CAS) - Used in Many Country Caller ID Profiles */
  /* 2129.88 Hz, -8.00 dBm0, 2750.24 Hz, -8.00 dBm0 */
  0x09, 0xD4, 0x16, 0xB8, 0x23, 0x80, 0x1D, 0x56, 0x23, 0x80,
  0x00, 0x05, 0x00, 0x50, /* Alert Tone Part 2 - Tone+Cadence = 80ms */
  0x00, 0x08, 0x00, 0xA0, 0x00, 0xD0, 0x00, 0x00, /* Detect Tone A | D, Timeout = 160ms */
  0x00, 0x06, 0x00, 0x64, /* Silence Interval for 100ms */
  0x00, 0x0A, 0x00, 0x41, /* Mark Signal for 65ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x03, /* Unmute the far end */
  0x00, 0x0D  /* End of Transmission */
};

/* Turkey Caller ID (Type 1 - On-Hook) - ETSI FSK */
const VpProfileDataType CID_TYPE1_TK[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-6.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x2A, 0x30, 0x0D, 0xDD, 0x2A, 0x30, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x01, 0x2C, /* Silence Interval for 300ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* Taiwan Caller ID (Type 1 - On-Hook) - Telcordia FSK */
const VpProfileDataType CID_TYPE1_TW[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-6.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x36, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x2A, 0x30, 0x0D, 0xDD, 0x2A, 0x30, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x26, /* Length of Elements Data */
  0x00, 0x06, 0x00, 0x1E, /* Silence Interval for 30ms */
  0x00, 0x04, /* Alert Tone */
  /* Caller ID Alert Tone (CAS) - Used in Many Country Caller ID Profiles */
  /* 2129.88 Hz, -8.00 dBm0, 2750.24 Hz, -8.00 dBm0 */
  0x09, 0xD4, 0x16, 0xB8, 0x23, 0x80, 0x1D, 0x56, 0x23, 0x80,
  0x00, 0x05, 0x00, 0x64, /* Alert Tone Part 2 - Tone+Cadence = 100ms */
  0x00, 0x07, 0x00, 0xC8, 0x00, 0x14, /* Silence Interval for 200ms, Masked-hook Interval for 20ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* USA Caller ID (Type 1 - On-Hook) - Telcordia FSK */
const VpProfileDataType CID_TYPE1_US[] =
{
  /* Space=2200Hz, Mark=1200Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x17, 0x77, 0x25, 0x99, 0x0C, 0xCC, 0x25, 0x99, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x01, 0x2C, /* Silence Interval for 300ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/* USA Caller ID (Type 2 - Off-Hook) - Telcordia FSK */
const VpProfileDataType CID_TYPE2_US[] =
{
  /* Space=2200Hz, Mark=1200Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x54, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x17, 0x77, 0x25, 0x99, 0x0C, 0xCC, 0x25, 0x99, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x44, /* Length of Elements Data */
  0x00, 0x02, /* Mute the far end */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x04, /* Alert Tone */
  /* USA Call Waiting Tone */
  /* 440.19 Hz, -7.00 dBm0, 0.00 Hz, -100.00 dBm0 */
  0x09, 0xD4, 0x04, 0xB2, 0x27, 0xD5, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x01, 0x2C, /* Alert Tone Part 2 - Tone+Cadence = 300ms */
  0x00, 0x06, 0x00, 0x32, /* Silence Interval for 50ms */
  0x00, 0x04, /* Alert Tone */
  /* Caller ID Alert Tone (CAS) - Used in Many Country Caller ID Profiles */
  /* 2129.88 Hz, -8.00 dBm0, 2750.24 Hz, -8.00 dBm0 */
  0x09, 0xD4, 0x16, 0xB8, 0x23, 0x80, 0x1D, 0x56, 0x23, 0x80,
  0x00, 0x05, 0x00, 0x50, /* Alert Tone Part 2 - Tone+Cadence = 80ms */
  0x00, 0x08, 0x00, 0xA0, 0x00, 0xD0, 0x00, 0x00, /* Detect Tone A | D, Timeout = 160ms */
  0x00, 0x06, 0x00, 0x32, /* Silence Interval for 50ms */
  0x00, 0x0A, 0x00, 0x41, /* Mark Signal for 65ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x06, 0x00, 0x05, /* Silence Interval for 5ms */
  0x00, 0x03, /* Unmute the far end */
  0x00, 0x0D  /* End of Transmission */
};

/* South Africa Caller ID (Type 1 - On-Hook) - ETSI FSK */
const VpProfileDataType CID_TYPE1_ZA[] =
{
  /* Space=2100Hz, Mark=1300Hz, Amp=-7.50dBm */
  /* Caller ID Profile */
  0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
  0x09, 0xD4, /* MPI Length and Command */
  0x16, 0x66, 0x25, 0x99, 0x0D, 0xDD, 0x25, 0x99, /* MPI Data */
  0x00, 0x01, /* Checksum Computed by device/API */
  0x00, 0x10, /* Length of Elements Data */
  0x00, 0x06, 0x01, 0xC2, /* Silence Interval for 450ms */
  0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
  0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
  0x00, 0x0B, /* Message Data (FSK Format) */
  0x00, 0x0D  /* End of Transmission */
};

/************** Metering_Profile **************/

/* Metering, 12kHz 0.5Vrms */
const VpProfileDataType METER_12KHZ_RF14[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x07, 0x00,       /* Metering Profile for ZL88702 device */
   0x08,                   /* Profile length = 8 + 4 = 12 bytes */
   0x00,                   /* Profile format version = 0 */
   0x05,                   /* MPI section length = 5 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD0, 0x21,             /* MTRF = 12 kHz; MLIM = 511.5/723.4 mV RMS/peak */
         0x2E,             /* SOREV = Smooth; MTRSL = 3.128/4.424 mA RMS/peak */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0x00                    /* Reserved */
};

/* Metering, 16kHz 0.5Vrms */
const VpProfileDataType METER_16KHZ_RF14[] =
{
/* Profile Header ---------------------------------------------------------- */
   0x0D, 0x07, 0x00,       /* Metering Profile for ZL88702 device */
   0x08,                   /* Profile length = 8 + 4 = 12 bytes */
   0x00,                   /* Profile format version = 0 */
   0x05,                   /* MPI section length = 5 bytes */
/* Raw MPI Section --------------------------------------------------------- */
   0xD0, 0x9C,             /* MTRF = 16 kHz; MLIM = 515.2/728.6 mV RMS/peak */
         0x31,             /* SOREV = Smooth; MTRSL = 3.332/4.712 mA RMS/peak */
         0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
   0x00                    /* Reserved */
};

/* end of file ZLR963222H_135VIB_Profile.c */
