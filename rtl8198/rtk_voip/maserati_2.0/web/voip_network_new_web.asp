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
<script language="javascript" src="common.js"></script>
<script language="javascript">

check_network()

</script>
</head>

<body bgcolor="#ffffff" text="#000000">
<form method="post" action="/boaform/voip_net_set" name=net_form>
	
<div class="column" <%voip_net_get("display_voip_interface_select");%>>
	<div class="column_title">
		<div class="column_title_left"></div>
		<p>VoIP Interface Selection</p>
		<div class="column_title_right"></div>
	</div>	
	<div class="data_common">				
		<table cellSpacing=1 cellPadding=2 border=0 width=450>
			<tr>
				<td bgColor=#aaddff width=150>VoIP Interface</td>
				<td bgColor=#ddeeff>
					<select name="voip_itf">
						<option value=1>Any WAN</option>
						<option value=2>LAN</option>
						<% if_wan_list("all"); %>
					</select>
				</td>
			</tr>
		</table>
	</div>
</div> 

<div class="column">
	<div class="column_title">
		<div class="column_title_left"></div>
		<p><% multilang(LANG_DSCP_FLAG); %></p>
		<div class="column_title_right"></div>
	</div>	
	<div class="data_common">				
		<table cellSpacing=1 cellPadding=2 border=0 width=450>
			<tr>
				<td bgColor=#aaddff><% multilang(LANG_SIP_DSCP); %></td>
				<td bgColor=#ddeeff>
				<input type=text name=sipDscp size=5 maxlength=2 value="<%voip_net_get("sipDscp"); %>">( 0~63 )
				</td>
			</tr>
			<tr>
				<td bgColor=#aaddff><% multilang(LANG_RTP_DSCP); %></td>
				<td bgColor=#ddeeff>
				<input type=text name=rtpDscp size=5 maxlength=2 value="<%voip_net_get("rtpDscp"); %>">( 0~63 )	
				</td>
			</tr>
		</table>
	</div>
</div> 

<div style="padding:10px 0;">
	<input class="link_bg" type="submit" value="<% multilang(LANG_APPLY); %>" onclick="return check_network()">
</div>
<!--
<input type="hidden" name="postSecurityFlag" value="">
-->
</form>
<% getInfo("voip_wan_intf"); %>
</body>
</html>
