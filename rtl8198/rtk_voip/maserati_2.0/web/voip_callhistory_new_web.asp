<!DOCTYPE html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html" charset="utf-8">
<title><% multilang(LANG_VOIP_CALLHISTORY); %></title>
<link rel="stylesheet" href="./admin/style.css">
<link rel="stylesheet" href="./admin/reset.css" />
<link rel="stylesheet" href="./admin/base.css" />
<SCRIPT language="javascript" src="common.js"></SCRIPT>
<SCRIPT>
function on_submit(obj)
{
	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	return true;
}
</SCRIPT>
</head>
<body>
	<div class="intro_main ">
		<p class="intro_title">VoIP CallHistory</p>
		<p class="intro_content">This page shows the VoIP Call log.</p>
	</div>
<form action=/boaform/voip_log_set method=POST name="voip_log_set"> 
		<input type="hidden" value="/voip_callhistory_new_web.asp" name="call_log_refesh">
	<div style="padding:10px 0;">
		<input class="link_bg" type="submit" value="<% multilang(LANG_REFRESH); %>" name="refresh" onClick="return on_submit(this)">
	</div>
	<div class="data_common data_common_notitle">
		<%voip_log_get();%>
	</div>
    <input type="hidden" name="postSecurityFlag" value="">
</form>

</body>
</html>
