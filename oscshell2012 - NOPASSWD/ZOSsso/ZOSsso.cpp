// ZOSsso.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "ZOSsso.h"

#include <stdio.h>
#include <SHLWAPI.H>
#include "eclall.hpp"    
HANDLE m_hEventExit = NULL;
ECLSession *session = NULL;
ECLConnection *ConnObj = NULL;
ECLConnMgr *connMgr = NULL;
DWORD WINAPI SessionThread(LPVOID lparam)
{
	if (m_hEventExit == NULL || ConnObj == NULL)
	{
		return 0;
	}
	while(true){
		Sleep(1000);
		BOOL started = ConnObj->IsStarted()	;
		if(!started){
			break;
		}
	}
	delete ConnObj;
	delete connMgr;
	delete session;
	SetEvent(m_hEventExit);
	return 0;
}

int SetZOSWS(char *cfgPath, char *wsPath, char *ip, int port){

	char cfgBuf[4096] = {0};
	char wsBuf[4096] = {0};
	memset(cfgBuf,0,sizeof(cfgBuf));
	memset(wsBuf,0,sizeof(wsBuf));

	FILE * fp = fopen(cfgPath,"r");
	if (fp != NULL)
	{
		fread(cfgBuf,1,4096,fp);
		fclose(fp);
	}else{
		return 0;
	}

	sprintf(wsBuf,cfgBuf,ip,port);
	fp = fopen(wsPath,"w+");
	if (fp != NULL)
	{
		fwrite(wsBuf,1,strlen(wsBuf),fp);
		fclose(fp);
	}else{
		return 0;
	}
	return 1;

}
ZOSSSO_API HANDLE ZOSsso(char *ip,int port,char *user, char *pwd)
{
	char connname='A';
	char username[256] = {0};
	char password[256] = {0};
	
	char profile[256] = {0};
	char cfgfile[256] = {0};

	char szPath[1024] = {0};
	GetModuleFileNameA(NULL,szPath,MAX_PATH);
	char* slash = strrchr(szPath, static_cast<int>('\\'));
	if (slash)
	{
		*(slash) = '\0';
	}
	sprintf(profile,"%s\\zos.ws",szPath);
	sprintf(cfgfile,"%s\\zos.cfg",szPath);

	if(SetZOSWS(cfgfile,profile,ip,port) == 0){
		return NULL;
	}


	sprintf(username,"logon %s[Enter]",user);
	sprintf(password,"%s[Enter]",pwd);

reconnect:
	try {  // Catch any ECL errors the samples do not trap
		char conStr[256] = {0};
		sprintf(conStr,"profile=\"%s\" connname=%c winstate=max",profile,connname);
		connMgr = new ECLConnMgr();
		connMgr->StartConnection(conStr);
		ConnObj =new ECLConnection(connname);


		long ConnHandle = ConnObj->GetHandle();
		session = new ECLSession(ConnHandle);
		ECLScreenDesc sereen;
		session->GetPS()->WaitForScreen(&sereen,10000);
		ULONG id = 0;
		while(id==0){
			char *Screen;             // Character plane buffer
			ULONG Size;               // Size of PS, plus 3
			ULONG LineLen, Rows; 

			session->GetPS()->GetFieldList()->Refresh();

			session->GetPS()->GetSize(&Rows, &LineLen);             // Get dimensions of PS
			Size = (Rows * LineLen) + 3;              // Calc reqd buffer size

			Screen = new char[Size];                  // Allocate buffer
			session->GetPS()->GetScreen(Screen, Size, TextPlane); 

			printf("%s\r\n",Screen);

			if(StrStrI(Screen,"Enter \"LOGON\"") != NULL){
				id = 1;
			}
			delete Screen;

		}
		session->GetPS()->SendKeys(username);



		id = 0;
		while(id==0){
			char *Screen;             // Character plane buffer
			ULONG Size;               // Size of PS, plus 3
			ULONG LineLen, Rows; 

			session->GetPS()->GetFieldList()->Refresh();

			session->GetPS()->GetSize(&Rows, &LineLen);             // Get dimensions of PS
			Size = (Rows * LineLen) + 3;              // Calc reqd buffer size

			Screen = new char[Size];                  // Allocate buffer
			session->GetPS()->GetScreen(Screen, Size, TextPlane); 

			printf("%s\r\n",Screen);

			if(StrStrI(Screen,"Password") != NULL){
				id = 1;
			}
			delete Screen;

		}


		session->GetPS()->SendKeys(password);

	} // try

	catch (ECLErr Err) {
		printf("\n\nSample program did not catch the following ECL error:\n%s\n",
			Err.GetMsgText());
		if (connname < 'Z')
		{
			connname ++;
			goto reconnect;
		}else{
			return NULL;
		}
	}
	m_hEventExit = CreateEvent(NULL,TRUE,FALSE,NULL);
	DWORD dwThreadId = 0;
	CreateThread(NULL,NULL,SessionThread,NULL,0,&dwThreadId);
	return m_hEventExit;
}
