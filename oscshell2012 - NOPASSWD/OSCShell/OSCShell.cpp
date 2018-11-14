//#include "OSCShell.h"
#include <io.h>
#include <Windows.h>
#include <commctrl.h>
#include <TlHelp32.h>
#include <SHLWAPI.H>
#include "pub.h"
#include "rdp.h"
#include "sftp.h"
#include <stdio.h>
#include <tchar.h>
//#include "Ini.h"
#include <time.h>
#include <atltime.h>
#include <ShlObj.h>
#include "datasafe.h"
#include "rdp.h"
#include "LogMan.h"
#include "db_mysql.h"
#include "Iphlpapi.h"
#include "sessionrdp.h"
#define DLLEXPORT __declspec(dllexport)
#define NO_AUDIT_RECORD 0x1
#define ONLY_AUDIT 0x2
#define BOTH_AUDIT_RECORD 0x8
#define ONLY_RECORD 0x4

#define CLASS_AS400     L"PCSWS:Main:00400000"
#define CLASS_XMANAGER  L"#32770"
#define TITLE_XMANAGER  L"Xstart"
#define TITLE_RADMIN	L"#32770"//L"Radmin *"
#define TITLE_RDP		L"Input Capture Window"
#define TITLE_MSSQL		L"连接到服务器"
#define CALSS_NAVICAT	L"TWDialogForm"


#define CLASS_XenCenter L"XenCenter"
#define TITLE_IPMI      L"IPMI View V2.9.6 (build 120316) - Super Micro Computer, Inc."
#define TITLE_IPMI_LOGIN      L"Confirm"

#define CLASS_PUTTY		L"PuTTY"
#define CLASS_SCRT		L"VanDyke Software - SecureCRT"
#define CLASS_SCP		L"TScpCommanderForm"
#define CLASS_SQLPLUS   L"ConsoleWindowClass"
#define CLASS_PLSQL     L"TPLSQLDevForm"
#define CLASS_TOAD      L"TPUtilWindow"
#define Label_OscShell  L"c:\\oscshell.ini"
#define Label_Audit     L"audit"
#define Label_Osc       L"osc"
#define Label_Node      L"node"

#define SYBASE_AES		"scjview.bat"
#define SYBASE_AES_TITLE		L"Sybase Central"
#define SYBASE_AES_CLASS		L"SunAwtFrame"
#define SYBASE_AES_LOGIN_TITLE		L"Connect to Adaptive Server Enterprise"
#define SYBASE_AES_LOGIN_CLASS		L"SunAwtDialog"

#define TNSNAMES	"\n%s =\n\
  (DESCRIPTION =\n\
    (ADDRESS = (PROTOCOL = TCP)(HOST = %s)(PORT = %d))\n\
    (CONNECT_DATA =\n\
      (%s = %s)\n\
    )\n\
  )"
#define SYSBASESQL "[%s]\r\n\
master=NLWNSCK,%s,%d\r\n\
query=NLWNSCK,%s,%d"

//2016-02-24 增加
#pragma comment(lib, "Shlwapi.lib")
//2016-02-24
//2016-03-10 增加
//#pragma  comment(lib, "SSO.lib")
//2016-03-10


typedef struct{
   unsigned long  warningTimes_1;
   unsigned long  warningTimes_2;
   unsigned long  warningTimes_3;
   int       videoFlag;
   int       cmdFlag;
   int       debugLevel;
   db_mysql  *sqlHandle;
   HINSTANCE instance;
   HANDLE    hRead; //管道读句柄
   HANDLE    hWrite;//管道写句柄
   int       exitFlag;  //0工具未退出 1工具已退出
   int       pipeFlag;
   int       monitorPort;
   HANDLE    otherProcess; 
   HANDLE    monitorProcess; 
   HANDLE    resProcess; 
   HANDLE    resThread;
   DWORD     dwProcessId;
   TCHAR     wzFile1[512];
   TCHAR     wzFile2[512];   
}STRUCT_OSCSHELL;

static STRUCT_DBAUDITINFO          p_cmdInfo;   //DB audit Info
static STRUCT_DBCONNECT            p_auditconn; //AUDITDB connect Info
static STRUCT_DBCONNECT            p_oscconn;   //OSCDB   connect Info
static STRUCT_AUTHACCESSRES        p_aares;     //auth access resource Info
static STRUCT_USERACTIVITYVIDEOLOG p_videolog;  //audit video Info
static STRUCT_LOGINLOG             p_loginlog;  //audit login Info
static STRUCT_OSCSHELL             p_shell;     //public variable
static STRUCT_ACCESSAUTH           *p_access;
static char lastEndTime[21];                    //authorization end time

extern "C" __declspec(dllimport) void __stdcall DailogSubmit(char * title,char *user,char *pwd);
extern "C" __declspec(dllimport) void __stdcall XmangerSubmit(HWND hWnd,char *user,char *pwd,char *host,char *cmd,char *protocol);
extern "C" __declspec(dllimport) void __stdcall AS400Submit(HWND hWnd,char *server,char *user,char *pwd);

extern void PostData(char *url,char *userName,char *password,char *param,LPDWORD pid, HANDLE *process,char *title[]);
extern int loadDbAudit(STRUCT_DBAUDITINFO *cmdInfo,char *accessPath,unsigned long pid);
extern int getLocalAddrByPing(char *szDestIp,char *localAddr);

extern int getLogicDiskInfo();
void StopLoadWindow();
int GetVistitime(int *timeout,int flag, int *delayFlag);
int getFileOpenMode(char*);
void __stdcall removeAliasDB2();
static char *PATH_ACCESSNAME = "access\\";
static char *PATH_AUDITNAME  = "audit\\";

/*#ifndef MAX
# define MAX(a,b) (((a)>(b))?(a):(b))
#endif*/
# define MIN(a,b) (((a)<(b))?(a):(b))

const char const_key[] = "Py*8Z)F92k&H)W@N";
const char aes_const_key[]="P$A8(r3E";
char aes_key[128] = {0};
char localLang[50] = {0};

char navicatPwd[256] = {0};

typedef unsigned char BYTE;
typedef void (*p_decrypt_key)(BYTE *input_asc,int len,BYTE *output_asc,BYTE *key,int mode_type);
typedef int (*pfnSSOApplication)(char * type,char * apppath,char * ip, char * port, char * username, char *password, char* param, LPDWORD pid, HANDLE *process);
p_decrypt_key _decrypt_key = NULL;

int edit_count = 0;
CRITICAL_SECTION g_csA;
CRITICAL_SECTION mysqlcsA; 
HWND GetWindowByProcessId(DWORD dwProcessId,TCHAR *className,int timeOut);
typedef HANDLE (__stdcall *pZOSsso)(char *ip,int port,char *user, char *pwd);


extern "C" __declspec(dllexport) int __stdcall GetLocalLang(){
	if (strlen(localLang) == 0)
	{
		return 0;
	}else if (strcmp(localLang,"zh") == 0)
	{
		return 0;
	}else if (strcmp(localLang,"en") == 0)
	{
		return 1;
	}else if (strcmp(localLang,"ja") == 0)
	{
		return 2;
	}
	return 3;
}

struct tm ltime(void)
{
	SYSTEMTIME st;
	struct tm tm;

	GetLocalTime(&st);
	tm.tm_sec=st.wSecond;
	tm.tm_min=st.wMinute;
	tm.tm_hour=st.wHour;
	tm.tm_mday=st.wDay;
	tm.tm_mon=st.wMonth-1;
	tm.tm_year=(st.wYear>=1900?st.wYear-1900:0);
	tm.tm_wday=st.wDayOfWeek;
	tm.tm_yday=-1; /* GetLocalTime doesn't tell us */
	tm.tm_isdst=0; /* GetLocalTime doesn't tell us */
	return tm;
}

int DecryptData(char * indata, int len, char *outdata){
	if (strlen(indata) % 44 == 0  && strlen(indata) < 100){
		DataSafe._encrypt(indata, len, outdata);
	}else if(strlen(aes_key) >0 && _decrypt_key != NULL){
		_decrypt_key((BYTE*)indata,len,(BYTE*)outdata,(BYTE*)aes_key,2);
	}else{
		DataSafe._encrypt(indata, len, outdata);
	}
	return 0;
}

void closeDBHandle()
{
	if(p_shell.sqlHandle!=NULL)
	{
		delete p_shell.sqlHandle;
        p_shell.sqlHandle=NULL; 
	}
}
int openDBHandle(STRUCT_DBCONNECT *conn)
{
	if(p_shell.sqlHandle!=NULL)
	{
       return 0;
	}
	p_shell.sqlHandle=new db_mysql(conn);
	if(p_shell.sqlHandle->conndb()==0)
	{
       return 0;
	}
    closeDBHandle();
	return -1;
}


char *split_user_host(char *host,char separator,size_t host_len)
{	
	int point;
	int size;
	if(host_len==0){
		size=strlen(host);
	}else{
		size=MIN(strlen(host),host_len);
	}
	for(point=size-2;point>0;point--){
		if(host[point]==separator)
			break;
	}
	if(point==0)
		return 0;
	host[point]=0;
	return host+point+1;
}


char *replace(char *st, char *orig, char *repl) {
	static char buffer[4096];
	char *ch;
	if (!(ch = strstr(st, orig)))
		return st;
	strncpy(buffer, st, ch-st);  
	buffer[ch-st] = 0;
	sprintf(buffer+(ch-st), "%s%s", repl, ch+strlen(orig));
	return buffer;
}

TCHAR *replace(TCHAR *st, TCHAR *orig, TCHAR *repl) {
	static TCHAR buffer[1024]={0};
	TCHAR *ch=NULL;
	if (!(ch = wcsstr(st, orig)))
		return st;
	_tcsncpy(buffer, st, ch-st);  
	buffer[ch-st] = 0;
	wsprintf(buffer+(ch-st), L"%s%s", repl, ch+_tcslen(orig));
	return buffer;
}

void ReplaceString(LPTSTR& lpSrc, LPTSTR lpOldStr, LPTSTR lpNewStr)
{
	if(!lpSrc || !lpOldStr || !lpNewStr)
		return;

	int nOldLen =  _tcslen(lpOldStr);
	int nNewLen =  _tcslen(lpNewStr);

	int nCount = 0;
	LPTSTR lpszStart = lpSrc;
	LPTSTR lpszEnd  = lpSrc + _tcslen(lpSrc);
	LPTSTR lpszTarget;
	LPTSTR lpTempSrc;

	while(lpszStart < lpszEnd)
	{
		while((lpszTarget = /*wcsstr*/wcswcs(lpszStart, lpOldStr)) != NULL)
		{
			nCount++;
			lpszStart = lpszTarget + nOldLen;
		}
		lpszStart += _tcslen(lpszStart) + 1;
	}

	if(nCount > 0)
	{
		int nOldSrcLen = _tcslen(lpSrc) + 1;
		int nNewSrcLen = nOldSrcLen + (nNewLen-nOldLen)*nCount;

		if(nNewSrcLen > nOldSrcLen)
		{
			lpTempSrc = new TCHAR[nOldSrcLen];
			if(lpTempSrc)
			{
				memset(lpTempSrc, 0, nOldSrcLen);
				//memcpy(lpTempSrc, lpSrc, nOldSrcLen);
				_tcsncpy(lpTempSrc,lpSrc,nOldSrcLen);
				lpSrc = new TCHAR[nNewSrcLen];
				if(lpSrc)
				{
					memset(lpSrc, 0 , nNewSrcLen);
					//memcpy(lpSrc, lpTempSrc, nOldSrcLen);
					_tcsncpy(lpSrc,lpTempSrc,nOldSrcLen);
				}
				delete []lpTempSrc;
				lpTempSrc = NULL;
			}
		}
		lpszStart = lpSrc;
		lpszEnd = lpSrc + _tcslen(lpSrc);
		while(lpszStart < lpszEnd)
		{
			while((lpszTarget = /*wcsstr*/wcswcs(lpszStart, lpOldStr)) != NULL)
			{
				int nBalance = nOldSrcLen - (lpszTarget - lpSrc + nOldLen);
				memmove(lpszTarget + nNewLen, lpszTarget + nOldLen, nBalance * sizeof(TCHAR));
				//memmove(lpszTarget + nNewLen, lpszTarget + nOldLen, nBalance);
				//memcpy(lpszTarget, lpNewStr, nNewLen * sizeof(TCHAR));
				//_tcsncpy(lpszTarget,lpNewStr,nNewLen * sizeof(TCHAR));
				_tcsncpy(lpszTarget,lpNewStr,nNewLen );
				lpszStart = lpszTarget + nNewLen;
				nOldSrcLen += (nNewLen - nOldLen);
			}
			lpszStart += _tcslen(lpszStart) + 1;
		}
	}
}

DWORD GetPortFromPid(DWORD pid)
{
	SECURITY_ATTRIBUTES sa;
	HANDLE hRead,hWrite;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	if (!CreatePipe(&hRead,&hWrite,&sa,0)) {
		return 0;
	}
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	si.cb = sizeof(STARTUPINFOA);
	GetStartupInfoA(&si);
	si.hStdError = hWrite;
	si.hStdOutput = hWrite;
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	char szfind[50]={0};
	sprintf(szfind,"netstat -nao |find \"%d\"",pid);
	char szcmd[100]={0};
	strcat(szcmd,"cmd.exe /c ");
	strcat(szcmd,szfind);
	Sleep(2000);
	if (!CreateProcessA(NULL,szcmd,NULL,NULL,TRUE,NULL,NULL,NULL,&si,&pi)) {
		CloseHandle(hWrite);
        CloseHandle(hRead);
		return 0;
	}
	CloseHandle(hWrite);
	char buffer[4096] = {0};
	DWORD bytesRead;
	while (true) {
		if(ReadFile(hRead,buffer,4095,&bytesRead,NULL) == NULL)
			break;

		int count = 0;
		char* pszParam = strtok(buffer," ");
		while(pszParam)
		{
			if(count == 1)
			{
				break;
			}
			pszParam = strtok(NULL, " ");
			count++;
		}
		char *szport = split_user_host(pszParam,':',strlen(pszParam));
		CloseHandle(hRead);
		if(!szport)
			return 0;
		return atoi(szport);
		Sleep(200);
	}
    CloseHandle(hRead);
	return 0;
}

int SearchObjectStatus(HWND hWnd,HWND hWndStatusBar,TCHAR *filter[])
{
	int i=0,j=0,flag=0;
	while(1)
	{
        Sleep(100);
		DWORD dwProcessId = 0;  
		GetWindowThreadProcessId(hWnd, &dwProcessId);  
		HANDLE hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, dwProcessId);  
		if(NULL == hProcess)  
		{  
		 return -1;  
		}  
		LRESULT nCount = ::SendMessage(hWndStatusBar, SB_GETPARTS, 0, 0);  
		LPVOID pBuf = VirtualAllocEx(hProcess, NULL, MAX_PATH, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);  
		if(NULL != pBuf)  
		{  
			 TCHAR buf[MAX_PATH] = {0};  
			 SIZE_T dwRead = 0;  
			 for(i=0; i<(int)nCount; i++)  
			 {  
				::SendMessage(hWndStatusBar, SB_GETTEXT, i, (LPARAM)pBuf);  
				if(ReadProcessMemory(hProcess, pBuf, buf, sizeof(buf), &dwRead))  
				{  
					j=0;
					while(filter[j]!=NULL)
					{
					   if(wcsstr(buf,filter[j])!=NULL)
					   {
			  			    VirtualFreeEx(hProcess, pBuf, 0, MEM_RELEASE);   
							CloseHandle(hProcess);  
							return j;
					   }
					   j++;
					}
				}     
			 }    
			 VirtualFreeEx(hProcess, pBuf, 0, MEM_RELEASE);   
		}  
		CloseHandle(hProcess);  
	}
    return -1;
}

HWND SearchObjectWindow(TCHAR *filter[])
{
	int	j=0;
	HWND Handle=NULL;
	while(filter[j]!=NULL)
	{
		Handle=::FindWindow(NULL,filter[j]);
	   if(Handle!=NULL)
	   {
           return Handle;
	   }
       j++;
	}
	return NULL;
}
DWORD WINAPI removeAliasDB2Thread(LPVOID lparam)
{
	
	
	TCHAR *cmd1 = L"\"C:\\Program Files\\IBM\\SQLLIB\\BIN\\DB2CMD.exe\" \"db2 uncatalog node ned1&exit\"";
	TCHAR *cmd2 = L"\"C:\\Program Files\\IBM\\SQLLIB\\BIN\\DB2CMD.exe\" \"db2 uncatalog database data1&exit\"";
	TCHAR  wzCmdline1[512]={0};
	TCHAR  data[512]={0};
	if(!MByteToWChar(p_aares.execApp,wzCmdline1,sizeof(wzCmdline1)/sizeof(wzCmdline1[0])))
	{
		return 0;
	}
	wsprintf(data,L"\"%s\"",wzCmdline1);
	CLogMan::info_log("removeAliasDB2.bat \"%s\"",p_aares.execApp) ;
	ShellExecute(NULL,_T("open"),L"removeAliasDB2.bat",data,NULL,SW_HIDE);
	Sleep(3000);
	//ShellExecute(NULL,_T("open"),L"\"C:\Program Files\IBM\SQLLIB\BIN\DB2CMD.exe\"",L"\"db2 uncatalog database data1&exit\"",NULL,SW_HIDE);

	return 0;
}
DWORD WINAPI RefalshPuttyTitleThread(LPVOID lparam)
{
	DWORD dwProcessId = *(DWORD *)lparam;
	HWND hWnd=GetWindowByProcessId(dwProcessId,CLASS_PUTTY,60);
	if(hWnd==NULL)
	{
		return 0;
	}
	char puttyTitle[1024] = {0};
	sprintf(puttyTitle,"%s@%s(%s)",p_access->accountNo,p_access->resName,p_access->resAddr);
	while(true){
		SetWindowTextA(hWnd , puttyTitle );
		Sleep(2000);
	}
	
}
DWORD WINAPI RefalshSCRTTitleThread(LPVOID lparam)
{
	DWORD dwProcessId = *(DWORD *)lparam;
	HWND hWnd=GetWindowByProcessId(dwProcessId,CLASS_SCRT,60);
	if(hWnd==NULL)
	{
		return 0;
	}
	char puttyTitle[1024] = {0};
	sprintf(puttyTitle,"%s@%s(%s)",p_access->accountNo,p_access->resName,p_access->resAddr);
	while(true){
		SetWindowTextA(hWnd , puttyTitle );
		Sleep(2000);
	}

}

DWORD WINAPI RefalshOracleTitleThread(LPVOID lparam)
{
	DWORD dwProcessId = *(DWORD *)lparam;
	HWND hWnd= NULL;
	int accessType=atoi(p_access->accessType);
	if (accessType == TYPE_SQLPLUS)
	{
		hWnd=GetWindowByProcessId(dwProcessId,CLASS_SQLPLUS,60);
	}
	else if (accessType == TYPE_PLSQL)
	{
		hWnd=GetWindowByProcessId(dwProcessId,CLASS_SQLPLUS,60);
	}
	
	if(hWnd==NULL)
	{
		CLogMan::debug_log("hWnd null");
		return 0;
	}
	char sqlplusTitle[1024] = {0};
	sprintf(sqlplusTitle,"%s@%s(%s)",p_access->accountNo,p_access->resName,p_access->resAddr);
	while(true){
		if(SetWindowTextA(hWnd , sqlplusTitle )==0)
		{
			CLogMan::debug_log("SetWindowTextA failed(%d)",GetLastError());
			return 0;
		}
		Sleep(2000);
	}
}
DWORD WINAPI RefalshOracleToadTitleThread(LPVOID lparam)
{
	DWORD dwProcessId = *(DWORD *)lparam;
	HWND hWnd= GetWindowByProcessId(dwProcessId,CLASS_TOAD,60);
	if(hWnd==NULL)
	{
		CLogMan::debug_log("hWnd null");
		return 0;
	}
	char toadTitle[1024] = {0};
	sprintf(toadTitle,"%s@%s(%s)",p_access->accountNo,p_access->resName,p_access->resAddr);
	while(true){
		if(SetWindowTextA(hWnd , toadTitle )==0)
		{
			CLogMan::debug_log("SetWindowTextA failed(%d)",GetLastError());
			return 0;
		}
		Sleep(2000);
	}
}
DWORD WINAPI RefalshSCPTitleThread(LPVOID lparam)
{
	DWORD dwProcessId = *(DWORD *)lparam;
	HWND hWnd=GetWindowByProcessId(dwProcessId,CLASS_SCP,60);
	if(hWnd==NULL)
	{
		return 0;
	}
	char puttyTitle[1024] = {0};
	sprintf(puttyTitle,"%s@%s(%s)",p_access->accountNo,p_access->resName,p_access->resAddr);
	while(true){
		char title[1024] = {0};
		GetWindowTextA(hWnd,title,1024);
		char *p = strstr(title,"FlashFXP");
		if (p != NULL)
		{
			p += 8;
			*p = 0;

			StopLoadWindow();
		}
		strcat(title,puttyTitle);
		SetWindowTextA(hWnd , title );
		Sleep(2000);
	}

}
DWORD WINAPI RDPThread(LPVOID lparam)
{
	
		HWND hWnd=(HWND)lparam;
		CLogMan::debug_log("hide RDP Input Capture Window");
		ShowWindow(hWnd,SW_HIDE);
	return 0;
}
DWORD WINAPI AS400Thread(LPVOID lparam)
{
	int i=0,flag=0;
	TCHAR *FILTER_WINDOW[]={L"Signon to iSeries",L"注册至 iSeries",NULL};
	TCHAR *FILTER1[]={L"Connecting",L"正在连接",L"Session successfully started",L"成功启动会话",NULL};
	TCHAR *FILTER2[]={L"Session successfully started",L"成功启动会话",NULL};

	HWND hWnd=(HWND)lparam;
	
	HWND hWndStatusBar = ::FindWindowEx(hWnd, NULL, _T("msctls_statusbar32"), NULL);  
//	HWND hWndStatusBar = ::FindWindowEx(hWnd, NULL, _T("PCSWS:Pres:00400000"), NULL);  
	if(NULL == hWndStatusBar)  
	{  
	    return 0;  
	} 
/*    
	flag=SearchObjectStatus(hWnd,hWndStatusBar,FILTER1);    
	if(flag<=1)  //exist Connecting auth window
	{
		HWND hWnd_pop=SearchObjectWindow(FILTER_WINDOW);
		if(hWnd_pop!=NULL)
		{
		   AS400Submit(hWnd_pop,p_access->resAddr,p_access->accountNo,p_access->accountPwd);
		}
		SearchObjectStatus(hWnd,hWndStatusBar,FILTER2);
	}
*/
	while(1)
	{
        Sleep(100);
		{
/*
			TCHAR buf[1024]={0};			
			SendMessage(hWndStatusBar,WM_GETTEXT,sizeof(buf)/sizeof(TCHAR),(LPARAM)(void*)buf);
			if(wcslen(buf)>0)
			{
				MessageBox(NULL,buf,L"info",MB_OK);
				break;
			}
*/
			DWORD dwProcessId = 0;  
			GetWindowThreadProcessId(hWnd, &dwProcessId);  
			HANDLE hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, dwProcessId);  
			if(NULL == hProcess)  
			{  
			 return TRUE;  
			}  
			LRESULT nCount = ::SendMessage(hWndStatusBar, SB_GETPARTS, 0, 0);  
			LPVOID pBuf = VirtualAllocEx(hProcess, NULL, MAX_PATH, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);  
			if(NULL != pBuf)  
			{  
				 TCHAR buf[MAX_PATH] = {0};  
				 SIZE_T dwRead = 0;  
				 for(i=0; i<(int)nCount; i++)  
				 {  
					::SendMessage(hWndStatusBar, SB_GETTEXT, i, (LPARAM)pBuf);  
					if(ReadProcessMemory(hProcess, pBuf, buf, sizeof(buf), &dwRead))  
					{  												
					   if(wcsstr(buf,L"Session successfully started")!=NULL||wcsstr(buf,L"成功启动会话")!=NULL)
					   {
                           flag=1; 
						   break;
					   }
					}     
				 }    
				 VirtualFreeEx(hProcess, pBuf, 0, MEM_RELEASE);   
			}  
			CloseHandle(hProcess);  
			if(flag==1)
			{
				break;
			}
//			EnumChildWindows(hWnd,enumChildProc,NULL); 
		}
	}

    Sleep(1000);
	for (i=0;i<strlen(p_access->accountNo);i++)
	{
		SendMessageA(hWnd,WM_SYSKEYDOWN,toupper(p_access->accountNo[i]),0);   //userName
	}
	SendMessageA(hWnd,WM_SYSKEYDOWN,VK_TAB,0);
	
	for (i=0;i<strlen(p_access->accountPwd);i++)
	{
	   SendMessageA(hWnd,WM_SYSKEYDOWN,toupper(p_access->accountPwd[i]),0);  //pwd
	}
	SendMessageA(hWnd,WM_SYSKEYDOWN,VK_RETURN,0);

	return 0;
}

int findShiftKey(char key){

	if(key == '~'){
		return 1;
	}else if(key == '!'){
		return 1;
	}else if(key == '@'){
		return 1;
	}else if(key == '#'){
		return 1;
	}else if(key == '$'){
		return 1;
	}else if(key == '%'){
		return 1;
	}else if(key == '^'){
		return 1;
	}else if(key == '&'){
		return 1;
	}else if(key == '*'){
		return 1;
	}else if(key == '('){
		return 1;
	}else if(key == ')'){
		return 1;
	}else if(key == '_'){
		return 1;
	}else if(key == '+'){
		return 1;
	}else if(key == '{'){
		return 1;
	}else if(key == '}'){
		return 1;
	}else if(key == '|'){
		return 1;
	}else if(key == ':'){
		return 1;
	}else if(key == '\"'){
		return 1;
	}else if(key == '<'){
		return 1;
	}else if(key == '>"'){
		return 1;
	}else if(key == '?'){
		return 1;
	}
	return 0;
}
DWORD XenCenterSSO(LPVOID lparam)
{
	char *ip = p_access->resAddr;
	char *user = p_access->accountNo;
	char *password = p_access->accountPwd;

	HWND hWnd=(HWND)lparam;
	HWND hTopWnd = NULL;
	while(hTopWnd != hWnd){
		::SetForegroundWindow(hWnd);//先将窗口设置为顶级窗口,以便接收到按键响应
		EnableWindow(hWnd,TRUE);
		hTopWnd = GetForegroundWindow();
		Sleep(100);
	}
	SendMessageA(hWnd,WM_SYSCOMMAND,SC_KEYMENU,0);
	Sleep(100);
	keybd_event('S',0,0,0);//再按下s键
	Sleep(100);
	keybd_event('S',0,KEYEVENTF_KEYUP,0);//先放开s键
	Sleep(100);
	keybd_event('A',0,0,0);//再按下s键
	Sleep(100);
	keybd_event('A',0,KEYEVENTF_KEYUP,0);//先放开s键

	Sleep(100);
	HWND aWnd = NULL;
	while(aWnd == NULL){
		aWnd = ::FindWindowA("WindowsForms10.Window.8.app.0.378734a","Add New Server");
		Sleep(100);
	}

	for(int i = 0 ; i < strlen(ip) ; i ++){
		int vk = VkKeyScan(ip[i]);
		keybd_event(vk,0,0,0);
		Sleep(100);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
	}

	keybd_event(VK_TAB,0,0,0);
	Sleep(100);
	keybd_event(VK_TAB,0,KEYEVENTF_KEYUP,0);

	for(int i = 0 ; i < strlen(user) ; i ++){		
		if (isupper(user[i])|| findShiftKey(user[i])==1)
		{
			keybd_event(VK_SHIFT,0,0,0);
			Sleep(100);
		}
		int vk = VkKeyScan(user[i]);
		keybd_event(vk,0,0,0);
		Sleep(100);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
		if (isupper(user[i])|| findShiftKey(user[i])==1)
		{
			keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
		}
	}

	keybd_event(VK_TAB,0,0,0);
	Sleep(100);
	keybd_event(VK_TAB,0,KEYEVENTF_KEYUP,0);

	for(int i = 0 ; i < strlen(password) ; i ++){
		if (isupper(password[i])|| findShiftKey(password[i])==1)
		{
			keybd_event(VK_SHIFT,0,0,0);
			Sleep(100);
		}
		int vk = VkKeyScan(password[i]);
		keybd_event(vk,0,0,0);
		Sleep(100);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
		if (isupper(password[i])|| findShiftKey(password[i])==1)
		{
			keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
		}
	}

	keybd_event(VK_RETURN,0,0,0);
	Sleep(100);
	keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);

	return 0;

}
DWORD MSSQLSSO(HWND hWnd,char *ip,char *user, char *password){

	HWND hTopWnd = NULL;
	while(hTopWnd != hWnd){
		::SetForegroundWindow(hWnd);//先将窗口设置为顶级窗口,以便接收到按键响应
		EnableWindow(hWnd,TRUE);
		hTopWnd = GetForegroundWindow();
		Sleep(100);
	}
	

	for(int i = 0 ; i < strlen(ip) ; i ++){
		int vk = VkKeyScan(ip[i]);
		keybd_event(vk,0,0,0);
		Sleep(10);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
	}

	keybd_event(VK_TAB,0,0,0);
	Sleep(10);
	keybd_event(VK_TAB,0,KEYEVENTF_KEYUP,0);

	keybd_event(VK_TAB,0,0,0);
	Sleep(10);
	keybd_event(VK_TAB,0,KEYEVENTF_KEYUP,0);

	for(int i = 0 ; i < strlen(user) ; i ++){		
		if (isupper(user[i])|| findShiftKey(user[i])==1)
		{
			keybd_event(VK_SHIFT,0,0,0);
			Sleep(10);
		}
		int vk = VkKeyScan(user[i]);
		keybd_event(vk,0,0,0);
		Sleep(10);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
		if (isupper(user[i])|| findShiftKey(user[i])==1)
		{
			keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
		}
	}

	keybd_event(VK_TAB,0,0,0);
	Sleep(10);
	keybd_event(VK_TAB,0,KEYEVENTF_KEYUP,0);

	if (strlen(password) <= 0)
	{
		return 0;
	}
	for(int i = 0 ; i < strlen(password) ; i ++){
		if (isupper(password[i])|| findShiftKey(password[i])==1)
		{
			keybd_event(VK_SHIFT,0,0,0);
			Sleep(10);
		}
		int vk = VkKeyScan(password[i]);
		keybd_event(vk,0,0,0);
		Sleep(10);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
		if (isupper(password[i])|| findShiftKey(password[i])==1)
		{
			keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
		}
	}

	keybd_event(VK_RETURN,0,0,0);
	Sleep(10);
	keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);

	return 0;
}
DWORD NavicatMysqlSSO(HWND hWnd){
	HWND hTopWnd = NULL;
	while(hTopWnd != hWnd){
		::SetForegroundWindow(hWnd);//先将窗口设置为顶级窗口,以便接收到按键响应
		EnableWindow(hWnd,TRUE);
		hTopWnd = GetForegroundWindow();
		Sleep(100);
	}


	for(int i = 0 ; i < strlen(navicatPwd) ; i ++){
		if (isupper(navicatPwd[i])|| findShiftKey(navicatPwd[i])==1)
		{
			keybd_event(VK_SHIFT,0,0,0);
			Sleep(10);
		}
		int vk = VkKeyScan(navicatPwd[i]);
		keybd_event(vk,0,0,0);
		Sleep(10);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
		if (isupper(navicatPwd[i])|| findShiftKey(navicatPwd[i])==1)
		{
			keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
		}
	}
	keybd_event(VK_RETURN,0,0,0);
	Sleep(10);
	keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);

	return 0;
}
DWORD SybaseAesSSO(HWND hWnd){

	char *user = p_access->accountNo;
	char *password = p_access->accountPwd;
	char *ip = p_aares.proxyHost;
	char *port = p_aares.proxyPort;

	HWND hTopWnd = NULL;
	while(hTopWnd != hWnd){
		::SetForegroundWindow(hWnd);//先将窗口设置为顶级窗口,以便接收到按键响应
		EnableWindow(hWnd,TRUE);
		hTopWnd = GetForegroundWindow();
		Sleep(100);
	}

	keybd_event(VK_F11,0,0,0);
	Sleep(100);
	keybd_event(VK_F11,0,KEYEVENTF_KEYUP,0);

	Sleep(200);

	keybd_event(VK_MENU,0,0,0);
	Sleep(50);
	keybd_event('T',0,0,0);//再按下s键
	Sleep(50);
	keybd_event('T',0,KEYEVENTF_KEYUP,0);//先放开s键
	keybd_event(VK_MENU,0,KEYEVENTF_KEYUP,0);

	keybd_event(VK_TAB,0,0,0);
	Sleep(10);
	keybd_event(VK_TAB,0,KEYEVENTF_KEYUP,0);


	for(int i = 0 ; i < strlen(user) ; i ++){		
		if (isupper(user[i])|| findShiftKey(user[i])==1)
		{
			keybd_event(VK_SHIFT,0,0,0);
			Sleep(10);
		}
		int vk = VkKeyScan(user[i]);
		keybd_event(vk,0,0,0);
		Sleep(10);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
		if (isupper(user[i])|| findShiftKey(user[i])==1)
		{
			keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
		}
	}

	keybd_event(VK_TAB,0,0,0);
	Sleep(10);
	keybd_event(VK_TAB,0,KEYEVENTF_KEYUP,0);

	if (strlen(password) <= 0)
	{
		return 0;
	}
	for(int i = 0 ; i < strlen(password) ; i ++){
		if (isupper(password[i])|| findShiftKey(password[i])==1)
		{
			keybd_event(VK_SHIFT,0,0,0);
			Sleep(10);
		}
		int vk = VkKeyScan(password[i]);
		keybd_event(vk,0,0,0);
		Sleep(10);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
		if (isupper(password[i])|| findShiftKey(password[i])==1)
		{
			keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
		}
	}

	keybd_event(VK_TAB,0,0,0);
	Sleep(10);
	keybd_event(VK_TAB,0,KEYEVENTF_KEYUP,0);

	keybd_event(VK_TAB,0,0,0);
	Sleep(10);
	keybd_event(VK_TAB,0,KEYEVENTF_KEYUP,0);

	for(int i = 0 ; i < strlen(ip) ; i ++){
		if (isupper(ip[i])|| findShiftKey(ip[i])==1)
		{
			keybd_event(VK_SHIFT,0,0,0);
			Sleep(10);
		}
		int vk = VkKeyScan(ip[i]);
		keybd_event(vk,0,0,0);
		Sleep(10);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
		if (isupper(ip[i])|| findShiftKey(ip[i])==1)
		{
			keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
		}
	}
	keybd_event(VK_TAB,0,0,0);
	Sleep(10);
	keybd_event(VK_TAB,0,KEYEVENTF_KEYUP,0);

	for(int i = 0 ; i < strlen(port) ; i ++){
		if (isupper(port[i])|| findShiftKey(port[i])==1)
		{
			keybd_event(VK_SHIFT,0,0,0);
			Sleep(10);
		}
		int vk = VkKeyScan(port[i]);
		keybd_event(vk,0,0,0);
		Sleep(10);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
		if (isupper(port[i])|| findShiftKey(port[i])==1)
		{
			keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
		}
	}
	keybd_event(VK_RETURN,0,0,0);
	Sleep(10);
	keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);
	return 0;
}
DWORD IPMISSO(LPVOID lparam)
{
	char *ip = p_access->resAddr;
	char *user = p_access->accountNo;
	char *password = p_access->accountPwd;

	HWND hWnd=(HWND)lparam;
	HWND hTopWnd = NULL;
	while(hTopWnd != hWnd){
		::SetForegroundWindow(hWnd);//先将窗口设置为顶级窗口,以便接收到按键响应
		EnableWindow(hWnd,TRUE);
		hTopWnd = GetForegroundWindow();
		Sleep(100);
	}
	Sleep(3000);
	//HWND hWnd = GetForegroundWindow();
	//SendMessageA(hWnd,WM_SYSCOMMAND,SC_KEYMENU,0);
	Sleep(100);
	keybd_event(18,0,0,0);
	Sleep(100);
	keybd_event(18,0,KEYEVENTF_KEYUP,0);

	Sleep(100);
	keybd_event(VK_RETURN,0,0,0);
	Sleep(100);
	keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);
	Sleep(100);
	keybd_event(VK_DOWN,0,0,0);
	Sleep(100);
	keybd_event(VK_DOWN,0,KEYEVENTF_KEYUP,0);
	Sleep(100);
	keybd_event(VK_RETURN,0,0,0);
	Sleep(100);
	keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);
	Sleep(100);
	keybd_event(VK_DOWN,0,0,0);
	Sleep(100);
	keybd_event(VK_DOWN,0,KEYEVENTF_KEYUP,0);
	Sleep(100);
	keybd_event(VK_RETURN,0,0,0);
	Sleep(100);
	keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);

	keybd_event(VK_TAB,0,0,0);
	Sleep(100);
	keybd_event(VK_TAB,0,KEYEVENTF_KEYUP,0);

	keybd_event(VK_CONTROL,0,0,0);
	keybd_event('A',0,0,0);
	keybd_event('A',0,KEYEVENTF_KEYUP,0);
	keybd_event(VK_CONTROL,0,KEYEVENTF_KEYUP,0);
	keybd_event(VK_BACK,0,0,0);
	keybd_event(VK_BACK,0,KEYEVENTF_KEYUP,0);

	Sleep(100);

	for(int i = 0 ; i < strlen(ip) ; i ++){
		int vk = VkKeyScan(ip[i]);
		keybd_event(vk,0,0,0);
		Sleep(50);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
	}

	Sleep(100);
	keybd_event(VK_RETURN,0,0,0);
	Sleep(100);
	keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);
	Sleep(2000);
	for(int i = 0 ; i < 13 ; i ++){
		keybd_event(VK_TAB,0,0,0);
		Sleep(10);
		keybd_event(VK_TAB,0,KEYEVENTF_KEYUP,0);
	}
	Sleep(100);
	keybd_event(VK_RETURN,0,0,0);
	Sleep(100);
	keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);
	Sleep(1000);

	hWnd=GetWindowByProcessId(0,TITLE_IPMI_LOGIN,3);
	if(hWnd != NULL){
		/*keybd_event(VK_TAB,0,0,0);
		Sleep(10);
		keybd_event(VK_TAB,0,KEYEVENTF_KEYUP,0);
		Sleep(100);
		keybd_event(VK_RETURN,0,0,0);
		Sleep(100);
		keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);
		Sleep(500);
		hWnd=GetWindowByProcessId(0,TITLE_IPMI_LOGIN,1);*/

		keybd_event(VK_SPACE,0,0,0);
		Sleep(100);
		keybd_event(VK_SPACE,0,KEYEVENTF_KEYUP,0);
		Sleep(1000);
	}

	
	for(int i = 0 ; i < strlen(user) ; i ++){		
		if (isupper(user[i]) || findShiftKey(user[i])==1)
		{
			keybd_event(VK_SHIFT,0,0,0);
			Sleep(50);
		}
		int vk = VkKeyScan(user[i]);
		keybd_event(vk,0,0,0);
		Sleep(100);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
		if (isupper(user[i]) || findShiftKey(user[i])==1)
		{
			keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
		}
	}

	keybd_event(VK_TAB,0,0,0);
	Sleep(100);
	keybd_event(VK_TAB,0,KEYEVENTF_KEYUP,0);

	for(int i = 0 ; i < strlen(password) ; i ++){

		if (isupper(password[i]) || findShiftKey(password[i])==1)
		{
			keybd_event(VK_SHIFT,0,0,0);
			Sleep(100);
		}
		int vk = VkKeyScan(password[i]);
		keybd_event(vk,0,0,0);
		Sleep(50);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
		if (isupper(password[i]) || findShiftKey(password[i])==1)
		{
			keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
		}
	}

	keybd_event(VK_RETURN,0,0,0);
	Sleep(100);
	keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);

	return 0;
}
DWORD WindowsSecurity(LPVOID lparam){
	char *user = p_access->accountNo;
	char *password = p_access->accountPwd;

	HWND hWnd=(HWND)lparam;
	HWND hTopWnd = NULL;
	while(hTopWnd != hWnd){
		::SetForegroundWindow(hWnd);//先将窗口设置为顶级窗口,以便接收到按键响应
		EnableWindow(hWnd,TRUE);
		hTopWnd = GetForegroundWindow();
		Sleep(100);
	}

	for(int i = 0 ; i < strlen(user) ; i ++){		
		if (isupper(user[i]) || findShiftKey(user[i])==1)
		{
			keybd_event(VK_SHIFT,0,0,0);
			Sleep(50);
		}
		int vk = VkKeyScan(user[i]);
		keybd_event(vk,0,0,0);
		Sleep(100);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
		if (isupper(user[i]) || findShiftKey(user[i])==1)
		{
			keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
		}
	}

	keybd_event(VK_TAB,0,0,0);
	Sleep(100);
	keybd_event(VK_TAB,0,KEYEVENTF_KEYUP,0);

	for(int i = 0 ; i < strlen(password) ; i ++){

		if (isupper(password[i]) || findShiftKey(password[i])==1)
		{
			keybd_event(VK_SHIFT,0,0,0);
			Sleep(100);
		}
		int vk = VkKeyScan(password[i]);
		keybd_event(vk,0,0,0);
		Sleep(50);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
		if (isupper(password[i]) || findShiftKey(password[i])==1)
		{
			keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
		}
	}

	keybd_event(VK_RETURN,0,0,0);
	Sleep(100);
	keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);
	return 0;
}
DWORD RadminSSO(LPVOID lparam)
{

	char *user = p_access->accountNo;
	char *password = p_access->accountPwd;

	HWND hWnd=(HWND)lparam;
	HWND hTopWnd = NULL;
	while(hTopWnd != hWnd){
		::SetForegroundWindow(hWnd);//先将窗口设置为顶级窗口,以便接收到按键响应
		EnableWindow(hWnd,TRUE);
		hTopWnd = GetForegroundWindow();
		Sleep(100);
	}
	if (edit_count>=2)
	{
		for(int i = 0 ; i < strlen(user) ; i ++){		
			if (isupper(user[i]) || findShiftKey(user[i])==1)
			{
				keybd_event(VK_SHIFT,0,0,0);
				Sleep(100);
			}
			int vk = VkKeyScan(user[i]);
			keybd_event(vk,0,0,0);
			Sleep(100);
			keybd_event(vk,0,KEYEVENTF_KEYUP,0);
			if (isupper(user[i]) || findShiftKey(user[i])==1)
			{
				keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
			}
		}

		keybd_event(VK_TAB,0,0,0);
		Sleep(100);
		keybd_event(VK_TAB,0,KEYEVENTF_KEYUP,0);
	}
	

	for(int i = 0 ; i < strlen(password) ; i ++){
		if (isupper(password[i]) || findShiftKey(password[i])==1)
		{
			keybd_event(VK_SHIFT,0,0,0);
			Sleep(100);
		}
		int vk = VkKeyScan(password[i]);
		keybd_event(vk,0,0,0);
		Sleep(100);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
		if (isupper(password[i]) || findShiftKey(password[i])==1)
		{
			keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
		}
	}

	keybd_event(VK_RETURN,0,0,0);
	Sleep(100);
	keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);

	return 0;

}

void PostpresqlSSO(char * ip,int port,char *user,char *pwd,char *dbname)
{
	char szPath[1024] = {0};
	char regData[10240] = {0};
	char regRealData[10240] = {0};
	memset(szPath,0,sizeof(szPath));
	memset(regData,0,sizeof(regData));
	memset(regRealData,0,sizeof(regRealData));

	GetModuleFileNameA(NULL,szPath,MAX_PATH);
	char* slash = strrchr(szPath, static_cast<int>('\\'));
	if (slash)
	{
		*(slash) = '\0';
	}
	strcat(szPath,"\\pgadmin.cfg");
	FILE * fp = fopen(szPath,"r");
	if (fp != NULL)
	{
		fread(regData,1,1024,fp);
		fclose(fp);
	}else{
		return;
	}
	sprintf(regRealData,regData,ip,ip,port,dbname,user);

	memset(szPath,0,sizeof(szPath));
	GetModuleFileNameA(NULL,szPath,MAX_PATH);
	slash = strrchr(szPath, static_cast<int>('\\'));
	if (slash)
	{
		*(slash) = '\0';
	}
	strcat(szPath,"\\pgadmin.reg");
	fp = fopen(szPath,"w");
	if (fp != NULL)
	{
		fwrite(regRealData,1,strlen(regRealData),fp);
		fclose(fp);
	}else{
		return;
	}

	char runParam[1024] = {0};
	sprintf(runParam,"/s \"%s\"",szPath);

	ShellExecuteA(NULL,("open"),"regedit",runParam,NULL,SW_HIDE);

	char autoPwd[1024] = {0};
	memset(autoPwd,0,sizeof(autoPwd));
	sprintf(autoPwd,"%s:%d:*:%s:%s",ip,port,user,pwd);

	memset(szPath,0,sizeof(szPath));
	SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, NULL, szPath);
	strcat(szPath,"\\postgresql\\pgpass.conf");

	fp = fopen(szPath,"w");
	if (fp != NULL)
	{
		fwrite(autoPwd,1,strlen(autoPwd),fp);
		fclose(fp);
	}else{
		return;
	}
}
typedef struct
{
     HWND hWnd;
     DWORD dwProcessId;
	 TCHAR wzBuf[256];
}EnumFunArg;
 
BOOL CALLBACK EnumChildProc(HWND hWnd,LPARAM lParam)
{
	CHAR szBuf[256]={0};
	GetClassNameA(hWnd,szBuf,256);
	if (strstr(szBuf,"Edit"))
	{
		edit_count ++;
	}
	return true;
}

static BOOL CALLBACK lpEnumFunc(HWND hwnd, LPARAM lParam)
{

     EnumFunArg *pArg = reinterpret_cast<EnumFunArg *> (lParam);
     DWORD  dwProcessId;
     GetWindowThreadProcessId(hwnd, &dwProcessId);    
     if( dwProcessId == pArg->dwProcessId  || pArg->dwProcessId == 0)
     {
		 if(wcslen(pArg->wzBuf)>0)
		 {
			 TCHAR szBuf[256]={0};
			 GetClassName(hwnd,szBuf,256);

			 TCHAR szBufText[256]={0};
			 GetWindowText(hwnd, szBufText, sizeof(TCHAR)*MAX_PATH); 

			 CLogMan::debug_log("lpEnumFunc:[%ws][%ws][%ws]",pArg->wzBuf,szBuf,szBufText) ;

			 if(_tcsicmp(szBuf,pArg->wzBuf) == 0 || _tcsicmp(szBufText,pArg->wzBuf) == 0)
			 {
				 pArg->hWnd = hwnd;
				 return FALSE;
			 }else if(wcsstr(pArg->wzBuf,L"*")!=NULL){
				 TCHAR szTmpext[256]={0};
				 wcscpy(szTmpext,pArg->wzBuf);
				 TCHAR * pTmp = wcsstr(szTmpext,L"*");
				 *pTmp = 0;

				 if(wcsstr(szBuf,szTmpext)!=NULL||wcsstr(szBufText,szTmpext)!=NULL){
					 pArg->hWnd = hwnd;
					 return FALSE;
				 }
			 }
		 }else
		 {
			 pArg->hWnd = hwnd;
			 return FALSE;
		 }
     }
     return TRUE;
}

HWND GetWindowByProcessId(DWORD dwProcessId,TCHAR *className,int timeOut)
{
	 int pid_count=0;
     EnumFunArg arg;
     memset(&arg,0,sizeof(EnumFunArg));
	 arg.dwProcessId = dwProcessId;
     arg.hWnd = 0;
	 if(className!=NULL)
	 {
		wcscpy(arg.wzBuf,className); 
	 }
     EnumWindows(lpEnumFunc,reinterpret_cast<LPARAM>(&arg));
	 while(arg.hWnd==NULL && timeOut>0)
	 {
		 Sleep(1000);
		 pid_count++;
		 if(pid_count>timeOut)
		 {
//			 MessageBox(NULL,L"Not Found Process Window!",L"Info",MB_OK);
		     break; 
		 }
         EnumWindows(lpEnumFunc,reinterpret_cast<LPARAM>(&arg));
	 }
     return arg.hWnd;
}



typedef struct
{
	HWND hWnd;
	DWORD dwProcessId;
	CHAR szClass[256];
	CHAR szTitle[256];
}EnumFunArgSub;
bool isFindString(unsigned char *src, unsigned char *dest){
	if (strlen((char*)src) == 0)
	{
		return true;
	}
	else
	{
		if (strstr((char*)src,"*") == NULL)
		{
			if (strcmp((char*)src,(char*)dest) == 0)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			unsigned char szTmpext[256]={0};
			strcpy((char*)szTmpext,(char*)src);
			char *p = strstr((char*)szTmpext,"*");
			*p=0;

			if(strstr((char*)dest,(char*)szTmpext) != NULL){
				return true;
			}else{
				return false;
			}

		}
	}
}

static BOOL CALLBACK lpSubsubEnumFunc(HWND hwnd, LPARAM lParam)
{

	EnumFunArgSub *pArg = reinterpret_cast<EnumFunArgSub *> (lParam);
	DWORD  dwProcessId;
	GetWindowThreadProcessId(hwnd, &dwProcessId);    
	if( dwProcessId == pArg->dwProcessId  || pArg->dwProcessId == 0)
	{
		CHAR szClass[256]={0};
		CHAR szTitle[256]={0};

		GetClassNameA(hwnd,szClass,256);
		GetWindowTextA(hwnd, szTitle, 256); 


		if(isFindString((unsigned char *)pArg->szClass,(unsigned char *)szClass) == true && isFindString((unsigned char *)pArg->szTitle,(unsigned char *)szTitle) == true)
		{
			pArg->hWnd = hwnd;
			return FALSE;
		}
	}
	return TRUE;
}
static BOOL CALLBACK lpSubEnumFunc(HWND hwnd, LPARAM lParam)
{

	EnumFunArgSub *pArg = reinterpret_cast<EnumFunArgSub *> (lParam);
	DWORD  dwProcessId;
	GetWindowThreadProcessId(hwnd, &dwProcessId);    
	if( dwProcessId == pArg->dwProcessId  || pArg->dwProcessId == 0)
	{
		EnumChildWindows(hwnd,lpSubsubEnumFunc,lParam);
	}
	return TRUE;
}
HWND GetSubWindowByProcessId(DWORD dwProcessId,CHAR *className,CHAR *titleName,int timeOut){
	int pid_count=0;
	EnumFunArgSub arg;
	memset(&arg,0,sizeof(EnumFunArgSub));
	arg.dwProcessId = dwProcessId;
	arg.hWnd = 0;
	if(className!=NULL)
	{
		strcpy(arg.szClass,className); 
	}
	if(className!=NULL)
	{
		strcpy(arg.szTitle,titleName); 
	}

	EnumWindows(lpSubEnumFunc,reinterpret_cast<LPARAM>(&arg));
	while(arg.hWnd==NULL && timeOut>0)
	{
		Sleep(1000);
		pid_count++;
		if(pid_count>timeOut)
		{
			break; 
		}
		EnumWindows(lpSubEnumFunc,reinterpret_cast<LPARAM>(&arg));
	}
	return arg.hWnd;
}
void exitClear(char *msg)
{
	CLogMan::error_log("[exitClear]start");

	if(msg!=NULL)
	{
		CLogMan::error_log("[exitClear]%s",msg);
	}
	
	if(p_shell.otherProcess!=NULL)
	{
		::TerminateProcess(p_shell.otherProcess,0);    	
	}    
	if(p_shell.monitorProcess!=NULL)
	{
		::TerminateProcess(p_shell.monitorProcess,0);    	
	}    
	if(p_shell.resProcess!=NULL && p_shell.exitFlag==0)  //未退出时执行
	{
		TerminateProcess(p_shell.resProcess,0);  
		CLogMan::error_log("[exitClear]CloseHandle");
		if(CloseHandle(p_shell.resProcess) == 0)
		{
			p_shell.resProcess=NULL;
			CLogMan::error_log("[exitClear]CloseHandle failed(%d)!",GetLastError());
		}
		CloseHandle(p_shell.resThread);
	}    
}
extern STRUCT_SYSCONFIG sysconfig;
extern void loadCfgFile(char *cfgFile);
extern "C" __declspec(dllexport) void __stdcall oscLoadCfgFile(char *cfgFile, int *warningTime1, int *warningTime2, int *warningTime3)
{
	loadCfgFile(cfgFile);
	*warningTime1 = atoi(sysconfig.warningTimes_1);
	*warningTime2 = atoi(sysconfig.warningTimes_2);
	*warningTime3 = atoi(sysconfig.warningTimes_3);
}
extern "C" __declspec(dllexport) void __stdcall OpenLog(char *g_szAccessId=NULL)
{
	 char  szCmdline[1024] = {0};
	 TCHAR szPath[MAX_PATH] = {0};
	 GetModuleFileName(NULL,szPath,MAX_PATH);
	 LPTSTR tszSlash = _tcsrchr(szPath, static_cast<int>('\\'));
	 if (tszSlash)
	 {
		*++tszSlash = '\0';
	 }
	 wcscat(tszSlash,L"cfg.ini");

	 memset(szCmdline,0,sizeof(szCmdline));
	 WharToMByte(szPath,szCmdline);
     loadCfgFile(szCmdline);	
	 if (stricmp(sysconfig.level,"logoff") == 0)
	 {
		 CLogMan::log_switch = LOG_OFF;
	 }
	 else
	 {
		 CLogMan::log_switch = LOG_ON;
	 }
	 if (CLogMan::log_switch != LOG_ON)
	 {
		 return;
	 }
     CLogMan::OpenLogFile(g_szAccessId);
     CLogMan::setLogLevel(DEBUG_LEVEL);
	 CLogMan::info_log("%s","启动");
}

extern "C" __declspec(dllexport) void __stdcall WriteLog(char *msg)
{
	if(CLogMan::log_switch != 1)
	{
		return;
	}
	if(msg!=NULL)
	{
		CLogMan::info_log("%s", msg) ;
	}
}

extern "C" __declspec(dllexport) void __stdcall CloseLog()
{
	CLogMan::error_log("系统退出！");
    CLogMan::ClosLogFile();
	//exit(0);
}

int OpenTNS(char *tnsname){
	
	char szPath[1024] = {0};
	GetModuleFileNameA(NULL,szPath,MAX_PATH);
	char* slash = strrchr(szPath, static_cast<int>('\\'));
	if (slash)
	{
		*(slash) = '\0';
	}
	strcat(szPath,"\\tnsname.cfg");
	FILE * fp = fopen(szPath,"r");
	if (fp != NULL)
	{
		size_t size = fread(tnsname,1,1024,fp);
		fclose(fp);
	}else{
		strcpy(tnsname,TNSNAMES);
	}
	return 0;
}
HANDLE hProcess = NULL;
void StartLoadWindow()
{
	char szPath[1024] = {0};
	GetModuleFileNameA(NULL,szPath,MAX_PATH);
	char* slash = strrchr(szPath, static_cast<int>('\\'));
	if (slash)
	{
		*(slash) = '\0';
	}
	strcat(szPath,"\\LoadWindow.exe");

	STARTUPINFOA   StartInfo;   
	PROCESS_INFORMATION     ProceInfo;   
	ZeroMemory(&StartInfo,sizeof(StartInfo));   
	ZeroMemory(&ProceInfo,sizeof(ProceInfo));   
	StartInfo.cb=sizeof(StartInfo); 
	StartInfo.dwFlags =STARTF_USESHOWWINDOW;
	StartInfo.wShowWindow = SW_SHOW;

	CreateProcessA(NULL,     
			szPath,   
			NULL,     
			NULL,   
			FALSE,
     		CREATE_NEW_CONSOLE|NORMAL_PRIORITY_CLASS,
			NULL,   
			NULL,   
			&StartInfo,   
			&ProceInfo); 

	hProcess = ProceInfo.hProcess;
	
}
void StopLoadWindow(){
	if(hProcess != NULL){
		TerminateProcess(hProcess,4);
		hProcess = NULL;
	}
}

static int noPassword = 1;
extern "C" __declspec(dllexport) HANDLE __stdcall OSCShellEx(
						IN  char* szAccessID,  //动态访问ID
						IN  char* szClientIP,
						IN  int   diskFlag,
						IN  int   showWindow,
						OUT char* szloginfile,
						OUT char* szAvifile,
						OUT int * newtype,
						OUT DWORD * processId,
						OUT int *accType,  //访问方式
						OUT int *hasChild//是否需要查询子进程 0，no 1，yes
						)
{
	TCHAR szPath[512] = {0};
	TCHAR szTempDir[512] = {0};
	TCHAR wzCmdline[1024] = {0};
	char  szCmdline[1024] = {0};
	char  szCmdparam[512] = {0};
	char OSCresPassWord[20]={0};
	char OSCresPassWordEx[20]={0};
    char auditfilePath[100] ={0};
    char auditfileName[150] ={0};
	TCHAR wzMaxOutRows[50]={0};
	char szDebugTmp[60]={0};
	unsigned long warningTimes=0;
	int localOsc = 1;
	char mssql_host[256] = {0};
	
  //int debug_level=0;

	memset(&p_cmdInfo,0,sizeof(STRUCT_DBAUDITINFO));
	memset(&p_shell,0,sizeof(STRUCT_OSCSHELL));
	memset(&p_aares,0,sizeof(STRUCT_AUTHACCESSRES));
    memset(&p_auditconn,0,sizeof(STRUCT_DBCONNECT));
    memset(&p_oscconn,0,sizeof(STRUCT_DBCONNECT));
    memset(&p_videolog,0,sizeof(STRUCT_USERACTIVITYVIDEOLOG));
	memset(&p_loginlog,0,sizeof(STRUCT_LOGINLOG));

	InitializeCriticalSection(&g_csA);
	HINSTANCE hinstance= LoadLibrary(L"libAES.dll");
	if(hinstance !=NULL){
		_decrypt_key = (p_decrypt_key)::GetProcAddress(hinstance,"_decrypt_key");
	}else{
		CLogMan::debug_log("not found libAES.dll!");
	}

    p_access=&p_aares.accessAuth;

	GetModuleFileName(NULL,szPath,MAX_PATH);
	LPTSTR tszSlash = _tcsrchr(szPath, static_cast<int>('\\'));
	if (tszSlash)
	{
		*++tszSlash = '\0';
	}
	wcscpy(szTempDir,szPath);     //当前目录
	wcscat(tszSlash,L"cfg.ini");

	if(strlen(sysconfig.code)==0)
	{
		memset(szCmdline,0,sizeof(szCmdline));
		WharToMByte(szPath,szCmdline);
		loadCfgFile(szCmdline);	
		memset(szCmdline,0,sizeof(szCmdline));
	}	
	strcpy(auditfilePath,sysconfig.auditPath);	
	if(strlen(auditfilePath)==0)
	{
	    WharToMByte(szTempDir,auditfilePath);
	}

	if(auditfilePath[strlen(auditfilePath)-1] != '\\')
    {
	    strcat(auditfilePath,"\\");
    }
	strcpy(p_auditconn.dbHost,sysconfig.auditConn.dbHost);
	strcpy(p_auditconn.dbUser,sysconfig.auditConn.dbUser);
	strcpy(p_auditconn.dbName,sysconfig.auditConn.dbName);
	strcpy(p_auditconn.dbType,sysconfig.auditConn.dbType);
	p_auditconn.dbPort=atoi(sysconfig.auditConn.dbPort);
	if(strlen(sysconfig.auditConn.dbPwd)<20)
	{
		strcpy(p_auditconn.dbPwd,sysconfig.auditConn.dbPwd);
	}else{
		DataSafe._encrypt(sysconfig.auditConn.dbPwd,strlen(sysconfig.auditConn.dbPwd),p_auditconn.dbPwd);
	}

//OSC DB
	strcpy(p_oscconn.dbHost,sysconfig.oscConn.dbHost);
	strcpy(p_oscconn.dbUser,sysconfig.oscConn.dbUser);
	strcpy(p_oscconn.dbName,sysconfig.oscConn.dbName);
	strcpy(p_oscconn.dbType,sysconfig.oscConn.dbType);
	p_oscconn.dbPort=atoi(sysconfig.oscConn.dbPort);
	if(strlen(sysconfig.oscConn.dbPwd)<20)
	{
		strcpy(p_oscconn.dbPwd,sysconfig.oscConn.dbPwd);
	}else{
		DataSafe._encrypt(sysconfig.oscConn.dbPwd,strlen(sysconfig.oscConn.dbPwd),p_oscconn.dbPwd);
	}
//监控端口
	p_shell.monitorPort = atoi(sysconfig.monitorPort);
	if(p_shell.monitorPort<=0)
	{
      p_shell.monitorPort=6000;
	}

//路径
	strcpy(p_videolog.filePath,sysconfig.videoPath);

//输出行
    memset(wzMaxOutRows,0,sizeof(wzMaxOutRows));	
	int maxOutRows=atoi(sysconfig.maxOutRows);
	wsprintf(wzMaxOutRows,L"%d",maxOutRows);

//警告时间
	p_shell.warningTimes_1=atoi(sysconfig.warningTimes_1);
    if(p_shell.warningTimes_1<=300)
	{
       p_shell.warningTimes_1=300;
	}
	
	p_shell.warningTimes_2=atoi(sysconfig.warningTimes_2);
    if(p_shell.warningTimes_2<=180)
	{
       p_shell.warningTimes_2=180;
	}

	p_shell.warningTimes_3=atoi(sysconfig.warningTimes_3);
    if(p_shell.warningTimes_3<=60)
	{
       p_shell.warningTimes_3=60;
	}
   
	//排序  ASC
	if(p_shell.warningTimes_1<p_shell.warningTimes_2)
	{
		warningTimes=p_shell.warningTimes_1;
		p_shell.warningTimes_1=p_shell.warningTimes_2;
		p_shell.warningTimes_2=warningTimes;
	}

	if(p_shell.warningTimes_1<p_shell.warningTimes_3)
	{
   	    warningTimes=p_shell.warningTimes_1;	
		p_shell.warningTimes_1=p_shell.warningTimes_3;	
		p_shell.warningTimes_3=warningTimes;
	}

	if(p_shell.warningTimes_2<p_shell.warningTimes_3)
	{
   	    warningTimes=p_shell.warningTimes_2;	
		p_shell.warningTimes_2=p_shell.warningTimes_3;	
		p_shell.warningTimes_3=warningTimes;
	}
//debug
		memset(szDebugTmp,0,sizeof(szDebugTmp));
		strcpy(szDebugTmp,sysconfig.level);
		if(strcmp(szDebugTmp,"debug")==0)
		{
			p_shell.debugLevel=DEBUG_LEVEL;   //debug
		}else
		if(strcmp(szDebugTmp,"info")==0)
		{
			p_shell.debugLevel=INFO_LEVEL;   //info
		}

    //getLogicDiskInfo();
//	CLogMan::OpenLogFile();
	CLogMan::setLogLevel(p_shell.debugLevel);
    
	//CLogMan::debug_log("param[%s,%s,%d]！",szAccessID,szClientIP,diskFlag) ;
	CLogMan::debug_log("auditfilePath[%s]！",auditfilePath) ;
//node
	p_loginlog.nodeFlag = atoi(sysconfig.code);
	strcpy(p_loginlog.authaccessresid,szAccessID);
  
	strcpy(p_access->id,szAccessID);
	memset(localLang,0,sizeof(localLang));
	/*{		//test tnsnames

		char tnsname[4096]={0};
		char plsql[4096]={0};
		memset(tnsname,0,sizeof(tnsname));
		OpenTNS(tnsname);
		sprintf(plsql,tnsname,"orcl1","192.168.1.1",1521,"orcl1");
		printf("%s",plsql);
		FILE * fp = fopen("c:\\test.txt","w+");
		if (fp != NULL)
		{
			fwrite(plsql,1,strlen(plsql),fp);
			fclose(fp);
		}
	}*/
	int retval=openDBHandle(&p_oscconn);
	if(retval==0)
	{
		CLogMan::debug_log("connect database success!");			
	    retval=p_shell.sqlHandle->DBQueryAuthAccessRes(&p_aares);//根据szAccessID填充p_aares
		

		char pr_key[1024];
		char pr_basekey[1024];
		if(p_shell.sqlHandle->DBQueryKey(pr_key,pr_basekey)>0 && _decrypt_key != NULL){
			char kekey[1024]={0};
			char allkey[1024]={0};
			memset(kekey,0,sizeof(kekey));
			memset(allkey,0,sizeof(allkey));
			memset(aes_key,0,sizeof(aes_key));
			_decrypt_key((BYTE*)pr_basekey,strlen(pr_basekey),(BYTE*)kekey,(BYTE*)const_key,0);
			if(strlen(kekey) == 8){
				sprintf(allkey,"%s%s",aes_const_key,kekey);
				_decrypt_key((BYTE*)pr_key,strlen(pr_key),(BYTE*)aes_key,(BYTE*)allkey,0);
			}
		}

		DWORD Session_id = 0;
		char windowssessionid[10] = {0};
		ProcessIdToSessionId(::GetCurrentProcessId(),&Session_id);
		memset(windowssessionid,0,sizeof(windowssessionid));
		sprintf(windowssessionid,"%d",Session_id);
		p_shell.sqlHandle->DBExecSessionID(szAccessID,windowssessionid);
		p_shell.sqlHandle->DBQueryLocalLang(localLang,szAccessID);

		char pr_cooperator[500]={0};
		char pr_accessMemo[200]={0};

		p_shell.sqlHandle->DBDBQueryAuthAccessResPR(p_loginlog.authaccessresid,pr_cooperator,pr_accessMemo);
		strcpy(p_loginlog.pr_cooperator,pr_cooperator);
		strcpy(p_loginlog.pr_accessMemo,pr_accessMemo);

		//2016-03-24 增加 往数据库里添加OSCSHELL运行日志
		//p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "OSCShellApp应用正常启动");
		//2016-03-24
	}
	closeDBHandle();
	if(retval<=0){
		char szMsg[256] = {0};
		sprintf(szMsg,"server:%s,userName:%s,dbName:%s,port:%d,szAccessID:%s,Unable to obtain valid data!",p_oscconn.dbHost,p_oscconn.dbUser,p_oscconn.dbName,p_oscconn.dbPort,szAccessID);			
		exitClear(szMsg);
		return NULL;	    	
	}
	if(strlen(p_access->clientAddr)==0)
	{
       strcpy(p_access->clientAddr,szClientIP);
	}
	CLogMan::info_log("动态访问ID[%s],客户端ip[%s]！",szAccessID,p_access->clientAddr) ;
	//2016-07-11 增加 往数据库里添加OSCSHELL运行日志
	/*int returnValue = openDBHandle(&p_oscconn);
	if (returnValue == 0)
	{
		char msg[100] = {0};
		sprintf(msg, "动态访问ID[%s],客户端ip[%s]！",szAccessID,p_access->clientAddr);
		p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, msg);
	}
	closeDBHandle();*/
	//2016-07-11

	if(GetVistitime(&retval, 0, NULL)<=0)
	{
		//2016-03-24 增加 往数据库里添加OSCSHELL运行日志
		/*int returnValue = openDBHandle(&p_oscconn);
		if (returnValue == 0)
		{
			p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "当前时间超出该资源的授权时间");
		}
		closeDBHandle();*/
		//2016-03-24
		if (GetLocalLang() == 0)
		{
			exitClear("超出时间范围！");
		}else{
			exitClear("Beyond the range of time！");
		}		  
		  return NULL;						
	}
	strcpy(p_videolog.clientAddr,p_access->clientAddr);
	strcpy(p_loginlog.clientAddr,p_access->clientAddr);
//accessType: 100 SSH,101 telnet,102 sftp,103 ftp,104 rdp,105 xmanager,106 pcsws,201 sqlplus,202 plsql,203 db2,204 ms sql,205 sybase,900 http,901 https,999 其它
    //获取访问方式
	int accessType=atoi(p_access->accessType);
	*accType = accessType;
	p_loginlog.accessType=accessType;
    
	if(accessType == 0){
		//2016-04-08 增加 往数据库里添加OSCSHELL运行日志
		/*int returnValue = openDBHandle(&p_oscconn);
		if (returnValue == 0)
		{
			p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "访问类型不能为空！");
		}
		closeDBHandle();*/
		//2016-04-08
		if (GetLocalLang() == 0)
		{
			exitClear("访问类型不能为空！");
		}else{
			exitClear("Access Type can not is NULL！");
		}
		 return NULL;	
	}

    getLocalAddress(p_loginlog.proxyAddr);

	if(strlen(p_access->proxyAddr)>0)
	{
	   strcpy(p_loginlog.proxyAddr,p_access->proxyAddr);	
	}

	if(strlen(p_access->suFlag)>0 && atoi(p_access->suFlag)==1)
	{
	   strcpy(p_loginlog.accountNo,p_access->accountNoEx);      
	   strcpy(p_videolog.accountNo,p_access->accountNoEx);
	}else
	{
	   strcpy(p_loginlog.accountNo,p_access->accountNo);
	   strcpy(p_videolog.accountNo,p_access->accountNo);
    }

//loginlog
	strcpy(p_loginlog.resAddr,p_access->resAddr);
	strcpy(p_loginlog.resId,p_access->resId);
	strcpy(p_loginlog.resName,p_access->resName);
	strcpy(p_loginlog.resType,p_access->resType);
	strcpy(p_loginlog.uid,p_access->uid);
	strcpy(p_loginlog.userName,p_access->userName);
    strcpy(p_loginlog.apId,p_access->apId);

//videolog
	strcpy(p_videolog.resAddr,p_access->resAddr);
	strcpy(p_videolog.resId,p_access->resId);
	strcpy(p_videolog.resName,p_access->resName);
	strcpy(p_videolog.uid,p_access->uid);
	strcpy(p_videolog.userName,p_access->userName);
	strcpy(p_videolog.userId,p_access->userId);


//loginId
	if(p_shell.debugLevel==DEBUG_LEVEL)   //debug model
	{
	   get32Id(p_loginlog.id);
	}else
	{
	   strcpy(p_loginlog.id,p_access->id);//from accessId
	}
//密码为空，默认不需要SSO
	int accountPwdNULL = strlen(p_access->accountPwd)>0?1:0;
	noPassword = accountPwdNULL == 0?0:1;
	if(strlen(p_access->accountPwd)>0)
	{
        if(strlen(p_access->accountPwd)<20||strstr(p_access->accountPwd," ")!=NULL)
		{
			strcpy(OSCresPassWord,p_access->accountPwd);
		}else
		{
		    /*DataSafe._encrypt*/DecryptData(p_access->accountPwd,strlen(p_access->accountPwd),OSCresPassWord);
			strcpy(p_access->accountPwd,OSCresPassWord);
		}	
	}

	if(strlen(p_access->accountPwdEx)>0)
	{
        if(strlen(p_access->accountPwdEx)<20||strstr(p_access->accountPwdEx," ")!=NULL)
		{
			strcpy(OSCresPassWordEx,p_access->accountPwdEx);
		}else
		{
	        /*DataSafe._encrypt*/DecryptData(p_access->accountPwdEx,strlen(p_access->accountPwdEx),OSCresPassWordEx);
			strcpy(p_access->accountPwdEx,OSCresPassWordEx);
		}
	}

	if(strlen(p_aares.execApp)==0)
	{
		//2016-04-08 增加 往数据库里添加OSCSHELL运行日志
		/*int returnValue = openDBHandle(&p_oscconn);
		if (returnValue == 0)
		{
			p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "执行访问应用不能为空！");
		}
		closeDBHandle();*/
		//2016-04-08
		if (GetLocalLang() == 0)
		{
			exitClear("执行访问应用不能为空！");
		}else{
			exitClear("Run Appliction can not be NULL！");
		}
		return NULL;	   
	}	
 
	if(accessType==TYPE_SFTP){
		//StartLoadWindow();
	}
	if(strlen(p_aares.execApp)>0 &&
		(
			(strlen(p_access->ssoFlag)>0 && atoi(p_access->ssoFlag)==1)
			||accessType==TYPE_SSH
			||accessType==TYPE_TELNET
			||accessType==TYPE_MSSQL
			||accessType==TYPE_SQLPLUS
			||accessType==TYPE_PLSQL
			||accessType==TYPE_DB2
			||accessType==TYPE_SYBASE
			||accessType==TYPE_MYSQL
			||accessType==TYPE_PQSQL
		)
	)
	{
        if(strlen(p_aares.execParam)<20||strstr(p_aares.execParam," ")!=NULL)
		{
			strcpy(szCmdparam,p_aares.execParam);
		}else{
	        /*DataSafe._encrypt*/DecryptData(p_aares.execParam,strlen(p_aares.execParam),szCmdparam);
			strcpy(p_aares.execParam,szCmdparam);
		}
		CLogMan::debug_log("accessType[%d]",accessType) ;
		CLogMan::debug_log("p_aares[%s]",p_aares.execParam);
		char* plocal = strstr(szCmdparam,"-haschild");//根据协议参数中是否配置haschild判断是否需要检察子进程
		CLogMan::debug_log("%d",*hasChild);
		*hasChild = 0;//检查子进程退出
		//2016-07-11 增加 往数据库里添加OSCSHELL运行日志
		/*int returnValue = openDBHandle(&p_oscconn);
		if (returnValue == 0)
		{
			char msg[100] = {0};
			sprintf(msg, "访问类型[%d]",accessType);
			p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, msg);
			strcpy(msg,"");
			sprintf(msg, "访问工具[%s]",p_aares.execApp);
			p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, msg);
			strcpy(msg,"");
			sprintf(msg, "访问参数[%s]",p_aares.execParam);
			p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, msg);
		}
		closeDBHandle();*/
		//2016-07-11
		if(plocal != NULL)
		{
			*plocal = 0;
			*hasChild  = 1;
		}

		//ssh工具访问密码加密
		/*if(accessType==TYPE_SSH || accessType==TYPE_TELNET)
		{
			//putty
			char * pw = strstr(szCmdparam,"-pw");
			if(pw != NULL)
			{
				pw+=3;
				while(*pw == ' ')
					pw++;
				if(*pw == '\"')
				{
					pw++;
					char* lastQuote = strrchr(pw,static_cast<int>('\"'));
					char password[260]={0};
					if(lastQuote != NULL)
					{				
						memcpy(password,pw,lastQuote-pw);
					}
					else
					{
						pw--;
						strcpy(password,pw);
					}
					char encryptedPwd[260]={0};
					DataSafe.encrypt(password,strlen(password),encryptedPwd);
					char *param = replace(szCmdparam,password,encryptedPwd);
					strcpy(szCmdparam,param);
				}
				else
				{
					char * nextBlank = strstr(pw," ");
					char password[260]={0};
					char encryptedPwd[260]={0};
					if (nextBlank != NULL)
					{
						memcpy(password,pw,nextBlank-pw);
					}
					else
					{
						strcpy(password,pw);
					}
					DataSafe.encrypt(password,strlen(password),encryptedPwd);
					char *param = replace(szCmdparam,password,encryptedPwd);
					strcpy(szCmdparam,param);
				}			
			}
			else
			{
				///L test@#AccessID /PASSWORD paraview 192.168.1.65 /p 22
				char *secureCRTPwd = "/PASSWORD"; 
				pw = strstr(szCmdparam,secureCRTPwd);
				if (pw != NULL)
				{
					pw+=strlen(secureCRTPwd);
					while(*pw == ' ')
						pw++;
					if(*pw == '\"')
					{
						pw++;
						char* lastQuote = strrchr(pw,static_cast<int>('\"'));
						char password[260]={0};
						if(lastQuote != NULL)
						{				
							memcpy(password,pw,lastQuote-pw);
						}
						else
						{
							pw--;
							char *nextBlank = strstr(pw," ");
							memcpy(password,pw,nextBlank-pw);
						}
						CLogMan::debug_log("secureCRT login pwd1:%s",password);
						char encryptedPwd[260]={0};
						DataSafe.encrypt(password,strlen(password),encryptedPwd);
						char *param = replace(szCmdparam,password,encryptedPwd);
						strcpy(szCmdparam,param);
					}
					else
					{
						char * nextBlank = strstr(pw," ");
						char password[260]={0};
						char encryptedPwd[260]={0};
						if (nextBlank != NULL)
						{
							memcpy(password,pw,nextBlank-pw);
						}
						else
						{
							strcpy(password,pw);
						}
						CLogMan::debug_log("secureCRT login pwd2:%s",password);
						DataSafe.encrypt(password,strlen(password),encryptedPwd);
						char *param = replace(szCmdparam,password,encryptedPwd);
						strcpy(szCmdparam,param);
					}
				}
			}
		}
		else if (accessType == TYPE_SFTP)
		{
				//sftp://testuser@#1489049043473177:@#"paraview@192.168.1.65:22
			char *colon = strstr(szCmdparam,":");
			if (colon != NULL)
			{	
				colon++;
				char* nextColon=strstr(colon,":");
				if (nextColon != NULL)
				{
					nextColon++;
					char* lastAt = strrchr(nextColon,static_cast<int>('@'));
					if (lastAt != NULL)
					{
						char password[260]={0};
						char encryptedPwd[260]={0};
						memcpy(password,nextColon,lastAt-nextColon);
						DataSafe.encrypt(password,strlen(password),encryptedPwd);
						char *param = replace(szCmdparam,password,encryptedPwd);
						strcpy(szCmdparam,param);
					}
				}
			}
		}*/
		if ((strcmp(p_aares.accessAuth.accountType, "0")  == 0 || strcmp(p_aares.accessAuth.accountType, "")  == 0) 
			&& (accessType == TYPE_SSH || accessType == TYPE_TELNET))
		{
			char *_param = replace(szCmdparam,"AccessID","AccessID@#NOSSO");
			strcpy(szCmdparam,_param);
		}
		else if (accountPwdNULL == 0 && accessType == TYPE_SFTP)
		{
			*szCmdparam = 0;
			p_access->ssoFlag[0] = '0';//默认sftp协议对应ssoFlag为1时，由代理机写loginlog
		}
		char *param = replace(szCmdparam,"AccessID",szAccessID);
		strcpy(szCmdparam,param);
		strcpy(p_aares.execParam,szCmdparam);	
		/*if(accessType==TYPE_SQLPLUS)
		{
			HINSTANCE hinstance;
			typedef int (*pConnectServer)(char * ip, int port, char * buffer);
			char buffer[1024] = {0};
			sprintf(buffer,"%s$502",szAccessID);

			hinstance = LoadLibrary(L"DBAutoDll.dll");
			pConnectServer ConnectServer = (pConnectServer)::GetProcAddress(hinstance,"ConnectServer");
			int port = (*ConnectServer)(p_aares.proxyHost,atoi(p_aares.proxyPort),buffer);
			char szPort[10] = {0};
			sprintf(szPort,"%d",port);
			char *param = replace(szCmdparam,"proxyp",szPort);
			strcpy(szCmdparam,param);
			param = replace(szCmdparam,"proxyhost",p_aares.proxyHost);
			strcpy(szCmdparam,param);

			strcpy(p_aares.execParam,szCmdparam);
			FreeLibrary(hinstance);
			//strcpy(p_aares.execApp,"D:\\Ora10InstantClient\\sqlplus.exe");
		}else */
		if (accessType == TYPE_MSSQL)
		{
			HINSTANCE hinstance;
			typedef int (*pConnectServer)(char * ip, int port, char * buffer);
			char buffer[1024] = {0};
			sprintf(buffer,"%s$503",szAccessID);

			hinstance = LoadLibrary(L"DBAutoDll.dll");
			pConnectServer ConnectServer = (pConnectServer)::GetProcAddress(hinstance,"ConnectServer");
			int port = (*ConnectServer)(p_aares.proxyHost,atoi(p_aares.proxyPort),buffer);
			if(port == 0)
			{
				//2016-03-28 增加 往数据库里添加OSCSHELL运行日志
				/*int returnValue = openDBHandle(&p_oscconn);
				if (returnValue == 0)
				{
					p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "获取代理端口失败，访问协议为MSSQL");
				}
				closeDBHandle();*/
				//2016-03-28
				CLogMan::debug_log("get proxyPort failed!");
			}
			char szPort[10] = {0};
			sprintf(szPort,"%d",port);
			char *param = replace(szCmdparam,"proxyp",szPort);
			strcpy(szCmdparam,param);
			param = replace(szCmdparam,"proxyhost",p_aares.proxyHost);
			strcpy(szCmdparam,param);
			sprintf(mssql_host,"%s,%d",p_aares.proxyHost,port);
			memset(szCmdparam,0,sizeof(szCmdparam));
			strcpy(p_aares.execParam,szCmdparam);
			FreeLibrary(hinstance);
			//账户密码为空，不需要SSO
			if (accountPwdNULL == 0)
			{
				*szCmdparam = 0;
			}
						
			/*}else{
				int port =0;
				if(openDBHandle(&p_oscconn)==0)
				{
					p_shell.sqlHandle->GetResourcePort(p_access->resId,503,&port);
				}
				closeDBHandle();

				char szPort[10] = {0};
				sprintf(szPort,"%d",port);
				char *param = replace(szCmdparam,"proxyp",szPort);
				strcpy(szCmdparam,param);
				param = replace(szCmdparam,"proxyhost",p_access->resAddr);
				strcpy(szCmdparam,param);

				strcpy(p_aares.execParam,szCmdparam);
			}*/
		
		}else if(accessType==TYPE_PLSQL||accessType==TYPE_SQLPLUS){
			CLogMan::debug_log("p_aares.execApp:[%s]\n",p_aares.execApp);
			int port = 0;
			char host[20]={0};
			////不走代理，用户自己填写用户名、密码（实例名）
			if (accountPwdNULL == 0)			
			{
				memcpy(host,p_aares.accessAuth.resAddr,strlen(p_aares.accessAuth.resAddr));
				if(openDBHandle(&p_oscconn)==0)
				{
					p_shell.sqlHandle->GetResourcePort(p_aares.accessAuth.resId,502,&port);
				}else{
					port = 1521;
				}
				closeDBHandle();
			}
			else
			{
				HINSTANCE hinstance;
				typedef int (*pConnectServer)(char * ip, int port, char * buffer);
				char buffer[1024] = {0};
				sprintf(buffer,"%s$502",szAccessID);

				hinstance = LoadLibrary(L"DBAutoDll.dll");
				pConnectServer ConnectServer = (pConnectServer)::GetProcAddress(hinstance,"ConnectServer");
				port = (*ConnectServer)(p_aares.proxyHost,atoi(p_aares.proxyPort),buffer);
				if(port == 0)
				{
					//2016-03-28 增加 往数据库里添加OSCSHELL运行日志
					/*int returnValue = openDBHandle(&p_oscconn);
					if (returnValue == 0)
					{
						p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "获取代理端口失败，访问协议为PLSQL或SQLPLUS");
					}
					closeDBHandle();*/
					//2016-03-28
					CLogMan::debug_log("get proxyPort failed!");
				}
				memcpy(host,p_aares.proxyHost,strlen(p_aares.proxyHost));
			}
				
			char plsql[4096] = {0};				
			char name[50]={0};
			char dbname[1024] = {0};
			char dbname12[1024] = {0};
			char serviceName[1024]={0};
			int checkVersionRet =1;
			int returnValue = openDBHandle(&p_oscconn);
			if (returnValue == 0)
			{
				checkVersionRet=p_shell.sqlHandle->GetServiceName(p_aares.accessAuth.accountId,p_aares.accessAuth.resId,serviceName);
				closeDBHandle();
			}
			if(checkVersionRet==0){
				CLogMan::debug_log("Oracle Version Checked: %d %s", checkVersionRet, serviceName);
				strcpy(name,"SERVICE_NAME");
				strcpy(dbname12,serviceName);
				if(strstr(szCmdparam,"[SERVICE_NAME]")!=NULL){
					char *param = replace(szCmdparam,"[SERVICE_NAME]","");
					strcpy(szCmdparam,param);
				}else if(strstr(szCmdparam,"[SID]")!=NULL){
					char *param = replace(szCmdparam,"[SID]","");
					strcpy(szCmdparam,param);
				}
				
				// if account no is sys, add string as sysdba
				CLogMan::debug_log("Account NO: %s", p_aares.accessAuth.accountNo);
				if(strstr(p_aares.accessAuth.accountNo,"[") != NULL){
					char *after=strstr(p_aares.accessAuth.accountNo, "[");
					char *account=replace(p_aares.accessAuth.accountNo, after,"");
					//CLogMan::debug_log("Account NO ---: %s", account);
					if(strcmp(account, "SYS") == 0){
						int len=strlen(szCmdparam);
						szCmdparam[len-1]='\0';
						char *para=strcat(szCmdparam, " as sysdba\"");
						strcpy(szCmdparam,para);
						//CLogMan::debug_log("Oracle cmd Checked-0: %s", szCmdparam);
					}
				}

				if(strstr(szCmdparam,"[") != NULL){
					char *p=strstr(szCmdparam, "[");
					char *q=strstr(p,"]");
					char *s=replace(szCmdparam, p, "");
					char *v=strcat(s,q+1);
					strcpy(szCmdparam,v);
					CLogMan::debug_log("Oracle cmd Checked: %s", szCmdparam);
				}
			}
			else{
				if(strstr(szCmdparam,"[SERVICE_NAME]")!=NULL){
					strcpy(name,"SERVICE_NAME");
					char *param = replace(szCmdparam,"[SERVICE_NAME]","");
					strcpy(szCmdparam,param);
				}else if(strstr(szCmdparam,"[SID]")!=NULL){
					strcpy(name,"SID");
					char *param = replace(szCmdparam,"[SID]","");
					strcpy(szCmdparam,param);
				}else{
					strcpy(name,"SERVICE_NAME");
				}				
			}

				strcpy(dbname,strstr(szCmdparam,"@") + 1);
				if (strstr(dbname,"\"") != NULL)
				{
					char *p = strstr(dbname,"\"");
					*(p) = NULL;
				}
				if (strstr(dbname," as sysdba") != NULL)
				{
					char *p = strstr(dbname," as sysdba");
					*(p) = NULL;
				}

				//2016-08-19 增加 随机产生随机数
				char headInfo[50] = {0};
				time_t t;
				srand((unsigned) time(&t));
				int randomNum = rand() % (99999999 - 10000000 + 1) + 10000000;
				if(checkVersionRet==0){
					sprintf(headInfo, "%s%d",dbname12, randomNum);
				}
				else{
					sprintf(headInfo, "%s%d",dbname, randomNum);
				}				
				
				char src[100] = {0};
				char des[100] = {0};
				sprintf(src, "@%s",dbname);
				sprintf(des, "@%s",headInfo);
				char *param = replace(szCmdparam, src, des);
				strcpy(szCmdparam,param);
				
				//2016-08-19
				char tnsname[4096];
				memset(tnsname,0,sizeof(tnsname));
				OpenTNS(tnsname);
				//2016-08-19 修改 dbname-->headInfo
				if(checkVersionRet==0){
					sprintf(plsql,tnsname,headInfo,host,port,name,dbname12);
				}
				else{
					sprintf(plsql,tnsname,headInfo,host,port,name,dbname);
				}
				char oclpath[1024] = {0};
				strcpy(oclpath,p_aares.execApp);
				char* slash = strrchr(oclpath, static_cast<int>('\\'));
				if (slash)
				{
					*(slash) = '\0';
				}
				slash = strrchr(oclpath, static_cast<int>('\\'));
				if (slash)
				{
					*(slash) = '\0';
				}
				strcat(oclpath,"\\Ora10InstantClient\\tnsnames.ora");
				CLogMan::debug_log("oclpath:[%s]\n",oclpath);
				CLogMan::debug_log("plsql:[%s]\n",plsql);
				//2016-07-11 增加 往数据库里添加OSCSHELL运行日志
				/*int returnValue = openDBHandle(&p_oscconn);
				if (returnValue == 0)
				{
					char msg[100] = {0};
					sprintf(msg, "tnsnames路径:[%s]",oclpath);
					p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, msg);
				}
				closeDBHandle();*/
				//2016-07-11
				//char mode[3]={0};
				//if(getFileOpenMode(oclpath) == 0)
				//	strcpy(mode,"a+");
				//else
				//	strcpy(mode,"w+");
				//2016-08-19 修改"w+" -->"a+"
				FILE * fp = fopen(oclpath,"a+");
				if (fp != NULL)
				{
					fwrite("\n",1, 1, fp);
					fwrite(plsql,1,strlen(plsql),fp);
					fwrite("\n",1, 1, fp);
					fclose(fp);
				}
				else
				{
					//2016-03-28 增加 往数据库里添加OSCSHELL运行日志
					/*int returnValue = openDBHandle(&p_oscconn);
					if (returnValue == 0)
					{
						char msg[100] = {0};
						sprintf(msg, "打开%s 失败", oclpath);
						p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, msg);
					}
					closeDBHandle();*/
					//2016-03-28
					CLogMan::debug_log("[%s] open failed!",oclpath);
				}
				if(accountPwdNULL == 0)//不走代理，用户自己填写用户名、密码（实例名）
				{
					*szCmdparam = 0;
				}
				FreeLibrary(hinstance);
				if(accessType==TYPE_PLSQL){
					char userpath[1024] = {0};
					strcpy(userpath,p_aares.execApp);
					char* slash = strrchr(userpath, static_cast<int>('\\'));
					if (slash)
					{
						*(slash) = '\0';
					}
					strcat(userpath,"\\Preferences\\Administrator\\user.prefs");
					WritePrivateProfileStringA("LogonHistory",NULL,NULL,userpath);
				}
				
			/*}else{
				int port =0;
				if(openDBHandle(&p_oscconn)==0)
				{
					p_shell.sqlHandle->GetResourcePort(p_access->resId,502,&port);
				}
				closeDBHandle();

				char plsql[4096] = {0};
				char name[50]={0};
				if(strstr(szCmdparam,"[SERVICE_NAME]")!=NULL){
					strcpy(name,"SERVICE_NAME");
					char *param = replace(szCmdparam,"[SERVICE_NAME]","");
					strcpy(szCmdparam,param);
				}else if(strstr(szCmdparam,"[SID]")!=NULL){
					strcpy(name,"SID");
					char *param = replace(szCmdparam,"[SID]","");
					strcpy(szCmdparam,param);
				}else{
					strcpy(name,"SERVICE_NAME");
				}

				char dbname[1024] = {0};
				strcpy(dbname,strstr(szCmdparam,"@") + 1);
				if (strstr(dbname,"\"") != NULL)
				{
					char *p = strstr(dbname,"\"");
					*(p) = NULL;
				}
				if (strstr(dbname," as sysdba") != NULL)
				{
					char *p = strstr(dbname," as sysdba");
					*(p) = NULL;
				}

				char tnsname[4096]={0};
				memset(tnsname,0,sizeof(tnsname));
				OpenTNS(tnsname);
				sprintf(plsql,tnsname,dbname,p_access->resAddr,port,name,dbname);
				char oclpath[1024] = {0};
				strcpy(oclpath,p_aares.execApp);
				char* slash = strrchr(oclpath, static_cast<int>('\\'));
				if (slash)
				{
					*(slash) = '\0';
				}
				slash = strrchr(oclpath, static_cast<int>('\\'));
				if (slash)
				{
					*(slash) = '\0';
				}
				strcat(oclpath,"\\Ora10InstantClient\\tnsnames.ora");
				FILE * fp = fopen(oclpath,"w+");
				if (fp != NULL)
				{
					fwrite(plsql,1,strlen(plsql),fp);
					fclose(fp);
				}
			}*/
		}else if(accessType==TYPE_DB2){	
			//if(strlen(p_access->ssoFlag)>0 && atoi(p_access->ssoFlag)==1){
				HINSTANCE hinstance;
				typedef int (*pConnectServer)(char * ip, int port, char * buffer);
				char buffer[1024] = {0};
				sprintf(buffer,"%s$501",szAccessID);

				hinstance = LoadLibrary(L"DBAutoDll.dll");
				pConnectServer ConnectServer = (pConnectServer)::GetProcAddress(hinstance,"ConnectServer");
				int port = (*ConnectServer)(p_aares.proxyHost,atoi(p_aares.proxyPort),buffer);
				if(port == 0)
				{
					//2016-03-28 增加 往数据库里添加OSCSHELL运行日志
					/*int returnValue = openDBHandle(&p_oscconn);
					if (returnValue == 0)
					{
						p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "获取代理端口失败，访问协议为DB2");
					}
					closeDBHandle();*/
					//2016-03-28
					CLogMan::debug_log("get proxyPort failed!");
				}
				char szPort[10] = {0};
				sprintf(szPort,"%d",port);
				char *param = replace(szCmdparam,"proxyp",szPort);
				strcpy(szCmdparam,param);
				param = replace(szCmdparam,"proxyhost",p_aares.proxyHost);
				strcpy(szCmdparam,param);

				strcpy(p_aares.execParam,szCmdparam);
				FreeLibrary(hinstance);
				//账户密码为空，不需要SSO
				if (accountPwdNULL == 0)
				{
					sprintf(szCmdparam," db2");
				}
			/*}else{
				int port =0;
				if(openDBHandle(&p_oscconn)==0)
				{
					p_shell.sqlHandle->GetResourcePort(p_access->resId,501,&port);
				}
				closeDBHandle();

				char szPort[10] = {0};
				sprintf(szPort,"%d",port);
				char *param = replace(szCmdparam,"proxyp",szPort);
				strcpy(szCmdparam,param);
				param = replace(szCmdparam,"proxyhost",p_access->resAddr);
				strcpy(szCmdparam,param);

				strcpy(p_aares.execParam,szCmdparam);
			}*/
			//removeAliasDB2();
			removeAliasDB2Thread(NULL);
			//HANDLE ThreadHandle = CreateThread(NULL,NULL,removeAliasDB2Thread,0,0,0);
			//WaitForSingleObject(ThreadHandle,INFINITE);
		}else if(accessType==TYPE_SYBASE){	
			//if(strlen(p_access->ssoFlag)>0 && atoi(p_access->ssoFlag)==1){
				HINSTANCE hinstance;
				typedef int (*pConnectServer)(char * ip, int port, char * buffer);
				char buffer[1024] = {0};
				sprintf(buffer,"%s$504",szAccessID);

				hinstance = LoadLibrary(L"DBAutoDll.dll");
				pConnectServer ConnectServer = (pConnectServer)::GetProcAddress(hinstance,"ConnectServer");
				int port = (*ConnectServer)(p_aares.proxyHost,atoi(p_aares.proxyPort),buffer);
				if(port == 0)
				{
					//2016-03-28 增加 往数据库里添加OSCSHELL运行日志
					/*int returnValue = openDBHandle(&p_oscconn);
					if (returnValue == 0)
					{
						p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "获取代理端口失败，访问协议为SYSBASE");
					}
					closeDBHandle();*/
					//2016-03-28
					CLogMan::debug_log("get proxyPort failed!");
				}
				char plsql[4096] = {0};
				char dbname[1024] = {0};
				strcpy(dbname,strstr(szCmdparam,"-S") + 2);
				CLogMan::debug_log("dbname[%s] ip[%s] port[%d]",dbname,p_aares.proxyHost,port) ;
				sprintf(p_aares.proxyPort,"%d",port);
				
				sprintf(plsql,SYSBASESQL,dbname,p_aares.proxyHost,port,p_aares.proxyHost,port);
				CLogMan::debug_log("SYSBASESQL:%s",plsql) ;
				char sysbasepath[1024] = {0};
				strcpy(sysbasepath,p_aares.execApp);
				char* slash = strrchr(sysbasepath, static_cast<int>('\\'));
				if (slash)
				{
					*(slash) = '\0';
				}
				slash = strrchr(sysbasepath, static_cast<int>('\\'));
				if (slash)
				{
					*(slash) = '\0';
				}
				if(StrStrIA(p_aares.execApp,SYBASE_AES) != NULL)
				{
					slash = strrchr(sysbasepath, static_cast<int>('\\'));
					if (slash)
					{
						*(slash) = '\0';
					}
				}
				strcat(sysbasepath,"\\ini\\sql.ini");
				CLogMan::debug_log("sysbasepath:%s",sysbasepath);
				//2016-07-11 增加 往数据库里添加OSCSHELL运行日志
				/*int returnValue = openDBHandle(&p_oscconn);
				if (returnValue == 0)
				{
					char msg[100] = {0};
					sprintf(msg, "sysbasepath:%s",sysbasepath);
					p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, msg);
				}
				closeDBHandle();*/
				//2016-07-11
				FILE * fp = fopen(sysbasepath,"w+");
				if (fp != NULL)
				{
					fwrite(plsql,1,strlen(plsql),fp);
					fclose(fp);
				}
				FreeLibrary(hinstance);
				//账户密码为空，不需要SSO
				if (accountPwdNULL == 0)
				{
					*szCmdparam = 0;
				}
			/*}else{
				int port =0;
				if(openDBHandle(&p_oscconn)==0)
				{
					p_shell.sqlHandle->GetResourcePort(p_access->resId,504,&port);
				}
				closeDBHandle();

				char plsql[4096] = {0};
				char dbname[1024] = {0};
				strcpy(dbname,strstr(szCmdparam,"-S") + 2);
				CLogMan::debug_log("dbname[%s] ip[%s] port[%d]",dbname,p_aares.proxyHost,port) ;

				sprintf(plsql,SYSBASESQL,dbname,p_access->resAddr,port,p_aares.proxyHost,port);
				CLogMan::debug_log("SYSBASESQL:%s",plsql) ;
				char sysbasepath[1024] = {0};
				strcpy(sysbasepath,p_aares.execApp);
				char* slash = strrchr(sysbasepath, static_cast<int>('\\'));
				if (slash)
				{
					*(slash) = '\0';
				}
				slash = strrchr(sysbasepath, static_cast<int>('\\'));
				if (slash)
				{
					*(slash) = '\0';
				}
				strcat(sysbasepath,"\\ini\\sql.ini");
				CLogMan::debug_log("sysbasepath:%s",sysbasepath);
				FILE * fp = fopen(sysbasepath,"w+");
				if (fp != NULL)
				{
					fwrite(plsql,1,strlen(plsql),fp);
					fclose(fp);
				}
			}*/
		}else if(accessType==TYPE_MYSQL){	
			//if(strlen(p_access->ssoFlag)>0 && atoi(p_access->ssoFlag)==1){
				HINSTANCE hinstance;
				typedef int (*pConnectServer)(char * ip, int port, char * buffer);
				char buffer[1024] = {0};
				sprintf(buffer,"%s$505",szAccessID);

				hinstance = LoadLibrary(L"DBAutoDll.dll");
				pConnectServer ConnectServer = (pConnectServer)::GetProcAddress(hinstance,"ConnectServer");
				int port = (*ConnectServer)(p_aares.proxyHost,atoi(p_aares.proxyPort),buffer);
				if(port == 0)
				{
					//2016-03-28 增加 往数据库里添加OSCSHELL运行日志
					/*int returnValue = openDBHandle(&p_oscconn);
					if (returnValue == 0)
					{
						p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "获取代理端口失败，访问协议为MYSQL");
					}
					closeDBHandle();*/
					//2016-03-28
					CLogMan::debug_log("get proxyPort failed!");
				}
				char szPort[10] = {0};
				sprintf(szPort,"%d",port);
				char *param = replace(szCmdparam,"proxyp",szPort);
				strcpy(szCmdparam,param);
				param = replace(szCmdparam,"proxyhost",p_aares.proxyHost);
				strcpy(szCmdparam,param);

				strcpy(p_aares.execParam,szCmdparam);
				FreeLibrary(hinstance);
				//strcpy(p_aares.execApp,"D:\\Program Files (x86)\\Navicat for MySQL\\navicat.exe");

				if (StrStrIA(p_aares.execApp,"MySQLWorkbench.exe") != NULL)
				{
					char conStr[256] = {0};
					sprintf(conStr,"-query %s",p_aares.accessAuth.resName);
					strcpy(szCmdparam,conStr);
					strcpy(p_aares.execParam,szCmdparam);

					char szPath[1024] = {0};
					char connections[4096] = {0};
					char realConnections[4096] = {0};
					GetModuleFileNameA(NULL,szPath,MAX_PATH);
					char* slash = strrchr(szPath, static_cast<int>('\\'));
					if (slash)
					{
						*(slash) = '\0';
					}
					strcat(szPath,"\\connections.xml");
					FILE * fp = fopen(szPath,"r");
					if (fp != NULL)
					{
						fread(connections,1,4096,fp);
						fclose(fp);
					}
					if(strlen(p_access->ssoFlag)>0 && atoi(p_access->ssoFlag)==1){
						sprintf(realConnections,connections,p_aares.proxyHost,port,p_aares.accessAuth.accountNo,p_aares.accessAuth.accountPwd,p_aares.accessAuth.resName);
					}else{
						sprintf(realConnections,connections,p_aares.proxyHost,port,p_aares.accessAuth.accountNo,"",p_aares.accessAuth.resName);
					}

					char szCfgPath[MAX_PATH] = {0};
					SHGetSpecialFolderPathA(NULL,szCfgPath,CSIDL_APPDATA,FALSE);
					strcat(szCfgPath,"\\MySQL\\Workbench\\connections.xml");

					fp = fopen(szCfgPath,"w+");
					if (fp != NULL)
					{
						fwrite(realConnections,1,strlen(realConnections),fp);
						fclose(fp);
					}
				}else if(StrStrIA(p_aares.execApp,"navicat.exe") != NULL){
					CLogMan::info_log("读取navicat.exe 配置");
					strcpy(szCmdparam,"");
					strcpy(p_aares.execParam,"");

					char szPath[1024] = {0};
					char connections[4096] = {0};
					char realConnections[4096] = {0};
					GetModuleFileNameA(NULL,szPath,MAX_PATH);
					char* slash = strrchr(szPath, static_cast<int>('\\'));
					if (slash)
					{
						*(slash) = '\0';
					}
					strcat(szPath,"\\navicat.cfg");
					FILE * fp = fopen(szPath,"r");
					if (fp != NULL)
					{
						fread(connections,1,4096,fp);
						fclose(fp);
					}
					sprintf(realConnections,connections,p_aares.accessAuth.resAddr,p_aares.proxyHost,port,p_aares.accessAuth.accountNo);
					//if(strlen(p_access->ssoFlag)>0 && atoi(p_access->ssoFlag)==1){
						strcpy(navicatPwd,p_aares.accessAuth.accountPwd);
					//}else{
					//	memset(navicatPwd,0,sizeof(navicatPwd));
					//}

					char szCfgPath[MAX_PATH] = {0};
					GetModuleFileNameA(NULL,szCfgPath,MAX_PATH);
					slash = strrchr(szCfgPath, static_cast<int>('\\'));
					if (slash)
					{
						*(slash) = '\0';
					}
					strcat(szCfgPath,"\\navicat.reg");

					fp = fopen(szCfgPath,"w+");
					if (fp != NULL)
					{
						fwrite(realConnections,1,strlen(realConnections),fp);
						fclose(fp);
					}
					//删除原先保留的注册表项
					//WinExec("reg delete \"HKEY_CURRENT_USER\\Software\\PremiumSoft\\Navicat\\Servers\" /f",SW_HIDE);
					/*if(ShellExecuteA(NULL, "open", "regedit", "delete \"HKEY_CURRENT_USER\\Software\\PremiumSoft\\Navicat\\Servers\" /f",NULL,SW_HIDE) == NULL)
					{
						CLogMan::error_log("regedit Failed [delete \"HKEY_CURRENT_USER\\Software\\PremiumSoft\\Navicat\\Servers\" /f") ;

					}*/
					//2016-02-26 增加提示信息
					if (port == 0)
					{
						CLogMan::info_log("数据库登录返回端口号为0，请检查代理程序dbproxyconnet是否启动");
						return NULL;
					}
					//2016-02-26**************

					//删除原先保留的注册表项
					SHDeleteKeyA(HKEY_CURRENT_USER, "Software\\PremiumSoft\\Navicat\\Servers");
					//2016-07-11 增加 往数据库里添加OSCSHELL运行日志
					/*int returnValue = openDBHandle(&p_oscconn);
					if (returnValue == 0)
					{
						p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "删除原先保留的注册表项");
						p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "准备写注册表");
					}
					closeDBHandle();*/
					//2016-07-11

					char szCmd[256] = {0};
					sprintf(szCmd," /s \"%s\"",szCfgPath);
					CLogMan::info_log("szCmd:%s", szCmd);
					//WinExec(szCmd,SW_HIDE);
					//2016-02-17 通过注册表API操作
					/*if(ShellExecuteA(NULL, "open", "regedit", szCmd,NULL,SW_HIDE) == NULL)
					{
						CLogMan::error_log("regedit Failed [%s]！",szCmd) ;
						
					}*/
					
					// 根键、子键名称、和到子键的句柄
					HKEY hRoot = HKEY_CURRENT_USER;
					char szSubKey[1024] = {0};
					sprintf(szSubKey, "Software\\PremiumSoft\\Navicat\\Servers\\%s", p_aares.accessAuth.resAddr);
					CLogMan::info_log("szSubKey:%s",szSubKey);
					HKEY hKey;// 打开指定子键
					DWORD dwDisposition = REG_CREATED_NEW_KEY;
					// 如果不存在创建
					LONG lRet =  RegCreateKeyExA(
						hRoot,
						szSubKey,
						0,
						NULL,
						REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
						NULL,
						&hKey,
						&dwDisposition
						);
					if(lRet != ERROR_SUCCESS)
					{
						//2016-07-11 增加 往数据库里添加OSCSHELL运行日志
						/*int returnValue = openDBHandle(&p_oscconn);
						if (returnValue == 0)
						{
							p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "创建navicat注册表子键失败");
						}
						closeDBHandle();*/
						//2016-07-11
						CLogMan::info_log("创建navicat注册表子键失败");
						return NULL;
					}
					// 创建新的键值，设置键值数据
					char *connType = "ctMYSQL";
					char *connTypeOra = "ctoBasic";
					char *mssqlAuthenMode = "mamSQLServer";
					char *askPassword = "true";
					int codePage = 0x0000fde9;
					int autoConnect = 0x00000001;
					char *pgsslmode = "smRequire";
					int useCharacterSet = 0x00000001;		// 修改默认值，由0变为1，解决中文乱码问题
					lRet = RegSetValueExA(hKey, "ConnType", 0, REG_SZ, (BYTE*)connType, strlen(connType));
					lRet = RegSetValueExA(hKey, "ConnTypeOra", 0, REG_SZ, (BYTE*)connTypeOra, strlen(connTypeOra));
					lRet = RegSetValueExA(hKey, "Host", 0, REG_SZ, (BYTE*)p_aares.proxyHost, strlen(p_aares.proxyHost));
					lRet = RegSetValueExA(hKey, "Port", 0, REG_DWORD, (BYTE*)&port, 4);
					lRet = RegSetValueExA(hKey, "MSSQLAuthenMode", 0, REG_SZ, (BYTE*)mssqlAuthenMode, strlen(mssqlAuthenMode));
					lRet = RegSetValueExA(hKey, "UserName", 0, REG_SZ, (BYTE*)p_aares.accessAuth.accountNo, strlen(p_aares.accessAuth.accountNo));
					lRet = RegSetValueExA(hKey, "Pwd", 0, REG_SZ, NULL, 0);
					lRet = RegSetValueExA(hKey, "AskPassword", 0, REG_SZ, (BYTE*)askPassword, strlen(askPassword));
					lRet = RegSetValueExA(hKey, "Codepage", 0, REG_DWORD, (BYTE*)&codePage, 4);
					lRet = RegSetValueExA(hKey, "AutoConnect", 0, REG_DWORD, (BYTE*)&autoConnect, 4);
					lRet = RegSetValueExA(hKey, "PGSSLMode", 0, REG_SZ, (BYTE*)pgsslmode, strlen(pgsslmode));
					lRet = RegSetValueExA(hKey, "UseCharacterSet", 0, REG_DWORD, (BYTE*)&useCharacterSet, 4);
					
					//打开注册表
					int rtnRegOpen = RegOpenKeyExA(hRoot, szSubKey, 0, KEY_ALL_ACCESS, &hKey);
					if (rtnRegOpen != ERROR_SUCCESS)
					{
						//2016-07-11 增加 往数据库里添加OSCSHELL运行日志
						/*int returnValue = openDBHandle(&p_oscconn);
						if (returnValue == 0)
						{
							p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "打开navicat注册表子键失败");
						}
						closeDBHandle();*/
						//2016-07-11
						CLogMan::info_log("打开navicat注册表子键失败%d", rtnRegOpen);
						return NULL;
					}
					// 关闭子键句柄
					RegCloseKey(hKey);
					//2016-02-17*********************************************************
				}
				//账户密码为空，不需要SSO
				if (accountPwdNULL == 0)
				{
					//2016-03-28 增加 往数据库里添加OSCSHELL运行日志
					/*int returnValue = openDBHandle(&p_oscconn);
					if (returnValue == 0)
					{
						p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "账户密码为空，不需要SSO，访问协议为MYSQL");
					}
					closeDBHandle();*/
					//2016-03-28
					*szCmdparam = 0;
					CLogMan::debug_log("account password NULL,don't SSO!\n");
				}
			/*}else{
				int port =0;
				if(openDBHandle(&p_oscconn)==0)
				{
					p_shell.sqlHandle->GetResourcePort(p_access->resId,505,&port);
				}
				closeDBHandle();

				char szPort[10] = {0};
				sprintf(szPort,"%d",port);
				char *param = replace(szCmdparam,"proxyp",szPort);
				strcpy(szCmdparam,param);
				param = replace(szCmdparam,"proxyhost",p_access->resAddr);
				strcpy(szCmdparam,param);

				strcpy(p_aares.execParam,szCmdparam);
			}*/
		}else if (accessType == TYPE_PQSQL)
		{
			int pq_port = 0;
			char dbName[1024] = {0};
			memset(dbName,0,sizeof(dbName));
			if(openDBHandle(&p_oscconn)==0)
			{
				//p_shell.sqlHandle->GetResourcePort(p_access->resId,507,&pq_port);
				p_shell.sqlHandle->GetResourceDBName(p_access->accountId,dbName);
			}else{
				//pq_port = 5432;
			}
			closeDBHandle();

			//if(strlen(p_access->ssoFlag)>0 && atoi(p_access->ssoFlag)==1){
				HINSTANCE hinstance;
				typedef int (*pConnectServer)(char * ip, int port, char * buffer);
				char buffer[1024] = {0};
				sprintf(buffer,"%s$507",szAccessID);

				hinstance = LoadLibrary(L"DBAutoDll.dll");
				pConnectServer ConnectServer = (pConnectServer)::GetProcAddress(hinstance,"ConnectServer");
				pq_port = (*ConnectServer)(p_aares.proxyHost,atoi(p_aares.proxyPort),buffer);
				if(pq_port == 0)
				{
					//2016-03-28 增加 往数据库里添加OSCSHELL运行日志
					/*int returnValue = openDBHandle(&p_oscconn);
					if (returnValue == 0)
					{
						p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "获取代理端口失败，访问协议为PQSQL");
					}
					closeDBHandle();*/
					//2016-03-28

					CLogMan::debug_log("get proxyPort failed!");
				}
				PostpresqlSSO(p_aares.proxyHost,pq_port,p_access->accountNo,p_access->accountPwd,dbName);
				strcpy(szCmdparam,"");
				strcpy(p_aares.execParam,szCmdparam);
				FreeLibrary(hinstance);
				//账户密码为空，不需要SSO
				if (accountPwdNULL == 0)
				{
					//2016-03-28 增加 往数据库里添加OSCSHELL运行日志
					/*int returnValue = openDBHandle(&p_oscconn);
					if (returnValue == 0)
					{
						p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "账户密码为空，不需要SSO，访问协议为PQSQL");
					}
					closeDBHandle();*/
					//2016-03-28
					*szCmdparam = 0;
					CLogMan::debug_log("account password NULL,don't SSO!\n");
				}
			/*}else{
				int port =0;
				if(openDBHandle(&p_oscconn)==0)
				{
					p_shell.sqlHandle->GetResourcePort(p_access->resId,505,&port);
				}
				closeDBHandle();

				PostpresqlSSO(p_access->resAddr,port,p_access->accountNo,p_access->accountPwd,dbName);
				strcpy(szCmdparam,"");
				strcpy(p_aares.execParam,szCmdparam);
			}*/
		}else if(accessType==TYPE_PLSQL_N||accessType==TYPE_SQLPLUS_N){
			
			int port =0;
			if(openDBHandle(&p_oscconn)==0)
			{
				p_shell.sqlHandle->GetResourcePort(p_access->resId,602,&port);
			}else{
				port = 1521;
			}
			closeDBHandle();
			char plsql[4096] = {0};
			char name[50]={0};
			char dbname[1024] = {0};
			char dbname12[1024] = {0};
			char serviceName[1024]={0};
			int checkVersionRet =1;
			int returnValue = openDBHandle(&p_oscconn);
			if (returnValue == 0)
			{
				checkVersionRet=p_shell.sqlHandle->GetServiceName(p_access->accountId,p_access->resId,serviceName);
				closeDBHandle();
			}
			if(checkVersionRet==0){
				CLogMan::debug_log("Oracle Version Checked: %d %s", checkVersionRet, serviceName);
				strcpy(name,"SERVICE_NAME");
				strcpy(dbname12,serviceName);
				if(strstr(szCmdparam,"[SERVICE_NAME]")!=NULL){
					char *param = replace(szCmdparam,"[SERVICE_NAME]","");
					strcpy(szCmdparam,param);
				}else if(strstr(szCmdparam,"[SID]")!=NULL){
					char *param = replace(szCmdparam,"[SID]","");
					strcpy(szCmdparam,param);
				}

				// if account no is sys, add string as sysdba
				if(strstr(p_aares.accessAuth.accountNo,"[") != NULL){
					char *after=strstr(p_aares.accessAuth.accountNo, "[");
					char *account=replace(p_aares.accessAuth.accountNo, after,"");
					if(strcmp(account, "SYS") == 0){
						char *para=strcat(szCmdparam, " as sysdba");
						strcpy(szCmdparam,para);
					}
				}

				if(strstr(szCmdparam,"[") != NULL){
					char *p=strstr(szCmdparam, "[");
					char *q=strstr(p,"]");
					char *s=replace(szCmdparam, p, "");
					char *v=strcat(s,q+1);
					strcpy(szCmdparam,v);
					CLogMan::debug_log("Oracle cmd Checked: %s", szCmdparam);
				}
			}
			else{
				if(strstr(szCmdparam,"[SERVICE_NAME]")!=NULL){
					strcpy(name,"SERVICE_NAME");
					char *param = replace(szCmdparam,"[SERVICE_NAME]","");
					strcpy(szCmdparam,param);
				}else if(strstr(szCmdparam,"[SID]")!=NULL){
					strcpy(name,"SID");
					char *param = replace(szCmdparam,"[SID]","");
					strcpy(szCmdparam,param);
				}else{
					strcpy(name,"SERVICE_NAME");
				}			
			}

			strcpy(dbname,strstr(szCmdparam,"@") + 1);
			if (strstr(dbname,"\"") != NULL)
			{
				char *p = strstr(dbname,"\"");
				*(p) = NULL;
			}
			if (strstr(dbname," as sysdba") != NULL)
			{
				char *p = strstr(dbname," as sysdba");
				*(p) = NULL;
			}

			//2016-08-19 增加 随机产生随机数
			char headInfo[50] = {0};
			time_t t;
			srand((unsigned) time(&t));
			int randomNum = rand() % (99999999 - 10000000 + 1) + 10000000;
			if(checkVersionRet==0){
				sprintf(headInfo, "%s%d",dbname12, randomNum);
			}
			else{
				sprintf(headInfo, "%s%d",dbname, randomNum);
			}

			char src[100] = {0};
			char des[100] = {0};
			sprintf(src, "@%s",dbname);
			sprintf(des, "@%s",headInfo);
			if(checkVersionRet==0){
				char *param = replace(szCmdparam, serviceName, des);
				strcpy(szCmdparam,param);
			}
			else{
				char *param = replace(szCmdparam, src, des);
				strcpy(szCmdparam,param);
			}

			char tnsname[4096]={0};
			memset(tnsname,0,sizeof(tnsname));
			OpenTNS(tnsname);
			if(checkVersionRet==0){
				sprintf(plsql,tnsname,headInfo,p_access->resAddr,port,name,dbname12);
			}
			else{
				sprintf(plsql,tnsname,headInfo,p_access->resAddr,port,name,dbname);
			}
			char oclpath[1024] = {0};
			strcpy(oclpath,p_aares.execApp);
			char* slash = strrchr(oclpath, static_cast<int>('\\'));
			if (slash)
			{
				*(slash) = '\0';
			}
			slash = strrchr(oclpath, static_cast<int>('\\'));
			if (slash)
			{
				*(slash) = '\0';
			}
			strcat(oclpath,"\\Ora10InstantClient\\tnsnames.ora");
			//2016-08-19 修改"w+" -->"a+"
			FILE * fp = fopen(oclpath,"a+");
			if (fp != NULL)
			{
				fwrite("\n",1, 1, fp);
				fwrite(plsql,1,strlen(plsql),fp);
				fwrite("\n",1, 1, fp);
				fclose(fp);
			}
			else
			{
				CLogMan::debug_log("[%s] open failed!",oclpath);
			}
			//账户密码为空，不需要SSO
			if (accountPwdNULL == 0)
			{
				//2016-04-11 增加 往数据库里添加OSCSHELL运行日志
				/*int returnValue = openDBHandle(&p_oscconn);
				if (returnValue == 0)
				{
					p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "账户密码为空，不需要SSO，访问协议为PLSQL or SQLPLUS");
				}
				closeDBHandle();*/
				//2016-04-11
				*szCmdparam = 0;
				CLogMan::debug_log("account password NULL,don't SSO!\n");
			}
		}
		//-protocol ssh -host 192.168.1.41 -user root -password rootroot -port 22 -command /usr/bin/gnome-session --display $DISPLAY
		if (accountPwdNULL == 0 && accessType == TYPE_XMANAGER)
		{
			char  szCmdparamTmp[512];
			memset(szCmdparamTmp,0,sizeof(szCmdparamTmp));
			char *pUser = strstr(szCmdparam,"-user");
			//char *pPwd = strstr(szCmdparam,"password");
			char *pPort = strstr(szCmdparam,"-port");
			if (pUser != NULL)
			{
				memcpy(szCmdparamTmp,szCmdparam,pUser-szCmdparam);
				if (pPort != NULL)
				{
					strcat(szCmdparamTmp,pPort);
					memset(szCmdparam,0,sizeof(szCmdparam));
					memcpy(szCmdparam,szCmdparamTmp,strlen(szCmdparamTmp));
				}
			}
			CLogMan::debug_log("xrcmdParam:%s\n",szCmdparam);
		}
        sprintf(szCmdline,"%s %s",p_aares.execApp,szCmdparam);
	}else{
		if(strlen(p_aares.execParam)<20||strstr(p_aares.execParam," ")!=NULL)
		{
			strcpy(szCmdparam,p_aares.execParam);
		}else{
	        /*DataSafe._encrypt*/DecryptData(p_aares.execParam,strlen(p_aares.execParam),szCmdparam);
			strcpy(p_aares.execParam,szCmdparam);
		}
        strcpy(szCmdline,p_aares.execApp);
		if(accessType==TYPE_DB2){	
			strcat(szCmdline," db2");
		}else if (accessType == TYPE_PQSQL){
			int pq_port = 0;
			char dbName[1024] = {0};
			memset(dbName,0,sizeof(dbName));
			if(openDBHandle(&p_oscconn)==0)
			{
				//p_shell.sqlHandle->GetResourcePort(p_access->resId,507,&pq_port);
				p_shell.sqlHandle->GetResourceDBName(p_access->accountId,dbName);
			}else{
				//pq_port = 5432;
			}
			closeDBHandle();

			HINSTANCE hinstance;
			typedef int (*pConnectServer)(char * ip, int port, char * buffer);
			char buffer[1024] = {0};
			sprintf(buffer,"%s$507",szAccessID);

			hinstance = LoadLibrary(L"DBAutoDll.dll");
			pConnectServer ConnectServer = (pConnectServer)::GetProcAddress(hinstance,"ConnectServer");
			pq_port = (*ConnectServer)(p_aares.proxyHost,atoi(p_aares.proxyPort),buffer);

			PostpresqlSSO(p_aares.proxyHost,pq_port,p_access->accountNo,p_access->accountPwd,dbName);
			strcpy(szCmdparam,"");
			strcpy(p_aares.execParam,szCmdparam);
		}else if (accessType == TYPE_PQSQL_N)
		{
			int pq_port = 0;
			char dbName[1024] = {0};
			memset(dbName,0,sizeof(dbName));
			if(openDBHandle(&p_oscconn)==0)
			{
				p_shell.sqlHandle->GetResourcePort(p_access->resId,TYPE_PQSQL_N,&pq_port);
				p_shell.sqlHandle->GetResourceDBName(p_access->accountId,dbName);
			}else{
				pq_port = 5432;
			}
			closeDBHandle();

			/*HINSTANCE hinstance;
			typedef int (*pConnectServer)(char * ip, int port, char * buffer);
			char buffer[1024] = {0};
			sprintf(buffer,"%s$507",szAccessID);

			hinstance = LoadLibrary(L"DBAutoDll.dll");
			pConnectServer ConnectServer = (pConnectServer)::GetProcAddress(hinstance,"ConnectServer");
			pq_port = (*ConnectServer)(p_aares.proxyHost,atoi(p_aares.proxyPort),buffer);*/

			//PostpresqlSSO(p_aares.proxyHost,pq_port,p_access->accountNo,p_access->accountPwd,dbName);
			PostpresqlSSO(p_access->resAddr,pq_port,p_access->accountNo,p_access->accountPwd,dbName);
		}
	}
	if (strlen(p_access->ssoFlag)==0 || atoi(p_access->ssoFlag)==0)
	{
		if(accessType == TYPE_XMANAGER || accessType == TYPE_FTP || accessType == TYPE_SFTP ){
			if (accountPwdNULL == 0)
			{
				sprintf(szCmdline,"%s",p_aares.execApp);
			}
			else
			{
				sprintf(szCmdline,"%s %s",p_aares.execApp,szCmdparam);
			}
		}
		if(accessType == TYPE_RDP){
			if(strlen(p_aares.execParam)<20||strstr(p_aares.execParam," ")!=NULL)
			{
				strcpy(szCmdparam,p_aares.execParam);
			}else{
				/*DataSafe._encrypt*/DecryptData(p_aares.execParam,strlen(p_aares.execParam),szCmdparam);
				strcpy(p_aares.execParam,szCmdparam);
			}
			sprintf(szCmdline,"%s %s",p_aares.execApp,szCmdparam);
		}
	}

	if(!MByteToWChar((CHAR*)szCmdline,wzCmdline,sizeof(wzCmdline)/sizeof(wzCmdline[0])))
	{
		char szMsg[256] = {0};
		sprintf(szMsg,"szAppName:%s to unicode failed",p_aares.execApp);
		exitClear(szMsg);
		return NULL;
	}

 //磁盘映射
    if(diskFlag == 1)
	{
		TCHAR wzMapDiskDir[250] = {0};
		char  szMapDiskDir[250] = {0};
		wcscat(wzMapDiskDir,szTempDir);
		wcscat(wzMapDiskDir,L"RegNetUser.bat");		
		CLogMan::WideCharToChar(wzMapDiskDir,szMapDiskDir) ;
		CLogMan::debug_log("RegNetUser[%s]！",szMapDiskDir) ;
		//启动应用
		if(ShellExecute(NULL,L"open",wzMapDiskDir,NULL,NULL,SW_HIDE) == NULL)
		{
			CLogMan::error_log("RegNetUser Failed [%s]！",szMapDiskDir) ;
			CLogMan::ClosLogFile();
			exitClear(NULL);
			return NULL;
		}
		Sleep(1000);
	}
	//RDP
    if(accessType == TYPE_RDP)
	{		
		TCHAR wzWinDomainName[40] = {0};
		if(!MByteToWChar((CHAR*)p_access->domainName,wzWinDomainName,sizeof(wzWinDomainName)/sizeof(wzWinDomainName[0])))
		{
			char szMsg[256] = {0};
			sprintf(szMsg,"WinDomain:%s to unicode failed",p_access->domainName);
			exitClear(szMsg);
			return NULL;
		}
		//资源ip
		TCHAR wzOSCresIp[40] = {0};
		if(!MByteToWChar((CHAR*)p_access->resAddr,wzOSCresIp,sizeof(wzOSCresIp)/sizeof(wzOSCresIp[0])))
		{
			char szMsg[256] = {0};
			sprintf(szMsg,"wzOSCresIp:%s to unicode failed",p_access->resAddr);
			exitClear(szMsg);
			return NULL;
		}
		//用户名
		TCHAR wzAccountNo[100] = {0};
		if(!MByteToWChar((CHAR*)p_access->accountNo,wzAccountNo,sizeof(wzAccountNo)/sizeof(wzAccountNo[0])))
		{
			char szMsg[256] = {0};
			sprintf(szMsg,"AccountNo:%s to unicode failed",p_access->accountNo);
			exitClear(szMsg);
			return NULL;
		}
		//解密后密码
		TCHAR wzOSCresPassWord[100] = {0};
		if(!MByteToWChar((CHAR*)OSCresPassWord,wzOSCresPassWord,sizeof(wzOSCresPassWord)/sizeof(wzOSCresPassWord[0])))
		{
			char szMsg[256] = {0};
			sprintf(szMsg,"OSCresPassWord:%s to unicode failed",OSCresPassWord);
			exitClear(szMsg);
			return NULL;
		}
		
		TCHAR wzRdpFileName[512] = {0};
		if(!MByteToWChar((CHAR*)p_loginlog.id,wzRdpFileName,sizeof(wzRdpFileName)/sizeof(wzRdpFileName[0])))
		{
			char szMsg[256] = {0};
			sprintf(szMsg,"wzRdpFileName:%s to unicode failed",p_loginlog.id);
			exitClear(szMsg);
			return NULL;
		}
		if (!(strlen(p_access->ssoFlag)>0 && atoi(p_access->ssoFlag)==1))
		{
			memset(wzAccountNo,0,sizeof(wzAccountNo));
			memset(wzOSCresPassWord,0,sizeof(wzOSCresPassWord));
		}

	    wcscat(wzRdpFileName,L".rdp");
		wcscpy(p_shell.wzFile1,szTempDir);
		wcscat(p_shell.wzFile1,wzRdpFileName);
		writeRDP(p_shell.wzFile1,wzWinDomainName,wzOSCresIp,wzAccountNo,wzOSCresPassWord,diskFlag,16,2);
		//2016-07-11 增加 往数据库里添加OSCSHELL运行日志
		/*int returnValue = openDBHandle(&p_oscconn);
		if (returnValue == 0)
		{
			p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "写RDP文件，访问协议为RDP");
		}
		closeDBHandle();*/
		//2016-07-11
		//	CLogMan::info_log("rdp file path[%s]",p_shell.wzFile1) ;
		wcscat(wzCmdline,L" ");
		wcscat(wzCmdline,L"\"");
		wcscat(wzCmdline,p_shell.wzFile1);
		wcscat(wzCmdline,L"\"");
	}else
    if(accessType == TYPE_AS400)   //AS 400
    {
		//资源ip
		TCHAR wzOSCresIp[40] = {0};
		if(!MByteToWChar((CHAR*)p_access->resAddr,wzOSCresIp,sizeof(wzOSCresIp)/sizeof(wzOSCresIp[0])))
		{
			char szMsg[256] = {0};
			sprintf(szMsg,"wzOSCresIp:%s to unicode failed",p_access->resAddr);
			exitClear(szMsg);
			return NULL;
		}
		//用户名
		TCHAR wzAccountNo[100] = {0};
		if(!MByteToWChar((CHAR*)p_access->accountNo,wzAccountNo,sizeof(wzAccountNo)/sizeof(wzAccountNo[0])))
		{
			char szMsg[256] = {0};
			sprintf(szMsg,"AccountNo:%s to unicode failed",p_access->accountNo);
			exitClear(szMsg);
			return NULL;
		}

		char tmpAppPath[256]={0};
		TCHAR wzTmpAppPath[512]={0};
		strcpy(tmpAppPath,p_aares.execApp);
		int appLen=strlen(tmpAppPath);
		for(int i=appLen-1;i>=0;i--)
		{
			if(tmpAppPath[i]=='\\'||tmpAppPath[i]=='/')
			{
				strcpy(&tmpAppPath[i+1],"Private\\");
				break;
			}
		}
		if(!MByteToWChar((CHAR*)tmpAppPath,wzTmpAppPath,sizeof(wzTmpAppPath)/sizeof(wzTmpAppPath[0])))
		{
			char szMsg[256] = {0};
			sprintf(szMsg,"AS400 PrivatePath:%s to unicode failed",tmpAppPath);
			exitClear(szMsg);
			return NULL;
		}

		writeWS(p_shell.wzFile1,szTempDir,wzOSCresIp,wzAccountNo,wzTmpAppPath);		
	//	CLogMan::info_log("AS400 file path[%s]",p_shell.wzFile1) ;
		wcscat(wzCmdline,L" ");
		wcscat(wzCmdline,p_shell.wzFile1);
    }

	time_t t;
	time(&t);
	CTime ctime(t);	
	int day = ctime.GetDay();
	int month = ctime.GetMonth();
	int year = ctime.GetYear();
	int hour = ctime.GetHour() ;
	int minutes = ctime.GetMinute() ;
	int second = ctime.GetSecond() ;

	strcpy(p_videolog.sessionId,p_loginlog.id);
	strcpy(p_videolog.id,p_loginlog.id);
	
	sprintf(p_videolog.operDate,"%04d-%02d-%02d %02d:%02d:%02d",year,month,day,hour,minutes,second);
	
	//2016-01-21 增加 通过RDP访问时，从数据库中获取代理机的时间作为p_loginlog中的loginDate
	char proxyTime[30] = {0};
	if(openDBHandle(&p_oscconn)==0)
	{
		p_shell.sqlHandle->DBQueryProxyHostTime(proxyTime, p_access->id);
	}
	closeDBHandle();

	strcpy(p_loginlog.loginDate, proxyTime);
	//2016-01-21 *********************************************

	//auditType: 1录像 0不录像
	if(strlen(p_access->auditType)>0)
	{
		p_shell.videoFlag=atoi(p_access->auditType);
	}

	if(strlen(p_access->auditFlag)>0)
	{
		p_shell.cmdFlag=atoi(p_access->auditFlag);
	}

//	int auditType=atoi(p_access->auditType);
	if(p_shell.videoFlag>0){

		
		int waitcount = 0;
		while(waitcount < 10){
			if(GetFileAttributesA(p_videolog.filePath) == INVALID_FILE_ATTRIBUTES){
				waitcount ++;
				Sleep(1000);
			}else{
				break;
			}
		}
		if(waitcount>=10)
			CLogMan::debug_log("录像目录不存在!") ;

  	    char videofileName[100] ={0};
		char temp[100]={0};
		char *tmp_Op=NULL;
		strcpy(temp,p_access->accountNo);		
		tmp_Op = replace(temp,"\\","-");
	    if(p_videolog.filePath[strlen(p_videolog.filePath)-1] != '\\')
	    {
		  strcat(p_videolog.filePath,"\\");
	    }
//		sprintf(videofileName,"%04d%02i%02i%02i%02i%02i-%s.avi",year,month,day,hour,minutes,second,p_access->uid,temp);
		sprintf(videofileName,"%s_%s_%s.avi",p_videolog.sessionId,p_access->uid,tmp_Op);
		//添加月份目录，视频录像按照月份目录存储
		char cdate[10] = {0};
		sprintf(cdate, "%04d%02d", year, month);

		strcat(p_videolog.filePath, cdate);
		//创建月份文件夹
		CreateDirectoryA(p_videolog.filePath, NULL);
		strcat(p_videolog.filePath, "\\");
		strcat(p_videolog.filePath,videofileName);
	    strcpy(szAvifile,p_videolog.filePath);	  
		
		//重新给filePath赋值
		memset(p_videolog.filePath, 0x0, 200);
		strcpy(p_videolog.filePath, cdate);
		strcat(p_videolog.filePath, "/");
		strcat(p_videolog.filePath, videofileName);

		CLogMan::debug_log("启动录像[%s]!",szAvifile) ;
		//2016-04-11 增加 往数据库里添加OSCSHELL运行日志
		/*int returnValue = openDBHandle(&p_oscconn);
		if (returnValue == 0)
		{
			char msg[100] = {0};
			sprintf(msg, "启动录像[%s]", szAvifile);
			p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, msg);
		}
		closeDBHandle();*/
		//2016-04-11
	}else{
		//2016-04-11 增加 往数据库里添加OSCSHELL运行日志
		/*int returnValue = openDBHandle(&p_oscconn);
		if (returnValue == 0)
		{
			p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "不启动录像!");
		}
		closeDBHandle();*/
		//2016-04-11
		CLogMan::debug_log("不启动录像!");
	}	

    //SSH TELNET
	if(accessType == TYPE_SSH|| 
	   accessType == TYPE_TELNET||
	   accessType == TYPE_SQLPLUS || 
	   accessType == TYPE_PLSQL || 
	   accessType == TYPE_DB2 ||
	   accessType == TYPE_MSSQL||
	   accessType == TYPE_SYBASE||
	   accessType == TYPE_MYSQL||
	   accessType == TYPE_PQSQL){

		p_loginlog.cmdFlag = p_shell.cmdFlag;
		strcpy(auditfileName,auditfilePath);
		if(strlen(auditfileName)==0)
		{
		    CLogMan::WideCharToChar(szTempDir,auditfileName) ;
			if(auditfileName[strlen(auditfileName)-1] != '\\')
			{
			  strcat(auditfileName,"\\");
			}
		}
		strcat(auditfileName,PATH_ACCESSNAME);
		strcat(auditfileName,p_videolog.sessionId);   //session参数方式传递

		if(openDBHandle(&p_oscconn)==0)
		{
		    p_shell.sqlHandle->writeAuthData(auditfileName,&p_aares);
		}
		closeDBHandle();

		if(accessType == TYPE_SSH|| 
			accessType == TYPE_TELNET){
				if(strcmp(p_access->resType , "17") == 0){
					char time[100] = {0};
					char filePath[1024] = {0};
					memset(time,0,sizeof(time));
					memset(filePath,0,sizeof(filePath));

					struct tm tm;
					tm = ltime();
					strftime(time, 24,"%Y%m%d%H%M%S", &tm);


					CreateDirectoryA( "c:\\inspectionLog", NULL);
					sprintf(filePath,"c:\\inspectionLog\\%s@%s_%s.log",p_access->resName,p_access->resAddr,time);

					HKEY hkey;
					LONG res; 
					DWORD datatype=REG_SZ; 

					res =::RegOpenKeyExA(HKEY_CURRENT_USER, 
						"Software\\SimonTatham\\PuTTY\\Sessions\\Default%20Settings", 0, 
						KEY_WRITE|KEY_READ, &hkey);

					if(res==ERROR_SUCCESS)
					{
						res = ::RegSetValueExA(hkey, "LogFileName", 0, datatype, (const BYTE *)filePath, strlen(filePath));
						RegCloseKey(hkey);
					}
				}else{
					HKEY hkey;
					LONG res; 
					DWORD datatype=REG_SZ; 
					char filePath[1024] = {0};
					memset(filePath,0,sizeof(filePath));

					res =::RegOpenKeyExA(HKEY_CURRENT_USER, 
						"Software\\SimonTatham\\PuTTY\\Sessions\\Default%20Settings", 0, 
						KEY_WRITE|KEY_READ, &hkey);

					if(res==ERROR_SUCCESS)
					{
						res = ::RegSetValueExA(hkey, "LogFileName", 0, datatype, (const BYTE *)filePath, strlen(filePath));
						RegCloseKey(hkey);
					}
				}
		}
		
		/*if((accessType == TYPE_SSH || accessType == TYPE_TELNET) && strlen(p_aares.proxyHost) == 0 && false)
		{
		    TCHAR wzAuditfileName[150] = {0};
			if(!MByteToWChar((CHAR*)auditfileName,wzAuditfileName,sizeof(wzAuditfileName)/sizeof(wzAuditfileName[0])))
			{
				char szMsg[256] = {0};
				sprintf(szMsg,"auditfileName:%s to unicode failed",auditfileName);
				exitClear(szMsg);
				return NULL;
			}
			wcscat(wzCmdline,L" -PFile:");  //标识
			wcscat(wzCmdline,wzAuditfileName);
			
			if(wcslen(wzMaxOutRows)>0)
			{
				wcscat(wzCmdline,L" -Rows:");  //标识
				wcscat(wzCmdline,wzMaxOutRows);				
			}
		}*/
	}

	STARTUPINFO   StartInfo;   
	PROCESS_INFORMATION     ProceInfo;   
	ZeroMemory(&StartInfo,sizeof(StartInfo));   
	ZeroMemory(&ProceInfo,sizeof(ProceInfo));   
	StartInfo.cb=sizeof(StartInfo); 
	StartInfo.dwFlags =STARTF_USESHOWWINDOW;
  //StartInfo.wShowWindow = SW_SHOW
  //StartInfo.wShowWindow = SW_MAXIMIZE;
    StartInfo.wShowWindow = showWindow;
	if (accessType == TYPE_SYBASE)
	{	
		if(StrStrIA(p_aares.execApp,SYBASE_AES) != NULL)
		{
			StartInfo.wShowWindow = SW_HIDE;
		}
	}

	HANDLE zosHandle = NULL;
	
	CLogMan::WideCharToChar(wzCmdline,szCmdline) ;
	CLogMan::debug_log("命令行参数[%s]！",szCmdline) ;
	//2016-07-11 增加 往数据库里添加OSCSHELL运行日志
	/*returnValue = openDBHandle(&p_oscconn);
	if (returnValue == 0)
	{
		char msg[100] = {0};
		sprintf(msg, "命令行参数[%s]",szCmdline);
		p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, msg);
	}
	closeDBHandle();*/
	//2016-07-11
	
	if(accessType==TYPE_HTTP||accessType==TYPE_HTTPS)
	{
		 if(strlen(p_access->ssoFlag)>0 && atoi(p_access->ssoFlag)==1 && strlen(p_aares.execParam)>0)
		 {
			CLogMan::debug_log("account:%s,pwd:%s,p_aares.execApp:%s, p_aares.execParam:%s",p_access->accountNo,p_access->accountPwd,p_aares.execApp,p_aares.execParam);
			PostData(p_aares.execApp,p_access->accountNo,p_access->accountPwd,p_aares.execParam,&ProceInfo.dwProcessId,&ProceInfo.hProcess,NULL);
			//2016-08-19 屏蔽
			//HMODULE hModule = LoadLibrary(L"SSO.dll");
			//if(hModule == NULL){
			//	CLogMan::debug_log("load library sso.dll failed! 3400");
			//	//AfxMessageBox("SSO.dll");
			//	return NULL;
			//}

			//pfnSSOApplication fnSSOApplication = (pfnSSOApplication)GetProcAddress(hModule,"fnSSOApplication");
			//if (fnSSOApplication == NULL)
			//{
			//	CLogMan::debug_log("load library sso.dll failed! 3408");
			//	FreeLibrary(hModule);
			//	return NULL;
			//}
			//CLogMan::debug_log("p_aares.execApp:%s, p_access->accountNo:%s, p_access->accountPwd:%s", p_aares.execApp, p_access->accountNo, p_access->accountPwd);
			//fnSSOApplication(p_aares.execParam, p_aares.execApp, NULL, NULL, p_access->accountNo,p_access->accountPwd, NULL, &ProceInfo.dwProcessId,&ProceInfo.hProcess);
			//CLogMan::debug_log("call fnSSOApplication end!");
			//2016-08-19
		 }else
		 {
			if(strlen(p_access->ssoFlag)>0 && atoi(p_access->ssoFlag)==1)	
			{
				/*char *title_op[3];
				char title_buf1[100];
				char title_buf2[100];
				memset(title_buf1,0,sizeof(title_buf1));
				memset(title_buf2,0,sizeof(title_buf2));
				memset(title_op,0,sizeof(title_op));
				title_op[0]=title_buf1;
				title_op[1]=title_buf2;
				title_op[2]=0;
				sprintf(title_buf1,"连接到 %s",p_access->resAddr);
				strcpy(title_buf2,"Windows Security");

				PostData(p_aares.execApp,p_access->accountNo,p_access->accountPwd,NULL,&ProceInfo.dwProcessId,&ProceInfo.hProcess,title_op);
			    Sleep(1000);
				
				strcat(title_buf1,"|");
				strcat(title_buf1,title_buf2);

				char snTempDir[1024] = {0};
				CLogMan::WideCharToChar(szTempDir,snTempDir) ;
				strcat(snTempDir,"cmdStart.exe");
				char snCmdline[1024] = {0};
				sprintf(snCmdline,"3 \"%s\" \"%s\" \"%s\"",title_buf1,p_access->accountNo,p_access->accountPwd);
				ShellExecuteA(NULL,"open",snTempDir,snCmdline,NULL,SW_HIDE);
			//	DailogSubmit(title_buf,p_access->accountNo,p_access->accountPwd);*/
				SHELLEXECUTEINFOA ShExecInfo = {0};
				ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
				ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
				ShExecInfo.hwnd = NULL;
				ShExecInfo.lpVerb = NULL;
				ShExecInfo.lpFile = "iexplore.exe";
				ShExecInfo.lpParameters = szCmdline;
				ShExecInfo.lpDirectory = NULL;
				ShExecInfo.nShow = SW_SHOW;
				ShExecInfo.hInstApp = NULL;
				ShellExecuteExA(&ShExecInfo);
				ProceInfo.hProcess = ShExecInfo.hProcess;
				ProceInfo.dwProcessId = GetProcessId(ShExecInfo.hProcess);

				HWND hWnd=GetWindowByProcessId(0,L"Windows 安全",10);
				if(hWnd == NULL){
					hWnd=GetWindowByProcessId(0,L"Windows Security",5);
					if (hWnd == NULL)
					{
						TCHAR title_msg[100] = {0};
						wsprintf(title_msg,L"连接到 %s",p_access->resAddr);
						hWnd=GetWindowByProcessId(0,title_msg,2);
						if (hWnd != NULL)
						{
							WindowsSecurity(hWnd);
						}
					}else{
						WindowsSecurity(hWnd);
					}
				}else{
					WindowsSecurity(hWnd);
				}
				
			}else
			{
				PostData(p_aares.execApp,p_access->accountNo,p_access->accountPwd,NULL,&ProceInfo.dwProcessId,&ProceInfo.hProcess,NULL);
			}
		 }	
	}else if(accessType==TYPE_RADMIN){
		TCHAR wzExeApp[2048] = {0};
		MByteToWChar(p_aares.execApp,wzExeApp,sizeof(wzExeApp)/sizeof(wzExeApp[0]));
		CreateProcess(wzExeApp,   //lpApplicationName:   PChar   
			wzCmdline,   //lpCommandLine:   PChar   
			NULL,   //lpProcessAttributes:   PSecurityAttributes   
			NULL,   //lpThreadAttributes:   PSecurityAttributes   
			//	TRUE,   //bInheritHandles:   BOOL   
			FALSE,
			CREATE_NEW_CONSOLE|NORMAL_PRIORITY_CLASS,
			//	CREATE_NEW_CONSOLE,   
			NULL,   
			NULL,   
			&StartInfo,   
			&ProceInfo); 
	}else if(accessType==TYPE_XEN || accessType==TYPE_IPMI){
		StartInfo.wShowWindow = SW_MAXIMIZE;
		CreateProcess(wzCmdline,   //lpApplicationName:   PChar   
			NULL,   //lpCommandLine:   PChar   
			NULL,   //lpProcessAttributes:   PSecurityAttributes   
			NULL,   //lpThreadAttributes:   PSecurityAttributes   
			//	TRUE,   //bInheritHandles:   BOOL   
			FALSE,
			CREATE_NEW_CONSOLE|NORMAL_PRIORITY_CLASS,
			//	CREATE_NEW_CONSOLE,   
			NULL,   
			NULL,   
			&StartInfo,   
			&ProceInfo); 
	}else if(accessType == TYPE_ZOS){
		HINSTANCE hinstance= LoadLibrary(L"ZOSsso.dll");
		if(hinstance !=NULL){
			int port = 0;
			if(openDBHandle(&p_oscconn)==0)
			{
				p_shell.sqlHandle->GetResourcePort(p_access->resId,TYPE_ZOS,&port);
			}else{
				port = 23;
			}
			closeDBHandle();

			pZOSsso ZOSsso = (pZOSsso)::GetProcAddress(hinstance,"ZOSsso");
			zosHandle = ZOSsso(p_access->resAddr,port,p_access->accountNo,p_access->accountPwd);

			if(zosHandle == NULL)
			{
				CLogMan::error_log("OSCSHELL Failed [can not start ZOS client]！") ;
				CLogMan::ClosLogFile();
				exitClear(NULL);
				return NULL;
			}
		}
		CloseHandle(hinstance);

		PROCESSENTRY32 procEntry;
		memset(&procEntry,0,sizeof(procEntry));
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnap == INVALID_HANDLE_VALUE)
		{
			CLogMan::error_log("OSCSHELL Failed [can not start ZOS client 1 ]！") ;
			CLogMan::ClosLogFile();
			exitClear(NULL);
			return NULL;
		}
		procEntry.dwSize = sizeof(PROCESSENTRY32);
		if (!Process32First(hSnap, &procEntry))
		{
			CLogMan::error_log("OSCSHELL Failed [can not start ZOS client 2 ]！") ;
			CLogMan::ClosLogFile();
			exitClear(NULL);
			return NULL;
		}
		do
		{
			if (wcsicmp(procEntry.szExeFile,L"pcsws.exe") == 0)
			{
				ProceInfo.dwProcessId = procEntry.th32ProcessID;
				break;
			}
		} while (Process32Next(hSnap, &procEntry));
	
	}
	//2016-03-22 增加 通过SSO.dll兼容第三方访问工具 StrStrIA(p_aares.execParam, ".exe") != NULL && StrStrIA(p_aares.execApp,"navicat.exe") == NULL
	else if (accessType == TYPE_UNDEFINE)
	{
		//2016-03-24 增加 从数据库里获取访问该资源所用协议对于的端口
		int exe_port = 0;
		if(openDBHandle(&p_oscconn)==0)
		{
			p_shell.sqlHandle->GetResourcePort(p_access->resId,TYPE_UNDEFINE,&exe_port);
		}else{
			exe_port = 0;
		}
		closeDBHandle();
		char port[20] = {0}; 
		sprintf(port,"%d",exe_port);
		CLogMan::debug_log("Get EXE protocol port:%s",port);
		//2016-03-24
		HMODULE hModule = LoadLibrary(L"SSO.dll");
		if(hModule == NULL){
			CLogMan::debug_log("load library sso.dll failed! 3721");
			//AfxMessageBox("SSO.dll");
			return NULL;
		}

		pfnSSOApplication fnSSOApplication = (pfnSSOApplication)GetProcAddress(hModule,"fnSSOApplication");
		if (fnSSOApplication == NULL)
		{
			CLogMan::debug_log("load library sso.dll failed! 3729");
			FreeLibrary(hModule);
			return NULL;
		}
		CLogMan::debug_log("p_aares.execParam:%s, p_aares.execApp:%s, p_access->resAddr:%s, p_access->accountNo:%s, p_access->accountPwd:%s",p_aares.execParam, p_aares.execApp, p_access->resAddr, p_access->accountNo, p_access->accountPwd);
		fnSSOApplication(p_aares.execParam, p_aares.execApp, p_access->resAddr, port, p_access->accountNo,p_access->accountPwd, NULL, &ProceInfo.dwProcessId,&ProceInfo.hProcess);
		CLogMan::debug_log("ProceInfo.dwProcessId:%d ProceInfo.hProcess:%08x", ProceInfo.dwProcessId, ProceInfo.hProcess);
		CLogMan::debug_log("call fnSSOApplication end!");
	}
	//2016-03-22
	else
	{
		HANDLE gHnd = NULL;
		int isToad = strstr(szCmdline,"Toad.exe") != NULL ? 0:1;
		if (isToad == 0)
		{
			char* globlMutexName = "Global\\mutexForOSCToadAccess";
mutexWait:
			gHnd = CreateMutexA(NULL,FALSE,globlMutexName);
			if (GetLastError() == ERROR_ALREADY_EXISTS )
			{
				CLogMan::debug_log("error gMutext:%s already exist,wait!\n",globlMutexName);
				if (gHnd!= NULL && !CloseHandle(gHnd))
				{
					CLogMan::debug_log("close existed gMutex failed(%d)\n",GetLastError());
				}
				Sleep(10);
				goto mutexWait;
			}
		}
		CreateProcess(NULL,   //lpApplicationName:   PChar   
			wzCmdline,   //lpCommandLine:   PChar   
			NULL,   //lpProcessAttributes:   PSecurityAttributes   
			NULL,   //lpThreadAttributes:   PSecurityAttributes   
		//	TRUE,   //bInheritHandles:   BOOL   
			FALSE,
     		CREATE_NEW_CONSOLE|NORMAL_PRIORITY_CLASS,
		//	CREATE_NEW_CONSOLE,   
			NULL,   
			NULL,   
			&StartInfo,   
			&ProceInfo);
		//2016-07-11 增加 往数据库里添加OSCSHELL运行日志
		/*int returnValue = openDBHandle(&p_oscconn);
		if (returnValue == 0)
		{
			p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "启动访问工具");
		}
		closeDBHandle();*/
		//2016-07-11
		if(isToad == 0)
		{
			HWND hWnd= GetWindowByProcessId(ProceInfo.dwProcessId,CLASS_TOAD,60);
			if (hWnd == NULL)
			{
				CLogMan::debug_log("%s not found,try again!!!\n\n",CLASS_TOAD);
				hWnd = GetWindowByProcessId(ProceInfo.dwProcessId,CLASS_TOAD,60);
				if(hWnd == NULL){
					CLogMan::debug_log("%s not found,quit!!!\n\n",CLASS_TOAD);
				}
			}
			if (gHnd != NULL)
			{
				CloseHandle(gHnd);
			}
		}
	}
	*newtype = accessType;
//	CloseHandle(ProceInfo.hThread);
	CLogMan::debug_log("PID[%d] hProcess[%08x]！",ProceInfo.dwProcessId,ProceInfo.hProcess);
	//2016-04-11 增加 往数据库里添加OSCSHELL运行日志
	/*if (ProceInfo.dwProcessId == 0)
	{
		int returnValue = openDBHandle(&p_oscconn);
		if (returnValue == 0)
		{
			p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "访问工具在发布机上未成功启动，程序出错");
		}
		closeDBHandle();
	}*/
	//2016-04-11
	/*ProceInfo.hProcess = OpenProcess(PROCESS_ALL_ACCESS,FALSE,ProceInfo.dwProcessId);
	if(ProceInfo.hProcess == NULL)
	{
		memset(szCmdline,0,sizeof(szCmdline));
		CLogMan::WideCharToChar(wzCmdline,szCmdline) ;
		CLogMan::error_log("OSCSHELL Failed [%s]！",szCmdline) ;
		CLogMan::ClosLogFile();
		exitClear(NULL);
		return NULL;
	}*/

	//处理DB
	if((accessType == TYPE_SQLPLUS || 
	   accessType == TYPE_PLSQL || 
	   accessType == TYPE_DB2 ||
	   accessType == TYPE_MSSQL||
	   accessType == TYPE_SYBASE||
	   accessType == TYPE_MYSQL
	   ))
	{
	    p_loginlog.cmdFlag = p_shell.cmdFlag;
        Sleep(2000);	    
		memset(&p_cmdInfo,0,sizeof(STRUCT_DBAUDITINFO));
	    p_cmdInfo.flag=0;  //audit
		p_cmdInfo.debug=0; //0:no log and only write db 1: log and db 2: log and no write db		
		strcpy(p_cmdInfo.sessionId,p_videolog.sessionId);
		char accessFile[512]={0};	
		int pid_count=0;
		unsigned long pid=0;
				
		p_cmdInfo.pid=ProceInfo.dwProcessId;
		pid=p_cmdInfo.pid;
		strcpy(accessFile,auditfilePath);
		if(accessType==TYPE_DB2)
		{  //db2 audit
			int oldpid = 0;
			pid=getSessionProcessPID(L"db2.exe");
			while(pid==0 || pid != oldpid)
			{
				Sleep(2000);
				oldpid = pid;
				pid_count++;
				if(pid_count>10)
				{
					CLogMan::error_log("DB2 not found db2bp process,db2cmd pid[%d]！",ProceInfo.dwProcessId) ;
					CLogMan::ClosLogFile();
					exitClear(NULL);
					return NULL;
				}
				pid=getSessionProcessPID(L"db2.exe");		
			}
			p_cmdInfo.pid=pid;
			ProceInfo.dwProcessId=pid;	
			ProceInfo.hProcess = OpenProcess(PROCESS_ALL_ACCESS,FALSE,pid);
			CLogMan::debug_log("db2bp PID[%d] hProcess[%08x]！",pid,ProceInfo.hProcess);
		}
		//if (strstr(szCmdline,"Toad.exe") != NULL)
		//{
		//	CreateThread(NULL,NULL,RefalshOracleToadTitleThread,(LPVOID)&ProceInfo.dwProcessId,0,0);
		//}
		//else if(accessType==TYPE_SQLPLUS || accessType == TYPE_PLSQL)
		//{
		//	CreateThread(NULL,NULL,RefalshOracleTitleThread,(LPVOID)&ProceInfo.dwProcessId,0,0);
	    //}
		sprintf(p_cmdInfo.accessPath,"%s%s%s",auditfilePath,PATH_ACCESSNAME,p_cmdInfo.sessionId);
		sprintf(p_cmdInfo.auditPath,"%s%s%s",auditfilePath,PATH_AUDITNAME,p_cmdInfo.sessionId);

		CLogMan::error_log("loadAudit pid[%d], p_cmdInfo.pid[%d]！", pid, p_cmdInfo.pid);

		/*if(loadDbAudit(&p_cmdInfo,accessFile,pid)!=0)
		{
			CLogMan::error_log("loadAudit Failed pid[%d]！", p_cmdInfo.pid) ;
		}else
		{
		   CLogMan::error_log("loadAudit success,accessFile[%s]！", accessFile) ;
		}*/
		MByteToWChar(accessFile,p_shell.wzFile2,sizeof(p_shell.wzFile2)/sizeof(p_shell.wzFile2[0]));	


		if (accessType==TYPE_MSSQL){
			Sleep(1000);
			CLogMan::debug_log("start find mssql title");
			//2016-07-11 增加 往数据库里添加OSCSHELL运行日志
			/*int returnValue = openDBHandle(&p_oscconn);
			if (returnValue == 0)
			{
				p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "开始查找MSSQL窗口标题");
			}*/
			//closeDBHandle();
			//2016-07-11
			HWND hWnd=GetWindowByProcessId(ProceInfo.dwProcessId,TITLE_MSSQL,60);
			if(hWnd==NULL)
			{
				//2016-07-11 增加 往数据库里添加OSCSHELL运行日志
				/*p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "未找到MSSQL窗口标题");
				closeDBHandle();*/
				//2016-07-11
				exitClear("mssql application window not display!");
				return NULL;
			}
			//2016-07-11 增加 往数据库里添加OSCSHELL运行日志
			/*p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "MSSQLSSO");
			closeDBHandle();*/
			//2016-07-11
			if(strlen(p_access->ssoFlag)>0 && atoi(p_access->ssoFlag)==1){
				MSSQLSSO(hWnd,mssql_host,p_access->accountNo,p_access->accountPwd);
			}else{
				MSSQLSSO(hWnd,mssql_host,p_access->accountNo,"");
			}
		}
		if(StrStrIA(p_aares.execApp,"navicat.exe") != NULL && strlen(navicatPwd) > 0){
			Sleep(1000);
			CLogMan::debug_log("start find navicat title");
			//2016-07-11 增加 往数据库里添加OSCSHELL运行日志
			/*int returnValue = openDBHandle(&p_oscconn);
			if (returnValue == 0)
			{
				p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "开始查找MYSQL窗口标题");
			}*/
			//closeDBHandle();
			//2016-07-11
			HWND hWnd=GetWindowByProcessId(ProceInfo.dwProcessId,CALSS_NAVICAT,60);
			if(hWnd==NULL)
			{
				//2016-07-11 增加 往数据库里添加OSCSHELL运行日志
				/*p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "未找到MYSQL窗口标题");
				closeDBHandle();*/
				//2016-07-11
				exitClear("navicat application window not display!");
				return NULL;
			}
			//2016-07-11 增加 往数据库里添加OSCSHELL运行日志
			/*p_shell.sqlHandle->DBInsertAuthMessage(p_loginlog.authaccessresid, p_aares.accessAuth.uid, p_aares.accessAuth.userName, p_aares.accessAuth.resAddr, p_aares.accessAuth.accountNo,p_aares.accessAuth.clientAddr, "NavicatMysqlSSO");
			closeDBHandle();*/
			//2016-07-11

			//2016-03-21 修改
			NavicatMysqlSSO(hWnd);
			//HMODULE hModule = LoadLibrary(L"SSO.dll");
			//if(hModule == NULL){
			//	CLogMan::debug_log("load library sso.dll failed! 3721");
			//	//AfxMessageBox("SSO.dll");
			//	return NULL;
			//}

			//pfnSSOApplication fnSSOApplication = (pfnSSOApplication)GetProcAddress(hModule,"fnSSOApplication");
			//if (fnSSOApplication == NULL)
			//{
			//	CLogMan::debug_log("load library sso.dll failed! 3729");
			//	FreeLibrary(hModule);
			//	return NULL;
			//}
			//CLogMan::debug_log("p_aares.execParam:%s p_aares.execApp:%s, p_access->accountNo:%s, p_access->accountPwd:%s",p_aares.execParam, p_aares.execApp, p_access->accountNo, p_access->accountPwd);
			//fnSSOApplication("navicat.exe_73", NULL, NULL, NULL, NULL,p_access->accountPwd, NULL, &ProceInfo.dwProcessId,&ProceInfo.hProcess);
			//CLogMan::debug_log("call fnSSOApplication:navicat.exe end!");
			//2016-03-21
		}
		if (accessType == TYPE_SYBASE)
		{	
			if(StrStrIA(p_aares.execApp,SYBASE_AES) != NULL)
			{
				CLogMan::debug_log("start find SYBASE_AES title");
				HWND hWnd=GetWindowByProcessId(0,SYBASE_AES_TITLE,10);
				if(hWnd==NULL)
				{
					exitClear("SYBASE_AES application window not display!");
					return NULL;
				}
				SybaseAesSSO(hWnd);
			}

		}

	}else if(accessType == TYPE_SSH || accessType == TYPE_TELNET){
		
		if (strstr(p_aares.execApp,"putty.exe") != NULL)
		{
			CreateThread(NULL,NULL,RefalshPuttyTitleThread,(LPVOID)&ProceInfo.dwProcessId,0,0);
		}else if(strstr(p_aares.execApp,"SecureCRT.exe") != NULL){
			CreateThread(NULL,NULL,RefalshSCRTTitleThread,(LPVOID)&ProceInfo.dwProcessId,0,0);
		}
	}else if(accessType == TYPE_SFTP){
		//CreateThread(NULL,NULL,RefalshSCPTitleThread,(LPVOID)&ProceInfo.dwProcessId,0,0);
		
	}else if(accessType==TYPE_XEN){
		Sleep(2000);
		HWND hWnd=GetWindowByProcessId(ProceInfo.dwProcessId,CLASS_XenCenter,60);
		if(hWnd==NULL)
		{
			exitClear("XenCenter application window not display!");
			return NULL;
		}
		XenCenterSSO(hWnd);

	}else if(accessType==TYPE_IPMI){
		Sleep(2000);
		HWND hWnd=GetWindowByProcessId(0,TITLE_IPMI,30);
		if(hWnd==NULL)
		{
			exitClear("IPMI application window not display!");
			return NULL;
		}
		IPMISSO(hWnd);
	}else
	if(accessType == TYPE_AS400 && strlen(p_access->ssoFlag)>0 && atoi(p_access->ssoFlag)==1)            //as 400
	{
		HWND hWnd=GetWindowByProcessId(ProceInfo.dwProcessId,CLASS_AS400,10);
		if(hWnd==NULL)
		{
			exitClear("PCSWS application window not display!");
			return NULL;
		}
	    CreateThread(NULL,NULL,AS400Thread,(LPVOID)hWnd,0,0);
	}else if (accessType == TYPE_RADMIN){
		HWND hWnd=GetWindowByProcessId(ProceInfo.dwProcessId,TITLE_RADMIN,30);
		if(hWnd!=NULL)
		{
			edit_count = 0;
			::EnumChildWindows(hWnd,EnumChildProc,0);
			while(edit_count==0){
				Sleep(1000);
			}
			if (accountPwdNULL != 0)
				RadminSSO(hWnd);
		}
		
	}else if (accessType == TYPE_RDP)
	{
		/*HWND hWnd = GetSubWindowByProcessId(ProceInfo.dwProcessId,"IHWindowClass","Input Capture Window",30);
		if(hWnd!=NULL)
		{
			CLogMan::debug_log("Find Input Capture Window");
			CreateThread(NULL,NULL,RDPThread,(LPVOID)hWnd,0,0);
		}else{
			CLogMan::debug_log("Not find Input Capture Window");
		}*/
	
	}else if(accessType == TYPE_XMANAGER )   //xmanager
	{

		/*char tmpAppPath[512]={0};
		char tmpAppParam[512]={0};
		TCHAR wzTmpAppPath[512]={0};
		strcpy(tmpAppPath,p_aares.execApp);
		int appLen=strlen(tmpAppPath);
		for(int i=appLen-1;i>=0;i--)
		{
			if(tmpAppPath[i]=='\\'||tmpAppPath[i]=='/')
			{
				strcpy(&tmpAppPath[i+1],"Xstart.exe");
				break;
			}
		}
		STARTUPINFOA si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si,sizeof(STARTUPINFOA));   
		ZeroMemory(&pi,sizeof(PROCESS_INFORMATION));   
		si.cb = sizeof(STARTUPINFOA);
		si.dwFlags =STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_SHOWNORMAL;
		GetStartupInfoA(&si);

		if(!CreateProcessA(NULL,   //lpApplicationName:   PChar   
			tmpAppPath,   //lpCommandLine:   PChar   
			NULL,   //lpProcessAttributes:  PSecurityAttributes   
			NULL,   //lpThreadAttributes:   PSecurityAttributes   
			FALSE,
     		CREATE_NEW_CONSOLE|NORMAL_PRIORITY_CLASS,
			NULL,   
			NULL,   
			&si,   
			&pi))
		{
			exitClear("Xstart application run failure!");
			return NULL;

		}
		if(strlen(p_access->ssoFlag)>0 && atoi(p_access->ssoFlag)==1){
			const char *SSH_PARAM="={ssh}";
			const char *SSH_PORT_PARAM="={port:ssh}";
			const char *TELNET_PARAM="={telnet}";
			const char *TELNET_PORT_PARAM="={port:telnet}";
			char *TELNET_PROTOCOL="telnet";
			char *SSH_PROTOCOL="ssh";

			int  offsit_len=5;
			char protocolType[20]={0};

			memset(tmpAppParam,0,sizeof(tmpAppParam));
			memset(protocolType,0,sizeof(protocolType));
			char *op=strstr(p_aares.execParam,SSH_PARAM);  //only use ssh
			if(op==NULL)
			{
				op=strstr(p_aares.execParam,SSH_PORT_PARAM);  //both use ssh and port
				if(op==NULL)
				{
  					op=strstr(p_aares.execParam,TELNET_PARAM);  //only use telnet
					if(op==NULL)
					{
					   op=strstr(p_aares.execParam,TELNET_PORT_PARAM); //both use telnet and port
					   if(op!=NULL)
					   {
						   strcpy(protocolType,TELNET_PROTOCOL);
						   offsit_len=strlen(TELNET_PORT_PARAM); 
					   }
					}else{
					   strcpy(protocolType,TELNET_PROTOCOL);
					   offsit_len=strlen(TELNET_PARAM); 
					}
				}else
				{
					offsit_len=strlen(SSH_PORT_PARAM);
					strcpy(protocolType,SSH_PROTOCOL);
				}
			}else
			{
				offsit_len=strlen(SSH_PARAM);
				strcpy(protocolType,SSH_PROTOCOL);
			}
		    
			char *protocol_op=SSH_PROTOCOL;
			if(op!=NULL)
			{
				char localParam[100]={0};
				protocol_op=protocolType;
				if(offsit_len==strlen(SSH_PORT_PARAM)||offsit_len==strlen(TELNET_PORT_PARAM))
				{
					if(getLocalAddrByPing(p_access->resAddr,localParam)==0)
					{			
						int ip_len=strlen(localParam);
						DWORD xmanager_port=GetPortFromPid(ProceInfo.dwProcessId);
						if(xmanager_port>=6000)
						{
							xmanager_port-=6000;
						}
										
						if(xmanager_port<0)
						{
						   exitClear("get Xmanager id failure!");
						   return NULL;
						}
						sprintf(&localParam[ip_len],":%ld.0",xmanager_port);
						memcpy(tmpAppParam,p_aares.execParam,op-p_aares.execParam+1);
						strcat(tmpAppParam,localParam);
						strcat(tmpAppParam,op+offsit_len);
					}else
					{
					   exitClear("ping dest addr error!");
					   return NULL;
					}
				}else
				{
					memcpy(tmpAppParam,p_aares.execParam,op-p_aares.execParam);
					strcat(tmpAppParam,op+offsit_len);				
				}
			}else
			{
				strcpy(tmpAppParam,p_aares.execParam);
			}

			HWND hWnd=GetWindowByProcessId(pi.dwProcessId,CLASS_XMANAGER,10);
			if(hWnd==NULL)
			{
				exitClear("Xstart application window not display!");
				return NULL;
			}
			CLogMan::debug_log("Xstart 参数[account:%s,pwd:%s,addr:%s,param:%s]！",p_access->accountNo,p_access->accountPwd,p_access->resAddr,tmpAppParam) ;

	 //     XmangerSubmit(hWnd,p_access->accountNo,p_access->accountPwd,p_access->resAddr,tmpAppParam,protocol_op);
	   
			char snTempDir[1024] = {0};
			CLogMan::WideCharToChar(szTempDir,snTempDir) ;
			strcat(snTempDir,"cmdStart.exe");
			char snCmdline[1024] = {0};
			sprintf(snCmdline,"1 Xstart \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"",p_access->accountNo,p_access->accountPwd,p_access->resAddr,tmpAppParam,protocol_op);
			ShellExecuteA(NULL,"open",snTempDir,snCmdline,NULL,SW_HIDE);
		
//		if(p_shell.debugLevel!=DEBUG_LEVEL)
//		{
		    ShowWindow(hWnd,SW_HIDE);

			
			hWnd=GetWindowByProcessId(pi.dwProcessId,TITLE_XMANAGER,1);
			if(hWnd!=NULL)
			{
				ShowWindow(hWnd,SW_HIDE);
			}
//		}
		}
		p_shell.otherProcess=pi.hProcess;*/


	}
	
//	CLogMan::ClosLogFile();
	p_shell.dwProcessId = ProceInfo.dwProcessId;
	//if(localOsc == 0)
	//{
	//	processId = new DWORD[16];
	//	memset(processId,0,sizeof(DWORD)*16);
	//	childs = 0;
	//	getChildProcessID(ProceInfo.dwProcessId,processId,16,childs);
	//}
	//else
	//{
	//	processId = new DWORD(ProceInfo.dwProcessId);
	*processId = ProceInfo.dwProcessId;
	p_shell.resProcess=ProceInfo.hProcess;
	p_shell.resThread=ProceInfo.hThread;

	if(zosHandle != NULL){
		return zosHandle;
	}
	CLogMan::debug_log("ProceInfo.hProcess:%08x\r\n",ProceInfo.hProcess) ;
	return ProceInfo.hProcess;
}


int checkTimes(CTime ctime,char *sztime_begin,char *sztime_end)
{

	if(sztime_begin==NULL||sztime_end==NULL)
	{
		return 0;
	}
	//得到当前时间

	int year,month,day,hour,minute,second; 

	//比较当前时间是否在此时间段内
	sscanf(sztime_begin,"%04d-%02d-%02d %02d:%02d:%02d",&year,&month,&day,&hour,&minute,&second); 
	CTime tm_begin(year,   month,   day,   hour,   minute,   second); 
	sscanf(sztime_end,"%04d-%02d-%02d %02d:%02d:%02d",&year,&month,&day,&hour,&minute,&second); 
	CTime tm_end(year,   month,   day,   hour,   minute,   second); 

	//如果不在时间段任何时间段内
	if(ctime < tm_begin || ctime > tm_end)
	{
		return 0;
	}			

	//比较还差多少 还剩5分钟时警告 返回秒数
	CTimeSpan tm_left = tm_end - ctime;
	return tm_left.GetTotalSeconds();
}

int GetVistitime(int *timeout,int flag, int *delayFlag)
{
	//time_t t;
	//time(&t);
	//CTime ctime(t);

	int firstFlag=0;
    int timeFlag=0;   
	int accountType=0;  //1 个人 2特权
	int year = 0,month = 0,day = 0,hour = 0,minute = 0,second = 0; 
	
	//获取数据库中代理机的时间
	char proxyTime[30] = {0};
	if(openDBHandle(&p_oscconn)==0)
	{
		if (p_shell.sqlHandle->DBQueryProxyHostTime(proxyTime, p_access->id) == -1)
		{
			*timeout = 0;
			return 0;
		}
	}
	closeDBHandle();

	sscanf(proxyTime, "%04d-%02d-%02d %02d:%02d:%02d", &year, &month, &day, &hour, &minute, &second);
	CTime proxyHostTime(year, month, day, hour, minute, second);
	CLogMan::info_log("代理机时间为:%s",proxyTime);
	*timeout=0;
    
	if(strlen(p_aares.timeAuth.startTime)==0) 
	{
        return 0;
	}
	

	if(p_access!=NULL && strlen(p_access->accountType)>0) 
	{
	   accountType=atoi(p_access->accountType);
	}
//自填账户密码登陆，没有账号类型
	//if(accountType==0){
	//   return 0;
	//}
/*
	if(strlen(p_aares.timeAuth.timeFlag)>0) 
	{
		timeFlag=atoi(p_aares.timeAuth.timeFlag);
	}
*/
	if(strlen(p_aares.timeAuth.endTime)==0)
	{
	     *timeout=65500;
         return 1; 
	}

	STRUCT_TIMEAUTH tauth;
	memset(&tauth,0,sizeof(STRUCT_TIMEAUTH));
	memcpy(&tauth,&p_aares.timeAuth,sizeof(STRUCT_TIMEAUTH));
	if(flag==1)
	{
		if(openDBHandle(&p_oscconn)==0)
		{
		    p_shell.sqlHandle->DBQueryTimeAuth(&tauth,p_access->id);
		}
		closeDBHandle();
	}

	//sscanf(tauth.startTime,"%04d-%02d-%02d %02d:%02d:%02d",&year,&month,&day,&hour,&minute,&second); 
	//CTime tm_begin(year,   month,   day,   hour,   minute,   second);
	//2016-02-22 增加 比较上次保存的授权结束时间跟这次授权时间
	if (delayFlag != NULL && strcmp(lastEndTime, "") != 0 && strcmp(lastEndTime, tauth.endTime) != 0)
	{
		*delayFlag = 4;
	}
	//2016-02-22**************************************************

	sscanf(tauth.endTime,"%04d-%02d-%02d %02d:%02d:%02d",&year,&month,&day,&hour,&minute,&second); 
	CTime tm_end(year,   month,   day,   hour,   minute,   second);
	CLogMan::info_log("授权结束时间为:%s",tauth.endTime);
	
	//2016-02-22 增加 保存上次授权结束时间，以判断用户是否延长授权结束时间
	strcpy(lastEndTime, tauth.endTime);
	//2016-02-22****************************************************************

	//开始时间不需要比较，web端已经比较过
	if(proxyHostTime > tm_end)
	{
		return 0;
	}			

	CTimeSpan tm_left;
	if(strlen(tauth.timeFlag)>0)
	{
		if(strcmp(p_aares.timeAuth.timeFlag,tauth.timeFlag)!=0)
		{
			firstFlag=1;
			strcpy(p_aares.timeAuth.timeFlag,tauth.timeFlag);
		}
        timeFlag=atoi(tauth.timeFlag);
	}
	tm_left = tm_end - proxyHostTime;	
	*timeout=tm_left.GetTotalSeconds();
	CLogMan::info_log("当前代理机时间：%s 离授权结束时间：%s的秒数为:%d",proxyTime, tauth.endTime, *timeout);
	if(firstFlag==1 && (timeFlag==16||timeFlag==9))
	{
		 if(timeFlag==9 && strlen(tauth.resetPwdTime)>0)
		 {
			sscanf(tauth.resetPwdTime,"%04d-%02d-%02d %02d:%02d:%02d",&year,&month,&day,&hour,&minute,&second); 
			CTime tm_resetPwdTime(year,   month,   day,   hour,   minute,   second); 
			tm_left = tm_resetPwdTime - proxyHostTime;
     		*timeout=tm_left.GetTotalSeconds()/60;  //账号密码已被申请，最多可申请延时XX分钟的延时
		 }
	     return (0-timeFlag);
	}
	return timeFlag;
}

extern "C" __declspec(dllexport) int __stdcall GetVistitimeEx(int *timeout,int *delayFlag)//用于查询开始时间-结束时间
{
   int retval=GetVistitime(timeout, 1, delayFlag);   
   unsigned int time_out=*timeout;
   
   return retval;
}

int __stdcall RemoveMapInfo(int aviflag)  //用于移除临时信息
{
    int  ret=0;
    if(p_shell.debugLevel!=DEBUG_LEVEL)
	{
		ret=openDBHandle(&p_oscconn);
		if(ret==0)
		{
			p_shell.sqlHandle->DBExecAuthAccessRes(&p_aares,1);  //delete accessID
		}
		closeDBHandle();
	}

	return ret;
}

bool static checkSessionStatus(DWORD  dwSessionID){
	char *ppBuffer=NULL;
	DWORD pBytesReturned;
	bool isDisconn=false;
	if(WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE,
		dwSessionID,
		WTSConnectState,
		&ppBuffer,
		&pBytesReturned))
	{
		if(*((INT *)ppBuffer) == WTSDisconnected){
			isDisconn=true;
		}
	}
	WTSFreeMemory( ppBuffer );
	return isDisconn;
}

unsigned _stdcall PingProxyHostThread(void* pArguments)
{
	char localParam[100]={0};
	if(getLocalAddrByPing(p_access->clientAddr, localParam) == 0)
	{
		if(strcmp(p_access->clientAddr,localParam) == 0)
			CLogMan::debug_log("ping %s success!",p_access->clientAddr);
		else
			CLogMan::debug_log("recv data from %s",localParam);
	}
	else
		CLogMan::debug_log("Ping %s error!",p_access->clientAddr);
	return 0;
}

extern   "C" __declspec(dllexport) int __stdcall DoLoginLog(int act)
{
	//PWTS_PROCESS_INFOA pProcessInfo=NULL;
	//DWORD   ProcessCount;
	//if(WTSEnumerateProcessesA(WTS_CURRENT_SERVER_HANDLE,0,1,&pProcessInfo,&ProcessCount))
	//{

	//	for(DWORD i   =   0;i <ProcessCount;i++) {
	//		if (pProcessInfo[i].ProcessId != GetCurrentProcessId())
	//		{
	//			continue;
	//		}

	//		unsigned int nThreadId;
	//		HANDLE handle = (HANDLE)_beginthreadex(NULL,0,PingProxyHostThread,NULL,0,&nThreadId);
	//		CloseHandle(handle);
	//		if(checkSessionStatus(pProcessInfo[i].SessionId))
	//		{
	//			CLogMan::debug_log("会话断开");
	//			break;
	//		}
	//		else
	//		{
	//			CLogMan::debug_log("会话正常");
	//			break;
	//		}
	//	}
	//}
	//WTSFreeMemory(pProcessInfo);

	/*char *ppBuffer=NULL;
	DWORD pBytesReturned;
	if(WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSWinStationName, &ppBuffer, &pBytesReturned))
	{
		CLogMan::debug_log("会话名字：%s",ppBuffer);
	}
	WTSFreeMemory(ppBuffer);*/

	int accessType=atoi(p_access->accessType);
	if(noPassword == 1 && (accessType == TYPE_SSH|| 
		accessType == TYPE_TELNET||
		(accessType == TYPE_SFTP && (strlen(p_access->ssoFlag)>0 && atoi(p_access->ssoFlag)==1))||
		(accessType == TYPE_FTP && (strlen(p_access->ssoFlag)>0 && atoi(p_access->ssoFlag)==1))||
		accessType == TYPE_SQLPLUS || 
		accessType == TYPE_PLSQL || 
		accessType == TYPE_DB2 ||
		accessType == TYPE_MSSQL||
		accessType == TYPE_SYBASE||
		accessType == TYPE_MYSQL||
		accessType == TYPE_PQSQL)){
				CLogMan::debug_log("[DoLoginLog][return 0]");
				return 0;
	}
	if (noPassword == 0 &&(accessType == TYPE_SSH || accessType == TYPE_TELNET))
	{
		CLogMan::debug_log("[DoLoginLog][return 0]");
		return 0;
	}
	EnterCriticalSection(&g_csA);
#ifdef SYSLOG
	if (act == 1)//退出,get pr_name from pr_accessprotocol for syslog //sugl
	{
		db_mysql *oscSqlHandle=new db_mysql(&p_oscconn);
		if(oscSqlHandle->conndb()==0)
		{
			CLogMan::debug_log("[oscSqlHandle][oscSqlHandle->getResNamebyResType][%s]",p_loginlog.apId);
			oscSqlHandle->getResNamebyResType(p_loginlog.apId,p_loginlog.accessName);
		}else{
			CLogMan::debug_log("[oscSqlHandle][conndb failed]");
		}
		delete oscSqlHandle;
	}
#endif
	//获取数据库中代理机的时间
	char proxyTime[30] = {0};
	if(openDBHandle(&p_oscconn)==0)
	{
		p_shell.sqlHandle->DBQueryProxyHostTime(proxyTime, p_access->id);
	}
	closeDBHandle();
	//2016-01-21 增加 将获取的代理机时间+10s，作为退出时间，解决当用户刚单点登录进去，
	//立即关闭窗口,可能会出现登入时间和登出时间相同情况
	int year = 0,month = 0,day = 0,hour = 0,minute = 0,second = 0; 
	sscanf(proxyTime, "%04d-%02d-%02d %02d:%02d:%02d", &year, &month, &day, &hour, &minute, &second);
	CTime proxyHostTime(year, month, day, hour, minute, second);
	CTimeSpan interval(0, 0, 0, 10);
	CTime newProxyTime = proxyHostTime + interval;
	char outDateTime[40] = {0};
	day = newProxyTime.GetDay();
	month = newProxyTime.GetMonth();
	year = newProxyTime.GetYear();
	hour = newProxyTime.GetHour();
	minute = newProxyTime.GetMinute() ;
	second = newProxyTime.GetSecond() ;
	sprintf(outDateTime, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
	//2016-01-21 ***********************************************

	int ret=0;
	db_mysql *sqlHandle=new db_mysql(&p_auditconn);
	if(sqlHandle->conndb()==0)
	{
		CLogMan::debug_log("[DoLoginLog][DBExecLoginLog]");
		//NOPASSWD功能非代理应用没有审计功能
		if (noPassword == 0)
		{
			p_loginlog.cmdFlag = 0;
		}
		ret = sqlHandle->DBExecLoginLog(&p_loginlog,act, proxyTime, outDateTime);
	}else{
		CLogMan::debug_log("[DoLoginLog][conndb failed]");
	}
	delete sqlHandle;
	LeaveCriticalSection(&g_csA);
    return ret;
}

#define MAX 2000
#define MIN 0

int RandNum()
{
	time_t   t; 
	srand((unsigned) time(&t)); 
	int   i=int(rand()%(MAX-MIN+1))+MIN;//产生了从MIN到MAX之间的随机数 
	return i;
}

int GetMonitorPort(unsigned short port)
{
       PMIB_TCPTABLE       tcpTable = NULL;
       BYTE                pBuffer[2012];
       DWORD               dwTableSize = 2012;
 
       DWORD dwRet = GetTcpTable((PMIB_TCPTABLE)pBuffer, &dwTableSize, TRUE);
       if(dwRet == NO_ERROR)
       {
           tcpTable = (PMIB_TCPTABLE)pBuffer;   
           for(DWORD i=0; i<tcpTable->dwNumEntries; i++)
           {    
               unsigned short localPort = htons(tcpTable->table[i].dwLocalPort);
			   if(localPort==port){
                   return 1;	//已被占用			 
			   }
          }
      }
      return 0;
}

int GetUniqueRandNum()
{
	int num = 0;
	int count = 0;
	int index=0;
	while (1)
	{
		//产生随机数
		num = RandNum()+p_shell.monitorPort;
		if(GetMonitorPort(num)==0){
			break;
		}

		index++;
		if(index > 500)
			break;
		Sleep(50);
	}
	return num;
}


 void __stdcall removeAliasDB2()
{
	return ;
   TCHAR  wzCmdline1[512]={0};
   TCHAR  tmp[50];
   TCHAR  data[512];	
   char *tmpOp=NULL;
   char *op=NULL;
   char *asFlag=" as ";
   char *nodeFlag=" node ";
   int  len=0,i=0;
   if(p_loginlog.accessType!=TYPE_DB2)
   {
	   return;
   }
   CLogMan::debug_log("[ExitRemoteControlServer][removeAliasDB2]");
   if(!MByteToWChar(p_aares.execApp,wzCmdline1,sizeof(wzCmdline1)/sizeof(wzCmdline1[0])))
   {
       return;
   }
//remove database alias
   tmpOp=strstr(p_aares.execParam,asFlag);
   if(tmpOp==NULL)
   {
	  return;
   }
   tmpOp += strlen(asFlag); 
   len=strlen(tmpOp);
   for(i=0;i<len;i++)
   {
	   if(*(tmpOp+i)!=0x20)
	   {
		   break;
	   }
   }
   if(i>0){
      tmpOp +=i;
   }
   op=strchr(tmpOp,0x20);
   if(op!=NULL)
   {
      *op=0;
	  op++;
   }
   if(!MByteToWChar(tmpOp,tmp,sizeof(tmp)/sizeof(tmp[0])))
   {
       return;
   }
   wsprintf(data,L"db2 uncatalog database %s",tmp);
   ShellExecute(NULL,_T("open"),wzCmdline1,data,NULL,SW_HIDE);
   
//remove node alias
   tmpOp=strstr(op,nodeFlag);
   if(tmpOp==NULL)
   {
	  return;
   }
   tmpOp += strlen(nodeFlag); 
   len=strlen(tmpOp);
   for(i=0;i<len;i++)
   {
	   if(*(tmpOp+i)!=0x20)
	   {
		   break;
	   }
   }
   if(i>0){
      tmpOp +=i;
   }
   op=strchr(tmpOp,0x20);
   if(op!=NULL)
   {
      *op=0;
	  op++;
   }
   if(!MByteToWChar(tmpOp,tmp,sizeof(tmp)/sizeof(tmp[0])))
   {
       return;
   }
   wsprintf(data,L"db2 uncatalog node %s",tmp);
   ShellExecute(NULL,_T("open"),wzCmdline1,data,NULL,SW_HIDE);
}

 
extern "C" __declspec(dllexport) int __stdcall ExitRemoteControlServer(int exitFlag)
{	
	removeAliasDB2();

	if(wcslen(p_shell.wzFile1) > 0) 
	{
		DeleteFile(p_shell.wzFile1);
	}
	if(wcslen(p_shell.wzFile2) > 0) 
	{
		DeleteFile(p_shell.wzFile2);
	}
	
	CLogMan::debug_log("[ExitRemoteControlServer][RemoveMapInfo 1]");
    RemoveMapInfo(1);
	

	CLogMan::debug_log("[ExitRemoteControlServer][DoLoginLog 1]");
	DoLoginLog(1);

    p_shell.pipeFlag=0;	
	p_shell.exitFlag=exitFlag;

	exitClear(NULL);
	return 0;
}

DWORD WINAPI MonitorThread(LPVOID lparam)
{
	p_shell.pipeFlag=1;
	char ReadBuf[512]={0};
	DWORD bytes_read=0,ret=0;
	CLogMan::debug_log("MonitorThread");
	while(p_shell.pipeFlag)
	{
		bytes_read=0;
		memset(ReadBuf,0,sizeof(ReadBuf));
	    PeekNamedPipe(p_shell.hRead,ReadBuf,512,&bytes_read,NULL,NULL);
        if(bytes_read!=0)
		{
			ret = ReadFile(p_shell.hRead,ReadBuf,bytes_read,&bytes_read,NULL);
			if(ret<=0)
			{
				break;
			}
			if(strlen(ReadBuf)>0)
			{
				char *status_op=strstr(ReadBuf,"STATUS:");
				char *port_op=strstr(ReadBuf,"PORT:");
				char *error_op=strstr(ReadBuf,"ERROR:");
				char *disconnet_op=strstr(ReadBuf,"DISCONNECT:");
				if(status_op!=NULL)
				{
					status_op+=7;	
					int status=atoi(status_op);
					if(status==1||status==2)
					{
						db_mysql *sqlHandle=new db_mysql(&p_auditconn);
						if(sqlHandle->conndb()==0)
						{
							sqlHandle->DBExecLoginLogStatus(p_loginlog.id,status);
						}
						delete sqlHandle;
					}
				}else
			    if(port_op!=NULL)
			    {
				   port_op+=5;
				   p_loginlog.port = atoi(port_op);
				   db_mysql *sqlHandle=new db_mysql(&p_auditconn);
				   if(sqlHandle->conndb()==0)
				   {
				   	   ret=sqlHandle->DBExecSetPort(p_loginlog.proxyAddr,p_loginlog.port,p_loginlog.authaccessresid);
				   }
				   delete sqlHandle;
				   if(ret!=0)
				   {
					   CLogMan::error_log("[MonitorThread]auidt db set port error! port[%d]",p_loginlog.port);
				   }
				}else
				if(error_op!=NULL)
				{
					error_op+=6;
					CLogMan::error_log("%s","[MonitorThread]monitor server exit!");
				}
				if (disconnet_op!=NULL)
				{
					CLogMan::debug_log("[MonitorThread]视频监控调用停止会话");

					////2016-01-14 新增 在删除AccessID前，通过数据库查询代理机时间
					////获取数据库中代理机的时间
					//char proxyTime[30] = {0};
					//if(openDBHandle(&p_oscconn)==0)
					//{
					//	p_shell.sqlHandle->DBQueryProxyHostTime(proxyTime, p_access->id);
					//	CLogMan::debug_log("代理机时间为:%s", proxyTime);
					//}
					//closeDBHandle();
					////2016-01-14 新增***********************************************
					//
					////2016-01-13 新增 解决SSH或TELNET访问时，通过web点击关闭访问窗口，会话审计里的登出时间不显示		
					//int ret=0;
					//db_mysql *sqlHandle=new db_mysql(&p_auditconn);
					//if(sqlHandle->conndb()==0)
					//{
					//	CLogMan::debug_log("[DoLoginLog][DBExecLoginLog]");
					//	//NOPASSWD功能非代理应用没有审计功能
					//	if (noPassword == 0)
					//	{
					//		p_loginlog.cmdFlag = 0;
					//	}
					//	ret = sqlHandle->DBExecLoginLog(&p_loginlog, 1, proxyTime);
					//}else{
					//	CLogMan::debug_log("[DoLoginLog][conndb failed]");
					//}
					//delete sqlHandle;
					////2016-01-13 新增*****************************************************
					ExitRemoteControlServer(0);
				}

			}
		}
		Sleep(500);
	}    
    CloseHandle(p_shell.hRead);
	CloseHandle(p_shell.hWrite);
	return  0;
}

extern "C" __declspec(dllexport) int __stdcall ShellRemoteControlServer()
{
	STARTUPINFO   StartInfo;   
	PROCESS_INFORMATION     ProceInfo;

	CLogMan::debug_log("[ShellRemoteControlServer]start");

	TCHAR szPath[MAX_PATH+200] = {0};
	GetModuleFileName(NULL,szPath,MAX_PATH);
	LPTSTR tszSlash = _tcsrchr(szPath, static_cast<int>('\\'));
	if (tszSlash)
		*++tszSlash = '\0';
	wcscat(szPath,_T("AuditMonitor.exe"));

	
	ZeroMemory(&StartInfo,sizeof(StartInfo));   
	ZeroMemory(&ProceInfo,sizeof(ProceInfo));
	StartInfo.cb=sizeof(StartInfo);
	
	p_loginlog.port = 0;//p_shell.monitorPort;//GetUniqueRandNum();
	if(DoLoginLog(0)!=0)   //osc 3.5 loginlog由代理机插入
	{
		CLogMan::debug_log("[ShellRemoteControlServer][DoLoginLog(0)!=0]end");
		return 1;
	}
	char id[40] = {0};
	memset(id,0,sizeof(id));
	if(openDBHandle(&p_auditconn)==0)
	{
		p_shell.sqlHandle->DBQueryLoginlogId(id,p_loginlog.authaccessresid);
		strcpy(p_loginlog.id,id);
	}
	if(p_shell.videoFlag>0){ //是否录像		
			int accessType=atoi(p_access->accessType);
			if(accessType == TYPE_SSH|| 
				accessType == TYPE_TELNET||
				accessType == TYPE_SFTP||
				accessType == TYPE_FTP||
				accessType == TYPE_SQLPLUS || 
				accessType == TYPE_PLSQL || 
				accessType == TYPE_DB2 ||
				accessType == TYPE_MSSQL||
				accessType == TYPE_SYBASE||
				accessType == TYPE_MYSQL||
				accessType == TYPE_PQSQL){
					
					strcpy(p_videolog.sessionId,id);
					strcpy(p_videolog.id,id);
					
			}
   			p_shell.sqlHandle->DBExecUserActivityVideoLog(&p_videolog);
			
	}
	closeDBHandle();


	SECURITY_ATTRIBUTES stSecurity;  
	stSecurity.nLength = sizeof(SECURITY_ATTRIBUTES);  
	stSecurity.lpSecurityDescriptor = NULL;  
	stSecurity.bInheritHandle = TRUE;

	BOOL bRet = CreatePipe(&p_shell.hRead, &p_shell.hWrite, &stSecurity, 0);// 创建匿名管道
	GetStartupInfo(&StartInfo);
//	StartInfo.hStdInput   = hRead;  
	StartInfo.hStdOutput  = p_shell.hWrite;  
	StartInfo.hStdError   =  p_shell.hWrite;  
	StartInfo.dwFlags     = STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
	StartInfo.wShowWindow = SW_HIDE;

	wcscat(szPath,_T(" "));
	TCHAR wzRemoteNum[200] = {0};
	wsprintf(wzRemoteNum,L"%d@%d",p_shell.monitorPort,p_shell.dwProcessId);
	wcscat(szPath,wzRemoteNum);
	CLogMan::debug_log("[ShellRemoteControlServer]启动视频监控：%ws",szPath);
    CreateProcess(NULL,   //lpApplicationName:   PChar   
		szPath,   //lpCommandLine:   PChar   
		NULL,     //lpProcessAttributes:   PSecurityAttributes   
		NULL,     //lpThreadAttributes:   PSecurityAttributes   
		TRUE,     //bInheritHandles:   BOOL   
		0,
	//	CREATE_NEW_CONSOLE,   
		NULL,   
		NULL,   
		&StartInfo,   
		&ProceInfo); 

	if(ProceInfo.hProcess == NULL)
	{
		CLogMan::debug_log("[ShellRemoteControlServer]视频监控启动失败");
		return 2;
	}

	p_shell.monitorProcess = ProceInfo.hProcess;
	if(bRet)
	{
	   DWORD  dwThreadId;
	   CreateThread(NULL,NULL,MonitorThread,0,0,&dwThreadId);
	}
	CLogMan::debug_log("[ShellRemoteControlServer]end");
	return 0;
}

extern   "C" __declspec(dllexport) int __stdcall CheckWebOnline(char * websessionid){
	return 0;
	OpenLog();
	WinSessionId winSessionId[2000];
	memset(winSessionId,0,sizeof(winSessionId));

	char  szCmdline[1024] = {0};
	TCHAR szPath[MAX_PATH] = {0};
	GetModuleFileName(NULL,szPath,MAX_PATH);
	LPTSTR tszSlash = _tcsrchr(szPath, static_cast<int>('\\'));
	if (tszSlash)
	{
		*++tszSlash = '\0';
	}
	wcscat(tszSlash,L"cfg.ini");
	memset(szCmdline,0,sizeof(szCmdline));
	WharToMByte(szPath,szCmdline);
	loadCfgFile(szCmdline);	

	memset(&p_oscconn,0,sizeof(STRUCT_DBCONNECT));
	//OSC DB
	strcpy(p_oscconn.dbHost,sysconfig.oscConn.dbHost);
	strcpy(p_oscconn.dbUser,sysconfig.oscConn.dbUser);
	strcpy(p_oscconn.dbName,sysconfig.oscConn.dbName);
	strcpy(p_oscconn.dbType,sysconfig.oscConn.dbType);
	p_oscconn.dbPort=atoi(sysconfig.oscConn.dbPort);
	if(strlen(sysconfig.oscConn.dbPwd)<20)
	{
		strcpy(p_oscconn.dbPwd,sysconfig.oscConn.dbPwd);
	}else{
		DataSafe._encrypt(sysconfig.oscConn.dbPwd,strlen(sysconfig.oscConn.dbPwd),p_oscconn.dbPwd);
	}

	CLogMan::debug_log("CheckWebOnline:websessionid=%s  ---------------",websessionid);
	//InitializeCriticalSection(&mysqlcsA);
	//
	int retval=openDBHandle(&p_oscconn);
	EnterCriticalSection(&mysqlcsA);
	int count = 0;
	if(retval==0)
	{
		count = p_shell.sqlHandle->DBQueryWebOnline(websessionid,winSessionId);
		if (strcmp(sysconfig.level,"debug") != 0)
		{
			p_shell.sqlHandle->DBDeleteAuthAccessRes(websessionid);
		}
		
	}
	closeDBHandle();
	//LeaveCriticalSection(&mysqlcsA);
	//CLogMan::debug_log("LeaveCriticalSection mysqlcsA");
	for(int i = 0 ; i < count ; i ++){
		int sessionid = atoi(winSessionId[i].sessionid);
		if(sessionid > 0){
			CLogMan::debug_log("WTSLogoffSession:%d",sessionid);
			WTSLogoffSession(WTS_CURRENT_SERVER_HANDLE,sessionid,TRUE);
		}
	}
	//DeleteCriticalSection(&mysqlcsA);
	return 0;
}

int FindVideoFormat(char *path)
{
	SECURITY_ATTRIBUTES sa; 
	HANDLE hRead,hWrite; 
	char szPath[1024] = {0};
	char szParam[1024] = {0};
	memset(szPath,0,sizeof(szPath));
	memset(szParam,0,sizeof(szParam));

	GetModuleFileNameA(NULL,szPath,MAX_PATH);
	char* slash = strrchr(szPath, static_cast<int>('\\'));
	if (slash)
	{
		*(slash) = '\0';
	}
	strcat(szPath,"\\ffmpeg.exe");
	sprintf(szParam," -i %s",path);

	sa.nLength = sizeof(SECURITY_ATTRIBUTES); 
	sa.lpSecurityDescriptor = NULL; 
	sa.bInheritHandle = TRUE; 
	if (!CreatePipe(&hRead,&hWrite,&sa,0)) { 
		printf("Error On CreatePipe()\n"); 
		return false; 
	} 
	PROCESS_INFORMATION pi; 
	STARTUPINFOA si; 
	si.cb = sizeof(STARTUPINFO); 
	GetStartupInfoA(&si); 
	si.hStdError = hWrite; 
	si.hStdOutput = hWrite; 
	si.wShowWindow = SW_HIDE; 
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES; 
	if (!CreateProcessA(szPath, szParam, 
		NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi)) { 
			printf("Error on CreateProcess()\n"); 
			return false;   
	} 
	CloseHandle(hWrite); 
	const int BufferLength = 1024; 
	CStringA showedMsg = ""; 
	char buffer[BufferLength + 1] = {0}; 
	memset(buffer,0,sizeof(buffer));
	DWORD bytesRead; 
	while (ReadFile(hRead,buffer,BufferLength,&bytesRead,NULL)) { 
		showedMsg.Append(buffer); 
		memset(buffer,0,sizeof(buffer));
		Sleep(200); 
	} 
	CloseHandle(hRead);
	if (showedMsg.Find("Video: h264") > 0) { 

		return 1; 
	}else if (showedMsg.Find("Video: mpeg4") > 0) { 

		return 2; 
	}

	return 0;
}
int GetFileOpeDate(char * path)
{

	HANDLE     hFile;
	FILETIME   filetime;
	FILETIME   localtime;
	//SYSTEMTIME systemtime;

	hFile = CreateFileA(path, GENERIC_READ | GENERIC_WRITE,0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		GetFileTime(hFile, NULL, NULL, &filetime);      //取得UTC文件时间
		FileTimeToLocalFileTime(&filetime, &localtime); //换成本地时间
		//FileTimeToSystemTime(&localtime, &systemtime);  //换成系统时间格式


		/*TRACE("%04d-%02d-%02d %02d:%02d:%02d/r/n",
			systemtime.wYear, systemtime.wMonth, systemtime.wDay,
			systemtime.wHour, systemtime.wMinute, systemtime.wSecond);*/

		CloseHandle(hFile);

		CTime ftime(localtime);

		//取得当前时间
		time_t t;
		time(&t);
		CTime ctime(t);	
		CTimeSpan span = ctime - ftime;


		//int day = ctime.GetDay();
		//int month = ctime.GetMonth();
		//int year = ctime.GetYear();

		

		if (span.GetDays() <= 1/*day == systemtime.wDay && month == systemtime.wMonth && year == systemtime.wYear*/)
		{
			return 1;
		}



	}
	return 0;
}
void CompareFile(char * path)
{
	if(FindVideoFormat(path) != 2)
	{
		Sleep(1000);
		return;
	}
	char cmpCmd[1024] = {0};
	char cmpPath[1024] = {0};
	char bakPath[1024] = {0};
	sprintf(cmpPath,"%s.cmp.avi",path);
	sprintf(bakPath,"%s.bak",path);
	sprintf(cmpCmd," -i %s -vcodec libx264 %s",path,cmpPath);

	CLogMan::debug_log("CompareFile file=%s",path);

	SECURITY_ATTRIBUTES sa; 
	HANDLE hRead,hWrite; 
	char szPath[1024] = {0};
	memset(szPath,0,sizeof(szPath));

	GetModuleFileNameA(NULL,szPath,MAX_PATH);
	char* slash = strrchr(szPath, static_cast<int>('\\'));
	if (slash)
	{
		*(slash) = '\0';
	}
	strcat(szPath,"\\ffmpeg.exe");

	sa.nLength = sizeof(SECURITY_ATTRIBUTES); 
	sa.lpSecurityDescriptor = NULL; 
	sa.bInheritHandle = TRUE; 
	if (!CreatePipe(&hRead,&hWrite,&sa,0)) { 
		CLogMan::debug_log("Error On CreatePipe()\n"); 
		return ; 
	} 
	PROCESS_INFORMATION pi; 
	STARTUPINFOA si; 
	si.cb = sizeof(STARTUPINFO); 
	GetStartupInfoA(&si); 
	si.hStdError = hWrite; 
	si.hStdOutput = hWrite; 
	si.wShowWindow = SW_HIDE; 
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES; 
	if (!CreateProcessA(szPath, cmpCmd, 
		NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi)) { 
			CLogMan::debug_log("Error on CreateProcess()\n"); 
			return ;   
	} 
	CloseHandle(hWrite); 
	const int BufferLength = 1024; 
	CStringA showedMsg = ""; 
	char buffer[BufferLength + 1] = {0}; 
	memset(buffer,0,sizeof(buffer));
	DWORD bytesRead; 
	while (ReadFile(hRead,buffer,BufferLength,&bytesRead,NULL)) { 
		showedMsg.Append(buffer); 
		memset(buffer,0,sizeof(buffer));
		Sleep(200); 
	} 

	CloseHandle(hRead);

	if (showedMsg.Find("libx264 @") < 0) { 
		CLogMan::debug_log("CompareFile not find libx264 @");
		return ; 
	}
	if(MoveFileA(path,bakPath) == FALSE){
		CLogMan::debug_log("can not MoveFile");
		return;
	}
	if (MoveFileA(cmpPath,path) == FALSE)
	{
		MoveFileA(bakPath,path);
	}
	DeleteFileA(bakPath);

	CLogMan::debug_log("CompareFile end");

	Sleep(1000);
}

void CompareDir(char * dir,bool isnow)
{

	char szFind[MAX_PATH];

	WIN32_FIND_DATAA FindFileData;

	strcpy(szFind,dir);
	strcat(szFind,"*.*");

	CLogMan::debug_log("CompareDir dir=%s",dir);

	HANDLE hFind=::FindFirstFileA(szFind,&FindFileData);
	if(INVALID_HANDLE_VALUE == hFind)    return;
	while(TRUE)
	{
		if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
		}
		else
		{      
			char filepath[300] = {0};
			memset(filepath,0,sizeof(filepath));
			sprintf(filepath,"%s%s",dir,FindFileData.cFileName);
			if (isnow)
			{
				if(GetFileOpeDate(filepath) == 1)
				{
					CompareFile(filepath);
				}
			}else{
				CompareFile(filepath);
			}
			
		}
		if(!FindNextFileA(hFind,&FindFileData))
			break;
	}
	FindClose(hFind); 
}
DWORD WINAPI CompareVideoThread(LPVOID lparam)
{
	CLogMan::debug_log("start CompareVideoThread");

	CompareDir(sysconfig.videoPath,false);
	int count = 0;
	while (true)
	{
		Sleep(1000*60);
		count ++;
		if (count >= 60 * 6)
		{
			count = 0;
			CompareDir(sysconfig.videoPath,true);
		}

	}
}
void RestartService()
{
	// 打开服务管理对象
	SC_HANDLE hSC = ::OpenSCManager( NULL, 
		NULL, GENERIC_EXECUTE);
	if( hSC == NULL)
	{
		//TRACE( "open SCManager error");
		return;
	}
	// 打开www服务。
	SC_HANDLE hSvc = ::OpenService( hSC, L"NFS",
		SERVICE_START | SERVICE_QUERY_STATUS | SERVICE_STOP);
	if( hSvc == NULL)
	{
		//TRACE( "Open NFS erron。");
		::CloseServiceHandle( hSC);
		return;
	}
	// 获得服务的状态
	SERVICE_STATUS status;
	if( ::QueryServiceStatus( hSvc, &status) == FALSE)
	{
		//TRACE( "Get Service state error。");
		::CloseServiceHandle( hSvc);
		::CloseServiceHandle( hSC);
		return;
	}
	//如果处于停止状态则启动服务，否则停止服务。
	if( status.dwCurrentState == SERVICE_RUNNING)
	{
		// 停止服务
		if( ::ControlService( hSvc, 
			SERVICE_CONTROL_STOP, &status) == FALSE)
		{
			//TRACE( "stop service error。");
			::CloseServiceHandle( hSvc);
			::CloseServiceHandle( hSC);
			return;
		}
		// 等待服务停止
		while( ::QueryServiceStatus( hSvc, &status) == TRUE)
		{
			::Sleep( status.dwWaitHint);
			if( status.dwCurrentState == SERVICE_STOPPED)
			{
				//TRACE( "stop success。");
				::CloseServiceHandle( hSvc);
				::CloseServiceHandle( hSC);
				//return;
			}
		}
		// 启动服务
		if( ::StartService( hSvc, NULL, NULL) == FALSE)
		{
			//TRACE( "start service error。");
			::CloseServiceHandle( hSvc);
			::CloseServiceHandle( hSC);
			return;
		}
		// 等待服务启动
		while( ::QueryServiceStatus( hSvc, &status) == TRUE)
		{
			::Sleep( status.dwWaitHint);
			if( status.dwCurrentState == SERVICE_RUNNING)
			{
				//TRACE( "start success。");
				::CloseServiceHandle( hSvc);
				::CloseServiceHandle( hSC);
				return;
			}
		}

	}
	else if( status.dwCurrentState == SERVICE_STOPPED)
	{
		// 启动服务
		if( ::StartService( hSvc, NULL, NULL) == FALSE)
		{
			//TRACE( "start service error。");
			::CloseServiceHandle( hSvc);
			::CloseServiceHandle( hSC);
			return;
		}
		// 等待服务启动
		while( ::QueryServiceStatus( hSvc, &status) == TRUE)
		{
			::Sleep( status.dwWaitHint);
			if( status.dwCurrentState == SERVICE_RUNNING)
			{
				//TRACE( "start success。");
				::CloseServiceHandle( hSvc);
				::CloseServiceHandle( hSC);
				return;
			}
		}
	}
	//TRACE( "start error。");
	::CloseServiceHandle( hSvc);
	::CloseServiceHandle( hSC);
	return;
}

BOOL DirectoryExist(CString Path)
{
	WIN32_FIND_DATA fd;
	BOOL ret = FALSE;
	if (Path.Right(1) == "\\")
		Path = Path.Left(Path.GetLength()-1);

	HANDLE hFind = FindFirstFile(Path, &fd);
	if ((hFind != INVALID_HANDLE_VALUE) && (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{  //目录存在
		ret = TRUE;    
	}
	FindClose(hFind);
	return ret;
}
DWORD WINAPI CheckNFSThread(LPVOID lparam)
{
	while(true){
		TCHAR szFilePath[_MAX_PATH];
		TCHAR szmountPath[_MAX_PATH];
		::GetModuleFileName(NULL, szFilePath, _MAX_PATH);
		LPTSTR tszSlash = _tcsrchr(szFilePath, static_cast<int>('\\'));
		if (tszSlash)
		{
			*++tszSlash = '\0';
		}
		wcscat(szFilePath,L"cfg.ini");
		GetPrivateProfileString(L"node",L"videoPath",L"",szmountPath,sizeof(szmountPath),szFilePath);
		if(DirectoryExist(szmountPath) == FALSE){
			CLogMan::debug_log("start Restart NFS Service");
			RestartService();
		}


		Sleep(20*1000);
	}
	return 0;
}
extern	 "C" __declspec(dllexport) int __stdcall MonitorSession()
{
	char  szCmdline[1024] = {0};
	TCHAR szPath[MAX_PATH] = {0};
	GetModuleFileName(NULL,szPath,MAX_PATH);
	LPTSTR tszSlash = _tcsrchr(szPath, static_cast<int>('\\'));
	if (tszSlash)
	{
		*++tszSlash = '\0';
	}
	wcscat(tszSlash,L"cfg.ini");
	memset(szCmdline,0,sizeof(szCmdline));
	WharToMByte(szPath,szCmdline);
	OpenLog();
	loadCfgFile(szCmdline);	
	memset(&p_auditconn,0,sizeof(STRUCT_DBCONNECT));
	//audit DB
	strcpy(p_auditconn.dbHost,sysconfig.auditConn.dbHost);
	strcpy(p_auditconn.dbUser,sysconfig.auditConn.dbUser);
	strcpy(p_auditconn.dbName,sysconfig.auditConn.dbName);
	strcpy(p_auditconn.dbType,sysconfig.auditConn.dbType);
	p_auditconn.dbPort=atoi(sysconfig.auditConn.dbPort);
	if(strlen(sysconfig.auditConn.dbPwd)<20)
	{
		strcpy(p_auditconn.dbPwd,sysconfig.auditConn.dbPwd);
	}else{
		DataSafe._encrypt(sysconfig.auditConn.dbPwd,strlen(sysconfig.auditConn.dbPwd),p_auditconn.dbPwd);
	}
	
	
	CLogMan::debug_log("db[%s]",sysconfig.auditConn.dbName);
	CreateThread(NULL,NULL,CompareVideoThread,0,0,0);
	CreateThread(NULL,NULL,CheckNFSThread,0,0,0);

	int count=0;
	CLogMan::debug_log("[MonitorSession]start Monitor Thread===================================");

	while(true){    //监控进程的存在与退出         
		PWTS_PROCESS_INFOA pProcessInfo=NULL;
		DWORD   ProcessCount;
		if(WTSEnumerateProcessesA(WTS_CURRENT_SERVER_HANDLE,0,1,&pProcessInfo,&ProcessCount))
		{
			DWORD LastSessionId = 0;
			for(DWORD i   =   0;i <ProcessCount;i++) {
				if (pProcessInfo[i].SessionId == 0 || pProcessInfo[i].SessionId == LastSessionId)
				{
					continue;
				}
				LastSessionId=pProcessInfo[i].SessionId;
				if(checkSessionStatus(pProcessInfo[i].SessionId))
				{
					CLogMan::debug_log("[MonitorSession]注销断开的会话WTSLogoffSession[%d][%s]=========",pProcessInfo[i].SessionId,pProcessInfo[i].pProcessName);
					WTSLogoffSession(WTS_CURRENT_SERVER_HANDLE,pProcessInfo[i].SessionId,FALSE); //注销
				}
			}
		}
		WTSFreeMemory(pProcessInfo);
		count++;
		if(count>=3)  
		{
			int retval=openDBHandle(&p_auditconn);
			if (retval == 0)
			{
				//获取数据库中代理机的时间
				char proxyTime[30] = {0};
				if(openDBHandle(&p_oscconn)==0)
				{
					p_shell.sqlHandle->DBQueryProxyHostTime(proxyTime, p_access->id);
				}
				closeDBHandle();

				p_shell.sqlHandle->DBCheckLoginLog(atoi(sysconfig.code), proxyTime);
			}
			closeDBHandle();
			count=0;
		}
		Sleep(1000*10);
	}
	CLogMan::debug_log("exit Monitor Thread");
	return  0;
}
//0,a+
//1,w+
int getFileOpenMode(char* file)
{
	HANDLE hFile;
	hFile = CreateFileA(file, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, 0, NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		return 0;
	}
	FILETIME ftCreate, ftAccess, ftWrite;
	SYSTEMTIME stUTC, stLocal;
	DWORD dwRet;

	// Retrieve the file times for the file.
	if (!GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite))
	{
		CloseHandle(hFile);
		return 0;
	}
	CloseHandle(hFile);
	// Convert the last-write time to local time.
	FileTimeToSystemTime(&ftWrite, &stUTC);
	time_t t;
	time(&t);
	CTime ctime(t);	
	int nowDay = ctime.GetDay();
	if(stUTC.wDay != nowDay)
		return 1;
	else return 0;
}