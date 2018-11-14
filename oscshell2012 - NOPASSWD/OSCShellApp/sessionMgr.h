#ifndef SESSIONMGR
#define SESSIONMGR


class SESSIONMANAGER
{
private:
	 int  needAudit;
	 int  exitmsg;
	 int  noConsole;
	 HANDLE g_mutex;
	 int  channelFlag;
	
	 char disConnEventName[50];
	 char reConnEventName[50];
	 HANDLE g_disConnEvent;
	 HANDLE g_reConnEvent;
public:
    char szloginfile[1024];
	char szAvifile[1024];
	DWORD processId;
	
	
	bool isExit();
	void openLogs(char* g_szAccessId = NULL);
	void startRecord();
	bool exitSession();
	bool ExitOscShell_Session(int flag);
	void initSession(char *channelMutesName);
	void ExitOscShell(int flag);
	bool isLoop();
    SESSIONMANAGER(void);
    ~SESSIONMANAGER(void);
};
extern SESSIONMANAGER paraSession;

#endif
