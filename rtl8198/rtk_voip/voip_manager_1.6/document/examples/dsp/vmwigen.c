#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "voip_manager.h"
#include "fsk.h"


/**
FSK_PARAM_NULL          = 0,
FSK_PARAM_DATEnTIME     = 0x01,         // Date and Time
FSK_PARAM_CLI           = 0x02,         // Calling Line Identify (CLI)
FSK_PARAM_CLI_ABS       = 0x04,         // Reason for absence of CLI
FSK_PARAM_CLI_NAME      = 0x07,         // Calling Line Identify (CLI) Name
FSK_PARAM_CLI_NAME_ABS  = 0x08,         // Reason for absence of (CLI) Name
FSK_PARAM_MW            = 0x0b,         // Message Waiting
*/

/**
// Message Type
#define FSK_MSG_CALLSETUP		0x80		// Call Set-up
#define FSK_MSG_MWSETUP			0x82		// Message Waiting (VMWI) MDMF
#define FSK_MSG_ADVICECHARGE	0x86		// Advice of Charge
#define FSK_MSG_SMS				0x89		// Short Message Service 
#define FSK_MSG_SCALLNUMBER		0x04		// Single Calling Number 
#define FSK_MSG_SMWSETUP		0x06		// Single Message Waiting (VMWI) SDMF  
*/ 
 
 
int main(int argc, char *argv[])
{
    TstFskClid clid;
    int chid = 0, msg_type = 0, fsk_area = 0; 
    char vmwi_on = 0;
    char time_cid[64], date_time[64];
   

    if (argc < 5)
    {
        printf("Usage: vmwigen <chid> <fsk_area> <fsk_msg_type(0)> <time_cid>\n");
        printf("       vmwigen <chid> <fsk_area> <fsk_msg_type(1)> <vmwi>\n");
        printf("       vmwigen <chid> <fsk_area> <fsk_msg_type(2)> <vmwi> [date_time]\n");
        printf("Note: - fsk_area: 0(Bellcore), 1(ETSI), 2(BT), 3(NTT)\n");       
        printf("      - fsk_msg_type: 0(SDMF caller id), 1(SDMF Message waiting), 2(MDMF Message waiting)\n");
        printf("      - time_cid: time and caller id (mmddhhmm+caller id, ex:12051530035776598)\n");
        printf("      - vmwi: 0(off), 1(on)\n");
        printf("      - date_time (mmddhhmm) is optional.\n");
        printf("      - Example of VMWI chid 0, area ETSI, fsk_msg_type MDMF, vmwi on, date_time 01020304 (Bellcore)\n");
        printf("      - => vmwigen 0 1 2 1 01020304\n"); 
    }
    else
    {
        memset(time_cid, 0, sizeof(time_cid));
        memset(date_time, 0, sizeof(date_time));

        chid = atoi(argv[1]);
       
        fsk_area = atoi(argv[2]);
        if (fsk_area < 0 || fsk_area > 3) {
            printf("%s: fsk_area %d is wrong!\n", __FILE__, fsk_area);
            return -1;
        }

        msg_type = atoi(argv[3]);
        if (msg_type < 0 || msg_type > 2) {
            printf("%s: fsk_msg_type %d is wrong!\n", __FILE__, msg_type);
            return -1;
        }
        
        if (msg_type == 0) { /* SDMF caller id */
            strcpy(time_cid, argv[4]);
            printf("%s: time_cid:%s\n", __FILE__, time_cid);
        }
        else {
            vmwi_on = atoi(argv[4]);
            if (vmwi_on < 0 || vmwi_on > 1) {
                printf("%s: vmwi %d is wrong!\n", __FILE__, vmwi_on);
                return -1;
            }
        }

        if (argv[5]) {
            strcpy(date_time, argv[5]);
            printf("%s: date_time:%s\n", __FILE__, date_time);
        }

        /* fsk area -> 0:Bellcore 1:ETSI 2:BT 3:NTT */
        rtk_SetFskArea(chid, atoi(argv[2])|0x100/*area*/);

        clid.ch_id = chid;
        clid.service_type = 0; //service type 1

        if (msg_type == 0)         /* 0: SDMF caller id */                  
        {           
            clid.msg_type = FSK_MSG_SCALLNUMBER;                //Time and Caller ID
            strcpy(clid.cid_data[0].data, time_cid);         
        }
        else if (msg_type == 1)    /* 1: SDMF Message waiting */                       
        {           
            clid.msg_type = FSK_MSG_SMWSETUP;                                                  
            strcpy(clid.cid_data[0].data, &vmwi_on);            
        }        
        else if (msg_type == 2)    /* 2: MDMF Message waiting */                 
        {          
            clid.msg_type = FSK_MSG_MWSETUP;               
            clid.cid_data[0].type = FSK_PARAM_MW;               //MDMF Message Waiting before DATE and Time
            strcpy(clid.cid_data[0].data, &vmwi_on);

            if (argv[5])
            {
                clid.cid_data[1].type = FSK_PARAM_DATEnTIME;    //DATE and Time
                strcpy(clid.cid_data[1].data, date_time);
            }
            else
            {
                clid.cid_data[1].type = FSK_PARAM_NULL;
                printf("%s: date and time is NULL\n", __FILE__);
            }                                
        }
        else 
        {
            printf("%s: msg_type %d is msg_type!\n", __FILE__, msg_type);
            return -1;        
        }

        /* Only 2 elements for VMWI data. Set other element to 0 (MUST) */
        clid.cid_data[2].type = 0;
        clid.cid_data[3].type = 0;
        clid.cid_data[4].type = 0;

        /* Generate FSK VMWI */
        rtk_GenFskVmwi(chid, &clid, 0);	
    }	

    return 0;
}

