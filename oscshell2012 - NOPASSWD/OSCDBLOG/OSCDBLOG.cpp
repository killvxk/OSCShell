// OSCDBLOG.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "mysql.h"
#include "OSCDBLOG.h"

STRUCT_SYSCONFIG sysconfig={0};

static bool isFirstLoadCfg = true; 

int WharToMByte(wchar_t * wideChar,char* narrowChar) 
{
	int nLength = WideCharToMultiByte(CP_ACP,NULL,wideChar,-1,NULL,0,NULL,NULL);
	WideCharToMultiByte(CP_ACP,NULL,wideChar,-1,narrowChar,nLength,NULL,NULL);
	return 1 ;
}

OSCDBLOG_EXPORTS void loadCfgFile(char *cfgFile)
{
	//避免多次执行loadCfgFile
	if (strlen(sysconfig.auditConn.dbType) > 0)
	{
		return;
	}
	char buf[1024];
	char preSufix[50];
	char preSufixBuf[2048];
	memset(&sysconfig,0,sizeof(STRUCT_SYSCONFIG));
	memset(preSufix,0,sizeof(preSufix));

	ifstream infile(cfgFile);
	if(!infile) return;

	infile.seekg(0);
	while(!infile.eof()){
		memset(buf,0,sizeof(buf));
		infile.getline(buf,sizeof(buf)-1);        
		char *op=strstr(buf,"[audit]");
		if(op!=NULL)
		{
			strcpy(preSufix,"audit");
			continue;
		}
		op=strstr(buf,"[osc]");
		if(op!=NULL)
		{
			strcpy(preSufix,"osc");
			continue;
		}
		op=strstr(buf,"[node]");
		if(op!=NULL)
		{
			strcpy(preSufix,"node");
			continue;
		}

		int i=0;
		int offset=0;
		char *offsetOp=sysconfig.auditConn.dbType;
		memset(preSufixBuf,0,sizeof(preSufixBuf));
		sprintf(preSufixBuf,"%s.%s",preSufix,buf);
		while(titleMsg[i]!=NULL)
		{
			op=strstr(preSufixBuf,titleMsg[i]);
			if(op!=NULL)
			{
				op += strlen(titleMsg[i]);
				int len=strlen(op);
				if(len>=configFieldsLen[i])
				{
					len=configFieldsLen[i]-1;
				}
				memcpy((char *)(offsetOp+offset),op,len);            
				break;
			}
			offset += configFieldsLen[i];
			i++;
		}
	}
	infile.close();
}

int DBQueryAuthAccessRes()
{
	if (isFirstLoadCfg)
	{
		isFirstLoadCfg = false;
		TCHAR szPath[512] = {0};
		char path[512] = {0};
		GetModuleFileName(NULL,szPath,MAX_PATH);
		LPTSTR tszSlash = _tcsrchr(szPath, static_cast<int>('\\'));
		if (tszSlash)
		{
			*++tszSlash = '\0';
		}
		wcscat(tszSlash,L"cfg.ini");
		WharToMByte(szPath, path);
		loadCfgFile(path);
	}
	char password[20] = {0};
	if(strlen(sysconfig.oscConn.dbPwd)<20)
	{
		strcpy(p_oscconn.dbPwd,sysconfig.oscConn.dbPwd);
	}else{
		DataSafe._encrypt(sysconfig.oscConn.dbPwd,strlen(sysconfig.oscConn.dbPwd), password);
	}
	MYSQL *mysql;
	if(!mysql_real_connect(mysql,sysconfig.oscConn.dbHost,sysconfig.oscConn.dbUser,sysconfig.oscConn.dbPwd,sysconfig.oscConn.dbName,sysconfig.oscConn.dbPort,NULL,0)){
		return -1;
	}
	
	

}

OSCDBLOG_EXPORTS int DBInsertAuthMessage(int level, char *msg)
{
	
	



}

