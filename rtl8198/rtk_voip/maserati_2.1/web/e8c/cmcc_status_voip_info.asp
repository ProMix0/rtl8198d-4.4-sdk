<!-- add by liuxiao 2008-01-16 -->
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<html>
<head>
<title>中国电信—我的E家</title>
<meta http-equiv=pragma content=no-cache>
<meta http-equiv=cache-control content="no-cache, must-revalidate">
<meta http-equiv=content-type content="text/html; charset=utf-8">
<style type=text/css>
@import url(/style/default.css);
</style>
<script language="javascript" src="common.js"></script>
</head>
<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000">
	<blockquote>
		<tr>
			<table border="0" cellpadding="0" cellspacing="0" width="100%">
				<tr>
					<td width="10">&nbsp;</td>
					<td>
						<p><b>业务注册状态</b></p>
						<table class="tblList" border="1" cellpadding="1" cellspacing="1">
							<tr>
								<td class="table_title" width="20%">用户编号:</td>
								<td class="table_title" width="60%">用户注册状态:</td>
							</tr>
							<tr>
								<td>1</td>
								<% voip_e8c_get("registerStatus"); %>
							</tr>
<!--							
							<tr>
								<td>2</td>
								<% voip_e8c_get("registerStatus_2"); %>
							</tr>
-->							
						</table>
					</td>
					<td width="10">&nbsp;</td>
				</tr>
				<tr>
					<td width="10">&nbsp;</td>
					<td>
						<p><b>电话号码</b></p>
						<table class="tblList" border="1" cellpadding="1" cellspacing="1" width="100%">
							<tr>
								<td class="table_title" width="20%">编号:</td>
								<td class="table_title" width="60%">用户名(电话号码)</td>
							</tr>
							<% voip_e8c_get("cmcc_sip_number"); %>			
						</table>
					</td>
					<td width="10">&nbsp;</td>
				</tr>
			</table>
		</tr>		
	</blockquote>
</body>
<%addHttpNoCache();%>
</html>
