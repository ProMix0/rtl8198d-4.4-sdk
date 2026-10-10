#include <stdio.h>
#include "web_voip.h"

#include "csv_parser.h"
#include "csv_api.h"

#define _PATH_CALL_LOGS     "/tmp/call_log.csv"


void asp_voip_LogSet(request * wp, char *path, char *query)
{
	char *strValue, *submitUrl;

	submitUrl = boaGetVar(wp, "call_log_refesh", "");   // hidden page

	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
}


#ifdef CONFIG_APP_BOA
int asp_voip_LogGet(request * wp, int argc, char **argv)
#else
int asp_voip_LogGet(int ejid, request * wp, int argc, char **argv)
#endif
{
#if 0
    FILE *fp;
	char  buf[150];
	int nBytesSent=0;

	boaWrite(wp,"Port 1<br><table border=\"1\" width=500 >");
	boaWrite(wp, "<tr><td bgColor=#aaddff>DateTime</td><td bgColor=#aaddff>From</td><td bgColor=#aaddff>To</td><td bgColor=#aaddff>Type</td><td bgColor=#aaddff>Status</td><td bgColor=#aaddff>Duration</td></tr>\n");

     fp = fopen(_PATH_CALL_LOGS_1, "r");
       if (fp){              
	        while(fgets(buf,150,fp)){
			nBytesSent += boaWrite(wp, "%s", buf);
	       	}
			fclose(fp);
        }
	boaWrite(wp,"</table>");

#if CONFIG_RTK_VOIP_CON_CH_NUM > 1
	boaWrite(wp, "<br>Port 2<br><table border=\"1\" width=500>");
	boaWrite(wp, "<tr><td bgColor=#aaddff>DateTime</td><td bgColor=#aaddff>From</td><td bgColor=#aaddff>To</td><td bgColor=#aaddff>Type</td><td bgColor=#aaddff>Status</td><td bgColor=#aaddff>Duration</td></tr>\n");

	fp = fopen(_PATH_CALL_LOGS_2, "r");
	if (fp){			  
		while(fgets(buf, 150, fp)){
			nBytesSent += boaWrite(wp, "%s", buf);
		}
		fclose(fp);
	}
	boaWrite(wp,"</table>");

#endif
#else
	FILE *fp;
	char buf[150];
	int no=0, nBytesSent=0, _min=0, _sec=0;
	char status[CSV_STRING_16]={0}, caller[CSV_STRING_32]={0}, callee[CSV_STRING_32]={0};   
    char direction[CSV_STRING_16]={0}, duration[CSV_STRING_16]={0}, start_time[CSV_STRING_32]={0};

    boaWrite(wp, "<table border=0 width=600>");
	boaWrite(wp, "<tr><td bgColor=#aaddff>No.</td><td bgColor=#aaddff>Status</td><td bgColor=#aaddff>From</td><td bgColor=#aaddff>To</td><td bgColor=#aaddff>Type</td><td bgColor=#aaddff>Duration</td><td bgColor=#aaddff>DateTime</td></tr>\n");
	
    fp = fopen(_PATH_CALL_LOGS, "r");
	if (fp){              
		while(fgets(buf, 150, fp)){
			csv_api_call_log_get_by_line_num(no, status, caller, callee, direction, start_time, duration);
            if (strcmp("active", status) == 0) {
			    nBytesSent += boaWrite(wp, "<tr><td bgColor=#ddeeff>%d</td><td bgColor=#ddeeff><font color=\"#FF0000\">%s</font></td><td bgColor=#ddeeff>%s</td><td bgColor=#ddeeff>%s</td><td bgColor=#ddeeff>%s</td><td bgColor=#ddeeff>%s</td><td bgColor=#ddeeff>%s</td></tr>\n",
                                       ++no, status, caller, callee, direction, duration, start_time);
            }
			else{
				nBytesSent += boaWrite(wp, "<tr><td bgColor=#ddeeff>%d</td><td bgColor=#ddeeff>%s</td><td bgColor=#ddeeff>%s</td><td bgColor=#ddeeff>%s</td><td bgColor=#ddeeff>%s</td><td bgColor=#ddeeff>%s</td><td bgColor=#ddeeff>%s</td></tr>\n",
                                       ++no, status, caller, callee, direction, duration, start_time);
			}    
		}
        fclose(fp);
    }
	boaWrite(wp,"</table>");
#endif

	return 0;
}

