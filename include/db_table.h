#ifndef _dbtable_h
#define _dbtable_h

//#define PR_SPLITFLAG   "P^_^R"
#define MAXEXECPATH 513
#define MAXCMDBUFF  200


#define TYPE_SSH         100
#define TYPE_TELNET      101
#define TYPE_SFTP        102
#define TYPE_FTP         103
#define TYPE_RDP         104
#define TYPE_XMANAGER    105
#define TYPE_AS400       106    
#define TYPE_F5          107   
#define TYPE_XEN         108   
#define TYPE_IPMI        109 
#define TYPE_SQLPLUS     201
#define TYPE_PLSQL       202
#define TYPE_DB2         501
#define TYPE_MSSQL       503
#define TYPE_SYBASE      504
#define TYPE_MYSQL       505
#define TYPE_INFOMIX	 506
#define TYPE_PQSQL	     507
#define TYPE_PQSQL_N     607
#define TYPE_SQLPLUS_N   608
#define TYPE_PLSQL_N     609
#define TYPE_HTTP        900
#define TYPE_HTTPS       901
#define TYPE_UNDEFINE    999
#define TYPE_RADMIN      801
#define TYPE_ZOS		 802

typedef struct
{
	char timeFlag[5];     //0申请时间 1资源时间
    char startTime[21];
    char endTime[21];
	char resetPwdTime[21];
}STRUCT_TIMEAUTH;

//资源访问表
typedef struct
{
    char id[33];
    char userId[33];
    char uid[33];
    char userName[51];
    char resId[33];
    char resAddr[21];
    char resName[51];
    char resType[21];
    char resVersion[21];
    char accountId[33];
    char accountNo[21];
    char accountType[5];   // 1个人 2 特权 
    char accountPwd[300];   // 密文存放
    char suFlag[5];        //1执行  0不执行
	char ssoFlag[5];       //1执行  0不执行
	char auditFlag[5];     //auditFlag审计方式： 0全部，1不审计 2特权账号审计 3个人账号审计
	char auditType[5];     //pr_auditType审计类型：0混合审计 1命令审计 2图形审计 3不审计
	char cmdFlag[5];       //1黑名单    2白名单
    char clientAddr[21];
	char apId[33];         //访问协议ID
    char accessType[5];    // 访问协议类型(来自Pr_AccessProtocol表中code)
    char accountNoEx[21];
    char accountPwdEx[300]; // 密文存放
    char domainName[61];
    char screenWidth[6];
    char screenHeight[6];
    char proxyAddr[21];
}STRUCT_ACCESSAUTH;

//资源访问表
typedef struct
{
	STRUCT_ACCESSAUTH accessAuth;
	STRUCT_TIMEAUTH   timeAuth;
    char execApp[MAXEXECPATH];
	char execParam[MAXEXECPATH];
	char proxyHost[20];
	char proxyPort[10];
}STRUCT_AUTHACCESSRES;

//命令限制表
typedef struct
{
   char cmdFlag;
   char level; 
   char command[MAXCMDBUFF];
}STRUCT_AUTHACCESSLIMIT;

//命令日志表
typedef struct
{
    char id[33];
    char clientAddr[21];
    char proxyAddr[21];
    char resId[32];
    char resName[51];
    char resAddr[21];
    char command[512];
    char orgId[33];
    char orgName[51];
    char userId[33];
    char userName[51];
    char uid[33];
    char accountNo[51];
    char operDate[20];
    char sessionId[33];
    int  level;              // 告警级别：0none 1fatal 2critical 3minor 4warning 5information 
    int  accessType;         // 访问协议类型(来自Pr_AccessProtocol表中code)
}STRUCT_USERACTIVITYLOG;

//输出日志表
typedef struct
{
    char id[33];
    char ualId[33];
    char desc[1024];
    char operDate[20];
}STRUCT_USERACTIVITYOUTLOG;

//视频日志表
typedef struct
{
    char id[33];
    char resId[33];
    char resName[51];
    char resAddr[51];
    char clientAddr[51];
    char userId[32];
    char userName[51];
    char uid[33];
    char accountNo[50];
    char operDate[21];
    char sessionId[33];
    char filePath[200];
}STRUCT_USERACTIVITYVIDEOLOG;
#ifndef SYSLOG
#define SYSLOG //sugl
#define SVW
//#define QINGPU
#endif
typedef struct
{
    char id[33];
    char uid[33];
    char userName[51];
    char orgId[33];
    char orgName[51];
    char accountNo[21];
    char clientAddr[20];
    char resAddr[20];
    char resId[33];
    char resName[51];
    char resType[20];
	char apId[33];      // 访问协议ID
    int  accessType;    // 访问协议类型(来自Pr_AccessProtocol表中code)
    char loginDate[20];
    char loginOutDate[20];
	char onLineTime[20];
	char proxyAddr[20];
	int  cmdFlag;
	int  port;
	int  status;
	int  nodeFlag;    //多个堡垒主机标识
	char authaccessresid[32];
	char proxyHost[20];
	char pr_cooperator[500];
	char pr_accessMemo[200];
#ifdef SYSLOG
	char accessName[50];//访问协议类型(来自Pr_AccessProtocol表中name)
#endif
}STRUCT_LOGINLOG;


typedef struct
{
	char dbType[10];  //MYSQL,ORACLE,DB2
    char dbHost[20];
	char dbUser[20];
	char dbPwd[20];
	char dbName[20];
	int  dbPort;
}STRUCT_DBCONNECT;


typedef struct{
	   int   flag;          //-1: test 0:audit 1:attach
       unsigned long pid;   //flag==1   app pid
	   char  debug;         //0:no log and only write db 1: log and db 2: log and no write db
       char  sessionId[33]; 
	   char  auditPath[128]; //flag==1  userName
	   char  accessPath[128];//flag==1  userPwd
}STRUCT_DBAUDITINFO;

typedef struct
{
    char id[33];
    char timeStamp[20];   
	unsigned char  level;       //cmd level
	unsigned char  flag;        //1 cmd; 0 out     
	unsigned short suffixlen;   //suffix data len          
    unsigned short datalen;     //total data len
}LOG_HEADER;


typedef struct
{
	int flag;
	unsigned long sid;
	unsigned char onlineTime[20];
    unsigned char accessId[33];
	unsigned char loginId[33];
//	unsigned char videoName[100];
}NOTICE_STRUCT;



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

typedef struct{
	char pr_key[1024];
	char pr_basekey[1024];
}AESKEY;

typedef struct{
	char pr_cooperator[1024];
	char pr_accessMemo[1024];
}ResPR;

typedef struct {
	char sessionid[10];
}WinSessionId,*PWinSessionId;

#endif /* _dbtable_h */