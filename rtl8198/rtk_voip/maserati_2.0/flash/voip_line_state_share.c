#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <signal.h>
#include <errno.h>
#include "voip_flash.h"

#define DBG(...) ((void)0)
//#define DBG(fmt, ...) fprintf(stderr, "<%s:%d> "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

//static int sem_id = -1;
static int shm_id = -1;
//static int g_nShareSize = 0;
static voip_state_share_t *g_shmVoIPstate = NULL;

#if 0
static int sem_wait(const int sem_num)
{
	struct sembuf sem_b;

	sem_b.sem_num = sem_num;
	sem_b.sem_op = -1;
	sem_b.sem_flg = SEM_UNDO;
	return semop(sem_id, &sem_b, 1);
}

static int sem_signal(const int sem_num)
{
	struct sembuf sem_b;

	sem_b.sem_num = sem_num;
	sem_b.sem_op = 1;
	sem_b.sem_flg = SEM_UNDO;
	return semop(sem_id, &sem_b, 1);
}
#endif
int voip_line_state_share_get(voip_state_share_t **pVoIPShare)
{
	if (g_shmVoIPstate){
		*pVoIPShare=g_shmVoIPstate;
	}
	else{
		fprintf(stderr, "get g_shmVoIPstate = NULL\n");
		return -1;
	}
	return 0;
}
void voip_line_state_share_id_set(int chid, id_share_t index, void * pData)
{
	DBG("chid =%d index=%d \n", chid ,index);
	
	if(chid>=VOIP_PORTS)
		return;

	if (g_shmVoIPstate)
{
		switch(index){
			case OMCI_SHARE_SIP_STATUS:
				DBG("OMCI_SHARE_SIP_STATUS value =[%d] \n", *((unsigned char *)pData));
				g_shmVoIPstate->SIPStatus[chid]=*((unsigned char *)pData);
				break;			
			case OMCI_SHARE_HOOK_STATE:
				DBG("OMCI_SHARE_HOOK_STATE value =[%d] \n", *((unsigned char *)pData));
				g_shmVoIPstate->HookState[chid]=*((unsigned char *)pData);
				break;
			case OSGI_VOIP_PHONENUMBER:
				strncpy(g_shmVoIPstate->PhoneNumber[chid], (char *)pData, 40);
				DBG("OSGI_VOIP_PHONENUMBER value =[%s]  \n", (char *)pData);
				break;
			case OSGI_VOIP_LINESTATUS:
				strncpy(g_shmVoIPstate->LineStatus[chid], (char *)pData, 32);
				DBG("OSGI_VOIP_LINESTATUS value =[%s]  \n", (char *)pData);
				break;
			//9.9.11 VoIP line status		
			case OMCI_SHARE_LS_VOIPCODECUSED: 
				DBG("OMCI_SHARE_LS_VOIPCODECUSED value =[%d]  \n", *((unsigned short *)pData));
				g_shmVoIPstate->VOIPLineStatus[chid].voipCodecUsed=*((unsigned short *)pData);
				break;
			case OMCI_SHARE_LS_VOIPVOICESERVERSTATUS:
				DBG("OMCI_SHARE_LS_VOIPVOICESERVERSTATUS value =[%d] \n", *((unsigned char *)pData));
				g_shmVoIPstate->VOIPLineStatus[chid].voipVoiceServerStatus=*((unsigned char *)pData);
				break;
			case OMCI_SHARE_LS_VOIPPORTSESSIONTYPE: 
				DBG("OMCI_SHARE_LS_VOIPPORTSESSIONTYPE value =[%d]  \n", *((unsigned char *)pData));
				g_shmVoIPstate->VOIPLineStatus[chid].voipPortSessionType=*((unsigned char *)pData);
				break;
			case OMCI_SHARE_LS_VOIPCALL1PACKETPERIOD: 
				DBG("OMCI_SHARE_LS_VOIPCALL1PACKETPERIOD value =[%d]  \n", *((unsigned short *)pData));
				g_shmVoIPstate->VOIPLineStatus[chid].voipCall1PacketPeriod=*((unsigned short *)pData);
				break;
			case OMCI_SHARE_LS_VOIPCALL2PACKETPERIOD: 
				DBG("OMCI_SHARE_LS_VOIPCALL2PACKETPERIOD value =[%d]  \n", *((unsigned short *)pData));
				g_shmVoIPstate->VOIPLineStatus[chid].voipCall2PacketPeriod=*((unsigned short *)pData);
				break;
			case OMCI_SHARE_LS_VOIPCALL1DESTADDR:
				DBG("OMCI_SHARE_LS_VOIPCALL1DESTADDR value =[%s]  \n", (char *)pData);
				strncpy(g_shmVoIPstate->VOIPLineStatus[chid].voipCall1DestAddr, (char *)pData, OMCI_VOIP_CALL_DST_ADDR_LEN);
				break;
			case OMCI_SHARE_LS_VOIPCALL2DESTADDR: 
				DBG("OMCI_SHARE_LS_VOIPCALL2DESTADDR value =[%s]  \n", (char *)pData);
				strncpy(g_shmVoIPstate->VOIPLineStatus[chid].voipCall2DestAddr, (char *)pData, OMCI_VOIP_CALL_DST_ADDR_LEN);
				break;
			case OMCI_SHARE_LS_VOIPLINESTATE:
				DBG("OMCI_SHARE_LS_VOIPLINESTATE value =[%d]  \n", *((unsigned char *)pData));
				g_shmVoIPstate->VOIPLineStatus[chid].VoipLineState=*((unsigned char *)pData);				
				break;
			case OMCI_SHARE_LS_EMERGENCYCALLSTATUS:
				DBG("OMCI_SHARE_LS_EMERGENCYCALLSTATUS value =[%d]  \n", *((unsigned char *)pData));
				g_shmVoIPstate->VOIPLineStatus[chid].EmergencyCallStatus=*((unsigned char *)pData);				
				break;				
			//9.9.12 Call control performance monitoring history data	
			case OMCI_SHARE_CCPM_SETUPFAILURES:
				DBG("OMCI_SHARE_CCPM_SETUPFAILURES value =[%ld]  \n", *((unsigned long *)pData));
				g_shmVoIPstate->CCPMHistoryData[chid].setupFailures=*((unsigned long *)pData);
				break;
			case OMCI_SHARE_CCPM_SETUPTIMER:
				DBG("OMCI_SHARE_CCPM_SETUPTIMER org =%ld value =[%ld]  \n", g_shmVoIPstate->CCPMHistoryData[chid].setupTimer,*((unsigned long *)pData));
				if (*((unsigned long *)pData)>g_shmVoIPstate->CCPMHistoryData[chid].setupTimer)
				g_shmVoIPstate->CCPMHistoryData[chid].setupTimer=*((unsigned long *)pData);
				break;
			case OMCI_SHARE_CCPM_TERMINATEFAILURES:
				g_shmVoIPstate->CCPMHistoryData[chid].terminateFailures++;
				DBG("OMCI_SHARE_CCPM_TERMINATEFAILURES value =[%ld]  \n", g_shmVoIPstate->CCPMHistoryData[chid].terminateFailures);
				break;
			case OMCI_SHARE_CCPM_PORTRELEASE:
				g_shmVoIPstate->CCPMHistoryData[chid].portReleases++;
				DBG("OMCI_SHARE_CCPM_PORTRELEASE value =[%ld]  \n", g_shmVoIPstate->CCPMHistoryData[chid].portReleases);
				break;
			case OMCI_SHARE_CCPM_PORTOFFHOOKTIMER:
				DBG("OMCI_SHARE_CCPM_PORTOFFHOOKTIMER org =%d value =[%ld]  \n", g_shmVoIPstate->CCPMHistoryData[chid].portOffhookTimer,*((unsigned long *)pData));
				if(*((unsigned long *)pData)>g_shmVoIPstate->CCPMHistoryData[chid].portOffhookTimer)
				g_shmVoIPstate->CCPMHistoryData[chid].portOffhookTimer=*((unsigned long *)pData);
				break;
			//9.9.13 RTP performance monitoring history data	
			case OMCI_SHARE_RTPPM_RTPERRORS:
				DBG("OMCI_SHARE_RTPPM_RTPERRORS value =[%ld]  \n", *((unsigned long *)pData));
				g_shmVoIPstate->RTPPMHistoryData[chid].RTPErrors=g_shmVoIPstate->RTPPMHistoryData[chid].RTPErrors+*((unsigned long *)pData);
				DBG("g_shmVoIPstate->RTPPMHistoryData[chid].RTPErrors =[%ld]  \n", g_shmVoIPstate->RTPPMHistoryData[chid].RTPErrors);
				break;
			case OMCI_SHARE_RTPPM_PACKETLOSS:
				DBG("OMCI_SHARE_RTPPM_PACKETLOSS value =[%lu]  \n", *((unsigned long *)pData));
				g_shmVoIPstate->RTPPMHistoryData[chid].packetLoss=g_shmVoIPstate->RTPPMHistoryData[chid].packetLoss+*((unsigned long *)pData);
				DBG("g_shmVoIPstate->RTPPMHistoryData[chid].packetLoss value =[%lu]  \n", g_shmVoIPstate->RTPPMHistoryData[chid].packetLoss);
				break;				
			case OMCI_SHARE_RTPPM_MAXIMUMJITTER:
				DBG("OMCI_SHARE_RTPPM_MAXIMUMJITTER value =[%lu]  \n", *((unsigned long *)pData));
				if(*((unsigned long *)pData)>g_shmVoIPstate->RTPPMHistoryData[chid].maximumJitter)
				g_shmVoIPstate->RTPPMHistoryData[chid].maximumJitter=*((unsigned long *)pData);
				break;				
			case OMCI_SHARE_RTPPM_MAXIMUMTIMERTCP:
				DBG("OMCI_SHARE_RTPPM_MAXIMUMTIMERTCP value =[%lu]  \n", *((unsigned long *)pData));
				if(*((unsigned long *)pData)>g_shmVoIPstate->RTPPMHistoryData[chid].maximumTimeRTCP)
				g_shmVoIPstate->RTPPMHistoryData[chid].maximumTimeRTCP=*((unsigned long *)pData);
				break;
			case OMCI_SHARE_RTPPM_BUFFERUNDERFLOWS:
				DBG("OMCI_SHARE_RTPPM_BUFFERUNDERFLOWS value =[%u]  \n", *((unsigned short *)pData));
				g_shmVoIPstate->RTPPMHistoryData[chid].bufferUnderflows=g_shmVoIPstate->RTPPMHistoryData[chid].bufferUnderflows+*((unsigned short *)pData);
				DBG("g_shmVoIPstate->RTPPMHistoryData[chid].bufferUnderflows value =[%lu]  \n", g_shmVoIPstate->RTPPMHistoryData[chid].bufferUnderflows);
				break;				
			case OMCI_SHARE_RTPPM_BUFFEROVERFLOWS:
				DBG("OMCI_SHARE_RTPPM_BUFFEROVERFLOWS value =[%u]  \n", *((unsigned short *)pData));
				g_shmVoIPstate->RTPPMHistoryData[chid].bufferOverflows=g_shmVoIPstate->RTPPMHistoryData[chid].bufferOverflows+*((unsigned short *)pData);
				DBG("g_shmVoIPstate->RTPPMHistoryData[chid].bufferOverflows value =[%lu]  \n", g_shmVoIPstate->RTPPMHistoryData[chid].bufferOverflows);
				break;
			//9.9.14 SIP agent performance monitoring history data
			case OMCI_SHARE_SIPAGPM_TRANSACTIONS: 
				g_shmVoIPstate->SIPAGPMHistoryData[chid].transactions++;
				DBG("OMCI_SHARE_SIPAGPM_TRANSACTIONS value =[%ld]  \n",g_shmVoIPstate->SIPAGPMHistoryData[chid].transactions);
				//g_shmVoIPstate->SIPAGPMHistoryData[chid].transactions=*((unsigned long *)pData);
				break;			
			case OMCI_SHARE_SIPAGPM_RXINVITEREQS: 
				g_shmVoIPstate->SIPAGPMHistoryData[chid].rxInviteReqs++;
				DBG("OMCI_SHARE_SIPAGPM_RXINVITEREQS value =[%ld]  \n",g_shmVoIPstate->SIPAGPMHistoryData[chid].rxInviteReqs);
				//g_shmVoIPstate->SIPAGPMHistoryData[chid].rxInviteReqs=*((unsigned long *)pData);
				break;
			case OMCI_SHARE_SIPAGPM_RXINVITERETRANS: 
				g_shmVoIPstate->SIPAGPMHistoryData[chid].rxInviteRetrans++;
				DBG("OMCI_SHARE_SIPAGPM_RXINVITERETRANS value =[%ld]  \n", g_shmVoIPstate->SIPAGPMHistoryData[chid].rxInviteRetrans);
				//g_shmVoIPstate->SIPAGPMHistoryData[chid].rxInviteRetrans=*((unsigned long *)pData);
				break;
			case OMCI_SHARE_SIPAGPM_RXNONINVITEREQS: 
				g_shmVoIPstate->SIPAGPMHistoryData[chid].rxNoninviteReqs++;
				DBG("OMCI_SHARE_SIPAGPM_RXNONINVITEREQS value =[%ld]  \n", g_shmVoIPstate->SIPAGPMHistoryData[chid].rxNoninviteReqs);
				//g_shmVoIPstate->SIPAGPMHistoryData[chid].rxNoninviteReqs=*((unsigned long *)pData);
				break;
			case OMCI_SHARE_SIPAGPM_RXNONINVITERETRANS: 
				g_shmVoIPstate->SIPAGPMHistoryData[chid].rxNoninviteRetrans++;
				DBG("OMCI_SHARE_SIPAGPM_RXNONINVITERETRANS value =[%ld]  \n", g_shmVoIPstate->SIPAGPMHistoryData[chid].rxNoninviteRetrans);
				//g_shmVoIPstate->SIPAGPMHistoryData[chid].rxNoninviteRetrans=*((unsigned long *)pData);
				break;
			case OMCI_SHARE_SIPAGPM_RXRESPONSE: 
				g_shmVoIPstate->SIPAGPMHistoryData[chid].rxResponse++;
				DBG("OMCI_SHARE_SIPAGPM_RXRESPONSE value =[%ld]  \n", g_shmVoIPstate->SIPAGPMHistoryData[chid].rxResponse);
				//g_shmVoIPstate->SIPAGPMHistoryData[chid].rxResponse=*((unsigned long *)pData);
				break;
			case OMCI_SHARE_SIPAGPM_RXRESPONSERETRANSMISSIONS: 
				g_shmVoIPstate->SIPAGPMHistoryData[chid].rxResponseRetransmissions++;
				DBG("OMCI_SHARE_SIPAGPM_RXRESPONSERETRANSMISSIONS value =[%ld]  \n", g_shmVoIPstate->SIPAGPMHistoryData[chid].rxResponseRetransmissions);
				//g_shmVoIPstate->SIPAGPMHistoryData[chid].rxResponseRetransmissions=*((unsigned long *)pData);
				break;
			case OMCI_SHARE_SIPAGPM_TXINVITEREQS: 
				g_shmVoIPstate->SIPAGPMHistoryData[chid].txInviteReqs++;
				DBG("OMCI_SHARE_SIPAGPM_TXINVITEREQS value =[%ld]  \n",g_shmVoIPstate->SIPAGPMHistoryData[chid].txInviteReqs);
				//g_shmVoIPstate->SIPAGPMHistoryData[chid].txInviteReqs=*((unsigned long *)pData);
				break;
			case OMCI_SHARE_SIPAGPM_TXINVITERETRANS: 
				g_shmVoIPstate->SIPAGPMHistoryData[chid].txInviteRetrans++;
				DBG("OMCI_SHARE_SIPAGPM_TXINVITERETRANS value =[%ld]  \n", g_shmVoIPstate->SIPAGPMHistoryData[chid].txInviteRetrans);				
				//g_shmVoIPstate->SIPAGPMHistoryData[chid].txInviteRetrans=*((unsigned long *)pData);
				break;	
			case OMCI_SHARE_SIPAGPM_TXNONINVITEREQS: 
				g_shmVoIPstate->SIPAGPMHistoryData[chid].txNoninviteReqs++;
				DBG("OMCI_SHARE_SIPAGPM_TXNONINVITEREQS value =[%ld]  \n", g_shmVoIPstate->SIPAGPMHistoryData[chid].txNoninviteReqs);
				//g_shmVoIPstate->SIPAGPMHistoryData[chid].txNoninviteReqs=*((unsigned long *)pData);
				break;
			case OMCI_SHARE_SIPAGPM_TXNONINVITERETRANS: 
				g_shmVoIPstate->SIPAGPMHistoryData[chid].txNoninviteRetrans++;
				DBG("OMCI_SHARE_SIPAGPM_TXNONINVITERETRANS value =[%ld]  \n", g_shmVoIPstate->SIPAGPMHistoryData[chid].txNoninviteRetrans);
				//g_shmVoIPstate->SIPAGPMHistoryData[chid].txNoninviteRetrans=*((unsigned long *)pData);
				break;
			case OMCI_SHARE_SIPAGPM_TXRESPONSE: 
				g_shmVoIPstate->SIPAGPMHistoryData[chid].txResponse++;
				DBG("OMCI_SHARE_SIPAGPM_TXRESPONSE value =[%ld]  \n", g_shmVoIPstate->SIPAGPMHistoryData[chid].txResponse);
				//g_shmVoIPstate->SIPAGPMHistoryData[chid].txResponse=*((unsigned long *)pData);
				break;
			case OMCI_SHARE_SIPAGPM_TXRESPONSERETRANSMISSIONS: 
				g_shmVoIPstate->SIPAGPMHistoryData[chid].txResponseRetransmissions++;
				DBG("OMCI_SHARE_SIPAGPM_TXRESPONSERETRANSMISSIONS value =[%ld]  \n",g_shmVoIPstate->SIPAGPMHistoryData[chid].txResponseRetransmissions);
				//g_shmVoIPstate->SIPAGPMHistoryData[chid].txResponseRetransmissions=*((unsigned long *)pData);
				break;
				
			//9.9.15 SIP call initiation performance monitoring history data
			case OMCI_SHARE_SIPCIPM_FAILEDCONNECTCOUNTER: 
				g_shmVoIPstate->SIPCIPMHistoryData[chid].failedConnectCounter++;
				DBG("OMCI_SHARE_SIPCIPM_FAILEDCONNECTCOUNTER value =[%ld]  \n", g_shmVoIPstate->SIPCIPMHistoryData[chid].failedConnectCounter);
				break;
			case OMCI_SHARE_SIPCIPM_FAILEDVALIDATECOUNTER: 
				g_shmVoIPstate->SIPCIPMHistoryData[chid].failedValidateCounter++;
				DBG("OMCI_SHARE_SIPCIPM_FAILEDVALIDATECOUNTER value =[%ld]  \n", g_shmVoIPstate->SIPCIPMHistoryData[chid].failedValidateCounter);
				break;
			case OMCI_SHARE_SIPCIPM_TIMEOUTCOUNTER: 
				g_shmVoIPstate->SIPCIPMHistoryData[chid].timeoutCounter++;
				DBG("OMCI_SHARE_SIPCIPM_TIMEOUTCOUNTER value =[%ld]  \n", g_shmVoIPstate->SIPCIPMHistoryData[chid].timeoutCounter);
				break;
			case OMCI_SHARE_SIPCIPM_FAILURERECEIVEDCOUNTER: 
				g_shmVoIPstate->SIPCIPMHistoryData[chid].failureReceivedCounter++;
				DBG("OMCI_SHARE_SIPCIPM_FAILURERECEIVEDCOUNTER value =[%ld]  \n", g_shmVoIPstate->SIPCIPMHistoryData[chid].failureReceivedCounter);
				break;
			case OMCI_SHARE_SIPCIPM_FAILEDAUTHENTICATECOUNTER: 
				g_shmVoIPstate->SIPCIPMHistoryData[chid].failedAuthenticateCounter++;
				DBG("OMCI_SHARE_SIPCIPM_FAILEDAUTHENTICATECOUNTER value =[%ld]  \n", g_shmVoIPstate->SIPCIPMHistoryData[chid].failedAuthenticateCounter);
				break;
			case OAM_VOIP_PORTSTATUS:
				g_shmVoIPstate->VOIPLineStatus[chid].voip_Port_status=*((unsigned short *)pData);
				DBG("OAM_VOIP_PORTSTATUS value =[%d]  \n", *((unsigned short *)pData));
				break;
			default:
				break;			
		}
	}
	else
		fprintf(stderr, "set %s g_shmVoIPstate = NULL\n",__FUNCTION__);
	return;
}



int voip_line_state_share_set(voip_state_share_t *pVoIPShare)
{
	memcpy(g_shmVoIPstate, pVoIPShare, sizeof(voip_state_share_t));
	return 0;	
}

void voip_line_state_share_stop(void)
{
	
	if (g_shmVoIPstate != NULL)
	{
		if (shmdt(g_shmVoIPstate) == -1)
			fprintf(stderr, "shmdt failed: %s\n", strerror(errno));
	}

	if (shm_id != -1)
	{
		if (shmctl(shm_id, IPC_RMID, 0) == -1)
			fprintf(stderr, "shmctl failed: %s\n", strerror(errno));
	}


}

int voip_line_state_init_variables()
{

	key_t key = ftok(VOIP_PATHNAME, VOIP_LINE_STATE_VER);
	if (key == -1)
	{
		fprintf(stderr, "ftok failed: %s\n", strerror(errno));
		return -1;
	}
	
	fprintf(stderr, "voip omci line state share size = %d\n", sizeof(voip_state_share_t));	
	
	// get share memory ID (line_state)
	shm_id = shmget(key, sizeof(voip_flash_share_t), 0666 | IPC_CREAT);

	if (shm_id == -1)
	{
		fprintf(stderr, " get share memory ID shmget failed: %s\n", strerror(errno));
		goto voip_line_state_init_failed;
	}    

    // attach share memory (line_state)
	if (g_shmVoIPstate == NULL)
	{
	if ((g_shmVoIPstate = shmat(shm_id, NULL, 0)) == (void *)-1)
	{
		fprintf(stderr, "attach share memory shmat failed: %s\n", strerror(errno));
		goto voip_line_state_init_failed;
	}
	}


	return 0;
voip_line_state_init_failed:
	fprintf(stderr, "voip_line_state_init_failed \n");
	voip_line_state_share_stop();
	return -1;	
}


