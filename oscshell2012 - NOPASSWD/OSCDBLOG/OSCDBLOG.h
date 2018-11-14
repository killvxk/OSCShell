#ifndef OSCDBLOG_H
#define OSCDBLOG_H

#ifdef OSCDBLOG_EXPORTS 
#define OSCDBLOG_EXPORTS __declspec(dllexport)
#else
#define OSCDBLOG_EXPORTS __declspec(dllimport)
#endif

typedef struct
{
	char dbType[10];  //MYSQL,ORACLE,DB2
	char dbHost[20];
	char dbUser[20];
	char dbPwd[128];
	char dbName[20];
	char dbPort[20];
}STRUCT_DBCFGINFO;

typedef struct
{
	STRUCT_DBCFGINFO auditConn;
	STRUCT_DBCFGINFO oscConn;
	char code[10];
	char level[50];
	char monitorPort[10];
	char videoPath[256];
	char auditPath[256];
	char maxOutRows[10];
	char maxThreads[10];
	char maxCommitRows[10];
	char warningTimes_1[10];
	char warningTimes_2[10];
	char warningTimes_3[10];
	char proxyIp[50];
	char proxyConnectPort[10];
}STRUCT_SYSCONFIG;

OSCDBLOG_EXPORTS int DBInsertAuthMessage(int level, char *msg);

#endif