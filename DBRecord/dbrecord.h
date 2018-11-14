/* 
 * File:   dbrecord.h
 * Author: Administrator
 *
 * Created on 2012年2月9日, 上午11:05
 */

#ifndef DBRECORD_H
#define	DBRECORD_H

#include "../ODBCManage/DBTable.h"


#define BUFSIZE 10240
#define DIR_MAXDEPTH 32


extern int GetConfig(const char * username, PPROXYINFO pProxyInfo, int protocol);
extern int InsertUseractivityoutlog(PPROXYINFO pProxyInfo);
extern int InsertUseractivitylog(PPROXYINFO pProxyInfo);
extern int UserLogout(PPROXYINFO pProxyInfo);
extern int GetAccessProtocol(int pr_code,PPr_AccessProtocol pAccessProtocol);
extern int findSQL(char * data, int len,char * sql);
//2016-07-14 add
extern int InsertStopExchangeUserInfo(PPROXYINFO pProxyInfo);
extern int GetStopExchangeUserSwitch(char isStop[]);
//2016-07-14
#endif	/* DBRECORD_H */

