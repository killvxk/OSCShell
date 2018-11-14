#include <stdarg.h>
#include <io.h>
#include "LogMan.h"
#include "db_table.h"
#include <iostream>
#include <atltime.h>
using namespace std;

CMyLogFile * CLogMan::m_pSysLog=NULL;
int CLogMan::m_out_type =0;
HANDLE CLogMan::l_mutex = NULL;
extern STRUCT_SYSCONFIG sysconfig;
int CLogMan::log_switch=LOG_OFF;
CLogMan::CLogMan(void)
{		
}

CLogMan::~CLogMan(void)
{
}

int CLogMan::WideCharToChar(LPTSTR wideChar,char* narrowChar) 
{
	int nLength = WideCharToMultiByte(CP_ACP,NULL,wideChar,-1,NULL,0,NULL,NULL);
	WideCharToMultiByte(CP_ACP,NULL,wideChar,-1,narrowChar,nLength,NULL,NULL);
	return 1 ;
}

char* CLogMan::IntToChar(int iValue)
{
	char *sMsg = new char[30] ;
	_itoa_s(iValue,sMsg,30,10) ;
	return sMsg ;
}

extern BOOL MByteToWChar(LPCSTR lpcszStr, LPWSTR lpwszStr, DWORD dwSize);
void CLogMan::GetLogFile(TCHAR *szLogPath)
{
	//TCHAR szLogPath[MAX_PATH*2] = {0};
	TCHAR tmp[MAX_PATH] = {0};
	if(strlen(sysconfig.auditPath)==0)
	{
		GetModuleFileName(NULL, tmp, sizeof(tmp)/sizeof(tmp[0]));	
		LPTSTR tszSlash = _tcsrchr(tmp, static_cast<int>('\\'));
		if (tszSlash)
			*++tszSlash = '\0';
	}else{
  		MByteToWChar((CHAR*)sysconfig.auditPath,tmp,sizeof(tmp)/sizeof(tmp[0]));		
	}

	wcscpy(szLogPath,tmp);	
	wcscat(szLogPath,L"log");
//  MessageBoxA(NULL,szLogPath,"info",MB_OK);
//	CreateDirectory(szLogPath, NULL);
	CTime now = CTime::GetCurrentTime();
	int day = now.GetDay();
	int month = now.GetMonth();
	int year = now.GetYear();
	TCHAR szLogDate[32]={0};
	swprintf(szLogDate,L"%04d%02i%02i",year,month,day);
	wcscat(szLogPath,L"\\osc");
	wcscat(szLogPath,szLogDate);	
//	return szLogPath ;
}
void CLogMan::setLogLevel(int level)
{
	m_out_type=level;
}

int CLogMan::OpenLogFile(char *g_szAccessId)
{
	if (log_switch != LOG_ON)
	{
		return 0;
	}
	if(!l_mutex)
		l_mutex = CreateMutex(NULL, FALSE, NULL);
	TCHAR sFileName[512] ={0};
	GetLogFile(sFileName);
//MessageBox(NULL,sFileName,L"info",MB_OK);
	if( !m_pSysLog )
		m_pSysLog = new CMyLogFile();

	char FileName[MAX_PATH*2] = {0};
	WideCharToChar(sFileName,FileName); 
	if (g_szAccessId != NULL && strlen(g_szAccessId) > 0)
	{
		strcat(FileName,"-");
		strcat(FileName,g_szAccessId);
		strcat(FileName,".log");
	}
	else
		strcat(FileName,".log");
	m_pSysLog->SetFileName(FileName,strlen(FileName));
	m_pSysLog->Init();
	return 1 ;
}

int CLogMan::ClosLogFile()
{
	if(m_pSysLog!=NULL)
	{
	    DeleteAndNull(m_pSysLog);
		m_pSysLog=NULL;
	}
	if (l_mutex)
	{
		CloseHandle(l_mutex);
	}
	return 1 ;
}
int CLogMan::debug_log(__in_z __format_string const char * _Format, ...)
{
	if (log_switch != LOG_ON)
	{
		return 0;
	}
	char sMsg[2048] = {0};
	va_list _ArgList;
	va_start( _ArgList, _Format );
	vsprintf( sMsg, _Format, _ArgList ); 	
	return write_log(2,sMsg);

}
int CLogMan::info_log(__in_z __format_string const char * _Format, ...)
{
	if (log_switch != LOG_ON)
	{
		return 0;
	}
	char sMsg[2048] = {0};
	va_list _ArgList;
	va_start( _ArgList, _Format );
	vsprintf( sMsg, _Format, _ArgList ); 

	return write_log(1,sMsg);
}

int CLogMan::error_log(__in_z __format_string const char * _Format, ...)
{
	if (log_switch != LOG_ON)
	{
		return 0;
	}
	char sMsg[2048] = {0};
	va_list _ArgList;
	va_start( _ArgList, _Format );
	vsprintf( sMsg, _Format, _ArgList ); 
	return write_log(0,sMsg);
}

int CLogMan::write_log(int level,char *sMsg)
{
	if (log_switch != LOG_ON)
	{
		return 0;
	}
	WaitForSingleObject(l_mutex,INFINITE);
	if ( m_pSysLog==NULL )
		return -1;

	//²»Êä³ö
	if (m_out_type<level)
	{
		return 1;
	}
	OutputDebugStringA(sMsg);

	char sHead[200]  = {0};

	CTime now = CTime::GetCurrentTime();
	int day = now.GetDay();
	int month = now.GetMonth();
	int year = now.GetYear();
	int hour = now.GetHour() ;
	int minutes = now.GetMinute() ;
	int second = now.GetSecond() ;
	sprintf(sHead,"%04d-%02i-%02i %02i:%02i:%02i [%d]",year,month,day,hour,minutes,second,GetCurrentThreadId());
	m_pSysLog->WriteLine(sHead);

	if (level<=m_out_type)
	{
		m_pSysLog->WriteLine(sMsg);
		m_pSysLog->WriteLine("\n") ;
	}
	ReleaseMutex(l_mutex);
	return 1 ;
}