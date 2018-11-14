#include "mysql.h" 
#include "db_table.h"
#include "OleDbAccess.h"

#pragma	  warning(disable:4996)

class db_mysql
{
private:
	COleDbAccess *OleDb;
	MYSQL *mysql;
	STRUCT_DBCONNECT *dbconn;
	static int writeData(unsigned char *str,int len);
	int DBQueryAuthAccessLimit(void *handle);
	void preSqlStr(char *sour,int len);
	int autocommit(bool flag);
	int commit();
	int rollback();
	bool isMySql;

	static void TransTimeAuth(char *name,char *value,void *handle);
	static void TransProxyHostTime(char *name,char *value,void *handle);
	static void TransAuthAccessRes(char *name,char *value,void *handle);
	static void TransAuthAccessLimit(char *name,char *value,void *handle);
	static void TransDBQueryKey(char *name,char *value,void *handle);
	static void TransDBQueryLoginlogId(char *name,char *value,void *handle);
	static void TransDBQueryLocalLang(char *name,char *value,void *handle);
	static void TransDBQueryWebOnline(char *name,char *value,void *handle);
	static void TransDBQueryResPR(char *name,char *value,void *handle);
	static void TransDBQueryPort(char *name,char *value,void *handle);
	static void TransDBQueryDBName(char *name,char *value,void *handle);
	static void TransDBQueryResName(char *name,char *value,void *handle);
public:
	int conndb();
	db_mysql(STRUCT_DBCONNECT *conn);
	int DBQueryAuthAccessRes(void *handle);
	int DBExecAuthAccessRes(void *handle,int flag);
	void writeAuthData(char *authFile,void *handle);	
	int DBExecLoginLogStatus(char *id,int status);
	int DBExecLoginLog(void *handle,int flag, char *proxyTime, char *outDateTime);
	int DBExecUserActivityVideoLog(void *handle);
	int DBQueryTimeAuth(void *handle,char *id);
	int DBQueryProxyHostTime(char *proxyTime,char *id);
	int DBExecSetPort(char *proxyAddr,int port, char * authaccessresid);
	int DBQueryLoginlogId(char *id,char * authaccessresid);
	int DBQueryKey(char * pr_key,char * pr_basekey);
	int DBQueryWebOnline(char * websessionid,PWinSessionId windowssessionid);
	int DBDeleteAuthAccessRes(char * websessionid);
	int DBExecSessionID(char * authaccessresid,char * windowssessionid);
	int DBQueryLocalLang(char *localLang,char * authaccessresid);
	int DBCheckLoginLog(int nodeFlag, char *proxyTime);
	int DBDBQueryAuthAccessResPR(char * authaccessresid,char *pr_cooperator,char *pr_accessMemo);
	int DBInsertAuthMessage(char *authaccessresid, char *uid, char *userName, char *resAddr, char *accountNo, char *clientAddr, char *message);
	int GetResourcePort(char * pr_resId,int pr_code,int *port);
	int GetResourceDBName(char * pr_accountId,char *dbname);
	void getResNamebyResType(char* apId,char* resName);
	int db_mysql::GetServiceName(char * pr_raId,char *pr_resId, char *serviceName);

public:
	~db_mysql(void);
};
