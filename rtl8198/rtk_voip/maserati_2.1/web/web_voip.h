#ifndef __WEB_VOIP_H
#define __WEB_VOIP_H

#include <string.h>
#include "voip_flash.h"
//#include "voip_types.h"
#include "voip_flash_mib.h"
#include "voip_flash_tool.h"
#include <voip_manager.h>
#if defined(CONFIG_E8B) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
#include <user/boa/src/LINUX/form_src_e8/multilang.h>
#elif defined(CONFIG_E8B) && defined(CONFIG_RTK_L34_ENABLE)
#include <user/boa_e8/src/LINUX/form_src/multilang.h>
#else
#include <user/boa/src/LINUX/form_src_generic/multilang.h>
#endif
#include <syslog.h>

#ifdef CONFIG_APP_BOA
	#include "boa.h"
	#include "asp_page.h"
#else
	#include "webs.h"
#endif

#ifndef CONFIG_RTK_VOIP_PACKAGE_867X
#include "apform.h"
#endif

#if 1//#ifdef __mips__
#if CONFIG_RTK_VOIP_PACKAGE_865X
#define VOIP_CONFIG_PATH	"/www/config_voip.dat"
#elif CONFIG_RTK_VOIP_PACKAGE_867X
#define VOIP_CONFIG_PATH	"/var/config_voip.dat"
#else
#define VOIP_CONFIG_PATH	"/web/config_voip.dat"
#endif
#else
#define VOIP_CONFIG_PATH	"../web/config_voip.dat"
#endif

#ifdef CONFIG_APP_BOA
#define gstrcmp strcmp
#define boaRedirect	send_redirect_perm
#define boaWrite		req_format_write
#define boaGetVar		req_get_cstream_var
#endif

//for syslog
#define	LOG_EMERG	0 /* system is unusable               */
#define	LOG_ALERT	1 /* action must be taken immediately */
#define	LOG_CRIT	2 /* critical conditions              */
#define	LOG_ERR		3 /* error conditions                 */
#define	LOG_WARNING	4 /* warning conditions               */
#define	LOG_NOTICE	5 /* normal but significant condition */
#define	LOG_INFO	6 /* informational                    */
#define	LOG_DEBUG	7 /* debug-level messages             */


/* To show the register status on Web page. */

#define _PATH_TMP_STATUS "/tmp/voipstatus"
#define _PATH_TMP_E8C_STATUS "/tmp/E8C_voipstatus"

// SIP Config
#define _PATH_SIP_CONFIG	"/etc/solar.conf"

#if defined(CONFIG_RTK_VOIP_DRIVERS_FXO) && !defined(CONFIG_RTK_VOIP_DRIVERS_VIRTUAL_DAA)
//#define FXO_REDIAL
#endif

// init web functions
int web_voip_init();


#ifdef CONFIG_APP_BOA
int asp_voip_getInfo(request * wp, int argc, char **argv);
int asp_voip_GeneralGet(request * wp, int argc, char **argv);
int asp_voip_DialPlanGet( request * wp, int argc, char **argv);
int asp_voip_ToneGet(request * wp, int argc, char **argv);
int asp_voip_RingGet(request * wp, int argc, char **argv);
int asp_voip_OtherGet(request * wp, int argc, char **argv);
int asp_voip_ConfigGet(request * wp, int argc, char **argv);
int asp_voip_FwupdateGet(request * wp, int argc, char **argv);
int asp_voip_FxoGet(request * wp, int argc, char **argv);
int asp_voip_NetGet(request * wp, int argc, char **argv);
int asp_voip_LogGet(request * wp, int argc, char **argv);

#else
int asp_voip_getInfo(int eid, request * wp, int argc, char **argv);
int asp_voip_GeneralGet(int ejid, request * wp, int argc, char **argv);
int asp_voip_DialPlanGet(int ejid, request * wp, int argc, char **argv);
int asp_voip_ToneGet(int ejid, request * wp, int argc, char **argv);
int asp_voip_RingGet(int ejid, request * wp, int argc, char **argv);
int asp_voip_OtherGet(int ejid, request * wp, int argc, char **argv);
int asp_voip_ConfigGet(int ejid, request * wp, int argc, char **argv);
int asp_voip_FwupdateGet(int ejid, request * wp, int argc, char **argv);
int asp_voip_FxoGet(int ejid, request * wp, int argc, char **argv);
int asp_voip_NetGet(int ejid, request * wp, int argc, char **argv);
int asp_voip_LogGet(int ejid, request * wp, int argc, char **argv);

#endif

void asp_voip_GeneralSet(request * wp, char *path, char *query);
void asp_voip_DialPlanSet(request * wp, char *path, char *query);
void asp_voip_ToneSet(request * wp, char *path, char *query);
void asp_voip_RingSet(request * wp, char *path, char *query);
void asp_voip_OtherSet(request * wp, char *path, char *query);
void asp_voip_ConfigSet(request * wp, char *path, char *query);
void asp_voip_FwSet(request * wp, char *path, char *query);
void asp_voip_IvrReqSet(request * wp, char *path, char *query);
void asp_voip_FxoSet(request * wp, char *path, char *query);
void asp_voip_NetSet(request * wp, char *path, char *query);
void asp_voip_LogSet(request * wp, char *path, char *query);

//[SD6, bohungwu, e8c web
int asp_voip_e8c_get(int ejid, request * wp, int argc, char **argv);
void asp_voip_e8c_set(request * wp, char *path, char *query);
int asp_diag_voip_e8c_get(int ejid, request * wp, int argc, char **argv);
void asp_diag_voip_e8c_set(request * wp, char *path, char *query);
void asp_diag_call_set(request * wp, char *path, char *query);
void asp_diag_e8c_auto_call_set(request * wp, char *path, char *query);
void asp_voip2_e8c_set(request * wp, char *path, char *query);
void asp_voip_cmcc_set(request * wp, char *path, char *query);
void asp_voip2_cmcc_set(request * wp, char *path, char *query);
void asp_voip_cmcc_codec_set(request * wp, char *path, char *query);
void asp_voip_cmcc_account_set(request * wp, char *path, char *query);
void asp_voip_cmcc_account_del(request * wp, char *path, char *query);
void asp_debug_e8c_set(request * wp, char *path, char *query);
int asp_voip_flags_get(int ejid, request *wp, int argc, char **argv);
void asp_voip_flags_set(request *wp, char *path, char *query);

#ifdef CONFIG_RTK_VOIP_SIP_TLS
void asp_voip_TLSCertUpload(request * wp, char *path, char *query);
int asp_voip_TLSGetCertInfo(int eid, request * wp, int argc, char **argv);
#endif
#endif

// flash api in WEB
int web_flash_get(voipCfgParam_t **cfg);
int web_flash_set(voipCfgParam_t *cfg);

// misc function
int web_restart_solar();
int web_voip_saveConfig();

