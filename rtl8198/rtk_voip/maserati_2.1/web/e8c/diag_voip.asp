<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<!--系统默认模板-->
<HTML>

<head>
<title>手动上报 Inform</title>
<meta http-equiv=pragma content=no-cache>
<meta http-equiv=cache-control content="no-cache, must-revalidate">
<meta http-equiv=content-type content="text/html; charset=utf-8">
<style type=text/css>
@import url(/style/default.css);
</style>
<script language="javascript" src="common.js"></script>
<script language="javascript">

function on_init()
{
	if(!(document.getElementById("auto_call_state"))||document.getElementById("auto_call_state").innerHTML != "")
	{
		setTimeout(function(){
			   window.location.reload(true);
			}, 1000);
	}
}

function on_submit(submitform) 
{
	postTableEncrypt(document.forms[submitform].postSecurityFlag, document.forms[submitform]);
	document.forms[submitform].submit();
	//submit();
}
</script>
</head>
<!-------------------------------------------------------------------------------------->
<!--主页代码-->
<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000" onLoad="on_init();">
	<blockquote>
		<DIV align="left" style="padding-left:20px; padding-top:5px">
			<form action=/boaform/admin/diag_voip_e8c_set method=POST name="linetest">
				<b>语音诊断</b>
				<table border="0" cellpadding="0" cellspacing="0" width="500">
					<tr>
						<td width="200px">启动线路测试</td>
						<td width="200px"><input type="button" class="btnsaveup" name="btnDown" value="线路测试" onClick=on_submit("linetest")></td>
					</tr>
				</table>
				<br>
				<input type="hidden" name="submit-url" value="">
				<table border="0" cellpadding="0" cellspacing="0" width="500">
					<tr>
						<td width="200px">外线电压</td>
						<td width="200px"><% diag_voip_e8c_get("voltage"); %></td>
					</tr>
					<tr>
						<td width="200px">振铃阻抗</td>
						<td width="200px"><% diag_voip_e8c_get("ring_impedance"); %></td>
					</tr>
				</table>
				<input type="hidden" name="postSecurityFlag" value="">				
			</form>
		</DIV>
	</blockquote>
	
	<blockquote>
		<DIV align="left" style="padding-left:20px; padding-top:5px">
			<form action=/boaform/admin/diag_call_set method=POST name="calltest">			
				<b>主叫通话测试<br>
				<table border="0" cellpadding="0" cellspacing="0" width="500">
					<tr>
						<td width="200px">呼叫号码</td>
						<td><input name="called_number" size="16" maxlength="32" type="text" style="width:140px" value="<% diag_voip_e8c_get("called_number"); %>"></td>
					</tr>
					<tr>
						<td width="200px">状态</td>
						<% diag_voip_e8c_get("auto_call_state"); %>
					</tr>			
				</table>
				<br>
				<td width="200px"><input type="button" <% diag_voip_e8c_get("register_accept"); %> class="btndeleup_l" name="btnDown" value="模拟主叫通话测试" onClick=on_submit("calltest")></td>
				<input type="hidden" name="submit-url" value="">				
				<input type="hidden" name="postSecurityFlag" value="">
			</form>
		</DIV>
	</blockquote>

</body>
<%addHttpNoCache();%>
</html>
