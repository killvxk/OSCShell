// ScreenRecorderTest.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdio.h>

extern   "C" __declspec(dllimport) void __stdcall OnRecord(char *szRec);
extern   "C" __declspec(dllimport) void __stdcall OnStop();

int _tmain(int argc, _TCHAR* argv[])
{
	printf("Screen Recording ...\n");
	char *szRec = "c:\\testlx.avi";
	OnRecord(szRec);

	char c=0;
	//printf("press any key to stop ...\n");
	//printf("press enter key to stop ...\n");
	
	OnStop();
	//printf("please waiting for make swf ...\n");
	getchar();

	return 0;
}

