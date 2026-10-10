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
	form.submit();
}
function on_codec_submit() 
{
	form_codec.submit();
}
function on_account_submit() 
{
	form_account.submit();
}
function on_account_del()
{
	var checkedCount = 0; 
	if (form_account_table.port1_account1_del)
	{	
		if (form_account_table.port1_account1_del.checked == true)
		{
			checkedCount++;
		}
	}
	
	if (form_account_table.port1_account2_del)
	{
		if (form_account_table.port1_account2_del.checked == true)
		{
			checkedCount++;
		}
	}
	
	if ((form_account_table.account_number.value!="0") && (checkedCount==0))
	{
		alert("请选择用户");
	}
	else if (form_account_table.account_number.value!="0")
	{
		if (confirm("确定要删除用户"))
		{
	form_account_table.submit();
}
	}
}

function on_account_add()
{
	if (form_account_table.account_number.value == "2")
	{
		alert("超过最大用户数");
	}
	else
	{	
		document.all.form_account_add.style.display="inline";
}
}

function checkbox_gray()
{ 
	document.all.form_codec.frame_size0.disabled = !document.all.form_codec.codec_0_enable.checked;
	document.all.form_codec.frame_size1.disabled = !document.all.form_codec.codec_1_enable.checked;
	document.all.form_codec.frame_size2.disabled = !document.all.form_codec.codec_2_enable.checked;
	document.all.form_codec.frame_size3.disabled = !document.all.form_codec.codec_3_enable.checked;	
	document.all.form_codec.frame_size4.disabled = !document.all.form_codec.codec_4_enable.checked;
}
</SCRIPT>
</HEAD>

<!-------------------------------------------------------------------------------------->
<!--主页代码-->

<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000" >
	<blockquote>
		<DIV align="left" style="padding-left:20px; padding-top:5px">
			<form id="form" action=/boaform/admin/voip_cmcc_set method=POST>
			<input type=hidden name=max_voip_ports value="<%voip_e8c_get("max_voip_ports");%>">
	
			<table id="sip_config" name="sip_config" border="0" cellpadding="10" cellspacing="10" width="700">
				<tr>
					<td width=200>语音协议</td>
					<td><select name=servertype id="servertype" onChange="on_ctrlupdate()">"<%voip_e8c_get("servertype");%>"</select></td>
				</tr>	
			</table>
			
<div id = siptable <%voip_e8c_get("sip_display");%>>			
			<table id="sip_config" name="sip_config" border="0" cellpadding="10" cellspacing="10" width="700">
				<tr>
					<td width=200><b>接口基本参数</b></td>
				</tr>	
				<tr>
					<td>Outbound服务器地址:</td>
					<td><input name="proxy0_obAddr" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("proxy0_obAddr"); %>">(IP或域名)</td>
				</tr>
				<tr>
					<td>Outbound服务器端口号:</td>
					<td><input name="proxy0_obPort" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("proxy0_obPort"); %>">(1-65535)</td>
				</tr>
				<tr>
					<td>备用Outbound服务器地址:</td>
					<td><input name="proxy1_obAddr" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("proxy1_obAddr"); %>">(IP或域名)</td>
				</tr>
				<tr>
					<td>备用Outbound服务器端口号:</td>
					<td><input name="proxy1_obPort" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("proxy1_obPort"); %>">(1-65535)</td>
				</tr>				
				<tr>
					<td>主用服务器地址:</td>
					<td><input name="proxy0_addr" size="16" maxlength="39" type="text" style="width:140px" value="<% voip_e8c_get("proxy0_addr"); %>">(IP或域名)</td>
				</tr>
				<tr>
					<td>主用端口号:</td>
					<td><input name="proxy0_port" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("proxy0_port"); %>">(1-65535)</td>
				</tr>
				<tr>
					<td>备用服务器地址:</td>
					<td><input name="proxy1_addr" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("proxy1_addr"); %>">(IP或域名)</td>
				</tr>
				<tr>
					<td>备用端口号:</td>
					<td><input name="proxy1_port" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("proxy1_port"); %>">(1-65535)</td>
				</tr>
				<tr>
					<td>Registrar服务器地址:</td>
					<td><input name="reg_proxy0_addr" size="16" maxlength="39" type="text" style="width:140px" value="<% voip_e8c_get("reg_proxy0_addr"); %>">(IP或域名)</td>
				</tr>
				<tr>
					<td>Registrar服务器端口号:</td>
					<td><input name="reg_proxy0_port" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("reg_proxy0_port"); %>">(1-65535)</td>
				</tr>
				<tr>
					<td>备用Registrar服务器地址:</td>
					<td><input name="reg_proxy1_addr" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("reg_proxy1_addr"); %>">(IP或域名)</td>
				</tr>
				<tr>
					<td>备用Registrar服务器端口号:</td>
					<td><input name="reg_proxy1_port" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("reg_proxy1_port"); %>">(1-65535)</td>
				</tr>				
				<tr>
					<td>域名属性:</td>
					<td><input name="proxy0_domain_name" size="16" maxlength="39" type="text" style="width:140px" value="<% voip_e8c_get("proxy0_domain_name"); %>">(IP或域名)</td>
				</tr>
				<tr>
					<td>备用域名属性:</td>
					<td><input name="proxy1_domain_name" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("proxy1_domain_name"); %>">(IP或域名)</td>
				</tr>
				<tr>
					<td>传输协议:</td>
					<td><select name=transport_protocol>"<%voip_e8c_get("transport_protocol");%>"</select></td>
				</tr>
				<tr>
					<td>会话更新周期:</td>
					<td><input name="proxy0_sessionExpiry" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("proxy0_sessionExpiry"); %>">(单位: 秒)</td>
				</tr>
				<tr>
					<td>注册重试的间隔时间:</td>
					<td><input name="RegisterRetryInterval" size="16" maxlength="32" type="text" style="width:140px" value="<% voip_e8c_get("RegisterRetryInterval"); %>">(单位: 秒)(1-65534)</td>
				</tr>
				<tr>
					<td>注册周期:</td>
					<td><input name="proxy0_reg_expire" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("proxy0_reg_expire"); %>">(单位: 秒)(1-65534)</td>
				</tr>
			</table>			
</div>	<!-- sip_display -->

<div id = h248table <%voip_e8c_get("h248_display");%> >
			<table id="h248_config" name="h248_config"  border="0" cellpadding="0" cellspacing="0" width="700">
				<tr>
					<td width=200><b id="h248_mgc_caption" name="h248_mgc_caption">媒体服务器配置</b></td>
				</tr>
				<tr>
					<td>MG注册方式</td>
					<td><input name="MG_Register" size="10" maxlength="5" type="text" style="width:140px" disabled="true" value="ROOT"></td>
				</tr>
				<tr>
					<td>端口号:</td>
					<td><input name="h248_port" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("h248_port"); %>">(1025-65535)</td>
				</tr>
				<tr>
					<td>主用媒体网关控制服务器地址</td>
					<td><input name="h248_mgc0_address" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("h248_mgc0_address"); %>"></td>
				</tr>
				<tr>
					<td>主用媒体网关控制服务器端口</td>
					<td><input name="h248_mgc0_port" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("h248_mgc0_port"); %>">(1025-65535)</td>
				</tr>
				<tr>
					<td>备用媒体网关控制服务器地址</td>
					<td><input name="h248_mgc1_address" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("h248_mgc1_address"); %>"></td>
				</tr>
				<tr>
					<td>备用媒体网关控制服务器端口</td>
					<td><input name="h248_mgc1_port" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("h248_mgc1_port"); %>">(1025-65535)</td>
				</tr>
				<tr>
					<td>终端标识的类型</td>
					<td><select name=h248_id_type>"<%voip_e8c_get("h248_id_type");%>"</select></td>
				</tr>
				<tr>
					<td>终端标识</td>
					<td><input name="h248_termination_id" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("h248_termination_id"); %>"></td>
				</tr>
				<tr>
					<td>域名</td>
					<td><input name="h248_domain_name" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("h248_domain_name"); %>"></td>
				</tr>				
				<tr>
					<td>H.248协议编码类型</td>
					<td><select name=h248_encoding_type>"<%voip_e8c_get("h248_encoding_type");%>"</select></td>
				</tr>
				<tr>
					<td><b id="h248_mg_caption" name="h248_mg_caption">终结点配置</b></td>
				</tr>
				<tr>
					<td>終端配置方式</td>
					<td><select name=h248_phy_term_config_method>"<%voip_e8c_get("h248_phy_term_config_method");%>"</select></td>
				</tr>
				<tr>
					<td>终端</td>
					<td><input name="h248_phy_term_id" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("h248_phy_term_id"); %>"></td>
				</tr>
				<tr>
					<td>终端物理终结点标识前缀</td>
					<td><input name="h248_phy_term_prefix" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("h248_phy_term_prefix"); %>"></td>
				</tr>
				<tr>
					<td>终端物理终结点起始值</td>
					<td><input name="h248_phy_term_start" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("h248_phy_term_start"); %>">(0-32767)</td>
				</tr>
				<tr>
					<td>终端物理终结点标识前缀后面添加位数</td>
					<td><input name="h248_phy_term_len" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("h248_phy_term_len"); %>">(0-15)</td>
				</tr>
				<tr>
					<td>RTP终结点标识前缀</td>
					<td><input name="h248_rtp_term_prefix" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("h248_rtp_term_prefix"); %>"></td>
				</tr>
				<tr>
					<td>RTP终结点标识名起始值</td>
					<td><input name="h248_rtp_term_start" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("h248_rtp_term_start"); %>">(0-15)</td>
				</tr>
				<tr>
					<td>RTP终结点标识的前缀后面添加的数字部分的位数</td>
					<td><input name="h248_rtp_term_len" size="16" maxlength="32" style="width:140px" value="<% voip_e8c_get("h248_rtp_term_len"); %>">(0-15)</td>
				</tr>
			</table>
			<br>
				<% voip_e8c_get("h248_account"); %>
			<br>
</div> <!-- h248_display -->	
			<table   border="0" cellpadding="0" cellspacing="0" width="700">
				<tr>
					<td align="center">
						<button class="btnsaveup" name="btnDown" onclick="on_submit()">应用</button>
						<button class="BtnCnl" type="reset">取消</button>
						<input type="hidden" name="submit-url" value="">
					</td>
				</tr>
			</table>	
			</form>
			
<div id = siptable <%voip_e8c_get("sip_display");%>>
			<form id="form_codec" action=/boaform/admin/voip_cmcc_codec_set method=POST>
			<br><b>VoIP语音编码配置</b><br>
			<table id="codec_config" name="codec_config"  border="3" cellpadding="0" cellspacing="0" width="700">
				<tr>
					<td>编解码方式</td>
					<td>使能</td>
					<td>打包时长(ms)</td>
					<td>优先级(1-5)</td>
				</tr>
				<% voip_e8c_get("cmcc_codec"); %>
			</table>
			<script language=javascript>checkbox_gray()</script>
			
			<table   border="0" cellpadding="0" cellspacing="0" width="700">
				<tr>
					<td align="center">
						<button class="btnsaveup" name="btnDown" onclick="on_codec_submit()">应用</button>
						<button class="BtnCnl" type="reset">取消</button>
						<input type="hidden" name="submit-url" value="">
					</td>
				</tr>
			</table>	
			</form>			
			
			<form id="form_account_table" action=/boaform/admin/voip_cmcc_account_del method=POST>
			<br>
			<b>用戶基本參數</b><br>
			<table   border="0" cellpadding="0" cellspacing="0" width="700">
				<tr>
					<td align="center">
						<input id="btn_account_add" type="button" class="btnaddup" name="btnDown" value="新建" onClick="on_account_add()">
						<input id="btn_account_del" type="button" class="BtnDel" name="btnDown" value="刪除" onClick="on_account_del()">
						<input type="hidden" name="submit-url" value="">
					</td>
				</tr>
			</table>
			<table   border="1" cellpadding="0" cellspacing="0" width="700">
				<% voip_e8c_get("cmcc_port_account_table"); %>
			</table>
			</form>
			
			<div id=form_account_add style="display:none">
				<form id="form_account" action=/boaform/admin/voip_cmcc_account_set method=POST>
				<br>
				<b>新建用户</b><br>
					<table   border="0" cellpadding="0" cellspacing="0" width="700">
						<% voip_e8c_get("cmcc_port_account"); %>
						<tr>
							<td align="center">
								<button class="btnsaveup" name="btnDown" onclick="on_account_submit()">应用</button>
								<button class="BtnCnl" type="reset">取消</button>
								<input type="hidden" name="submit-url" value="">
							</td>
						</tr>
					</table>
				</form>
			</div>
</div>	<!-- sip_display -->
		</DIV>
	</blockquote>
</body>
<%addHttpNoCache();%>
</html>
