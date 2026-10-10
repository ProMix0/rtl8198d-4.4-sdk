<!DOCTYPE html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html">
<META HTTP-EQUIV="Pragma" CONTENT="no-cache">
<META HTTP-EQUIV="Cache-Control" CONTENT="no-cache">
<title>SIP</title>
<link rel="stylesheet" href="./admin/style.css">
<link rel="stylesheet" href="./admin/reset.css" />
<link rel="stylesheet" href="./admin/base.css" />
<script language="javascript" src=voip_script.js></script>
<SCRIPT language="javascript" src="common.js"></SCRIPT>
<script language="javascript">
<!--
function enableAutoBypass()
{
	document.other_form.auto_bypass_warning.disabled =
		!document.other_form.auto_bypass_relay.checked;
}
function InitOther()
{
	enableAutoBypass();
}
-->
</script>
</head>
<body bgcolor="#ffffff" text="#000000" onload="InitOther()">
<form method="post" action="/boaform/voip_other_set" name=other_form>
<!--
<%voip_general_get("not_ipphone_option_start");%>
<b><% multilang(LANG_FUNCTION_KEY); %></b>
<%voip_general_get("not_ipphone_option_end");%>
<table cellSpacing=1 cellPadding=2 border=0 width=450 <%voip_general_get("not_ipphone_table");%> >
<tr bgColor=#888888>
	<td colspan=2>
	<font color=#ffffff>
	<% multilang(LANG_MUST_BE); %> * + 0~9
	</font>
	</td>
</tr>	
<tr <%voip_other_get("display_funckey_pstn");%>>
   	<td bgColor=#aaddff width=150><% multilang(LANG_SWITCH_TO_PSTN); %></td>
	<td bgColor=#ddeeff>
		<input type=text name=funckey_pstn size=5 maxlength=2 value="<%voip_other_get("funckey_pstn");%>">
		( default: *0 )
	</td>
</tr>
<tr>
   	<td bgColor=#aaddff><% multilang(LANG_CALL_TRANSFER); %></td>
	<td bgColor=#ddeeff>
		<input type=text name=funckey_transfer size=5 maxlength=2 value="<%voip_other_get("funckey_transfer");%>">
		( <% multilang(LANG_DEFAULT); %>: *1 )
	</td>
</tr>	

</table>-->

<div class="column">
	<div class="column_title">
		<div class="column_title_left"></div>
		<p><%voip_other_get("auto_dial_display_title");%></p>
		<div class="column_title_right"></div>
	</div>	
	<div class="data_common">				
		<table <%voip_other_get("auto_dial_display");%>>
				<tr>
				<td bgColor=#aaddff width=150><% multilang(LANG_AUTO_DIAL_TIME); %></td>
				<td bgColor=#ddeeff>
					<input type=text name=auto_dial size=3 maxlength=1 value="<%voip_other_get("auto_dial");%>">
					( 3~9 <% multilang(LANG_SEC); %>, <% multilang(LANG_0_IS_DISABLE); %> )
				</td>
			</tr>
			<tr>
				<td bgColor=#aaddff width=150><% multilang(LANG_DIAL_OUT_BY_HASH_KEY); %></td>
				<td bgColor=#ddeeff>
					<input type=checkbox name=auto_dial_always <%voip_other_get("auto_dial_always");%>><% multilang(LANG_ENABLED); %>
				</td>
			</tr>
		</table>
	</div>
</div>

<div class="column" <%voip_other_get("display_funckey_pstn");%>>
	<div class="column_title">
		<div class="column_title_left"></div>
		<p><% multilang(LANG_PSTN_RELAY); %></p>
		<div class="column_title_right"></div>
	</div>	
	<div class="data_common">					
		<table>
			<tr>
				<td bgColor=#aaddff width=150><% multilang(LANG_AUTO_BYPASS_RELAY); %></td>
				<td bgColor=#ddeeff>
					<input type=checkbox name=auto_bypass_relay onclick=enableAutoBypass() <%voip_other_get("auto_bypass_relay");%>><% multilang(LANG_ENABLE);%>
				</td>
			</tr>
			<tr>
				<td bgColor=#aaddff width=150><% multilang(LANG_WARNING_TONE);%></td>
				<td bgColor=#ddeeff>
					<input type=checkbox name=auto_bypass_warning <%voip_other_get("auto_bypass_warning");%>><% multilang(LANG_ENABLE);%>
				</td>
			</tr>
		</table>
	</div>
</div> 

<div class="column">
	<div class="column_title">
		<div class="column_title_left"></div>
		<p><%voip_other_get("off_hook_alarm_display_title");%></p>
		<div class="column_title_right"></div>
	</div>	
	<div class="data_common">				
		<table <%voip_other_get("off_hook_alarm_display");%>>
		<tr>
			<td bgColor=#aaddff width=150><% multilang(LANG_OFF_HOOK_ALARM_TIME);%></td>
			<td bgColor=#ddeeff>
				<input type=text name=off_hook_alarm size=3 maxlength=2 value="<%voip_other_get("off_hook_alarm");%>">
				( 10~60 <% multilang(LANG_SEC);%>, <% multilang(LANG_0_IS_DISABLE);%> )
			</td>
		</tr>
		</table>
	</div>
</div> 

<!--
<div class="column" <%voip_other_get("display_cid_det");%>>
	<div class="column_title">
		<div class="column_title_left"></div>
		<p><% multilang(LANG_VOIP_TO_PSTN);%></p>
		<div class="column_title_right"></div>
	</div>	
	<div class="data_common">				
		<table>
		<tr bgColor=#888888>
			<td bgColor=#aaddff width=150><% multilang(LANG_ONE_STAGE_DIALING);%></td>
			<td bgColor=#ddeeff>
				<input type=checkbox name=one_stage_dial <%voip_other_get("one_stage_dial");%>>Enable
			</td>
		</tr>
		<tr bgColor=#888888>
			<td bgColor=#aaddff width=150><% multilang(LANG_TWO_STAGE_DIALING);%></td>
			<td bgColor=#ddeeff>
				<input type=checkbox name=two_stage_dial <%voip_other_get("two_stage_dial");%>>Enable
			</td>
		</tr>	
		</table>
	</div>
</div> 

<div class="column" <%voip_other_get("display_cid_det");%>>
	<div class="column_title">
		<div class="column_title_left"></div>
		<p><% multilang(LANG_PSTN_TO_VOIP);%></p>
		<div class="column_title_right"></div>
	</div>	
	<div class="data_common">				
		<table>
			<tr bgColor=#888888>
				<td bgColor=#aaddff width=150><% multilang(LANG_CALLER_ID_AUTO_DETECTION);%></td>
				<td bgColor=#ddeeff><%voip_other_get("caller_id_auto_det");%></td>
				<input type=hidden name=caller_id_test value=hello>
			</tr>	
			<tr bgColor=#888888>
				<td bgColor=#aaddff width=150><% multilang(LANG_CALLER_ID_DETECTION_MODE);%></td>
				<td bgColor=#ddeeff><%voip_other_get("caller_id_det");%></td>
			</tr>	
		</table>
	</div>
</div> 
-->

<%voip_general_get("not_ipphone_option_start");%>
<div class="column">
	<div class="column_title">
		<div class="column_title_left"></div>
		<p><% multilang(LANG_FXS_PULSE_DIAL_DETECTION);%></p>
		<div class="column_title_right"></div>
	</div>	
	<%voip_general_get("not_ipphone_option_end");%>
	<div class="data_common">				
		<table <%voip_general_get("not_ipphone_table");%> >
			<tr>
				<td bgColor=#aaddff> 
					<%voip_other_get("pulse_dial_detection");%>
				</td>	
			</tr>

			<tr>
				<td bgColor=#aaddff width=150><% multilang(LANG_INTERDIGIT_PAUSE_DURATION);%></td>
				<td bgColor=#ddeeff>
					<input type=text name=pulse_det_Pause size=5 maxlength=4 value="<%voip_other_get("pulse_det_Pause");%>">
					(<% multilang(LANG_MSEC);%>)
				</td>	
			</tr>
		</table>
	</div>
</div>
 
<!--
<div class="column" <%voip_other_get("display_pulse_dial_gen");%>>
	<div class="column_title">
		<div class="column_title_left"></div>
		<p>3G Configuration</p>
		<div class="column_title_right"></div>
	</div>	
	<div class="data_common">				
		<table width=600 border=0>
		  <tr bgcolor="#808080">
			<td width="15%" align=center><font size=2><b>Interface</b></font></td>
			<td width="15%" align=center><font size=2><b>Protocol</b></font></td>
			<td width="25%" align=center><font size=2><b>IP Address</b></font></td>
			<td width="25%" align=center><font size=2><b>Gateway</b></font></td>
			<td width="20%" align=center><font size=2><b>Status</b></font></td>
		  </tr>
		</table>
	</div>
</div> 

<div class="column" <%voip_other_get("display_pulse_dial_gen");%>>
	<div class="column_title">
		<div class="column_title_left"></div>
		<p><% multilang(LANG_FXO_PULSE_DIAL_GENERATION);%></p>
		<div class="column_title_right"></div>
	</div>	
	<div class="data_common">				
	<table>
		<tr>
		<td bgColor=#aaddff> 
			<%voip_other_get("pulse_dial_generation");%>
		</td>	
		</tr>		
		<tr>
		<td bgColor=#aaddff width=150><% multilang(LANG_PULSE_PER_SECOND);%></td>
		<td bgColor=#ddeeff> 
			<select name=pulse_gen_PPS>
			<%voip_other_get("pulse_gen_PPS");%>
			</select>
		</td>	
		</tr>		
		<tr>
			<td bgColor=#aaddff rowspan=2><% multilang(LANG_MAKE_DURATION);%></td>
			<td bgColor=#ddeeff>
			<input type=text name=pulse_gen_Make size=5 maxlength=2 value="<%voip_other_get("pulse_gen_Make");%>">
			(<% multilang(LANG_MSEC);%>)
		</td>
		</tr>		
		<tr>
			<td bgColor=#ddeeff><font size=2><% multilang(LANG_1_99_MSEC_FOR_10PPS_1_49_MSEC_FOR_20PPS);%></font>
			</td>	
		</tr>	
		<tr>
		<td bgColor=#aaddff width=150><% multilang(LANG_INTERDIGIT_PAUSE_DURATION);%></td>
		<td bgColor=#ddeeff>
			<input type=text name=pulse_gen_Pause size=5 maxlength=4 value="<%voip_other_get("pulse_gen_Pause");%>">
			(<% multilang(LANG_MSEC);%>)
		</td>	
		</tr>
		
	</table>
	</div>
</div> 
-->
	
<div class="column">
	<div class="column_title">
		<div class="column_title_left"></div>
		<p><% multilang(LANG_SIP_SETTING);%></p>
		<div class="column_title_right"></div>
	</div>	
	<div class="data_common">				
		<table cellSpacing=1 cellPadding=2 border=0 width=450 >
			<tr>
				<td bgColor=#aaddff width=150><% multilang(LANG_SIP_PRACK);%></td>
				<td bgColor=#ddeeff>
					<input type=checkbox name=sip_disable_prack <%voip_other_get("sip_disable_prack");%>><% multilang(LANG_DISABLED);%>
				</td>
			</tr>
			<tr>
				<td bgColor=#aaddff width=150><% multilang(LANG_SIP_SERVER_RENDUNDACY);%></td>
				<td bgColor=#ddeeff>
					<input type=checkbox name=sip_enable_redundancy <%voip_other_get("sip_enable_redundancy");%>><% multilang(LANG_ENABLED);%>
				</td>
			</tr>
			<tr>
				<td bgColor=#aaddff width=150><% multilang(LANG_SIP_CLIR_ANONYMOUSE_FROM_HEADER);%></td>
				<td bgColor=#ddeeff>
					<input type=checkbox name=sip_anonymouse_from <%voip_other_get("sip_anonymouse_from");%>><% multilang(LANG_ENABLED);%>
				</td>
			</tr>
			<tr>
				<td bgColor=#aaddff width=150><% multilang(LANG_NON_SIP_INBOX_CALL);%></td>
				<td bgColor=#ddeeff>
					<input type=checkbox name=sip_internalcall <%voip_other_get("sip_internalcall");%>><% multilang(LANG_ENABLED);%>
				</td>
			</tr>
			<tr>
				<td bgColor=#aaddff width=150><% multilang(LANG_HOOK_FLASH_RELAY_SETTING);%>:</td>
						<td bgColor=#ddeeff>
						<select name=hookflash_type>
							<%voip_other_get("hookflash_type");%>
						</select>
					</td>
			</tr>
			<tr>
   				<td bgColor=#aaddff width=150>SIP Min-SE</td>
					<td bgColor=#ddeeff>
						<input type=text name=sip_mini_se size=4 maxlength=4 value="<%voip_other_get("sip_mini_se");%>">
						(<% multilang(LANG_SEC);%>)
					</td>
			</tr>
			<tr>
				<td bgColor=#aaddff width=150><% multilang(LANG_USER_EQUAL_PHONE);%></td>
				<td bgColor=#ddeeff>
					<input type=checkbox name=user_equal_phone <%voip_other_get("user_equal_phone");%>><% multilang(LANG_ENABLED);%>
				</td>
			</tr>
			<tr>
				<td bgColor=#aaddff width=150><% multilang(LANG_POUND_TO_ASCII);%></td>
				<td bgColor=#ddeeff>
					<input type=checkbox name=pound_to_ascii <%voip_other_get("pound_to_ascii");%>><% multilang(LANG_ENABLED);%>
				</td>
			</tr>
		</table>
	</div>
</div> 

<div class="column">
	<div class="column_title">
		<div class="column_title_left"></div>
		<p><% multilang(LANG_SIP_OPTIONS);%></p>
		<div class="column_title_right"></div>
	</div>	
	<div class="data_common">				
		<table cellSpacing=1 cellPadding=2 border=0 width=450>
			<tr>
			<td bgColor=#aaddff> 
				<%voip_other_get("sip_option_flag");%>
			</td>	
			</tr>			
			<tr>
			<td bgColor=#aaddff width=150><% multilang(LANG_OPTIONS_INTERVAL_TIME);%></td>
			<td bgColor=#ddeeff>
				<input type=text name=sip_option_HeartbeatCycle size=5 maxlength=4 value="<%voip_other_get("sip_option_HeartbeatCycle");%>">
				(<% multilang(LANG_SEC);%>)
			</td>	
			</tr>			
		</table>
	</div>
</div> 

<p><%voip_general_get("not_ipphone_option_start");%></p>
<div style="padding:10px 0;">
	<input class="link_bg" type="submit" value="<% multilang(LANG_APPLY);%>" onclick="return check_other()">
</div>
<%voip_general_get("not_ipphone_option_end");%>

<input type="hidden" name="postSecurityFlag" value="">

</form>
</body>
</html>
