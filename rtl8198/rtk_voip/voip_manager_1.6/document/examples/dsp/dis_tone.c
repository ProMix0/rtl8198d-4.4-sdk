#include "voip_manager.h"

/**
 * @brief This sample codes show how you to set dis-connect tone detection parameters.<br>
 *        In this codes, it sets two kinds of tones as dis-connect tone parameters.<br>
 *        If these two tone are detected at FXO port, DSP will report dis-connect tone event.
 * @author thlin@realtek.com, version 0.1 @ 2011/06/01
 * @see TstVoipdistonedet_parm do_mgr_VOIP_MGR_SET_DIS_TONE_DET
 */
int main(void)
{
	TstVoipdistonedet_parm voip_ptr;
	
	rtk_GetVoIPFeature();

	voip_ptr.distone_num = 2;
	
	// For 1st Set, to detect USA Stutter Dial Tone (350Hz+440Hz, 0.1s on, 0.1s off)
#if 0	
	voip_ptr.d1Freq1 = 350;
	voip_ptr.d1Freq2 = 440;
	voip_ptr.d1Accur = 13;
	voip_ptr.d1Level = 700;
	voip_ptr.d1ONup = 15;
	voip_ptr.d1ONlow = 5;
	voip_ptr.d1OFFup = 15;
	voip_ptr.d1OFFlow = 5;
#endif

	voip_ptr.tone1_frequency1 = 350;
	voip_ptr.tone1_frequency2 = 440;
	voip_ptr.tone1_accuracy = 13;
	voip_ptr.tone1_level = 700;
	voip_ptr.tone1_distone_on_up_limit = 15;
	voip_ptr.tone1_distone_on_low_limit = 5;
	voip_ptr.tone1_distone_off_up_limit = 15;
	voip_ptr.tone1_distone_off_low_limit = 5;

	
	// For 2nd Set, to detect USA Busy Tone (480Hz+620Hz, 0.5s on, 0.5s off)
#if 0
	voip_ptr.d2Freq1 = 480;
	voip_ptr.d2Freq2 = 620;
	voip_ptr.d2Accur = 16;
	voip_ptr.d2Level = 800;
	voip_ptr.d2ONup = 54;
	voip_ptr.d2ONlow = 46;
	voip_ptr.d2OFFup = 54;
	voip_ptr.d2OFFlow = 46;
#endif
	voip_ptr.tone2_frequency1 = 480;
	voip_ptr.tone2_frequency2 = 620;
	voip_ptr.tone2_accuracy = 16;
	voip_ptr.tone2_level = 800;
	voip_ptr.tone2_distone_on_up_limit = 54;
	voip_ptr.tone2_distone_on_low_limit = 46;
	voip_ptr.tone2_distone_off_up_limit = 54;
	voip_ptr.tone2_distone_off_low_limit = 46;


	rtk_SetDisTonePara(&voip_ptr);
	return 0;
}

