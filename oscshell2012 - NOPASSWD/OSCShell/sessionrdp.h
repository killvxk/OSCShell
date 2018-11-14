#ifndef SESSIONRDP_H
#define SESSIONRDP_H
#include <Windows.h>
#include <wtsapi32.h>

#pragma	  warning(disable:4996)

void getLocalAddress(char * IpAddress);
DWORD getSessionProcessPID(TCHAR *name);

#endif