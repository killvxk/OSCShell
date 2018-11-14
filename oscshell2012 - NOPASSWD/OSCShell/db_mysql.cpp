//#include <Windows.h>
#include <winsock.h> 
#include <stdio.h>
#include "db_mysql.h"
#include <atltime.h>
#include "LogMan.h"
extern "C"
{
#include "syslog.h"
};
//#include<string>
//#include <vector>
//using namespace std; 
/*
#define backfunc void (*func)(char *field,char *value,void *handle)
extern "C" __declspec(dllimport) bool __stdcall oleDb_Open(char *dbtype,char *address,char *dbname,int port,char *dbuser,char *dbpwd);
extern "C" __declspec(dllimport) bool __stdcall oleDb_Query(char *sqlStr,char* fields[],backfunc,void *handle,int *recordCount);
extern "C" __declspec(dllimport) bool __stdcall oleDb_OneUpdate(char *sqlStr);
extern "C" __declspec(dllimport) bool __stdcall oleDb_Update(char *sqlStr,bool isTrans);
extern "C" __declspec(dllimport) bool __stdcall oleDb_UpdateArray(char *sqlStrs[],bool isTrans);
extern "C" __declspec(dllimport) void __stdcall oleDb_GetError(char *errMsg);
extern "C" __declspec(dllimport) void __stdcall oleDb_Close();
extern "C" __declspec(dllimport) void __stdcall beginTrans();
extern "C" __declspec(dllimport) void __stdcall commitTrans();
extern "C" __declspec(dllimport) void __stdcall rollbackTrans();
*/

FILE  *stream=NULL;
//extern CRITICAL_SECTION mysqlcsA;
db_mysql::db_mysql(STRUCT_DBCONNECT *dbconn)
{
	//InitializeCriticalSection(&mysqlcsA);
	isMySql=false;
   if(strlen(dbconn->dbType)==0|| strcmp(dbconn->dbType,"MYSQL")==0)
   {
	    isMySql=true;
        mysql=mysql_init(0);
   }
   this->dbconn=dbconn;
   stream=NULL;
   OleDb=NULL;
}

db_mysql::~db_mysql(void)
{
    //DeleteCriticalSection(&mysqlcsA);
   if(!isMySql)
   {
      if(OleDb!=NULL)
	  {
         OleDb->oleDb_Close();
	     delete OleDb;
	  }
	  return;
   }
   mysql_close(mysql);
}

int db_mysql::DBQueryAuthAccessRes(void *handle)
{
	return DBExecAuthAccessRes(handle,0);
}

int db_mysql::conndb()
{
	bool ret=false;
    if(!isMySql)
    {
		OleDb=new COleDbAccess();
		ret=OleDb->oleDb_Open(dbconn->dbType,dbconn->dbHost,dbconn->dbName,dbconn->dbPort,dbconn->dbUser,dbconn->dbPwd);  
        if(ret)
		{
			return 0;
		}
		
		CLogMan::error_log("connect OLE DB failure dbType[%s],dbHost[%s],dbName[%s],dbPort[%d],dbUser[%s],dbPwd[%s]",dbconn->dbType,dbconn->dbHost,dbconn->dbName,dbconn->dbPort,dbconn->dbUser,dbconn->dbPwd);
		CLogMan::error_log("Ole Error:%s",OleDb->oleDb_GetError());
		return -1;
    }
    if(!mysql_real_connect(mysql,dbconn->dbHost,dbconn->dbUser,dbconn->dbPwd,dbconn->dbName,dbconn->dbPort,NULL,0)){
		CLogMan::error_log("connect MySql failure dbType[%s],dbHost[%s],dbName[%s],dbPort[%d],dbUser[%s],dbPwd[%s]",dbconn->dbType,dbconn->dbHost,dbconn->dbName,dbconn->dbPort,dbconn->dbUser,dbconn->dbPwd);
		CLogMan::error_log("mysql_error:%s",mysql_error(mysql));
        return -1;
    }
    mysql_set_character_set(mysql,"utf8");	
	mysql_query(mysql,"set names 'gbk'");
	return 0;
}

int db_mysql::autocommit(bool flag)
{
	char cmdSql[50];
	memset(cmdSql,0,sizeof(cmdSql));
	if(!flag)
	{
		strcpy(cmdSql,"SET AUTOCOMMIT = 0;");
	}else
	{
        strcpy(cmdSql,"SET AUTOCOMMIT = 1;");
	}
    return mysql_real_query(mysql,cmdSql,strlen(cmdSql));
}

int db_mysql::commit()
{
	char cmdSql[50];
	memset(cmdSql,0,sizeof(cmdSql));
	strcpy(cmdSql,"COMMIT;");
    return mysql_real_query(mysql,cmdSql,strlen(cmdSql));
}

int db_mysql::rollback()
{
	char cmdSql[50];
	memset(cmdSql,0,sizeof(cmdSql));
	strcpy(cmdSql,"ROLLBACK;");
    return mysql_real_query(mysql,cmdSql,strlen(cmdSql));
}


void db_mysql::TransTimeAuth(char *name,char *value,void *handle)
{
	if(value==NULL)
	{
		return;
	}
    STRUCT_TIMEAUTH *tauth=(STRUCT_TIMEAUTH *)handle;
    if(strcmp("pr_endTime",name)==0)
	{
		strcpy(tauth->endTime,value);
	}else
    if(strcmp("pr_resetPwdTime",name)==0)
	{
  	   strcpy(tauth->resetPwdTime,value);
	}else
    if(strcmp("pr_timeFlag",name)==0)
	{
		strcpy(tauth->timeFlag,value);
	}
}

void db_mysql::TransProxyHostTime(char *name,char *value,void *handle)
{
	if (value == NULL)
	{
		return;
	}
	char *proxyHostTime = (char *)handle;
	if (strcmp("pr_updateTime", name) == 0)
	{
		strcpy(proxyHostTime, value);
	}
}

int db_mysql::DBQueryTimeAuth(void *handle,char *id)
{
    MYSQL_ROW row;  
	int  recordcount=0;
	if(handle==NULL){
	   return -1;
	}
    char sqlStr[2048]={0};
    sprintf(sqlStr,"select pr_endTime,pr_resetPwdTime,pr_timeFlag from pr_AuthAccessRes where pr_id='%s'",id);	
	CLogMan::debug_log("DBQueryTimeAuth:%s",sqlStr);

	if(!isMySql)
	{
	    char *fields[]={"pr_endTime","pr_resetPwdTime","pr_timeFlag",NULL};
		bool ret=OleDb->oleDb_Query(sqlStr,fields,TransTimeAuth,handle,&recordcount);
		if(!ret)
		{
			CLogMan::error_log("query failure [%s]",sqlStr);
			CLogMan::error_log("Ole Error:%s",OleDb->oleDb_GetError());
			return -1;
		}
		return recordcount;

	}
	STRUCT_TIMEAUTH *tauth=(STRUCT_TIMEAUTH *)handle;

	if(mysql_real_query(mysql,sqlStr,strlen(sqlStr))!=0){
		CLogMan::error_log("query failure [%s]",sqlStr);
		return -1;
	}

	//保存查询结果
	MYSQL_RES * res = mysql_store_result(mysql);
	//得到记录数量
	recordcount = (int)mysql_num_rows(res) ; 
	
	if(recordcount>0){      
		while(row = mysql_fetch_row(res))  //依次读取各条记录
		{  
		   int j = mysql_num_fields( res ) ;  
		   for (int k = 0 ; k < j ; k++){
                if(row[k]!=NULL && strlen(row[k])>0)
				{
                   if(k==0)
				   {
					   strcpy(tauth->endTime,row[k]);
				   }else
				   if(k==1)
				   {
					   strcpy(tauth->resetPwdTime,row[k]);
				   }else
				   if(k==2)
				   {
					   strcpy(tauth->timeFlag,row[k]);
				   }
				}
		   }
		   break;
		}  
	}
	mysql_free_result(res ) ;
    return recordcount;
}

int db_mysql::DBQueryProxyHostTime(char *proxyTime,char *id)
{
	MYSQL_ROW row;
	int recordCount = 0;
	if (proxyTime == NULL)
	{
		return -1;
	}
	char sqlStr[1024]={0};
	char checkSql[1024]={0};
	sprintf(checkSql, "select * from pr_authaccessres where pr_id = '%s'",id);
	sprintf(sqlStr, "select pr_updateTime from pr_proxy_status where pr_hostid = (select pr_id from pr_proxyhost where pr_address = (select pr_proxyhost from pr_authaccessres where pr_id = '%s'))", id);
	CLogMan::debug_log("DBQueryProxyHostTime: %s",sqlStr);
	
	if (!isMySql)
	{
		char *fileds[] = {"pr_updateTime",NULL};
		bool rtn = OleDb->oleDb_Query(sqlStr, fileds, TransProxyHostTime, (void *)proxyTime, &recordCount);
		if (!rtn)
		{
			CLogMan::error_log("query failure [%s]", sqlStr);
			CLogMan::error_log("Ole Error:%s",OleDb->oleDb_GetError());
		}
		return recordCount;
	}
	//2016-09-13增加 添加检查pr_id 在表pr_authaccessres是否存
	if(mysql_real_query(mysql,checkSql,strlen(checkSql))!=0){
		CLogMan::error_log("query failure [%s]",checkSql);
		return -1;
	}
	//保存查询结果
	MYSQL_RES *res = mysql_store_result(mysql);
	//记录数量
	recordCount = (int)mysql_num_rows(res);
	if (recordCount == 0)
	{
		CLogMan::error_log("query failure pr_id:%s 在表pr_authaccessres不存在",id);
		return -1;
	}
	//2016-09-13增加


	if(mysql_real_query(mysql,sqlStr,strlen(sqlStr))!=0){
		CLogMan::error_log("query failure [%s]",sqlStr);
		return -1;
	}
	//保存查询结果
	res = mysql_store_result(mysql);
	//记录数量
	recordCount = (int)mysql_num_rows(res);
	if (recordCount > 0)
	{
		row = mysql_fetch_row(res);
		strcpy(proxyTime, row[0]);
	}
	mysql_free_result(res);
	return recordCount;
}

void db_mysql::TransAuthAccessRes(char *name,char *value,void *handle)
{
	int i=0,offset=0;
	if(value==NULL)
	{
		return;
	}
	STRUCT_AUTHACCESSRES *aares=(STRUCT_AUTHACCESSRES *)handle;
	STRUCT_ACCESSAUTH *access=&aares->accessAuth;
unsigned int fieldsLen[]={sizeof(access->id),sizeof(access->userId),sizeof(access->uid),sizeof(access->userName),sizeof(access->resId),\
sizeof(access->resAddr),sizeof(access->resName),sizeof(access->resType),sizeof(access->resVersion),sizeof(access->accountId),\
sizeof(access->accountNo),sizeof(access->accountType),sizeof(access->accountPwd),sizeof(access->suFlag),sizeof(access->ssoFlag),\
sizeof(access->auditFlag),sizeof(access->auditType),sizeof(access->cmdFlag),sizeof(access->clientAddr),sizeof(access->apId),sizeof(access->accessType),\
sizeof(access->accountNoEx),sizeof(access->accountPwdEx),sizeof(access->domainName),sizeof(access->screenWidth),\
sizeof(access->screenHeight),sizeof(access->proxyAddr),sizeof(aares->timeAuth.timeFlag),sizeof(aares->timeAuth.startTime),\
sizeof(aares->timeAuth.endTime),sizeof(aares->timeAuth.resetPwdTime),sizeof(aares->execApp),sizeof(aares->execParam),sizeof(aares->proxyHost),sizeof(aares->proxyPort)};
char *fields[]={"pr_id","pr_userId","pr_uid","pr_userName","pr_resId","pr_resAddr","pr_resName","pr_resType","pr_resVersion",
	"pr_accountId","pr_accountNo","pr_accountType","pr_accountPwd","pr_suFlag","pr_ssoFlag","pr_auditFlag","pr_auditType",
	"pr_cmdFlag","pr_clientAddr","pr_apId","pr_accessType","pr_accountNoEx","pr_accountPwdEx","pr_domainName","pr_screenWidth",
	"pr_screenHeight","pr_proxyAddr","pr_timeFlag","pr_startTime","pr_endTime","pr_resetPwdTime","pr_execApp","pr_execParam","pr_proxyHost","pr_proxyPort",NULL};
	
    char *op=aares->accessAuth.id;
	while(fields[i]!=NULL)
	{
		if(strcmp(fields[i],name)==0)
		{
			strcpy((char *)(op+offset),value);            
			break;
		}
		offset += fieldsLen[i];
		i++;
	}
}
void db_mysql::TransDBQueryResPR(char *name,char *value,void *handle)
{
	if(value==NULL)
	{
		return;
	}
	ResPR *resPR= (ResPR*)handle;
	if(strcmp("pr_cooperator",name)==0)
	{
		strcpy(resPR->pr_cooperator,value);
	}else if(strcmp("pr_accessMemo",name)==0){
		strcpy(resPR->pr_accessMemo,value);
	}
}
int db_mysql::DBDBQueryAuthAccessResPR(char * authaccessresid,char *pr_cooperator,char *pr_accessMemo){
	int recordNum=0;
	MYSQL_ROW row;  

	char sqlStr[2048]={0};


	sprintf(sqlStr,"select pr_cooperator,pr_accessMemo from pr_authaccessres where pr_id='%s'",authaccessresid);
	CLogMan::debug_log("DBDBQueryAuthAccessResPR:%s",sqlStr);

	if(!isMySql)   //oracle or db2
	{
		int count = 0;
		char *fields[]={"pr_cooperator","pr_accessMemo",NULL};
		while(count < 30){
			ResPR resPR;
			memset(&resPR,0,sizeof(resPR));
			bool ret=OleDb->oleDb_Query(sqlStr,fields,TransDBQueryResPR,(void *)&resPR,&recordNum);
			if(!ret)
			{
				CLogMan::error_log("query failure [%s]",sqlStr);
				CLogMan::error_log("Ole Error:%s",OleDb->oleDb_GetError());
				return -1;
			}
			if (recordNum>0)
			{
				strcpy(pr_cooperator,resPR.pr_cooperator);
				strcpy(pr_accessMemo,resPR.pr_accessMemo);
				break;
			}
			count ++;
		}
		return 0;		

	}

	if(mysql_real_query(mysql,sqlStr,strlen(sqlStr))!=0){
		CLogMan::error_log("query failure [%s]",sqlStr);
		return -1;
	}

	//保存查询结果
	MYSQL_RES * res = mysql_store_result(mysql);
	//得到记录数量
	recordNum = (int)mysql_num_rows(res) ; 
	if (0 < recordNum && (row = mysql_fetch_row(res))) {

		if(row[0]!=NULL)
			strcpy(pr_cooperator,row[0]);
		if(row[1]!=NULL)
			strcpy(pr_accessMemo,row[1]);
		
	}
	mysql_free_result(res) ;
	return 0;

}

int db_mysql::DBInsertAuthMessage(char *authaccessresid, char *uid, char *userName, char *resAddr, char *accountNo, char *clientAddr, char *message)
{
	char sqlStr[2048]={0};
	memset(sqlStr,0,sizeof(sqlStr));
	sprintf(sqlStr, "insert into pr_authmessage(pr_id,pr_uid,pr_userName,pr_resAddr,pr_accountNo,pr_clientAddr,pr_logInfo) \
					 values(\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\')",authaccessresid, uid, userName, resAddr, accountNo, clientAddr, message);
	if(!isMySql)   //oracle or db2
	{
		bool ret=OleDb->oleDb_OneUpdate(sqlStr);
		if(!ret)
		{
			CLogMan::error_log("update failure [%s]",sqlStr);		
			CLogMan::error_log("Ole Error:%s",OleDb->oleDb_GetError());
			return -1;
		}
		return 0;
	}
	return mysql_real_query(mysql,sqlStr,strlen(sqlStr));
}

int db_mysql::DBExecAuthAccessRes(void *handle,int flag)
{
	int  recordcount=0;
    MYSQL_ROW row;  
//  vector<string> m_name;//字段名称
//  vector<enum_field_types> m_type;//字段类型
	
	if(handle==NULL){
	   return -1;
	}
	STRUCT_AUTHACCESSRES *aares=(STRUCT_AUTHACCESSRES *)handle;
	STRUCT_ACCESSAUTH *access=&aares->accessAuth;
    char sqlStr[2048]={0};

char *selectStr="pr_id,pr_userId,pr_uid,pr_userName,pr_resId,pr_resAddr,pr_resName,pr_resType,pr_resVersion,pr_accountId,\
pr_accountNo,pr_accountType,pr_accountPwd,pr_suFlag,pr_ssoFlag,pr_auditFlag,pr_auditType,pr_cmdFlag,pr_clientAddr,pr_apId,pr_accessType,\
pr_accountNoEx,pr_accountPwdEx,pr_domainName,pr_screenWidth,pr_screenHeight,pr_proxyAddr,pr_timeFlag,pr_startTime,pr_endTime,pr_resetPwdTime,\
pr_execApp,pr_execParam,pr_proxyHost,pr_proxyPort";

	if(flag==0)
	{
	   sprintf(sqlStr,"select %s from pr_AuthAccessRes where pr_id='%s'",selectStr,access->id);
	}else{
	   sprintf(sqlStr,"delete from pr_AuthAccessRes where pr_id='%s'",access->id);
	}

	if(!isMySql)   //oracle or db2
	{
		bool ret=false;
		if(flag==0)
		{
			char *fields[]={"pr_id","pr_userId","pr_uid","pr_userName","pr_resId","pr_resAddr","pr_resName","pr_resType","pr_resVersion",
				"pr_accountId","pr_accountNo","pr_accountType","pr_accountPwd","pr_suFlag","pr_ssoFlag","pr_auditFlag","pr_auditType",
				"pr_cmdFlag","pr_clientAddr","pr_apId","pr_accessType","pr_accountNoEx","pr_accountPwdEx","pr_domainName","pr_screenWidth",
				"pr_screenHeight","pr_proxyAddr","pr_timeFlag","pr_startTime","pr_endTime","pr_resetPwdTime","pr_execApp","pr_execParam","pr_proxyHost","pr_proxyPort",NULL};
			ret=OleDb->oleDb_Query(sqlStr,fields,TransAuthAccessRes,handle,&recordcount);
			if(!ret)
			{
			    CLogMan::error_log("query authAccessRes failure [%s]",sqlStr);
				CLogMan::error_log("Ole Error:%s",OleDb->oleDb_GetError());
				return -1;
			}
			return recordcount;
		}
		char *sqlDeleteStr[3]={0};
		sqlDeleteStr[0]=(char *)malloc(sizeof(char)*(strlen(sqlStr)+1));
		strcpy(sqlDeleteStr[0],sqlStr);
        
		sprintf(sqlStr,"delete from pr_AuthAccessLimit where pr_arId='%s'",access->id);
		sqlDeleteStr[1]=(char *)malloc(sizeof(char)*(strlen(sqlStr)+1));
		strcpy(sqlDeleteStr[1],sqlStr);

		sqlDeleteStr[2]=NULL;		
		ret=OleDb->oleDb_UpdateArray(sqlDeleteStr,true);

		free(sqlDeleteStr[1]);
		free(sqlDeleteStr[0]);
		if(!ret)
		{
			CLogMan::error_log("delete failure [%s]",sqlStr);
			CLogMan::error_log("Ole Error:%s",OleDb->oleDb_GetError());
			return -1;
		}
		return 0;
	}

	
	if(mysql_real_query(mysql,sqlStr,strlen(sqlStr))!=0){
		CLogMan::error_log("query failure [%s]",sqlStr);
		return -1;
	}
	CLogMan::error_log("query success [%s]",sqlStr);
	if(flag!=0){
        sprintf(sqlStr,"delete from pr_AuthAccessLimit where pr_arId='%s'",access->id);
		if(mysql_real_query(mysql,sqlStr,strlen(sqlStr))!=0){
			CLogMan::error_log("delete  failure [%s]",sqlStr);
			return -1;
		}
		return 0;
	}

unsigned int fieldsLen[]={sizeof(access->id),sizeof(access->userId),sizeof(access->uid),sizeof(access->userName),sizeof(access->resId),\
sizeof(access->resAddr),sizeof(access->resName),sizeof(access->resType),sizeof(access->resVersion),sizeof(access->accountId),\
sizeof(access->accountNo),sizeof(access->accountType),sizeof(access->accountPwd),sizeof(access->suFlag),sizeof(access->ssoFlag),\
sizeof(access->auditFlag),sizeof(access->auditType),sizeof(access->cmdFlag),sizeof(access->clientAddr),sizeof(access->apId),sizeof(access->accessType),\
sizeof(access->accountNoEx),sizeof(access->accountPwdEx),sizeof(access->domainName),sizeof(access->screenWidth),\
sizeof(access->screenHeight),sizeof(access->proxyAddr),sizeof(aares->timeAuth.timeFlag),sizeof(aares->timeAuth.startTime),\
sizeof(aares->timeAuth.endTime),sizeof(aares->timeAuth.resetPwdTime),sizeof(aares->execApp),sizeof(aares->execParam),sizeof(aares->proxyHost),sizeof(aares->proxyPort)};

	//保存查询结果
	MYSQL_RES * res = mysql_store_result(mysql);
	//得到记录数量
	recordcount = (int)mysql_num_rows(res) ; 
	
//	for (int x = 0 ; fd = mysql_fetch_field(res); x++)
//	{
//		m_type.push_back(fd->type);
//	}

    int offset=0;
	char *op=aares->accessAuth.id;
	if(recordcount>0){      
		while(row = mysql_fetch_row(res))  //依次读取各条记录
		{  
		   int j = mysql_num_fields( res ) ;  
		   for (int k = 0 ; k < j ; k++){
                if(row[k]!=NULL && strlen(row[k])>0)
				{
			      strcpy((char *)(op+offset),row[k]);
				}
				else{
					// 如果字段内容为空，初始化结构体字段
					memset((char *)(op+offset),0x00,fieldsLen[k]);
				}
				offset += fieldsLen[k];
		   }
		   break;
		}  
	}
	CLogMan::error_log("query res account no [%s]",access->accountNo);
	CLogMan::error_log("query res account no ex [%s]",access->accountNoEx);
	mysql_free_result(res ) ;
    return recordcount;
}

void db_mysql::TransAuthAccessLimit(char *name,char *value,void *handle)
{
	if(value==NULL)
	{
		return;
	}
	STRUCT_AUTHACCESSLIMIT *aalimit=(STRUCT_AUTHACCESSLIMIT *)handle;   

	if(strcmp("pr_command",name)==0)
	{
	   strcpy(aalimit->command,value);
	}else
	if(strcmp("pr_cmdFlag",name)==0)
	{
		aalimit->cmdFlag=atoi(value);

	}else
	if(strcmp("pr_level",name)==0)   //last field
	{
		aalimit->level=atoi(value);

		int  j=sizeof(STRUCT_AUTHACCESSLIMIT)-sizeof(aalimit->command)+strlen(aalimit->command);
        writeData((unsigned char *)&j,sizeof(int));  //命令长度
        writeData((unsigned char *)aalimit,j);      //命令结构
		memset(aalimit,0,sizeof(STRUCT_AUTHACCESSLIMIT));
	}

}
int db_mysql::DBQueryAuthAccessLimit(void *handle)
{
	int recordNum=0;
    MYSQL_ROW row;  

 //   vector<vector<string>> m_record;
 //   vector<enum_field_types> m_type;//字段类型
	
	if(handle==NULL){
	   return -1;
	}

	STRUCT_AUTHACCESSRES *aares=(STRUCT_AUTHACCESSRES *)handle;   
    STRUCT_ACCESSAUTH *access=&aares->accessAuth;

	writeData((unsigned char *)access,sizeof(STRUCT_ACCESSAUTH));

    char sqlStr[2048]={0};
	char *selectStr="pr_command,pr_cmdFlag,pr_level";

	sprintf(sqlStr,"select %s from pr_AuthAccessLimit where pr_arId='%s'",selectStr,access->id);


	if(!isMySql)   //oracle or db2
	{
		    char *fields[]={"pr_command","pr_cmdFlag","pr_level",NULL};
		    STRUCT_AUTHACCESSLIMIT  aalimit;
		    memset(&aalimit,0,sizeof(STRUCT_AUTHACCESSLIMIT));
		    
			bool ret=OleDb->oleDb_Query(sqlStr,fields,TransAuthAccessLimit,&aalimit,&recordNum);
			if(!ret)
			{
			    CLogMan::error_log("query failure [%s]",sqlStr);
				CLogMan::error_log("Ole Error:%s",OleDb->oleDb_GetError());
				return -1;
			}
			return recordNum;		

	}

	if(mysql_real_query(mysql,sqlStr,strlen(sqlStr))!=0){
		 CLogMan::error_log("query failure [%s]",sqlStr);
		return -1;
	}

	//保存查询结果
	MYSQL_RES * res = mysql_store_result(mysql);

	//得到记录数量
	int  recordcount = (int)mysql_num_rows(res) ; 
	
	
   // for (int x = 0 ; fd = mysql_fetch_field(res); x++)
//	{
	//	m_type.push_back(fd->type);
//	}

    recordNum=0;
	if(recordcount>0){      
		while(row = mysql_fetch_row(res))  //依次读取各条记录
		{  
		// vector<string> m_value;
		   STRUCT_AUTHACCESSLIMIT  aalimit;
		   memset(&aalimit,0,sizeof(STRUCT_AUTHACCESSLIMIT));
		   memset(sqlStr,0,sizeof(sqlStr));
		   int j = mysql_num_fields( res ) ;  
		   for (int k = 0 ; k < j ; k++){
               if(row[k]==NULL||strlen(row[k])==0)
			   {
				   continue;
			   }
			   if(k==0)
			   {
				 strcpy(aalimit.command,row[k]);
			   }else
			   if(k==1)
			   {
				 strcpy(sqlStr,row[k]);
				 aalimit.cmdFlag=atoi(sqlStr); 
			   }else			   
			   if(k==2)
			   {
				 strcpy(sqlStr,row[k]);
				 aalimit.level=atoi(sqlStr); 
			   }

		   }
		   if(strlen(aalimit.command)==0)
		   {
			   continue;
		   }

		   j=sizeof(STRUCT_AUTHACCESSLIMIT)-sizeof(aalimit.command)+strlen(aalimit.command);
           writeData((unsigned char *)&j,sizeof(int));  //命令长度
		   writeData((unsigned char *)&aalimit,j);      //命令结构
		   recordNum++;
//		   m_record.push_back(m_value);
		}  
	}
	mysql_free_result(res) ;

    return recordNum;
}

void db_mysql::writeAuthData(char *authFile,void *handle)
{
    if(stream==NULL)
	{
	   stream = fopen(authFile, "wb");
	}    
    
    DBQueryAuthAccessLimit(handle);   

    if(stream)
    {
	 fclose(stream);  
    }
	stream=NULL;
}

int db_mysql::writeData(unsigned char *str,int len)
{
	if(stream)
	{
	   fwrite(str,len,1,stream);
	}
	return 0;
}

int db_mysql::DBExecLoginLogStatus(char *id,int status)
{
	char sqlStr[2048]={0};
	memset(sqlStr,0,sizeof(sqlStr));
    sprintf(sqlStr,"update Pr_LoginLog set pr_status=%d where pr_id='%s'",status,id);
	if(!isMySql)   //oracle or db2
	{
		bool ret=OleDb->oleDb_OneUpdate(sqlStr);
		if(!ret)
		{
		    CLogMan::error_log("update failure [%s]",sqlStr);		
			CLogMan::error_log("Ole Error:%s",OleDb->oleDb_GetError());
			return -1;
		}
		return 0;
	}
    return mysql_real_query(mysql,sqlStr,strlen(sqlStr));
}
void db_mysql::TransDBQueryKey(char *name,char *value,void *handle)
{
	if(value==NULL)
	{
		return;
	}
	AESKEY *aeskey = (AESKEY*)handle;
	if(strcmp("pr_key",name)==0)
	{
		strcpy(aeskey->pr_key,value);
	}else if(strcmp("pr_baseKey",name)==0){
		strcpy(aeskey->pr_basekey,value);
	}
}
int db_mysql::DBQueryKey(char * pr_key,char * pr_basekey)
{
	int recordNum=0;
	MYSQL_ROW row;  
	int count = 0;

	char sqlStr[2048]={0};


	sprintf(sqlStr,"SELECT pr_key,pr_baseKey from pr_productkey");
	CLogMan::debug_log("DBQueryLoginlogId:%s",sqlStr);

	if(!isMySql)   //oracle or db2
	{
		char *fields[]={"pr_key","pr_baseKey",NULL};
		while(count < 30){
			AESKEY aeskey;
			memset(&aeskey,0,sizeof(aeskey));
			bool ret=OleDb->oleDb_Query(sqlStr,fields,TransDBQueryKey,(void *)&aeskey,&recordNum);
			if(!ret)
			{
				CLogMan::error_log("query failure [%s]",sqlStr);
				CLogMan::error_log("Ole Error:%s",OleDb->oleDb_GetError());
				return -1;
			}
			if (recordNum>0)
			{
				strcpy(pr_key,aeskey.pr_key);
				strcpy(pr_basekey,aeskey.pr_basekey);
				break;
			}
			count ++;
		}
		return recordNum;		

	}

	
		CLogMan::debug_log("DBQueryLoginlogId:%d",count);
		if(mysql_real_query(mysql,sqlStr,strlen(sqlStr))!=0){
			CLogMan::error_log("query failure [%s]",sqlStr);
			return -1;
		}

		//保存查询结果
		MYSQL_RES * res = mysql_store_result(mysql);
		//得到记录数量
		recordNum = (int)mysql_num_rows(res) ; 
		if (0 < recordNum && (row = mysql_fetch_row(res))) {


			strcpy(pr_key, row[0]);
			strcpy(pr_basekey, row[1]);
			
		}
		mysql_free_result(res) ;
		
	

	return recordNum;
}
void db_mysql::TransDBQueryWebOnline(char *name,char *value,void *handle)
{
	if(value==NULL)
	{
		return;
	}
	PWinSessionId windowssessionid = (PWinSessionId)handle;
	if(strcmp("pr_windowssessionid",name)==0)
	{
		int i = 0;
		for (i = 0 ; i < 2000 ; i ++)
		{
			if(strlen(windowssessionid[i].sessionid) == 0){
				break;
			}
		}
		strcpy(windowssessionid[i].sessionid,value);
	}
}

int db_mysql::DBQueryWebOnline(char * websessionid,PWinSessionId windowssessionid){
	int recordNum=0;
	MYSQL_ROW row;  
	int count = 0;

	char sqlStr[2048]={0};


	sprintf(sqlStr,"select pr_windowssessionid from pr_authaccessres where pr_websessionid='%s'",websessionid);
	CLogMan::debug_log("DBQueryLoginlogId:%s",sqlStr);

	if(!isMySql)   //oracle or db2
	{
		char *fields[]={"pr_windowssessionid",NULL};
		bool ret=OleDb->oleDb_Query(sqlStr,fields,TransDBQueryWebOnline,(void *)windowssessionid,&recordNum);
		if(!ret)
		{
			CLogMan::error_log("query failure [%s]",sqlStr);
			CLogMan::error_log("Ole Error:%s",OleDb->oleDb_GetError());
			return -1;
		}
		return recordNum;		
	}

	if(mysql_real_query(mysql,sqlStr,strlen(sqlStr))!=0){
		CLogMan::error_log("query failure [%s]",mysql_error(mysql));
		return -1;
	}
	CLogMan::error_log("query success!");
	//保存查询结果
	MYSQL_RES * res = mysql_store_result(mysql);
	//得到记录数量
	recordNum = (int)mysql_num_rows(res) ; 
	CLogMan::error_log("record num[%d]",recordNum);
	if (0 < recordNum){
		while(row = mysql_fetch_row(res)){
			if(row[0] == NULL)
				continue;
			strcpy(windowssessionid[count].sessionid ,row[0]);
			CLogMan::debug_log("DBQueryLoginlogId:id:%s",windowssessionid[count].sessionid);

			count ++;
		}
	}
	mysql_free_result(res) ;

	CLogMan::debug_log("DBQueryLoginlogId:count:%d",count);

	return count;
}
void db_mysql::TransDBQueryLoginlogId(char *name,char *value,void *handle)
{
	if(value==NULL)
	{
		return;
	}
	if(strcmp("pr_id",name)==0)
	{
		strcpy((char*)handle,value);
	}
}

int db_mysql::DBQueryLoginlogId(char *id,char * authaccessresid){
	int recordNum=0;
	MYSQL_ROW row;  
	int count = 0;

	char sqlStr[2048]={0};
	

	sprintf(sqlStr,"select pr_id from Pr_LoginLog where pr_authaccessresid='%s'",authaccessresid);
	CLogMan::debug_log("DBQueryLoginlogId:%s",sqlStr);

	if(!isMySql)   //oracle or db2
	{
		char *fields[]={"pr_id",NULL};
		while(count < 200){
			bool ret=OleDb->oleDb_Query(sqlStr,fields,TransDBQueryLoginlogId,(void *)id,&recordNum);
			if(!ret)
			{
				CLogMan::error_log("query failure [%s]",sqlStr);
				CLogMan::error_log("Ole Error:%s",OleDb->oleDb_GetError());
				return -1;
			}
			if (recordNum>0)
			{
				break;
			}
			count ++;
			Sleep(2000);
		}
		return recordNum;		

	}

	while(count < 200){
		CLogMan::debug_log("DBQueryLoginlogId:%d",count);
		if(mysql_real_query(mysql,sqlStr,strlen(sqlStr))!=0){
			CLogMan::error_log("query failure [%s]",sqlStr);
			return -1;
		}
		
		//保存查询结果
		MYSQL_RES * res = mysql_store_result(mysql);
		//得到记录数量
		recordNum = (int)mysql_num_rows(res) ; 
		if (0 < recordNum && (row = mysql_fetch_row(res))) {
		
			
			strcpy(id, row[0]);
			CLogMan::debug_log("DBQueryLoginlogId loginlogid=%s",id);
			mysql_free_result(res) ;
			break;
		}
		mysql_free_result(res) ;
		count ++;
		Sleep(2000);
	}

	return recordNum;

}

void db_mysql::TransDBQueryLocalLang(char *name,char *value,void *handle)
{
	if(value==NULL)
	{
		return;
	}
	if(strcmp("pr_locallang",name)==0)
	{
		strcpy((char*)handle,value);
	}
}
int db_mysql::DBQueryLocalLang(char *localLang,char * authaccessresid){
	int recordNum=0;
	MYSQL_ROW row;  

	char sqlStr[2048]={0};


	sprintf(sqlStr,"select pr_locallang from pr_authaccessres where pr_id='%s'",authaccessresid);
	CLogMan::debug_log("DBQueryLocalLang:%s",sqlStr);

	if(!isMySql)   //oracle or db2
	{
		char *fields[]={"pr_id",NULL};
		
			bool ret=OleDb->oleDb_Query(sqlStr,fields,TransDBQueryLocalLang,(void *)localLang,&recordNum);
			if(!ret)
			{
				CLogMan::error_log("query failure [%s]",sqlStr);
				CLogMan::error_log("Ole Error:%s",OleDb->oleDb_GetError());
				return -1;
			}
			
		return recordNum;		

	}

	
		if(mysql_real_query(mysql,sqlStr,strlen(sqlStr))!=0){
			CLogMan::error_log("query failure [%s]",sqlStr);
			return -1;
		}

		//保存查询结果
		MYSQL_RES * res = mysql_store_result(mysql);
		//得到记录数量
		recordNum = (int)mysql_num_rows(res) ; 
		if (0 < recordNum && (row = mysql_fetch_row(res))) {


			strcpy(localLang, row[0]);
			CLogMan::debug_log("DBQueryLocalLang pr_locallang=%s",localLang);
		}
		mysql_free_result(res) ;


	

	return recordNum;
}
int db_mysql::DBExecSetPort(char *proxyAddr,int port, char * authaccessresid)
{
	char sqlStr[1024]={0};
	memset(sqlStr,0,sizeof(sqlStr));
    sprintf(sqlStr,"update Pr_LoginLog set pr_proxyAddr='%s',pr_port=%d where pr_authaccessresid='%s'",proxyAddr,port,authaccessresid);
	if(!isMySql)   //oracle or db2
	{
		bool ret=OleDb->oleDb_OneUpdate(sqlStr);
		if(!ret)
		{
			CLogMan::error_log("update failure [%s]",sqlStr);	
			CLogMan::error_log("Ole Error:%s",OleDb->oleDb_GetError());
			return -1;
		}
		return 0;
	}
	CLogMan::debug_log("DBExecSetPort:%s",sqlStr);
	int rtn = 1;
	int count = 0;
	Sleep(5000);
	while (rtn != 0)
	{
		rtn = mysql_real_query(mysql,sqlStr,strlen(sqlStr));
		count ++;
		if (count > 60 || rtn == 0)
		{
			CLogMan::debug_log("update success");	
			break;
		}
		CLogMan::debug_log("update failure [%d]",count);	
		Sleep(1000);
	}
    return 0;

	
}
int db_mysql::DBExecSessionID(char * authaccessresid,char * windowssessionid)
{
	char sqlStr[1024]={0};
	memset(sqlStr,0,sizeof(sqlStr));
	sprintf(sqlStr,"update pr_authaccessres set pr_windowssessionid='%s' where pr_id='%s'",windowssessionid,authaccessresid);
	if(!isMySql)   //oracle or db2
	{
		bool ret=OleDb->oleDb_OneUpdate(sqlStr);
		if(!ret)
		{
			CLogMan::error_log("update failure [%s]",sqlStr);	
			CLogMan::error_log("Ole Error:%s",OleDb->oleDb_GetError());
			return -1;
		}
		return 0;
	}
	CLogMan::debug_log("DBExecSessionID:%s",sqlStr);
	int rtn = 1;
	int count = 0;
	rtn = mysql_real_query(mysql,sqlStr,strlen(sqlStr));
	if (rtn != 0)
	{
		CLogMan::debug_log("update failure [%d]",rtn);	
	}
	return 0;
}
void db_mysql::getResNamebyResType(char* apId,char*resName)
{
	int recordNum=0;
	char sql[1024];
	MYSQL_RES *res;
	MYSQL_ROW row;
	int num;
	int rtn = 0;

	memset(sql, 0, sizeof (sql));
	sprintf(sql,"select pr_name from pr_accessprotocol where pr_id =%s",apId);

	if(!isMySql)   //oracle or db2
	{
		int count = 0;
		char *fields[]={"pr_name",NULL};
		while(count < 30){
			char pr_name[50]={0};
			bool ret=OleDb->oleDb_Query(sql,fields,TransDBQueryResName,(void *)pr_name,&recordNum);
			if(!ret)
			{
				CLogMan::error_log("query failure [%s]",sql);
				CLogMan::error_log("Ole Error:%s",OleDb->oleDb_GetError());
				return;
			}
			if (recordNum>0)
			{
				memcpy(resName,pr_name,strlen(pr_name));
				break;
			}
			count ++;
		}
		return;		

	}

	if (mysql_query(mysql, sql)) {
		CLogMan::error_log("%s[%d]",mysql_error(mysql),__LINE__);
		return;
	}
	if (!(res = mysql_store_result(mysql))) {
		CLogMan::error_log("%s[%d]",mysql_error(mysql),__LINE__);
		return;
	}
	num = mysql_num_rows(res);
	if (0 < num && (row = mysql_fetch_row(res))) {
		strcpy(resName,row[0]);
	} 
	mysql_free_result(res);

}
extern DWORD MByteANSIToMByteUTF8(char *lpcszStr, int len, char* utf8Char);
int db_mysql::DBExecLoginLog(void *handle,int flag, char *proxyTime, char *outDateTime)
{
	char sqlStr[2048]={0};
	memset(sqlStr,0,sizeof(sqlStr));
	if(handle==NULL){
	   return -1;
	}
	STRUCT_LOGINLOG *loginlog=(STRUCT_LOGINLOG *)handle;

	CLogMan::debug_log("[DBExecLoginLog][start]flag=%d[accountNo:%s]",flag,loginlog->accountNo);
	if(loginlog->accountNo != NULL && strlen(loginlog->accountNo) > 0)
	{
		preSqlStr(loginlog->accountNo,strlen(loginlog->accountNo));
	}
	if(flag == 0)  //登录
	{
		char *insertSql="insert into Pr_LoginLog(pr_id,pr_uid,pr_userName,pr_orgId,pr_orgName,pr_accountNo,pr_clientAddr,pr_resAddr,pr_resId,pr_resName,pr_resType,pr_apId,pr_accessType,pr_loginDate,pr_onLineTime,pr_proxyAddr,pr_cmdFlag,pr_status,pr_nodeFlag,pr_authAccessResId,pr_cooperator,pr_accessMemo) ";
		if(strcmp("ORACLE",dbconn->dbType)==0)
		{
			
		sprintf(sqlStr,
			"%s values(\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',%d,to_date(\'%s\','yyyy-mm-dd hh24:mi:ss'),to_date(\'%s\','yyyy-mm-dd hh24:mi:ss'),\'%s\',%d,1,%d,'%s','%s','%s')",
			insertSql,
			loginlog->id,
			loginlog->uid,
			loginlog->userName,
			loginlog->orgId,
			loginlog->orgName,
			loginlog->accountNo,
			loginlog->clientAddr,
			loginlog->resAddr,
			loginlog->resId,
			loginlog->resName,
			loginlog->resType,
			loginlog->apId,
			loginlog->accessType,
			loginlog->loginDate,
			loginlog->loginDate,
			loginlog->proxyAddr,
			loginlog->cmdFlag,
			loginlog->nodeFlag,
			loginlog->authaccessresid,
			loginlog->pr_cooperator,
			loginlog->pr_accessMemo
			);
		}else
		{
			
		sprintf(sqlStr,
			"%s values(\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',%d,\'%s\',\'%s\',\'%s\',%d,1,%d,'%s','%s','%s')",
			insertSql,
			loginlog->id,
			loginlog->uid,
			loginlog->userName,
			loginlog->orgId,
			loginlog->orgName,
			loginlog->accountNo,
			loginlog->clientAddr,
			loginlog->resAddr,
			loginlog->resId,
			loginlog->resName,
			loginlog->resType,
			loginlog->apId,
			loginlog->accessType,
			loginlog->loginDate,
			loginlog->loginDate,
			loginlog->proxyAddr,
			loginlog->cmdFlag,
			loginlog->nodeFlag,
			loginlog->authaccessresid,
			loginlog->pr_cooperator,
			loginlog->pr_accessMemo
			);
		}	
	}
	else if(flag == 1)   //退出
	{
		
		int year = 0,month = 0,day = 0,hour = 0,minutes = 0,second = 0; 
		sscanf(outDateTime, "%04d-%02d-%02d %02d:%02d:%02d", &year, &month, &day, &hour, &minutes, &second);
		/*int day = ctime.GetDay();
		int month = ctime.GetMonth();
		int year = ctime.GetYear();
		int hour = ctime.GetHour() ;
		int minutes = ctime.GetMinute() ;
		int second = ctime.GetSecond() ;*/
		sprintf(loginlog->loginOutDate,"%04d-%02d-%02d %02d:%02d:%02d",year,month,day,hour,minutes,second);
		if(strcmp("ORACLE",dbconn->dbType)==0)
		{
			sprintf(sqlStr,"update Pr_LoginLog set pr_loginOutDate = to_date('%s','yyyy-mm-dd hh24:mi:ss'),pr_status=0  where pr_id='%s'",loginlog->loginOutDate,loginlog->id);
		}else
		{
			sprintf(sqlStr,"update Pr_LoginLog set pr_loginOutDate = '%s',pr_status=0  where pr_id='%s'",loginlog->loginOutDate,loginlog->id);
		}
		//syslog sugl
#ifdef SYSLOG
		char formatStr[2048]={0};
		CLogMan::debug_log("start syslog!");
#ifdef SVW
		char splitChar = ',';//0x05
		sprintf(formatStr,"login%c%s%c%s%c%s%c%s%c%s%c%s%c%s%c%s%c%s",splitChar,loginlog->uid,splitChar,loginlog->userName,splitChar,loginlog->accountNo,splitChar,loginlog->resName,splitChar,loginlog->resAddr,
		splitChar,loginlog->clientAddr,splitChar,loginlog->accessName,splitChar,loginlog->loginDate,splitChar,loginlog->loginOutDate);
		syslog(LOG_USER|LOG_INFO,"%s",formatStr);//大众默认
		CLogMan::debug_log("syslog[%s](%d)",formatStr,strlen(formatStr));	
#endif
#ifdef QINGPU
		char descriptions[1024]={0};
		sprintf(descriptions,"login,用户主账号:%s 用户名称:%s 资源账号:%s 资源名称:%s 资源地址:%s 客户端地址:%s 访问类型:%s 登陆时间:%s 登出时间:%s",loginlog->uid,loginlog->userName,loginlog->accountNo,loginlog->resName,loginlog->resAddr,\
			loginlog->clientAddr,loginlog->accessName,loginlog->loginDate,loginlog->loginOutDate);
		
		sprintf(formatStr,"id=4A time=\"%s\" 4A=\"IT云安全运维平台\" pri=1 type=\"audit info\" description=\"%s\" dip=%s \
dport=\"\"  sdomain=\"\"  sip=%s  sport=\"\"  protol=tcp  info=\"\"  solution=\"\" result=\"succeeded\"",loginlog->loginDate,\
			descriptions,loginlog->resAddr,loginlog->clientAddr);
		char utf8Str[3072]={0};
		MByteANSIToMByteUTF8(formatStr,strlen(formatStr),utf8Str);
		syslog(LOG_CRON|LOG_CRIT,"%s",utf8Str);//青浦
		CLogMan::debug_log("syslog[%s](%d)",utf8Str,strlen(utf8Str));	
#endif
#endif
	}
	else
	if(flag == 2){  //心跳
		//取得当前时间

		//获取数据库中代理机的时间
		int year = 0,month = 0,day = 0,hour = 0,minutes = 0,second = 0; 
		sscanf(proxyTime, "%04d-%02d-%02d %02d:%02d:%02d", &year, &month, &day, &hour, &minutes, &second);

		sprintf(loginlog->onLineTime,"%04d-%02d-%02d %02d:%02d:%02d",year,month,day,hour,minutes,second);

		if(strcmp("ORACLE",dbconn->dbType)==0)
		{
			sprintf(sqlStr,"update Pr_LoginLog set pr_onlineTime = to_date('%s','yyyy-mm-dd hh24:mi:ss') where pr_id='%s'",loginlog->onLineTime,loginlog->id);
		}else
		{
			sprintf(sqlStr,"update Pr_LoginLog set pr_onlineTime = '%s' where pr_id='%s'",loginlog->onLineTime,loginlog->id);
		}
	}

    if(strlen(sqlStr)>0)
	{
  		CLogMan::debug_log("check login[%s]",sqlStr);	
		if(!isMySql)   //oracle or db2
		{
			bool ret=OleDb->oleDb_OneUpdate(sqlStr);
			if(!ret)
			{
				CLogMan::error_log("update failure [%s]",sqlStr);	
				CLogMan::error_log("Ole Error:%s",OleDb->oleDb_GetError());
				return -3;
			}
			return 0;
		}

	   if(mysql_real_query(mysql,sqlStr,strlen(sqlStr))!=0){
		   CLogMan::error_log("update failure [%d]",mysql_error(mysql));	
		   return -3;
	   }

	}
	return 0;
}

void db_mysql::preSqlStr(char *sour,int len)
{	 
     int i=0,j=0;
	 char *op=(char *)malloc(sizeof(char)*len*2);
	 memset(op,0,sizeof(char)*len*2);
	 for(i=0,j=0;i<len;i++)
	 {
        if(sour[i]=='\\')
		{
           op[j++]=sour[i];		   
		}else
		if(sour[i]=='\'')		
		{
           op[j++]='\\';
		   
		}
        op[j++]=sour[i];		
	 }
	 strcpy(sour,op);
	 free(op);
}

int db_mysql::DBExecUserActivityVideoLog(void *handle)
{
    char sqlStr[1024]={0};
	if(handle==NULL){
	   return -1;
	}
	STRUCT_USERACTIVITYVIDEOLOG *uavlog=(STRUCT_USERACTIVITYVIDEOLOG *)handle;

    char *insertSql="insert into pr_userActivityVideoLog(pr_id,pr_resId,pr_resName,pr_resAddr,pr_clientAddr,pr_userId,pr_userName,pr_uid,pr_accountNo,pr_operDate,pr_sessionId,pr_filePath) ";

	// 若 account No 是空值，则不做处理
	if(uavlog->accountNo != NULL && strlen(uavlog->accountNo)>0){
		CLogMan::error_log("user account before: [%s]",uavlog->accountNo);	
		preSqlStr(uavlog->accountNo,strlen(uavlog->accountNo));
	}
	preSqlStr(uavlog->filePath,strlen(uavlog->filePath));
	CLogMan::error_log("user account after: [%s]",uavlog->accountNo);	
	CLogMan::debug_log("DBExecUserActivityVideoLog:%s",insertSql);
	if(strcmp("ORACLE",dbconn->dbType)==0)
	{
	 sprintf(sqlStr,
			"%s values(\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',to_date(\'%s\','yyyy-mm-dd hh24:mi:ss'),\'%s\',\'%s\')",
			insertSql,
			uavlog->id,
			uavlog->resId,
			uavlog->resName,
			uavlog->resAddr,
			uavlog->clientAddr,
			uavlog->userId,
			uavlog->userName,
			uavlog->uid,
			uavlog->accountNo,
			uavlog->operDate,
			uavlog->sessionId,
			uavlog->filePath);      
	}else
	{
	 sprintf(sqlStr,
			"%s values(\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\')",
			insertSql,
			uavlog->id,
			uavlog->resId,
			uavlog->resName,
			uavlog->resAddr,
			uavlog->clientAddr,
			uavlog->userId,
			uavlog->userName,
			uavlog->uid,
			uavlog->accountNo,
			uavlog->operDate,
			uavlog->sessionId,
			uavlog->filePath);
	}
	
	if(!isMySql)   //oracle or db2
	{
		char *sqlDeleteStr[3]={0};
		sqlDeleteStr[0]=(char *)malloc(sizeof(char)*(strlen(sqlStr)+1));
		strcpy(sqlDeleteStr[0],sqlStr);
        
		sprintf(sqlStr,"update Pr_LoginLog set pr_videoFlag = 1 where pr_id='%s'",uavlog->sessionId);
		sqlDeleteStr[1]=(char *)malloc(sizeof(char)*(strlen(sqlStr)+1));
		strcpy(sqlDeleteStr[1],sqlStr);

		sqlDeleteStr[2]=NULL;		
		bool ret=OleDb->oleDb_UpdateArray(sqlDeleteStr,true);

		free(sqlDeleteStr[1]);
		free(sqlDeleteStr[0]);
		if(!ret)
		{
			CLogMan::error_log("user account: [%s]",uavlog->accountNo);	
			CLogMan::error_log("update failure [%s]",sqlStr);	
			CLogMan::error_log("Ole Error:%s",OleDb->oleDb_GetError());
			return -3;
		}
		return 0;
	}

    if(mysql_real_query(mysql,sqlStr,strlen(sqlStr))!=0){
		CLogMan::error_log("user account: [%s]",uavlog->accountNo);	
		CLogMan::error_log("update failure [%s]",sqlStr);	
		CLogMan::error_log("[%s]",mysql_error(mysql));	
		return -3;
    }

	sprintf(sqlStr,"update Pr_LoginLog set pr_videoFlag = 1 where pr_id='%s'",uavlog->sessionId);
    if(mysql_real_query(mysql,sqlStr,strlen(sqlStr))!=0){
	   CLogMan::error_log("update loginLogin failure [%s]",sqlStr);	
       return -3;
    }
    return 0;
}
int db_mysql::DBDeleteAuthAccessRes(char * websessionid){
	char sqlStr[1024]={0};
	memset(sqlStr,0,sizeof(sqlStr));
	sprintf(sqlStr,"delete from pr_AuthAccessRes where pr_websessionid='%s'",websessionid);
	if(!isMySql)   //oracle or db2
	{
		bool ret=OleDb->oleDb_OneUpdate(sqlStr);
		if(!ret)
		{
			CLogMan::error_log("update failure [%s]",sqlStr);	
			CLogMan::error_log("Ole Error:%s",OleDb->oleDb_GetError());
			return -1;
		}
		return 0;
	}
	CLogMan::debug_log("DBExecSessionID:%s",sqlStr);
	int rtn = 1;
	int count = 0;
	rtn = mysql_real_query(mysql,sqlStr,strlen(sqlStr));
	if (rtn != 0)
	{
		CLogMan::debug_log("update failure [%d]",rtn);	
	}
	return 0;
}

int db_mysql::DBCheckLoginLog(int nodeFlag, char *proxyTime)
{
	/*time_t t;
	time(&t);
	CTime ctime(t);	
	int day = ctime.GetDay();
	int month = ctime.GetMonth();
	int year = ctime.GetYear();
	int hour = ctime.GetHour() ;
	int minutes = ctime.GetMinute() ;
	int second = ctime.GetSecond() ;*/
	int year = 0,month = 0,day = 0,hour = 0,minutes = 0,second = 0; 

	sscanf(proxyTime, "%04d-%02d-%02d %02d:%02d:%02d", &year, &month, &day, &hour, &minutes, &second);
	CTime ctime(year, month, day, hour, minutes, second);

	ctime -=CTimeSpan(0,0,0,30);//10s old 
	day = ctime.GetDay();
	month = ctime.GetMonth();
	year = ctime.GetYear();
	hour = ctime.GetHour() ;
	minutes = ctime.GetMinute();
	second = ctime.GetSecond();
	char szTime[40]={0};
	sprintf(szTime,"%04d-%02d-%02d %02d:%02d:%02d",year,month,day,hour,minutes,second);

	char sqlStr[1024]={0};

	// select * from (select pr_id, pr_uid, pr_userName,rownumber() over(order by pr_id asc ) as rowid  from pr_loginlog) as a where a.rowid >= 0 AND a.rowid <2
	if(strcmp("ORACLE",dbconn->dbType)==0)
	{	
		sprintf(sqlStr,"update Pr_LoginLog set pr_status=0,pr_loginOutDate=to_date('%s','yyyy-mm-dd hh24:mi:ss') where (pr_status = 1 or pr_status = 2) and  pr_onlineTime<to_date('%s','yyyy-mm-dd hh24:mi:ss')",szTime,szTime);
	}else
		if(strcmp("DB2",dbconn->dbType)==0)
		{	
			sprintf(sqlStr,"update Pr_LoginLog set pr_status=0,pr_loginOutDate='%s' where pr_status = 1 and  pr_onlineTime<'%s'",szTime,szTime);
		}else
		{
			sprintf(sqlStr,"update Pr_LoginLog set pr_status=0,pr_loginOutDate='%s' where pr_status = 1 and  pr_onlineTime<'%s'",szTime,szTime);
		}


		if(!isMySql)   //oracle or db2
		{
			bool ret=OleDb->oleDb_OneUpdate(sqlStr);
			if(!ret)
			{
				CLogMan::error_log("DBCheckLoginLog update failure [%s]",sqlStr);	
				CLogMan::error_log("Ole Error:%s",OleDb->oleDb_GetError());
				return -3;
			}
			return 0;
		}

		if(mysql_real_query(mysql,sqlStr,strlen(sqlStr))!=0){
			CLogMan::error_log("DBCheckLoginLog update failure [%s]",sqlStr);	
			CLogMan::error_log("mysql_error [%s]",mysql_error(mysql));
		//	CLogMan::error_log("currentdb[%s],sysconfig.auditdb[%s]",dbconn->dbName,sysconfig.auditConn.dbName);
		//	CLogMan::error_log("currentport[%d],sysconfig.auditport[%s]",dbconn->dbPort,sysconfig.auditConn.dbPort);
			return -3;
		}
		CLogMan::error_log("DBCheckLoginLog update successful [%s]",sqlStr);	
		return 0;
}

void db_mysql::TransDBQueryPort(char *name,char *value,void *handle)
{
	if(value==NULL)
	{
		return;
	}
	int *pr_port= (int*)handle;
	if(strcmp("pr_port",name)==0)
	{
		*pr_port  = atoi(value);
	}
}
void db_mysql::TransDBQueryResName(char *name,char *value,void *handle)
{
	if(value==NULL)
	{
		return;
	}
	if(strcmp("pr_name",name)==0)
	{
		strcpy((char*)handle,value);
	}
}
int db_mysql::GetResourcePort(char * pr_resId,int pr_code,int *port){
	int recordNum=0;
	char sql[1024];
	char real_pwd[1024];
	MYSQL_RES *res;
	MYSQL_ROW row;
	int num;
	int rtn = 0;


	memset(real_pwd,0,sizeof(real_pwd));
	memset(sql, 0, sizeof (sql));
	if(pr_code == 502){
		sprintf(sql, "SELECT pr_resprotocol.pr_port as pr_port from pr_resprotocol,pr_accessprotocol where pr_resprotocol.pr_resId='%s' and pr_resprotocol.pr_apId=pr_accessprotocol.pr_id and (pr_accessprotocol.pr_code=%d or pr_accessprotocol.pr_code=201 or pr_accessprotocol.pr_code=202)", 
			pr_resId, pr_code);        
	}else if(pr_code == 602){
		sprintf(sql, "SELECT pr_resprotocol.pr_port as pr_port from pr_resprotocol,pr_accessprotocol where pr_resprotocol.pr_resId='%s' and pr_resprotocol.pr_apId=pr_accessprotocol.pr_id and (pr_accessprotocol.pr_code=%d or pr_accessprotocol.pr_code=608 or pr_accessprotocol.pr_code=609)", 
			pr_resId, pr_code);        

	}else{
		sprintf(sql, "SELECT pr_resprotocol.pr_port as pr_port from pr_resprotocol,pr_accessprotocol where pr_resprotocol.pr_resId='%s' and pr_resprotocol.pr_apId=pr_accessprotocol.pr_id and pr_accessprotocol.pr_code=%d", 
			pr_resId, pr_code);
	}


	if(!isMySql)   //oracle or db2
	{
		int count = 0;
		char *fields[]={"pr_port",NULL};
		while(count < 30){
			int pr_port = 0;
			bool ret=OleDb->oleDb_Query(sql,fields,TransDBQueryPort,(void *)&pr_port,&recordNum);
			if(!ret)
			{
				CLogMan::error_log("query failure [%s]",sql);
				CLogMan::error_log("Ole Error:%s",OleDb->oleDb_GetError());
				return -1;
			}
			if (recordNum>0)
			{
				*port = pr_port;
				break;
			}
			count ++;
		}
		return 0;		

	}

	if (mysql_query(mysql, sql)) {
		return 2;
	}
	if (!(res = mysql_store_result(mysql))) {
		return 3;
	}
	num = mysql_num_rows(res);
	if (0 < num && (row = mysql_fetch_row(res))) {
		*port = atoi(row[0]);
	} else {
		rtn = 4;
	}
	mysql_free_result(res);
	return rtn;
}
void db_mysql::TransDBQueryDBName(char *name,char *value,void *handle)
{
	if(value==NULL)
	{
		return;
	}
	char *pr_dbName= (char*)handle;
	if(strcmp("pr_dbName",name)==0)
	{
		strcpy(pr_dbName,value);
	}
}
int db_mysql::GetResourceDBName(char * pr_accountId,char *dbname)
{
	int recordNum=0;
	char sql[1024];
	char real_pwd[1024];
	MYSQL_RES *res;
	MYSQL_ROW row;
	int num;
	int rtn = 0;


	memset(real_pwd,0,sizeof(real_pwd));
	memset(sql, 0, sizeof (sql));

	sprintf(sql, "SELECT pr_dbName from pr_resaccount where pr_id='%s'", 
			pr_accountId);

	if(!isMySql)   //oracle or db2
	{
		int count = 0;
		char *fields[]={"pr_dbName",NULL};
		while(count < 30){
			char pr_dbName[1024];
			memset(pr_dbName,0,sizeof(pr_dbName));
			bool ret=OleDb->oleDb_Query(sql,fields,TransDBQueryDBName,(void *)pr_dbName,&recordNum);
			if(!ret)
			{
				CLogMan::error_log("query failure [%s]",sql);
				CLogMan::error_log("Ole Error:%s",OleDb->oleDb_GetError());
				return -1;
			}
			if (recordNum>0)
			{
				strcpy(dbname,pr_dbName);
				break;
			}
			count ++;
		}
		return 0;		

	}


	if (mysql_query(mysql, sql)) {
		return 2;
	}
	if (!(res = mysql_store_result(mysql))) {
		return 3;
	}
	num = mysql_num_rows(res);
	if (0 < num && (row = mysql_fetch_row(res))) {
		if (row[0] != NULL)
		{
			strcpy(dbname,row[0]);
		}
		
	} else {
		rtn = 4;
	}
	mysql_free_result(res);
	return rtn;
}

int db_mysql::GetServiceName(char * pr_raId,char *pr_resId, char *serviceName)
{
	int recordNum=0;
	char sql[1024];
	char real_pwd[1024];
	MYSQL_RES *res;
	MYSQL_ROW row;
	int num;
	int rtn = 0;


	memset(real_pwd,0,sizeof(real_pwd));
	memset(sql, 0, sizeof (sql));

	sprintf(sql, "SELECT pr_value from pr_resaccountext where pr_raId='%s' and (select pr_value from pr_resourceext where pr_resId='%s' and pr_varid='103412485558360')='3'", 
			pr_raId, pr_resId);
	CLogMan::debug_log(sql);

	if(!isMySql)   //oracle or db2
	{
		CLogMan::debug_log("000");
		int count = 0;
		char *fields[]={NULL};
		while(count < 30){
			char name[1024];
			memset(name,0,sizeof(name));
			bool ret=OleDb->oleDb_Query(sql,fields,TransDBQueryDBName,(void *)name,&recordNum);
			if(!ret)
			{
				CLogMan::error_log("query failure [%s]",sql);
				CLogMan::error_log("Ole Error:%s",OleDb->oleDb_GetError());
				return -1;
			}
			if (recordNum>0)
			{
				strcpy(serviceName,name);
				break;
			}
			count ++;
		}
		return 0;		

	}

	if (mysql_query(mysql, sql)) {
		return 2;
	}
	if (!(res = mysql_store_result(mysql))) {
		return 3;
	}
	num = mysql_num_rows(res);
	if (0 < num && (row = mysql_fetch_row(res))) {
		if (row[0] != NULL)
		{
			strcpy(serviceName,row[0]);
		}		
	} else {
		rtn = 4;
	}
	mysql_free_result(res);
	return rtn;
}