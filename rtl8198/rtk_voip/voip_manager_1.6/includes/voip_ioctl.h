#ifndef __VOIP_IOCTL__
#define __VOIP_IOCTL__

#define VOIP_MGR_IOCTL_DEV_NAME		"/dev/voip/mgr"
#define VOIP_TAPI_MUTEX 0
extern int g_VoIP_Mgr_FD;
extern int voip_tapi_sem_wait(const int sem_num);
extern int voip_tapi_sem_signal(const int sem_num);

#define VOIP_IOCTL(optid, varptr, vartype, qty, ioctl_fn) \
		{	\
			int ret;	\
						\
			if( g_VoIP_Mgr_FD < 0 )	\
				return -1;	\
			if (voip_tapi_sem_wait(VOIP_TAPI_MUTEX) == -1)	\
				return -1;	\
			if( ( ret = ioctl_fn( g_VoIP_Mgr_FD, 	\
						VOIP_MGR_IOC_CMD( optid, sizeof(vartype)*qty ), 	\
						varptr ) ) < 0 ) 	\
			{							\
				if (voip_tapi_sem_signal(VOIP_TAPI_MUTEX) == -1)	\
					return -1;	\
				return ret;				\
			}							\
			if (voip_tapi_sem_signal(VOIP_TAPI_MUTEX) == -1)	\
				return -1;	\
		}

#ifdef __ECOS
extern int linux_wrapper_chrdev_ioctl( int handle, unsigned int cmd, unsigned long arg );
#define SETSOCKOPT(optid, varptr, vartype, qty)	\
			VOIP_IOCTL( optid, ( unsigned long )varptr, vartype, qty, linux_wrapper_chrdev_ioctl )
#else
#define SETSOCKOPT(optid, varptr, vartype, qty)	\
			VOIP_IOCTL( optid, varptr, vartype, qty, ioctl );
#endif

#endif /* __VOIP_IOCTL__ */

