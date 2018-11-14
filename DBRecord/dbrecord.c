
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <errno.h>

#include <net/if.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "dbrecord.h"
#include "DBoperate.h"
#include <time.h>

char* strings_replace_free(char* pStrings, char* pNeedReplace, char* pNew);
long int find_strings_count(char* pStrings, char* pFindStrings);
char* strings_replace(char *pInput, char *pOutput, char *pSrc, char *pDst) ;
pthread_mutex_t mutex;//线程中的锁
int isMutex = 0;
int isLoginlog = 0;
char loginlog_id[32];

void InitMutex(){
    if(isMutex == 0){
        isMutex = 1;
        pthread_mutex_init(&mutex,NULL);
    }
}


char*
strings_replace_free(char* pStrings, char* pNeedReplace, char* pNew)
 {
    if (strstr(pStrings, pNeedReplace)) {
        long int counts = find_strings_count(pStrings, pNeedReplace);
        int len = strlen(pNew) - strlen(pNeedReplace);
        int strings_len = strlen(pStrings);
        long int malloc_len = strings_len + counts * len + 1;
        char* p_back = (char*) malloc(malloc_len);
        p_back[malloc_len - 1] = '\0';
        p_back = strings_replace(pStrings, p_back, pNeedReplace, pNew);
        return p_back;
    } else {
		char* p_back = (char*) malloc(strlen(pStrings)+1);
		memset(p_back,0,strlen(pStrings)+1);
		strcpy(p_back,pStrings);
        return p_back;
    }
} /* ----- end of function strings_replace ----- */

long int
find_strings_count(char* pStrings, char* pFindStrings) {
    long int n_counts = 0;
    long int n_lengs = strlen(pFindStrings);
    char* p_head = pStrings;
    char* p_point = pStrings;
    while (1) {
        p_point = strstr(p_head, pFindStrings);
        if (p_point != NULL) {
            n_counts++;
        } else {
            break;
        }
        p_head = p_point + n_lengs;
    }
    return n_counts;
} /* ----- end of function calu_strings_num ----- */

char*
strings_replace(char *pInput, char *pOutput, char *pSrc, char *pDst) {
    char* point = NULL;
    char* input_head = pInput;
    char* output_head = pOutput;
    int lens = 0;
    int src_lens = strlen(pSrc);
    int dst_lens = strlen(pDst);
    
    while (1) {
        point = strstr(input_head, pSrc);
        if (point != NULL) {
            lens = (int) (point - input_head);
        } else {
            int len = (int) (&pInput[strlen(pInput)] - input_head);
            memcpy(output_head, input_head, len);
            break;
        }
        memcpy(output_head, input_head, lens);
        input_head = input_head + lens + src_lens;
        output_head = output_head + lens;
        memcpy(output_head, pDst, dst_lens);
        output_head = output_head + dst_lens;
    }
    return pOutput;
}

//username 
//userName@password@destAddr@destPort@destAccount@accessID

int GetConfig(const char * username, PPROXYINFO pProxyInfo, int protocol) {
    char seps[] = "#";
    char param1[DIR_MAXDEPTH + 5];
    char param2[DIR_MAXDEPTH + 5];
    char param3[DIR_MAXDEPTH + 5];
    char param4[DIR_MAXDEPTH + 5];
    char param5[DIR_MAXDEPTH + 5];
    char param6[DIR_MAXDEPTH + 5];
    char *token;
    int n = 0;
    int i = 0;
    int right = 0;
    int nclright = 0;
    int pclright = 0;
    int tclright = 0;
    int rtn = 1;
    int id = 0;
    //HOST_USER host_user;

    if (username == NULL) {
        return 0;
    }

    InitMutex();
    pthread_mutex_lock(&mutex);
    token = strtok(username, seps);
    while (token != NULL) {
        if (n == 0) {
            memset(param1, 0, sizeof (param1));
            strcpy(param1, token);
            printdblog("param1 %s\n", param1);
        } else if (n == 1) {
            memset(param2, 0, sizeof (param2));
            strcpy(param2, token);
            printdblog("param2 %s\n", param2);
        } else if (n == 2) {
            memset(param3, 0, sizeof (param3));
            strcpy(param3, token);
            printdblog("param3 %s\n", param3);
        } else if (n == 3) {
            memset(param4, 0, sizeof (param4));
            strcpy(param4, token);
            printdblog("param4 %s\n", param4);
        } else if (n == 4) {
            memset(param5, 0, sizeof (param5));
            strcpy(param5, token);
            printdblog("param5 %s\n", param5);
        } else if (n == 5) {
            memset(param6, 0, sizeof (param6));
            strcpy(param6, token);
            printdblog("param6 %s\n", param6);
        } 
        n++;
        token = strtok(NULL, seps);
    }
    /*if (n < 5 || n > 6) {
        printdblog("Manual param error\n");
        pthread_mutex_unlock(&mutex);
        return 0;
    }*/

    

    if(ConnectMysql() != 0){
        printdblog("ConnectMysql error\n");
        pthread_mutex_unlock(&mutex);
        return 0;
    }
    GetAESKey();
    if (n == 5) {
        strcpy(pProxyInfo->inputName, param1);
        strcpy(pProxyInfo->inputPwd, param2);
        strcpy(pProxyInfo->serverIP, param3);
        pProxyInfo->serverPort = atoi(param4);
        strcpy(pProxyInfo->serverName,param5);
        pProxyInfo->type = 2;
        memset(&pProxyInfo->auth, 0, sizeof (Pr_AuthAccessRes));
       rtn = GetConfigForUser(pProxyInfo,protocol);
        if(rtn == 0){
            goto exit;
        }
    } else if (n == 1) {
        strcpy(pProxyInfo->pr_id, param1);
        printdblog("DBProxy-dbrecord.c[GetConfig] call GetConfigForID\n");
        rtn = GetConfigForID(pProxyInfo,protocol);
        if(rtn == 0){
            goto exit;
        }
    } else {
        printdblog("param count error\n");
        CloseDB();
        pthread_mutex_unlock(&mutex);
        return 0;
    }
    GetLeveCmd(pProxyInfo);
    if(isLoginlog==0){
        CreateLoginlog(pProxyInfo);
        memset(loginlog_id,0,sizeof(loginlog_id));
        strcpy(loginlog_id,pProxyInfo->loginlog_id);
        isLoginlog = 1;
    }else{
	    strcpy(pProxyInfo->loginlog_id,loginlog_id);
    }
exit:
    CloseDB();
    pthread_mutex_unlock(&mutex);
    return rtn;
}

int InsertUseractivityoutlog(PPROXYINFO pProxyInfo) {
    Pr_Useractivityoutlog useroutlog;
    memset(&useroutlog, 0, sizeof (useroutlog));
    strcpy(useroutlog.pr_id, "");
    strcpy(useroutlog.pr_ualId, pProxyInfo->userlog_id);
    if (pProxyInfo->lineOutLen >= 1024 * 4 ) {
        pProxyInfo->lineOutBuffer[1024 * 4-1] = 0;
    }
    strcpy(useroutlog.pr_desc, pProxyInfo->lineOutBuffer);
    strcpy(useroutlog.pr_operDate, "");
    strcpy(useroutlog.pr_seesionId, pProxyInfo->loginlog_id);
    if(pProxyInfo->auth.pr_auditFlag == 0){
        printdblog("not audit cmd out\n");
    }else{
        SetUseractivityoutlog(useroutlog,1);
    }
    pProxyInfo->lineOutLen = 0;
    memset(pProxyInfo->lineOutBuffer, 0, sizeof (pProxyInfo->lineOutBuffer));
    return 0;
}

int InsertUseractivitylog(PPROXYINFO pProxyInfo) {
    Pr_Useractivitylog userlog;
     int i = 0;
    pProxyInfo->start_in = 1;
    char * cmd [BUFSIZE];
    memset(cmd,0,sizeof(cmd));
    char * sql = strings_replace_free(pProxyInfo->lineOutBuffer,"'","\"");
    if(sql != NULL){
        strcpy(cmd,sql);
        free(sql);
    }else{
        strcpy(cmd,pProxyInfo->lineOutBuffer);
    }
   
    for( i = 0 ; i < strlen(cmd) ; i ++){
	    if((unsigned char)cmd[i] == 0x3f){
		    cmd[i] = 0x20;
	    }
    }
    printdblog("cmd:%s*\n", cmd);
    if(strlen(cmd) >= sizeof(pProxyInfo->cmd)){
        memcpy(pProxyInfo->cmd,cmd,sizeof(pProxyInfo->cmd)-1);
    }else{
        strcpy(pProxyInfo->cmd, cmd);
    }
  //  if (MatchCclItem(&pProxyInfo->policy, cmd, pProxyInfo->auth.pr_cmdFlag) != 0) {
 //       pProxyInfo->level = 1;
//    }
    
    for (i = 0; i < pProxyInfo->table_count; i++) {
        if (MatchCclItem(&pProxyInfo->policy[i], cmd, pProxyInfo->auth.pr_cmdFlag) != 0) {
	    pProxyInfo->level = 1;
	    printdblog("cmd:%s*\n", cmd);
            break;
        }
    }
    //ReflashLoginlog(pProxyInfo->loginlog_id, 1);

    memset(&userlog, 0, sizeof (Pr_Useractivitylog));
    strcpy(userlog.pr_id, "");
    strcpy(userlog.pr_clientAddr, pProxyInfo->auth.pr_clientAddr);
    strcpy(userlog.pr_proxyAddr, pProxyInfo->auth.pr_proxyAddr);
    strcpy(userlog.pr_resId, pProxyInfo->auth.pr_resId);
    strcpy(userlog.pr_resName, pProxyInfo->auth.pr_resName);
    strcpy(userlog.pr_resAddr, pProxyInfo->auth.pr_resAddr);
    strcpy(userlog.pr_command, pProxyInfo->cmd);
    strcpy(userlog.pr_cmdPrefix, pProxyInfo->precmd);
    strcpy(userlog.pr_orgId, "");
    strcpy(userlog.pr_orgName, "");
    strcpy(userlog.pr_userId, pProxyInfo->auth.pr_userId);
    strcpy(userlog.pr_userName, pProxyInfo->auth.pr_userName);
    strcpy(userlog.pr_uid, pProxyInfo->auth.pr_uid);
    strcpy(userlog.pr_accountNo, pProxyInfo->auth.pr_accountNo);
    strcpy(userlog.pr_operDate, "");
    strcpy(userlog.pr_sessionId, pProxyInfo->loginlog_id);
    userlog.pr_level =  FindLevelCmd(pProxyInfo,pProxyInfo->cmd);
    userlog.pr_accessType = pProxyInfo->auth.pr_accessType;
    if(pProxyInfo->auth.pr_auditFlag == 0){
        printdblog("not audit cmd\n");
    }else{       
        SetUseractivitylog(userlog, pProxyInfo->userlog_id);
    }
    
    return 0;
}

//2016-07-14 add
int InsertStopExchangeUserInfo(PPROXYINFO pProxyInfo)
{
    Pr_Useractivitylog userlog;
    char msg[100] = {0};

    memset(&userlog, 0, sizeof (Pr_Useractivitylog));
    strcpy(userlog.pr_id, "");
    strcpy(userlog.pr_clientAddr, pProxyInfo->auth.pr_clientAddr);
    strcpy(userlog.pr_proxyAddr, pProxyInfo->auth.pr_proxyAddr);
    strcpy(userlog.pr_resId, pProxyInfo->auth.pr_resId);
    strcpy(userlog.pr_resName, pProxyInfo->auth.pr_resName);
    strcpy(userlog.pr_resAddr, pProxyInfo->auth.pr_resAddr);
    sprintf(msg, "username:%s",pProxyInfo->auth.pr_accountNo);
    strcpy(userlog.pr_command, msg);
    strcpy(userlog.pr_cmdPrefix, "prevent exchange user!");
    strcpy(userlog.pr_orgId, "");
    strcpy(userlog.pr_orgName, "");
    strcpy(userlog.pr_userId, pProxyInfo->auth.pr_userId);
    strcpy(userlog.pr_userName, pProxyInfo->auth.pr_userName);
    strcpy(userlog.pr_uid, pProxyInfo->auth.pr_uid);
    strcpy(userlog.pr_accountNo, pProxyInfo->auth.pr_accountNo);
    strcpy(userlog.pr_operDate, "");
    strcpy(userlog.pr_sessionId, pProxyInfo->loginlog_id);
    userlog.pr_level =  1;
    userlog.pr_accessType = pProxyInfo->auth.pr_accessType;
    if(pProxyInfo->auth.pr_auditFlag == 0){
        printdblog("not audit cmd\n");
    }else{       
        SetUseractivitylog(userlog, pProxyInfo->userlog_id);
    }

    memset(pProxyInfo->lastprecmd, 0, sizeof (pProxyInfo->lastprecmd));
    memset(pProxyInfo->cmd, 0, sizeof (pProxyInfo->cmd));
    memset(pProxyInfo->precmd, 0, sizeof (pProxyInfo->precmd));

    pProxyInfo->lineOutLen = 0;
    memset(pProxyInfo->lineOutBuffer, 0, sizeof (pProxyInfo->lineOutBuffer));
    return 0;
}

int GetStopExchangeUserSwitch(char isStop[]) 
{
    FILE *fp;

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
                if (strstr(logcommand, "stopchangeuser:=") != NULL) {
                    strcpy(isStop, logcommand + strlen("stopchangeuser:="));
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
//2016-07-14

int UserLogout(PPROXYINFO pProxyInfo){
    //ReflashLoginlog(pProxyInfo->loginlog_id, 2);
    return 0;
}
int GetAccessProtocol(int pr_code,PPr_AccessProtocol pAccessProtocol){
    InitMutex();
    pthread_mutex_lock(&mutex);
    if(ConnectMysql() != 0){
        printdblog("ConnectMysql error\n");
        pthread_mutex_unlock(&mutex);
        return 1;
    }
    int rtn = GetPrAccessProtocol(pr_code,pAccessProtocol);
    
    CloseDB();
    pthread_mutex_unlock(&mutex);
    return rtn;
}

int findSQL(char * data, int len,char * sql){
    int isfind = -1;
    int i = 0 ,j = 0; 
    printdblog("findSQL:start\n");
    for(i = 0 ; i < len ; i ++){
        if(data[i] == sql[0]){
            for(j = 0 ; j < strlen(sql) ; j ++){
                if(data[i+j] != sql[j])
                    break;
            }
            isfind = i;
            if(isfind > 5){
                break;
            }
        }
    }
    if(isfind >= 0){
        printdblog("findSQL:replace\n");
        data[isfind] = '%';
    }
    return isfind ;
}