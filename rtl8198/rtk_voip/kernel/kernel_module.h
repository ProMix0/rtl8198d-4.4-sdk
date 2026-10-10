#ifndef __RTK_KERNEL_MODULE__
#define __RTK_KERNEL_MODULE__

/* module_init */
#if defined(CONFIG_RTK_VOIP_STANDALONE_ARCH) && defined(CONFIG_RTK_VOIP_PLATFORM_7742)
extern int ca77xx_audio_init(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ZARLINK
extern int VpSysInitParams(void);
#endif
#ifdef SLICDUMPREG
#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_MICROSEMI)
extern int Voip_Proc_Dumpreg_Init(void);
#endif
#if (defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE88601) || \
     defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE89116) || \
     defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9622) || \
     defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9632) || \
     defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9641) || \
     defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9643) || \
     defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9651) || \
     defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9653) || \
     defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9661) || \
     defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9662) || \
     defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x) || \
     defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3218x) || \
     defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3219x))
extern int voip_proc_dumpreg_init(void);
#endif
#if (defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9641) || \
     defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9643) || \
     defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9651) || \
     defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9653) || \
     defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9661))
extern int voip_proc_dumpreg2_init(void);
#endif
#if (defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x) || \
     defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3228x))
extern int voip_proc_add_test5_init(void);
#endif
#endif /* SLICDUMPREG */
#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_INTEL)
extern int kTAPI_ModuleCreate(void);
extern int dxs_module_init(void);
extern int dxs_firmware_init(void);
#endif
#ifdef CONFIG_RTK_VOIP_LINUX_SPI
extern int spi_linux_wrapper_init(void);
#endif
#if !defined(CONFIG_RTK_VOIP_LINUX_SPI)
extern int slicInterface(void);
#endif


/* initcall_entry */
extern int voip_con_start_setup_init(void);

/* initcall_bus */
#ifdef CONFIG_RTK_VOIP_DRIVERS_IIS
extern int voip_bus_iis_init(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_PCM
extern int voip_bus_pcm_init(void);
#endif

/* initcall_ioc_relay */
#ifdef CONFIG_RTK_VOIP_IOC_SLIC_RELAY
extern int voip_init_ioc_gpio_relay(void);
#endif

/* initcall_snd_setup */
extern int voip_snd_reset(void);

/* initcall_snd_fts */

/* initcall_snd */
/** Legerity/Zarlink/Microsemi **/
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE88111
extern int voip_snd_zarlink_init_le88111(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE89116
extern int voip_snd_zarlink_init_le89116(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE88221
extern int voip_snd_zarlink_init_le88221(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE88601
extern int voip_snd_zarlink_init_le88601(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE89316
extern int voip_snd_zarlink_init_le89316(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9622
extern int voip_snd_zarlink_init_le9622(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9632
extern int voip_snd_zarlink_init_le9632(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9641
extern int voip_snd_zarlink_init_le9641(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9643
extern int voip_snd_zarlink_init_le9643(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9652
extern int voip_snd_zarlink_init_le9652(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9651
extern int voip_snd_zarlink_init_le9651(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9653
extern int voip_snd_zarlink_init_le9653(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9661
extern int voip_snd_zarlink_init_le9661(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9662
extern int voip_snd_zarlink_init_le9662(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_MICROSEMI
extern int Voip_Snd_Microsemi_Init(void);
#endif

/** Silicon Labs **/
#ifdef CONFIG_RTK_VOIP_DRIVERS_SI3050
extern int voip_snd_si3050_init(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x
extern int voip_snd_proslic_init_3217x(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3218x
extern int voip_snd_proslic_init_3218x(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3219x
extern int voip_snd_proslic_init_3219x(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226
extern int voip_snd_si3226_init(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x
extern int voip_snd_si3226x_init(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3228x
extern int voip_snd_si3228x_init(void);
#endif

/** Intel **/
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_INTEL
extern int voip_snd_intel_init_dxs(void);
#endif

/** Realtek **/
#ifdef CONFIG_RTK_VOIP_DRIVERS_CODEC_ALC5621
extern int voip_snd_alc5621_init(void);
#endif

#ifdef CONFIG_RTK_VOIP_SLIC_RELAY
extern int voip_snd_slic_relay_init(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_VIRTUAL_DAA
extern int voip_snd_virtual_daa_init(void);
#endif
#ifdef CONFIG_RTK_VOIP_IPC_ARCH_FULLY_OFFLOAD
extern int voip_snd_init_shadow_slic(void);
extern int voip_snd_init_shadow_daa(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_MIRROR_SLIC
extern int voip_snd_init_mirror_slic(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_MIRROR_DAA
extern int voip_snd_init_mirror_daa(void);
#endif

/* initcall_ioc */
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE88221_IO
extern int voip_ioc_zarlink_init_le88221(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE88601_IO
extern int voip_ioc_zarlink_init_le88601(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE89116_IO
extern int voip_ioc_zarlink_init_le89116(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9622_IO
extern int voip_ioc_zarlink_init_le9622(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9632_IO
extern int voip_ioc_zarlink_init_le9632void();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9641_IO
extern int voip_ioc_zarlink_init_le9641(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9643_IO
extern int voip_ioc_zarlink_init_le9643(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9652_IO
extern voip_ioc_zarlink_init_le9652(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9651_IO
extern voip_ioc_zarlink_init_le9651(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9653_IO
extern voip_ioc_zarlink_init_le9653(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9661_IO
extern voip_ioc_zarlink_init_le9661(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9662_IO
extern int voip_ioc_zarlink_init_le9662(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_MICROSEMI
extern int Voip_Ioc_Microsemi_Init(void);
#endif

#ifdef CONFIG_RTK_VOIP_IOC_LED
#ifndef CONFIG_RTK_VOIP_LINUX_LED
extern int voip_init_ioc_gpio_led(void);
#else
extern int voip_init_ioc_linux_led(void);
#endif
#endif

/* initcall_dsp */
#if defined(CONFIG_RTK_VOIP_STANDALONE_ARCH)
extern int voip_dsp_rtk_register(void);
#endif

/* initcall_con */
extern int voip_con_main_init(void);

/* initcall */
extern int rtk_voip_init(void);

/** voip_rx **/
#ifdef CONFIG_RTK_VOIP_RX
extern int rtk_trap_init_module(void);
#endif

/** voip_dsp **/
#ifdef CONFIG_RTK_VOIP_STANDALONE_ARCH
extern int V152_Initialize(void);
extern int dtmf_decoder_init(void);
extern int fskgen_init_and_default(void);
extern int initialize_level_tracking(void);
extern int init_silence_det(void);
extern int voip_ivr_dev_init(void);
#endif

/** voip_manager **/
extern int rtk_voip_mgr_init_module(void);
#ifndef CONFIG_RTK_VOIP_IPC_ARCH_IS_DSP
extern int voip_mgr_dev_init(void);
#endif
extern int check_voip_mgr_table(void);
extern int voip_events_init(void);
#ifdef CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST
extern int voip_user_buffer_pool_init(void);
#endif

/** voip_drivers **/
#ifdef CONFIG_RTK_VOIP_PULSE_DIAL_GEN_TIMER
#if defined (CONFIG_RTK_VOIP_DRIVERS_PCM865xC) || defined (CONFIG_RTK_VOIP_DRIVERS_PCM8972B_FAMILY) || defined(CONFIG_RTK_VOIP_DRIVERS_PCM89xxC)
extern int timer1_init(void);
#endif
#endif
#ifdef CONFIG_RTK_VOIP_QOS
extern int init_check_voip_channel_loading(void);
#endif
#if defined(CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST) || defined(CONFIG_RTK_VOIP_STANDALONE_ARCH)
extern int voip_dsp_rtk_init(void);
#endif
extern int voip_init_led_blinking_timer(void);
#ifdef CONFIG_RTK_VOIP_DRIVERS_PCM89xxD
extern int _init_GpioShare_setting(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_PCM89xxC
extern int _init_GpioShare_setting(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_IP_PHONE
extern int ipphone_init(void);
#endif

/** voip_fs **/
extern int voip_proc_structure_init(void);
extern int ethernet_dsp_info_init(void);
#ifdef CONFIG_RTK_VOIP_IPC_ARCH
extern int voip_dd_ipc_init(void);
#endif
extern int voip_dd_dev_init(void);
#ifdef CONFIG_RTK_VOIP_CIRCULAR_LOG_DEV
extern int voip_cl_dev_init(void);
#endif

/** voip_ipc **/
#ifdef CONFIG_RTK_VOIP_COPROCESS_DSP
extern int coprocessor_dsp_rx_init(void);
#endif
#ifdef CONFIG_RTK_VOIP_ETHERNET_DSP
extern int rtk_voip_ethernet_dsp_init(void);
extern int ethernet_dsp_rx_init(void);
#endif

/* initcall_proc */
extern int voip_proc_debug_init(void);
extern int voip_proc_reset_event_history_init(void);

/** voip_rx **/
#ifdef CONFIG_RTK_VOIP_RX
extern int voip_trap_proc_init(void);
#endif

/** voip_dsp **/
#ifdef CONFIG_RTK_VOIP_STANDALONE_ARCH
extern int voip_v152_proc_init(void);
extern int voip_dsp_pktrx_proc_init(void);
extern int voip_gain_proc_init(void);
extern int voip_dtmf_det_proc_init(void);
extern int voip_fax_proc_init(void);
extern int voip_cedtone_proc_init(void);
extern int voip_anstone_proc_init(void);
extern int voip_lec_proc_init(void);
extern int voip_level_cluster_proc_init(void);
extern int voip_level_tracking_proc_init(void);
extern int voip_silence_det_proc_init(void);
extern int voip_rtp_receiver_proc_init(void);
extern int voip_rfc2833_tx_debug_proc_init(void);
extern int voip_rfc2833_proc_init(void);
extern int voip_rtp_transmitter_proc_init(void);
#endif

/** voip_manager **/
#ifndef AUDIOCODES_VOIP
extern int voip_dsp_channel_info_proc_init(void);
#endif
extern int voip_events_proc_init(void);
#ifdef CONFIG_RTK_VOIP_LOG_IOCTL
extern int voip_proc_log_ioctl_init(void);
#endif

/** voip_drivers **/
extern int voip_con_main_proc_init(void);
#ifdef CONFIG_RTK_VOIP_QOS
extern int voip_check_channel_loading_proc_init(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_IIS
extern int voip_iis_proc_init(void);
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_PCM
extern int voip_pcm_proc_init(void);
#endif
#ifdef CONFIG_RTK_VOIP_CON_POLLING_PROCESS
extern int voip_hook_proc_init(void);
#endif
#ifndef CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST
extern int voip_clid_proc_init(void);
#endif
#ifndef CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST
#ifdef CONFIG_RTK_VOIP_T38
extern int voip_t38_proc_init(void);
#endif
#endif
#ifdef CONFIG_RTK_VOIP_IPC_ARCH_IS_DSP
extern int voip_event_tx_proc_init(void);
#endif
extern int voip_feature_proc_init(void);
extern int voip_con_register_proc_init(void);
#ifdef CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST
extern int voip_host_cch_invert_table_proc_init(void);
#endif
#ifndef CONFIG_RTK_VOIP_LINUX_SPI
#ifdef CONFIG_RTK_VOIP_HARDWARE_SPI
extern int voip_spi_proc_init(void);
#endif
#endif
extern int voip_timer_proc_init(void);

/** voip_fs **/
extern int voip_version_init(void);
extern int voip_proc_echo_sel_init(void);
extern int voip_proc_add_echo_init(void);
#ifdef CONFIG_RTK_VOIP_QOS
extern int voip_proc_qos_init(void);
#endif
extern int voip_info_init(void);
extern int voip_cpu_info_proc_init(void);
extern int voip_proc_counter_init(void);
extern int voip_devices_proc_init(void);

/* initcall_led */
#ifdef CONFIG_RTK_VOIP_LED
extern int LED_Init(void);
#endif

/* initcall_sync */
extern int voip_con_init_polling_timer(void);

#ifdef CONFIG_RTK_VOIP_IPC_ARCH_IS_DSP
extern int Add_event_tx_timer(void);
#endif

#ifdef CONFIG_RTK_VOIP_IPC_ARCH_IS_DSP
#ifdef CONFIG_RTL8686_IPC_DEV
extern int Set_DSP_Ready(void);
#endif
#endif

#endif /* __RTK_KERNEL_MODULE__ */
