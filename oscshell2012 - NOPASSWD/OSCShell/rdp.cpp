#include "rdp.h"
#include <Wincrypt.h>
#include <stdio.h>
#include "bin_hex.h"
#include "pub.h"
#include <atltime.h>
#include <wtsapi32.h>

#define MAXADDRLIST 10
static struct in_addr  inList[MAXADDRLIST];
static int addrListNum=0;

extern void getRemoteLogicDiskInfo(char *drivers);

typedef struct{
	TCHAR* line;
} writeLine;

static writeLine rdpLines[] = {
	{L"screen mode id:i:%d\r\n"}, //2全屏幕
	{L"desktopwidth:i:%d\r\n"},//1366
	{L"desktopheight:i:%d\r\n"},//768
	{L"session bpp:i:%d\r\n"}, //16,24
	{L"winposstr:s:0,1,398,83,1198,683\r\n"},
	{L"full address:s:%s\r\n"},
	{L"compression:i:1\r\n"},
	{L"keyboardhook:i:2\r\n"},
	{L"audiomode:i:0\r\n"},
//	{L"redirectdrives:i:0\r\n"},
	{L"redirectclipboard:i:%d\r\n"},	
	{L"redirectprinters:i:0\r\n"},
	{L"redirectcomports:i:0\r\n"},
	{L"redirectsmartcards:i:0\r\n"},
	{L"authentication level:i:0\r\n"},	
	{L"drivestoredirect:s:%s\r\n"},
	{L"displayconnectionbar:i:1\r\n"},
	{L"autoreconnection enabled:i:1\r\n"},
	{L"username:s:%s\r\n"},
	{L"domain:s:%s\r\n"},
	{L"alternate shell:s:\r\n"},
	{L"shell working directory:s:\r\n"},
	{L"password 51:b:%s\r\n"},
	{L"disable wallpaper:i:1\r\n"},
	{L"disable full window drag:i:1\r\n"},
	{L"disable menu anims:i:1\r\n"},
	{L"disable themes:i:0\r\n"},
	{L"disable cursor setting:i:0\r\n"},
	{L"bitmapcachepersistenable:i:1\r\n"},
	NULL
};

int encryptPWD(TCHAR* szPass, BYTE* szReturn, DWORD* dSize){
	DWORD nLen = wcslen(szPass);

	DATA_BLOB blobIn, blobOut;
	blobIn.cbData = (nLen+1)*sizeof(WCHAR);
	blobIn.pbData = (PBYTE )szPass;
	blobOut.cbData = 0;
	blobOut.pbData = NULL;
	if (!CryptProtectData(&blobIn, L"rdp", NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &blobOut))
	{
		OutputDebugString(L"Password encryption failed.");
		return FALSE;
	}
	*dSize=blobOut.cbData;
	memcpy(szReturn, blobOut.pbData, blobOut.cbData);
	LocalFree(blobOut.pbData);
	return TRUE;
}

LPTSTR encryptPWD(TCHAR* szPass,int len)
{
	BYTE* pByte = new BYTE[len];
	DWORD nBytes=0;
	//encrypt the password and get the binary array
	encryptPWD(szPass, pByte, &nBytes);
	memset(szPass,0,len);
	BinaryToString(pByte, nBytes,szPass, len);
	delete[] pByte;
	return szPass;
}

void writeRDP(TCHAR *szPath,TCHAR *DomainName,TCHAR *ServerName,TCHAR *UserName,TCHAR *Password,int diskFlag,int bpp,int Fullscreen){
	FILE  *pFile=NULL;
	
/*	wcscpy(szPath,filename);
	time_t t;
	time(&t);
	CTime ctime(t);	
	int day = ctime.GetDay();
	int month = ctime.GetMonth();
	int year = ctime.GetYear();
	int hour = ctime.GetHour() ;
	int minutes = ctime.GetMinute() ;
	int second = ctime.GetSecond() ;
	WCHAR sztime[100]={0};
*/
//	swprintf(sztime,L"c:\\osc\\oscshellapp\\%d%02i%02i%02i%02i%02i_rdp.rdp",year,month,day,hour,minutes,second);
	
//	swprintf(sztime,L"%04d%02i%02i%02i%02i%02i_%s.rdp",year,month,day,hour,minutes,second,UserName);
//	swprintf(sztime,L"%s.rdp",);
//	wcscat(szPath,sztime);
	TCHAR szPassword[1024]={0};
	memset(szPassword,0,sizeof(szPassword));
	wcscpy(szPassword,Password);

	BYTE buffer[2];
	buffer[0]=0xFF;
	buffer[1]=0xFE;
	TCHAR szTemp[2048]={0};
	TCHAR wzDrivers[512]={0};
	char  drivers[100]={0};
	char  szDrivers[512]={0};
	size_t str_length = 0;

	if(diskFlag==1)
	{
		memset(drivers,0,sizeof(drivers));
		/*getRemoteLogicDiskInfo(drivers);
		for(int i=0,j=0;i<strlen(drivers);i++)
		{
			szDrivers[j++]=drivers[i];
			szDrivers[j++]=':';
			szDrivers[j++]=';';
			sprintf(&szDrivers[j],"%c (\\\\TSCLIENT) (%c:);",(0x63+(0x5a-drivers[i])),drivers[i]);
			j=strlen(szDrivers);
		}*/
		strcpy(szDrivers,"*");
		if(strlen(szDrivers)>0)
		{
			MByteToWChar((CHAR*)szDrivers,wzDrivers,sizeof(wzDrivers)/sizeof(wzDrivers[0]));		
		}
	}

	pFile = _wfopen(szPath, L"w+b");
	if(pFile != NULL)
	{
		fwrite(buffer, 2, 1, pFile);
		fflush(pFile);
		int c=0;
		do{
			if(wcsstr(rdpLines[c].line, L"screen mode")!=NULL)
				wsprintf(szTemp, rdpLines[c].line, Fullscreen);
			else if(wcsstr(rdpLines[c].line, L"username")!=NULL)
				wsprintf(szTemp, rdpLines[c].line, UserName);
			else if(wcsstr(rdpLines[c].line, L"password")!=NULL)
				wsprintf(szTemp, rdpLines[c].line,encryptPWD(szPassword,sizeof(szPassword)));
			else if(wcsstr(rdpLines[c].line, L"domain")!=NULL && DomainName)
				wsprintf(szTemp, rdpLines[c].line, DomainName);
			else if(wcsstr(rdpLines[c].line, L"full address")!=NULL)
				wsprintf(szTemp, rdpLines[c].line, ServerName);
			else if(wcsstr(rdpLines[c].line, L"desktopheight")!=NULL)
				wsprintf(szTemp, rdpLines[c].line,600);
			else if(wcsstr(rdpLines[c].line, L"desktopwidth")!=NULL)
				wsprintf(szTemp, rdpLines[c].line,800);
			else if(wcsstr(rdpLines[c].line, L"session bpp")!=NULL)
				wsprintf(szTemp, rdpLines[c].line, bpp);
			else if(wcsstr(rdpLines[c].line, L"drivestoredirect")!=NULL)
				wsprintf(szTemp, rdpLines[c].line, wzDrivers);
			else if(wcsstr(rdpLines[c].line, L"redirectclipboard")!=NULL)
				wsprintf(szTemp, rdpLines[c].line, diskFlag);
			else
				wsprintf(szTemp, rdpLines[c].line);
			//write line by line
			str_length = wcslen(szTemp) * sizeof(TCHAR); //unicode!
			fwrite(szTemp , str_length, 1, pFile);
			fflush(pFile);

			c++;
		}while( rdpLines[c].line != NULL );
		fclose(pFile);
	}
	
//	return szPath;
}

static writeLine wsLines[] = {
	{L"[Profile]\r\n"},
	{L"ID=WS\r\n"},
	{L"Version=8\r\n"},
	{L"[CT]\r\n"},
	{L"trace=N\r\n"},
	{L"[Telnet5250]\r\n"},
	{L"HostName=%s\r\n"},
	{L"AssociatedPrinterStartMinimized=N\r\n"},
	{L"AssociatedPrinterClose=N\r\n"},
	{L"AssociatedPrinterTimeout=0\r\n"},
	{L"Security=CA400\r\n"},
	{L"SSLClientAuthentication=Y\r\n"},
	{L"CertSelection=AUTOSELECT\r\n"},
	{L"[Communication]\r\n"},
	{L"Link=telnet5250\r\n"},
	{L"Session=5250\r\n"},
	{L"[5250]\r\n"},
//	{L"WorkStationID=%s\r\n"},
	{L"HostCodePage=1388-P\r\n"},
	{L"PrinterType=IBM3812\r\n"},
/*	{L"[Menu]\r\n"},
	{L"DeleteID.1=1094\r\n"},
	{L"DeleteID.2=1104\r\n"},
	{L"DeleteID.3=1112\r\n"},
	{L"DeleteID.4=1128\r\n"},
	{L"DeleteID.5=1129\r\n"},	
*/
	{L"[Keyboard]\r\n"},
	{L"CuaKeyboard=2\r\n"},
	{L"Language=Prc\r\n"},	
	{L"IBMDefaultKeyboard=N\r\n"},	
	{L"DefaultKeyboard=%sAS400.KMP\r\n"},	
	NULL
};

void writeWS(TCHAR *szPath,LPTSTR filename,TCHAR *ServerName,TCHAR *UserName,TCHAR *privatePath)
{
	FILE  *pFile=NULL;
	wcscpy(szPath,filename);
	time_t t;
	time(&t);
	CTime ctime(t);	
	int day = ctime.GetDay();
	int month = ctime.GetMonth();
	int year = ctime.GetYear();
	int hour = ctime.GetHour() ;
	int minutes = ctime.GetMinute() ;
	int second = ctime.GetSecond() ;
	WCHAR sztime[100]={0};
	swprintf(sztime,L"%04d%02i%02i%02i%02i%02i_%s.ws",year,month,day,hour,minutes,second,UserName);
	wcscat(szPath,sztime);

	BYTE buffer[2];
	buffer[0]=0xFF;
	buffer[1]=0xFE;

	TCHAR szTemp[2048]={0};
	size_t str_length = 0;
	pFile = _wfopen(szPath, L"w+b");
	if(pFile != NULL)
	{
		fwrite(buffer, 2, 1, pFile);
		fflush(pFile);
		int c=0;
		do{
			if(wcsstr(wsLines[c].line, L"HostName")!=NULL)
				wsprintf(szTemp, wsLines[c].line, ServerName);
			else if(wcsstr(wsLines[c].line, L"WorkStationID")!=NULL)
				wsprintf(szTemp, wsLines[c].line, UserName);			
			else if(wcsstr(wsLines[c].line, L"DefaultKeyboard")!=NULL)
				wsprintf(szTemp, wsLines[c].line, privatePath);			
			else
				wsprintf(szTemp, wsLines[c].line);

			str_length = wcslen(szTemp) * sizeof(TCHAR); //unicode!
			fwrite(szTemp , str_length, 1, pFile);
			fflush(pFile);
			c++;
		}while( wsLines[c].line != NULL );
		fclose(pFile);
	}
}

//根据目的地址段获取相同地址段本地地址
int checkLocalAddrByDest(char *destAddress,char *outAddress)
{
	int i=0;
	struct   in_addr   sin_addr;  
	sin_addr.S_un.S_addr=inet_addr(destAddress);
	
	for(i=0;i<addrListNum;i++)
	{
		if(sin_addr.S_un.S_un_b.s_b1==inList[i].S_un.S_un_b.s_b1 &&
	       sin_addr.S_un.S_un_b.s_b2==inList[i].S_un.S_un_b.s_b2 &&
           sin_addr.S_un.S_un_b.s_b3==inList[i].S_un.S_un_b.s_b3)
	    {
	       sprintf(outAddress,"%d.%d.%d.%d",
		   inList[i].S_un.S_un_b.s_b1,
		   inList[i].S_un.S_un_b.s_b2,
		   inList[i].S_un.S_un_b.s_b3,
		   inList[i].S_un.S_un_b.s_b4);
		   return 0;
	    }
	}
	return 1;
}


/*本地IP*/
void getLocalAddress(char *IpAddress)
{   
	struct   hostent   *thisHost;
	memset(&inList,0,sizeof(struct in_addr)*MAXADDRLIST);
	WSADATA  wsaData;
	char   HostName[150];
	WSAStartup(MAKEWORD(2,0),&wsaData);
	gethostname(HostName,128);
	thisHost=gethostbyname(HostName);

	while (*thisHost->h_addr_list){    
		struct   in_addr in;
		memcpy((char *)&in,*thisHost->h_addr_list++,sizeof(in));
        inList[addrListNum]=in;
		addrListNum++;
		if(addrListNum>=MAXADDRLIST)
		{
			break;
		}
	}	
	sprintf(IpAddress,"%d.%d.%d.%d",
		inList[0].S_un.S_un_b.s_b1,
		inList[0].S_un.S_un_b.s_b2,
		inList[0].S_un.S_un_b.s_b3,
		inList[0].S_un.S_un_b.s_b4);

	WSACleanup();
}

/*RDP 客户端IP*/
/*
char *getClientAddress(void)
{
	DWORD pBytesReturned;
	WTS_CLIENT_ADDRESS 	*ipAddr;
	char   IpAddress[128];
	if(!IsRDPUser()){
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
*/

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
