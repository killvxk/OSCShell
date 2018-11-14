20181114 release notes
1. 因意外断开会话导致录像不能正常结束，录制的内容无法播放
2. 会话审计中部分会话记录没有操作按钮
3. 大众Oracle 12i 版本问题修复

修改说明：
一、代理程序修改了如下程序
OracleProxy下的oracleproxy.c 
DBRecord下的dbrecord.c 
解决问题：当用户PL/SQL单点登录时，阻止切换用户

二、OSCShellApp 修改了如下程序：
oscshell2012 - NOPASSWD\OSCShell\下的OSCShell.cpp，
修改内容：修改了PLSQL或SQLPLUS 登录oracle资源，之前修改tnsname.ora文件是采用覆盖的方式，即下一个用户单点登录oracle资源时，会覆盖掉上次用户修改的tnsname.ora，现在改成追加修改tnsname.ora文件
解决问题：修复了当用户通过PL/SQL访问Oracle资源时，经常断线的问题。


注：其中在配置文件mysql.cfg中增加了配置项:stopchangeuser:=0，为1是开启该功能，为0时关闭该功能。