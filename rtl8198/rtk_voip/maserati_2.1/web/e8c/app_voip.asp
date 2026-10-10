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
<!--<SCRIPT language="javascript" src="urlEncodeGBK.js"></SCRIPT> -->
<SCRIPT language="javascript" src="common.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript">

var svr_type_ims = "IMS SIP";
var svr_type_soft_switch = "Soft Switch SIP";
var svr_type_h248 = "H.248";
var cur_svr_type = "";


var h248_support = 0;

/********************************************************************
**          on document load
********************************************************************/
function on_init()
{
	var svr_select_index = document.forms[0].servertype.selectedIndex;
	var svr_options      = document.forms[0].servertype.options;
	var svr_length       = document.forms[0].servertype.length;

	//console.log( "index: " + svr_select_index );
	//console.log( "index: " + svr_options[svr_select_index].index + " is " + svr_options[svr_select_index].text );
	//console.log( "length: " + svr_length );	
	
	if( svr_length == 3 ){ //length 3 means support h248
		h248_support = 1;
		//console.log( "support h248 " + h248_support );
	}
	else {
		h248_support = 0;
		//console.log( "not support h248 " + h248_support );		
	}
		
	h248_hide();
	sip_hide();

	cur_svr_type = document.forms[0].servertype.value;
	check_svr_type();
}

/********************************************************************
**          on document submit
********************************************************************/
function on_submit() 
{

	if((form.proxy0_port.value == "") || !sji_checkdigitrange(form.proxy0_port.value, 1, 65535))
	{
		alert("主用SIP Proxy请输入合法的端口号！");
		return false;
	}

	if(!(form.proxy1_port.value == "") && !sji_checkdigitrange(form.proxy1_port.value, 1, 65535))
	{
		alert("备用SIP Proxy请输入合法的端口号！");
		return false;
	}

	//if(!(form.port1_number.value == "") && !sji_checkusername(form.port1_number.value, 1, 32))
	//{
		//alert("线路1 请输入合法的号码！");
		//return false;
	//}
	//if(!(form.port1_login_id.value == "") && !sji_checkusername(form.port1_login_id.value, 1, 32))
	//{
		//alert("线路1 请输入合法的用户名！");
		//return false;
	//}
	//if(!(form.port1_password.value == "") &&!sji_checkpswnor(form.port1_password.value, 1, 32))
	//{
		//alert("线路1 请输入合法的密码！");
		//return false;
	//}
	//if((form.max_voip_ports.value == 2)&&!(form.port2_number.value == "") && !sji_checkusername(form.port2_number.value, 1, 32))
	//{
		//alert("线路2 请输入合法的号码！");
		//return false;
	//}
	//if((form.max_voip_ports.value == 2)&&!(form.port2_login_id.value == "") && !sji_checkusername(form.port2_login_id.value, 1, 32))
	//{
		//alert("线路2 请输入合法的用户名！");
		//return false;
	//}
	//if((form.max_voip_ports.value == 2)&&!(form.port2_password.value == "") &&!sji_checkpswnor(form.port2_password.value, 1, 32))
	//{
		//alert("线路2 请输入合法的密码！");
		//return false;
	//}
	if((form.proxy0_reg_expire.value == "") || !sji_checkdigitrange(form.proxy0_reg_expire.value, 10, 86400))
	{
		alert("主用SIP Proxy Register Expire out of range [10-86400]！");
		return false;
	}
	if((form.proxy1_reg_expire.value == "") || !sji_checkdigitrange(form.proxy1_reg_expire.value, 10, 86400))
	{
		alert("备用SIP Proxy Register Expire out of range [10-86400]！");
		return false;
	}
	if((form.proxy1_enable.checked == true) && (form.port1_account_enable.checked != true)&&(form.port2_account_enable.checked != true))
	{
		alert("启用备用sip proxy 需啟用线路账号");
		return false;
	}
	if((form.max_voip_ports.value == 2) && (form.port1_number.value!="")&&(form.port1_number.value == form.port2_number.value))
	{
		alert("线路1号码与线路2号码相同");
		return false;
	}
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	form.submit();
	//submit();
}

function sip_show_inner(display)
{
	document.getElementById('sip_config').style.display               = display;
	document.getElementById('sip_main_proxy_caption').style.display   = display;
	document.getElementById('sip_backup_proxy_caption').style.display = display;
	document.getElementById('sip_account').style.display              = display;
	document.getElementById('sip_account_caption').style.display      = display;
}

function h248_show_inner(display)
{
	document.getElementById('h248_config').style.display              = display;
	document.getElementById('h248_mgc_caption').style.display         = display;
	document.getElementById('h248_mg_caption').style.display          = display;
	
	if (h248_support){ // because this was from backend
		document.getElementById('h248_account').style.display             = display;
		document.getElementById('h248_account_caption').style.display     = display;
	}
}

function sip_show()
{
	sip_show_inner("block");
	h248_show_inner("none");
}

function h248_show()
{
	sip_show_inner("none");
	h248_show_inner("block");
}

function sip_hide()
{
	sip_show_inner("none");
}

function h248_hide()
{
	h248_show_inner("none");
}

function check_svr_type()
{
	//console.log( "check_svr_type" );
	if( cur_svr_type == svr_type_ims || cur_svr_type == svr_type_soft_switch ){
		sip_show();
	}
	else if (cur_svr_type == svr_type_h248){
		h248_show();
	}
}

function on_ctrlupdate()
{
	//if (document.all.serverdisplay.value == "IMS SIP" || document.all.serverdisplay.value == "Soft Switch SIP")
    if (document.all.serverdisplay.value.match('SIP') != null)
	{ 
		document.all.siptable.style.display="inline"; 
		document.all.h248table.style.display="none"; 
	} 
	else 
	{ 
		document.all.siptable.style.display="none"; 
		document.all.h248table.style.display="inline"; 
	} 
}

</SCRIPT>
</HEAD>

<!-------------------------------------------------------------------------------------->
<!--主页代码-->

<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000" >
	<blockquote>
		<DIV align="left" style="padding-left:20px; padding-top:5px">
			<form id="form" action=/boaform/admin/voip_e8c_set method=POST>
			<input type=hidden name=max_voip_ports value="<%voip_e8c_get("max_voip_ports");%>">
			<input type=hidden name=serverdisplay value="<%voip_e8c_get("serverdisplay");%>">
				<td>
				<table border="0" cellpadding="0" cellspacing="0" width="700">
					<tr>
						<td width="200px">语音协议</td><td><select name="servertype" id="servertype" onChange="on_ctrlupdate()">"<%voip_e8c_get("servertype");%>"</select></td>
					</tr>
				</table>	
<!--SIP-->
<div id = siptable <%voip_e8c_get("sip_display");%> >
			<table id="sip_config" name="sip_config"  border="0" cellpadding="0" cellspacing="0" width="700">
			<tr>
					<td><b id="sip_main_proxy_caption" name="sip_main_proxy_caption"  >主用SIP 代理</b></td>
				</tr>	
<!--				
				<tr>
					<td>开启订阅</td><td><input name="proxy0_subscribe_enable" type="checkbox"  value="enable" <% voip_e8c_get("proxy0_subscribe_enable"); %>></td>
				</tr>				
-->		
<input type=hidden name="proxy0_subscribe_enable" type="checkbox"  value="enable" <% voip_e8c_get("proxy0_subscribe_enable"); %>>
				<tr>
					<td width="200px">服务器地址:</td><td><input name="proxy0_addr" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("proxy0_addr"); %>"></td>
				</tr>
				<tr>
					<td>端口号:</td><td><input name="proxy0_port" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("proxy0_port"); %>"></td>
				</tr>
				<tr>
					<td width="200px">注册地址:</td><td><input name="proxy0_register_addr" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("proxy0_register_addr"); %>"></td>
				</tr>
				<tr>
					<td>注册端口号:</td><td><input name="proxy0_register_port" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("proxy0_register_port"); %>"></td>
				</tr>				
				<tr>
					<td>外发代理启用</td><td><input name="proxy0_obEnable" type="checkbox" value="enable" <% voip_e8c_get("proxy0_obEnable"); %>></td>
				</tr>
				<tr>
					<td>外发代理地址:</td><td><input name="proxy0_obAddr" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("proxy0_obAddr"); %>"></td>
				</tr>
				<tr>
					<td>外发代理端口号:</td><td><input name="proxy0_obPort" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("proxy0_obPort"); %>"></td>
				</tr>
				<tr>
					<td>SIP 域名:</td><td><input name="proxy0_domain_name" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("proxy0_domain_name"); %>"></td>
				</tr>
				<tr>
					<td>注册有效期 (秒):</td><td><input name="proxy0_reg_expire" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("proxy0_reg_expire"); %>"></td>
				</tr>
				<tr>
					<td>启用会话更新</td><td><input name="proxy0_sessionEnable" type="checkbox" value="enable" <% voip_e8c_get("proxy0_sessionEnable"); %>></td>
				</tr>
				<tr>
					<td>会话更新周期 (秒):</td><td><input name="proxy0_sessionExpiry" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("proxy0_sessionExpiry"); %>"></td>
				</tr>
				<tr>
					<td><b id="sip_backup_proxy_caption" name="sip_backup_proxy_caption" >备用SIP 代理</b></td>
				</tr>
				<tr>
					<td>备用SIP启用</td><td><input name="proxy1_enable" type="checkbox"  value="enable" <% voip_e8c_get("proxy1_enable"); %>></td>
				</tr>
<!--				
				<tr>
					<td>开启订阅</td><td><input name="proxy1_subscribe_enable" type="checkbox"  value="enable" <% voip_e8c_get("proxy1_subscribe_enable"); %>></td>
				</tr>
-->		
<input  type=hidden name="proxy1_subscribe_enable" type="checkbox"  value="enable" <% voip_e8c_get("proxy1_subscribe_enable"); %>>
				<tr>
					<td>服务器地址:</td><td><input name="proxy1_addr" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("proxy1_addr"); %>"></td>
				</tr>
				<tr>
					<td>端口号:</td><td><input name="proxy1_port" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("proxy1_port"); %>"></td>
				</tr>
				<tr>
					<td>注册地址:</td><td><input name="proxy1_register_addr" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("proxy1_register_addr"); %>"></td>
				</tr>
				<tr>
					<td>注册端口号:</td><td><input name="proxy1_register_port" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("proxy1_register_port"); %>"></td>
				</tr>
				<tr>
					<td>外发代理启用</td><td><input name="proxy1_obEnable" type="checkbox" value="enable" <% voip_e8c_get("proxy1_obEnable"); %>></td>
				</tr>
				<tr>
					<td>外发代理地址:</td><td><input name="proxy1_obAddr" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("proxy1_obAddr"); %>"></td>
				</tr>
				<tr>
					<td>外发代理端口号:</td><td><input name="proxy1_obPort" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("proxy1_obPort"); %>"></td>
				</tr>
				<tr>
					<td>SIP 域名:</td><td><input name="proxy1_domain_name" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("proxy1_domain_name"); %>"></td>
				</tr>
				<tr>
					<td>注册有效期(秒):</td><td><input name="proxy1_reg_expire" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("proxy1_reg_expire"); %>"></td>
				</tr>
				<tr>
					<td>启用会话更新</td><td><input name="proxy1_sessionEnable" type="checkbox" value="enable" <% voip_e8c_get("proxy1_sessionEnable"); %>></td>
				</tr>
				<tr>
					<td>会话更新周期 (秒):</td><td><input name="proxy1_sessionExpiry" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("proxy1_sessionExpiry"); %>"></td>
				</tr>
			</table>
			<br>
				<% voip_e8c_get("port_account"); %>
			<br>
</div>			
<!--H248-->
<div id = h248table <%voip_e8c_get("h248_display");%> >
		<table id="h248_config" name="h248_config"  border="0" cellpadding="0" cellspacing="0" width="700">
				<tr>
					<td><b id="h248_mgc_caption" name="h248_mgc_caption" >媒体服务器配置</b></td>
				</tr>
				<tr>
					<td width="200px">端口号:</td><td><input name="h248_port" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("h248_port"); %>">(1025-65535)</td>
				</tr>
				<tr>
					<td>主用媒体网关控制服务器地址</td><td><input name="h248_mgc0_address" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("h248_mgc0_address"); %>"></td>
				</tr>
				<tr>
					<td>主用媒体网关控制服务器端口</td><td><input name="h248_mgc0_port"    size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("h248_mgc0_port"); %>">(1025-65535)</td>
				</tr>
				<tr>
					<td>备用媒体网关控制服务器地址</td><td><input name="h248_mgc1_address" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("h248_mgc1_address"); %>"></td>
				</tr>
				<tr>
					<td>备用媒体网关控制服务器端口</td><td><input name="h248_mgc1_port"    size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("h248_mgc1_port"); %>">(1025-65535)</td>
				</tr>
				<tr>
					<td>终端标识的类型</td>
						<td><select name=h248_id_type>"<%voip_e8c_get("h248_id_type");%>"</select></td>
				</tr>
				<tr>
					<td>终端标识</td><td><input name="h248_termination_id"    size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("h248_termination_id"); %>"></td>
				</tr>
				<tr>
					<td>H.248协议编码类型</td>
                        <td><select name=h248_encoding_type>"<%voip_e8c_get("h248_encoding_type");%>"</select></td>
				</tr>

				<tr>
					<td><b id="h248_mg_caption" name="h248_mg_caption" >终结点配置</b></td>
				</tr>
				<tr>
					<td>终端物理终结点标识前缀</td><td><input name="h248_phy_term_prefix" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("h248_phy_term_prefix"); %>"></td>
				</tr>
				<tr>
					<td>终端物理终结点起始值</td><td><input name="h248_phy_term_start" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("h248_phy_term_start"); %>">(0-32767)</td>
				</tr>
				<tr>
					<td>终端物理终结点标识前缀后面添加位数</td><td><input name="h248_phy_term_len" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("h248_phy_term_len"); %>">(0-15)</td>
				</tr>
				<tr>
					<td>RTP终结点标识前缀</td><td><input name="h248_rtp_term_prefix" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("h248_rtp_term_prefix"); %>"></td>
				</tr>
				<tr>
					<td>RTP终结点标识名起始值</td><td><input name="h248_rtp_term_start" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("h248_rtp_term_start"); %>">(0-15)</td>
				</tr>
				<tr>
					<td>RTP终结点标识的前缀后面添加的数字部分的位数</td><td><input name="h248_rtp_term_len" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("h248_rtp_term_len"); %>">(0-15)</td>
				</tr>
		</table>
			<br>
				<% voip_e8c_get("h248_account"); %>
			<br>
</div>

				
				
				<input id="btn_apply" type="button" class="btnsaveup" name="btnDown" value="保存/应用" onClick="on_submit()">
				<input type="hidden" name="submit-url" value="">
				<input type="hidden" name="postSecurityFlag" value="">
			</form>
		</DIV>
	</blockquote>
</body>
<%addHttpNoCache();%>
</html>
