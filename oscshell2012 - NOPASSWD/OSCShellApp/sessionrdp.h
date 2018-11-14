#ifndef SESSIONRDP_H
#define SESSIONRDP_H
#include <Windows.h>
#include <wtsapi32.h>

bool  DisconnSession(void);
bool  checkSessionStatus(void);
bool  clientProtocolType(void);
char  *getClientAddress(void);
bool  IsRDPUser(void);
DWORD getSessionProcessPID(char *name);
DWORD  getCurrSessionID();
#endif