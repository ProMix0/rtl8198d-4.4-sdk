#ifndef CSV_API_H
#define CSV_API_H

#define CSV_CONF_FILE       "/etc/config.csv"
#define CSV_KEY_CONTACT               0x0001
#define CSV_KEY_CALL_LOG              0x0002
#define CSV_KEY_WAKEUP_CALL           0x0003
#define CSV_KEY_RINGING_SCHEDULE      0x0004

#define MAX_FILENAME_SZ     127

// in call log  

//#define CSV_CALL_NUMBER_ONLY_CALLER 1

#if CSV_CALL_NUMBER_ONLY_CALLER

#define CSV_COL_SEQ         0
#define CSV_COL_IS_ACTIVE   1
#define CSV_COL_NUMBER      2
#define CSV_COL_CALL_TYPE   3
#define CSV_COL_START_TIME  4
#define CSV_COL_END_TIME    5

#else /* incoming and outgoing */

#define CSV_COL_SEQ                  0
#define CSV_COL_IS_ACTIVE            1
#define CSV_COL_NUMBER_CALLER        2
#define CSV_COL_NUMBER_CALLEE        3
#define CSV_COL_CALL_TYPE            4
#define CSV_COL_START_TIME           5
#define CSV_COL_END_TIME             6

#endif

// in contact
#define CSV_COL_NAME        0
#define CSV_CONTACT_TYPE    1
#define CSV_CONTACT_NUM     2

// in wakeup call
#define CSV_COL_WUC_NAME    	     0
#define CSV_COL_WUC_RINGTIME         1
#define CSV_COL_WUC_DAYS             2
#define CSV_COL_WUC_ASSIGN_FXS       3
#define CSV_COL_WUC_ONOFF            4

// in ringing schedule
#define CSV_COL_RS_FROM    0
#define CSV_COL_RS_TO      1

void csv_api_init(void);

#endif //CSV_API_H

