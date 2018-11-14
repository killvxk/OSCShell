// CompareVideo.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "CompareVideo.h"
#include <afx.h>

#define MAX_LOADSTRING 100

// 全局变量:
HINSTANCE hInst;								// 当前实例
TCHAR szTitle[MAX_LOADSTRING];					// 标题栏文本
TCHAR szWindowClass[MAX_LOADSTRING];			// 主窗口类名
void CompareDir(char * dir,bool isnow);
int WharToMByte(wchar_t * wideChar,char* narrowChar) ;

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);


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

	char videoPath[256] = {0};
	GetPrivateProfileStringA("node","videoPath","",videoPath,sizeof(videoPath),szCmdline);
 	
	if (strlen((const char*)lpCmdLine) > 0)
	{
		CompareDir(videoPath,true);
	}
	else
	{
		CompareDir(videoPath,false);
	}
	return 0;
}

int WharToMByte(wchar_t * wideChar,char* narrowChar) 
{
	int nLength = WideCharToMultiByte(CP_ACP,NULL,wideChar,-1,NULL,0,NULL,NULL);
	WideCharToMultiByte(CP_ACP,NULL,wideChar,-1,narrowChar,nLength,NULL,NULL);
	return 1 ;
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

	//CLogMan::debug_log("CompareFile file=%s",path);

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
		//CLogMan::debug_log("Error On CreatePipe()\n"); 
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
			//CLogMan::debug_log("Error on CreateProcess()\n"); 
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
		//CLogMan::debug_log("CompareFile not find libx264 @");
		return ; 
	}
	if(MoveFileA(path,bakPath) == FALSE){
		//CLogMan::debug_log("can not MoveFile");
		return;
	}
	if (MoveFileA(cmpPath,path) == FALSE)
	{
		MoveFileA(bakPath,path);
	}
	DeleteFileA(bakPath);

	//CLogMan::debug_log("CompareFile end");

	Sleep(1000);
}

void CompareDir(char * dir,bool isnow)
{

	char szFind[MAX_PATH];
	char szFile[MAX_PATH];

	WIN32_FIND_DATAA FindFileData;

	strcpy(szFind,dir);
	strcat(szFind,"*.*");

	//CLogMan::debug_log("CompareDir dir=%s",dir);

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
				if(GetFileOpeDate(filepath) != 1)
				{
					continue;
				}
			}
			CompareFile(filepath);
		}
		if(!FindNextFileA(hFind,&FindFileData))
			break;
	}
	FindClose(hFind); 
}