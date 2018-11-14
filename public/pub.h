#ifndef PUB_H
#define PUB_H
#include <Windows.h>

void get32Id(char *id);
int WharToMByte(LPTSTR wideChar,char* narrowChar) ;
BOOL MByteToWChar(LPCSTR lpcszStr, LPWSTR lpwszStr, DWORD dwSize);

void SetHooks(void);
void RemoveHooks(void);
#endif