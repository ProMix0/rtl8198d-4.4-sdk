#include <linux/module.h>
#include "rtk_voip.h"
#include "kernel_module.h"

int __init voip_module_init_fns(void)
{
#if defined(CONFIG_RTK_VOIP_STANDALONE_ARCH) && defined(CONFIG_RTK_VOIP_PLATFORM_7742)
	ca77xx_audio_init();
#endif

#if !defined(CONFIG_RTK_VOIP_LINUX_SPI)
	slicInterface();
#endif

#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ZARLINK
	VpSysInitParams();
#endif
#ifdef SLICDUMPREG
#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_MICROSEMI)
	Voip_Proc_Dumpreg_Init();
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
	voip_proc_dumpreg_init();
#endif
#if (defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9641) || \
     defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9643) || \
     defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9651) || \
     defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9653) || \
     defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9661))
	voip_proc_dumpreg2_init();
#endif
#if (defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x) || \
     defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3228x))
	voip_proc_add_test5_init();
#endif
#endif /* SLICDUMPREG */
#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_INTEL)
	kTAPI_ModuleCreate();
	dxs_module_init();
	dxs_firmware_init();
#endif

#ifdef CONFIG_RTK_VOIP_LINUX_SPI
	spi_linux_wrapper_init();
#endif

	return 0;
}

int __init voip_initcall_entry_fns(void)
{
	voip_con_start_setup_init();

	return 0;
}

int __init voip_initcall_bus_fns(void)
{
#ifdef CONFIG_RTK_VOIP_DRIVERS_IIS
	voip_bus_iis_init();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_PCM
	voip_bus_pcm_init();
#endif

	return 0;
}

int __init voip_initcall_ioc_relay_fns(void)
{
#ifdef CONFIG_RTK_VOIP_IOC_SLIC_RELAY
	voip_init_ioc_gpio_relay();
#endif

	return 0;
}

int __init voip_initcall_snd_setup_fns(void)
{
	voip_snd_reset();

	return 0;
}

int __init voip_initcall_snd_fts_fns(void)
{
	return 0;
}

int __init voip_initcall_snd_fns(void)
{
	/* Legerity/Zarlink/Microsemi */
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE88111
	voip_snd_zarlink_init_le88111();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE89116
	voip_snd_zarlink_init_le89116();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE88221
	voip_snd_zarlink_init_le88221();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE88601
	voip_snd_zarlink_init_le88601();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE89316
	voip_snd_zarlink_init_le89316();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9622
	voip_snd_zarlink_init_le9622();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9632
	voip_snd_zarlink_init_le9632();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9641
	voip_snd_zarlink_init_le9641();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9643
	voip_snd_zarlink_init_le9643();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9652
	voip_snd_zarlink_init_le9652();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9651
	voip_snd_zarlink_init_le9651();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9653
	voip_snd_zarlink_init_le9653();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9661
	voip_snd_zarlink_init_le9661();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9662
	voip_snd_zarlink_init_le9662();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_MICROSEMI
	Voip_Snd_Microsemi_Init();
#endif

	/* Silicon Labs */
#ifdef CONFIG_RTK_VOIP_DRIVERS_SI3050
	voip_snd_si3050_init();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3217x
	voip_snd_proslic_init_3217x();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3218x
	voip_snd_proslic_init_3218x();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3219x
	voip_snd_proslic_init_3219x();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226
	voip_snd_si3226_init();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3226x
	voip_snd_si3226x_init();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_SI3228x
	voip_snd_si3228x_init();
#endif

	/* Intel */
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_INTEL
	voip_snd_intel_init_dxs();
#endif

	/* Realtek */
#ifdef CONFIG_RTK_VOIP_DRIVERS_CODEC_ALC5621
	voip_snd_alc5621_init();
#endif

#ifdef CONFIG_RTK_VOIP_SLIC_RELAY
	voip_snd_slic_relay_init();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_VIRTUAL_DAA
	voip_snd_virtual_daa_init();
#endif
#ifdef CONFIG_RTK_VOIP_IPC_ARCH_FULLY_OFFLOAD
	voip_snd_init_shadow_slic();
	voip_snd_init_shadow_daa();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_MIRROR_SLIC
	voip_snd_init_mirror_slic();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_MIRROR_DAA
	voip_snd_init_mirror_daa();
#endif

	return 0;
}

int __init voip_initcall_ioc_fns(void)
{
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE88221_IO
	voip_ioc_zarlink_init_le88221();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE88601_IO
	voip_ioc_zarlink_init_le88601();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE89116_IO
	voip_ioc_zarlink_init_le89116();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9622_IO
	voip_ioc_zarlink_init_le9622();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9632_IO
	voip_ioc_zarlink_init_le9632();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9641_IO
	voip_ioc_zarlink_init_le9641();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9643_IO
	voip_ioc_zarlink_init_le9643();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9652_IO
	voip_ioc_zarlink_init_le9652();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9651_I0
	voip_ioc_zarlink_init_le9651();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9653_IO
	voip_ioc_zarlink_init_le9653();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9661_IO
	voip_ioc_zarlink_init_le9661();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9662_IO
	voip_ioc_zarlink_init_le9662();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_MICROSEMI
	Voip_Ioc_Microsemi_Init();
#endif

#ifdef CONFIG_RTK_VOIP_IOC_LED
#ifndef CONFIG_RTK_VOIP_LINUX_LED
	voip_init_ioc_gpio_led();
#else
	voip_init_ioc_linux_led();
#endif
#endif

	return 0;
}

int __init voip_initcall_dsp_fns(void)
{
#if defined(CONFIG_RTK_VOIP_STANDALONE_ARCH)
	voip_dsp_rtk_register();
#endif

	return 0;
}

int __init voip_initcall_con_fns(void)
{
	voip_con_main_init();

	return 0;
}

int __init voip_initcall_fns(void)
{
	rtk_voip_init();

	/* voip_rx */
#ifdef CONFIG_RTK_VOIP_RX
	rtk_trap_init_module();
#endif

	/* voip_dsp */
#ifdef CONFIG_RTK_VOIP_STANDALONE_ARCH
	V152_Initialize();
	dtmf_decoder_init();
	fskgen_init_and_default();
	initialize_level_tracking();
	init_silence_det();
	voip_ivr_dev_init();
#endif

	/* voip_manager */
	rtk_voip_mgr_init_module();
#ifndef CONFIG_RTK_VOIP_IPC_ARCH_IS_DSP
	voip_mgr_dev_init();
#endif
	check_voip_mgr_table();
	voip_events_init();
#ifdef CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST
	voip_user_buffer_pool_init();
#endif

	/* voip_drivers */
#ifdef CONFIG_RTK_VOIP_PULSE_DIAL_GEN_TIMER
#if defined (CONFIG_RTK_VOIP_DRIVERS_PCM865xC) || defined (CONFIG_RTK_VOIP_DRIVERS_PCM8972B_FAMILY) || defined(CONFIG_RTK_VOIP_DRIVERS_PCM89xxC)
	timer1_init();
#endif
#endif
#ifdef CONFIG_RTK_VOIP_QOS
	init_check_voip_channel_loading();
#endif
#ifndef CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST
	voip_dsp_rtk_init();
#endif
	voip_init_led_blinking_timer();
#ifdef CONFIG_RTK_VOIP_DRIVERS_PCM89xxD
	_init_GpioShare_setting();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_PCM89xxC
	_init_GpioShare_setting();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_IP_PHONE
	ipphone_init();
#endif

	/* voip_fs */
	voip_proc_structure_init();
	ethernet_dsp_info_init();
#ifdef CONFIG_RTK_VOIP_IPC_ARCH
	voip_dd_ipc_init();
#endif
	voip_dd_dev_init();
#ifdef CONFIG_RTK_VOIP_CIRCULAR_LOG_DEV
	voip_cl_dev_init();
#endif

	/* voip_ipc */
#ifdef CONFIG_RTK_VOIP_COPROCESS_DSP
	coprocessor_dsp_rx_init();
#endif
#ifdef CONFIG_RTK_VOIP_ETHERNET_DSP
	rtk_voip_ethernet_dsp_init();
	ethernet_dsp_rx_init();
#endif

	return 0;
}

int __init voip_initcall_proc_fns(void)
{
	voip_proc_debug_init();
#if defined(CONFIG_DEFAULTS_KERNEL_3_10)
#if !defined(CONFIG_RTK_VOIP_STANDALONE_ARCH)
	voip_proc_reset_event_history_init();
#endif
#endif

	/* voip_rx */
#ifdef CONFIG_RTK_VOIP_RX
	voip_trap_proc_init();
#endif

	/* voip_dsp */
#ifdef CONFIG_RTK_VOIP_STANDALONE_ARCH
	voip_v152_proc_init();
	voip_dsp_pktrx_proc_init();
	voip_gain_proc_init();
	voip_dtmf_det_proc_init();
	voip_fax_proc_init();
	voip_cedtone_proc_init();
	voip_anstone_proc_init();
	voip_lec_proc_init();
	voip_level_cluster_proc_init();
	voip_level_tracking_proc_init();
	voip_silence_det_proc_init();
	voip_rtp_receiver_proc_init();
	voip_rfc2833_tx_debug_proc_init();
	voip_rfc2833_proc_init();
	voip_rtp_transmitter_proc_init();
#endif

	/* voip_manager */
#ifndef AUDIOCODES_VOIP
	voip_dsp_channel_info_proc_init();
#endif
	voip_events_proc_init();
#ifdef CONFIG_RTK_VOIP_LOG_IOCTL
	voip_proc_log_ioctl_init();
#endif

	/* voip_drivers */
	voip_con_main_proc_init();
#ifdef CONFIG_RTK_VOIP_QOS
	voip_check_channel_loading_proc_init();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_IIS
	voip_iis_proc_init();
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_PCM
	voip_pcm_proc_init();
#endif
#ifdef CONFIG_RTK_VOIP_CON_POLLING_PROCESS
	voip_hook_proc_init();
#endif
#ifndef CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST
	voip_clid_proc_init();
#endif
#ifndef CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST
#ifdef CONFIG_RTK_VOIP_T38
	voip_t38_proc_init();
#endif
#endif
#ifdef CONFIG_RTK_VOIP_IPC_ARCH_IS_DSP
	voip_event_tx_proc_init();
#endif
	voip_feature_proc_init();
	voip_con_register_proc_init();
#ifdef CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST
	voip_host_cch_invert_table_proc_init();
#endif
#ifndef CONFIG_RTK_VOIP_LINUX_SPI
#ifdef CONFIG_RTK_VOIP_HARDWARE_SPI
	voip_spi_proc_init();
#endif
#endif
	voip_timer_proc_init();

	/* voip_fs */
	voip_version_init();
	voip_proc_echo_sel_init();
	voip_proc_add_echo_init();
#ifdef CONFIG_RTK_VOIP_QOS
	voip_proc_qos_init();
#endif
	voip_info_init();
	voip_cpu_info_proc_init();
	voip_proc_counter_init();
	voip_devices_proc_init();

	return 0;
}

int __init voip_initcall_led_fns(void)
{
#ifdef CONFIG_RTK_VOIP_LED
	LED_Init();
#endif

	return 0;
}

int __init voip_initcall_sync_fns(void)
{
	voip_con_init_polling_timer();

#ifdef CONFIG_RTK_VOIP_IPC_ARCH_IS_DSP
	Add_event_tx_timer();
#endif

#ifdef CONFIG_RTK_VOIP_IPC_ARCH_IS_DSP
#ifdef CONFIG_RTL8686_IPC_DEV
	Set_DSP_Ready();
#endif
#endif

	return 0;
}

int __exit voip_exitcall_fns(void)
{
	return 0;
}

int __exit voip_module_exit_fns(void)
{
	return 0;
}

static int __init voip_module_init(void)
{
	voip_module_init_fns();

	/* initcall_level 98x */
	voip_initcall_entry_fns();
	voip_initcall_bus_fns();
	voip_initcall_ioc_relay_fns();
	voip_initcall_snd_setup_fns();
	voip_initcall_snd_fts_fns();
	voip_initcall_snd_fns();
	voip_initcall_ioc_fns();
	voip_initcall_dsp_fns();
	voip_initcall_con_fns();

	/* initcall_level 99x */
	voip_initcall_fns();
	voip_initcall_proc_fns();
	voip_initcall_led_fns();
	voip_initcall_sync_fns();

	return 0;
}
module_init(voip_module_init);

static void __exit voip_module_exit(void)
{
	voip_exitcall_fns();

	voip_module_exit_fns();
}
module_exit(voip_module_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RTK VoIP kernel functions");
