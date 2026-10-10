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
	if(document.getElementById("autoCallTestStatus").innerHTML == "Requested")
	{
		setTimeout(function(){
			   window.location.reload(true);
			}, 2000);
	}
	

	if(document.getElementById("autoCallTestSelector").value == 0)
	{
		document.getElementById('autoCallTestType').setAttribute("disabled","disabled");
		document.getElementById('autoCallCallednumber').setAttribute("disabled","disabled");
		document.getElementById('autoCallDailDTMFConfirmEnable').setAttribute("disabled","disabled");
		document.getElementById('autoCallDailDTMFConfirmNumber').setAttribute("disabled","disabled");
	}else{
		document.getElementById('autoCallTestType').removeAttribute("disabled");
		document.getElementById('autoCallCallednumber').removeAttribute("disabled");
		document.getElementById('autoCallDailDTMFConfirmEnable').removeAttribute("disabled");
		document.getElementById('autoCallDailDTMFConfirmNumber').removeAttribute("disabled");		
	}

    if(document.getElementById("autoCallTestType").value == 1)
        document.getElementById('autoCallCallednumber').setAttribute("disabled","disabled");
    else
        document.getElementById('autoCallCallednumber').removeAttribute("disabled");
}

function on_TestSelector_change()
{
	if(document.getElementById("autoCallTestSelector").value == 0)
	{
		document.getElementById('autoCallTestType').setAttribute("disabled","disabled");
		document.getElementById('autoCallCallednumber').setAttribute("disabled","disabled");
		document.getElementById('autoCallDailDTMFConfirmEnable').setAttribute("disabled","disabled");
		document.getElementById('autoCallDailDTMFConfirmNumber').setAttribute("disabled","disabled");
	}else{
		document.getElementById('autoCallTestType').removeAttribute("disabled");
		document.getElementById('autoCallCallednumber').removeAttribute("disabled");
		document.getElementById('autoCallDailDTMFConfirmEnable').removeAttribute("disabled");
		if(document.getElementById("autoCallDailDTMFConfirmEnable").checked == true)
			document.getElementById('autoCallDailDTMFConfirmNumber').removeAttribute("disabled");
		else
			document.getElementById('autoCallDailDTMFConfirmNumber').setAttribute("disabled","disabled");		
	} 
}

function on_TestType_change()
{
    if(document.getElementById("autoCallTestType").value == 1)
        document.getElementById('autoCallCallednumber').setAttribute("disabled","disabled");
    else
        document.getElementById('autoCallCallednumber').removeAttribute("disabled");
}

function on_DTMFCFEnable_click()
{
	if(document.getElementById("autoCallDailDTMFConfirmEnable").checked == true)
		document.getElementById('autoCallDailDTMFConfirmNumber').removeAttribute("disabled");
	else
		document.getElementById('autoCallDailDTMFConfirmNumber').setAttribute("disabled","disabled");	
}

function on_submit() 
{
	form.submit();
	//submit();
}
</script>
</head>
<!-------------------------------------------------------------------------------------->
<!--主页代码-->
<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000" onLoad="on_init();">
	<blockquote>
		<DIV align="left" style="padding-left:20px; padding-top:5px">
			<form action=/boaform/admin/diag_e8c_auto_call_set method=POST name="linetest">
				<b>测试设置</b>
				<table border="0" cellpadding="0" cellspacing="0" width="500">
					<tr>
						<td width="200px">测试选择:</td>
						<td><select id="autoCallTestSelector" name="autoCallTestSelector" onChange="on_TestSelector_change()"> <%diag_voip_e8c_get("autoCallTestSelector");%></select></td>
					</tr>
					<tr>
						<td width="200px">仿真测试类型: </td>
						<td><select id="autoCallTestType" name="autoCallTestType" onChange="on_TestType_change()"> <%diag_voip_e8c_get("autoCallTestType");%></select></td>
					</tr>
					<tr>
						<td width="200px">主叫仿真时的被叫号码: </td>
						<td><input id="autoCallCallednumber" name="autoCallCallednumber"  size="16" maxlength="24" type="text" style="width:140px" value="<% diag_voip_e8c_get("autoCallCallednumber"); %>"></td>
					</tr>
					<tr>
						<td width="200px">拨号确认: </td>
						<td><input id="autoCallDailDTMFConfirmEnable" type=checkbox name="autoCallDailDTMFConfirmEnable"  onClick="on_DTMFCFEnable_click()" size=20 <% diag_voip_e8c_get("autoCallDailDTMFConfirmEnable"); %>></td>
					</tr>
					<tr>
						<td width="200px">拨号确认号码:	 </td>
						<td><input id="autoCallDailDTMFConfirmNumber" name="autoCallDailDTMFConfirmNumber"  size="16" maxlength="24" type="text" style="width:140px" value="<% diag_voip_e8c_get("autoCallDailDTMFConfirmNumber"); %>"></td>
						<td width="100px">(*#0-9)</td>
					</tr>
					<tr>
						<td width="200px"><input type="button" class="btnsaveup" name="btnDown" value="开始" onClick="submit()"></td>
					</tr>
				</table>
				<b>测试结果<br>
				<table class="flat" border="1" cellpadding="1" cellspacing="1">
					<tr>
						<td width="200px">测试状态: </td>
						<% diag_voip_e8c_get("autoCallTestStatus"); %>
					</tr>
					<tr>
						<td width="200px">是否连接了话机: </td>
						<% diag_voip_e8c_get("phoneConnectivity"); %>
					</tr>
					<tr>
						<td width="200px">拨号确认结果: </td>
						<% diag_voip_e8c_get("autoCallDailDTMFConfirmResult"); %>
					</tr>
					<tr>
						<td width="200px">仿真当前状态: </td>
						<% diag_voip_e8c_get("autoCallSimulateStatus"); %>
					</tr>
					<tr>
						<td width="200px">仿真结果: </td>
						<% diag_voip_e8c_get("autoCallConclusion"); %>
					</tr>
					<tr>
						<td width="200px">主叫仿真失败原因: </td>
						<% diag_voip_e8c_get("autoCallCallerFailReason"); %>
					</tr>
					<tr>
						<td width="200px">被叫仿真失败原因: </td>
						<% diag_voip_e8c_get("autoCallCalledFailReason"); %>
					</tr>
					<tr>
						<td width="200px">仿真失败时接收到的错误码: </td>
						<% diag_voip_e8c_get("autoCallFailedResponseCode"); %>
					</tr>						
				</table>				
			</form>
		</DIV>
	</blockquote>	
</body>
<%addHttpNoCache();%>
</html>
