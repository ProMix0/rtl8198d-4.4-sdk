#include <stdlib.h>
#include <stdio.h>
#include "voip_manager.h"

int main(int argc, char *argv[])
{
	TstRtpRtcpStatistics stRtpRtcpStatistics;
	
	uint32 chid  = 0;
	uint32 mid   = 0;
	uint32 reset = 0;
	
	rtk_GetVoIPFeature();	
		
	if (argc == 4)
	{
		chid  = atoi(argv[1]);
		mid   = atoi(argv[2]);
		reset = atoi(argv[3]);
		
		rtk_GetRtpRtcpStatistics(chid , mid , reset , &stRtpRtcpStatistics);
		
		printf(
		"ch_id=%u \n"
		"m_id=%u \n"
		"bResetStatistics=%u \n"
		"nTxPkts=%u \n"
		"nTxBytes=%u \n"
		"nRxPkts=%u \n"
		"nRxBytes=%u \n"
		"nLost=%u \n"
		"nMaxFractionLost=%u \n"
		"nMinFractionLost=%u \n"
		"nAvgFractionLost=%u \n"
		"nCurFractionLost=%u \n"
		"nDiscarded=%u \n"
		"nTxRtcpPkts=%u \n"
		"nRxRtcpPkts=%u \n"
		"nTxRtcpXrPkts=%u \n"
		"nRxRtcpXrPkts=%u \n"
		"nOverRuns=%u \n"
		"nUnderRuns=%u \n"
		"nMaxJitter=%u \n"
		"nMinJitter=%u \n"
		"nAvgJitter=%u \n"
		"nCurJitter=%u \n"
		"nMaxRtcpTime=%u \n"
		"\n"
		,stRtpRtcpStatistics.ch_id
		,stRtpRtcpStatistics.m_id
		,stRtpRtcpStatistics.bResetStatistics
		,stRtpRtcpStatistics.nTxPkts
		,stRtpRtcpStatistics.nTxBytes
		,stRtpRtcpStatistics.nRxPkts
		,stRtpRtcpStatistics.nRxBytes
		,stRtpRtcpStatistics.nLost
		,stRtpRtcpStatistics.nMaxFractionLost
		,stRtpRtcpStatistics.nMinFractionLost
		,stRtpRtcpStatistics.nAvgFractionLost
		,stRtpRtcpStatistics.nCurFractionLost
		,stRtpRtcpStatistics.nDiscarded
		,stRtpRtcpStatistics.nTxRtcpPkts
		,stRtpRtcpStatistics.nRxRtcpPkts
		,stRtpRtcpStatistics.nTxRtcpXrPkts
		,stRtpRtcpStatistics.nRxRtcpXrPkts
		,stRtpRtcpStatistics.nOverRuns
		,stRtpRtcpStatistics.nUnderRuns
		,stRtpRtcpStatistics.nMaxJitter
		,stRtpRtcpStatistics.nMinJitter
		,stRtpRtcpStatistics.nAvgJitter
		,stRtpRtcpStatistics.nCurJitter
		,stRtpRtcpStatistics.nMaxRtcpTime
		);
	}
	else
	{
		printf("Usage error!\n");
		printf("rtp_rtcp_stats chid mid reset \n");
	}
	
	return 0;
}


