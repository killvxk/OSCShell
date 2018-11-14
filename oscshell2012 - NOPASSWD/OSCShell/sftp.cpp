#include "sftp.h"
#include <stdio.h>
#include "pub.h"

void shellSftp(CHAR *ServerName,CHAR *UserName,CHAR *Password)
{
	CHAR szCmdline[MAX_PATH];
	sprintf(szCmdline,"%s@%s -pw %s",UserName,ServerName,Password);
	ShellExecuteA(NULL,"open","psftp",szCmdline,NULL,SW_SHOW);
}