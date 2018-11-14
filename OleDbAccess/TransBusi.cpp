#include "stdafx.h"
#include <Windows.h>
#include <stdio.h>
#include <iostream>
#include <comdef.h>
#include <conio.h>
#include <atltime.h>
#include "TransBusi.h"

TRANSBUSI::TRANSBUSI(char *dbtype,char *address,char *dbname,int port,char *dbuser,char *dbpwd)
{
	isOpen=false;
	db=NULL;
	memset(ErrStr,0,sizeof(ErrStr));

	memset(&connInfo,0,sizeof(CONNECT_STRUCT));
	strcpy(connInfo.address,address);
	strcpy(connInfo.dbname,dbname);
	strcpy(connInfo.dbpwd,dbpwd);
	strcpy(connInfo.dbuser,dbuser);
//	strcpy(connInfo.provider,provider);
	strcpy(connInfo.dbtype,dbtype);
	connInfo.port=port;    
	memset(CnnStr,0,sizeof(CnnStr));
	if(strcmp(dbtype,"ORACLE")==0)
	{
		sprintf(CnnStr,
			"Provider=OraOLEDB.Oracle;Persist Security Info=True;Data Source=(DESCRIPTION =(ADDRESS_LIST =(ADDRESS =(PROTOCOL = TCP)(HOST =%s)(PORT = %d)))(CONNECT_DATA =(SERVICE_NAME = %s)));User ID=%s;Password=%s",
			address,port,dbname,dbuser,dbpwd);
	}else
	if(strcmp(dbtype,"DB2")==0)
	{
		sprintf(CnnStr,
			"Provider=IBMDADB2;HOSTNAME=%s;PROTOCOL=TCPIP;PORT=%d;Database=%s;uid=%s;pwd=%s",
			address,port,dbname,dbuser,dbpwd);
	}else if (strcmp(dbtype,"PGSQL") == 0)
	{
		sprintf(CnnStr,"Driver={PostgreSQL ANSI};Server=%s;Port=%d;Database=%s;Uid=%s;Pwd=%s;",
			address,port,dbname,dbuser,dbpwd);
	}
	else if(strcmp(dbtype,"MYSQL")==0)
	{

	}    
    if(strlen(CnnStr)==0)
	{
		return;
	}

	db=new Database();
	::CoInitialize(NULL);	
	if(!db->Open("","",CnnStr))
	{
		db->GetErrorErrStr(ErrStr);		
	}else{
        isOpen=true;
	}
}
TRANSBUSI::~TRANSBUSI()
{
	if(db!=NULL)
	{
       ::CoUninitialize();	
	   delete db;
	   db=NULL;
	}
	isOpen=false;
}

bool TRANSBUSI::getStatus()
{
	return isOpen;
}

bool TRANSBUSI::query(char *sqlStr,char* fields[],backfunc,void *handle,int *recordCount)
{
	int i=0;
	if(!isOpen)
	{
		return false;
	}
    Table *tbl=new Table();
	
	if(!db->Execute(sqlStr,tbl))
	{
		db->GetErrorErrStr(ErrStr);
		delete tbl;
		return false;
	}
	char val[10240];	
	*recordCount=0;
	if(tbl->ISEOF()>=0)
		tbl->MoveFirst();
	
	while(tbl->ISEOF()>=0)
	{
		i=0;
		while(fields[i]!=NULL)
		{
			memset(val,0,sizeof(val));			
			if(tbl->Get(fields[i],val))
			{
               if(strlen(val)>0)
			   {
			      func(fields[i],val,handle); 
			   }else
			   {
			      func(fields[i],NULL,handle); 
			   }
			}else{
			   tbl->GetErrorErrStr(ErrStr);
			   func(fields[i],NULL,handle); 
//			   delete tbl;
//			   return false;
			}
			i++;
		}
		(*recordCount)++;
		tbl->MoveNext();
	}
    delete tbl;
	return true;
}

char *TRANSBUSI::getError()
{
	return ErrStr;
//  strcpy(errMsg,ErrStr);
//	memset(ErrStr,0,sizeof(ErrStr));
}

bool TRANSBUSI::update(char *sqlStr)
{
	if(!isOpen)
	{
		return false;
	}	
	if(!db->Execute(sqlStr))
	{
		db->GetErrorErrStr(ErrStr);
		return false;
	}	
	return true;
}

void TRANSBUSI::beginTrans()
{
	if(!isOpen)
	{
		return;
	}
	db->m_Cnn->BeginTrans();
}

void TRANSBUSI::commitTrans()
{
	if(!isOpen)
	{
		return;
	}
	db->m_Cnn->CommitTrans();
}

void TRANSBUSI::rollbackTrans()
{
	if(!isOpen)
	{
		return;
	}
	db->m_Cnn->RollbackTrans();
}


bool TRANSBUSI::update(char *sqlStr,bool isTrans)
{
	if(!isOpen)
	{
		return false;
	}
	if(isTrans)
	db->m_Cnn->BeginTrans();

	if(!db->Execute(sqlStr))
	{
		db->GetErrorErrStr(ErrStr);
		if(isTrans)
			db->m_Cnn->RollbackTrans();
		return false;
	}	
	if(isTrans)
		db->m_Cnn->CommitTrans();
	return true;

}

bool TRANSBUSI::update(char *sqlStrs[],bool isTrans)
{
	int i=0;
	if(!isOpen)
	{
		return false;
	}
	if(isTrans)
	db->m_Cnn->BeginTrans();

	while(sqlStrs[i]!=NULL)
	{
		if(!db->Execute(sqlStrs[i]))
		{
			db->GetErrorErrStr(ErrStr);
			if(isTrans)
				db->m_Cnn->RollbackTrans();
			return false;
		}	
		i++;
	}

	if(isTrans)
		db->m_Cnn->CommitTrans();
	return true;
}

