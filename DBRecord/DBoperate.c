//#include "datasafe.h"
#include "DBoperate.h"
  

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>
#include "syslogEx.h"
#include "../ODBCManage/datasafe.h"
#include <iconv.h>
char serverip[32];
int port;
char username[50];
char userpwd[256];
char database[50];
char dbtype[50];


int _isconnected = 0;
MYSQL mysql;
MYSQL *mysql_sock = NULL;


int audit_port;
char audit_username[50];
char audit_userpwd[256];
char audit_database[50];
char audit_dbtype[50];
char audit_serverip[32];
int _audit_isconnected = 0;
MYSQL audit_mysql;
MYSQL *audit_mysql_sock = NULL;
char *strupr(char *str) {
    char *ptr = str;
    while (*ptr != 0) {
        if (islower(*ptr))
            *ptr = toupper(*ptr);
        ptr++;
    }
    return str;
}

int printdblog(const char* format, ...) {
    static char sprint_buf[1024 * 20];
    va_list args;
    int n;
	if(access("/etc/dbrecord.log",F_OK) != 0)
        return 0;
    FILE *fa;
    fa = fopen("/etc/dbrecord.log", "a+");
    if (fa == NULL) {
        return 0;
    }

    va_start(args, format);
    n = vsprintf(sprint_buf, format, args);
    va_end(args);

    fprintf(fa, sprint_buf);
    fclose(fa);
    return 0;
}

int ConnectDB(char * serverip, int port, char * username, char * userpwd, char * database) {
    printdblog("check _isconnected:%d\n", _isconnected);
    if (_isconnected == 1) {
        return 0;
    }
    if (mysql_sock != NULL) {
        return 0;
    }
    if (serverip == NULL) {
        return 1;
    }
    if (username == NULL) {
        return 2;
    }
    if (userpwd == NULL) {
        return 3;
    }
    if (database == NULL) {
        return 4;
    }

    mysql_init(&mysql);
    if (mysql_options(&mysql, MYSQL_SET_CHARSET_NAME, "utf8")) {//gb2312
        printdblog("Couldn't SET_CHARSET_NAME!\n%s\n\n", mysql_error(&mysql));
    }
    printdblog("connect mysql server:%s,port:%d,name:%s,password:%s,db:%s\n", serverip, port, username, "**", database);
    if (!(mysql_sock = mysql_real_connect(&mysql, serverip, username, userpwd, database, port, NULL, 0))) {
        printdblog("Couldn't connect to engine!\n%s\n\n", mysql_error(&mysql));
        return 5;
    }
    printdblog("connect mysql success\n");
    _isconnected = 1;
    printdblog("set _isconnected:%d\n", _isconnected);
    return 0;
}

int ConnectMysql() {
    if (_isconnected == 1) {
        return 0;
    }
    if (GetMysqlConfig() != 0) {
        return 1;
    }
    if(strcmp(dbtype,"MYSQL") != 0){
	    return ConnectODBC();
    }
    return ConnectDB(serverip, port, username, userpwd, database);
}

int GetMysqlConfig() {
    FILE *fp;

    memset(serverip, 0, sizeof (serverip));
    memset(username, 0, sizeof (username));
    memset(userpwd, 0, sizeof (userpwd));
    memset(database, 0, sizeof (database));

    if ((fp = fopen(MysqlConfigPath, "r")) != NULL) {
        while (!feof(fp)) {
            char logcommand[255] = {0};
            memset(logcommand, 0, sizeof (logcommand));
            if (fgets(logcommand, 255, fp) == NULL) {
                break;
            } else {
                int i = strlen(logcommand);
                if (logcommand[i - 1] == '\n' || logcommand[i - 1] == '\r') {
                    logcommand[i - 1] = 0;
                }
                if (logcommand[i - 2] == '\r') {
                    logcommand[i - 2] = 0;
                }
                if (strstr(logcommand, DBCFG_SERVERIP) != NULL) {
                    strcpy(serverip, logcommand + strlen(DBCFG_SERVERIP));
                } else if (strstr(logcommand, DBCFG_PORT) != NULL) {
                    char szport[10];
                    memset(szport, 0, sizeof (szport));
                    strcpy(szport, logcommand + strlen(DBCFG_PORT));
                    port = atoi(szport);
                } else if (strstr(logcommand, DBCFG_USERNAME) != NULL) {
                    strcpy(username, logcommand + strlen(DBCFG_USERNAME));
                } else if (strstr(logcommand, DBCFG_USERPWD) != NULL) {
                    strcpy(userpwd, logcommand + strlen(DBCFG_USERPWD));
                    printdblog("encode before password:%s\n", userpwd);
                        if(strlen(userpwd) >= 20)
                        {
                            char _userpwd[264]={0};
                            _encrypts(userpwd,strlen(userpwd),_userpwd);
                            memset(userpwd,0,sizeof(userpwd));
                            memcpy(userpwd,_userpwd,strlen(_userpwd));
                        }
                    printdblog("password:%s\n",userpwd);
                } else if (strstr(logcommand, DBCFG_DATABASE) != NULL) {
                    strcpy(database, logcommand + strlen(DBCFG_DATABASE));
                } else if (strstr(logcommand, DBCFG_DBTYPE) != NULL) {
                    strcpy(dbtype, logcommand + strlen(DBCFG_DBTYPE));
                }
            }
        }
        fclose(fp);
    } else {
        printdblog("cannot open mysql.cfg\n");

        return 1;
    }
    return 0;
}

void CloseDB() {
    if(strcmp(dbtype,"MYSQL") != 0){
	    CloseODBC();
	    return;
    }
    if (mysql_sock != NULL) {
        mysql_close(mysql_sock);
        _isconnected = 0;
        mysql_sock = NULL;
    }
}

int Audit_ConnectDB(char * serverip, int port, char * username, char * userpwd, char * database) {
    //printdblog("check _isconnected:%d\n", _audit_isconnected);
    if (_audit_isconnected == 1) {
        printdblog("_audit_isconnected is already 1!\n");
        return 0;
    }
    if (audit_mysql_sock != NULL) {
        printdblog("Audit_ConnectDB audit_mysql_sock is not NULL!\n");
        return 0;
    }
    if (audit_serverip == NULL) {
        return 1;
    }
    if (audit_username == NULL) {
        return 2;
    }
    if (audit_userpwd == NULL) {
        return 3;
    }
    if (audit_database == NULL) {
        return 4;
    }

    mysql_init(&audit_mysql);
    if (mysql_options(&audit_mysql, MYSQL_SET_CHARSET_NAME, "utf8")) {//gb2312
        printdblog("Couldn't SET_CHARSET_NAME!\n%s\n\n", mysql_error(&audit_mysql));
    }
    if (!(audit_mysql_sock = mysql_real_connect(&audit_mysql, audit_serverip, audit_username, audit_userpwd, audit_database, audit_port, NULL, 0))) {
        printdblog("connect audit_mysql server:%s,port:%d,name:%s,password:%s,db:%s\n", audit_serverip, audit_port, audit_username, audit_userpwd, audit_database);
        printdblog("Couldn't connect to engine!\n%s\n\n", mysql_error(&audit_mysql));
        return 5;
    }
    else
    {
        printdblog("connect audit_mysql server:%s,port:%d,name:%s,password:%s,db:%s success\n", audit_serverip, audit_port, audit_username, audit_userpwd, audit_database);
    }
   // printdblog("connect mysql success\n");
    _audit_isconnected = 1;
    //printdblog("set _isconnected:%d\n", _audit_isconnected);
    return 0;
}

int Audit_ConnectMysql() {
    if (_audit_isconnected == 1) {
        return 0;
    }
    if (Audit_GetMysqlConfig() != 0) {
        return 1;
    }
    if(strcmp(audit_dbtype,"MYSQL") != 0){
	    return ConnectODBCAudit();
    }
    return Audit_ConnectDB(audit_serverip, audit_port, audit_username, audit_userpwd, audit_database);
}

int Audit_GetMysqlConfig() {
    FILE *fp;

    memset(audit_serverip, 0, sizeof (audit_serverip));
    memset(audit_username, 0, sizeof (audit_username));
    memset(audit_userpwd, 0, sizeof (audit_userpwd));
    memset(audit_database, 0, sizeof (audit_database));
    if ((fp = fopen(MysqlAuditConfigPath, "r")) != NULL) {
        while (!feof(fp)) {
            char logcommand[255] = {0};
            memset(logcommand, 0, sizeof (logcommand));
            if (fgets(logcommand, 255, fp) == NULL) {
                break;
            } else {
                int i = strlen(logcommand);
                if (logcommand[i - 1] == '\n' || logcommand[i - 1] == '\r') {
                    logcommand[i - 1] = 0;
                }
                if (logcommand[i - 2] == '\r') {
                    logcommand[i - 2] = 0;
                }
                if (strstr(logcommand, DBCFG_SERVERIP) != NULL) {                
                    strcpy(audit_serverip, logcommand + strlen(DBCFG_SERVERIP));
                } else if (strstr(logcommand, DBCFG_PORT) != NULL) {
                    char szport[10];
                    memset(szport, 0, sizeof (szport));
                    strcpy(szport, logcommand + strlen(DBCFG_PORT));
                    audit_port = atoi(szport);
                } else if (strstr(logcommand, DBCFG_USERNAME) != NULL) {
                    strcpy(audit_username, logcommand + strlen(DBCFG_USERNAME));
                } else if (strstr(logcommand, DBCFG_USERPWD) != NULL) {
                    strcpy(audit_userpwd, logcommand + strlen(DBCFG_USERPWD));
                    if(strlen(audit_userpwd) >= 20)
                    {
                         char _audit_userpwd[256]={0};
                        _encrypts(audit_userpwd,strlen(audit_userpwd),_audit_userpwd);
                        memset(audit_userpwd,0,sizeof(audit_userpwd));
                        strcpy(audit_userpwd,_audit_userpwd);
                    }
                } else if (strstr(logcommand, DBCFG_DATABASE) != NULL) {
                    strcpy(audit_database, logcommand + strlen(DBCFG_DATABASE));
                } else if (strstr(logcommand, DBCFG_DBTYPE) != NULL) {
                    strcpy(audit_dbtype, logcommand + strlen(DBCFG_DBTYPE));
                }
            }
        }
        fclose(fp);
    } else if((fp = fopen(MysqlConfigPath, "r")) != NULL) {
        while (!feof(fp)) {
            char logcommand[255] = {0};
            memset(logcommand, 0, sizeof (logcommand));
            if (fgets(logcommand, 255, fp) == NULL) {
                break;
            } else {
                int i = strlen(logcommand);
                if (logcommand[i - 1] == '\n' || logcommand[i - 1] == '\r') {
                    logcommand[i - 1] = 0;
                }
                if (logcommand[i - 2] == '\r') {
                    logcommand[i - 2] = 0;
                }
                if (strstr(logcommand, DBCFG_SERVERIP) != NULL) {
                    strcpy(audit_serverip, logcommand + strlen(DBCFG_SERVERIP));
                } else if (strstr(logcommand, DBCFG_PORT) != NULL) {
                    char szport[10];
                    memset(szport, 0, sizeof (szport));
                    strcpy(szport, logcommand + strlen(DBCFG_PORT));
                    audit_port = atoi(szport);
                } else if (strstr(logcommand, DBCFG_USERNAME) != NULL) {
                    strcpy(audit_username, logcommand + strlen(DBCFG_USERNAME));
                } else if (strstr(logcommand, DBCFG_USERPWD) != NULL) {
                    strcpy(audit_userpwd, logcommand + strlen(DBCFG_USERPWD));
                    if(strlen(audit_userpwd) >= 20)
                    {
                        char _audit_userpwd[256]={0};
                        _encrypts(audit_userpwd,strlen(audit_userpwd),_audit_userpwd);
                        memset(audit_userpwd,0,sizeof(audit_userpwd));
                        memcpy(audit_userpwd,_audit_userpwd,strlen(_audit_userpwd));
                    }
                } else if (strstr(logcommand, DBCFG_DATABASE) != NULL) {
                    strcpy(audit_database, logcommand + strlen(DBCFG_DATABASE));
                } else if (strstr(logcommand, DBCFG_DBTYPE) != NULL) {
                    strcpy(audit_dbtype, logcommand + strlen(DBCFG_DBTYPE));
                }
            }
        }
        fclose(fp); 
    }else{
        printdblog("cannot open mysql.cfg\n");

        return 1;
    }
    return 0;
}

void Audit_CloseDB() {
    if(strcmp(audit_dbtype,"MYSQL") != 0){
	    CloseODBCAudit();
	    return;
    }
    if (audit_mysql_sock != NULL) {
        mysql_close(audit_mysql_sock);
        _audit_isconnected = 0;
        audit_mysql_sock = NULL;
    }
}

int GetAuth_id(char* pr_id, PPr_AuthAccessRes pAuth) {
    printdblog("DBRecord-DBoperate.c:[GetAuth_id] enter function!\n");
    if(strcmp(dbtype,"MYSQL") != 0){
        printdblog("DBRecord-DBoperate.c:[GetAuth_id] dbtype:%s\n", dbtype);
	return OGetAuth_id(pr_id,pAuth);
    }

    char sql[1024];
    char real_pwd[1024];
    MYSQL_RES *res;
    MYSQL_ROW row;
    int num;
    int rtn = 0;

    if (pAuth == NULL) {
        printdblog("DBRecord-DBoperate.c:[GetAuth_id] pAuth is NULL!\n");
        return 1;
    }

    memset(real_pwd, 0, sizeof (real_pwd));
    memset(sql, 0, sizeof (sql));
    sprintf(sql, "select * from pr_authaccessres where pr_id='%s'\n", pr_id);
    printdblog(sql);
    printdblog("\n");

    if (mysql_query(mysql_sock, sql)) {
        printdblog("DBRecord-DBoperate.c:[GetAuth_id] mysql_query failed!\n ");
        return 2;
    }
    if (!(res = mysql_store_result(mysql_sock))) {
        printdblog("DBRecord-DBoperate.c:[GetAuth_id] mysql_store_result failed!\n");
        return 3;
    }
    num = mysql_num_rows(res);
    if (0 < num && (row = mysql_fetch_row(res))) {
        strcpy(pAuth->pr_id, row[0]);
        strcpy(pAuth->pr_userId, row[1]);
        strcpy(pAuth->pr_uid, row[2]);
        strcpy(pAuth->pr_userName, row[3]);
        strcpy(pAuth->pr_resId, row[4]);
        strcpy(pAuth->pr_resAddr, row[5]);
        strcpy(pAuth->pr_resName, row[6]);
        strcpy(pAuth->pr_resType, row[7]);
        if (row[8] != NULL)
            strcpy(pAuth->pr_resVersion, row[8]);
        strcpy(pAuth->pr_accountId, row[9]);
        strcpy(pAuth->pr_accountNo, row[10]);
        pAuth->pr_accountType = atoi(row[11]);
        /*_encrypt*/DecryptData(row[12], strlen(row[12]), real_pwd);

        strcpy(pAuth->pr_accountPwd, real_pwd);
        pAuth->pr_suFlag = atoi(row[13]);
        pAuth->pr_ssoFlag = atoi(row[14]);
        pAuth->pr_auditFlag = atoi(row[15]);
        pAuth->pr_auditType = atoi(row[16]);
        pAuth->pr_cmdFlag = atoi(row[17]);
        if (row[18] != NULL)
            strcpy(pAuth->pr_clientAddr, row[18]);
        strcpy(pAuth->pr_apId, row[19]);
        pAuth->pr_accessType = atoi(row[20]);
        if (row[21] != NULL)
            strcpy(pAuth->pr_accountNoEx, row[21]);
        if (row[22] != NULL)
            strcpy(pAuth->pr_accountPwdEx, row[22]);
        if (row[23] != NULL)
            strcpy(pAuth->pr_domainName, row[23]);
        if (row[24] != NULL)
            pAuth->pr_screenWidth = atoi(row[24]);
        if (row[25] != NULL)
            pAuth->pr_screenHeight = atoi(row[25]);
        strcpy(pAuth->pr_proxyAddr, row[26]);
        if (row[27] != NULL)
            strcpy(pAuth->pr_startTime, row[27]);
        if (row[28] != NULL)
            strcpy(pAuth->pr_endTime, row[28]);
        if (row[29] != NULL)
            strcpy(pAuth->pr_resetPwdTime, row[29]);
        pAuth->pr_timeFlag = atoi(row[30]);
        strcpy(pAuth->pr_execApp, row[31]);
        if (row[32] != NULL)
            strcpy(pAuth->pr_execParam, row[32]);
        if (row[33] != NULL)
            strcpy(pAuth->pr_operDate, row[33]);
        if (row[34] != NULL)
            strcpy(pAuth->pr_proxyHost, row[34]);
        if (row[35] != NULL)
            strcpy(pAuth->pr_proxyPort, row[35]);
        if (row[36] != NULL)
            strcpy(pAuth->pr_cooperator, row[36]);
        if (row[37] != NULL)
            strcpy(pAuth->pr_accessMemo, row[37]);


    } else {
        printdblog("DBRecord-DBoperate.c[GetAuth_id] sql:[%s] the result num is %d\n", sql, num);
        rtn = 4;
    }
    mysql_free_result(res);
    return rtn;

}

int GetAuth_uid(char* pr_uid, char* pr_resAddr, char* pr_accountNo, PPr_AuthAccessRes pAuth, int pr_code) {
    if(strcmp(dbtype,"MYSQL") != 0){
	    return OGetAuth_uid(pr_uid,pr_resAddr,pr_accountNo,pAuth,pr_code);
    }
    char sql[1024];
    char real_pwd[1024];
    MYSQL_RES *res;
    MYSQL_ROW row;
    int num;
    int rtn = 0;

    if (pAuth == NULL) {
        return 1;
    }

    memset(real_pwd,0,sizeof(real_pwd));
    memset(sql, 0, sizeof (sql));
    if(pr_code == 502){
        sprintf(sql, "select * from pr_authaccessres where pr_uid='%s' and pr_resAddr='%s' and pr_accountNo='%s' and (pr_accessType=%d or pr_accessType=201 or pr_accessType=202)", pr_uid, pr_resAddr, pr_accountNo, pr_code);
    }else{
        sprintf(sql, "select * from pr_authaccessres where pr_uid='%s' and pr_resAddr='%s' and pr_accountNo='%s' and pr_accessType=%d", pr_uid, pr_resAddr, pr_accountNo, pr_code);
    }
    printdblog(sql);
    printdblog("\n");

    if (mysql_query(mysql_sock, sql)) {
        return 2;
    }
    if (!(res = mysql_store_result(mysql_sock))) {
        return 3;
    }
    num = mysql_num_rows(res);
    if (0 < num && (row = mysql_fetch_row(res))) {
        strcpy(pAuth->pr_id, row[0]);
        strcpy(pAuth->pr_userId, row[1]);
        strcpy(pAuth->pr_uid, row[2]);
        strcpy(pAuth->pr_userName, row[3]);
        strcpy(pAuth->pr_resId, row[4]);
        strcpy(pAuth->pr_resAddr, row[5]);
        strcpy(pAuth->pr_resName, row[6]);
        strcpy(pAuth->pr_resType, row[7]);
        if (row[8] != NULL)
            strcpy(pAuth->pr_resVersion, row[8]);
        strcpy(pAuth->pr_accountId, row[9]);
        strcpy(pAuth->pr_accountNo, row[10]);
        pAuth->pr_accountType = atoi(row[11]);
        /*_encrypt*/DecryptData(row[12], strlen(row[12]), real_pwd);
        strcpy(pAuth->pr_accountPwd, real_pwd);
        pAuth->pr_suFlag = atoi(row[13]);
        pAuth->pr_ssoFlag = atoi(row[14]);
        pAuth->pr_auditFlag = atoi(row[15]);
        pAuth->pr_auditType = atoi(row[16]);
        pAuth->pr_cmdFlag = atoi(row[17]);
        if (row[18] != NULL)
            strcpy(pAuth->pr_clientAddr, row[18]);
        strcpy(pAuth->pr_apId, row[19]);
        pAuth->pr_accessType = atoi(row[20]);
        if (row[21] != NULL)
            strcpy(pAuth->pr_accountNoEx, row[21]);
        if (row[22] != NULL)
            strcpy(pAuth->pr_accountPwdEx, row[22]);
        if (row[23] != NULL)
            strcpy(pAuth->pr_domainName, row[23]);
        if (row[24] != NULL)
            pAuth->pr_screenWidth = atoi(row[24]);
        if (row[25] != NULL)
            pAuth->pr_screenHeight = atoi(row[25]);
        strcpy(pAuth->pr_proxyAddr, row[26]);
        if (row[27] != NULL)
            strcpy(pAuth->pr_startTime, row[27]);
        if (row[28] != NULL)
            strcpy(pAuth->pr_endTime, row[28]);
        if (row[29] != NULL)
            strcpy(pAuth->pr_resetPwdTime, row[29]);
        pAuth->pr_timeFlag = atoi(row[30]);
        strcpy(pAuth->pr_execApp, row[31]);
        if (row[32] != NULL)
            strcpy(pAuth->pr_execParam, row[32]);
        if (row[33] != NULL)
            strcpy(pAuth->pr_operDate, row[33]);
        if (row[34] != NULL)
            strcpy(pAuth->pr_proxyHost, row[34]);
        if (row[35] != NULL)
            strcpy(pAuth->pr_proxyPort, row[35]);
        if (row[36] != NULL)
            strcpy(pAuth->pr_cooperator, row[36]);
        if (row[37] != NULL)
            strcpy(pAuth->pr_accessMemo, row[37]);


    } else {
        rtn = 4;
    }
    mysql_free_result(res);
    return rtn;
}

int SetAuth(PPr_AuthAccessRes pAuth) {
    if(strcmp(dbtype,"MYSQL") != 0){
	    return OSetAuth(pAuth);
    }
    char sql[1024];
    MYSQL_RES *res;
    MYSQL_ROW row;
    int num;
    int rtn = 0;

    return 0;
    long ts;
    struct tm *ptm;
    int y, m, d, h, n, s;
    char timenow[1024];
    ts = time(NULL);
    ptm = localtime(&ts);

    y = ptm->tm_year + 1900; //�?
    m = ptm->tm_mon + 1; //�?
    d = ptm->tm_mday; //�?
    h = ptm->tm_hour; //�?
    n = ptm->tm_min; //�?
    s = ptm->tm_sec; //�?
    //time
    memset(timenow, 0, sizeof (timenow));
    sprintf(pAuth->pr_id, "%02d%02d%02d%d%d", h, n, s, rand(), rand());
    sprintf(pAuth->pr_startTime, "%04d-%02d-%02d %02d:%02d:%02d", y, m, d, h, n, s);
    sprintf(pAuth->pr_operDate, "%04d-%02d-%02d %02d:%02d:%02d", y, m, d, h, n, s);

    memset(sql, 0, sizeof (sql));
    sprintf(sql, "INSERT INTO pr_authaccessres VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%d', '%s', '%d', '%d', '%d', '%d', '%d', '%s', '%s', '%d', '%s', '%s', '%s', '%d', '%d', '%s', '%s', null, null, '%d', '%s', '%s', '%s',1);",
            pAuth->pr_id,
            pAuth->pr_userId,
            pAuth->pr_uid,
            pAuth->pr_userName,
            pAuth->pr_resId,
            pAuth->pr_resAddr,
            pAuth->pr_resName,
            pAuth->pr_resType,
            pAuth->pr_resVersion,
            pAuth->pr_accountId,
            pAuth->pr_accountNo,
            pAuth->pr_accountType,
            pAuth->pr_accountPwd,
            pAuth->pr_suFlag,
            pAuth->pr_ssoFlag,
            pAuth->pr_auditFlag,
            pAuth->pr_auditType,
            pAuth->pr_cmdFlag,
            pAuth->pr_clientAddr,
            pAuth->pr_apId,
            pAuth->pr_accessType,
            pAuth->pr_accountNoEx,
            pAuth->pr_accountPwdEx,
            pAuth->pr_domainName,
            pAuth->pr_screenWidth,
            pAuth->pr_screenHeight,
            pAuth->pr_proxyAddr,
            pAuth->pr_startTime,
            //pAuth->pr_endTime,
            //pAuth->pr_resetPwdTime,
            pAuth->pr_timeFlag,
            pAuth->pr_execApp,
            pAuth->pr_execParam,
            pAuth->pr_operDate
            );
    printdblog("LogTrans:%s\n", sql);
    //SaveDB2File(sql);
    if (mysql_query(mysql_sock, sql)) {
        return 1;
    }
    return 0;
}

int CheckEmployee(char * username, char * password) {
    if(strcmp(dbtype,"MYSQL") != 0){
	    return OCheckEmployee(username,password);
    }
    char sql[1024];
    char name[1024];
    char real_pwd[1024];
    char seps[] = "@#";
    char *token;
    MYSQL_RES *res;
    MYSQL_ROW row;
    int num;
    int rtn = 0;
    
    
    memset(real_pwd, 0, sizeof (real_pwd));
    memset(sql, 0, sizeof (sql));
    memset(name,0,sizeof(name));
    
     token = strtok(username, seps);
    if (token != NULL) {
            strcpy(name, token);
    }
    
    sprintf(sql, "select pr_pwd from pr_employee where pr_uid='%s'", name);
    printdblog(sql);
    printdblog("\n");

    if (mysql_query(mysql_sock, sql)) {
        printdblog("CheckEmployee db error\n");
        return 2;
    }
    if (!(res = mysql_store_result(mysql_sock))) {
        printdblog("CheckEmployee sql error\n");
        return 3;
    }
    num = mysql_num_rows(res);
    if (0 < num && (row = mysql_fetch_row(res))) {
        /*_encrypt*/DecryptData(row[0], strlen(row[0]), real_pwd);
        if(strstr(real_pwd,password) != NULL && strlen(real_pwd) == strlen(password)){
            printdblog("CheckEmployee password ok\n");
            rtn = 0;
        }else{
            printdblog("CheckEmployee password error\n");
            rtn = 1;
        }
    } else {
        printdblog("CheckEmployee username error\n");
        rtn = 4;
    }
    mysql_free_result(res);
    return rtn;
}

int GetEmployee(char * pr_id, PPr_Employee pEmployee) {
    if(strcmp(dbtype,"MYSQL") != 0){
	    return OGetEmployee(pr_id,pEmployee);
    }
    char sql[1024];
    char real_pwd[1024];
    MYSQL_RES *res;
    MYSQL_ROW row;
    int num;
    int rtn = 0;

    if (pEmployee == NULL) {
        return 1;
    }

    memset(real_pwd, 0, sizeof (real_pwd));
    memset(sql, 0, sizeof (sql));
    sprintf(sql, "select * from pr_employee where pr_id='%s'", pr_id);
    printdblog(sql);
    printdblog("\n");

    if (mysql_query(mysql_sock, sql)) {
        printdblog("GetEmployee 2\n");
        return 2;
    }
    if (!(res = mysql_store_result(mysql_sock))) {
        printdblog("GetEmployee 3\n");
        return 3;
    }
    num = mysql_num_rows(res);
    if (0 < num && (row = mysql_fetch_row(res))) {
        strcpy(pEmployee->pr_id, row[0]);
        strcpy(pEmployee->pr_userName, row[1]);
        if (row[2] != NULL)
            strcpy(pEmployee->pr_pinyin, row[2]);
        pEmployee->pr_userType = atoi(row[3]);
        strcpy(pEmployee->pr_uid, row[4]);
        /*_encrypt*/DecryptData(row[5], strlen(row[5]), real_pwd);
        strcpy(pEmployee->pr_pwd, real_pwd);
        if (row[6] != NULL)
            strcpy(pEmployee->pr_orgId, row[6]);
        if (row[7] != NULL)
            strcpy(pEmployee->pr_orgName, row[7]);
        pEmployee->pr_sex = atoi(row[8]);
        if (row[9] != NULL)
            strcpy(pEmployee->pr_email, row[9]);
        if (row[10] != NULL)
            strcpy(pEmployee->pr_phone, row[10]);
        if (row[11] != NULL)
            strcpy(pEmployee->pr_relUID, row[11]);
        if (row[12] != NULL)
            strcpy(pEmployee->pr_relName, row[12]);
        if (row[13] != NULL)
            strcpy(pEmployee->pr_desc, row[13]);
        pEmployee->pr_status = atoi(row[14]);
        if (row[15] != NULL)
                strcpy(pEmployee->pr_createTime, row[15]);
        if (row[16] != NULL)
            strcpy(pEmployee->pr_modifyTime, row[16]);
        pEmployee->pr_authType = atoi(row[17]);
        if (row[18] != NULL)
            strcpy(pEmployee->pr_loginNextTime, row[18]);
        if (row[19] != NULL)
            pEmployee->pr_loginFailCount = atoi(row[19]);


    } else {
        printdblog("GetEmployee 4\n");
        rtn = 4;
    }
    printdblog("GetEmployee 0\n");
    mysql_free_result(res);
    return rtn;
}

int GetResPwd(char * res_id, char * accountNo, char * pwd) {
    if(strcmp(dbtype,"MYSQL") != 0){
	    return OGetResPwd(res_id,accountNo,pwd);
    }

    char sql[1024];
    char real_pwd[1024];
    MYSQL_RES *res;
    MYSQL_ROW row;
    int num;
    int rtn = 0;


    memset(real_pwd,0,sizeof(real_pwd));
    memset(sql, 0, sizeof (sql));
    sprintf(sql, "SELECT pr_accountPwd FROM pr_resaccount where pr_resId='%s' and pr_accountNo = '%s'", res_id, accountNo);
    printdblog(sql);
    printdblog("\n");

    if (mysql_query(mysql_sock, sql)) {
        return 2;
    }
    if (!(res = mysql_store_result(mysql_sock))) {
        return 3;
    }
    num = mysql_num_rows(res);
    if (0 < num && (row = mysql_fetch_row(res))) {
        /*_encrypt*/DecryptData(row[0], strlen(row[0]), real_pwd);
        strcpy(pwd, real_pwd);
    } else {
        rtn = 4;
    }
    mysql_free_result(res);
    return rtn;
}
int GetResourcePort(char * pr_resId,int pr_code,int *port){
    if(strcmp(dbtype,"MYSQL") != 0){
	    return OGetResourcePort(pr_resId,pr_code,port);
    }

    char sql[1024];
    char real_pwd[1024];
    MYSQL_RES *res;
    MYSQL_ROW row;
    int num;
    int rtn = 0;


    memset(real_pwd,0,sizeof(real_pwd));
    memset(sql, 0, sizeof (sql));
    if(pr_code == 502){
       sprintf(sql, "SELECT pr_resprotocol.pr_port from pr_resprotocol,pr_accessprotocol where pr_resprotocol.pr_resId='%s' and pr_resprotocol.pr_apId=pr_accessprotocol.pr_id and (pr_accessprotocol.pr_code=%d or pr_accessprotocol.pr_code=201 or pr_accessprotocol.pr_code=202)", 
                pr_resId, pr_code);        
    }else{
        sprintf(sql, "SELECT pr_resprotocol.pr_port from pr_resprotocol,pr_accessprotocol where pr_resprotocol.pr_resId='%s' and pr_resprotocol.pr_apId=pr_accessprotocol.pr_id and pr_accessprotocol.pr_code=%d", 
                pr_resId, pr_code);
    }
    printdblog(sql);
    printdblog("\n");

    if (mysql_query(mysql_sock, sql)) {
        return 2;
    }
    if (!(res = mysql_store_result(mysql_sock))) {
        return 3;
    }
    num = mysql_num_rows(res);
    if (0 < num && (row = mysql_fetch_row(res))) {
        *port = atoi(row[0]);
    } else {
        rtn = 4;
    }
    mysql_free_result(res);
    return rtn;
}
int GetResource(char * pr_id, char * accountNo, PPr_Resource pResource) {
    if(strcmp(dbtype,"MYSQL") != 0){
	    return OGetResource(pr_id,accountNo,pResource);
    }
    char sql[1024];
    MYSQL_RES *res;
    MYSQL_ROW row;
    int num;
    int rtn = 0;

    if (pResource == NULL) {
        return 1;
    }

    memset(sql, 0, sizeof (sql));
    sprintf(sql, "select * from pr_resource where pr_id='%s'", pr_id);
    printdblog(sql);
    printdblog("\n");

    if (mysql_query(mysql_sock, sql)) {
        return 2;
    }
    if (!(res = mysql_store_result(mysql_sock))) {
        return 3;
    }
    num = mysql_num_rows(res);
    if (0 < num && (row = mysql_fetch_row(res))) {
        if(row[0]!=NULL)strcpy(pResource->pr_id,row[0]);
        if(row[1]!=NULL)strcpy(pResource->pr_rtId,row[1]); 
        if(row[2]!=NULL)strcpy(pResource->pr_name,row[2]); 
        if(row[3]!=NULL)strcpy(pResource->pr_address,row[3]); 
        if(row[4]!=NULL)strcpy(pResource->pr_userName,row[4]); 
        //if(row[5]!=NULL)strcpy(pResource->pr_passwd,row[5]); 
        if(row[6]!=NULL)strcpy(pResource->pr_port,row[6]); 
        if(row[7]!=NULL)pResource->pr_status=atoi(row[7]);
        if(row[8]!=NULL)pResource->pr_order=atoi(row[8]);
        if(row[9]!=NULL)pResource->pr_flag=atoi(row[9]);
        if(row[10]!=NULL)pResource->pr_ssoFlag=atoi(row[10]);
        if(row[11]!=NULL)pResource->pr_auditFlag=atoi(row[11]);
        if(row[12]!=NULL)pResource->pr_auditType=atoi(row[12]);
        if(row[13]!=NULL)pResource->pr_accessType=atoi(row[13]);
        if(row[14]!=NULL)strcpy(pResource->pr_desc,row[14]); 
        if(row[15]!=NULL)strcpy(pResource->pr_version,row[15]); 
        if(row[16]!=NULL)strcpy(pResource->pr_domainName,row[16]); 
        if(row[17]!=NULL)strcpy(pResource->pr_baseDN,row[17]); 
        if(row[18]!=NULL)pResource->pr_useSuDo=atoi(row[18]);
        if(row[19]!=NULL)strcpy(pResource->pr_hostName,row[19]); 
        if(row[20]!=NULL)strcpy(pResource->pr_relationId,row[20]); 


    } else {
        rtn = 4;
    }
    mysql_free_result(res);

    if (rtn == 0) {
        return GetResPwd(pResource->pr_id, accountNo, pResource->pr_passwd);
    }
    return rtn;
}
int GetResaccountCount(char * pr_resId,char * pr_userId,int * length){
    if(strcmp(dbtype,"MYSQL") != 0){
	    return OGetResaccountCount(pr_resId,pr_userId,length);
    }
    char sql[1024];
    MYSQL_RES *res;
    MYSQL_ROW row;
    int num;
    int rtn = 0;
    int i = 0;

    memset(sql, 0, sizeof (sql));
    sprintf(sql, "select count(*) from pr_resaccount where pr_resId='%s' and pr_userId='%s'", pr_resId,pr_userId);
    printdblog(sql);
    printdblog("\n");

    if (mysql_query(mysql_sock, sql)) {
        return 2;
    }
    if (!(res = mysql_store_result(mysql_sock))) {
        return 3;
    }
    num = mysql_num_rows(res);
    if (0 < num && (row = mysql_fetch_row(res))) {
        *length = atoi(row[0]);
    } else {
        rtn = 4;
    }
    mysql_free_result(res);
    return rtn;
}
int GetResaccount(char * pr_resId,char * pr_userId,PPr_Resaccount pResaccount){
    if(strcmp(dbtype,"MYSQL") != 0){
	    return OGetResaccount(pr_resId,pr_userId,pResaccount);
    }
    char sql[1024];
    MYSQL_RES *res;
    MYSQL_ROW row;
    int num;
    int rtn = 0;
    int i = 0;

    if (pResaccount == NULL) {
        return 1;
    }

    memset(sql, 0, sizeof (sql));
    sprintf(sql, "select pr_accountNo,pr_accountPwd from pr_resaccount where pr_resId='%s' and pr_userId='%s'", pr_resId,pr_userId);
    printdblog(sql);
    printdblog("\n");

    if (mysql_query(mysql_sock, sql)) {
        return 2;
    }
    if (!(res = mysql_store_result(mysql_sock))) {
        return 3;
    }
    num = mysql_num_rows(res);
    while (0 < num && (row = mysql_fetch_row(res))) {
        strcpy(pResaccount[i].pr_accountNo, row[0]);
        strcpy(pResaccount[i].pr_accountPwd, row[1]);
        
        i++;
    }
    mysql_free_result(res);
    return rtn;
}
int GetProtocolProt(char * pr_code,int * pr_port){
    if(strcmp(dbtype,"MYSQL") != 0){
	    return OGetProtocolProt(pr_code,pr_port);
    }
    char sql[1024];
    MYSQL_RES *res;
    MYSQL_ROW row;
    int num;
    int rtn = 0;
    int i = 0;

    memset(sql, 0, sizeof (sql));
    sprintf(sql, "select pr_port from pr_accessprotocol where pr_code='%s'", pr_code);
    printdblog(sql);
    printdblog("\n");

    if (mysql_query(mysql_sock, sql)) {
        return 2;
    }
    if (!(res = mysql_store_result(mysql_sock))) {
        return 3;
    }
    num = mysql_num_rows(res);
    if (0 < num && (row = mysql_fetch_row(res))) {
        *pr_port = atoi(row[0]);
    } else {
        rtn = 4;
    }
    mysql_free_result(res);
    return rtn;
}
int GetEmployeeaccountCount(char * pr_uid,char * pr_rgId,int * length){
    if(strcmp(dbtype,"MYSQL") != 0){
	    return OGetEmployeeaccountCount(pr_uid,pr_rgId,length);
    }
    char sql[1024];
    MYSQL_RES *res;
    MYSQL_ROW row;
    int num;
    int rtn = 0;
    int i = 0;

    memset(sql, 0, sizeof (sql));
    sprintf(sql, "select count(*) from pr_employeeaccount where pr_uid='%s'", pr_uid);
    printdblog(sql);
    printdblog("\n");

    if (mysql_query(mysql_sock, sql)) {
        return 2;
    }
    if (!(res = mysql_store_result(mysql_sock))) {
        return 3;
    }
    num = mysql_num_rows(res);
    if (0 < num && (row = mysql_fetch_row(res))) {
        *length = atoi(row[0]);
    } else {
        rtn = 4;
    }
    mysql_free_result(res);
    return rtn;
}
int GetEmployeeaccounts(char * pr_uid,char * pr_rgId,PPr_Employeeaccount pEmployeeaccount,int offset){
    if(strcmp(dbtype,"MYSQL") != 0){
	    return OGetEmployeeaccounts(pr_uid,pr_rgId,pEmployeeaccount,offset);
    }
    char sql[1024];
    MYSQL_RES *res;
    MYSQL_ROW row;
    int num;
    int rtn = 0;
    int i = 0;

    if (pEmployeeaccount == NULL) {
        return 1;
    }

    memset(sql, 0, sizeof (sql));
    sprintf(sql, "select * from pr_employeeaccount where pr_uid='%s'", pr_uid);
    printdblog(sql);
    printdblog("\n");

    if (mysql_query(mysql_sock, sql)) {
        return 2;
    }
    if (!(res = mysql_store_result(mysql_sock))) {
        return 3;
    }
    num = mysql_num_rows(res);
    while (0 < num && (row = mysql_fetch_row(res))) {
        strcpy(pEmployeeaccount[i].pr_id, row[0]);
        strcpy(pEmployeeaccount[i].pr_userId, row[1]);
        strcpy(pEmployeeaccount[i].pr_uid, row[2]);
        strcpy(pEmployeeaccount[i].pr_userName, row[3]);
        strcpy(pEmployeeaccount[i].pr_resId, row[4]);
        strcpy(pEmployeeaccount[i].pr_resAddr, row[5]);
        strcpy(pEmployeeaccount[i].pr_resName, row[6]);
        strcpy(pEmployeeaccount[i].pr_resType, row[7]);
        strcpy(pEmployeeaccount[i].pr_accountId, row[8]);
        strcpy(pEmployeeaccount[i].pr_accountNo, row[9]);
        strcpy(pEmployeeaccount[i].pr_accountPwd, row[10]);
        pEmployeeaccount[i].pr_requestFlag = atoi(row[11]);
        if (row[12] != NULL)
            strcpy(pEmployeeaccount[i].pr_groupId, row[12]);
        pEmployeeaccount[i].pr_flag = atoi(row[13]);
        strcpy(pEmployeeaccount[i].pr_startTime, row[14]);
        if (row[15] != NULL)
            strcpy(pEmployeeaccount[i].pr_endTime, row[15]);
        if (row[16] != NULL)
            strcpy(pEmployeeaccount[i].pr_delayTime, row[16]);
        if (row[17] != NULL)
            strcpy(pEmployeeaccount[i].pr_approverId, row[17]);
        if (row[18] != NULL)
            strcpy(pEmployeeaccount[i].pr_approverName, row[18]);
        if (row[19] != NULL)
            strcpy(pEmployeeaccount[i].pr_operDate, row[19]);
        if (row[20] != NULL)
            strcpy(pEmployeeaccount[i].pr_desc, row[20]);
        pEmployeeaccount[i].pr_status = atoi(row[21]);
        if (row[22] != NULL)
            strcpy(pEmployeeaccount[i].pr_adOu, row[22]);
        if (row[23] != NULL)
            strcpy(pEmployeeaccount[i].pr_dbName, row[23]);
        
        i++;
    }
    mysql_free_result(res);
    return rtn;
}
int GetEmployeeaccount(char* pr_id, PPr_Employeeaccount pEmployeeaccount) {
    if(strcmp(dbtype,"MYSQL") != 0){
	    return OGetEmployeeaccount(pr_id,pEmployeeaccount);
    }
    char sql[1024];
    MYSQL_RES *res;
    MYSQL_ROW row;
    int num;
    int rtn = 0;

    if (pEmployeeaccount == NULL) {
        return 1;
    }

    memset(sql, 0, sizeof (sql));
    sprintf(sql, "select * from pr_employeeaccount where pr_id='%s'", pr_id);
    printdblog(sql);
    printdblog("\n");

    if (mysql_query(mysql_sock, sql)) {
        return 2;
    }
    if (!(res = mysql_store_result(mysql_sock))) {
        return 3;
    }
    num = mysql_num_rows(res);
    if (0 < num && (row = mysql_fetch_row(res))) {
        strcpy(pEmployeeaccount->pr_id, row[0]);
        strcpy(pEmployeeaccount->pr_userId, row[1]);
        strcpy(pEmployeeaccount->pr_uid, row[2]);
        strcpy(pEmployeeaccount->pr_userName, row[3]);
        strcpy(pEmployeeaccount->pr_resId, row[4]);
        strcpy(pEmployeeaccount->pr_resAddr, row[5]);
        strcpy(pEmployeeaccount->pr_resName, row[6]);
        strcpy(pEmployeeaccount->pr_resType, row[7]);
        strcpy(pEmployeeaccount->pr_accountId, row[8]);
        strcpy(pEmployeeaccount->pr_accountNo, row[9]);
        strcpy(pEmployeeaccount->pr_accountPwd, row[10]);
        pEmployeeaccount->pr_requestFlag = atoi(row[11]);
        if (row[12] != NULL)
            strcpy(pEmployeeaccount->pr_groupId, row[12]);
        pEmployeeaccount->pr_flag = atoi(row[13]);
        strcpy(pEmployeeaccount->pr_startTime, row[14]);
        if (row[15] != NULL)
            strcpy(pEmployeeaccount->pr_endTime, row[15]);
        if (row[16] != NULL)
            strcpy(pEmployeeaccount->pr_delayTime, row[16]);
        if (row[17] != NULL)
            strcpy(pEmployeeaccount->pr_approverId, row[17]);
        if (row[18] != NULL)
            strcpy(pEmployeeaccount->pr_approverName, row[18]);
        if (row[19] != NULL)
            strcpy(pEmployeeaccount->pr_operDate, row[19]);
        if (row[20] != NULL)
            strcpy(pEmployeeaccount->pr_desc, row[20]);
        pEmployeeaccount->pr_status = atoi(row[21]);
        if (row[22] != NULL)
            strcpy(pEmployeeaccount->pr_adOu, row[22]);
        if (row[23] != NULL)
            strcpy(pEmployeeaccount->pr_dbName, row[23]);


    } else {
        rtn = 4;
    }
    mysql_free_result(res);
    return rtn;
}

int GetJOINTable(char * pr_uid, char * pr_address, char * pr_userName, int pr_code, PJOINTableID pTableID, int * table_count) {
    if(strcmp(dbtype,"MYSQL") != 0){
	    return OGetJOINTable(pr_uid,pr_address,pr_userName,pr_code,pTableID,table_count);
    }
    char sql[10240];
    MYSQL_RES *res;
    MYSQL_ROW row;
    int num;
    int rtn = 0;

    if (pTableID == NULL) {
        return 1;
    }
    long ts;
    struct tm *ptm;
    int y, m, d, h, n, s;
    char timenow[1024];
    ts = time(NULL);
    ptm = localtime(&ts);

    y = ptm->tm_year + 1900;
    m = ptm->tm_mon + 1;
    d = ptm->tm_mday;
    h = ptm->tm_hour;
    n = ptm->tm_min;
    s = ptm->tm_sec;

    sprintf(timenow, "%04d-%02d-%02d %02d:%02d:%02d", y, m, d, h, n, s);

    memset(sql, 0, sizeof (sql));
    if(pr_code == 502){
         sprintf(sql, "SELECT Pr_Employee.pr_id,Pr_EmployeeAccount.pr_id,Pr_Resource.pr_id,pr_resaccount.pr_id,pr_employeerole.pr_roleId,Pr_AccessProtocol.pr_id  FROM \
                     Pr_Employee,Pr_EmployeeAccount,Pr_Resource, pr_resaccount,Pr_ResProtocol,Pr_AccessProtocol,Pr_EmployeeRole where \
                     Pr_Employee.pr_uid='%s' AND\
                     Pr_Employee.pr_id=Pr_EmployeeAccount.pr_userId and\
                    Pr_EmployeeAccount.pr_resId = Pr_Resource.pr_id and \
                     pr_employeeaccount.pr_accountId=pr_resaccount.pr_id and\
                     Pr_Resource.pr_address='%s' and\
                     pr_resaccount.pr_resId=pr_resource.pr_id AND\
                     pr_resaccount.pr_accountNo='%s' and\
                     Pr_Resource.pr_id=Pr_ResProtocol.pr_resId and\
                     Pr_ResProtocol.pr_apId=Pr_AccessProtocol.pr_id and\
                     (Pr_AccessProtocol.pr_code=%d or\
                     Pr_AccessProtocol.pr_code=201 or\
                     Pr_AccessProtocol.pr_code=202) and\
                     Pr_Employee.pr_id=Pr_EmployeeRole.pr_userId AND \
                     Pr_Resource.pr_status=1 and \
                     pr_employeeaccount.pr_status=1 AND \
                     ( pr_employeeaccount.pr_flag=1 OR \
                            (pr_employeeaccount.pr_flag=2 AND pr_employeeaccount.pr_endTime is NULL) OR \
                            (pr_employeeaccount.pr_flag=2 AND pr_employeeaccount.pr_endTime>'%s')\
                     )",
                pr_uid, pr_address, pr_userName, pr_code, timenow);       
    }else{
        sprintf(sql, "SELECT Pr_Employee.pr_id,Pr_EmployeeAccount.pr_id,Pr_Resource.pr_id,pr_resaccount.pr_id,pr_employeerole.pr_roleId,Pr_AccessProtocol.pr_id  FROM \
                     Pr_Employee,Pr_EmployeeAccount,Pr_Resource, pr_resaccount,Pr_ResProtocol,Pr_AccessProtocol,Pr_EmployeeRole where \
                     Pr_Employee.pr_uid='%s' AND\
                     Pr_Employee.pr_id=Pr_EmployeeAccount.pr_userId and\
                    Pr_EmployeeAccount.pr_resId = Pr_Resource.pr_id and \
                     pr_employeeaccount.pr_accountId=pr_resaccount.pr_id and\
                     Pr_Resource.pr_address='%s' and\
                     pr_resaccount.pr_resId=pr_resource.pr_id AND\
                     pr_resaccount.pr_accountNo='%s' and\
                     Pr_Resource.pr_id=Pr_ResProtocol.pr_resId and\
                     Pr_ResProtocol.pr_apId=Pr_AccessProtocol.pr_id and\
                     Pr_AccessProtocol.pr_code=%d AND\
                     Pr_Employee.pr_id=Pr_EmployeeRole.pr_userId AND \
                     Pr_Resource.pr_status=1 and \
                     pr_employeeaccount.pr_status=1 AND \
                     ( pr_employeeaccount.pr_flag=1 OR \
                            (pr_employeeaccount.pr_flag=2 AND pr_employeeaccount.pr_endTime is NULL) OR \
                            (pr_employeeaccount.pr_flag=2 AND pr_employeeaccount.pr_endTime>'%s')\
                     )",
                pr_uid, pr_address, pr_userName, pr_code, timenow);
    }
    printdblog(sql);
    printdblog("\n");

    if (mysql_query(mysql_sock, sql)) {
        return 2;
    }
    if (!(res = mysql_store_result(mysql_sock))) {
        return 3;
    }
    num = mysql_num_rows(res);
    if (0 < num) {
        int index = 0;
        while (row = mysql_fetch_row(res)) {
            strcpy(pTableID[index].pr_employee_id, row[0]);
            strcpy(pTableID[index].pr_employeeaccount_id, row[1]);
            strcpy(pTableID[index].pr_resource_id, row[2]);
            strcpy(pTableID[index].pr_resaccount_id, row[3]);
            strcpy(pTableID[index].pr_role_id, row[4]);
            strcpy(pTableID[index].pr_accessprotocol_id, row[5]);
            index++;
        }
        *table_count = index;
    } else {
        rtn = 4;
    }
    mysql_free_result(res);
    return rtn;
}

int GetPolicy(PJOINTableID pTableID, PPolicy pPolicy, int* isResource) {
    if(strcmp(dbtype,"MYSQL") != 0){
	    return OGetPolicy(pTableID,pPolicy,isResource);
    }
    char sql[4096];
    MYSQL_RES *res;
    MYSQL_ROW row;
    int num;
    int rtn = 0;
    int index = 0;

    if (pTableID == NULL) {
        return 1;
    }


    memset(sql, 0, sizeof (sql));
    sprintf(sql, "SELECT Pr_ProlicyRoleItem.pr_id from Pr_ProlicyRoleItem  WHERE \
             Pr_ProlicyRoleItem.pr_roleId='%s' and \
             pr_prolicyroleitem.pr_relationId='%s' AND \
             pr_prolicyroleitem.pr_type='2'",
            pTableID->pr_role_id, pTableID->pr_resource_id);
requery:
    printdblog(sql);
    printdblog("\n");
    if (mysql_query(mysql_sock, sql)) {
        printdblog("error\n");
        return 2;
    }
    if (!(res = mysql_store_result(mysql_sock))) {
        return 3;
    }
    num = mysql_num_rows(res);
    if (0 < num && (row = mysql_fetch_row(res))) {
        strcpy(pPolicy->pr_prolicyroleitem_id, row[0]);
        if (index == 0)
            *isResource = 1;
    } else if (*isResource == 0) {

        if (index == 0) {
            index++;
            sprintf(sql, "SELECT Pr_ProlicyRoleItem.pr_id from Pr_ProlicyRoleItem,pr_resprolicygroupitem WHERE    \
                        pr_resprolicygroupitem.pr_resId='%s'  and \
                        pr_resprolicygroupitem.pr_rpgId=Pr_ProlicyRoleItem.pr_relationId and   \
                        Pr_ProlicyRoleItem.pr_roleId='%s'",
                    pTableID->pr_resource_id, pTableID->pr_role_id);
            mysql_free_result(res);
            goto requery;

        } else if (index == 1) {
            index++;
            mysql_free_result(res);

            char groupid[32] = {0};
            sprintf(sql, "SELECT pr_resprolicygroupitem.pr_rpgId from pr_resprolicygroupitem WHERE    \
                        pr_resprolicygroupitem.pr_resId='%s'",
                    pTableID->pr_resource_id);
            printdblog(sql);
            printdblog("\n");
            if (mysql_query(mysql_sock, sql)) {
                printdblog("error\n");
                return 2;
            }
            if (!(res = mysql_store_result(mysql_sock))) {
                return 3;
            }
            num = mysql_num_rows(res);
            if (0 < num && (row = mysql_fetch_row(res))) {
                strcpy(groupid, row[0]);
                mysql_free_result(res);
            } else {
                mysql_free_result(res);
                return 5;
            }
requery2:
            sprintf(sql, "SELECT pr_resprolicygroup.pr_parentId from pr_resprolicygroup WHERE \
                                 pr_resprolicygroup.pr_id='%s'",
                    groupid);
            printdblog(sql);
            printdblog("\n");
            if (mysql_query(mysql_sock, sql)) {
                printdblog("error\n");
                return 2;
            }
            if (!(res = mysql_store_result(mysql_sock))) {
                return 3;
            }
            num = mysql_num_rows(res);
            if (0 < num && (row = mysql_fetch_row(res))) {
                strcpy(groupid, row[0]);
                mysql_free_result(res);
            } else {
                return 5;
                mysql_free_result(res);
            }


            if (strcmp(groupid, "1000000000") == 0) {
                sprintf(sql, "SELECT Pr_ProlicyRoleItem.pr_id from Pr_ProlicyRoleItem,pr_resprolicygroup WHERE    \
                                    Pr_ProlicyRoleItem.pr_relationId='%s' and   \
                                    Pr_ProlicyRoleItem.pr_roleId='0'",
                        groupid);
            } else {
                sprintf(sql, "SELECT Pr_ProlicyRoleItem.pr_id from Pr_ProlicyRoleItem,pr_resprolicygroup WHERE    \
                                Pr_ProlicyRoleItem.pr_relationId='%s' and   \
                                Pr_ProlicyRoleItem.pr_roleId='%s'",
                        groupid, pTableID->pr_role_id);
            }
            printdblog(sql);
            printdblog("\n");
            if (mysql_query(mysql_sock, sql)) {
                printdblog("error\n");
                return 2;
            }
            if (!(res = mysql_store_result(mysql_sock))) {
                return 3;
            }
            num = mysql_num_rows(res);
            if (0 < num && (row = mysql_fetch_row(res))) {
                strcpy(pPolicy->pr_prolicyroleitem_id, row[0]);
                goto exitquery;
            }

            if ((strcmp(groupid, "1000000000") == 0||strcmp(groupid, "0000000000") == 0) && num == 0) {
                rtn = 6;
                goto exitquery;
            }

            goto requery2;
        }
        rtn = 4;
    } else {
        rtn = 4;
    }
exitquery:
    mysql_free_result(res);

    if (rtn != 0) {
        return rtn;
    }

    memset(sql, 0, sizeof (sql));
    sprintf(sql, "SELECT Pr_NclItem.pr_startIp,Pr_NclItem.pr_endIp from Pr_ProlicyRoleItem,Pr_NclItem WHERE \
	Pr_ProlicyRoleItem.pr_id='%s' and \
	Pr_NclItem.pr_relationId=Pr_ProlicyRoleItem.pr_id  \
	GROUP BY Pr_NclItem.pr_id ",
            pPolicy->pr_prolicyroleitem_id);
    printdblog(sql);
    printdblog("\n");
    if (mysql_query(mysql_sock, sql)) {
        return 2;
    }
    if (!(res = mysql_store_result(mysql_sock))) {
        return 3;
    }
    num = mysql_num_rows(res);
    if (num > 0) {
        int index = 0;
        pPolicy->nclLen = num;
        pPolicy->pNclItem = malloc(num * sizeof (Pr_NclItem));

        while (row = mysql_fetch_row(res)) {
            strcpy(pPolicy->pNclItem[index].pr_startIp, row[0]);
            strcpy(pPolicy->pNclItem[index].pr_endIp, row[1]);
            index++;
        }
    }
    mysql_free_result(res);

    memset(sql, 0, sizeof (sql));
    sprintf(sql, "SELECT Pr_AccessProtocol.pr_name,Pr_AccessProtocol.pr_code from Pr_ProlicyRoleItem,Pr_PclItem,Pr_AccessProtocol WHERE \
	Pr_ProlicyRoleItem.pr_id='%s' and \
	Pr_PclItem.pr_relationId=Pr_ProlicyRoleItem.pr_id  and \
	Pr_PclItem.pr_apId=Pr_AccessProtocol.pr_id \
	GROUP BY Pr_AccessProtocol.pr_id ",
            pPolicy->pr_prolicyroleitem_id);
    printdblog(sql);
    printdblog("\n");
    if (mysql_query(mysql_sock, sql)) {
        return 2;
    }
    if (!(res = mysql_store_result(mysql_sock))) {
        return 3;
    }
    num = mysql_num_rows(res);
    if (num > 0) {
        int index = 0;
        pPolicy->pclLen = num;
        pPolicy->pPclItem = malloc(num * sizeof (Pr_PclItem));

        while (row = mysql_fetch_row(res)) {
            strcpy(pPolicy->pPclItem[index].pr_name, row[0]);
            pPolicy->pPclItem[index].pr_code = atoi(row[1]);
            index++;
        }
    }
    mysql_free_result(res);

    memset(sql, 0, sizeof (sql));
    sprintf(sql, "SELECT Pr_CclItem.pr_command,Pr_CclItem.pr_type,Pr_CclItem.pr_tname from Pr_ProlicyRoleItem,Pr_CclItem WHERE \
	Pr_ProlicyRoleItem.pr_id='%s' and \
	Pr_CclItem.pr_relationId=Pr_ProlicyRoleItem.pr_id \
	GROUP BY Pr_CclItem.pr_id",
            pPolicy->pr_prolicyroleitem_id); //3.5.2
   /* sprintf(sql, "SELECT Pr_CclItem.pr_command,Pr_CclItem.pr_type from Pr_ProlicyRoleItem,Pr_CclItem WHERE \
	Pr_ProlicyRoleItem.pr_id='%s' and \
	Pr_CclItem.pr_relationId=Pr_ProlicyRoleItem.pr_id \
	GROUP BY Pr_CclItem.pr_id",
            pPolicy->pr_prolicyroleitem_id); *///3.5.1
    
    printdblog(sql);
    printdblog("\n");
    if (mysql_query(mysql_sock, sql)) {
        return 2;
    }
    if (!(res = mysql_store_result(mysql_sock))) {
        return 3;
    }
    num = mysql_num_rows(res);
    if (num > 0) {
        int index = 0;
        pPolicy->cclLen = num;
        pPolicy->pCclItem = malloc(num * sizeof (Pr_CclItem));

        while (row = mysql_fetch_row(res)) {
            strcpy(pPolicy->pCclItem[index].pr_command, strupr(row[0]));
            pPolicy->pCclItem[index].pr_type = atoi(row[1]);
            strcpy(pPolicy->pCclItem[index].pr_table, strupr(row[2]));
            index++;
        }
    }
    mysql_free_result(res);

    memset(sql, 0, sizeof (sql));
    sprintf(sql, "SELECT Pr_TclItem.pr_cycleFlag,Pr_TclItem.pr_cycleLimit,Pr_TclItem.pr_statTime,Pr_TclItem.pr_endTime,Pr_TclItem.pr_timeFlag,Pr_TclItem.pr_startDate from Pr_ProlicyRoleItem,Pr_Role,Pr_TclItem WHERE \
	Pr_ProlicyRoleItem.pr_id='%s' and \
	Pr_TclItem.pr_relationId=Pr_ProlicyRoleItem.pr_id \
	GROUP BY Pr_TclItem.pr_id",
            pPolicy->pr_prolicyroleitem_id);
    printdblog(sql);
    printdblog("\n");
    if (mysql_query(mysql_sock, sql)) {
        return 2;
    }
    if (!(res = mysql_store_result(mysql_sock))) {
        return 3;
    }
    num = mysql_num_rows(res);
    if (num > 0) {
        int index = 0;
        pPolicy->tclLen = num;
        pPolicy->pTclItem = malloc(num * sizeof (Pr_TclItem));

        while (row = mysql_fetch_row(res)) {
            pPolicy->pTclItem[index].pr_cycleFlag = atoi(row[0]);
            strcpy(pPolicy->pTclItem[index].pr_cycleLimit, row[1]);
            if (row[2] != NULL)
                strcpy(pPolicy->pTclItem[index].pr_statTime, row[2]);
            if (row[3] != NULL)
                strcpy(pPolicy->pTclItem[index].pr_endTime, row[3]);
            pPolicy->pTclItem[index].pr_timeFlag = atoi(row[4]);
            if (row[5] != NULL)
                strcpy(pPolicy->pTclItem[index].pr_startDate, row[5]);
            index++;
        }
    }
    mysql_free_result(res);

    return rtn;
}

int MatchNclItem(PPolicy pPolicy, char * Ip) {
    int i;
    if (pPolicy == NULL) {
        return 0;
    }
    if (pPolicy->nclLen <= 0) {
        return 0;
    }
    for (i = 0; i < pPolicy->nclLen; i++) {
        if (ntohl(inet_addr(Ip)) >= ntohl(inet_addr(pPolicy->pNclItem[i].pr_startIp))
                && ntohl(inet_addr(Ip)) <= ntohl(inet_addr(pPolicy->pNclItem[i].pr_endIp))) {
            return 0;
        }
    }
    return 1;
}

int MatchPclItem(PPolicy pPolicy, int code) {
    int i;
    if (pPolicy == NULL) {
        return 0;
    }
    if (pPolicy->pclLen <= 0) {
        return 0;
    }
    for (i = 0; i < pPolicy->pclLen; i++) {
        if(code == 502){
           if (pPolicy->pPclItem[i].pr_code == code || pPolicy->pPclItem[i].pr_code == 201 || pPolicy->pPclItem[i].pr_code == 202) {
                return 0;
            } 
        }else{
            if (pPolicy->pPclItem[i].pr_code == code) {
                return 0;
            }
        }
    }
    return 1;
}

/**
 * Split a string into some strings according to a list of separators.
 *
 * @Param dest			out: storage the strings has be split.
 * @Param count			out: the number of strings has be split successfully, 0 for failed to split.
 * @Param s_str			in:  the strings for split.
 * @Param separator			in:  the list of split separators.
 * @Param number_separator		in:  the numbers of separators.
 * @Param compress_separator		in:  will be create a empty string when two split adjacent
 *					if compress_separator > 0 and not for compress_separator == 0
 * @Param keep_separator		in:  the separators will be put into parameter 'dest' if keep_separator > 0
 */
int strsplit(char ***dest, int *count, char *s_str, char **separator, int number_separators, int compress_separator, int keep_separator)
{
	int i = 0;
	char **result = NULL;
	char **temp_result = NULL;
	unsigned int curt_size = 0;
	unsigned int new_size = 0;
	char *look_ahead = NULL;
	char *most_front_separator_start = NULL;
	char *most_front_separator_end = NULL;
	char *separator_start = NULL;
	int find_a_separator = 0;
	int find_a_string = 0;
	int rtn = 0;

	*count = 0;
	*dest = NULL;

	/* check parameters */
	if (
		dest == NULL 
		|| s_str == NULL || *s_str == '\0' 
		|| separator == NULL 
		|| number_separators <= 0
		|| compress_separator < 0
		|| keep_separator < 0
		)
		return -1;

	for (i = 0; i < number_separators; i++)
		if (separator[i] == NULL || *separator[i] == '\0')
			return -1;

	for (look_ahead = s_str; *look_ahead != '\0'; look_ahead = most_front_separator_end)
	{
		most_front_separator_start = look_ahead + strlen(look_ahead);
		most_front_separator_end = look_ahead + strlen(look_ahead);
		find_a_separator = 0;

		/* find the next separator. */
		for (i = 0; i < number_separators; i++)
		{
			separator_start = strstr(look_ahead, separator[i]);
			if (separator_start == NULL)
				continue;

			find_a_separator = 1;
			rtn=1;
			/* update the most front separator. */
			if (separator_start < most_front_separator_start)
			{
				most_front_separator_start = separator_start;
				most_front_separator_end = most_front_separator_start + strlen(separator[i]);
			}
		}

		find_a_string = (look_ahead == most_front_separator_start) ? 0 : 1;

		/* allow put the new string into result if need. */
		new_size = (find_a_string > 0) ? (curt_size + 1) : ((compress_separator > 0) ? curt_size : (curt_size + 1));
		/* allow put the separator into result if need. */
		new_size = (keep_separator > 0) ? (new_size + 1) : new_size;
		if (new_size == curt_size)
			continue;

		temp_result = (char **)malloc((new_size) * sizeof(char *));
		if (temp_result == NULL)
		{
			if (result != NULL)
			{
				for (i = 0; i < curt_size; i++)
					if (result[i] != NULL) 
						free(result[i]);
				free(result);
				result = NULL;
			}

			return -2;
		}
	
		/* copy the pointers of string find early. */
		memset(temp_result, 0, new_size);
		for (i = 0; i < curt_size; i++)
			temp_result[i] = result[i];

		if (find_a_string == 0)
		{
			if (compress_separator == 0)
			{
				temp_result[curt_size] = (char *)malloc(sizeof(char));
				if (temp_result[curt_size] == NULL)
				{
					if (temp_result != NULL)
					{
						for (i = 0; i < curt_size; i++)
							if (temp_result[i] != NULL) 
								free(temp_result[i]);
						free(temp_result);
						temp_result = NULL;
					}

					return -2;
				}
				memset(temp_result[curt_size], 0, 1);
			}
		} 
		else
		{
			/* put the new string into result. */
			temp_result[curt_size] = (char *)malloc((most_front_separator_start - look_ahead + 1) * sizeof(char));
			if (temp_result[curt_size] == NULL)
			{
				if (temp_result != NULL)
				{
					for (i = 0; i < curt_size; i++)
						if (temp_result[i] != NULL) 
							free(temp_result[i]);
					free(temp_result);
					temp_result = NULL;
				}

				return -2;
			}
			memset(temp_result[curt_size], 0, most_front_separator_start - look_ahead + 1);
			strncpy(temp_result[curt_size], look_ahead, most_front_separator_start - look_ahead);
			temp_result[curt_size][most_front_separator_start - look_ahead] = '\0';
		}
		
		if (keep_separator > 0)
		{	
			/* put the separator into result. */
			temp_result[new_size - 1] = (char *)malloc(most_front_separator_end - most_front_separator_start + 1);
			if (temp_result[new_size - 1] == NULL)
			{
				if (temp_result != NULL)
				{
					for (i = 0; i < new_size - 1; i++)
						if (temp_result[i] != NULL) 
							free(temp_result[i]);
					free(temp_result);
					temp_result = NULL;
				}

				return -2;
			}
			memset(temp_result[new_size - 1], 0, most_front_separator_end - most_front_separator_start + 1);
			strncpy(temp_result[new_size - 1], most_front_separator_start, most_front_separator_end - most_front_separator_start);
			temp_result[new_size - 1][most_front_separator_end - most_front_separator_start] = '\0';
		}

		/* update result. */
		free(result);
		result = temp_result;
		temp_result = NULL;
		curt_size = new_size;
	}

	*dest = result;
	*count = curt_size;
	
	return rtn;
}
int FindDBTable(char *sql,char * cmd,char * table){
	char *separator[] = {"SELECT", "INSERT", "UPDATE", "DELETE"};
	char **result = NULL;
	int n_str = 0;
	int rtn = 0;
	
	int i = strsplit(&result, &n_str, sql, separator, 4, 1, 1);
	if(i==1){
		for (i = 0; i < n_str-1; i++){
			if(strlen(result[i])==strlen(cmd)&&strstr(result[i],cmd)!=NULL){
				if(strstr(result[i+1],table)!=NULL){
					rtn =1;
					break;
				}
			}else{
				if(strstr(result[i],cmd)!=NULL&&strstr(result[i+1],table)!=NULL){
					rtn =1;
					break;
				}
			}
		}
			
	}else{
		if(strstr(sql,cmd)!=NULL&&strstr(sql,table)!=NULL){
					rtn =1;
		}
	}
	for (i = 0; i < n_str; i++)
		free(result[i]);
	free(result);
	
	return rtn;
}
int MatchCclItem(PPolicy pPolicy, char * cmd, int type) {
    int rtn = 0;
    int i;
    if (pPolicy == NULL) {
        return 0;
    }
    if (pPolicy->cclLen <= 0) {
        return 0;
    }
    if (type == 1) {
        rtn = 0;
    } else if (type == 2) {
        rtn = 1;
    }
    char mycmd[10240];
    memset(mycmd,0,sizeof(mycmd));
    strcpy(mycmd,cmd);
    strupr((char*)mycmd);
    
    
    printdblog("MatchCclItem cmd:%s", mycmd);
     for (i = 0; i < pPolicy->cclLen; i++) {
	     char tname[1024] = {0};
	     memset(tname,0,sizeof(tname));
	     
	     strcpy(tname , pPolicy->pCclItem[i].pr_table);
	     printdblog(" [%d][%s][%s]", i,pPolicy->pCclItem[i].pr_command,tname);
	     if(strlen(tname) <= 0){
		    if (strstr(mycmd, pPolicy->pCclItem[i].pr_command) != NULL) {
			 printdblog("==\n");
			if (rtn == 0) return 1;
			else return 0;
		    }  
	     }else{
		     char seps[] = ",";
		     char *token;
		     token = strtok(tname, seps);
		     while (token != NULL) {
			    char param[1024] = {0};
			    memset(param,0,sizeof(param));
			    strcpy(param, token);
			    if (FindDBTable(mycmd, pPolicy->pCclItem[i].pr_command, param) == 1) {
				printdblog("--\n");
				if (rtn == 0) return 1;
				else return 0;
			    }
			     token = strtok(NULL, seps);
		    }
	     }
        

	
       }
     printdblog("\n");
   /* for (i = 0; i < pPolicy->cclLen; i++) {
        if (strstr(mycmd, pPolicy->pCclItem[i].pr_command) != NULL) {
            if (rtn == 0) return 1;
            else return 0;
        }

    }*/
    return rtn;
}

int MatchTclItem(PPolicy pPolicy) {
    int rtn = 1;
    int i;
    if (pPolicy == NULL) {
        return 0;
    }
    if (pPolicy->tclLen <= 0) {
        return 0;
    }

    long ts;
    struct tm *ptm;
    int y, m, d, h, n, s;
    char timenow[1024];
    ts = time(NULL);
    ptm = localtime(&ts);

    y = ptm->tm_year + 1900;
    m = ptm->tm_mon + 1;
    d = ptm->tm_mday;
    h = ptm->tm_hour;
    n = ptm->tm_min;
    s = ptm->tm_sec;
    memset(timenow, 0, sizeof (timenow));
    sprintf(timenow, "%02d:%02d:%02d", h, n, s);

    for (i = 0; i < pPolicy->tclLen; i++) {
        if (strcmp(pPolicy->pTclItem[i].pr_statTime, timenow) <= 0 && strcmp(pPolicy->pTclItem[i].pr_endTime, timenow) >= 0) {
            return 0;
        }
    }
    return rtn;
}

int SetLoginlog(Pr_Loginlog loginlog, char * id) {
    if(strcmp(dbtype,"MYSQL") != 0){
	    return OSetLoginlog(loginlog,id);
    }
    char sql[1024];
    MYSQL_RES *res;
    MYSQL_ROW row;
    int num;
    int rtn = 0;

    long ts;
    struct tm *ptm;
    int y, m, d, h, n, s;
    char timenow[1024];
    ts = time(NULL);
    ptm = localtime(&ts);

    y = ptm->tm_year + 1900;
    m = ptm->tm_mon + 1;
    d = ptm->tm_mday;
    h = ptm->tm_hour;
    n = ptm->tm_min;
    s = ptm->tm_sec;
    //time
    memset(timenow, 0, sizeof (timenow));
    sprintf(loginlog.pr_id, "%02d%02d%02d%d%d", h, n, s, rand(), rand());
    sprintf(loginlog.pr_loginDate, "%04d-%02d-%02d %02d:%02d:%02d", y, m, d, h, n, s);
    sprintf(loginlog.pr_onLineTime, "%04d-%02d-%02d %02d:%02d:%02d", y, m, d, h, n, s);
    strcpy(id, loginlog.pr_id);

    memset(sql, 0, sizeof (sql));
    sprintf(sql, "INSERT INTO pr_loginlog VALUES ('%s', '%s', '%s', '', '', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%d', '%s', null, '%s', '%s', '%d', '%d', '%d', '%s', '%d', '%d', null,'%s','%s','%s','%s');",
            loginlog.pr_id,
            loginlog.pr_uid,
            loginlog.pr_userName,
            // loginlog.pr_orgId[32],
            //loginlog.pr_orgName[80],
            loginlog.pr_accountNo,
            loginlog.pr_clientAddr,
            loginlog.pr_resAddr,
            loginlog.pr_resId,
            loginlog.pr_resName,
            loginlog.pr_resType,
            loginlog.pr_apId,
            loginlog.pr_accessType,
            loginlog.pr_loginDate,
            //loginlog.pr_loginOutDate,
            loginlog.pr_onLineTime,
            loginlog.pr_proxyAddr,
            loginlog.pr_port,
            loginlog.pr_videoFlag,
            loginlog.pr_cmdFlag,
            loginlog.pr_logRecord,
            loginlog.pr_status,
            loginlog.pr_nodeFlag,
            //loginlog.pr_auditUserId[32],
            loginlog.pr_authaccessresid,
            loginlog.pr_proxyHost,
            loginlog.pr_cooperator,
            loginlog.pr_accessMemo
            );
    printdblog("LogTrans:%s\n", sql);
    //SaveDB2File(sql);
    if(Audit_ConnectMysql()!=0)
        return 0;
    if(audit_mysql_sock == NULL)
        return 0;
    if(mysql_query(audit_mysql_sock,sql))
    {
        //Audit_CloseDB() ;
        return 1;
    }
    //Audit_CloseDB() ;
    return 0;
}

//type : 1=onLine ; 2=loginOut
#define SYSLOG//sugl
#define SVW
//#define LUOAN
int code_convert(char *from_charset,char *to_charset,char *inbuf,int inlen,char *outbuf,int outlen) 
   { 
   iconv_t cd; 
   int rc; 
   char **pin = &inbuf; 
   char **pout = &outbuf; 

   cd = iconv_open(to_charset,from_charset); 
   if (cd==0) return -1; 
   memset(outbuf,0,outlen); 
   if (iconv(cd,pin,&inlen,pout,&outlen)==-1) return -1; 
   iconv_close(cd); 
   return 0; 
   } 
int ReflashLoginlog(char * loginlog_id, int type) {
    if(strcmp(dbtype,"MYSQL") != 0){
	    return OReflashLoginlog(loginlog_id,type);
    }
    char sql[1024];

    long ts;
    struct tm *ptm;
    int y, m, d, h, n, s;
    char timenow[1024];
    ts = time(NULL);
    ptm = localtime(&ts);

    y = ptm->tm_year + 1900;
    m = ptm->tm_mon + 1;
    d = ptm->tm_mday;
    h = ptm->tm_hour;
    n = ptm->tm_min;
    s = ptm->tm_sec;
    //time
    memset(timenow, 0, sizeof (timenow));
    sprintf(timenow, "%04d-%02d-%02d %02d:%02d:%02d", y, m, d, h, n, s);

    memset(sql, 0, sizeof (sql));
    if (type == 1) {
        sprintf(sql, "UPDATE pr_loginlog set pr_onLineTime='%s' where pr_id='%s'", timenow, loginlog_id);
    } else {
        sprintf(sql, "UPDATE pr_loginlog set pr_loginOutDate='%s',pr_status=0 where pr_id='%s'", timenow, loginlog_id);
    }
    printdblog("ReflashLoginlog LogTrans:%s\n", sql);
    //SaveDB2File(sql);
    if(Audit_ConnectMysql()!=0)
    {
        printdblog("connect mysql failed!\n");
        return 0;
    }

    if(audit_mysql_sock == NULL)
    {
        printdblog("audit_mysql_sock is NULL!\n");
        return 0;
    }
    
    if(mysql_query(audit_mysql_sock, sql))
    {        
        printdblog("mysql_query failed! error information:%s\n", mysql_error(audit_mysql_sock));
           //Audit_CloseDB() ;
        return 1;
    }
#ifdef SYSLOG
	if(type != 1)//syslog
			{
				MYSQL_RES *res,*resX;
			    MYSQL_ROW row,rowX;
				memset(sql,0,sizeof(sql));
				sprintf(sql,"select pr_uid,pr_userName,pr_accountNo,pr_resName,pr_resAddr,pr_clientAddr,pr_apId,pr_loginDate from pr_loginlog where pr_id='%s'",loginlog_id);
				printdblog("LogTrans:%s\n", sql);
				if (mysql_query(audit_mysql_sock, sql)) {
					printdblog("mysql_query failed:[%s]",mysql_error(audit_mysql_sock));
		        	return 0;
		    	}
			    if (!(res = mysql_store_result(audit_mysql_sock))) {
					printdblog("mysql_store_result failed:[%s]",mysql_error(audit_mysql_sock));
			        return 0;
			    }
			        int num = mysql_num_rows(res);
			    if (0 < num && (row = mysql_fetch_row(res))) {
			        char formatStr[2048]={0};
					//getaccess pr_name
					char sqlAccessName[256]={0};
					sprintf(sqlAccessName,"select pr_name from pr_accessprotocol where pr_id =%s",row[6]);
					ConnectMysql();
					if (mysql_query(mysql_sock, sqlAccessName)) {
					printdblog("mysql_query failed:[%s][1]",mysql_error(mysql_sock));
		        	return 0;
			    	}
				    if (!(resX = mysql_store_result(mysql_sock))) {
						printdblog("mysql_store_result failed:[%s][1]",mysql_error(mysql_sock));
				        return 0;
				    }
					num = mysql_num_rows(resX);
					if (0 < num && (rowX = mysql_fetch_row(resX))) {
#ifdef SVW
						printdblog("LogTrans:%s\n", sqlAccessName);
                                                char splitChar = 5;//0x05
						sprintf(formatStr,"login%c%s%c%s%c%s%c%s%c%s%c%s%c%s%c%s%c%s",splitChar,row[0],splitChar,row[1],splitChar,row[2],splitChar,row[3],splitChar,row[4],splitChar,
							row[5],splitChar,rowX[0],splitChar,row[7],splitChar,timenow);
						syslogEx(LOG_USER|LOG_INFO,"%s",formatStr);
						printdblog("syslog[%s]\n",formatStr);
#endif
#ifdef LUOAN
			 		    char descriptions[1024]={0};
				        sprintf(descriptions,"login,用户主账号:%s 用户名称:%s 资源账号:%s 资源名称:%s 资源地址ַ:%s 客户端地址:%s 访问类型:%s 登录时间:%s 登出时间:%s",
							row[0],row[1],row[2],row[3],row[4],row[5],rowX[0],row[7],timenow);
					sprintf(formatStr,"id=4A time=\"%s\" 4A=\"IT云运维安全平台̨\" pri=1 type=\"access info\" description=\"%s\" dip=%s \
dport=\"\" sdomain=\"\" sip=%s sport=\"\" protol=udp info=\"\" solution=\"\" result=\"succeeded\"",row[7],\
						descriptions,row[4],row[5]);
					char coded[3072]={0};
					code_convert("gb2312","utf-8",formatStr,strlen(formatStr),coded,2048); 
					syslogEx(LOG_CRON|LOG_CRIT,"%s",coded);
					printdblog("syslog[%s]\n",coded);
#endif
					}
					CloseDB();
		   		}
				mysql_free_result(res);
				mysql_free_result(resX);
		    Audit_CloseDB() ;
		}
#endif
   //Audit_CloseDB() ;
    return 0;
}

int SetUseractivitylog(Pr_Useractivitylog userlog, char * userlog_id) {
    char sql[1024];
    MYSQL_RES *res;
    MYSQL_ROW row;
    int num;
    int rtn = 0;
    char * vt = "\033[m";

    long ts;
    struct tm *ptm;
    int y, m, d, h, n, s;
    char timenow[1024];
    ts = time(NULL);
    ptm = localtime(&ts);

    y = ptm->tm_year + 1900;
    m = ptm->tm_mon + 1;
    d = ptm->tm_mday;
    h = ptm->tm_hour;
    n = ptm->tm_min;
    s = ptm->tm_sec;
    //time
    memset(timenow, 0, sizeof (timenow));
    sprintf(userlog.pr_id, "%02d%02d%02d%d%d", h, n, s, rand(), rand());
    sprintf(userlog.pr_operDate, "%04d-%02d-%02d %02d:%02d:%02d", y, m, d, h, n, s);
    strcpy(userlog_id, userlog.pr_id);

    if (strstr(userlog.pr_cmdPrefix, vt) != NULL) {
        strcpy(userlog.pr_cmdPrefix, userlog.pr_cmdPrefix + strlen(vt));
    }


    memset(sql, 0, sizeof (sql));
    if(strcmp(audit_dbtype,"ORACLE") == 0){
	sprintf(sql, "INSERT INTO pr_useractivitylog VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s ', null, null, '%s', '%s', '%s', '%s', to_date('%s','yyyy-mm-dd hh24:mi:ss'), '%s', '%d', '%d');",
		userlog.pr_id,
		userlog.pr_clientAddr,
		userlog.pr_proxyAddr,
		userlog.pr_resId,
		userlog.pr_resName,
		userlog.pr_resAddr,
		userlog.pr_command,
		userlog.pr_cmdPrefix,
		//userlog.pr_orgId,
		//userlog.pr_orgName,
		userlog.pr_userId,
		userlog.pr_userName,
		userlog.pr_uid,
		userlog.pr_accountNo,
		userlog.pr_operDate,
		userlog.pr_sessionId,
		userlog.pr_level,
		userlog.pr_accessType
		);    
    }else{
	sprintf(sql, "INSERT INTO pr_useractivitylog VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s ', null, null, '%s', '%s', '%s', '%s', '%s', '%s', '%d', '%d');",
		userlog.pr_id,
		userlog.pr_clientAddr,
		userlog.pr_proxyAddr,
		userlog.pr_resId,
		userlog.pr_resName,
		userlog.pr_resAddr,
		userlog.pr_command,
		userlog.pr_cmdPrefix,
		//userlog.pr_orgId,
		//userlog.pr_orgName,
		userlog.pr_userId,
		userlog.pr_userName,
		userlog.pr_uid,
		userlog.pr_accountNo,
		userlog.pr_operDate,
		userlog.pr_sessionId,
		userlog.pr_level,
		userlog.pr_accessType
		);
    }
    printdblog("LogTrans:%s\n", sql);
    SaveDB2File(sql);
#ifdef SYSLOG
#ifdef SVW
        char formatStr[2028]={0};
	char splitChar = ',';//0x05
	sprintf(formatStr,"audit%c%s%c%s%c%s%c%s%c%s%c%s%c%s",splitChar,userlog.pr_userName,splitChar,userlog.pr_uid,splitChar,userlog.pr_clientAddr,splitChar,userlog.pr_accountNo,splitChar,userlog.pr_resAddr,splitChar,userlog.pr_operDate,splitChar,userlog.pr_command);
	syslogEx(LOG_USER|LOG_INFO,"%s",formatStr);
	printdblog("syslog[%s]\n",formatStr);
#endif
#ifdef LUOAN
        char formatStr[2028]={0};
	char descriptions[1024]={0};
	sprintf(descriptions,"audit,用户名称:%s 用户主账号:%s 客户端地址:%s 资源账号:%s 资源地址ַ:%s 操作时间:%s 关键字:%s",userlog.pr_userName,userlog.pr_uid,userlog.pr_clientAddr,userlog.pr_accountNo,userlog.pr_resAddr,userlog.pr_operDate,userlog.pr_command);
	sprintf(formatStr,"id=4A time=\"%s\" 4A=\"IT云安全运维平台\" pri=1 type=\"audit info\" description=\"%s\" dip=%s \
dport=\"\" sdomain=\"\" sip=%s sport=\"\" protol=tcp info=\"\" solution=\"\" result=\"succeeded\"",userlog.pr_operDate,\
			descriptions,userlog.pr_resAddr,userlog.pr_clientAddr);
    char coded[3072]={0};
    code_convert("gb2312","utf-8",formatStr,strlen(formatStr),coded,2048);
	syslogEx(LOG_CRON|LOG_CRIT,"%s",coded);
	printdblog("syslog[%s]\n",coded);
#endif
#endif
    //if(mysql_query(mysql_sock,sql))
    //{
    //        return 1;
    //}
    return 0;
}

int SetUseractivityoutlog(Pr_Useractivityoutlog useroutlog,int indb) {
    char sql[20*1024];
    char encode[20*1024];
    long ts; 
    struct tm *ptm; 
    int y,m,d,h,n,s; 
    char timenow[1024];
    ts = time(NULL); 
    ptm = localtime(&ts); 

    y = ptm->tm_year+1900; 
    m = ptm->tm_mon+1; 
    d = ptm->tm_mday;
    h = ptm->tm_hour; 
    n = ptm->tm_min;  
    s = ptm->tm_sec; 
//time
//    memset(encode,0,sizeof(encode));
//    memset(timenow,0,sizeof(timenow));
//    sprintf(useroutlog.pr_id,"%02d%02d%02d%d%d",h,n,s,rand(),rand());
//    sprintf(useroutlog.pr_operDate,"%04d-%02d-%02d %02d:%02d:%02d",y,m,d,h,n,s);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    memset(encode,0,sizeof(encode));
    memset(timenow,0,sizeof(timenow));
    sprintf(useroutlog.pr_id,"%d%d",tv.tv_sec,tv.tv_usec);
    sprintf(useroutlog.pr_operDate,"%04d-%02d-%02d %02d:%02d:%02d",y,m,d,h,n,s);
    
    EncodeData(encode,useroutlog.pr_desc);
    
   
    
    if(indb == 1){
        memset(encode,0,sizeof(encode));
        ClearData(encode,useroutlog.pr_desc);
        memset(sql, 0, sizeof(sql));
        if(strcmp(audit_dbtype,"ORACLE") == 0){
		sprintf(sql,"INSERT INTO pr_useractivityoutlog VALUES ('%s', '%s', '%s', to_date('%s','yyyy-mm-dd hh24:mi:ss'));",
			    useroutlog.pr_id,
			    useroutlog.pr_ualId,
			    encode,
			    useroutlog.pr_operDate
		    );
        }else{
		sprintf(sql,"INSERT INTO pr_useractivityoutlog VALUES ('%s', '%s', '%s', '%s');",
			    useroutlog.pr_id,
			    useroutlog.pr_ualId,
			    encode,
			    useroutlog.pr_operDate
		    );
        }
        printdblog("LogTrans:%s:%d\n",sql,strlen(sql));
        SaveDB2File(sql);
    }else{
         memset(sql, 0, sizeof(sql));
        sprintf(sql,"%s@#%s@#%s@#%s",
                    useroutlog.pr_id,
                    useroutlog.pr_ualId,
                    encode,
                    useroutlog.pr_operDate
            );
        //printdblog("LogTrans:%s:%d\n",sql,strlen(sql));
        SaveVideo2File(sql,useroutlog.pr_seesionId);
    }
    return 0;           
}

int EncodeData(char * src, char * dest) {
    int i;
    for (i = 0; i < strlen(dest); i++) {
        sprintf(src + 2 * i, "%02X", dest[i]);
    }
}


char* ClearVT100Data(char * dest){
    if(*dest == '\033'){
        dest ++;
        if(*dest != '['){
            dest ++;
            return dest;
        }else{
            dest ++;
            while(1){
                int isrtn = 0;
                if(*dest == 'c'){
                    isrtn = 1;
                }else if(*dest == 'n'){
                    isrtn = 1;
                }else if(*dest == 'R'){
                    isrtn = 1;
                }else if(*dest == 'h'){
                    isrtn = 1;
                }else if(*dest == 'l'){
                    isrtn = 1;
                }else if(*dest == 'H'){
                    isrtn = 1;
                }else if(*dest == 'A'){
                    isrtn = 1;
                }else if(*dest == 'B'){
                    isrtn = 1;
                }else if(*dest == 'C'){
                    isrtn = 1;
                }else if(*dest == 'D'){
                    isrtn = 1;
                }else if(*dest == 'f'){
                    isrtn = 1;
                }else if(*dest == 's'){
                    isrtn = 1;
                }else if(*dest == 'u'){
                    isrtn = 1;
                }else if(*dest == 'r'){
                    isrtn = 1;
                }else if(*dest == 'K'){
                    isrtn = 1;
                }else if(*dest == 'J'){
                    isrtn = 1;
                }else if(*dest == 'i'){
                    isrtn = 1;
                }else if(*dest == 'p'){
                    isrtn = 1;
                }else if(*dest == 'm'){
                    isrtn = 1;
                }
                dest ++;
                if(isrtn == 1)
                    break;
            }
        }
    }
    return dest;
}
int ClearData(char * src, char * dest){
    int i=0,j=0;
    while(*dest != 0){
        if(*dest == '\r' || *dest == '\n'){
            dest++;
            continue;
        }
        while(*dest == '\033'){
            dest = ClearVT100Data(dest);
        }
        src[j] = *dest;
        dest ++;
        j++;
    }
}

int SaveDB2File(char * sql){

    FILE *fa;
    
    mkdir("/tmp/proxy", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    
    long ts; 
    struct tm *ptm; 
    int y,m,d,h,n,s; 
    char filename[1024];
    ts = time(NULL); 
    ptm = localtime(&ts); 

    y = ptm->tm_year+1900; 
    m = ptm->tm_mon+1; 
    d = ptm->tm_mday;
    h = ptm->tm_hour; 
    n = ptm->tm_min;  
    s = ptm->tm_sec; 
//time
    memset(filename,0,sizeof(filename));
    sprintf(filename,"/tmp/proxy/db.db.%04d%02d%02d%02d%02d",y,m,d,h,n);
    
    fa = fopen(filename, "a+");
    if (fa == NULL) {
        return 0;
    }
    
    int fd = fileno(fa);
    lockf(fd, F_LOCK, 0l);

    fputs(sql, fa);
    fputs("\r\n",fa);
    
    lockf(fd, F_ULOCK, 0l);
    fclose(fa);
    return 0;
                
    return 0;
}

int SaveVideo2File(char * sql,char *sessionId){

    FILE *fa;
    
    mkdir("/tmp/proxyvideo", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    char filename[1024];
    
    
    memset(filename,0,sizeof(filename));
    sprintf(filename,"/tmp/proxyvideo/%s",sessionId);
    
    fa = fopen(filename, "a+");
    if (fa == NULL) {
        return 0;
    }
    
    int fd = fileno(fa);
    lockf(fd, F_LOCK, 0l);

    fputs(sql, fa);
    fputs("\r\n",fa);
    
    lockf(fd, F_ULOCK, 0l);
    fclose(fa);
    return 0;
                
    return 0;
}
char proxyIP[100];
int GetLeveCmd(PPROXYINFO pProxyInfo){
    if(strcmp(dbtype,"MYSQL") != 0){
	    return OGetLeveCmd(pProxyInfo);
    }
    char sql[1024];
    MYSQL_RES *res;
    MYSQL_ROW row;
    int num;
    int rtn = 0;
    
    memset(sql, 0, sizeof (sql));
    sprintf(sql, "select pr_auditlevelcmd.pr_alId,pr_auditlevelcmd.pr_command from pr_auditlevelres,pr_auditlevelcmd where pr_auditlevelres.pr_resId='%s' and pr_auditlevelcmd.pr_alId=pr_auditlevelres.pr_alId",
            pProxyInfo->auth.pr_resId);
    printdblog(sql);
    printdblog("\n");
    if (mysql_query(mysql_sock, sql)) {
        return 2;
    }
    if (!(res = mysql_store_result(mysql_sock))) {
        return 3;
    }
    num = mysql_num_rows(res);
    if (num > 0) {
        int index = 0;
        pProxyInfo->level_count = num;
        while (row = mysql_fetch_row(res)) {
            pProxyInfo->levelcmd[index].level = atoi(row[0]);
            strcpy(pProxyInfo->levelcmd[index].cmd , strupr(row[1]));
            index++;
        }
    }
    mysql_free_result(res);
    return rtn;    
}
int FindLevelCmd(PPROXYINFO pProxyInfo,char *cmd){
    int i = 0;
    char mycmd[512];
    memset(mycmd,0,sizeof(mycmd));
    strcpy(mycmd,cmd);
    strupr(mycmd);
    for(i = 0 ; i < pProxyInfo->level_count ; i ++){
        if(strstr(mycmd,pProxyInfo->levelcmd[i].cmd) != NULL){
            return pProxyInfo->levelcmd[i].level;
        }
    }
    return 0;
}
int GetInterfaceName(char *interfacname)
{
	int findname = 0;
	FILE *fp;
	if ((fp = fopen(MysqlConfigPath, "r")) != NULL) {
		while (!feof(fp)) {
			char logcommand[255] = {0};
			memset(logcommand, 0, sizeof(logcommand));
			if (fgets(logcommand, 255, fp) == NULL) {
				break;
			} else {
				int i = strlen(logcommand);
				if (logcommand[i - 1] == '\n' || logcommand[i - 1] == '\r') {
					logcommand[i - 1] = 0;
				}
				if (logcommand[i - 2] == '\r') {
					logcommand[i - 2] = 0;
				}
				if (strstr(logcommand, "interfacename:=") != NULL) {
					strcpy(interfacname, logcommand + strlen("interfacename:="));
					findname = 1;
					break;
				}
			}
		}
		fclose(fp);
	} 
	if(findname == 0){
		strcpy(interfacname, "eth0");
	}
	return 0;
}
void
GetIp() {
    struct ifreq req;
    int sock;
    char interfacename[100] = {0};
    GetInterfaceName(interfacename);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    strncpy(req.ifr_name, interfacename, IFNAMSIZ);
    if (ioctl(sock, SIOCGIFADDR, &req) < 0) {
        printdblog("ioctl error: %s\n", strerror(errno));
        return NULL;
    }
    sprintf(proxyIP, "%s", (char *) inet_ntoa(*(struct in_addr *) &((struct sockaddr_in *) &req.ifr_addr)->sin_addr));
    shutdown(sock, 2);
    close(sock);

    printdblog("%s:%s",interfacename,proxyIP);
}

int GetConfigForUser(PPROXYINFO pProxyInfo, int pr_accessType) {
    GetIp();
    printdblog("current ip address: %s\n", proxyIP);
    memset(&pProxyInfo->auth, 0, sizeof (Pr_AuthAccessRes));
    
    int rtn = 4;//GetAuth_uid(pProxyInfo->inputName, pProxyInfo->serverIP, pProxyInfo->serverName, &pProxyInfo->auth, pr_accessType);
    if (rtn == 0) {
        memset(&pProxyInfo->employee, 0, sizeof (Pr_Employee));
        if (GetEmployee(pProxyInfo->auth.pr_userId, &pProxyInfo->employee) != 0) {
            printdblog("Manual param error\n");
            return 0;
        }
        if (strstr(pProxyInfo->inputPwd, pProxyInfo->employee.pr_pwd) == NULL) {
            printdblog("Manual param error\n");
            return 0;
        }
        memset(&pProxyInfo->tableId, 0, sizeof (JOINTableID));
        if (GetJOINTable(pProxyInfo->inputName, pProxyInfo->serverIP, pProxyInfo->serverName, pr_accessType, &pProxyInfo->tableId, &pProxyInfo->table_count) != 0) {
            printdblog("Manual param error\n");
            return 0;
        }

        memset(&pProxyInfo->policy, 0, sizeof (Policy));

        int right = 0;
        int nclright = 0;
        int pclright = 0;
        int tclright = 0;
        int findRes = 0;
        int isResource = 0;
        int findPolicy = 0;
        int i = 0;
        for (i = 0; i < pProxyInfo->table_count; i++) {
            isResource = findRes;
            if (GetPolicy(&pProxyInfo->tableId[i], &pProxyInfo->policy[i], &isResource) != 0) {
                continue;
            }
            findPolicy = 1;
            if (isResource == 1) {
                findRes = 1;
            } else if (findRes == 1) {
                break;
            }

            if (MatchNclItem(&pProxyInfo->policy[i], pProxyInfo->clientIP) == 0) {
                nclright = 1;
            }
            if (MatchPclItem(&pProxyInfo->policy[i], pr_accessType) == 0) {
                pclright = 1;
            }
            if (MatchTclItem(&pProxyInfo->policy[i]) == 0) {
                tclright = 1;
            }
        }
        if ((nclright == 0 || pclright == 0 || tclright == 0) && findPolicy == 1) {
            printdblog("Policy error\n");
            return 0;
        }

        strcpy(pProxyInfo->serverPwd, pProxyInfo->auth.pr_accountPwd);
    } else if (rtn == 4) {
        memset(&pProxyInfo->tableId, 0, sizeof (JOINTableID));
        if (GetJOINTable(pProxyInfo->inputName, pProxyInfo->serverIP, pProxyInfo->serverName, pr_accessType, &pProxyInfo->tableId, &pProxyInfo->table_count) != 0) {
            printdblog("Manual param error\n");
            return 0;
        }

        memset(&pProxyInfo->policy, 0, sizeof (Policy));
        int right = 0;
        int nclright = 0;
        int pclright = 0;
        int tclright = 0;
        int findRes = 0;
        int isResource = 0;
        int findPolicy = 0;
        int i = 0;
        for (i = 0; i < pProxyInfo->table_count; i++) {
            isResource = findRes;
            if (GetPolicy(&pProxyInfo->tableId[i], &pProxyInfo->policy[i], &isResource) != 0) {
                continue;
            }
            findPolicy = 1;
            if (isResource == 1) {
                findRes = 1;
            } else if (findRes == 1) {
                break;
            }

            if (MatchNclItem(&pProxyInfo->policy[i], pProxyInfo->clientIP) == 0) {
                nclright = 1;
            }
            if (MatchPclItem(&pProxyInfo->policy[i], pr_accessType) == 0) {
                pclright = 1;
            }
            if (MatchTclItem(&pProxyInfo->policy[i]) == 0) {
                tclright = 1;
            }
        }
        if ((nclright == 0 || pclright == 0 || tclright == 0) && findPolicy == 1) {
            printdblog("Policy error\n");
            return 0;
        }

        memset(&pProxyInfo->employee, 0, sizeof (Pr_Employee));
        if (GetEmployee(pProxyInfo->tableId[0].pr_employee_id, &pProxyInfo->employee) != 0) {
            printdblog("Manual param error\n");
            return 0;
        }
        if (strstr(pProxyInfo->inputPwd, pProxyInfo->employee.pr_pwd) == NULL) {
            printdblog("Manual param error\n");
            return 0;
        }

        memset(&pProxyInfo->resource, 0, sizeof (Pr_Resource));
        if (GetResource(pProxyInfo->tableId[0].pr_resource_id, pProxyInfo->serverName, &pProxyInfo->resource) != 0) {
            printdblog("Manual param error\n");
            return 0;
        }
        strcpy(pProxyInfo->serverPwd, pProxyInfo->resource.pr_passwd);
        memset(&pProxyInfo->employeeaccount, 0, sizeof (Pr_Employeeaccount));
        if (GetEmployeeaccount(pProxyInfo->tableId[0].pr_employeeaccount_id, &pProxyInfo->employeeaccount) != 0) {
            printdblog("Manual param error\n");
            return 0;
        }

        //strcpy(pProxyInfo->auth.pr_id,row[0]);
        strcpy(pProxyInfo->auth.pr_userId, pProxyInfo->employee.pr_id);
        strcpy(pProxyInfo->auth.pr_uid, pProxyInfo->employee.pr_uid);
        strcpy(pProxyInfo->auth.pr_userName, pProxyInfo->employee.pr_userName);
        strcpy(pProxyInfo->auth.pr_resId, pProxyInfo->resource.pr_id);
        strcpy(pProxyInfo->auth.pr_resAddr, pProxyInfo->resource.pr_address);
        strcpy(pProxyInfo->auth.pr_resName, pProxyInfo->resource.pr_name);
        strcpy(pProxyInfo->auth.pr_resType, pProxyInfo->employeeaccount.pr_resType);
        strcpy(pProxyInfo->auth.pr_resVersion, ""); //not found
        strcpy(pProxyInfo->auth.pr_accountId, pProxyInfo->employeeaccount.pr_accountId);
        strcpy(pProxyInfo->auth.pr_accountNo, pProxyInfo->employeeaccount.pr_accountNo);
        pProxyInfo->auth.pr_accountType = 1; //not found
        strcpy(pProxyInfo->auth.pr_accountPwd, pProxyInfo->employeeaccount.pr_accountPwd);
        pProxyInfo->auth.pr_suFlag = (pProxyInfo->resource.pr_useSuDo);
        pProxyInfo->auth.pr_ssoFlag = (pProxyInfo->resource.pr_ssoFlag);
        pProxyInfo->auth.pr_auditType=1;
        pProxyInfo->auth.pr_auditFlag=1;
        //pProxyInfo->auth.pr_auditFlag = (pProxyInfo->resource.pr_auditFlag);
        //pProxyInfo->auth.pr_auditType = (pProxyInfo->resource.pr_auditType);
        pProxyInfo->auth.pr_cmdFlag = 1; //not found
        if (pProxyInfo->clientIP != NULL)
            strcpy(pProxyInfo->auth.pr_clientAddr, pProxyInfo->clientIP);
        strcpy(pProxyInfo->auth.pr_apId, pProxyInfo->tableId[0].pr_accessprotocol_id);
        pProxyInfo->auth.pr_accessType = pr_accessType;
        strcpy(pProxyInfo->auth.pr_accountNoEx, ""); //not found
        strcpy(pProxyInfo->auth.pr_accountPwdEx, ""); //not found


        strcpy(pProxyInfo->auth.pr_domainName, ""); //not found
        pProxyInfo->auth.pr_screenWidth = 0; //not found
        pProxyInfo->auth.pr_screenHeight = 0; //not found
        strcpy(pProxyInfo->auth.pr_proxyAddr, proxyIP);
        //pProxyInfo->auth.pr_endTime=NULL;
        //pProxyInfo->auth.pr_resetPwdTime=NULL;
        pProxyInfo->auth.pr_timeFlag = 0;
        strcpy(pProxyInfo->auth.pr_execApp, "");
        strcpy(pProxyInfo->auth.pr_execParam, "");

       /* if (SetAuth(&pProxyInfo->auth) != 0) {
            printdblog("Manual param error\n");
            return 0;
        }*/

    } else {
        printdblog("Manual param error\n");
        return 0;
    }
    return 1;
}

int GetConfigForID(PPROXYINFO pProxyInfo,int pr_accessType) {
    memset(&pProxyInfo->auth, 0, sizeof (Pr_AuthAccessRes));
    if (GetAuth_id(pProxyInfo->pr_id, &pProxyInfo->auth) != 0) {
        printdblog("DBRecord-DBoperate.c[GetConfigForID] auto param error\n");
        return 0;
    }
        strcpy(pProxyInfo->inputName, pProxyInfo->auth.pr_uid);
        strcpy(pProxyInfo->inputPwd, "");
        strcpy(pProxyInfo->serverIP, pProxyInfo->auth.pr_resAddr);
        pProxyInfo->serverPort = 0;
        strcpy(pProxyInfo->serverName,pProxyInfo->auth.pr_accountNo);
        
    memset(&pProxyInfo->employee, 0, sizeof (Pr_Employee));
    if (GetEmployee(pProxyInfo->auth.pr_userId, &pProxyInfo->employee) != 0) {
        printdblog("Manual param error\n");
        return 0;
    }
    if(GetResourcePort(pProxyInfo->auth.pr_resId,pr_accessType,&pProxyInfo->serverPort ) != 0){
        printdblog("Manual param error\n");
        return 0;
    }
    /*if (strstr(pProxyInfo->inputPwd, pProxyInfo->employee.pr_pwd) == NULL) {
        printdblog("Manual param error\n");
        return 0;
    }*/
    memset(&pProxyInfo->tableId, 0, sizeof (JOINTableID));
    if (GetJOINTable(pProxyInfo->inputName, pProxyInfo->serverIP, pProxyInfo->serverName, pr_accessType, &pProxyInfo->tableId, &pProxyInfo->table_count) != 0) {
        printdblog("Manual param error\n");
        return 0;
    }

    memset(&pProxyInfo->policy, 0, sizeof (Policy));
    int right = 0;
    int nclright = 0;
    int pclright = 0;
    int tclright = 0;
    int findRes = 0;
    int isResource = 0;
    int findPolicy = 0;
    int i = 0;
    for (i = 0; i < pProxyInfo->table_count; i++) {
        isResource = findRes;
        if (GetPolicy(&pProxyInfo->tableId[i], &pProxyInfo->policy[i], &isResource) != 0) {
            continue;
        }
        findPolicy = 1;
        if (isResource == 1) {
            findRes = 1;
        } else if (findRes == 1) {
            break;
        }

        if (MatchNclItem(&pProxyInfo->policy[i], pProxyInfo->clientIP) == 0) {
            nclright = 1;
        }
        if (MatchPclItem(&pProxyInfo->policy[i], pr_accessType) == 0) {
            pclright = 1;
        }
        if (MatchTclItem(&pProxyInfo->policy[i]) == 0) {
            tclright = 1;
        }
    }
    if ((nclright == 0 || pclright == 0 || tclright == 0) && findPolicy == 1) {
        printdblog("Policy error\n");
        return 0;
    }

    strcpy(pProxyInfo->serverPwd, pProxyInfo->auth.pr_accountPwd);
    return 1;
}
int CreateLoginlog(PPROXYINFO pProxyInfo){

    Pr_Loginlog loginlog;
    memset(&loginlog, 0, sizeof (Pr_Loginlog));
    strcpy(loginlog.pr_id, "");
    strcpy(loginlog.pr_uid, pProxyInfo->auth.pr_uid);
    strcpy(loginlog.pr_userName, pProxyInfo->auth.pr_userName);
    //loginlog.pr_orgId[32],
    //loginlog.pr_orgName[80],
    strcpy(loginlog.pr_accountNo, pProxyInfo->auth.pr_accountNo);
    strcpy(loginlog.pr_clientAddr, pProxyInfo->auth.pr_clientAddr);
    strcpy(loginlog.pr_resAddr, pProxyInfo->auth.pr_resAddr);
    strcpy(loginlog.pr_resId, pProxyInfo->auth.pr_resId);
    strcpy(loginlog.pr_resName, pProxyInfo->auth.pr_resName);
    strcpy(loginlog.pr_resType, pProxyInfo->auth.pr_resType);
    strcpy(loginlog.pr_apId, pProxyInfo->auth.pr_apId);
    loginlog.pr_accessType = pProxyInfo->auth.pr_accessType;
    strcpy(loginlog.pr_loginDate, "");
    strcpy(loginlog.pr_loginOutDate, "");
    strcpy(loginlog.pr_onLineTime, "");
    strcpy(loginlog.pr_proxyAddr, pProxyInfo->auth.pr_proxyAddr);
    loginlog.pr_port =pProxyInfo->clientPort;
    loginlog.pr_videoFlag = pProxyInfo->auth.pr_auditType;
    loginlog.pr_cmdFlag = pProxyInfo->auth.pr_auditFlag;
    strcpy(loginlog.pr_logRecord, "");
    loginlog.pr_status = 1;
    loginlog.pr_nodeFlag = 1;
    //loginlog.pr_auditUserId[32]      
    strcpy(loginlog.pr_authaccessresid,pProxyInfo->auth.pr_id);
    strcpy(loginlog.pr_proxyHost,pProxyInfo->auth.pr_proxyHost);
    strcpy(loginlog.pr_cooperator,pProxyInfo->auth.pr_cooperator);
    strcpy(loginlog.pr_accessMemo,pProxyInfo->auth.pr_accessMemo);

    SetLoginlog(loginlog, pProxyInfo->loginlog_id);    
}

int GetPrAccessProtocol(int pr_code,PPr_AccessProtocol pAccessProtocol){
    if(strcmp(dbtype,"MYSQL") != 0){
	    return OGetPrAccessProtocol(pr_code,pAccessProtocol);
    }
    char sql[1024];
    MYSQL_RES *res;
    MYSQL_ROW row;
    int num;
    int rtn = 0;

    if (pAccessProtocol == NULL) {
        return 1;
    }

    memset(sql, 0, sizeof (sql));
    sprintf(sql, "select * from pr_accessprotocol where pr_code=%d", pr_code);
    printdblog(sql);
    printdblog("\n");

    if (mysql_query(mysql_sock, sql)) {
        return 2;
    }
    if (!(res = mysql_store_result(mysql_sock))) {
        return 3;
    }
    num = mysql_num_rows(res);
    if (0 < num && (row = mysql_fetch_row(res))) {
        
        if (row[0] != NULL)
            strcpy(pAccessProtocol->pr_id, row[0]);
        if (row[1] != NULL)
            strcpy(pAccessProtocol->pr_name, row[1]);
        if (row[2] != NULL)
            pAccessProtocol->pr_code = atoi(row[2]);
        if (row[3] != NULL)
            pAccessProtocol->pr_status = atoi(row[3]);
        if (row[4] != NULL)
            pAccessProtocol->pr_order = atoi(row[4]);
        if (row[5] != NULL)
            strcpy(pAccessProtocol->pr_port, row[5]);
        if (row[6] != NULL)
            strcpy(pAccessProtocol->pr_iconPath, row[6]);
        

    } else {
        rtn = 4;
    }
    mysql_free_result(res);
    return rtn;
}

typedef unsigned char BYTE;
void _decrypt_key(BYTE *input_asc,int len,BYTE *output_asc,BYTE *key,int mode_type);
const char const_key[] = "Py*8Z)F92k&H)W@N";
const char aes_const_key[]="P$A8(r3E";
char aes_key[128] = {0};
int GetAESKey(){
    if(strcmp(dbtype,"MYSQL") != 0){
	    return OGetAESKey();
    }
    char sql[1024];
    char pr_key[1024];
    char pr_basekey[1024];
    MYSQL_RES *res;
    MYSQL_ROW row;
    int num;
    int rtn = 0;
    
    memset(sql, 0, sizeof (sql));
    memset(pr_key, 0, sizeof (pr_key));
    memset(pr_basekey, 0, sizeof (pr_basekey));
    sprintf(sql, "SELECT pr_key,pr_baseKey from pr_productkey");
    printdblog(sql);
    printdblog("\n");
    if (mysql_query(mysql_sock, sql)) {
        return 2;
    }
    if (!(res = mysql_store_result(mysql_sock))) {
        return 3;
    }
    num = mysql_num_rows(res);
    if (num > 0) {
        if (row = mysql_fetch_row(res)) {
            strcpy(pr_key , row[0]);
            strcpy(pr_basekey , row[1]);
        }
    }
    mysql_free_result(res);
    
    char kekey[1024]={0};
    char allkey[1024]={0};
    memset(kekey,0,sizeof(kekey));
    memset(allkey,0,sizeof(allkey));
    memset(aes_key,0,sizeof(aes_key));
    _decrypt_key(pr_basekey,strlen(pr_basekey),kekey,const_key,0);
    if(strlen(kekey) != 8)
        return -1;
    sprintf(allkey,"%s%s",aes_const_key,kekey);
    _decrypt_key(pr_key,strlen(pr_key),aes_key,allkey,0);
    
    return rtn;
}

int SetAuthMessage(char *pr_id, char *uid, char *userName, char *resAddr, char *accountNo, char *clientAddr, char *message)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into pr_authmessage(pr_id,pr_uid,pr_userName,pr_resAddr,pr_accountNo,pr_clientAddr,pr_logInfo) \
                                     values (\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\')",pr_id, uid, userName, resAddr, accountNo, clientAddr, message);
    
    if (mysql_query(mysql_sock, sql)) {
        printdblog("insert table pr_authmessage failed %s\n\n", mysql_error(mysql_sock));
        return 2;
    }
    return 0;
}

int DecryptData(char * indata, int len, char *outdata)
{
	if (strlen(indata) % 44 == 0) {
		_encrypt(indata, len, outdata);
	}else if (strlen(aes_key) > 0) {
		_decrypt_key(indata, len, outdata, aes_key, 2);
	} else {
		_encrypt(indata, len, outdata);
	}
	return 0;
}