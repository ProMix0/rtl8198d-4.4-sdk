<!DOCTYPE html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html">
<META HTTP-EQUIV="Pragma" CONTENT="no-cache">
<META HTTP-EQUIV="Cache-Control" CONTENT="no-cache">
<title>SIP</title>
<!--<link rel="stylesheet" href="/admin/style.css">
<link rel="stylesheet" href="/admin/reset.css" />
<link rel="stylesheet" href="/admin/base.css" />-->
<link rel="stylesheet" href="./admin/style.css">
<link rel="stylesheet" href="./admin/reset.css" />
<link rel="stylesheet" href="./admin/base.css" />
<script language="javascript" src=voip_script.js></script>
<style> TABLE {width:375} </style>
<SCRIPT language="javascript" src="common.js"></SCRIPT>
<script language="javascript">

function dtmfMode_change()
{
	document.sipform.dtmf_2833_pt.disabled = (document.sipform.dtmfMode.selectedIndex != 0);
	document.sipform.dtmf_2833_pi.disabled = (document.sipform.dtmfMode.selectedIndex != 0);
	document.sipform.fax_modem_2833_pt.disabled = (document.sipform.dtmfMode.selectedIndex != 0);
	document.sipform.fax_modem_2833_pi.disabled = (document.sipform.dtmfMode.selectedIndex != 0);
	document.sipform.sipInfo_duration.disabled = (document.sipform.dtmfMode.selectedIndex != 1);
}

function spd_dial_edit()
{
	for (var i=0; i<10; i++)
	{
		document.sipform.spd_sel[i].disabled = document.all.spd_url[i].value == "";
		if (document.sipform.spd_sel[i].disabled)
			document.all.spd_name[i].value = "";
	}
}

function spd_dial_remove_sel()
{
	var flag=false;

	for (var i=0; i<10; i++)
	{
		if (document.sipform.spd_sel[i].checked)
		{
			flag=true;
			break;
		}
	}

	if (!flag)
	{
		alert('You have to select first.');
		return false;
	}

	if (!confirm('Do you really want to remove the selected items?'))
	{
		return false;
	}


	for (var i=0; i<10; i++)
	{
		if (document.sipform.spd_sel[i].checked)
		{
			document.sipform.spd_sel[i].checked = false;
			document.sipform.spd_sel[i].disabled = true;
			document.all.spd_name[i].value = "";
			document.all.spd_url[i].value = "";
		}
	}
	return true;
}

function spd_dial_remove_all()
{
	if (!confirm('Do you really want to remove all items in the phone book?'))
	{
		return false;
	}

	for (var i=0; i<10; i++)
	{
		document.sipform.spd_sel[i].checked = false;
		document.sipform.spd_sel[i].disabled = true;
		document.all.spd_name[i].value = "";
		document.all.spd_url[i].value = "";
	}
	return true;
}

function t38param_click_check()
{
	document.sipform.T38MaxBuffer.disabled = !document.sipform.T38ParamEnable.checked;
	document.sipform.T38RateMgt.disabled = !document.sipform.T38ParamEnable.checked;
	document.sipform.T38MaxRate.disabled = !document.sipform.T38ParamEnable.checked;
	document.sipform.T38EnableECM.disabled = !document.sipform.T38ParamEnable.checked;
	document.sipform.T38ECCSignal.disabled = !document.sipform.T38ParamEnable.checked;
	document.sipform.T38ECCData.disabled = !document.sipform.T38ParamEnable.checked;
	document.sipform.T38EnableSpoof.disabled = !document.sipform.T38ParamEnable.checked;
	document.sipform.T38DuplicateNum.disabled = !document.sipform.T38ParamEnable.checked;
}

function enable_hotline()
{
	document.sipform.hotline_number.disabled = !document.sipform.hotline_enable.checked;
}

function enable_dnd()
{
	document.sipform.dnd_from_hour.disabled = !document.sipform.dnd_mode[1].checked;
	document.sipform.dnd_from_min.disabled = !document.sipform.dnd_mode[1].checked;
	document.sipform.dnd_to_hour.disabled = !document.sipform.dnd_mode[1].checked;
	document.sipform.dnd_to_min.disabled = !document.sipform.dnd_mode[1].checked;
}

function vad_enable()
{
	document.sipform.sid_mode.disabled = !document.sipform.useVad.checked;
	document.sipform.sid_noiselevel.disabled = (!document.sipform.sid_mode[1].checked) || (!document.sipform.useVad.checked) ;
	document.sipform.sid_noisegain.disabled = (!document.sipform.sid_mode[2].checked) || (!document.sipform.useVad.checked) ;
}

function rtcp_click_check()
{
	document.sipform.RTCPInterval.disabled = !document.sipform.useRTCP.checked;
	document.sipform.useRTCPXR.disabled = !document.sipform.useRTCP.checked;
}
</script>
</head>

<body>
<form method="post" action="/boaform/voip_general_set" name=sipform>
<input type=hidden name=voipPort value="<%voip_general_get("voip_port");%>">

<%voip_general_get("proxy");%>

<!--
<p>
<b>NAT Traversal</b>
<table cellSpacing=1 cellPadding=2 border=0>
<%voip_general_get("stun");%>
</table>
-->

<div class="column">
	<div class="column_title">
		<div class="column_title_left"></div>
		<p><% multilang(LANG_SIP_ADVANCED); %></p>
		<div class="column_title_right"></div>
	</div>	
	<div class="data_common">				
		<table>
		<tr>
			<td bgColor=#aaddff><% multilang(LANG_SIP_PORT); %></td>
			<td bgColor=#ddeeff>
			<input type=text name=sipPort size=10 maxlength=5 value="<%voip_general_get("sipPort"); %>">
			<%voip_general_get("sipPorts");%>
			</td>
		</tr>
		<tr>
			<td bgColor=#aaddff><% multilang(LANG_MEDIA_PORT); %></td>
			<td bgColor=#ddeeff>
			<input type=text name=rtpPort size=10 maxlength=5 value="<%voip_general_get("rtpPort"); %>">
			<%voip_general_get("rtpPorts");%>
			</td>
		</tr>
		<tr>
			<td bgColor=#aaddff><% multilang(LANG_DMTF_RELAY); %></td>
			<td bgColor=#ddeeff>
				<select name=dtmfMode onchange="dtmfMode_change()">
					"<%voip_general_get("dtmfMode");%>"
				</select>
			</td>
		</tr>
		<tr>
			<td bgColor=#aaddff><% multilang(LANG_DTMF_RFC2833_PAYLOAD_TYPE); %></td>
			<td bgColor=#ddeeff>
				<input type=text name=dtmf_2833_pt size=12 maxlength=3 value=<%voip_general_get("dtmf_2833_pt");%>>
			</td>
		</tr>
		<tr>
			<td bgColor=#aaddff><% multilang(LANG_DTMF_RFC2833_PACKET_INTERVAL); %></td>
			<td bgColor=#ddeeff>
				<input type=text name=dtmf_2833_pi size=12 maxlength=3 value=<%voip_general_get("dtmf_2833_pi");%>>(<% multilang(LANG_MSEC); %>) (<% multilang(LANG_MUST_BE_MULTIPLE_OF_10MSEC); %>)
			</td>
		</tr>
		<tr>
			<td bgColor=#aaddff><% multilang(LANG_USE_DTMF_RFC2833_PT_AS_FAX_MODEM_RFC2833_PT); %></td>
			<td bgColor=#ddeeff><input type=checkbox name=fax_modem_2833_pt_same_dtmf <%voip_general_get("fax_modem_2833_pt_same_dtmf");%>><% multilang(LANG_ENABLE); %></td>
		</tr>
		<tr>
			<td bgColor=#aaddff><% multilang(LANG_FAX_MODEM_RFC2833_PAYLOAD_TYPE); %></td>
			<td bgColor=#ddeeff>
				<input type=text name=fax_modem_2833_pt size=12 maxlength=3 value=<%voip_general_get("fax_modem_2833_pt");%>>
			</td>
		</tr>
		<tr>
			<td bgColor=#aaddff><% multilang(LANG_FAX_MODEM_RFC2833_PACKET_INTERVAL); %></td>
			<td bgColor=#ddeeff>
				<input type=text name=fax_modem_2833_pi size=12 maxlength=3 value=<%voip_general_get("fax_modem_2833_pi");%>>(<% multilang(LANG_MSEC); %>) (<% multilang(LANG_MUST_BE_MULTIPLE_OF_10MSEC); %>)
			</td>
		</tr>
		<tr>
			<td bgColor=#aaddff><% multilang(LANG_SIP_INFO_DURATION_MS); %></td>
			<td bgColor=#ddeeff>
				<input type=text name=sipInfo_duration size=12 maxlength=4 value=<%voip_general_get("sipInfo_duration");%>>
			</td>
		</tr>
		<tr>
			<td bgColor=#aaddff><% multilang(LANG_CALL_WAITING); %></td>
			<td bgColor=#ddeeff><input type=checkbox name=call_waiting onclick="enable_callwaiting();" <%voip_general_get("call_waiting");%>><% multilang(LANG_ENABLE); %></td>
		</tr>
		<tr>
			<td bgColor=#aaddff><% multilang(LANG_CALL_WAITING_CALLER_ID); %></td>
			<td bgColor=#ddeeff><input type=checkbox name=call_waiting_cid <%voip_general_get("call_waiting_cid");%>><% multilang(LANG_ENABLE); %></td>
		</tr>
		<tr>
			<td bgColor=#aaddff><% multilang(LANG_REJECT_DIRECT_IP_CALL); %></td>
			<td bgColor=#ddeeff><input type=checkbox name=reject_direct_ip_call <%voip_general_get("reject_direct_ip_call");%>><% multilang(LANG_ENABLE); %></td>
		</tr>
		   <tr>
					<td bgColor=#aaddff><% multilang(LANG_SEND_CALLER_ID_HIDDEN); %></td>
					<td bgColor=#ddeeff><input type=checkbox name=caller_id_hidden <%voip_general_get("caller_id_hidden");%>><% multilang(LANG_ENABLE); %></td>
			</tr>
		 <tr>
					<td bgColor=#aaddff>call transfer</td>
					<td bgColor=#ddeeff><input type=checkbox name=Call_Transfer <%voip_general_get("Call_Transfer");%>><% multilang(LANG_ENABLE); %></td>
			</tr>
			 <tr>
				<td bgColor=#aaddff>3 way conference</td>
				<td bgColor=#ddeeff><input type=checkbox name=3PTY_Conference <%voip_general_get("3PTY_Conference");%>><% multilang(LANG_ENABLE); %></td>
			</tr>
			 <tr>
					<td bgColor=#aaddff>conference on server/CPE</td>
					<td bgColor=#ddeeff><%voip_general_get("Conference_server_cpe");%></td>
			</tr>
			  <tr>
				<td bgColor=#aaddff >conference-uri</td>
				<td bgColor=#ddeeff >
			<input type="text" name="conference_uri" size=20 maxlength=39 value="<%voip_general_get("conference_uri");%>"></td>
		</tr>
	</table>
	</div>
</div> 


<div class="column">
	<div class="column_title">
		<div class="column_title_left"></div>
		<p><% multilang(LANG_FORWARD_MODE); %></p>
		<div class="column_title_right"></div>
	</div>	
	<div class="data_common">				
		<table>
			<tr>
			<td bgColor=#aaddff width=155><% multilang(LANG_IMMEDIATE_FORWARD_TO); %></td>
			<td bgColor=#ddeeff width=170>
			<%voip_general_get("CFAll");%>
			</td>
			</tr>
			<tr>
			<td bgColor=#aaddff><% multilang(LANG_IMMEDIATE_NUMBER); %></td>
			<td bgColor=#ddeeff>
			<%voip_general_get("CFAll_No");%>
			</td>
			</tr>
			<tr>
			<td bgColor=#aaddff><% multilang(LANG_BUSY_FORWARD_TO); %></td>
			<td bgColor=#ddeeff>
			<%voip_general_get("CFBusy");%>
			</td>
			</tr>
			<tr>
			<td bgColor=#aaddff><% multilang(LANG_BUSY_NUMBER); %></td>
			<td bgColor=#ddeeff>
			<%voip_general_get("CFBusy_No");%>
			</td>
			</tr>
			<%voip_general_get("CFNoAns");%>
		</table>
	</div>
</div> 
<!--
<div class="column">
	<div class="column_title">
		<div class="column_title_left"></div>
		<p><%voip_general_get("speed_dial_display_title");%></p>
		<div class="column_title_right"></div>
	</div>	
	<div class="data_common">				
		<table <%voip_general_get("speed_dial_display");%>>
			<tr align=center>
				<td bgcolor=#aaddff><% multilang(LANG_POSITION); %></td>
				<td bgcolor=#aaddff><% multilang(LANG_NAME_1); %></td>
				<td bgcolor=#aaddff><% multilang(LANG_PHONE_NUMBER); %></td>
				<td bgcolor=#aaddff><% multilang(LANG_SELECT); %></td>
			</tr>
			<%voip_general_get("speed_dial");%>
			<tr align=center>
				<td colspan=4 bgcolor=#ddeeff>
				<input type=button value="<% multilang(LANG_REMOVE_SELECTED); %>" name="RemoveSelected" onClick="spd_dial_remove_sel()">
				<input type=button value="<% multilang(LANG_REMOVE_ALL); %>" name="RemoveAll" onClick="spd_dial_remove_all()">
				</td>
			</tr>
		</table>
	</div>
</div> 

<%voip_general_get("not_ipphone_option_start");%>

<div class="column">
	<div class="column_title">
		<div class="column_title_left"></div>
		<p><% multilang(LANG_ABBREVIATED_DIAL); %></p>
		<div class="column_title_right"></div>
	</div>	
	<div class="data_common">	
<%voip_general_get("not_ipphone_option_end");%>	
	<table cellSpacing=1 cellPadding=2 border=0 <%voip_general_get("not_ipphone_table");%> >
		<tr align=center>
			<td bgcolor=#aaddff><% multilang(LANG_ABBREVIATED_NAME); %></td>
			<td bgcolor=#aaddff><% multilang(LANG_PHONE_NUMBER); %></td>
		</tr>
		<%voip_general_get("abbreviated_dial");%>
	</table>
	</div>
</div> 
-->
<div class="column">
	<div class="column_title">
		<div class="column_title_left"></div>
		<p><%voip_general_get("display_dialplan_title");%></p>
		<div class="column_title_right"></div>
	</div>	
	<div class="data_common">				
		<table <%voip_general_get("display_dialplan");%>>
		<!--
			<tr>
				<td bgColor=#aaddff width=155>Replace prefix code</td>
				<td bgColor=#ddeeff width=170>
				<%voip_general_get("ReplaceRuleOption");%>
				</td>
			</tr>
			<tr>
				<td bgColor=#aaddff width=155>Relace rule</td>
				<td bgColor=#ddeeff width=170>
				<input type="text" name="ReplaceRuleSource" size=12 maxlength=79 value="<%voip_general_get("ReplaceRuleSource");%>"> ->
				<input type="text" name="ReplaceRuleTarget" size=3 maxlength=9 value="<%voip_general_get("ReplaceRuleTarget");%>"></td>
			</tr>
		-->	
			<tr>
				<td bgColor=#aaddff width=155><% multilang(LANG_ENABLE_DIALPLAN); %></td>
				<td bgColor=#ddeeff width=170>
				<%voip_general_get("digitmap_enable");%>
				</td>
			</tr>
			
			<tr>
				<td bgColor=#aaddff width=155><% multilang(LANG_DIAL_PLAN); %></td>
				<td bgColor=#ddeeff width=170>
				<input type="text" name="dialplan" size=20 maxlength=1024 value="<%voip_general_get("dialplan");%>"></td>
			</tr>
		<!--
			<tr>
				<td bgColor=#aaddff width=155>Auto Prefix</td>
				<td bgColor=#ddeeff width=170>
				<input type="text" name="AutoPrefix" size=5 maxlength=4 value="<%voip_general_get("AutoPrefix");%>">
			</tr>
			<tr>
				<td bgColor=#aaddff width=155>Prefix Unset Plan</td>
				<td bgColor=#ddeeff width=170>
				<input type="text" name="PrefixUnsetPlan" size=20 maxlength=79 value="<%voip_general_get("PrefixUnsetPlan");%>">
			</tr>
		-->	
		</table>
	</div>
</div> 
<div class="column">
	<div class="column_title">
		<div class="column_title_left"></div>
		<p><% multilang(LANG_CODEC); %></p>
		<div class="column_title_right"></div>
	</div>	
	<div class="data_common">				
		<!-- RTP Redundant  -->
		<table  cellSpacing=1 cellPadding=1 border=0 <%voip_general_get("RTP_RED_BUILD");%>>
			<tr>
				<td bgColor=#aaddff rowspan=2><% multilang(LANG_RTP_REDUNDANT); %><br>(<% multilang(LANG_FIRST_PRECEDENCE); %>)</td>
				<td bgColor=#ddeeff><% multilang(LANG_CODEC); %></td>
				<td bgColor=#ddeeff>
					<select name=rtp_redundant_codec>
						<%voip_general_get("rtp_redundant_codec_options");%>
					</select>
				</td>
			</tr>
			<tr>
				<td bgColor=#ddeeff><% multilang(LANG_PAYLOAD_TYPE); %></td>
				<td bgColor=#ddeeff><input type=text name=rtp_redundant_payload_type size=10 value="<%voip_general_get("rtp_redundant_payload_type");%>"></td>
			</tr>
		</table>
	</div>
</div> 

<div class="data_common data_common_notitle">
	<%voip_general_get("codec_var");%>
</div>		

<div class="data_common data_common_notitle">
	<table cellSpacing=1 cellPadding=2 border=0>
	<%voip_general_get("codec");%>
	</table>
</div>		
		
<div class="data_common data_common_notitle">
	<%voip_general_get("codec_opt");%>
</div>	

<div class="column">
	<div class="column_title">
		<div class="column_title_left"></div>
		<p><%voip_general_get("hotline_option_display_title");%></p>
		<div class="column_title_right"></div>
	</div>	
	<div class="data_common">				
		<!-- Hot Line -->
		<table cellSpacing=1 cellPadding=2 border=0 width=450 <%voip_general_get("hotline_option_display");%>>
			<tr>
				<td bgColor=#aaddff width=150><% multilang(LANG_USE_HOT_LINE); %></td>
				<td bgColor=#ddeeff>
				<input type=checkbox name=hotline_enable onClick="enable_hotline()" <%voip_general_get("hotline_enable");%>><% multilang(LANG_ENABLE); %>
				</td>
			</tr>
				<tr>
				<td bgColor=#aaddff width=150><% multilang(LANG_HOT_LINE_NUMBER); %></td>
				<td bgColor=#ddeeff>
					<input type=text name=hotline_number size=20 maxlength=39 value="<%voip_general_get("hotline_number");%>">
				</td>
			</tr>
		</table>
		<script language=javascript>enable_hotline()</script>
	</div>
</div> 

<div class="column">
	<div class="column_title">
		<div class="column_title_left"></div>
		<p>DND (Don't Disturb)</p>
		<div class="column_title_right"></div>
	</div>	
	<div class="data_common">				
		<!-- DND (Don't Disturb) -->
		<table cellSpacing=1 cellPadding=2 border=0 width=450>
		<tr>
			<td bgColor=#aaddff width=150>DND <% multilang(LANG_MODE); %></td>
			<td bgColor=#ddeeff>
				<input type=radio name=dnd_mode value=2 onClick="enable_dnd()" <%voip_general_get("dnd_always");%>><% multilang(LANG_ALWAYS); %>
				<input type=radio name=dnd_mode value=1 onClick="enable_dnd()" <%voip_general_get("dnd_enable");%>><% multilang(LANG_ENABLE); %>
				<input type=radio name=dnd_mode value=0 onClick="enable_dnd()" <%voip_general_get("dnd_disable");%>><% multilang(LANG_DISABLE); %>
			</td>
		</tr>
		<tr>
			<td bgColor=#aaddff width=150><% multilang(LANG_FROM); %></td>
			<td bgColor=#ddeeff>
				<input type=text name=dnd_from_hour size=3 maxlength=2 value="<%voip_general_get("dnd_from_hour");%>">:
				<input type=text name=dnd_from_min size=3 maxlength=2 value="<%voip_general_get("dnd_from_min");%>">
				(hh:mm)
			</td>
		</tr>
		<tr>
			<td bgColor=#aaddff width=150> <% multilang(LANG_TO); %></td>
			<td bgColor=#ddeeff>
				<input type=text name=dnd_to_hour size=3 maxlength=2 value="<%voip_general_get("dnd_to_hour");%>">:
				<input type=text name=dnd_to_min size=3 maxlength=2 value="<%voip_general_get("dnd_to_min");%>">
				(hh:mm)
			</td>
		</tr>
		</table>
		<script language=javascript>enable_dnd()</script>
	</div>
</div> 




<!--
<%voip_general_get("not_ipphone_option_start");%>
<p>
<b><% multilang(LANG_AUTHENTICATION); %></b>
<%voip_general_get("not_ipphone_option_end");%>
<table cellSpacing=1 cellPadding=2 border=0 width=450 <%voip_general_get("not_ipphone_table");%> >
<tr>
   	<td bgColor=#aaddff width=150><% multilang(LANG_OFF_HOOK_PASSWORD); %></td>
	<td bgColor=#ddeeff>
		<input type=text name=offhook_passwd size=20 maxlength=9 value="<%voip_general_get("offhook_passwd");%>">
	</td>
</tr>
</table>
-->

<!-- Alarm -->
<div class="column">
<%voip_general_get("not_ipphone_option_start");%>
<%voip_general_get("not_dect_port_option");%>
	<div class="column_title">
		<div class="column_title_left"></div>
		<p><% multilang(LANG_ALARM); %></p>
		<div class="column_title_right"></div>
	</div>	
	<div class="data_common">
<%voip_general_get("not_ipphone_option_end");%>	
		<table cellSpacing=1 cellPadding=2 border=0 width=450 <%voip_general_get("not_ipphone_table");%> >
			<tr>
				<td bgColor=#aaddff width=150><% multilang(LANG_ENABLE); %></td>
				<td bgColor=#ddeeff>
					<input type=checkbox name=alarm_enable <%voip_general_get("alarm_enable");%> <%voip_general_get("alarm_disabled");%>>
				</td>
			</tr>
			<tr>
				<td bgColor=#aaddff width=150><% multilang(LANG_TIME); %></td>
				<td bgColor=#ddeeff>
					<input type=text name=alarm_hh size=3 maxlength=2 value="<%voip_general_get("alarm_hh");%>" <%voip_general_get("alarm_disabled");%>>:
					<input type=text name=alarm_mm size=3 maxlength=2 value="<%voip_general_get("alarm_mm");%>" <%voip_general_get("alarm_disabled");%>> (hh:mm)
				</td>
			</tr>
		</table>
	</div>
</div> 
<!-- 
<p>
<b>DSP</b>
<table cellSpacing=1 cellPadding=2 border=0>
  	<tr>
    	<td bgColor=#aaddff width=155 rowspan=2>FXS Volume</td>
		<td bgColor=#ddeeff width=170>Handset Gain</td>
		<td bgColor=#ddeeff>
			<select name=slic_txVolumne>
				  "<%voip_general_get("slic_txVolumne");%>"
			</select>
		</td>
	</tr>
	<tr>
		<td bgColor=#ddeeff width=170>Handset Volume</td>
		<td bgColor=#ddeeff>
			<select name=slic_rxVolumne>
				 "<%voip_general_get("slic_rxVolumne");%>"
			</select>
		</td>
	</tr>
-->
<!--
  	<tr>
    	<td bgColor=#aaddff>LEC Tail Length (ms)</td>
		<td bgColor=#ddeeff>
		<select name=echoTail>
			 "<%voip_general_get("echoTail");%>"
		</select>
		</td>
		<td bgColor=#ddeeff></td>
	</tr>
-->
<!--
  	<tr>
    	<td bgColor=#aaddff width=155>ANSTONE-CNG</td>
		<td bgColor=#ddeeff width=140>
			<input type=checkbox name=useANSTONE_CNG_TDM size=20 <%voip_general_get("useANSTONE_CNG_TDM");%>><% multilang(LANG_ENABLE); %> TDM
		</td>
		<td bgColor=#ddeeff width=140>
			<input type=checkbox name=useANSTONE_CNG_IP size=20 <%voip_general_get("useANSTONE_CNG_IP");%>><% multilang(LANG_ENABLE); %> IP
		</td>
	</tr>
  	<tr>
    	<td bgColor=#aaddff width=155>ANSTONE-ANS</td>
		<td bgColor=#ddeeff width=140>
			<input type=checkbox name=useANSTONE_ANS_TDM size=20 <%voip_general_get("useANSTONE_ANS_TDM");%>><% multilang(LANG_ENABLE); %> TDM
		</td>
		<td bgColor=#ddeeff width=140>
			<input type=checkbox name=useANSTONE_ANS_IP size=20 <%voip_general_get("useANSTONE_ANS_IP");%>><% multilang(LANG_ENABLE); %> IP
		</td>
	</tr>
  	<tr>
    	<td bgColor=#aaddff width=155>ANSTONE-ANSAM</td>
		<td bgColor=#ddeeff width=140>
			<input type=checkbox name=useANSTONE_ANSAM_TDM size=20 <%voip_general_get("useANSTONE_ANSAM_TDM");%>><% multilang(LANG_ENABLE); %> TDM
		</td>
		<td bgColor=#ddeeff>
			<input type=checkbox name=useANSTONE_ANSAM_IP size=20 <%voip_general_get("useANSTONE_ANSAM_IP");%>><% multilang(LANG_ENABLE); %> IP
		</td>
	</tr>

  	<tr>
    	<td bgColor=#aaddff width=155>ANSTONE-ANSBAR</td>
		<td bgColor=#ddeeff width=140>
			<input type=checkbox name=useANSTONE_ANSBAR_TDM size=20 <%voip_general_get("useANSTONE_ANSBAR_TDM");%>><% multilang(LANG_ENABLE); %> TDM
		</td>
		<td bgColor=#ddeeff>
			<input type=checkbox name=useANSTONE_ANSBAR_IP size=20 <%voip_general_get("useANSTONE_ANSBAR_IP");%>><% multilang(LANG_ENABLE); %> IP
		</td>
	</tr>
  	<tr>
    	<td bgColor=#aaddff width=155>ANSTONE-ANSAMBAR</td>
		<td bgColor=#ddeeff width=140>
			<input type=checkbox name=useANSTONE_ANSAMBAR_TDM size=20 <%voip_general_get("useANSTONE_ANSAMBAR_TDM");%>><% multilang(LANG_ENABLE); %> TDM
		</td>
		<td bgColor=#ddeeff>
			<input type=checkbox name=useANSTONE_ANSAMBAR_IP size=20 <%voip_general_get("useANSTONE_ANSAMBAR_IP");%>><% multilang(LANG_ENABLE); %> IP
		</td>
	</tr>

  	<tr>
    	<td bgColor=#aaddff width=155>ANSTONE-BELLANS</td>
		<td bgColor=#ddeeff width=140>
			<input type=checkbox name=useANSTONE_BELLANS_TDM size=20 <%voip_general_get("useANSTONE_BELLANS_TDM");%>><% multilang(LANG_ENABLE); %> TDM
		</td>
		<td bgColor=#ddeeff>
			<input type=checkbox name=useANSTONE_BELLANS_IP size=20 <%voip_general_get("useANSTONE_BELLANS_IP");%>><% multilang(LANG_ENABLE); %> IP
		</td>
	</tr>
  	<tr>
    	<td bgColor=#aaddff width=155>ANSTONE-V.22ANS</td>
		<td bgColor=#ddeeff width=140>
			<input type=checkbox name=useANSTONE_V22ANS_TDM size=20 <%voip_general_get("useANSTONE_V22ANS_TDM");%>><% multilang(LANG_ENABLE); %> TDM
		</td>
		<td bgColor=#ddeeff>
			<input type=checkbox name=useANSTONE_V22ANS_IP size=20 <%voip_general_get("useANSTONE_V22ANS_IP");%>><% multilang(LANG_ENABLE); %> IP
		</td>
	</tr>
  	<tr>
    	<td bgColor=#aaddff width=155>ANSTONE-V8bis_Cre</td>
		<td bgColor=#ddeeff width=140>
			<input type=checkbox name=useANSTONE_V8bis_Cre_TDM size=20 <%voip_general_get("useANSTONE_V8bis_Cre_TDM");%>><% multilang(LANG_ENABLE); %> TDM
		</td>
		<td bgColor=#ddeeff>
			<input type=checkbox name=useANSTONE_V8bis_Cre_IP size=20 <%voip_general_get("useANSTONE_V8bis_Cre_IP");%>><% multilang(LANG_ENABLE); %> IP
		</td>
	</tr>
  	<tr>
    	<td bgColor=#aaddff width=155>ANSTONE-V21flag</td>
		<td bgColor=#ddeeff width=140>
			<input type=checkbox name=useANSTONE_V21flag_TDM size=20 <%voip_general_get("useANSTONE_V21flag_TDM");%>><% multilang(LANG_ENABLE); %> TDM
		</td>
		<td bgColor=#ddeeff>
			<input type=checkbox name=useANSTONE_V21flag_IP size=20 <%voip_general_get("useANSTONE_V21flag_IP");%>><% multilang(LANG_ENABLE); %> IP
		</td>
	</tr>
  	<tr>
    	<td bgColor=#aaddff width=155>ANSTONE-V21DIS</td>
		<td bgColor=#ddeeff width=140>
			<input type=checkbox name=useANSTONE_V21DIS_TDM size=20 <%voip_general_get("useANSTONE_V21DIS_TDM");%>><% multilang(LANG_ENABLE); %> TDM
		</td>
		<td bgColor=#ddeeff>
			<input type=checkbox name=useANSTONE_V21DIS_IP size=20 <%voip_general_get("useANSTONE_V21DIS_IP");%>><% multilang(LANG_ENABLE); %> IP
		</td>
	</tr>
  	<tr>
    	<td bgColor=#aaddff width=155>ANSTONE-V21DCN</td>
		<td bgColor=#ddeeff width=140>
			<input type=checkbox name=useANSTONE_V21DCN_TDM size=20 <%voip_general_get("useANSTONE_V21DCN_TDM");%>><% multilang(LANG_ENABLE); %> TDM
		</td>
		<td bgColor=#ddeeff>
			<input type=checkbox name=useANSTONE_V21DCN_IP size=20 <%voip_general_get("useANSTONE_V21DCN_IP");%>><% multilang(LANG_ENABLE); %> IP
		</td>
	</tr>
  	<tr>
    	<td bgColor=#aaddff width=155>ANSTONE-BELL202_CP</td>
		<td bgColor=#ddeeff width=140>
			<input type=checkbox name=useANSTONE_BELL202_CP_TDM size=20 <%voip_general_get("useANSTONE_BELL202_CP_TDM");%>><% multilang(LANG_ENABLE); %> TDM
		</td>
		<td bgColor=#ddeeff>
			<input type=checkbox name=useANSTONE_BELL202_CP_IP size=20 <%voip_general_get("useANSTONE_BELL202_CP_IP");%>><% multilang(LANG_ENABLE); %> IP
		</td>
	</tr>
  	<tr>
    	<td bgColor=#aaddff width=155>ANSTONE-BELL202_AP</td>
		<td bgColor=#ddeeff width=140>
			<input type=checkbox name=useANSTONE_BELL202_AP_TDM size=20 <%voip_general_get("useANSTONE_BELL202_AP_TDM");%>><% multilang(LANG_ENABLE); %> TDM
		</td>
		<td bgColor=#ddeeff>
			<input type=checkbox name=useANSTONE_BELL202_AP_IP size=20 <%voip_general_get("useANSTONE_BELL202_AP_IP");%>><% multilang(LANG_ENABLE); %> IP
		</td>
	</tr>
  	<tr>
    	<td bgColor=#aaddff width=155>ANSTONE-V21CH1</td>
		<td bgColor=#ddeeff width=140>
			<input type=checkbox name=useANSTONE_V21CH1_TDM size=20 <%voip_general_get("useANSTONE_V21CH1_TDM");%>><% multilang(LANG_ENABLE); %> TDM
		</td>
		<td bgColor=#ddeeff>
			<input type=checkbox name=useANSTONE_V21CH1_IP size=20 <%voip_general_get("useANSTONE_V21CH1_IP");%>><% multilang(LANG_ENABLE); %> IP
		</td>
	</tr>
  	<tr>
    	<td bgColor=#aaddff width=155>ANSTONE-V21CH2</td>
		<td bgColor=#ddeeff width=140>
			<input type=checkbox name=useANSTONE_V21CH2_TDM size=20 <%voip_general_get("useANSTONE_V21CH2_TDM");%>><% multilang(LANG_ENABLE); %> TDM
		</td>
		<td bgColor=#ddeeff>
			<input type=checkbox name=useANSTONE_V21CH2_IP size=20 <%voip_general_get("useANSTONE_V21CH2_IP");%>><% multilang(LANG_ENABLE); %> IP
		</td>
	</tr>
  	<tr>
    	<td bgColor=#aaddff width=155>ANSTONE-V23</td>
		<td bgColor=#ddeeff width=140>
			<input type=checkbox name=useANSTONE_V23_TDM size=20 <%voip_general_get("useANSTONE_V23_TDM");%>><% multilang(LANG_ENABLE); %> TDM
		</td>
		<td bgColor=#ddeeff>
			<input type=checkbox name=useANSTONE_V23_IP size=20 <%voip_general_get("useANSTONE_V23_IP");%>><% multilang(LANG_ENABLE); %> IP
		</td>
	</tr>
-->
<div style="padding:10px 0;">
	<input class="link_bg" type="button" value="<% multilang(LANG_APPLY); %>" onclick="changeStartEnd();">
	&nbsp;&nbsp;&nbsp;&nbsp;
	<input class="link_bg" type="reset" value="<% multilang(LANG_RESET); %>">
</div>
	<input type="hidden" name="postSecurityFlag" value="">
<script language=javascript>init();</script>

</form>

</body>
</html>
