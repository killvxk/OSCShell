#ifndef OSCSHELL_H
#define OSCSHELL_H
#include <Windows.h>

#pragma	  warning(disable:4996)

#ifndef IN
#define IN
#define OUT
#endif 

extern   "C" __declspec(dllimport) HANDLE __stdcall OSCShellEx(
						IN  char* szAccessID,  //∂ØÃ¨∑√Œ MAP ID
						IN  char* szClientIP,
						IN  int  diskFlag,    //0≤ª”≥…‰¥≈≈Ã,1”≥…‰¥≈≈Ã
						IN  int  showWindow,
						OUT char* szloginfile,
						OUT char* szAvifile,
						OUT int * newtype,
						OUT DWORD *processId,
						OUT int *accType,
						OUT int *hasChild
						);

extern   "C" __declspec(dllimport) int __stdcall GetVistitimeEx(int *timeout,int *delayFlag);

extern   "C" __declspec(dllimport) int __stdcall DoLoginLog(int act);

extern   "C" __declspec(dllimport) int __stdcall ShellRemoteControlServer();

extern   "C" __declspec(dllimport) int __stdcall ExitRemoteControlServer(int exitFlag);

extern   "C" __declspec(dllimport) void __stdcall OpenLog(char *g_szAccessId=NULL);

extern   "C" __declspec(dllimport) void __stdcall WriteLog(char *msg);

extern   "C" __declspec(dllimport) void __stdcall CloseLog();

extern   "C" __declspec(dllimport) int __stdcall CheckWebOnline(char * websessionid);

extern	 "C" __declspec(dllimport) int __stdcall GetLocalLang();

extern	 "C" __declspec(dllimport) int __stdcall MonitorSession();

extern "C" __declspec(dllimport) void __stdcall oscLoadCfgFile(char *cfgFile, int *warningTime1, int *warningTime2, int *warningTime3);

#endif