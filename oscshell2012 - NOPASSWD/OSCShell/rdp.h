#ifndef RDP_H
#define RDP_H
#include <Windows.h>

#pragma	  warning(disable:4996)

int checkLocalAddrByDest(char *destAddress,char *outAddress);
void writeWS(TCHAR *szPath,LPTSTR filename,TCHAR *ServerName,TCHAR *UserName,TCHAR *privatePath);
void writeRDP(TCHAR *szPath,TCHAR *DomainName,TCHAR *ServerName,TCHAR *UserName,TCHAR *Password,int diskFlag,int bpp,int Fullscreen);
#endif