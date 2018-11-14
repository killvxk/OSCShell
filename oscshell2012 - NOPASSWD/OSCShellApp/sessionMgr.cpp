#include "stdafx.h"
#include <Windows.h>
#include "../OSCShell/OSCShell.h"
#include "screen2swf.h"
#include <atltime.h>
#include "sessionrdp.h"
#include "db_table.h"

#include "sessionMgr.h"


SESSIONMANAGER paraSession;

SESSIONMANAGER::SESSIONMANAGER(void)
{
	memset(szloginfile,0,sizeof(szloginfile));
	memset(szAvifile,0,sizeof(szAvifile));
	needAudit   = 0;
	exitmsg     = 0;
	noConsole   = 0;
	g_mutex   = NULL;
	channelFlag = -1;
	processId = 0;

	g_disConnEvent = NULL;
	g_reConnEvent  = NULL;

	memset(disConnEventName,0,sizeof(disConnEventName));
	memset(reConnEventName,0,sizeof(reConnEventName));

	DWORD csid=getCurrSessionID();
	sprintf(disConnEventName,"Global\\OscShellApp-%d-Disconnect",csid);
	g_disConnEvent=CreateEvent( NULL, FALSE, FALSE, disConnEventName);
}

SESSIONMANAGER::~SESSIONMANAGER(void)
{
	if(g_disConnEvent)
	{
		CloseHandle(g_disConnEvent);
		g_disConnEvent=NULL;
	}

}
bool SESSIONMANAGER::ExitOscShell_Session(int flag)
{
	ExitOscShell(flag);
	return exitSession();
}

void SESSIONMANAGER::ExitOscShell(int flag)
{
	char msgLog[64]={0};
	sprintf(msgLog,"[ExitOscShell] Flag:[%d]",flag);
	WriteLog(msgLog);
	//WriteLog("[ExitOscShell]");
	if(exitmsg!=1)
	{
		WriteLog("[ExitOscShell]exitmsg!=1");
		exitmsg=1;

		WriteLog("[ExitOscShell]µ÷ÓÃÍ£Ö¹Â¼Ïñ½Ó¿Ú");
		if(strlen(szloginfile) > 0) 
		{
			DeleteFile(szloginfile);			
		}
		//Í£Ö¹Â¼Ïñ
		WriteLog("[ExitOscShell]Í£Ö¹Â¼Ïñ");
		if(needAudit>0)
		{
			OnStop();
		}
		
		//Sleep(20*1000);
		//WriteLog("µÈ´ý20Ãëºó£¬Í£Ö¹Â¼Ïñ£¡");
		WriteLog("[ExitOscShell]ExitRemoteControlServer");
		ExitRemoteControlServer(flag);
	}else{
		WriteLog("[ExitOscShell]exitmsg==1");
	}
}

void SESSIONMANAGER::openLogs(char* g_szAccessId)
{
	OpenLog(g_szAccessId);
}

void SESSIONMANAGER::startRecord()
{
	if(strlen(szAvifile)>0){
		needAudit=1;
		//Æô¶¯Â¼Ïñ
		OnRecordEx(szAvifile,10,processId);
	}
}

bool SESSIONMANAGER::isLoop()
{
	if(exitmsg==1)
	{
		return FALSE;
	}
	return TRUE;
}


bool SESSIONMANAGER::isExit()
{	
	DWORD dReturn = WaitForSingleObject(g_disConnEvent,INFINITE);
	if (WAIT_OBJECT_0 == dReturn)
	{
		return TRUE;
	}
	return FALSE;
}


void SESSIONMANAGER::initSession(char *channelMutesName)
{
	if(!clientProtocolType()){ //·Ç¿ØÖÆÌ¨
		noConsole=1;
		initEvent();
	}
	g_mutex = CreateMutex(NULL, FALSE, NULL);	    
}

bool SESSIONMANAGER::exitSession()
{
	if(g_mutex)
	{
		CloseHandle(g_mutex);	
		g_mutex=NULL;
	}

	if(noConsole==1)
	{
		freeEvent();
	}
	CloseLog();

	if(channelFlag==0)
	{
		channelFlag = -1;
	}
	Sleep(500);
	return DisconnSession();
}
