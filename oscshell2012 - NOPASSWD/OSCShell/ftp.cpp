#include "ftp.h"
#include <stdio.h>
#include "pub.h"

typedef struct{
	CHAR* line;
} ftpLine;

ftpLine ftpLines[] = {
	{"open %s\r\n"},
	{"%s\r\n"},
	//{"%s\r\n"},
	NULL
};



void GetFilePath(LPTSTR strFile)
{
	char dir[_MAX_FNAME];
	char drive[3];
	char sFile[MAX_PATH*2]={0};
	char sAppDir[MAX_PATH*2]={0};
	WharToMByte(strFile,sFile);
	_splitpath(sFile,drive,dir,NULL,NULL);
	strcat(sAppDir,drive);
	strcat(sAppDir,dir);
	//×ª»»
	MByteToWChar(sAppDir,strFile,MAX_PATH);
}

LPTSTR GetWorkDir()
{
	TCHAR szPath[MAX_PATH];
	GetModuleFileName(NULL,szPath,MAX_PATH);
	GetFilePath(szPath);
	if( szPath[wcslen(szPath) -1] != '\\' )
	{
		wcscat(szPath, L"\\");
	}
	return szPath;
}

void writeFTP(CHAR *ServerName,CHAR *UserName,CHAR *Password){
	FILE  *pFile=NULL;
	LPTSTR filename = GetWorkDir();
	TCHAR szPath[MAX_PATH];
	wcscpy(szPath,filename);
	wcscat(szPath,L"ftp.scr");
	
	//BYTE buffer[2];
	//buffer[0]=0xFF;
	//buffer[1]=0xFE;
	CHAR szTemp[MAX_PATH];
	size_t str_length = 0;

	pFile = _wfopen(szPath,L"w+b");
	if(pFile != NULL)
	{
		//fwrite(buffer, 2, 1, pFile);
		int c=0;
		do{
			switch(c)
			{
			case 0:
				sprintf(szTemp, ftpLines[c].line, ServerName);
				break;
			case 1:
				sprintf(szTemp, ftpLines[c].line, UserName);
				break;
			case 2:
			//	sprintf(szTemp, ftpLines[c].line, Password);
				break;
			}
			//write line by line
			str_length = strlen(szTemp);
			fwrite(szTemp , str_length, 1, pFile);
			c++;
		}while( ftpLines[c].line != NULL );
		fclose(pFile);
	}
}
