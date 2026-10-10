<html>
<! Copyright (c) Realtek Semiconductor Corp., 2003. All Rights Reserved. ->
<head>
<meta http-equiv="Content-Type" content="text/html" charset="utf-8">
<title><% multilang(LANG_VOIP_CALLHISTORY); %></title>
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
<blockquote>
<h2><font color="#0000FF">VoIP CallHistory</font></h2>

<table border=0 width="600" cellpadding=0>
    <tr><td><font size=3>This page shows the VoIP Call log.</font></td></tr>
    <tr><td><hr size=1 noshade align=top></td></tr>
</table>

<form action=/boaform/voip_log_set method=POST name="voip_log_set"> 
    <input type="hidden" value="/voip_callhistory.asp" name="call_log_refesh">
    <input type="submit" value="<% multilang(LANG_REFRESH); %>" name="refresh" onClick="return on_submit(this)">
    <br>
    <%voip_log_get();%>
    <input type="hidden" name="postSecurityFlag" value="">
</form>
</blockquote>
</body>
</html>
