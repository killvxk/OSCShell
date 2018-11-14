// OSCShellApp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include "../OSCShell/OSCShell.h"
#include "screen2swf.h"
#include <atltime.h>
#include <TlHelp32.h>
#include "sessionrdp.h"
//#include <iostream>
//#include <fstream>
//using namespace std;
//#include <Winuser.h>
#include "db_table.h"
#include "sessionMgr.h"
#include "resource.h"
#define  WARNTIME  300
#define  WARNTIME1 180
#define  WARNTIME2 60
#define  ID_SWITCH_CONSOLEMODE 0xE00F 
#define  OSCAUDITWINDOW "OscAuditServerApp"

void GetSysInfo();
//部分进程启动子进程之后，出进程退出
void getChildProcessID(IN DWORD parentProcessID,OUT DWORD * childProcessIDs,IN int MAXIDS,OUT int& childsCount );
typedef struct
{
    char title[10];
	char msg[1024];
	UINT type;
}STRUCT_MSG;

static HHOOK  g_hook = NULL;
static HWND OscHandle=NULL;
static NOTICE_STRUCT noticeMsg;

static HANDLE hProcess=NULL;
static HANDLE appThreadHandle=NULL;
static HANDLE sessionThreadHandle=NULL;

static int isExitSession=0;

char msg1[]					= "资源访问时间限制";
char msg1_english[]			= "resource access time limit";
char msg2[]					= "申请访问时间限制";
char msg2_english[]			= "apply access time limit";
char msg3[]					= "策略时间限制";
char msg3_english[]			= "policy time limit";
char msg4[]					= "密码申请时间限制";
char msg4_english[]			= "apply password time limit";
char msg5[]					= "账号已被密码申请、时间到期";
char msg5_english[]			= "account has been password apply、the end of time";
char msg6[]					= "远程访问将在%d小时后结束！";
char msg6_english[]			= "remote access at the end of %d hours";
char msg7[]					= "远程访问将在%d分钟后结束！";
char msg7_english[]			= "remote access at the end of %d minute";
char msg8[]					= "远程访问将在%d秒钟后结束！";
char msg8_english[]			= "remote access at the end of %d second";
char msg9[]					= "如有疑问请联系管理员。";
char msg9_english[]			= "if you have any questions please contact administrator。";
char msg10[]				= "远程访问将在半小时后结束！";
char msg10_english[]		= "remote access at the end of halfhours";
char msg11[]				= "请备份好数据";
char msg11_english[]		= "please backup your data";
char msg12[]				= "请备份好数据或做延时申请";
char msg12_english[]		= "please backup your data or apply for delay ";
char msg13[]				= "账号密码已被申请、访问结束前最大延时申请为%ld分钟！";
char msg13_english[]		= "account has been password apply、visit before the end of the maximum delay for %ld minutes";

void CheckTimePolicy();

DWORD static WINAPI MsgShowThread(LPVOID lparam)
{
	STRUCT_MSG *msgHandle=(STRUCT_MSG *)lparam;
	HWND hWnd = GetForegroundWindow();
	MessageBox(hWnd,msgHandle->msg,msgHandle->title,msgHandle->type); 
	return 0;
}
DWORD static WINAPI MsgSFTPShowThread(LPVOID lparam)
{
	HWND hWnd = GetForegroundWindow();
	MessageBox(hWnd,"SFTP连接中,请耐心等待...","提示",MB_ICONWARNING|MB_RETRYCANCEL|MB_TOPMOST); 
	return 0;
}
void static MsgSFTPShow()
{
    DWORD  dwThreadId;
	CreateThread(NULL,NULL,MsgSFTPShowThread,NULL,0,&dwThreadId);
}
void static MsgShow(char *msg,char *title,UINT type)
{
	STRUCT_MSG msgHandle;
	memset(&msgHandle,0,sizeof(STRUCT_MSG));
	strcpy(msgHandle.msg,msg);
	strcpy(msgHandle.title,title);
	msgHandle.type=type;
    DWORD  dwThreadId;
	CreateThread(NULL,NULL,MsgShowThread,&msgHandle,0,&dwThreadId);
}

void ShowWarning(char *msg)
{
   MsgShow(msg,"warning",MB_ICONWARNING|MB_OK|MB_TOPMOST);
}

DWORD WINAPI AnswerThread(LPVOID lparam)
{
	HANDLE handle=(HANDLE)lparam;
	int nhandles=1;
	DWORD  dw=WaitForSingleObject(handle,INFINITE);
	WriteLog("[AnswerThread]应用关闭!");
	if(paraSession.isLoop() && isExitSession==0)
	{
		if(sessionThreadHandle!=NULL)
		{
			CloseHandle(sessionThreadHandle);
			sessionThreadHandle=NULL;
		}
		//WriteLog("[AnswerThread]注销会话!");
		//paraSession.ExitOscShell_Session(1);
		//线程中执行，无法保证主线程退出之前执行完成
		paraSession.ExitOscShell(1);
	}
	WriteLog("[AnswerThread]等待注销会话!");
	return  0;
}
struct CHILDPROCESS
{
	HANDLE childHandles[16];//子进程ID
	int childsCount;//子进程数
};
int processIdToHandle(DWORD* processId,HANDLE* handles,int IdCount)
{
	int num = 0;
	if (processId == NULL || handles == NULL)
	{
		return 0;
	}
	for (int i=0;i<IdCount;i++)
	{
		handles[i] = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId[i]);
		if (handles[i] == NULL)
		{
			char errMsg[128];
			memset(errMsg,0,sizeof(errMsg));
			sprintf(errMsg,"OpenProcess[%d]failed!(%s)",processId[i],GetLastError());
			WriteLog(errMsg);
		}
		else
		{
			num++;
		}
	}
	return num;
}
//监控所有子进程退出
DWORD WINAPI AnswerThreadEx(LPVOID lparam)
{
    CHILDPROCESS *childs = (CHILDPROCESS*)lparam;
	DWORD dw = WaitForMultipleObjects(childs->childsCount,childs->childHandles,TRUE,INFINITE);
	for (int i=0;i<childs->childsCount;i++)
	{
		CloseHandle(childs->childHandles[i]);
	}
	//DWORD  dw=WaitForSingleObject(handle,INFINITE);
	WriteLog("[AnswerThread]应用关闭!");
	if(paraSession.isLoop() && isExitSession==0)
	{

		if(sessionThreadHandle!=NULL)
		{
			CloseHandle(sessionThreadHandle);
			sessionThreadHandle=NULL;
		}
		WriteLog("[AnswerThread]注销会话!");
		paraSession.ExitOscShell_Session(1);
	}
	WriteLog("[AnswerThread]等待注销会话!");
	return  0;
}
void RestartService()
{
	ShellExecute(NULL, "open", "cmd.exe", "/c net stop NFS & net start NFS", "", SW_HIDE);
	return;
}
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		WriteLog("[WM_DESTROY]");
		PostQuitMessage(0);
		break;
	case WM_ENDSESSION:
	case WM_QUERYENDSESSION:
		{
			WriteLog("[WM_ENDSESSION、WM_QUERYENDSESSION]会话被强制结束！");
			//MessageBox(NULL,"会话被强制结束","警告",MB_ICONWARNING|MB_OK|MB_TOPMOST);
			//if(hProcess != NULL)
			//	TerminateProcess(hProcess,0);
			paraSession.ExitOscShell(0);

			// 等待若干时间后执行退出操作，使录像线程有充足的时间正常退出，否则会导致录像文件无法打开
			Sleep(4000);
			PostQuitMessage(0);
			WriteLog("[WM_ENDSESSION、WM_QUERYENDSESSION]系统强制退出！");
			break;
		}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

DWORD static WINAPI ShowDisableWindowThread(LPVOID lparam)
{
	//////////////////////////////////////////////////[]
	WriteLog("[main]创建隐藏窗口防止强制结束进程导致录像、审计不完整==========================");
	HINSTANCE hInstance;  
	hInstance=GetModuleHandle(NULL);  
	WNDCLASS Draw;  
	Draw.cbClsExtra = 0;  
	Draw.cbWndExtra = 0;  
	Draw.hCursor = LoadCursor(hInstance, IDC_ARROW);;  
	Draw.hIcon = LoadIcon(hInstance, IDI_APPLICATION);;  
	Draw.lpszMenuName = NULL;  
	Draw.style = CS_HREDRAW | CS_VREDRAW;  
	Draw.hbrBackground = (HBRUSH)COLOR_WINDOW;  
	Draw.lpfnWndProc = WindowProc;  
	Draw.lpszClassName = _T("DDraw");  
	Draw.hInstance = hInstance;  
	RegisterClass(&Draw);  
	HWND hwnd = CreateWindow(    
		_T("DDraw"),           //上面注册的类名，要完全一致    
		_T("oscshell"),  //窗口标题文字    
		WS_OVERLAPPEDWINDOW, //窗口外观样式    
		38,             //窗口相对于父级的X坐标    
		20,             //窗口相对于父级的Y坐标    
		480,                //窗口的宽度    
		250,                //窗口的高度    
		NULL,               //没有父窗口，为NULL    
		NULL,               //没有菜单，为NULL    
		hInstance,          //当前应用程序的实例句柄    
		NULL);              //没有附加数据，为NULL    
	ShowWindow(hwnd, SW_HIDE);      
	UpdateWindow(hwnd); 

	MSG msg;    
	while(GetMessage(&msg, NULL, 0, 0))    
	{    
		TranslateMessage(&msg);    
		DispatchMessage(&msg);    
	}  

	return 0;
}
void static ShowDisableWindow()
{
	DWORD  dwThreadId;
	CreateThread(NULL,NULL,ShowDisableWindowThread,NULL,0,&dwThreadId);
}
HANDLE singleInstanceSem = NULL;
void singleSemporeWatcher()
{
	WriteLog("开始监控RDP会话！");
	WaitForSingleObject(singleInstanceSem,INFINITE);
	WriteLog("[singleSemporeWatcher]发现RDP会话串联！退出当前会话！");
	if(singleInstanceSem != NULL)
		CloseHandle(singleInstanceSem);
	paraSession.ExitOscShell_Session(0);
}
int main(int argc, char* argv[])
{
	int retval=0;	
	char szParam[1024] = {0};
	RestartService();
	memset(&noticeMsg,0,sizeof(NOTICE_STRUCT));
	noticeMsg.sid=getCurrSessionID();
	if(argc < 2){
		MessageBox(NULL,"OSCShellApp参数个数 < 1","警告",MB_ICONWARNING|MB_OK|MB_TOPMOST);
		return paraSession.exitSession();

	}else if(argc > 2){
		strcpy(szParam,argv[1]);
		strcat(szParam,argv[2]);
	}else{
		strcpy(szParam,argv[1]);
	}

	char szAccessId[50] = {0};
	char szClientIP[20]={0};
	char szTmpFlag[20]={0};
	char warningMsg[512]={0};

    int  diskFlag=0;
    int  wShowWindow = SW_MAXIMIZE;

	//解析参数@分隔 工具ID@账户ID@访问类型@OSC平台主账号ID@客户端IP@从账户ID(用于远程登录后 su 非必要参数)
	int count = 0;
	char* pszParam = strtok(szParam,"@");
	while(pszParam)
	{
		switch(count)
		{
		case 0:
			strcpy(szAccessId,pszParam);
			break;
		case 1:
			strcpy(szClientIP,pszParam);
			break;
		case 2:
			memset(szTmpFlag,0,sizeof(szTmpFlag));
			strcpy(szTmpFlag,pszParam);
			if(strlen(szTmpFlag)>0){
			   diskFlag=atoi(szTmpFlag);
			}
			break;
		case 3:
			memset(szTmpFlag,0,sizeof(szTmpFlag));
			strcpy(szTmpFlag,pszParam);
			if(strlen(szTmpFlag)>0){
			   if(atoi(szTmpFlag)==0)
			   {
                  wShowWindow=SW_NORMAL;  
			   }
			}
			break;
		}
		pszParam = strtok(NULL, "@");
		count++;
	}
	//初始化日志
	paraSession.initSession(szAccessId);
	char osclogPrefix[MAX_PATH]={0};
	sprintf(osclogPrefix,"%s-%s",szAccessId,szClientIP);
	paraSession.openLogs(osclogPrefix);
	char sessionIds[64]={0};
	sprintf(sessionIds,"[main]sessionId:[%d]",noticeMsg.sid);
	WriteLog(sessionIds);
	WriteLog(szAccessId);
	//进程互斥检查
	//HANDLE singleInstanceMutex = NULL;
	////singleInstanceMutex  = CreateMutexA(NULL,FALSE,"oscShellAppSingleMutex");
	//singleInstanceMutex  = CreateMutexA(NULL,TRUE,"oscShellAppSingleMutex");
	//if (GetLastError() == ERROR_ALREADY_EXISTS)
	//{
	//	CloseHandle(singleInstanceMutex);
	//	singleInstanceMutex = NULL;
	//	char *msg="另一个用户已经连接到此远程计算机，请尝试再次连接或联系管理员！";
	//	WriteLog(msg);
	//	MessageBox(NULL,msg,"远程桌面连接",MB_OK|MB_HELP);
	//	return paraSession.exitSession();
	//	//return paraSession.ExitOscShell_Session(0);
	//}
	singleInstanceSem = CreateSemaphore(NULL,0,1,"oscShellAppSingleMutex");
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		char *msg="另一个用户已经连接到此远程计算机，请尝试再次连接或联系管理员！";
		//MessageBox(NULL,msg,"远程桌面连接",MB_OK);
		ReleaseSemaphore(singleInstanceSem,1,NULL);
		CloseHandle(singleInstanceSem);
		singleInstanceSem = NULL;
		WriteLog(msg);
		//return paraSession.exitSession();
	}
	if (singleInstanceSem == NULL)
	{
		char errMsg[32]={0};
		sprintf(errMsg,"CreateSemaphore(%d)",GetLastError());
		WriteLog(errMsg);
	}
	if (count < 2)
	{
		MessageBox(NULL,"param count < 2","warning",MB_ICONWARNING|MB_OK|MB_TOPMOST);
		return paraSession.exitSession();
	}  
	int newtype = 0;
	DWORD processId = 0;
	//if(strstr(szClientIP,"172.28.112.144") != NULL || strstr(szClientIP,"172.28.112.135") != NULL)
	//MsgSFTPShow();
	WriteLog("[main]开始启动访问应用==========================");
	//写应用正常登入日志
	FILE *file = NULL;
	char prePath[MAX_PATH] = {0};
	char logName[MAX_PATH] = {0};
	if (strlen(prePath) <= 0)
	{
		memset(prePath,0,sizeof(prePath));
		::GetModuleFileName(NULL, prePath, _MAX_PATH);
		if(strlen(prePath) <= 0)
			return 0;
		char *slash = strrchr(prePath,static_cast<int>('\\'));
		if(slash != NULL)
			*slash = 0;
	}
	//获取当前时间
	time_t now_sec = time(NULL);
	char timestr[24];
	strftime(timestr, _countof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&now_sec));
	
	//2016-01-28 增加 判断log目录是否存在，如不存在，新建文件夹
	char baseDirectory[MAX_PATH] = {0};
	sprintf(baseDirectory, "%s\\log", prePath);
	if (!PathFileExists(baseDirectory))
	{
		CreateDirectory(baseDirectory, NULL);
	}
	//oscShellAppLogin文件名中增加年月信息
	tm *myTime = localtime(&now_sec);
	int year = myTime->tm_year + 1900;
	int month = myTime->tm_mon + 1;
	sprintf(logName,"%s\\log\\oscShellAppLogin%4d%02d.log",prePath, year, month);
	//2016-01-28***************************************************
	file = fopen(logName, "a+");
	fprintf(file, "%s:OSCShellApp 命令行参数[%s] 应用正常启动!\n",timestr, argv[1]);
	fclose(file);
	int haschild = 0;//
	int accessType = 0;
	hProcess = OSCShellEx(szAccessId,szClientIP,diskFlag,wShowWindow,paraSession.szloginfile,paraSession.szAvifile,&newtype,&processId, &accessType, &haschild);
	if(!hProcess)
	{
		MessageBox(NULL,"application start error,session exit!","error",MB_ICONWARNING|MB_OK|MB_TOPMOST);
		return paraSession.ExitOscShell_Session(1);		
	}
	//////////////////////////////////////////////////[]
	 ShowDisableWindow();
	//////////////////////////////////////////////////
	DWORD  dwThreadId;
	WriteLog("[main]监控RDPsession串联==========================");
	CreateThread(NULL,NULL,(LPTHREAD_START_ROUTINE)singleSemporeWatcher,NULL,0,&dwThreadId);// wait calling process to end
	WriteLog("[main]检查访问应用状态==========================");
	//用户自己填写登陆用户名、密码时，windows程序，检查是否有子进程
	//常规应用linux访问、数据库访问，不需要检查子进程
	if (haschild != 0)
	{
		DWORD childsID[15]={0};
		int childsNum = 0;
		getChildProcessID(processId,childsID,15,childsNum);
		DWORD  dwThreadId;
		if (childsNum == 0)
		{
			appThreadHandle=CreateThread(NULL,NULL,AnswerThread,hProcess,0,&dwThreadId);// wait calling process to end
		}
		else
		{
			static CHILDPROCESS childs={0};
			childs.childHandles[0] = hProcess;
			childs.childsCount = processIdToHandle(childsID,childs.childHandles+1,childsNum);
			appThreadHandle=CreateThread(NULL,NULL,AnswerThreadEx,&childs,0,&dwThreadId);// wait calling processes to end
		}
	}
	else
	{
		dwThreadId;
		appThreadHandle=CreateThread(NULL,NULL,AnswerThread,hProcess,0,&dwThreadId);// wait calling process to end
	}
	paraSession.processId = processId;

	WriteLog("[main]启动录像===================================");
	paraSession.startRecord();

	WriteLog("[main]启动视频监控===============================");
	retval = ShellRemoteControlServer();

    if(retval>0)
	{
	    memset(warningMsg,0,sizeof(warningMsg));
		HWND hWnd = GetForegroundWindow();
		switch(retval)
		{
			case 1:
				strcpy(warningMsg,"Database or table error！");
				break;
			case 2:
				strcpy(warningMsg,"Monitor service start failed！");
				break;
			default:
				strcpy(warningMsg,"Data conversion treatment failed！");
				break;
	   }
       WriteLog("init error,session exit!");
	   MessageBox(hWnd,warningMsg,"error",MB_ICONERROR|MB_OK|MB_TOPMOST);
	   return paraSession.ExitOscShell_Session(0);
	}
	WriteLog("[main]循环检查时间策略控制========================");
	CheckTimePolicy();
	//if (singleInstanceMutex != NULL)
	//{
	//	CloseHandle(singleInstanceMutex);
	//	singleInstanceMutex = NULL;
	//}
	if(singleInstanceSem != NULL)
	CloseHandle(singleInstanceSem);
	WriteLog("[main]OSCShell 正常结束!==========================");
	////写应用正常登出日志
	//获取当前时间
	memset(timestr, 0x0, 24);
	now_sec = time(NULL);
	strftime(timestr, _countof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&now_sec));
	file = fopen(logName, "a+");
	fprintf(file, "%s:OSCShellApp 命令行参数[%s] 应用正常结束!\n",timestr, argv[1]);

	fclose(file);
  	return paraSession.ExitOscShell_Session(0);	
}


//循环检查时间策略控制
void CheckTimePolicy()
{
	int count = 0;
	char warningMsg[512]={0};
	int retval;
	//这里应该守护进程 监测rdp的连接状态 如果是断开状态 停止录像 退出OSCSheelApp
	int showmsg = 0;
	int checkFlag=0;
	int remindFlag = 0;
	//获取配置文件中警告时间
	char szPath[512] = {0};
	GetModuleFileName(NULL,szPath,MAX_PATH);
	char *szSlash = strrchr(szPath, static_cast<int>('\\'));
	if (szSlash)
	{
		*++szSlash = '\0';
	}
	strcat(szSlash, "cfg.ini");
	int warningTimes_1 = 0;
	int warningTimes_2 = 0;
	int warningTimes_3 = 0;
	oscLoadCfgFile(szPath, &warningTimes_1, &warningTimes_2, &warningTimes_3);	
	memset(szPath,0,sizeof(szPath));

	while (paraSession.isLoop())
	{
		Sleep(1000);
		if(checkFlag==0)
		{
		   count++;
		}

		//2016-01-21 修改 从每2s刷新一次onLineTime时间改成每10s刷新一次
		if(count>0 && count%10==0 && paraSession.isLoop())
		{
			DoLoginLog(2);  //心跳
		}
		//2016-01-21 **************************************************

		//半分钟检查一次 时间策略控制
		if(count >= 30) 
		{
			checkFlag=1;
			count=0;
		}

		if(checkFlag==1) 
		{

			checkFlag = 0;
			int state = 0;
			int warningFlag = 0;

			retval=0;
			retval=GetVistitimeEx(&state,&remindFlag);
			if(state == 0)
			{
				WriteLog("access time zero, session exit!");
				if(hProcess!=NULL)
				{
					WriteLog("TerminateProcess process") ;
					TerminateProcess(hProcess,0);
					hProcess=NULL;
				}				
				break;
			}	
			memset(warningMsg,0,sizeof(warningMsg));
			if(showmsg == 0 && state>0)
			{
				switch(retval)
				{
					case -9:
						if(GetLocalLang() == 0){
							sprintf(warningMsg,"账号密码已被申请、访问结束前最大延时申请为%ld分钟！\n请备份好数据，如有疑问请联系管理员。",state);
						}else{
							sprintf(warningMsg,"account has been password apply,visit before the end of the maximum delay for %ld minutes\nplease backup your data,if you have any questions please contact administrator.",state);
						}
						break;
					case -16:
						if(GetLocalLang() == 0){
							strcpy(warningMsg,"该账号被申请账号密码、将在半小时后退出！\n请备份好数据，如有疑问请联系管理员。");
						}else{
							strcpy(warningMsg,"account has been password apply,remote access at the end of halfhours!\nplease backup your data,if you have any questions please contact administrator.");
						}
						break;
				}
			}
			if(state>0)
			{
							
				switch(retval)
				{
					case 1:
					case 9:
						if(GetLocalLang() == 0){
							strcpy(warningMsg,"资源访问时间限制");
						}else{
							strcpy(warningMsg,"resource access time limit");
						}
						break;
					case 2:
					case 3:
					case 10:
					case 11:
						if(GetLocalLang() == 0){
							strcpy(warningMsg,"申请访问时间限制");
						}else{
							strcpy(warningMsg,"apply access time limit");
						}
						break;
					case 4:
					case 12:
						if(GetLocalLang() == 0){
							strcpy(warningMsg,"策略时间限制");
						}else{
							strcpy(warningMsg,"policy time limit");
						}
						break;
					case 8:
						if(GetLocalLang() == 0){
							strcpy(warningMsg,"密码申请时间限制");
						}else{
							strcpy(warningMsg,"apply password time limit");
						}
						break;
					case 16:
						if(GetLocalLang() == 0){
							strcpy(warningMsg,"账号已被密码申请、时间到期");
						}else{
							strcpy(warningMsg,"account has been password apply,the end of time");
						}
						break;
				}

				int msglen=strlen(warningMsg);
				if(msglen>0)
				{				
					if(state>=warningTimes_1 && state < (warningTimes_1 + 300) && (remindFlag == 0 || remindFlag == 4))
					{
						if(GetLocalLang() == 0){
							sprintf(&warningMsg[msglen],"，远程访问将在%d分钟后结束！", state/60);
							//sprintf(&warningMsg[msglen],"，远程访问将在%d分钟后结束！",state/60);
						}else{
							sprintf(&warningMsg[msglen],",remote access at the end of %d minute!", state/60);
							//sprintf(&warningMsg[msglen],",remote access at the end of %d minute!",state/60);
						}
						remindFlag = 1;

						if(	retval==1 ||retval==9)
						{
							if(GetLocalLang() == 0){
								strcat(warningMsg,"\n请备份好数据或做延时申请，如有疑问请联系管理员。");	
							}else{
								strcat(warningMsg,"\nplease backup your data or apply for delay,if you have any questions please contact administrator.");	
							}
						}else
						{
							if(GetLocalLang() == 0){
								strcat(warningMsg,"\n请备份好数据，如有疑问请联系管理员。");		
							}else{
								strcat(warningMsg,"\nplease backup your data,if you have any questions please contact administrator.");	
							}
						}

						char temp[1024] = {0};
						sprintf(temp, "账号授权时间快要到期，提示信息：%s", warningMsg);
						WriteLog(temp);
						ShowWarning(warningMsg);
					}
					else if(state>= warningTimes_2 && state < (warningTimes_2 + 300) && (remindFlag == 0 || remindFlag == 1 || remindFlag == 4))
					{
						if(GetLocalLang() == 0){
							sprintf(&warningMsg[msglen],"，远程访问将在%d分钟后结束！", state/60);	
						}else{

							sprintf(&warningMsg[msglen],",remote access at the end of %d minute!", state/60);	
						}
						remindFlag = 2;

						if(	retval==1 ||retval==9)
						{
							if(GetLocalLang() == 0){
								strcat(warningMsg,"\n请备份好数据或做延时申请，如有疑问请联系管理员。");	
							}else{
								strcat(warningMsg,"\nplease backup your data or apply for delay,if you have any questions please contact administrator.");	
							}
						}else
						{
							if(GetLocalLang() == 0){
								strcat(warningMsg,"\n请备份好数据，如有疑问请联系管理员。");		
							}else{
								strcat(warningMsg,"\nplease backup your data,if you have any questions please contact administrator.");	
							}
						}

						char temp[1024] = {0};
						sprintf(temp, "账号授权时间快要到期，提示信息：%s", warningMsg);
						WriteLog(temp);
						ShowWarning(warningMsg);
					}
					else if (state>= warningTimes_3 && state < (warningTimes_3 + 300) && (remindFlag == 0 || remindFlag == 2 || remindFlag == 4))
					{
						if(GetLocalLang() == 0){
							sprintf(&warningMsg[msglen],"，远程访问将在%d分钟后结束！", state/60);	
						}else{

							sprintf(&warningMsg[msglen],",remote access at the end of %d minute!", state/60);	
						}
						remindFlag = 3;

						if(	retval==1 ||retval==9)
						{
							if(GetLocalLang() == 0){
								strcat(warningMsg,"\n请备份好数据或做延时申请，如有疑问请联系管理员。");	
							}else{
								strcat(warningMsg,"\nplease backup your data or apply for delay,if you have any questions please contact administrator.");	
							}
						}else
						{
							if(GetLocalLang() == 0){
								strcat(warningMsg,"\n请备份好数据，如有疑问请联系管理员。");		
							}else{
								strcat(warningMsg,"\nplease backup your data,if you have any questions please contact administrator.");	
							}
						}

						char temp[1024] = {0};
						sprintf(temp, "账号授权时间快要到期，提示信息：%s", warningMsg);
						WriteLog(temp);
						ShowWarning(warningMsg);
					}		
					
				}
			}
			/*if(showmsg == 1 &&  state>0 && warningFlag ==2)
			{
				showmsg=2;
				switch(retval)
				{
					case 1:
					case 9:
					   
					   if(GetLocalLang() == 0){
						   strcpy(warningMsg,"资源访问时间限制");
					   }else{
						   strcpy(warningMsg,"resource access time limit");
					   }
						break;
					case 2:
					case 3:
					case 10:
					case 11:
						if(GetLocalLang() == 0){
							strcpy(warningMsg,"申请访问时间限制");
						}else{
							strcpy(warningMsg,"apply access time limit");
						}
						break;
					case 4:
					case 12:
						if(GetLocalLang() == 0){
							strcpy(warningMsg,"策略时间限制");
						}else{
							strcpy(warningMsg,"policy time limit");
						}
						break;
					case 8:
						if(GetLocalLang() == 0){
							strcpy(warningMsg,"密码申请时间限制");
						}else{
							strcpy(warningMsg,"apply password time limit");
						}
						break;
					case 16:
						if(GetLocalLang() == 0){
							strcpy(warningMsg,"账号已被密码申请、时间到期");
						}else{
							strcpy(warningMsg,"account has been password apply、the end of time");
						}
						break;
				}
				int msglen=strlen(warningMsg);
				if(msglen>0)
				{
					if(state>=3600)
					{
					   if(GetLocalLang() == 0){
						   sprintf(&warningMsg[msglen],"，远程访问将在%d小时后结束！",state/3600);	
					   }else{
						   sprintf(&warningMsg[msglen],",remote access at the end of %d hours!",state/3600);	
					   }
					}else
					if(state>=60)
					{
						if(GetLocalLang() == 0){
							sprintf(&warningMsg[msglen],"，远程访问将在%d分钟后结束！",state/60);
						}else{
							sprintf(&warningMsg[msglen],",remote access at the end of %d minute!",state/60);
						}
					}else{
						if(GetLocalLang() == 0){
							sprintf(&warningMsg[msglen],"，远程访问将在%d秒后结束！",state);	
						}else{
							sprintf(&warningMsg[msglen],",remote access at the end of %d second!",state);	
						}
					}

					if(	retval==1 ||retval==9)
					{
						if(GetLocalLang() == 0){
							strcat(warningMsg,"\n请备份好数据或做延时申请，如有疑问请联系管理员。");	
						}else{
							strcat(warningMsg,"\nplease backup your data or apply for delay,if you have any questions please contact administrator.");	
						}
					}else
					{
						if(GetLocalLang() == 0){
							strcat(warningMsg,"\n请备份好数据，如有疑问请联系管理员。");		
						}else{
							strcat(warningMsg,"\nplease backup your data,if you have any questions please contact administrator.");	
						}
					}
					char temp[1024] = {0};
					sprintf(temp, "账号授权时间快要到期，提示信息：%s", warningMsg);
					WriteLog(temp);
					ShowWarning(warningMsg);
				}
			}
			if(showmsg==2 && state>0 && warningFlag==3)
			{
				showmsg=3;
				switch(retval)
				{
					case 1:
					case 9:
						if(GetLocalLang() == 0){
							strcpy(warningMsg,"资源访问时间限制");
						}else{
							strcpy(warningMsg,"resource access time limit");
						}
						break;
					case 2:
					case 3:
					case 10:
					case 11:
						if(GetLocalLang() == 0){
							strcpy(warningMsg,"申请访问时间限制");
						}else{
							strcpy(warningMsg,"apply access time limit");
						}
						break;
					case 4:
					case 12:
						if(GetLocalLang() == 0){
							strcpy(warningMsg,"策略时间限制");
						}else{
							strcpy(warningMsg,"policy time limit");
						}
						break;
					case 8:
						if(GetLocalLang() == 0){
							strcpy(warningMsg,"密码申请时间限制");
						}else{
							strcpy(warningMsg,"apply password time limit");
						}
						break;
					case 16:
						if(GetLocalLang() == 0){
							strcpy(warningMsg,"账号已被密码申请、时间到期");
						}else{
							strcpy(warningMsg,"account has been password apply、the end of time");
						}
						break;
				}
				int msglen=strlen(warningMsg);
				if(msglen>0)
				{
					if(state>=3600)
					{
						if(GetLocalLang() == 0){
							sprintf(&warningMsg[msglen],"，远程访问将在%d小时后结束！",state/3600);	
						}else{
							sprintf(&warningMsg[msglen],",remote access at the end of %d hours!",state/3600);	
						}
					}else
						if(state>=60)
						{
							if(GetLocalLang() == 0){
								sprintf(&warningMsg[msglen],"，远程访问将在%d分钟后结束！",state/60);
							}else{
								sprintf(&warningMsg[msglen],",remote access at the end of %d minute!",state/60);
							}
						}else{
							if(GetLocalLang() == 0){
								sprintf(&warningMsg[msglen],"，远程访问将在%d秒后结束！",state);	
							}else{
								sprintf(&warningMsg[msglen],",remote access at the end of %d second!",state);	
							}
						}

						if(	retval==1 ||retval==9)
						{
							if(GetLocalLang() == 0){
								strcat(warningMsg,"\n请备份好数据或做延时申请，如有疑问请联系管理员。");	
							}else{
								strcat(warningMsg,"\nplease backup your data or apply for delay,if you have any questions please contact administrator.");	
							}
						}else
						{
							if(GetLocalLang() == 0){
								strcat(warningMsg,"\n请备份好数据，如有疑问请联系管理员。");		
							}else{
								strcat(warningMsg,"\nplease backup your data,if you have any questions please contact administrator.");	
							}
						}
						char temp[1024] = {0};
						sprintf(temp, "账号授权时间快要到期，提示信息：%s", warningMsg);
						WriteLog(temp);
						ShowWarning(warningMsg);
				}
			}*/
		}
	}
}

void getChildProcessID(IN DWORD parentProcessID,OUT DWORD *childProcessIDs,IN int MAXIDS,OUT int& childsCount )
{
	int repeatTimes = 0;
	childsCount = 0;
tryAgain:
	PROCESSENTRY32 procEntry;
	procEntry.dwSize = sizeof(PROCESSENTRY32);
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap == INVALID_HANDLE_VALUE)
	{
		return;
	}
	if (!Process32First(hSnap, &procEntry))//从Snapshot得到第一个进程记录信息
	{
		return;
	}
	int i = 0;
	do
	{
		if(i > MAXIDS)
			break;
		DWORD winlogonSessId = 0;
		if(procEntry.th32ParentProcessID == parentProcessID)
		{
			childProcessIDs[i++] = procEntry.th32ProcessID;
			childsCount ++;
		}
	} while (Process32Next(hSnap, &procEntry));
	CloseHandle(hSnap);
	if(i <= 0 && repeatTimes <3)
	{
		repeatTimes ++;
		Sleep(100);
		goto tryAgain;
	}
}