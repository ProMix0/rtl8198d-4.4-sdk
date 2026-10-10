<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">

<HTML>
<HEAD>
<TITLE>Debugging</TITLE>
<META http-equiv=pragma content=no-cache>
<META http-equiv=cache-control content="no-cache, must-revalidate">
<META http-equiv=content-type content="text/html; charset=gbk">
<META http-equiv=content-script-type content=text/javascript>

<STYLE type=text/css>
@import url(/style/default.css);
</STYLE>

<SCRIPT language="javascript" src="urlEncodeGBK.js"></SCRIPT>
<SCRIPT language="javascript" src="common.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript">


/********************************************************************
**          on document submit
********************************************************************/
function on_submit()
{
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	form.submit();
}
</SCRIPT>
</HEAD>

<!-------------------------------------------------------------------------------------->
<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000">
	<blockquote>
		<DIV align="left" style="padding-left:20px; padding-top:5px">
			<form id="form" action=/boaform/admin/voip_flags_set method=POST>
				<table border="0" cellpadding="0" cellspacing="0" width="500">
					<tr>
						<td>SIP_DISABLE_PRACK</td><td>(1<<0)</td>
						<td><input type="checkbox" name="rfc_flags_0" size=20 <%voip_flags_get("rfc_flags_0");%>> </td>
					</tr>
					<tr>
						<td>SIP_FLASHHOOK_INFO</td><td>(1<<1)</td>
						<td><input type="checkbox" name="rfc_flags_1" size=20 <%voip_flags_get("rfc_flags_1");%>> </td>
					</tr>
					<tr>
						<td>SIP_RTP_ROUTING</td><td>(1<<2)</td>
						<td><input type="checkbox" name="rfc_flags_2" size=20 <%voip_flags_get("rfc_flags_2");%>> </td>
					</tr>
					<tr>
						<td>SIP_DISABLE_REGISTER_OUTBOUND</td><td>(1<<3)</td>
						<td><input type="checkbox" name="rfc_flags_3" size=20 <%voip_flags_get("rfc_flags_3");%>> </td>
					</tr>
					<tr>
						<td>SIP_ANONYMOUSE_FROM</td><td>(1<<4)</td>
						<td><input type="checkbox" name="rfc_flags_4" size=20 <%voip_flags_get("rfc_flags_4");%>> </td>
					</tr>
					<tr>
						<td>SIP_SERVER_REDUNDANCY</td><td>(1<<5)</td>
						<td><input type="checkbox" name="rfc_flags_5" size=20 <%voip_flags_get("rfc_flags_5");%>> </td>
					</tr>
					<tr>
						<td>SIP_INTERNALL_CALL</td><td>(1<<6)</td>
						<td><input type="checkbox" name="rfc_flags_6" size=20 <%voip_flags_get("rfc_flags_6");%>> </td>
					</tr>
					<tr>
						<td>SIP_OPTION</td><td>(1<<7)</td>
						<td><input type="checkbox" name="rfc_flags_7" size=20 <%voip_flags_get("rfc_flags_7");%>> </td>
					</tr>
					<tr>
						<td>SIP_DATE</td><td>(1<<8)</td>
						<td><input type="checkbox" name="rfc_flags_8" size=20 <%voip_flags_get("rfc_flags_8");%>> </td>
					</tr>
					<tr>
						<td>SIP_FAX_AUTO</td><td>(1<<9)</td>
						<td><input type="checkbox" name="rfc_flags_9" size=20 <%voip_flags_get("rfc_flags_9");%>> </td>
					</tr>
					<tr>
						<td>SIP_DEBUG</td><td>(1<<10)</td>
						<td><input type="checkbox" name="rfc_flags_10" size=20 <%voip_flags_get("rfc_flags_10");%>> </td>
					</tr>
					<tr>
						<td>SIP_DIGIT_MAP_MODE_MIN</td><td>(1<<11)</td>
						<td><input type="checkbox" name="rfc_flags_11" size=20 <%voip_flags_get("rfc_flags_11");%>> </td>
					</tr>
					<tr>
						<td>FXS_REVERSE_POLARIY_OFF</td><td>(1<<12)</td>
						<td><input type="checkbox" name="rfc_flags_12" size=20 <%voip_flags_get("rfc_flags_12");%>> </td>
					</tr>
					<tr>
						<td>SIP_SUBSREG</td><td>(1<<13)</td>
						<td><input type="checkbox" name="rfc_flags_13" size=20 <%voip_flags_get("rfc_flags_13");%>> </td>
					</tr>
					<tr>
						<td>SIP_DTMFMethodSIPInfoBody_TELEven</td><td>(1<<14)</td>
						<td><input type="checkbox" name="rfc_flags_14" size=20 <%voip_flags_get("rfc_flags_14");%>> </td>
					</tr>
					<tr>
						<td>SIP_CONFIG_BY_OMCI</td><td>(1<<15)</td>
						<td><input type="checkbox" name="rfc_flags_15" size=20 <%voip_flags_get("rfc_flags_15");%>> </td>
					</tr>
					<tr>
						<td>REGISTER_FROM_USE_LOGINID</td><td>(1<<16)</td>
						<td><input type="checkbox" name="rfc_flags_16" size=20 <%voip_flags_get("rfc_flags_16");%>> </td>
					</tr>
					<tr>
						<td>RETRY_MAIN_PROXY_ON</td><td>(1<<17)</td>
						<td><input type="checkbox" name="rfc_flags_17" size=20 <%voip_flags_get("rfc_flags_17");%>> </td>
					</tr>
					<tr>
						<td>FXS_3WAY_ON_CLIENT</td><td>(1<<18)</td>
						<td><input type="checkbox" name="rfc_flags_18" size=20 <%voip_flags_get("rfc_flags_18");%>> </td>
					</tr>
					<tr>
						<td>SIP_DATE_FROM_SEVEER</td><td>(1<<19)</td>
						<td><input type="checkbox" name="rfc_flags_19" size=20 <%voip_flags_get("rfc_flags_19");%>> </td>
					</tr>
					<tr>
						<td>SIP_NOREG_NOTONE</td><td>(1<<20)</td>
						<td><input type="checkbox" name="rfc_flags_20" size=20 <%voip_flags_get("rfc_flags_20");%>> </td>
					</tr>
					<tr>
						<td>SIP_SESSION_TIMER_USE_INVITE</td><td>(1<<21)</td>
						<td><input type="checkbox" name="rfc_flags_21" size=20 <%voip_flags_get("rfc_flags_21");%>> </td>
					</tr>
					<tr>
						<td>SIP_ENABLE_POUND_KEY_TO_ASCII</td><td>(1<<22)</td>
						<td><input type="checkbox" name="rfc_flags_22" size=20 <%voip_flags_get("rfc_flags_22");%>> </td>
					</tr>
					<tr>
						<td>SIP_DIGITMAP_ERROR_DIALOUT_DISABLE</td><td>(1<<23)</td>
						<td><input type="checkbox" name="rfc_flags_23" size=20 <%voip_flags_get("rfc_flags_23");%>> </td>
					</tr>
					<tr>
						<td>SIP_FJ_CHANGE_REGISTER_TIMER</td><td>(1<<24)</td>
						<td><input type="checkbox" name="rfc_flags_24" size=20 <%voip_flags_get("rfc_flags_24");%>> </td>
					</tr>
					<tr>
						<td>SIP_IPV6_ENABLE</td><td>(1<<25)</td>
						<td><input type="checkbox" name="rfc_flags_25" size=20 <%voip_flags_get("rfc_flags_25");%>> </td>
					</tr>
					<tr>
						<td>SIP_USER_EQUAL_PHONE</td><td>(1<<26)</td>
						<td><input type="checkbox" name="rfc_flags_26" size=20 <%voip_flags_get("rfc_flags_26");%>> </td>
					</tr>
				</table>
				<br>
				<input id="btn_apply" type="button" class="btnsaveup" name="btnDown" value="保存/应用" onClick="on_submit()">
				<input type="hidden" name="submit-url" value="">
				<input type="hidden" name="postSecurityFlag" value="">
			</form>
		</DIV>
	</blockquote>
</body>
<%addHttpNoCache();%>
</html>
