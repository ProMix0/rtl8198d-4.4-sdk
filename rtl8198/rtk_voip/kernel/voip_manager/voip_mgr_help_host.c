#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,12,0)
#include <linux/uaccess.h>
#else
#include <asm/uaccess.h>
#endif
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include "rtk_voip.h"
#include "voip_types.h"
#include "voip_control.h"
#include "voip_init.h"
#include "voip_limit.h"
#include "voip_errno.h"
#include "voip_mgr_define.h"
#include "ipc_arch_tx.h"
#include "con_mux.h"

#include "voip_mgr_transfer_id.h"
#include "voip_mgr_help_host.h"

//int slic_ch_num = SLIC_CH_NUM;
//int voip_ch_num = VOIP_CH_NUM;
//int sess_num = SESS_NUM;
//int rtcp_sid_offset = RTCP_SID_OFFSET;
//int pcm_period = PCM_PERIOD;
//uint64 gVoipFeature;

/* PCM Channel Info */
//char chanEnabled[MAX_VOIP_CH_NUM] = {0};

/************************************************************************/

#if 0
extern uint32 API_OpenSid_Dsp(uint32 chid, uint32 mid, uint32 dsp_id);
extern uint32 API_GetSid_Dsp(uint32 chid, uint32 mid, uint32 dsp_id);
extern uint32 API_GetMid_Dsp(uint32 chid, uint32 sid, uint32 dsp_id);
extern uint32 API_CloseSid_Dsp(uint32 chid, uint32 mid, uint32 dsp_id);
#endif

/* T.38 Info */
//t38_running_state_t t38RunningState[MAX_VOIP_CH_NUM]; // need T.38 info for Host in rx trap.

/* RTP Statistic Info */
//unsigned char RtcpOpen[MAX_SESS_NUM] = {0};
//unsigned char RtpOpen[MAX_SESS_NUM] = {0};
//uint32 rtpHold[MAX_SESS_NUM];
//uint32 rtpConfigOK[MAX_SESS_NUM];
//uint32 nRxRtpStatsLostPacket[MAX_VOIP_CH_NUM];

int32 Host_pktRx( uint32 chid, uint32 sid, void* packet, uint32 pktLen, uint32 flags )
{

	// Send RTP packet to DSP by voice packet.
	TstTxPktCtrl stTxPktCtrl = { .seq_no = -1, .resend_flag = 0 };
	//unsigned char rtp_read_tmp[1500];
	unsigned int chid_dsp/*, sid_dsp*/, mid;
	int isRtcp = 0;
	//voice_rtp2dsp_content_t * const voice_rtp2dsp_content = 
	//		( voice_rtp2dsp_content_t * )rtp_read_tmp;
	voice_rtp2dsp_content_t voice_rtp2dsp_content_header;
	
	if( sid >= RTCP_SID_OFFSET ) {
		sid -= RTCP_SID_OFFSET;
		isRtcp = 1;
	} 
	
	stTxPktCtrl.dsp_cpuid = API_get_DSP_ID(0, chid);
	
	/* Transfer Host chid, sid to DSP ID, chid */	
	chid_dsp = API_get_DSP_CH(0, chid);
	mid = API_GetMid(chid, sid);
	//sid_dsp = API_GetSid_Dsp(chid_dsp, mid, stTxPktCtrl.dsp_id);

#if 1
	voice_rtp2dsp_content_header.chid = chid_dsp;
	voice_rtp2dsp_content_header.mid = mid + ( isRtcp ? RTCP_MID_OFFSET : 0 );
	voice_rtp2dsp_content_header.flags = flags;
#elif 1
	voice_rtp2dsp_content ->chid = chid_dsp;
	voice_rtp2dsp_content ->mid = mid + ( isRtcp ? RTCP_MID_OFFSET : 0 );
	voice_rtp2dsp_content ->flags = flags;
#else	
	*(uint32*)rtp_read_tmp= chid_dsp;
	*(uint32*)((unsigned char*)rtp_read_tmp+4)= sid_dsp;
	*(uint32*)((unsigned char*)rtp_read_tmp+8)= flags;
#endif
	
	//if (pktLen > (1500-12))
	//{
	//	PRINT_R("Error! no enought buf size, %s-%s-%d\n", __FILE__, __FUNCTION__, __LINE__);
	//	PRINT_R("Need %d bytes, sid=%d\n", pktLen, sid);
	//}
	//memcpy(&rtp_read_tmp[12], packet, pktLen);
	//memcpy(voice_rtp2dsp_content ->voice, packet, pktLen);
	
	ipc_pkt_tx_final( 0, IPC_PROT_VOICE_TO_DSP, 
						(unsigned char*)packet /*voice_rtp2dsp_content*/, 
						pktLen /*+ SIZE_OF_VOICE_RTP2DSP_CONT_HEADER*/, 
						&stTxPktCtrl, NULL,
						( uint8 * )&voice_rtp2dsp_content_header);
	
	return SUCCESS;
}

void ipc_Host2DSP_T38Tx( uint32 chid_pkt, uint32 sid_pkt, void* packet, uint32 pktLen )
{
	/* Send T.38 packet to DSP */
	TstTxPktCtrl stTxPktCtrl;
	//unsigned char t38_read_tmp[1500];
	unsigned int chid, mid, /*sid,*/ len;
	//voice_content_t * const voice_content =
	//			( voice_content_t * )t38_read_tmp;
	voice_content_t voice_content_header;

	stTxPktCtrl.dsp_cpuid = API_get_DSP_ID(0, chid_pkt);
	stTxPktCtrl.seq_no = -1;
	stTxPktCtrl.resend_flag = 0;
	
	/* Transfer Host chid, sid to DSP ID, chid */
	chid = API_get_DSP_CH(0, chid_pkt);
	mid = API_GetMid(chid_pkt, sid_pkt);
	//sid = API_GetSid(chid, mid);

#if 1
	voice_content_header.chid = chid;
	voice_content_header.mid = mid;
#elif 1
	voice_content ->chid = chid;
	voice_content ->mid = mid;
#else	
	*(uint32*)t38_read_tmp = chid; // chid
	*(uint32*)((unsigned char*)t38_read_tmp+4) = sid; // sid
#endif
	
	len = pktLen;
	
	//if ( len > (1500-8))
	//{
	//	PRINT_R("Error! no enought buf size, %s-%s-%d\n", __FILE__, __FUNCTION__, __LINE__);
	//	PRINT_R("Need %d bytes, sid=%d\n", len, sid_pkt);
	//}
	//memcpy(&t38_read_tmp[8], skb->data + sizeof(struct udphdr), len);
	//memcpy(voice_content ->voice, packet, len);
	
	ipc_pkt_tx_final( 0, IPC_PROT_T38_TO_DSP, 
						(unsigned char*)packet/*voice_content*/, 
						len /*+ SIZE_OF_VOICE_CONT_HEADER*/, 
						&stTxPktCtrl, NULL,
						( uint8 * )&voice_content_header);
	//printk("T%d ", ptr->c_id);
}

/************************************************************************/
static int read_mgr_chid( uint32 *mgr_chid, void *user, unsigned int len, const voip_mgr_entry_t *p_entry )
{
	unsigned char *field;
	
	if( p_entry ->field_offset + p_entry ->field_size > len ) {
		PRINT_R( "Bad field offset + size (%d > %d) in reading!\n",
				 p_entry ->field_offset + p_entry ->field_size, len );
		return -EVOIP_IOCTL_IPC_HOST_CHID_CHECK_ERR;
	}
	
	field = ( unsigned char * )user;
	field += p_entry ->field_offset;
	
	switch( p_entry ->field_size ) {
	case 1:
		*mgr_chid = *( ( uint8 * )field );
		break;
	case 2:
		*mgr_chid = *( ( uint16 * )field );
		break;
	case 4:
		*mgr_chid = *( ( uint32 * )field );
		break;
	default:
		PRINT_R( "Bad field_size=%u in reading!\n", p_entry ->field_size );
		return -EVOIP_IOCTL_IPC_HOST_CHID_CHECK_ERR;
	}
	
	return 0;
}

static int write_mgr_chid( uint32 mgr_chid, void *user, unsigned int len, const voip_mgr_entry_t *p_entry )
{
	unsigned char *field;
	
	if( p_entry ->field_offset + p_entry ->field_size > len ) {
		PRINT_R( "Bad field offset + size (%d > %d) in writing!\n",
				 p_entry ->field_offset + p_entry ->field_size, len );
		return -EVOIP_IOCTL_IPC_HOST_SIZE_CHECK_ERR;
	}
	
	field = ( unsigned char * )user;
	field += p_entry ->field_offset;
	
	switch( p_entry ->field_size ) {
	case 1:
		*( ( uint8 * )field ) = mgr_chid;
		break;
	case 2:
		*( ( uint16 * )field ) = mgr_chid;
		break;
	case 4:
		*( ( uint32 * )field ) = mgr_chid;
		break;
	default:
		PRINT_R( "Bad field_size=%u in writing!\n", p_entry ->field_size );
		return -EVOIP_IOCTL_IPC_HOST_SIZE_CHECK_ERR;
	}
	
	return 0;
}

#define HSND_PRINTK		PRINT_Y

#define USER_BUFFER_SIZE		1500
//static unsigned char user_buffer[ USER_BUFFER_SIZE ]; //use kmalloc 

#ifdef VOIP_USER_BUFFER_POOL_SUPPORT
#define PRIVATE_USER_BUFFER_POOL_TOTAL  (8)
#define PRIVATE_USER_BUFFER_SIZE        (USER_BUFFER_SIZE)
#define BUFFER_POOL_TOTAL               (PRIVATE_USER_BUFFER_POOL_TOTAL+1)

#define VOIP_USER_BUFFER_POOL_USE_MUTEX               1
//#define VOIP_USER_BUFFER_POOL_USE_SPINLOCK            1
//#define VOIP_USER_BUFFER_POOL_USE_DISABLE_INTERRUPT   1

typedef struct _private_user_buffer_pool{
	unsigned char	m[PRIVATE_USER_BUFFER_POOL_TOTAL][PRIVATE_USER_BUFFER_SIZE];
} private_user_buffer_pool_t;

typedef struct _buffer_pool{
	void * 				bc[BUFFER_POOL_TOTAL];
	unsigned int		ins;
	unsigned int		del;
	unsigned int		cnt_ins;
	unsigned int		cnt_del;
} buffer_pool_t;

static private_user_buffer_pool_t   private_user_buffer_pool;
static buffer_pool_t                buffer_pool;

#if defined(VOIP_USER_BUFFER_POOL_USE_MUTEX)
	static struct semaphore pool_sem;
	#define VOIP_USER_BUFFER_POOL_ENTER_CRITICAL_SECTION()   down(&pool_sem)
	#define VOIP_USER_BUFFER_POOL_LEAVE_CRITICAL_SECTION()   up(&pool_sem)
#elif defined(VOIP_USER_BUFFER_POOL_USE_DISABLE_INTERRUPT)
	#define VOIP_USER_BUFFER_POOL_ENTER_CRITICAL_SECTION(x)   local_irq_save(x)
	#define VOIP_USER_BUFFER_POOL_LEAVE_CRITICAL_SECTION(x)   local_irq_restore(x)
#elif defined(VOIP_USER_BUFFER_POOL_USE_SPINLOCK)
	#ifdef CONFIG_SMP  //debug version
		#include <linux/spinlock.h>
		#include <linux/smp.h>
		spinlock_t lock_voip_user_buffer_pool;
		int lock_voip_user_buffer_pool_owner = -1;
		#if 1
			#define VOIP_USER_BUFFER_POOL_ENTER_CRITICAL_SECTION(x)	\
				do { \
					 if(lock_voip_user_buffer_pool_owner!=smp_processor_id()) \
						 spin_lock_irqsave(&lock_voip_user_buffer_pool, (x)); \
					 else \
						printk("\x1B[31m" "[%s %s %d] recursion detection\n" "\x1B[0m", __FILE__, __FUNCTION__,__LINE__); \
					 lock_voip_user_buffer_pool_owner=smp_processor_id();\
				 } while(0)
			#define VOIP_USER_BUFFER_POOL_LEAVE_CRITICAL_SECTION(x)	\
				do { \
					 lock_voip_user_buffer_pool_owner=-1; \
					 spin_unlock_irqrestore(&lock_voip_user_buffer_pool, (x)); \
				 } while(0)
		#else
			#define VOIP_USER_BUFFER_POOL_ENTER_CRITICAL_SECTION(x)   spin_lock_irqsave(&lock_voip_user_buffer_pool, x)
			#define VOIP_USER_BUFFER_POOL_LEAVE_CRITICAL_SECTION(x)   spin_unlock_irqrestore(&lock_voip_user_buffer_pool, x)
		#endif
	#endif
#endif

static int
voip_buffer_pool_full( void )
{
	if(((buffer_pool.del + BUFFER_POOL_TOTAL - buffer_pool.ins)%BUFFER_POOL_TOTAL) == 1){
		PRINT_R( "%s(%d) full\n" , __FUNCTION__ , __LINE__ );
		return 1;
		}
	else{
		return 0;
		}
}

static int
voip_buffer_pool_empty( void )
{	
	if(((buffer_pool.ins + BUFFER_POOL_TOTAL - buffer_pool.del)%BUFFER_POOL_TOTAL) == 0){
		return 1;
		}
	else{
		return 0;
		}
}

static int
voip_buffer_pool_enqueue_init( void *dp )
{
	int ret=-1;

	if(voip_buffer_pool_full()){
		ret=-1;
		}
	else{
		buffer_pool.bc[ buffer_pool.ins ] = dp;
		buffer_pool.ins = (buffer_pool.ins+1) % BUFFER_POOL_TOTAL;
		ret=0;
		}
	return ret;
}


static void
voip_buffer_pool_init( void )
{
	int i;
	int ret=-1;
	buffer_pool.ins = 0;
	buffer_pool.del = 0;

	for( i=0 ; i<PRIVATE_USER_BUFFER_POOL_TOTAL ; i++ ){
		ret = voip_buffer_pool_enqueue_init( &private_user_buffer_pool.m[i] );
		if(ret==-1){
			PRINT_R( "%s(%d)BC init failed\n" , __FUNCTION__ , __LINE__ );
			}
		}
}

int __init
voip_user_buffer_pool_init( void )
{
#if defined(VOIP_USER_BUFFER_POOL_USE_SPINLOCK)
	spin_lock_init(&lock_voip_user_buffer_pool);
#endif

#if defined(VOIP_USER_BUFFER_POOL_USE_MUTEX)	
	sema_init(&pool_sem , 1);
#endif

	memset(&private_user_buffer_pool , 0 , sizeof(private_user_buffer_pool));
	memset(&buffer_pool , 0 , sizeof(buffer_pool));
	voip_buffer_pool_init();

	return 0;
}


int
voip_buffer_pool_enqueue( void *dp )
{
	int ret=-1;

#if defined(VOIP_USER_BUFFER_POOL_USE_MUTEX)
	VOIP_USER_BUFFER_POOL_ENTER_CRITICAL_SECTION();
#else
	unsigned long flags;
	VOIP_USER_BUFFER_POOL_ENTER_CRITICAL_SECTION(flags);
#endif
	
	if(voip_buffer_pool_full()){
		ret=-1;
		}
	else{
		memset(dp , 0 , PRIVATE_USER_BUFFER_SIZE);
		buffer_pool.bc[ buffer_pool.ins ] = dp;
		buffer_pool.ins = (buffer_pool.ins+1) % BUFFER_POOL_TOTAL;
		buffer_pool.cnt_ins++;
		ret=0;
		}

#if defined(VOIP_USER_BUFFER_POOL_USE_MUTEX)
	VOIP_USER_BUFFER_POOL_LEAVE_CRITICAL_SECTION();
#else
	VOIP_USER_BUFFER_POOL_LEAVE_CRITICAL_SECTION(flags);
#endif
	
	return ret;
}


void *
voip_buffer_pool_dequeue( void )
{
	void *retp = NULL;
	
#if defined(VOIP_USER_BUFFER_POOL_USE_MUTEX)
		VOIP_USER_BUFFER_POOL_ENTER_CRITICAL_SECTION();
#else
		unsigned long flags;
		VOIP_USER_BUFFER_POOL_ENTER_CRITICAL_SECTION(flags);
#endif


	if(voip_buffer_pool_empty()){
		retp = NULL;
		}
	else{
		retp = buffer_pool.bc[ buffer_pool.del ];
		memset(retp , 0 , PRIVATE_USER_BUFFER_SIZE);
		buffer_pool.del = (buffer_pool.del+1) % BUFFER_POOL_TOTAL;
		buffer_pool.cnt_del++;
		}

#if defined(VOIP_USER_BUFFER_POOL_USE_MUTEX)
		VOIP_USER_BUFFER_POOL_LEAVE_CRITICAL_SECTION();
#else
		VOIP_USER_BUFFER_POOL_LEAVE_CRITICAL_SECTION(flags);
#endif


	return retp;
}


voip_initcall( voip_user_buffer_pool_init );

#endif

int do_voip_mgr_ctl_in_host( int cmd, void *user, unsigned int len, const voip_mgr_entry_t *p_entry )
{
	mgr_flags_t flags = p_entry ->flags;
	int ret = 0;
	uint32 mgr_chid;
	int copy_to_user_buffer_ok = 0;
	
	volatile static int entry = 0;
	//unsigned char *user_buffer = NULL;

#ifdef VOIP_USER_BUFFER_POOL_SUPPORT
	const unsigned char *user_buffer = (unsigned char*)voip_buffer_pool_dequeue();
#else
	const unsigned char *user_buffer = (unsigned char*)kzalloc(USER_BUFFER_SIZE , GFP_KERNEL);
#endif

#if 0	
	switch( cmd ) {
	case VOIP_MGR_CHECK_DSP_ALL_SW_READY:
	case IPC_EVT_FAX_MODEM_PASS_FIFO:
		break;
	
	default:
		HSND_PRINTK( ">%d %d\n", cmd, len );
		break;
	}
#endif
	
	//PRINT_MSG( "%s\n", p_entry ->cmd_name );
	
	// check re-entry. Because we have only one copy of user_buffer 
	while( entry ) {
		//PRINT_R( "host voip_mgr_ctl re-entry!! (old=%d new=%d)\n", entry, cmd );
		schedule();
	}
	
	entry = ( cmd ? cmd : 1 );
	
	if(user_buffer == NULL){
		goto label_err_return;
	}
		
	// check type size 
	if( ( flags & MF_AUTOFW ) ) {
		if( cmd == VOIP_MGR_PLAY_IVR )
			;	// structure fill header only 
		else {
			if( ( flags & MF_CHANNEL ) && ( p_entry ->type_size != len ) ) 
			{
				PRINT_R( "cmd %d field size is different %d!=%d\n", cmd, p_entry ->type_size, len );
				ret = -EVOIP_IOCTL_IPC_HOST_SIZE_CHECK_ERR;
				goto label_err_return;
			}
		}
		if( len > USER_BUFFER_SIZE ) {
			PRINT_R( "cmd %d length is too long %d>%d\n", cmd, len, USER_BUFFER_SIZE );
			ret = -EVOIP_IOCTL_IPC_HOST_SIZE_CHECK_ERR;
			goto label_err_return;
		}
	}
	
	// check if SND locates in host 
	if( ( flags & ( MF_AUTOFW | MF_CHANNEL | MF_SNDCMD ) )
			   == ( MF_AUTOFW | MF_CHANNEL | MF_SNDCMD ) )
	{
		// channel: M_NORMAL_SND_MGR, M_FETCH_SND_MGR  
		if ((ret = copy_from_user( (void *)user_buffer, user, len )) != 0)
		{
			ret = -EVOIP_IOCTL_COPY_FROM_USER_ERR;
			goto label_err_return;
		}
		copy_to_user_buffer_ok = 1;
		
		if ((ret = read_mgr_chid( &mgr_chid, (void *)user_buffer, len, p_entry )) != 0)
			goto label_err_return;

		// if SND locates host 
		if( snd_locate_host_cch( mgr_chid ) ) {
			flags &= ~MF_AUTOFW;
			flags |= MF_BODY;
		}
	} else if( ( flags & ( MF_AUTOFW | MF_CHANNEL | MF_SNDCMD ) )
					  == ( MF_AUTOFW |      0     | MF_SNDCMD ) )
	{
		// no channel: M_NOCHANNEL_SND_MGR
		
		// if SND locates host 
		flags |= MF_BODY;
	}
	
	// main process 
	if( flags & MF_AUTOFW ) {
		
		if( !copy_to_user_buffer_ok )
		{
			if ((ret = copy_from_user( (void *)user_buffer, user, len )) != 0)
			{
				ret = -EVOIP_IOCTL_COPY_FROM_USER_ERR;
				goto label_err_return;
			}
		}
		
		if( flags & MF_CHANNEL ) {
			uint32 dsp_cpuid, dsp_cch;
			
			//mgr_chid = stVoipCfg.ch_id;
			if ((ret = read_mgr_chid( &mgr_chid, (void *)user_buffer, len, p_entry )) != 0)
				goto label_err_return;

			//stVoipCfg.ch_id = API_get_DSP_CH(cmd, stVoipCfg.ch_id);	// convert to DSP chid
			if ((ret = API_get_DSP_info( cmd, mgr_chid, &dsp_cpuid, &dsp_cch )) != 0)
				goto label_err_return;

			PRINT_MSG( "%s ch_id=%d(%d:%d)\n", p_entry ->cmd_name, mgr_chid, dsp_cpuid, dsp_cch );
			if ((ret = write_mgr_chid( dsp_cch, (void *)user_buffer, len, p_entry )) != 0)
				goto label_err_return;

			//ret = ethernetDspSentL2ControlPacket(cmd, mgr_chid, &stVoipCfg, sizeof(TstVoipCfg));
			if ((ret = ipcSentControlPacket(cmd, mgr_chid, (void *)user_buffer, len, flags ))!=0)
				goto label_err_return;
			//stVoipCfg.ch_id = mgr_chid;
			//write_mgr_chid( mgr_chid, user_buffer, len, p_entry );	// move to below 
			//stVoipCfg.ret_val = ret;
		} else {
			if((p_entry ->cmd_name != NULL) && (strcmp(p_entry ->cmd_name, "VOIP_MGR_PRINT")))
				PRINT_MSG( "%s\n", p_entry ->cmd_name );

			if ((ret = ipcSentControlPacketNoChannel( cmd, (void *)user_buffer, len, flags )) != 0)
				goto label_err_return;
		}
	}
	
	if( flags & MF_BODY ) {
		// do_mgr function do copy_from_user by itself 
		if ((p_entry ->do_mgr) == NULL)
		{
			PRINT_R("IOCTL command %d has no do_mgr\n", cmd);
			ret = -EVOIP_IOCTL_NO_MGR_ERR;
			goto label_err_return;
		}

		ret = p_entry ->do_mgr( cmd, user, len, 0 );
	}
	
	if( flags & MF_AUTOFW ) {
		if( flags & MF_FETCH ) {
			unsigned short dsp_id;
			uint32 dsp_chid;
			
			//ethernetDspCheckL2RespPacket(cmd, &stSlicReg, &dsp_id);
			ipcCheckRespPacket(cmd, (void *)user_buffer, &dsp_id); //always return 0
			//stSlicReg.ch_id = API_get_Host_CH( dsp_id, stSlicReg.ch_id);/* Get Host chid */
			if ((ret = read_mgr_chid( &dsp_chid, (void *)user_buffer, len, p_entry )) != 0)
				goto label_err_return;

			if ( (mgr_chid = API_get_Host_CH( dsp_id, dsp_chid )) < 0 ) /* Get Host chid */
			{
				ret = mgr_chid;
				goto label_err_return;
			}
			if ((ret = write_mgr_chid( mgr_chid, (void *)user_buffer, len, p_entry )) != 0)
				goto label_err_return;

			//stSlicReg.ret_val = ret; // update ret_val must after check response ack      	
			
			if ((ret = copy_to_user( user, (void *)user_buffer, len )) != 0)
			{
				ret = -EVOIP_IOCTL_COPY_TO_USER_ERR;
				goto label_err_return;
			}
		} else if( flags & MF_CHANNEL ) {
			if ((ret = write_mgr_chid( mgr_chid, (void *)user_buffer, len, p_entry )) != 0)
				goto label_err_return;
		}
	}

label_err_return:

	entry = 0;

	if (user_buffer){
#ifdef VOIP_USER_BUFFER_POOL_SUPPORT
		voip_buffer_pool_enqueue((void *)user_buffer);
#else
		kfree(user_buffer);
#endif
	}
	
	return ret;
}

/**********************************************/

#if 0
void __init voip_mgr_help_host_init(void)
{
	extern void __init ipc_arch_is_host_init(void);

	//int i = 0, j = 0;
	
	PRINT_Y("=== IPC: Host Init ===\n");	
	
	ipc_arch_is_host_init();

#if 0	
	for (j=0; j<CONFIG_RTK_VOIP_DSP_DEVICE_NR; j++)
	{
		for (i=0; i<SessNumPerDsp[j]; i++)
		{
			API_OpenSid_Dsp(i, 0, j); // mid 0
			API_OpenSid_Dsp(i, 1, j); // mid 1
			PRINT_Y("Open sid%d\n", i);
		}
	}
#endif
	
	PRINT_Y("=== Done ===\n");
	
}
#endif

#if 0
void __exit voip_mgr_help_host_exit(void)
{
	PRINT_Y("============= IPC: Host exit ============\n");
	
	PRINT_Y("==> Done !\n");
}
#endif

