<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<!--系统默认模板-->
<HTML>
<HEAD>
<TITLE>日常应用</TITLE>
<META http-equiv=pragma content=no-cache>
<META http-equiv=cache-control content="no-cache, must-revalidate">
<META http-equiv=content-type content="text/html; charset=utf-8">
<META http-equiv=content-script-type content=text/javascript>
<!--系统公共css-->
<STYLE type=text/css>
@import url(/style/default.css);
</STYLE>
<!--系统公共脚本-->
<SCRIPT language="javascript" src="common.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript">


/********************************************************************
**          on document load
********************************************************************/
function on_init()
{

}

/********************************************************************
**          on document submit
********************************************************************/
function on_submit() 
{
/*
	if( (form.preced0.value == form.preced1.value) || (form.preced0.value == form.preced2.value) || (form.preced0.value == form.preced3.value) ||
	    (form.preced1.value == form.preced2.value) || (form.preced1.value == form.preced3.value) || (form.preced2.value == form.preced3.value) )
	{
			alert("宽带电话高级设置错误，相同设定在不同优先顺位!");
			return false;
	}
*/	
	form.submit();
}
</SCRIPT>
</HEAD>

<!-------------------------------------------------------------------------------------->
<!--主页代码-->
<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000">
	<blockquote>
		<DIV align="left" style="padding-left:20px; padding-top:5px">
			<form id="form" action=/boaform/admin/voip2_cmcc_set method=POST>					
				<br><b>宽带电话高级设置<br>
				<table border="0" cellpadding="0" cellspacing="0" width="500">
					<tr>
						<td width="200px">注册周期(秒)</td>
						<td width="200px"><input name="proxy0_reg_expire" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("proxy0_reg_expire"); %>">(1-65534)</td>
					</tr>
					<tr>
						<td>最大启动时延(秒)</td>
						<td><input name="max_start_time" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("max_start_time"); %>"></td>
					</tr>				
					<tr>
						<td>注册失败再次尝试注册时间间隔(秒)</td>
						<td><input name="RegisterRetryInterval" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("RegisterRetryInterval"); %>"></td>
					</tr>
					<tr>
						<td>拨号启动定时器时长(秒)</td>
						<td><input name="off_hook_alarm" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("off_hook_alarm"); %>"></td>
					</tr>
					<tr>
						<td>拨号短定时器时长(秒)</td>
						<td><input name="auto_dial" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("auto_dial"); %>"></td>
					</tr>
					<tr>
						<td>拨号长定时器时长(秒)</td>
						<td><input name="InterDigitTimerLong" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("InterDigitTimerLong"); %>"></td>
					</tr>
					<tr>
						<td>拨号T定时器时长(秒)</td>
						<td><input name="t_timer" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("t_timer"); %>"></td>
					</tr>					
					<tr>
						<td>忙音时长(秒)</td>
						<td><input name="BusyToneTimer" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("BusyToneTimer"); %>"></td>
					</tr>
					<tr>
						<td>翱鸣音时长(秒)</td>
						<td><input name="HangingReminderToneTimer" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("HangingReminderToneTimer"); %>"></td>
					</tr>
					<tr>
						<td>久叫不应时间(秒)</td>
						<td><input name="NoAnswerTimer" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("NoAnswerTimer"); %>">(0:关闭)</td>
					</tr>
					<tr>
						<td>号码表T模式</td> 
						<td><select name=digit_timer_mode> <%voip_e8c_get("digit_timer_mode");%></select></td>
					</tr>				
					<tr>
						<td>DTMF传输方式</td>
						<td><select name=dtmfMode onchange="dtmfMode_change()"> <%voip_e8c_get("dtmfMode");%></select></td>
					</tr>
					<tr>
						<td>闪断转发设置</td>  
						<td><select name=hookflash_type> <%voip_e8c_get("hookflash_type");%></select></td>
					</tr>
					<tr>
						<td>心跳消息时长(秒)</td>
						<td><input name="HeartbeatCycle" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("HeartbeatCycle"); %>">(0:关闭)</td>
					</tr>
					<tr>
						<td>心跳OPTION模式</td>
						<td><select name=heartbeat_mode> <%voip_e8c_get("heartbeat_mode");%></select></td>
					</tr>
					<tr>
						<td>传真协商模式</td>
						<td><select name=FAX_options> <%voip_e8c_get("FAX_options");%></select></td>
					</tr>
					<tr>
						<td>传真透传编解码</td> 
						<td><select name=V152_codec_type> <%voip_e8c_get("V152_codec_type");%></select></td>
					</tr>
					<tr>
						<td>RTP冗余</td>
						<td><select name="rtp_redundant_codec_options"> <%voip_e8c_get("rtp_redundant_codec_options");%></select></td>
					</tr>
					<tr>
						<td>RTP冗余payload type</td>
						<td><input name="rtp_redundant_payload_type" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("rtp_redundant_payload_type"); %>"> (96-127)</td>
					</tr>		
					<tr>
						<td>热线实现方式</td>
						<td><select name=hotline_mode> <%voip_e8c_get("hotline_mode");%></select></td>
					</tr>				
					<tr>
						<td>注册请求是否带端口号</td>
						<td><select name=register_wtih_port> <%voip_e8c_get("register_wtih_port");%></select></td>
					</tr>
					<tr>
						<td>报文是否带"user=phone"</td>
						<td><select name=user_equal_phone> <%voip_e8c_get("user_equal_phone");%></select></td>
					</tr>
					<tr>
						<td>热线激活码</td>
						<td><input name="hotline_activate" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("hotline_activate"); %>"></td>
					</tr>
					<tr>
						<td>呼叫等待类型</td>
						<td><select name=call_waiting_type> <%voip_e8c_get("call_waiting_type");%></select></td>
					</tr>
					<tr>
						<td>未注册拨号音开关</td>
						<td><select name=unreg_dial_tone> <%voip_e8c_get("unreg_dial_tone");%></select></td>
					</tr>						
					<tr>
						<td>极性反转方式</td>
						<td><select name="reverse_polariy"> <%voip_e8c_get("reverse_polariy");%></select></td>
					</tr>
					<tr>
						<td>语音编码优先级控制</td>	
						<td><select name=codec_prior_control> <%voip_e8c_get("codec_prior_control");%></select></td>
					</tr>
					<tr>
						<td>号码匹配方式</td>
						<td><select name=digit_map_mode> <%voip_e8c_get("digit_map_mode");%></select></td>
					</tr>
					<tr>
						<td>补充业务类型</td>
						<td><select name=business_type> <%voip_e8c_get("business_type");%></select></td>
					</tr>
					<tr>
						<td>终止符处理模式</td>
						<td><select name=terminator_type> <%voip_e8c_get("terminator_type");%></select></td>
					</tr>			
					<tr>
						<td>支持#转换%23</td>
						<td><input type=checkbox name=pound_to_ascii size=20 <%voip_e8c_get("pound_to_ascii");%>></td>
					</tr>
					<tr>
						<td>支持@转换%40</td>
						<td><input type=checkbox name=at_to_ascii size=20 <%voip_e8c_get("at_to_ascii");%>></td>
					</tr>				
					<tr>
						<td>支持T38</td>
						<td><input type=checkbox name=useT38 size=20 <%voip_e8c_get("useT38");%>></td>
					</tr>
					<tr>
						<td>回声消除启用</td>
						<td><input type=checkbox name=useLec size=20 <%voip_e8c_get("useLec");%>></td>
					</tr>
					<tr>
						<td>舒适噪音启用</td>
						<td><input type=checkbox name=useCng size=20 <%voip_e8c_get("useCng");%>></td>
					</tr>
					<tr>
						<td>静音压缩启用</td>
						<td><input type=checkbox name=useVad size=20 <%voip_e8c_get("useVad");%>></td>
					</tr>
					<tr>
						<td>支持prack</td>
						<td><input type=checkbox name=sip_disable_prack size=20 <%voip_e8c_get("sip_disable_prack");%>></td>
					</tr>
					<tr>
						<td>是否先发送注销消息</td>
						<td><input type=checkbox name=priority_unregister size=20 <%voip_e8c_get("priority_unregister");%>></td>
					</tr>				
					<tr>
						<td>自动调整同步话机时间开关</td>
						<td><input type=checkbox name=sip_date size=20 <%voip_e8c_get("sip_date");%>></td>
					</tr>
					<tr>
						<td>是否校准终端时间</td>	
						<td><input type=checkbox name=cal_terminal_time size=20 <%voip_e8c_get("cal_terminal_time");%>></td>
					</tr>
					<tr>
						<td>启用802.1p设置</td>  
						<td><input type=checkbox name=enable_802_11p size=20 <%voip_e8c_get("enable_802_11p");%>></td>
					</tr>
					<tr>
						<td>会话更新周期(秒)</td>
						<td><input name="proxy0_sessionExpiry" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("proxy0_sessionExpiry"); %>">(0:关闭)</td>
					</tr>
					<tr>
						<td>最小会话更新周期</td>
						<td><input name="minSessionExpiry" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("minSessionExpiry"); %>"></td>
					</tr>
					<tr>
						<td>会话更新方</td>
						<td><select name=uac_uas> <%voip_e8c_get("uac_uas");%></select></td>
					</tr>					
					<tr>
						<td>会话更新消息类型</td>
						<td><select name=session_options> <%voip_e8c_get("session_options");%></select></td>
					</tr>
					<tr>
						<td>TOS/DSCP优先级(SIP)</td>
						<td><input name="sipDscp" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("sipDscp"); %>">(0~63)</td>
					</tr>
					<tr>
						<td>TOS/DSCP优先级(RTP)</td>
						<td><input name="rtpDscp" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("rtpDscp"); %>">(0~63)</td>
					</tr>
					<tr>
						<td>自适应jitter buffer启用</td> 
						<td><input type=checkbox name=jitterFactor size=20 <%voip_e8c_get("jitterFactor");%>></td>
					</tr>
					<tr>
						<td>jitter buffer最大值</td>
						<td><select name="maxDelay"> <%voip_e8c_get("maxDelay");%></select></td>
					</tr>					
				</table>
				<br>

				<b>音量大小设置(dB)<br>
				<table border="0" cellpadding="0" cellspacing="0" width="500">			
					<tr>
						<td width="200px">接收音量</td>
						<td width="200px"><input type=text name=mic_voice_gain size=10 maxlength=5 value="<%voip_e8c_get("mic_voice_gain"); %>">(-32~31)</td>
					</tr>
					<tr>
						<td>发送音量</td>
						<td><input type=text name=spk_voice_gain size=10 maxlength=5 value="<%voip_e8c_get("spk_voice_gain"); %>">(-32~31)</td>
					</tr>
				</table>
				<br>
				
				<b>补充业务设置<br>
				<table border="0" cellpadding="0" cellspacing="0" width="500">
					<tr>
						<td width="200px">启用热线与热线延迟</td>
						<td width="200px"><select name=hot_line_options> <%voip_e8c_get("hot_line_options");%></select></td>
					</tr>
					<tr>
						<td>热线号码</td>
						<td><input name="hotline_number" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("hotline_number"); %>"></td>
					</tr>				
					<tr>
						<td>启用呼叫等待</td>
						<td><input type=checkbox name=call_waiting size=20 <%voip_e8c_get("call_waiting");%>></td>
					</tr>
					<tr>
						<td>启用呼叫转移</td>
						<td><input type=checkbox name=call_transfer size=20 <%voip_e8c_get("call_transfer");%>></td>
					</tr>
					<tr>
						<td>呼叫前转号码</td>
						<td><input name="uc_call_forward_number" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("uc_call_forward_number"); %>"></td>
					</tr>
					<tr>
						<td>无条件呼叫前转</td>
						<td><input type=checkbox name=uc_call_forward size=20 <%voip_e8c_get("uc_call_forward");%>></td>
					</tr>
					<tr>
						<td>遇忙呼叫前转</td>
						<td><input type=checkbox name=busy_call_forward size=20 <%voip_e8c_get("busy_call_forward");%>></td>
					</tr>
					<tr>
						<td>无应答呼叫前转</td>
						<td><input type=checkbox name=na_call_forward size=20 <%voip_e8c_get("na_call_forward");%>></td>
					</tr>
					<tr>
						<td>无应答呼叫前转等待时间</td>
						<td><input name="na_call_forward_time" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("na_call_forward_time"); %>"></td>
					</tr>
					<tr>
						<td>启用三方会议</td>
						<td><input type=checkbox name=3_way size=20 <%voip_e8c_get("3_way");%>></td>
					</tr>
					<tr>
						<td>启用服务器混音</td>
						<td><input type=checkbox name=Conference_server_cpe size=20 <%voip_e8c_get("Conference_server_cpe");%>></td>
					</tr>
					<tr>
						<td>三方会议资源接入码</td> 
						<td><input name="conference_uri" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("conference_uri"); %>"></td>
					</tr>				
					<tr>
						<td>来电显示模式</td>
						<td><%voip_e8c_get("caller_id");%></td>
					</tr>
					<tr>
						<td>订阅类型</td>
						<td><select name=SUBS_options> <%voip_e8c_get("SUBS_options");%></select></td>
					</tr>
					<tr>
						<td>订阅周期(秒)</td>
						<td><input type=text name=SUBS_time size=10 maxlength=20 value="<%voip_e8c_get("SUBS_time"); %>"></td>
					</tr>
					<tr>
						<td>最小最大拍叉簧时间设置(毫秒)<br>(10的整数倍，最小:80，最大:2000)</td>
						<td><%voip_e8c_get("flash_hook_time");%></td>
					</tr>
					<tr>
						<td>挂机最小时间间隔(毫秒)</td>
						<td><input name="min_onhook_time" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("min_onhook_time"); %>"></td>
					</tr>
					<tr>
						<td>启用同步话机时间</td>
						<td><select name=sync_phone_time> <%voip_e8c_get("sync_phone_time");%></select></td>
					</tr>							
					<tr>
						<td>获取来显号码方式</td>
						<td><select name=callid_from_PAI> <%voip_e8c_get("callid_from_PAI");%></select></td>
					</tr>				
					<tr>
						<td>语音来电号码前缀显示方式</td> 
						<td><select name=display_prefix_num> <%voip_e8c_get("display_prefix_num");%></select></td>
					</tr>					
					<tr>
						<td>URL格式</td>
						<td><select name="url_type"> <%voip_e8c_get("url_type");%></select></td>
					</tr>						
				</table>
				<br>

				
				<b>数图设置<br>
				<table border="0" cellpadding="0" cellspacing="0" width="500">				
					<tr>
						<td width="200px">启用普通数图</td>
						<td width="200px"><%voip_e8c_get("cmcc_digitmap_enable");%></td>
					</tr> 					
					<tr>
						<td>数图</td>
						<td><textarea cols=30 rows=5 name="dialplan" maxlength=4096><%voip_e8c_get("dialplan");%></textarea></td>
					</tr>					
					<tr>
						<td>启用特定数图</td>
						<td><input type=checkbox name=digitmap_special_enable size=20 <%voip_e8c_get("digitmap_special_enable");%>></td>
					</tr>
					<tr>
						<td>特定数图</td>
						<td><textarea cols=30 rows=5 name="digitmap_special" maxlength=128><%voip_e8c_get("digitmap_special");%></textarea></td>
					</tr>					
					<tr>
						<td>启用PBX前缀</td>
						<td><input type=checkbox name=PBXPrefixEnable size=20 <%voip_e8c_get("PBXPrefixEnable");%>></td>
					</tr>
					<tr>
						<td>PBX前缀</td>
						<td><input name="PBXPrefix" size="16" maxlength="128" type="text" style="width:140px" value="<%voip_e8c_get("PBXPrefix");%>"></td>
					</tr>
					<tr>
						<td>PBX启用二次拨号音提示</td>
						<td><input type=checkbox name=pbxdial_second_enable size=20 <%voip_e8c_get("pbxdial_second_enable");%>></td>
					</tr>
				</table>
				<br>
				
				<input type="button" class="btnsaveup" name="btnDown" value="保存/应用" onClick="on_submit()">
				<input type="hidden" name="submit-url" value="">
			</form>
		</DIV>
	</blockquote>
</body>
<%addHttpNoCache();%>
</html>
