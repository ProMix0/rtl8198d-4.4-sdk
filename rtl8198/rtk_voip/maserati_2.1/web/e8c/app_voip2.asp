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
<!--<SCRIPT language="javascript" src="urlEncodeGBK.js"></SCRIPT>  -->
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
	var minSessionExpiry = form.minSessionExpiry.value;
	var sessionExpiry = form.sessionExpiry.value;
	if( (form.preced0.value == form.preced1.value) || (form.preced0.value == form.preced2.value) || (form.preced0.value == form.preced3.value) ||
	    (form.preced1.value == form.preced2.value) || (form.preced1.value == form.preced3.value) || (form.preced2.value == form.preced3.value) )
	{
			alert("宽带电话高级设置错误，相同设定在不同优先顺位!");
			return false;
	}
	
	if(parseInt(minSessionExpiry) > parseInt(sessionExpiry)) 
	{
		alert("会话更新周期 "+sessionExpiry + "< 最小会话更新周期 "+minSessionExpiry);
		return false;
	}
	
	if(parseInt(minSessionExpiry) < 90) 
	{
		alert("最小会话更新周期 最小值90秒");
		return false;
	}
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);	
	form.submit();
	//submit();
}

function TimeServerSync_change()
{
    form.NtpEnable.value = form.TimeServerSync.value;   
}    

function NtpEnable_change()
{
    form.TimeServerSync.value = form.NtpEnable.value;   
}  

function HangingReminderToneTimer_change()
{
    form.HangingReminderToneTimer2.value = form.HangingReminderToneTimer.value;   
}  

function HangingReminderToneTimer2_change()
{
    form.HangingReminderToneTimer.value = form.HangingReminderToneTimer2.value;   
}   

function InterDigitTimerLong_change()
{
    form.InterDigitTimerLong2.value = form.InterDigitTimerLong.value;     
} 

function InterDigitTimerLong2_change()
{
    form.InterDigitTimerLong.value = form.InterDigitTimerLong2.value;     
} 

</SCRIPT>
</HEAD>

<!-------------------------------------------------------------------------------------->
<!--主页代码-->
<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000">
	<blockquote>
		<DIV align="left" style="padding-left:20px; padding-top:5px">
			<form id="form" action=/boaform/admin/voip2_e8c_set method=POST>

<div id = "siptable1" <%voip_e8c_get("sip_display");%> >				
				<b>SIP</b>
</div>
<div id = "h248table1" <%voip_e8c_get("h248_display");%> >
				<b>H.248</b>
</div>										

<!--SIP-->	
<%voip_e8c_get("sip_display_mark_start");%>			
				<table border="0" cellpadding="0" cellspacing="0" width="500">
					<tr>
						<td width="200px">SIP 本地端口</td>
						<td width="200px"><input type=text name=sipPort size=10 maxlength=5 value="<%voip_e8c_get("sipPort"); %>"></td>
					</tr>
					<tr>
						<td>SIP DSCP</td>
						<td><select name=sipDscp>"<%voip_e8c_get("sipDscp");%>"</select></td>
					</tr>							
					<tr>
						<td>RTP初始端口</td>
						<td><input type=text name=rtpPort size=10 maxlength=5 value="<%voip_e8c_get("rtpPort"); %>"></td>
					</tr>
					<tr>
						<td>RTP DSCP</td>
						<td><select name=rtpDscp>"<%voip_e8c_get("rtpDscp");%>"</select></td>
					</tr>
					<tr>
						<td>二次拨号模式</td>
						<td><select name=dtmfMode onchange="dtmfMode_change()">"<%voip_e8c_get("dtmfMode");%>"</select></td>
					</tr>
				</table>
				<br>
				
				<b>宽带电话高级设置<br>
				<table border="0" cellpadding="0" cellspacing="0" width="500">
				<tr>
						<td>回声抑制设置</td>
						<td><input type="checkbox" name="echo_cancellation" value="1" <% voip_e8c_get("useLec"); %>></td>
				</tr>
				<tr>
					<td>VAD</td>
					<td><input type=checkbox name=useVad size=20 <%voip_e8c_get("useVad");%>>Enable</td>
				</tr>
				<tr>
						<td>传真模式</td>
						<td><%voip_e8c_get("fax_type");%></td>
				</tr>
				<tr>
					<td>传真协商模式</td>
					<td><select name=FAX_options> <%voip_e8c_get("FAX_options");%></select></td>
				</tr>
				<tr>
						<td>订阅类型</td>
						<td><select name=SUBS_options> <%voip_e8c_get("SUBS_options");%></select></td>
					</tr>
					<tr>
						<td>订阅周期（秒）</td>
						<td><input type=text name=SUBS_time size=10 maxlength=20 value="<%voip_e8c_get("SUBS_time"); %>"></td>
					</tr>
					<tr>
					<td>话机时间同步</td>
					<td><input type=checkbox name=sip_date size=20 <%voip_e8c_get("sip_date");%>>Enable</td>
				</tr>
				<tr>
					<td>反极性信号</td>
					<td><input type=checkbox name=reverse_polariy size=20 <%voip_e8c_get("reverse_polariy");%>>Enable</td>
				</tr>
				<tr>
					<td>三方通话</td>
					<td><input type=checkbox name=3_way size=20 <%voip_e8c_get("3_way");%>>Enable</td>
				</tr>
				<tr>
					<td>呼叫等待</td>
					<td><input type=checkbox name=call_waiting size=20 <%voip_e8c_get("call_waiting");%>>Enable</td>
				</tr>
				<tr>
					<td>热线呼叫</td>
					<td><select name=hot_line_options> <%voip_e8c_get("hot_line_options");%></select></td>
				</tr>
				<tr>
					<td>热线呼叫号码</td>
					<td><input type=text name=hotline_number size=10 maxlength=20 value="<%voip_e8c_get("hotline_number"); %>"></td>
				</tr>
				<tr>
					<td>来电显示制式</td>
					<td><%voip_e8c_get("caller_id");%></td>
				</tr>
				<tr>
					<td>接收增益</td>
						<td><input type=text name=mic_voice_gain size=10 maxlength=5 value="<%voip_e8c_get("mic_voice_gain"); %>"></td>
				</tr>
				<tr>
					<td>发送增益</td>
						<td><input type=text name=spk_voice_gain size=10 maxlength=5 value="<%voip_e8c_get("spk_voice_gain"); %>"></td>
				</tr>	
				<tr>
						<td>最小最大拍叉簧时间设置（毫秒<br>[10的整数倍，最小：80，最大：2000]</td>
					<td><%voip_e8c_get("flash_hook_time");%></td>
				</tr>
				<tr>
					<td>摘机不拨号的时间(sec)</td>
					<td><input name="off_hook_alarm" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("off_hook_alarm"); %>"></td>
				</tr>
				<tr>
					<td>位间短定时器时间(sec)</td>
					<td><input name="auto_dial" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("auto_dial"); %>"></td>
				</tr>
				<tr>
					<td>位间长定时器时间(sec)</td>
					<td><input name="InterDigitTimerLong" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("InterDigitTimerLong"); %>"></td>
				</tr>
					<tr> 
					<td>播放忙音时间(sec)</td>
					<td><input name="BusyToneTimer" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("BusyToneTimer"); %>"></td>
				</tr>
				<tr>
					<td>播放催挂音时间(sec)</td>
					<td><input name="HangingReminderToneTimer" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("HangingReminderToneTimer"); %>"></td>
				</tr>
				<tr>
					<td>注册重试周期(sec)</td>
					<td><input name="RegisterRetryInterval" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("RegisterRetryInterval"); %>"></td>
				</tr>
				<tr>
					<td>心跳时间(sec)[0:disable ]</td>
					<td><input name="HeartbeatCycle" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("HeartbeatCycle"); %>"></td>
				</tr>
				<tr>
					<td>久叫不应(sec)[0:disable ]</td>
					<td><input name="NoAnswerTimer" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("NoAnswerTimer"); %>"></td>
				</tr>
				<tr>
						<td>启用会话更新</td>
						<td><input name="sessionEnable" type="checkbox" value="enable" <% voip_e8c_get("sessionEnable"); %>></td>
					</tr>
					<tr>
						<td>会话更新周期(秒):</td>
						<td><input name="sessionExpiry" size="16" maxlength="32" type="text" value="<% voip_e8c_get("sessionExpiry"); %>"></td>
					</tr>
					<tr>
						<td>最小会话更新周期(秒)[最小值90]:</td>
						<td><input name="minSessionExpiry" size="16" maxlength="32" type="text" value="<% voip_e8c_get("minSessionExpiry"); %>"></td>
					</tr>
					<tr>
						<td>会话更新消息类型</td>
						<td><select name=session_options> <%voip_e8c_get("session_options");%></select></td>
					</tr>					
					<tr>
					<td>振铃电压</td>
					<td><select name=RingVoltage> <%voip_e8c_get("RingVoltage");%></select></td>
				</tr>
				<tr>
						<td>支持#转换%23</td>
						<td><input type=checkbox name=pound_to_ascii size=20 <%voip_e8c_get("pound_to_ascii");%>>Enable</td>
					</tr>
				</table>
				<table id="codec_config" name="codec_config"  border="1" cellpadding="0" cellspacing="0" width="500">
					<tr>
						<td>编解码方式</td>
						<td>使能</td>
						<td>打包时长(ms)</td>
						<td>优先级(1-4)</td>
					</tr>
						<% voip_e8c_get("e8c_codec"); %>
				</table>
				<table border="0" cellpadding="0" cellspacing="0" width="500">	
				<tr>
			    	<td bgColor=#aaddff width=155>数图启用</td>
			    	<td bgColor=#ddeeff width=170>
			    	<%voip_e8c_get("digitmap_enable");%>
			    	</td>
				</tr>
				<tr>
					<td bgColor=#aaddff>数图匹配模式</td>
					<td><select name=digit_map_mode> <%voip_e8c_get("digit_map_mode");%></select></td>
				</tr>			
			  	<tr>
			    	<td bgColor=#aaddff width=155>数图</td>
					</tr>			
					<tr>
						<td bgColor=#aaddff><TEXTAREA COLS=50 ROWS=5 name="dialplan" maxlength=4096><%voip_e8c_get("dialplan");%></TEXTAREA></td>
					</tr>


					<tr>
						<td>特殊数图开关</td>
						<td bgColor=#ddeeff width=170>
						<%voip_e8c_get("digitmap_special_enable");%></td>
					</tr>
					
								
					<tr>
						<td bgColor=#aaddff width=155>特殊数图</td>
					</tr>			
					<tr>
						<td bgColor=#aaddff><TEXTAREA COLS=50 ROWS=5 name="digitmap_special" maxlength=128><%voip_e8c_get("digitmap_special");%></TEXTAREA></td>
					</tr>
					<tr>
						<td>PBX出群字冠开关</td>
						<td><input type=checkbox name=PBXPrefixEnable size=20 <%voip_e8c_get("PBXPrefixEnable");%>>Enable</td>
					</tr>
					<tr>
						<td>PBX特殊拨号音开关</td>
						<td><input type=checkbox name=pbxdial_second_enable size=20 <%voip_e8c_get("pbxdial_second_enable");%>>Enable</td>
					</tr>
					<tr>
						<td>PBX出群字冠</td>
						<td><input name="PBXPrefix" size="16" maxlength="128" type="text" style="width:140px"  value="<% voip_e8c_get("PBXPrefix"); %>"></td>
					</tr>


					
					
				</table>
<%voip_e8c_get("sip_display_mark_end");%>
				<br>
<!-- H248 -->
<%voip_e8c_get("h248_display_mark_start");%>
				<table border="0" cellpadding="0" cellspacing="0" width="500">						
					<tr>
						<td width="200px">TOS/DSCP优先级(信令)</td>
						<td width="200px"><select name=sipDscp>"<%voip_e8c_get("sipDscp");%>"</select></td>
					</tr>					
					<tr>
						<td>RTP 初始端口</td>
						<td><input type=text name=rtpPort size=10 maxlength=5 value="<%voip_e8c_get("rtpPort"); %>"></td>
					</tr>					
					<tr>
						<td>TOS/DSCP优先级(RTP)</td>
						<td><select name=rtpDscp>"<%voip_e8c_get("rtpDscp");%>"</select></td>
				</tr>
					<tr>
						<td>二次拨号模式</td>
						<td><select name=dtmfMode onchange="dtmfMode_change()">"<%voip_e8c_get("dtmfMode");%>"</select></td>
					</tr>
					<tr>
						<td>DTMF 二次拨号</td>
						<td>透传方式</td>
					</tr>
				</table>
<div id = "siptable2" <%voip_e8c_get("sip_display");%> >				
				<table border="0" cellpadding="0" cellspacing="0" width="500">	
					<tr>
						<td width="200px">SIP 本地端口</td>
						<td width="200px"><input type=text name=sipPort size=10 maxlength=5 value="<%voip_e8c_get("sipPort"); %>"></td>
					</tr>
				</table>
</div>				
				<br>
				
				<b>宽带电话高级设置<br>				
				<table border="0" cellpadding="0" cellspacing="0" width="500">	
					<tr>
						<td width="200px">回声抑制设置</td>
						<td width="200px"><input type="checkbox" name="echo_cancellation" value="1" <% voip_e8c_get("useLec"); %>></td>
					</tr>				
					<tr>
						<td>VAD</td>
						<td><input type=checkbox name=useVad size=20 <%voip_e8c_get("useVad");%>>Enable</td>
					</tr>
					<tr>
						<td>传真协议</td>
						<td><%voip_e8c_get("fax_type");%></td>
					</tr>
					<tr>
						<td>传真模式</td>
						<td><select name=FAX_options> <%voip_e8c_get("FAX_options");%></select></td>
					</tr>
					<tr>
						<td>来电显示制式</td>
						<td><%voip_e8c_get("caller_id");%></td>
					</tr>
					<tr>
						<td>接收增益</td>
						<td><input type=text name=mic_voice_gain size=10 maxlength=5 value="<%voip_e8c_get("mic_voice_gain"); %>"></td>
					</tr>
					<tr>
						<td>发送增益</td>
						<td><input type=text name=spk_voice_gain size=10 maxlength=5 value="<%voip_e8c_get("spk_voice_gain"); %>"></td>
					</tr>	
					<tr>
						<td>最小最大拍叉簧时间设置（毫秒<br>[10的整数倍，最小：80，最大：2000]</td>
						<td><%voip_e8c_get("flash_hook_time");%></td>
					</tr>
					<tr>
						<td>注册重试周期(sec)</td>
						<td><input name="RegisterRetryInterval" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("RegisterRetryInterval"); %>"></td>
					</tr>
					<tr>
						<td>振铃电压</td>
						<td><select name=RingVoltage> <%voip_e8c_get("RingVoltage");%></select></td>
					</tr>
				</table>
				<table border="0" cellpadding="0" cellspacing="0" width="500">
					<tr>
						<td width="200px">摘机不拨号的时间(sec)</td>
						<td width="200px"><input name="off_hook_alarm" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("off_hook_alarm"); %>"></td>
					</tr>
					<tr>
						<td>位间短定时器时间(sec)</td>
						<td><input name="auto_dial" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("auto_dial"); %>"></td>
					</tr>
					<tr>
						<td>位间长定时器时间(sec)</td>
						<td><input name="InterDigitTimerLong" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("InterDigitTimerLong"); %>" onchange="InterDigitTimerLong_change()"></td>
					</tr>
					<tr> 
						<td>播放忙音时间(sec)</td>
						<td><input name="BusyToneTimer" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("BusyToneTimer"); %>"></td>
					</tr>
					<tr>
						<td>播放催挂音时间(sec)</td>
						<td><input name="HangingReminderToneTimer" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("HangingReminderToneTimer"); %>" onchange="HangingReminderToneTimer_change()"></td>
					</tr>		
					<tr>
						<td>心跳时间(sec)[0:disable ]</td>
						<td><input name="HeartbeatCycle" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("HeartbeatCycle"); %>"></td>
					</tr>			
					<tr>	
						<td>久叫不应(sec)[0:disable ]</td>
						<td><input name="NoAnswerTimer" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("NoAnswerTimer"); %>"></td>
					</tr>
				</table>	
				<table border="0" cellpadding="0" cellspacing="0" width="500">
					<tr>
						<td width="200px">jitter buffer 静态</td>
						<td width="200px"><input name="JitterBufferEnable" type="checkbox" value="enable" <% voip_e8c_get("JitterBufferEnable"); %>></td>
					</tr>
					<tr>
						<td>抖动缓冲区:</td>
						<td><select name="jitterDelay"<% voip_e8c_get("jitterDelay"); %>></select></td>
					</tr>
					<tr>
						<td>DTMF Payload</td>
						<td><input name="dtmf_2833_pt" size="16" maxlength="32" type="text" value="<% voip_e8c_get("dtmf_2833_pt"); %>"></td>
					</tr>
					<tr>
						<td>心跳方式</td>
						<td><select name="heartbeat_mode"> <%voip_e8c_get("heartbeat_mode");%></select></td>
					</tr>
					<tr>
						<td>SIP 注册周期 (秒)</td>
						<td><input name="sip_reg_expire" size="16" maxlength="32" type="text" value="<% voip_e8c_get("sip_reg_expire"); %>"></td>
					</tr>
					<tr>
						<td>同步话机时间功能</td>
						<td><input name="NtpEnable" type="checkbox" value="enable" <% voip_e8c_get("NtpEnable"); %> onchange="NtpEnable_change()"></td>
					</tr>
					<tr>
						<td>内置嗷鸣音定时器 (秒)</td>
						<td><input name="HangingReminderToneTimer2" size="16" maxlength="32" type="text" value="<% voip_e8c_get("HangingReminderToneTimer2"); %>" onchange="HangingReminderToneTimer2_change()"></td>
					</tr>					
					<tr>
						<td>数图最长定时器 (秒)</td>
						<td><input name="InterDigitTimerLong2" size="16" maxlength="32" type="text" value="<% voip_e8c_get("InterDigitTimerLong2"); %>" onchange="InterDigitTimerLong2_change()"></td>
					</tr>					
					<tr>
						<td>号码智能匹配</td>
						<td><%voip_e8c_get("digitmap_enable");%></td>
					</tr>					
				</table>				
				<table id="codec_config" name="codec_config"  border="1" cellpadding="0" cellspacing="0" width="500">
					<tr>
						<td>编解码方式</td>
						<td>使能</td>
						<td>RTP 打包时长(ms)</td>
						<td>优先级(1-4)</td>
					</tr>
					<% voip_e8c_get("e8c_codec"); %>
				</table>
<div id = "siptable3" <%voip_e8c_get("sip_display");%> >				
				<table border="0" cellpadding="0" cellspacing="0" width="500">
					<tr>
						<td>话机时间同步</td>
						<td><input type=checkbox name=sip_date size=20 <%voip_e8c_get("sip_date");%>>Enable</td>
					</tr>
					<tr>
						<td>订阅类型</td>
						<td><select name=SUBS_options> <%voip_e8c_get("SUBS_options");%></select></td>
					</tr>
					<tr>
						<td>订阅周期（秒）</td>
						<td><input type=text name=SUBS_time size=10 maxlength=20 value="<%voip_e8c_get("SUBS_time"); %>"></td>
					</tr>
					<tr>
						<td>反极性信号</td>
						<td><input type=checkbox name=reverse_polariy size=20 <%voip_e8c_get("reverse_polariy");%>>Enable</td>
					</tr>
					<tr>
						<td>三方通话</td>
						<td><input type=checkbox name=3_way size=20 <%voip_e8c_get("3_way");%>>Enable</td>
					</tr>
					<tr>
						<td>呼叫等待</td>
						<td><input type=checkbox name=call_waiting size=20 <%voip_e8c_get("call_waiting");%>>Enable</td>
					</tr>
					<tr>
						<td>热线呼叫</td>
						<td><select name=hot_line_options> <%voip_e8c_get("hot_line_options");%></select></td>
					</tr>
					<tr>
						<td>热线呼叫号码</td>
						<td><input type=text name=hotline_number size=10 maxlength=20 value="<%voip_e8c_get("hotline_number"); %>"></td>
					</tr>
					<tr>
						<td>启用会话更新</td>
						<td><input name="sessionEnable" type="checkbox" value="enable" <% voip_e8c_get("sessionEnable"); %>></td>
					</tr>
					<tr>
						<td>会话更新周期 (秒):</td>
						<td><input name="sessionExpiry" size="16" maxlength="32" type="text" value="<% voip_e8c_get("sessionExpiry"); %>"></td>
					</tr>
					<tr>
						<td>最小会话更新周期(秒)[最小值90]:</td>
						<td><input name="minSessionExpiry" size="16" maxlength="32" type="text" value="<% voip_e8c_get("minSessionExpiry"); %>"></td>
					</tr>
					<tr>
						<td>会话更新消息类型</td>
						<td><select name=session_options> <%voip_e8c_get("session_options");%></select></td>
					</tr>
					<tr>
						<td>支持#转换%23</td>
						<td><input type=checkbox name=pound_to_ascii size=20 <%voip_e8c_get("pound_to_ascii");%>>Enable</td>
					</tr>
					<tr>
						<td bgColor=#aaddff>数图匹配模式</td>
						<td><select name=digit_map_mode> <%voip_e8c_get("digit_map_mode");%></select></td>
					</tr>			
					<tr>
						<td bgColor=#aaddff width=155>数图</td>
					</tr>			
				<tr>
					<td bgColor=#aaddff><TEXTAREA COLS=50 ROWS=5 name="dialplan" maxlength=4096><%voip_e8c_get("dialplan");%></TEXTAREA></td>
				</tr>
				</table>
</div>				
<%voip_e8c_get("h248_display_mark_end");%>				
				<br>	



				<input type="button" class="btnsaveup" name="btnDown" value="保存/应用" onClick="on_submit()">
				<input type="hidden" name="submit-url" value="">
				<input type="hidden" name="postSecurityFlag" value="">
			</form>
		</DIV>
	</blockquote>
</body>
<%addHttpNoCache();%>
</html>
