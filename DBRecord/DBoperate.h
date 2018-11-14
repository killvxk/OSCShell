#ifndef DBOPERATE_H
#define	DBOPERATE_H


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <regex.h>
#include <memory.h>
#include <malloc.h>
#include <time.h>
#include <mysql.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h>
//#include "DBTable.h"
//#include "../../liunxodbcmanger/odbcmanage.h"
#include "../ODBCManage/odbcmanage.h"
#include "../ODBCManage/DBTable.h"
#define BUFFSIZE        		1024
#define NUM_SUBSTR_MATCH    10
#define ERR_BUF_LEN    			128    
#define BUF_LEN        			1024  

#define MysqlConfigPath		"/etc/mysql.cfg"
#define MysqlAuditConfigPath		"/etc/mysql_cache.cfg"


#define DBCFG_SERVERIP		"serverip:="
#define DBCFG_PORT                                 "port:="
#define DBCFG_USERNAME		"username:="
#define DBCFG_USERPWD		"userpwd:="
#define DBCFG_DATABASE		"database:="
#define DBCFG_DBTYPE		"dbtype:="


int printdblog( const char* format, ...);
int ConnectDB(char * serverip, int port, char * username, char * userpwd, char * database);
int ConnectMysql();
int GetMysqlConfig();
void CloseDB();
int GetAuth_id(char* pr_id,PPr_AuthAccessRes pAuth);
int GetAuth_uid(char* pr_uid,char* pr_resAddr,char* pr_accountNo,PPr_AuthAccessRes pAuth, int pr_code);
int SetAuth(PPr_AuthAccessRes pAuth);
int CheckEmployee(char * username, char * password) ;
int GetEmployee(char * pr_id,PPr_Employee pEmployee);
int GetResource(char * pr_id, char * accountNo,PPr_Resource pResource);
int GetEmployeeaccount(char* pr_id,PPr_Employeeaccount pEmployeeaccount);
int GetJOINTable(char * pr_uid, char * pr_address, char * pr_userName, int pr_code, PJOINTableID pTableID,int * table_count);
int GetPolicy(PJOINTableID pTableID,PPolicy pPolicy,int* isResource);
int MatchNclItem(PPolicy pPolicy,char * Ip);
int MatchPclItem(PPolicy pPolicy,int code);
int MatchCclItem(PPolicy pPolicy,char * cmd,int type);
int MatchTclItem(PPolicy pPolicy);
int SetLoginlog(Pr_Loginlog loginlog, char * id);
int ReflashLoginlog(char * loginlog_id,int type);
int SetUseractivitylog(Pr_Useractivitylog userlog,char * userlog_id);
int SetUseractivityoutlog(Pr_Useractivityoutlog useroutlog,int indb);
int SaveDB2File(char * sql);
int EncodeData(char * src, char * dest);
int ClearData(char * src, char * dest);
int GetProtocolProt(char * pr_code,int * pr_port);
int GetResaccountCount(char * pr_resId,char * pr_userId,int * length);
int GetResaccount(char * pr_resId,char * pr_userId,PPr_Resaccount pResaccount);

int GetConfigForUser(PPROXYINFO pProxyInfo,int pr_accessType);
int GetConfigForID(PPROXYINFO pProxyInfo,int pr_accessType);
int CreateLoginlog(PPROXYINFO pProxyInfo);
int GetPrAccessProtocol(int pr_code,PPr_AccessProtocol pAccessProtocol);
int GetResourcePort(char * pr_resId,int pr_code,int *port);
int GetLeveCmd(PPROXYINFO pProxyInfo);
int FindLevelCmd(PPROXYINFO pProxyInfo,char *cmd);
int SetAuthMessage(char *pr_id, char *uid, char *userName, char *resAddr, char *accountNo, char *clientAddr, char *message);
int GetAESKey();
int DecryptData(char * indata, int len, char *outdata);
#endif	/* DBOPERATE_H */

