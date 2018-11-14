#include <winsock2.h>
#include <stdio.h>
#include <tlhelp32.h>
#include "easyhook.h" 
#include "db_table.h"
#pragma comment(lib,"ws2_32")
#pragma comment (lib,"Advapi32.lib")

BOOL writeData(void *data,int len,char *dbPidFile)
{
	FILE *stream = NULL;
	__try
	{
		if((stream = fopen(dbPidFile, "wb")) ==NULL)
		{
			__leave;
		}		
		fwrite(data,len,1,stream);

	}//end of try
	__finally
	{
		if(stream)
			fclose(stream);
	}
	return TRUE;
}


int loadDbAudit(STRUCT_DBAUDITINFO *auditInfo,char *accessPath,unsigned long pid)
 
{
	PWSTR pszLibFileRemote = NULL;
	HANDLE hRemoteProcess = NULL,hRemoteThread = NULL;
	char CurPath[512];

	HKEY hk;
	BYTE Vals[100];
	DWORD lenIt = 100;

	memset(CurPath, 0, sizeof(CurPath));
	strcpy(CurPath,accessPath);

	if(RegCreateKeyA( HKEY_LOCAL_MACHINE, "SOFTWARE\\ParaView\\OscShell\\", &hk ) == ERROR_SUCCESS )
	 {
		  memset(Vals,0,sizeof(Vals));
		  RegQueryValueExA(hk, "Path", 0, NULL, (BYTE*)Vals, &lenIt );
		  if(strlen((char *)Vals)==0 || strcmp(CurPath,(char *)Vals)!=0)
		  {
			  if(RegSetValueExA( hk, "Path", 0, REG_SZ, (LPBYTE)(LPCSTR)CurPath, strlen(CurPath)+1 ) != ERROR_SUCCESS )
			  {
				 MessageBoxA(NULL,"write db audit path error!","error",1);
				 RegCloseKey( hk );
				 return -1;
			  }
		  }

	 }
	 RegCloseKey( hk );

	sprintf(accessPath,"%s%d",CurPath,pid);	

	writeData(auditInfo,sizeof(STRUCT_DBAUDITINFO),accessPath);	

	strcat(CurPath, "OSCAudit.dll");
	WCHAR wCurPath[512];
	MultiByteToWideChar(CP_ACP,0,CurPath,-1,wCurPath,512);
   
	NTSTATUS  nt=RhInjectLibrary(pid, 0L, EASYHOOK_INJECT_DEFAULT,wCurPath, NULL, NULL,0L);	
	if(nt != 0L)
		return -1;
	return 0;
}