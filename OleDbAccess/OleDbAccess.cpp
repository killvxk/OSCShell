// OleDbAccess.cpp : Defines the entry point for the DLL application.
//
#include "stdafx.h"
#include "OleDbAccess.h"

COleDbAccess::COleDbAccess(void)
{
	busiHandle=NULL;
}

COleDbAccess::~COleDbAccess(void)
{

}

void COleDbAccess::beginTrans(void)
{

    if(busiHandle!=NULL)
	{
	   busiHandle->beginTrans();
	}
}
void COleDbAccess::commitTrans(void)
{
    if(busiHandle!=NULL)
	{
	    busiHandle->commitTrans();
	}
}
void COleDbAccess::rollbackTrans(void)
{
    if(busiHandle!=NULL)
	{
	    busiHandle->rollbackTrans();
	}
}
bool COleDbAccess::oleDb_Open(char *dbtype,char *address,char *dbname,int port,char *dbuser,char *dbpwd)
{
     busiHandle=new TRANSBUSI(dbtype,address,dbname,port,dbuser,dbpwd); 
	 return busiHandle->getStatus();
}

bool COleDbAccess::oleDb_Query(char *sqlStr,char* fields[],backfunc,void *handle,int *recordCount)
{
     if(busiHandle==NULL)
	 {
		 return false;
	 }	 
	 return busiHandle->query(sqlStr,fields,func,handle,recordCount);
}

bool COleDbAccess::oleDb_OneUpdate(char *sqlStr)
{
     if(busiHandle==NULL)
	 {
		 return false;
	 }	 
	 return busiHandle->update(sqlStr);
}
bool COleDbAccess::oleDb_Update(char *sqlStr,bool isTrans)
{
     if(busiHandle==NULL)
	 {
		 return false;
	 }	 
	 return busiHandle->update(sqlStr,isTrans);
}

bool COleDbAccess::oleDb_UpdateArray(char *sqlStrs[],bool isTrans)
{
     if(busiHandle==NULL)
	 {
		 return false;
	 }	 
	 return busiHandle->update(sqlStrs,isTrans);
}

char *COleDbAccess::oleDb_GetError(void)
{
     if(busiHandle==NULL)
	 {
		 return NULL;
	 }	 
	 return busiHandle->getError();	
}
void COleDbAccess::oleDb_Close(void)
{
     if(busiHandle!=NULL)
	 {
		 delete busiHandle;
		 busiHandle = NULL;
	 }	 
}
