#pragma once

#include <Windows.h>
#include <stdio.h>
#include <string.h>

#define DEBUG_LEVEL  2
#define INFO_LEVEL   1
#define ERROR_LEVEL  0
#define LOG_ON 1
#define LOG_OFF -1

#pragma	  warning(disable:4996)
#pragma	  warning(disable:4267)

class CMyLogFile 
{
public:
	char *m_strFile;
public:
	CMyLogFile()
	{
		m_strFile = NULL;
	};
	~CMyLogFile(){delete [] m_strFile;};
	void SetFileName(char *strFile,int len)
	{  
		if (m_strFile == NULL)
		{
			m_strFile = new char[len+1];
			strcpy(m_strFile,strFile);
		}

	};
	void Init()
	{
		if(m_strFile == NULL)
			return;
		FILE *file = NULL;
		file = fopen(m_strFile,"ab+");
		if (file != NULL)
		{
			fclose(file);
		}
	};
	void WriteLine(char *strline)
	{
		if(m_strFile==NULL)
			return;

		FILE *file = NULL;
		file = fopen(m_strFile,"rt+");
		if (file != NULL)
		{
			fseek(file, 0L, SEEK_END);
			fwrite( strline, sizeof(char),strlen(strline)+1, file );
			fclose(file);
		}
	};
};

#define DeleteAndNull(p){delete (p);(p)=NULL;}
#define DeleteArrayAndNull(p){delete[] (p);(p)=NULL;}
struct LY_SERVER_LOG
{
	char	szConAddr[30];
	char	szLogType[20];
	char	szLogDate[30];
	char    szLogCmd[30] ;
	char    szLogInfo[200];	
};

class CLogMan
{
public:
	CLogMan(void);
public:
	~CLogMan(void);
public:
	static HANDLE l_mutex;
	//日志文件句柄
	static CMyLogFile * m_pSysLog ;
	//输出类型: 0：error;1:info;2:debug
	static int m_out_type; 
    static int log_switch;//1,打开，其他，关闭
public:
	static int WideCharToChar(LPTSTR wideChar,char* narrowChar) ;
	static char* IntToChar(int iValue) ;
	//写日志到文件中
	//获得日至文件
	static void GetLogFile(TCHAR *szLogPath) ;
	//打开日至文件
	static int OpenLogFile(char *g_szAccessId=NULL);
	//打开日至文件
	static void setLogLevel(int level);
	//关闭日至文件
	static int ClosLogFile() ;
	//写入日至文件 va_list _ArgList
	static int error_log(__in_z __format_string const char * _Format, ...);  //error level 0
	static int info_log(__in_z __format_string const char * _Format, ...);   //info  level 1
	static int debug_log(__in_z __format_string const char * _Format, ...);  //debug level 2
	static int write_log(int level,char *sMsg) ;

};
