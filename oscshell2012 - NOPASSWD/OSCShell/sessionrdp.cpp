#include "sessionrdp.h"
#include <stdio.h>
bool checkSessionStatus(void){
	TCHAR *ppBuffer=NULL;
	DWORD pBytesReturned;
	bool isDisconn=false;
	if(WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE,
		WTS_CURRENT_SESSION,
		WTSConnectState,
		&ppBuffer,
		&pBytesReturned))
	{
		if(*((INT *)ppBuffer) == WTSActive){
			isDisconn=true;
		}
	}
	WTSFreeMemory( ppBuffer );
	/*
	WTS_SESSION_INFO *sessionInfo = NULL;
	DWORD sessionInfoCount;
	BOOL result = WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0,  hg
	WTSFreeMemory(sessionInfo);
	*/
	return isDisconn;
}

bool DisconnSession(void){
	if(!clientProtocolType()){ //非控制台
		WTSLogoffSession(WTS_CURRENT_SERVER_HANDLE,WTS_CURRENT_SESSION,FALSE); //注销
		return WTSDisconnectSession(WTS_CURRENT_SERVER_HANDLE,WTS_CURRENT_SESSION,FALSE);//断开
	}
	return false;
}

bool clientProtocolType(void){
	TCHAR *ppBuffer=NULL;
	DWORD pBytesReturned;
	bool isConsole=false;
	if( WTSQuerySessionInformation( WTS_CURRENT_SERVER_HANDLE,
		WTS_CURRENT_SESSION,
		WTSClientProtocolType,
		&ppBuffer,
		&pBytesReturned) )
	{
		if(*ppBuffer==WTS_PROTOCOL_TYPE_CONSOLE){
			isConsole=true;
		}
	}
	WTSFreeMemory( ppBuffer );
	return isConsole;
}

/*本地IP*/
char *getLocalAddress(void)
{   
	struct   hostent   *thisHost;
	struct   in_addr   in;
	WSADATA  wsaData;
	char   HostName[150];
	char   IpAddress[150];
	memset(IpAddress,0,sizeof(IpAddress));
	WSAStartup(MAKEWORD(2,0),&wsaData);
	gethostname(HostName,128);
	thisHost=gethostbyname(HostName);
	in = *((LPIN_ADDR)*thisHost->h_addr_list);
	sprintf(IpAddress,"%d.%d.%d.%d ",
		in.S_un.S_un_b.s_b1,
		in.S_un.S_un_b.s_b2,
		in.S_un.S_un_b.s_b3,
		in.S_un.S_un_b.s_b4);
	WSACleanup();
	return IpAddress;
}

/*RDP 客户端IP*/
char *getClientAddress(void)
{
	DWORD pBytesReturned;
	WTS_CLIENT_ADDRESS 	*ipAddr;
	char   IpAddress[128];
	if(!IsRDPUser()){/*No RDP*/
		return getLocalAddress();
	}

	memset(IpAddress,0,sizeof(IpAddress));
	if(WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE,WTS_CURRENT_SESSION,
		WTSClientAddress,(LPTSTR *)&ipAddr,&pBytesReturned)){
			if(ipAddr->AddressFamily!=0){
				sprintf(IpAddress,"%u.%u.%u.%u",ipAddr->Address[2],ipAddr->Address[3],ipAddr->Address[4],ipAddr->Address[5]);
			}
	}
	WTSFreeMemory(ipAddr);
	return IpAddress;
}

bool IsRDPUser(void)
{
	TCHAR *ppBuffer=NULL;
	DWORD pBytesReturned;
	bool isRDP=false;
	if(WTSQuerySessionInformation( WTS_CURRENT_SERVER_HANDLE,
		WTS_CURRENT_SESSION,
		WTSClientProtocolType,
		&ppBuffer,
		&pBytesReturned))
	{
		if(*ppBuffer==WTS_PROTOCOL_TYPE_RDP){
			isRDP=true;
		}
	}
	WTSFreeMemory( ppBuffer);
	return isRDP;
}


DWORD  getSessionProcessPID(TCHAR *name)
{
        PWTS_PROCESS_INFO pProcessInfo=NULL;
        DWORD   ProcessCount;
        DWORD   ProcessId=0;
        DWORD   dwCurSessionID = -1;  
        LPWSTR  pSessionInfo=NULL;  
        DWORD   dwBytes;  

		if(WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE,WTS_CURRENT_SESSION,WTSSessionId,(LPWSTR *)&pSessionInfo, &dwBytes))
		{  
           dwCurSessionID =*((DWORD*)pSessionInfo);  
		}
	    WTSFreeMemory(pSessionInfo); 

        if(WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE,0,1,&pProcessInfo,&ProcessCount))
        {
             for(DWORD i   =   0;i <ProcessCount;i++) {
                   if(pProcessInfo[i].SessionId==dwCurSessionID)
                   {
                      if(wcscmp(pProcessInfo[i].pProcessName,name)==0){
                          ProcessId= pProcessInfo[i].ProcessId;
                          break;
                      }
                   }
	         }
		 }
         WTSFreeMemory(pProcessInfo);
         return ProcessId;
}