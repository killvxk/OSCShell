/* 
 * File:   oraclefilter.c
 * Author: Administrator
 *
 * Created on 2012年2月20日, 上午9:42
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include "oraclefilter.h"
//#include "../DBRecord/dbrecord.h"
//#include "../DBFilter/work.h"

/*
 *	由于没有进行TCP流重组, 所以行类型和标志位可能会发生错误
 */
char pszRowStr[64][1024*1]; //行字符串
int pRowStrLen[64]; //行字符串
unsigned char pszFieldType[20]; //行字符串
unsigned int nMaxColNum = 0; //字段数
unsigned int nGlobMaxColNum = 0; //字段数
unsigned char szOldCol[128]; //上一行的列标志字符串
char sql_reslut[4096]={0};
int iversion = 0;
int isquery = 0;
int isjdbc=0;
int istoad = 0;
int isplsql = 0;
int bitversion = 32;
int oracleFlag = 0;

int print2log(const char* format, ...);
int getNewProt();

int printreslut( const char* format, ...) {
    static char sprint_buf[4096];
    va_list args;
    int n;


    va_start(args, format);
    n = vsprintf(sprint_buf, format, args);
    va_end(args);

     if(strlen(sql_reslut) >= sizeof(sql_reslut)){
         return 0;
     }else if(strlen(sql_reslut) + strlen(sprint_buf) >= sizeof(sql_reslut)){
        sprint_buf[sizeof(sql_reslut)-strlen(sql_reslut)] = 0;
        strcat(sql_reslut,sprint_buf);
    }else{
        strcat(sql_reslut,sprint_buf);
    }

    return 0;
}


void PrintfNum(unsigned char uNum) {
    if (uNum >= 0 && uNum < 100) {
        printreslut("%02d", uNum);
    } else {
    }
}

void PrintData(unsigned char utype, unsigned char *pszData, int len) {
    int i = 0;
    if(len == 0){
        printreslut("NULL|");
        return;
    }
    if (utype == 0x00) //当成字符串处理
    {
        printreslut("%s|", pszData);
    }else if(utype == 0x0b||utype == 0xd0){
        printreslut("ROWID|");
    } else if (utype == 01)//varchar2 or nvarchar2
    {
        for(i=0;i<len;i++){
            if(*(pszData+i) != 0)
                printreslut("%c", *(pszData+i));
        }
        printreslut("|");
    } else if (utype == 96)//char or nchar
    {
        for(i=0;i<len;i++){
            if(*(pszData+i) != 0)
                printreslut("%c", *(pszData+i));
        }
        printreslut("|");
    }else if(utype == 0x65){ //biner_double
        if(len == 8){
            double var = 0;
            
            memcpy(((char*)&var),pszData+7,1);
            memcpy(((char*)&var)+1,pszData+6,1);
            memcpy(((char*)&var)+2,pszData+5,1);
            memcpy(((char*)&var)+3,pszData+4,1);
            memcpy(((char*)&var)+4,pszData+3,1);
            memcpy(((char*)&var)+5,pszData+2,1);
            memcpy(((char*)&var)+6,pszData+1,1);
            memcpy(((char*)&var)+7,pszData,1);
           
            if(iversion == 11 || bitversion == 64){
                if(*pszData >= 0x80){
                    int v = (int)*(((char*)&var)+7);
                    v = v - 0x80;
                    *(((char*)&var)+7) = (char)v;
                }else{
                    int i = 0;
                    for(i = 0 ; i < 8 ; i ++){
                        int v = (int)*(((char*)&var)+i);
                        v = 0xff - v;
                        *(((char*)&var)+i) = (char)v;
                    }
                }
            }

            printreslut("%f|", var);
        }
    }else if(utype == 0x64){//biner_float
          if(len == 4){
            float var = 0;
            memcpy(((char*)&var),pszData+3,1);
            memcpy(((char*)&var)+1,pszData+2,1);
            memcpy(((char*)&var)+2,pszData+1,1);
            memcpy(((char*)&var)+3,pszData,1);

            if(iversion == 11|| bitversion == 64){
                if(*pszData >= 0x80){
                    int v = (int)*(((char*)&var)+3);
                    v = v - 0x80;
                    *(((char*)&var)+3) = (char)v;
                }else{
                    int i = 0;
                    for(i = 0 ; i < 4 ; i ++){
                        int v = (int)*(((char*)&var)+i);
                        v = 0xff - v;
                        *(((char*)&var)+i) = (char)v;
                    }
                }
            }
            printreslut("%f|", var);
        }
    }else if(utype == 0xb6){ //INTERVAL YEAR(2) TO MONTH
        if(len == 5){
            
            int day = 0;
            memcpy(((char*)&day),pszData+3,1);
            memcpy(((char*)&day)+1,pszData+2,1);
            memcpy(((char*)&day)+2,pszData+1,1);
            memcpy(((char*)&day)+3,pszData,1);
             *(((char*)&day)+3) = *(((char*)&day)+3)-0x80;
            
            int month = 0x3c-*(pszData+4);

            
            printreslut("%d-%d|", day,month);
        }else{
            printreslut("NULL|");
        }
    }else if(utype == 0xb7){ //INTERVAL DAY(2) TO SECOND(6)
        if(len == 11){
            
            int day = 0;
            memcpy(((char*)&day),pszData+3,1);
            memcpy(((char*)&day)+1,pszData+2,1);
            memcpy(((char*)&day)+2,pszData+1,1);
            memcpy(((char*)&day)+3,pszData,1);
             *(((char*)&day)+3) = *(((char*)&day)+3)-0x80;
            
            int hour = *(pszData+4)-0x3c;
            int minute = *(pszData+5)-0x3c;
            int second = *(pszData+6)-0x3c;
            int msecond = 0;
            
            memcpy(((char*)&msecond),pszData+10,1);
            memcpy(((char*)&msecond)+1,pszData+9,1);
            memcpy(((char*)&msecond)+2,pszData+8,1);
            memcpy(((char*)&msecond)+3,pszData+7,1);
            *(((char*)&msecond)+3) = *(((char*)&msecond)+3)-0x80;
            
            printreslut("%d %d:%d:%d:%d|", day,hour,minute,second,msecond);
        }else{
            printreslut("NULL|");
        }
    } else if (utype == 02)//number
    {
        unsigned char uCurChar = *pszData;
        if (uCurChar < 0x3f) //负数
        {
            int intlen = 63 - uCurChar; //0x3f;
            int i = 0;
            int nonzero = 0;
            
            for(i = 0 ; i < len ; i ++){
                if(*(pszData+1) != 0){
                    nonzero = 1;
                    break;
                }
            }
            if(nonzero == 0){
                printreslut("NULL|");
                return;
            }

            if (len - 2 > 0) {
                printreslut("-");
            }
            i = 1;
            while (i < len - 1) {
                if ((i - 1) == intlen) //有小数位
                {
                    if (intlen == 0) {
                        printreslut("0");
                    }
                    printreslut(".");
                }

                uCurChar = *(pszData + i);
                uCurChar = 101 - uCurChar;


                if (i == 1 && i < intlen) {
                    printreslut("%d", uCurChar);
                } else {
                    PrintfNum(uCurChar);
                }

                i++;
            }

            if (len - 2 > 0 && intlen > len - 2) //没有小数位,且整数后少2*(intlen+1-len)个零
            {
                uCurChar = *pszData;
                for (i = 0; i < intlen + 2 - len; i++) {
                    printreslut("00");
                }
            }
            printreslut("|");
        } else if (uCurChar >= 0xc0) //正数
        {
            int intlen = uCurChar - 192; //0xc0;
            int i = 0;

            i = 1;

            while (i < len) {
                if ((i - 1) == intlen) //有小数位
                {
                    if (intlen == 0) {
                        printreslut("0");
                    }
                    printreslut(".");
                }

                uCurChar = *(pszData + i);
                uCurChar--;

                if (i == 1 && i < intlen) {
                    printreslut("%d", uCurChar);
                } else {
                    PrintfNum(uCurChar);
                }

                i++;
            }

            if (len - 1 > 0 && intlen > len - 1) //没有小数位,且整数后少2*(intlen+1-len)个零
            {
                uCurChar = *pszData;
                for (i = 0; i < intlen + 1 - len; i++) {
                    printreslut("00");
                }
            }
            printreslut("|");
        }else if (uCurChar >= 0x80){ //0
            printreslut("0|");
        }
    } else if (utype == 12)//date
    {
        unsigned char *p = (unsigned char *) pszData;

        if (*p > 100) //year1
        {
            PrintfNum(*p - 100);
            //printf("-");
        }

        p++;
        if (*p > 100) //year2
        {
            PrintfNum(*p - 100);
            printreslut("-");
        }
        p++;
        if (*p > 0) //month
        {
            PrintfNum(*p);
            printreslut("-");
        }
        p++;
        if (*p > 0) //day
        {
            PrintfNum(*p);
            printreslut(" ");
        }
        p++;
        if (*p > 0) //hour
        {
            PrintfNum(*p - 1);
            printreslut(":");
        }
        p++;
        if (*p > 0) //minute
        {
            PrintfNum(*p - 1);
            printreslut(":");
        }
        p++;
        if (*p > 0) //second
        {
            PrintfNum(*p - 1);
        }
        printreslut("|");
    } else if (utype == 0x08 || utype == 0x18) //long /long row
    {
        if(len > 0){
            if(*pszData == 0){
                printreslut("NULL|");
            }else{
                printreslut("%s|", pszData);
            }
        }else{
            printreslut("NULL|");
        }
    } else if (utype == 0x17) //raw
    {
        for(i=0;i<len;i++){
                printreslut("%02X", *(pszData+i));
        }
        printreslut("|");
    }else if(utype == 0xb4 || utype == 0xe7 || utype == 0xb5){//TIMESTAMP(6)   TIMESTAMP(6) WITH LOCAL TIME ZONE
        if(len < 7){
            printreslut("NULL|");
            return ;
        }
        int year = (*(pszData) -100)*100 + *(pszData+1)-100;
        int month = *(pszData+2)-1;
        int day = *(pszData+3)-1;
        if(iversion == 11 || bitversion == 64)
        {
            month+=1;
            day+=1;
        }
        int hour = *(pszData+4)-1;
        int minute = *(pszData+5)-1;
        int second = *(pszData+6)-1;
        int msecond = 0;
        if(len >= 11){
            memcpy(((char*)&msecond),pszData+10,1);
            memcpy(((char*)&msecond)+1,pszData+9,1);
            memcpy(((char*)&msecond)+2,pszData+8,1);
            memcpy(((char*)&msecond)+3,pszData+7,1);
        }
        if(len >= 13){
            int z1 = *(pszData+11) - 0x14;
            int z2 = *(pszData+12) - 0x3c;
            
            hour += z1;
            minute += z2;
            
            if(minute < 0){
                minute += 60;
                hour --;
            }
            if(hour < 0){
                hour += 24;
                day --;
            }
            if(day < 0){
                day += 30;
                month --;
            }
            if(month < 0){
                month += 12;
                year --;
            }
            
            if(z2 < 0){
                z2 = 0 - z2;
            }
            printreslut("%d-%02d-%02d %02d:%02d:%02d:%06d %02d:%02d|", year,month,day,hour,minute,second,msecond,z1,z2);
            
            
        }else{
            printreslut("%d-%02d-%02d %02d:%02d:%02d:%06d|", year,month,day,hour,minute,second,msecond);
        }
        
    } else if (utype == 0x69) //raw id
    {
    } else {
        printreslut("%s|", pszData);
    }
}
unsigned char* FindVer(PPROXYINFO pProxyInfo);

/*
 *	函数名称:   ParseStrbyLen
 *  输    入:   
 *              pszBuf      需要解析的字符串
 *              iBuflen     需要解析的字符串的长度
 *  输    出:
 *              pszSubStr   解析出的字符串
 *              piSubStrLen 解析处的字符串的长度
 *  创建日期:   
 *  作    者:   yangjp
 *  返    回:   长度字段个数和FE格式
 *  描    述:   
 *              ORACLE字符串数据的格式是:
 *              1. 如果字符串前面有0XFE,则后面有几个连续的有长度加该长度的字符串,直到字符串长度为0
 *              2. 如果字符串前面没有0XFE,则后面只有一个长度字段和该长度的字符串字段
 */
int ParseStrbyLen(char *pszBuf, int iBuflen, char *pszSubStr, int *piSubStrLen) {
    char alen = 0;
    unsigned char ilen = 0;
    int iQueryStrPlace = 0;
    char *pszDataTemp = pszBuf;
    int iIsFe = 1; //字符串长度和标志为的总长度

    char dbName[7] = {0};
    memcpy(dbName, pszBuf, 6);
    if (strcmp(dbName, "Oracle") == 0)
    {
        oracleFlag = 1;
    }

    //如果字符串为多个字符串连接块组成,则需要不断解析知道字符串长度为0
    if ((unsigned char) *(pszDataTemp) == 0xfe) {
        pszDataTemp++;
        while (1) {
            memcpy(&alen, pszDataTemp, 1);
            pszDataTemp++;
            ilen = alen;

            if (iBuflen < (iQueryStrPlace + ilen)) {
                *piSubStrLen = 0;
                return -1;
            }
            iIsFe++;
            if (ilen > 0) {
                memcpy(pszSubStr + iQueryStrPlace, pszDataTemp, ilen);
                pszDataTemp += ilen;
                iQueryStrPlace += ilen;
            } else {
                break;
            }
        }
    } else if ((unsigned char) *(pszDataTemp) == 0xff) {
        // iIsFe++;
        //if(iversion == 11){
            pszSubStr = 0xff;
            *piSubStrLen = 0;
            return 1;
        /*}else{
            pszDataTemp++;
            memcpy(pszSubStr, pszDataTemp, 4);
            iQueryStrPlace += 4;
        }*/
    } else if (oracleFlag == 1){
        strcpy(pszSubStr, pszDataTemp);
        iQueryStrPlace += strlen(pszDataTemp);    
    }else {
        memcpy(&alen, pszDataTemp, 1);
        ilen = alen;
        //pszDataTemp++;
        if (ilen > 0 && iBuflen > ilen) {
            memcpy(pszSubStr, pszDataTemp, ilen);
            iQueryStrPlace += ilen;
        } else if (ilen == 0 && iBuflen > ilen) {
            *piSubStrLen = 0;
            return 1;
        } else {
            *piSubStrLen = 0;
            return -1;
        }
    }
    *piSubStrLen = iQueryStrPlace;
    return iIsFe;
}

/***************************************************************
Function:       DecodeTnsData035e
Description:    解析0x035e结构包
Input:                      
                1. PACKET_INFO *pPacketInfo     包相关信息
                2. char *pszSqlQueryData       数据流指针
                3. int iSqlQueryDataLen            数据长度
Output:   
返回查询信息
Return:  
                >0   成功,返回包长度
                <0  失败
Others:         
 ***************************************************************/
int DecodeTnsData0303(char *pszSqlQueryData, int iSqlQueryDataLen, PPROXYINFO pProxyInfo) {
    unsigned int nCurPlace;
    if (iSqlQueryDataLen < sizeof (DATA0303) + 48) {
        return -1;
    }
    char pszQueryStr[40960];
    int iQueryStrLen;

    DATA0303 *pData0303 = (DATA0303 *) pszSqlQueryData;
    nCurPlace = sizeof (DATA0303);

    memset(pszQueryStr, 0, sizeof (pszQueryStr));
    int iRet = ParseStrbyLen(pszSqlQueryData + sizeof (DATA0303), iSqlQueryDataLen - sizeof (DATA0303), pszQueryStr, &iQueryStrLen);

    nCurPlace += (iRet + iQueryStrLen);
if(iQueryStrLen < 4096){
    //printf("Query String: %s\n", pszQueryStr);
    DBOracleOperate(pszQueryStr, pProxyInfo);
}else{
    pszQueryStr[4096] = 0;
    DBOracleOperate(pszQueryStr, pProxyInfo);
    //printf("Query String: %s\n", pszQueryStr);
}
    if(pProxyInfo->level == 1){
        pProxyInfo->level = 0;
                findSQL(pszSqlQueryData + sizeof (DATA0303), iSqlQueryDataLen - sizeof (DATA0303),pszQueryStr);
    }
    return nCurPlace;
}

int DecodeTnsData0376(char *pszSqlQueryData, int iSqlQueryDataLen, PPROXYINFO pProxyInfo) {
    unsigned int nCurPlace = 0;
    if (iSqlQueryDataLen < 29) {
        return -1;
    }
    char pszName[266];
    int iNameLen = 0;
    //2016-07-18 modify 29-->17
    nCurPlace = pszSqlQueryData[17];

    memset(pszName, 0, sizeof (pszName));
    //2016-07-14 modify pszSqlQueryData+30-->pszSqlQueryData+18  nCurPlace-->30
    memcpy(pszName, pszSqlQueryData + 18, nCurPlace);

    
    nCurPlace += 30;

    print2log("LoginName: %s\n", pszName);
    //print2log("");
    //2016-07-14 add
    char isStop[2] = {0};
    GetStopExchangeUserSwitch(isStop);
    if (strcmp(isStop, "1") == 0 && strcmp(pszName, pProxyInfo->auth.pr_accountNo) != 0)
    {
        print2log("clear pszSqlQueryData\n");
        print2log("pszName:%s pProxyInfo->auth.pr_accountNo:%s\n",pszName, pProxyInfo->auth.pr_accountNo);
        //memset(pszSqlQueryData,0,iSqlQueryDataLen);
        print2log("call InsertStopExchangeUserInfo");
        //InsertStopExchangeUserInfo(pProxyInfo);
    }
    //2016-07-14
    //DBMysqlLogin("", pszName, pProxyInfo);
    return nCurPlace;
}

int CheckValidChar(char* pBuf, int iLen) {
    int i = 0;
    for (i = 0; i < iLen; i++) {
        if ((pBuf[i] != 0x0a)
                && (pBuf[i] < 0x20 || pBuf[i] > 126)
                ) {
            return 0;
        }
    }
    return 1;
}

int CheckSqlString(char* pData, int iLen) {
    char alen = 0;
    unsigned char ilen = 0;
    char *pszDataTemp = pData;
    int fRet = 0;
    if ((unsigned char) *(pszDataTemp) == 0xfe) {
        int iQueryStrPlace = 1;
        pszDataTemp++;
        while (1) {
            memcpy(&alen, pszDataTemp, 1);
            pszDataTemp++;
            ilen = alen;

            if (iLen < (iQueryStrPlace + ilen)) {
                return 0;
            }

            if (ilen > 8 && ilen < (iLen - 1 - iQueryStrPlace) && CheckValidChar(pszDataTemp, ilen)) {
                fRet = 1;
                pszDataTemp += ilen;
                iQueryStrPlace += (ilen + 1);
            } else {
                break;
            }
        }
        return fRet;
    }/*    else if((unsigned char)*(pszDataTemp) == 0xff)
    {
       // iIsFe++;
		printf("after 035e is 0xff");
        pszDataTemp++;
        iQueryStrPlace +=4;
		return true;
    }*/
    else {
        memcpy(&alen, pszDataTemp, 1);
        ilen = alen;
        pszDataTemp++;
        if (ilen > 8 && iLen > ilen && CheckValidChar(pszDataTemp, ilen)) {
            //printf("ilen=%d\n",ilen);
            return 1;
        }
    }

    return 0;
}

/***************************************************************
Function:       DecodeTnsData035e
Description:    解析0x035e结构包
Input:                      
                1. PACKET_INFO *pPacketInfo     包相关信息
                2. char *pszSqlQueryData       数据流指针
                3. int iSqlQueryDataLen            数据长度
Output:   
                返回查询信息
Return:  
                >0   成功,返回包长度
                <0  失败
Others:         
 ***************************************************************/
int DecodeTnsData035e(char *pszSqlQueryData, int iSqlQueryDataLen, PPROXYINFO pProxyInfo) {
    //printf("iSqlQueryDataLen=%d,sizeof(035e)=%d\n",iSqlQueryDataLen,sizeof(DATA035e)+48);
#if 1
    int i = 93;
    int j,k;
    nGlobMaxColNum = 0;
    //for (i = 0; i < iSqlQueryDataLen - 1; i++) {
    //    if (CheckSqlString(pszSqlQueryData + i, iSqlQueryDataLen - i)) {
    //log info
    char pszQueryStr[40960];
    int iQueryStrLen = 0;
    
    if (iversion == 9)
        i = 81;
    if (iversion == 11 && istoad == 1){
        i = 93 + 20; //toad have 20 * 0x0
    }
    if (bitversion == 64) {
        i = 54;
        if (iversion == 9)
            i = 48;
        else if (iversion == 11)
        {
            //i = 30;
            i = 28;
            while(1)
            {
                if(pszSqlQueryData[i] == 0x0 && pszSqlQueryData[i+1] == 0x0 && pszSqlQueryData[i+2] == 0x0
                        && pszSqlQueryData[i+3] == 0x0 && pszSqlQueryData[i+4] == 0x0)
                {
                    i+=5;
                    break;
                }
                else
                {
                    i++;
                }
            }
            //add 2015-10-25 by lijy
            i+=21;
        }
        else if(iversion == 10)
        {
            i = 31;
        }
    }
    memset(pszQueryStr, 0, sizeof (pszQueryStr));
    //printf("%02X,%02X,%02X,%02X,%02X,%02X\n",*(pszSqlQueryData+i),*(pszSqlQueryData+i+1),*(pszSqlQueryData+i+2),*(pszSqlQueryData+i+3),*(pszSqlQueryData+i+4),*(pszSqlQueryData+i+5));
    int findfe40 = 0;
    for(j = 0 ; j < iSqlQueryDataLen -1 ; j ++){
        if(*(unsigned char *)(pszSqlQueryData+j) == 0xfe && *(unsigned char *)(pszSqlQueryData+j+1) == 0x40 ){
            findfe40 = j;
            break;
        }
    }
     if(findfe40 > 0){
        findfe40 += 2;
        for(k = 0 ; k < iSqlQueryDataLen-findfe40 ; k ++){
            if(*(unsigned char *)(pszSqlQueryData+findfe40+k) == 0){
                break;
            }
            pszQueryStr[k] = *(pszSqlQueryData+findfe40+k);
            iQueryStrLen = k;
        }
    }else{
        while(iversion == 9 || iversion == 10){//while(iversion == 9)
            int offset = 0;
            char *data = pszSqlQueryData;
            data ++;
            offset ++;
            int len = *(unsigned char *)(data);
            if(len < iSqlQueryDataLen){
                data = data + 1 + len;
                offset = offset + 1 + len;

                if(*(unsigned char *)(data) == 0x00 && *(unsigned char *)(data+1) == 0x01 ){
                    data = data + 2;
                    offset = offset + 2;
                    int sqllen = 0;
                    int sqllentype = *(unsigned char *)(data);
                    data ++;
                    offset ++;
                    if(sqllentype == 1){
                        sqllen = *(unsigned char *)(data);
                        data ++;
                        offset ++;
                    }else if(sqllentype == 2){
                        sqllen = *(unsigned char *)(data) * 256 + *(unsigned char *)(data+1);
                        data = data + 2;
                        offset = offset + 2;
                    }else{
                        break;
                    }
                    
                    data = data + 17;
                    offset = offset + 17;
                    if(iversion == 10 )
                    {
                        if(istoad == 0)
                        {
                            offset+=4;
                            if(isplsql == 1)
                                offset --;
                        }
                    }
                    else if(iversion == 9 && istoad == 0 && isplsql == 0)
                    {
                        offset+=1;
                    }
                    if(*(unsigned char *)(data) == 0x01 && *(unsigned char *)(data+1) == 0x00 ){
                        data = data + 2;
                        offset = offset + 2;
                        if (bitversion == 64 && iversion == 10 && istoad == 1) {//
                            data = data + 3;
                            offset = offset + 3;
                        }
                    }
                    else if(*(unsigned char *)(data) == 0x00 && *(unsigned char *)(data+1) == 0x01 && iversion == 10 && istoad == 1 )
                    {
                        offset = offset + 3;
                    }
                    
                    int sqllenrel = 0;
                    if(sqllentype == 1){
                        sqllenrel = *(unsigned char *)(data);
                        if(sqllenrel <= sqllen){
                            i = offset;
                        }
                    }else if(sqllentype == 2){
                        sqllenrel = *(unsigned char *)(data) * 256 + *(unsigned char *)(data+1);
                        if(sqllenrel == sqllen){
                            i = offset;
                        }
                    }
                }
            }
            break;
        }
        
        
        
        ParseStrbyLen(pszSqlQueryData + i, iSqlQueryDataLen - i, pszQueryStr, &iQueryStrLen);
    }
    if (iQueryStrLen <= 1)
        return iSqlQueryDataLen;

    if (strlen(sql_reslut) > 0) {
        DBOracleReslut(sql_reslut, pProxyInfo);
    }
    if (iQueryStrLen < 4096) {
        printf("Query String: %s\n", pszQueryStr);
        DBOracleOperate(pszQueryStr, pProxyInfo);
    } else {
        pszQueryStr[4096] = 0;
        DBOracleOperate(pszQueryStr, pProxyInfo);
        printf("Query String: %s\n", pszQueryStr);
    }
    if (pProxyInfo->level == 1) {
        pProxyInfo->level = 0;
        findSQL(pszSqlQueryData + i, iSqlQueryDataLen - i, pszQueryStr);
    }

    return iSqlQueryDataLen;
    // }
    // }

#else
    if (iSqlQueryDataLen < sizeof (DATA035e) + 48) {
        return -1;
    }
    char pszQueryStr[4096];
    int iQueryStrLen;
    unsigned int nCurPlace;

    DATA035e *pData035e = (DATA035e *) pszSqlQueryData;

    unsigned char ucVer[3];
    memset(ucVer, 0, sizeof (ucVer));
    memcpy(ucVer, FindVer(pnethead), 2);
    if (ucVer[0] == 0x01 && ucVer[1] == 0x39) {
        nCurPlace = sizeof (DATA0303);
    } else if (ucVer[0] == 0x01 && ucVer[1] == 0x3a) {
        nCurPlace = sizeof (DATA0303) + 20;
    }

    memset(pszQueryStr, 0, sizeof (pszQueryStr));
    int iRet = ParseStrbyLen(pszSqlQueryData + sizeof (DATA035e) + nCurPlace, iSqlQueryDataLen - sizeof (DATA035e) - nCurPlace, pszQueryStr, &iQueryStrLen);

    nCurPlace += (iRet + iQueryStrLen);

    printf("Query String: %s\n", pszQueryStr);
    DBOracleOperate(pszQueryStr, pnethead);

    return nCurPlace;
#endif
}

/***************************************************************
Function:       GetColStrLen
Description:    根据列数获取列信息字的长度
Input:                      
                1. unsigned short uColNum     列数
Output:   
Return:         
                返回列的标志长度
Others:         列的标志的每一位代表该列是否存在
 ***************************************************************/
int GetColStrLen(unsigned short uColNum) {
    int nColLen = 0;
    if (uColNum == 0) {
        return 1;
    } else if (uColNum % 8 == 0) {
        nColLen = uColNum / 8;
    } else {
        nColLen = uColNum / 8 + 1;
    }
    return nColLen;
}

/***************************************************************
Function:       DecodeTnsData0600
Description:    解析0x0600结构包
Input:                      
                1. PACKET_INFO *pPacketInfo     包相关信息
                2. char *pszRowResultData       数据流指针
                3. int iRowResultLen            数据长度
Output:   
                返回行结果信息
Return:  
                >0   成功,返回包长度
                <0  失败
Others:         
 ***************************************************************/
int DecodeTnsData0600(char *pszRowResultData, unsigned int iRowResultLen, PPROXYINFO pProxyInfo) {
      //    char    pszRowStr[20][1024];        //行字符串
    int iRowStrLen = 0; //当前列的程度
    unsigned short uColNum = 0; //当前行的列数
    unsigned int iCurPlace = 0; //当前内存中的位置
    unsigned char szCol[128]; //当前行的列标志字符串
    //    unsigned char   szOldCol[128];              //上一行的列标志字符串
    unsigned int nColLen = 0; //标志位的字符串长度
    unsigned int nCurCol = 0; //当前处理的列
    unsigned int nRealCol = 0; //实际发送的行
    //    unsigned int    nMaxColNum = 0;             
    int iRet;
    unsigned int nRowNum = 0; //行数
    DATA0600 *pData0600 = (DATA0600 *) pszRowResultData;

    if(iRowResultLen>20*1024)
        return iRowResultLen;
    
    uColNum = pData0600->nColNum;
    if (uColNum < 0) {
        return -1;
    }else if(uColNum == 0){
        /*memset(pszRowStr, 0, sizeof (pszRowStr));
        iCurPlace = sizeof (DATA0600);
        memset(szOldCol, 0, sizeof (szOldCol));
        goto row_split;*/
    }else if(uColNum > 64){
        return -1;
    }
    
    if(nGlobMaxColNum == 0){
        nGlobMaxColNum = uColNum;
    }
    

    
    iCurPlace = sizeof (DATA0600);
    
    
    if(iversion == 11){
        //iCurPlace+=2;
        if(*(pszRowResultData + iCurPlace + 1) == 0x07){
            if(uColNum < nGlobMaxColNum){
                iCurPlace+=2;
            }
        }
    }else if(iversion == 10){
        if(pData0600->szMagic1[0] == 0x0a){
            iCurPlace+=4;
        }
        if(*(pszRowResultData + iCurPlace + 1) == 0x07){
            if(uColNum < nGlobMaxColNum){
                iCurPlace+=2;
            }
        }
        //iCurPlace+=4;
    }else if(iversion == 9){
        iCurPlace-=4;
        if(*(pszRowResultData + iCurPlace + 1) == 0x07){
            if(uColNum < nGlobMaxColNum){
                iCurPlace+=2;
            }
        }
    }
    if(uColNum < nGlobMaxColNum){
        nColLen = GetColStrLen(nGlobMaxColNum);
        memset(szCol, 0, sizeof (szCol));
        memcpy(szCol, pszRowResultData + iCurPlace - nColLen - 1, nColLen);
        nRowNum ++;
    }else{
        memset(pszRowStr, 0, sizeof (pszRowStr));
        memset(szOldCol, 0, sizeof (szOldCol));
    }
    //nMaxColNum = 0;

    while (1) {
        nCurCol = 0;
        nRealCol = 0;

        if (nMaxColNum < uColNum) {
            nMaxColNum = uColNum;
        }

        while (nCurCol < nMaxColNum) {
            if (iCurPlace > iRowResultLen) {
                return -2;
            }


            //nCurCol++;
            /*
             *	判断数据是否包含第nCurCol列,如果该行没有数据，但是其上n行有数据则需要显示，否则不显示数据
             */
            if (nRowNum > 0 && (szCol[(nCurCol / 8)]&(1 << (nCurCol % 8))) == 0) {

                //如果上一行nCurCol列有数据而本行nCurCol列没有数据，则按上一行数据显示
                if ((szOldCol[(nCurCol / 8)]&(1 << (nCurCol % 8))) != 0) {
                    PrintData(pszFieldType[nCurCol], pszRowStr[nCurCol], pRowStrLen[nCurCol]);
                } else {
                    printreslut("NULL|");
                }

                nCurCol++;
                continue;
            }

            
            if (nRealCol >= uColNum) {
                break;
            }

            if (nCurCol < 0) {
                return -1;
            }else if(nCurCol > 64){
                return -1;
            }

            memset(pszRowStr[nCurCol], 0, sizeof (pszRowStr[nCurCol]));

            
            if(pszFieldType[nCurCol]==0x71){
                int i = 0;
                int isadd = 0;
                unsigned int temp = 0;
                memcpy(&temp,pszRowResultData + iCurPlace,4);
                if(temp == 0){
                    printreslut("NULL");
                    iCurPlace += 4;
                }else{
                    iCurPlace += 4;
                    if(*(unsigned char *)(pszRowResultData + iCurPlace) == 0xfe && *(unsigned char *)(pszRowResultData + iCurPlace + 1) == 0xff){
                        iCurPlace+=2;
                        isadd = 1;
                    }else{
                        iCurPlace+=1;
                    }
                    unsigned short len = *(unsigned char *)(pszRowResultData + iCurPlace) * 256 + *(unsigned char *)(pszRowResultData + iCurPlace + 1);
                    if(len > iRowResultLen)
                        break;
                    iCurPlace += 2;
                    iCurPlace += 100;
                    len -= 100;
                    for(i = 0 ; i < len ; i ++){
                        printreslut("%02x", *(pszRowResultData + iCurPlace + i));
                    }
                    iCurPlace += len;
                    if(isadd == 1){
                        iCurPlace += 2;
                    }
                }
                printreslut("|");
            }else if(pszFieldType[nCurCol]==0x70){//clob nclob
                int i = 0;
                int isadd = 0;
                unsigned int temp = 0;
                if(*(unsigned char *)(pszRowResultData + iCurPlace) == 0xff && *(unsigned char *)(pszRowResultData + iCurPlace + 1) == 0xff){//is nclob
                    iCurPlace+=4;
                }
                memcpy(&temp,pszRowResultData + iCurPlace,4);
                if(temp == 0){
                    printreslut("NULL");
                    iCurPlace += 4;
                }else{
                    iCurPlace += 4;
                    if(*(unsigned char *)(pszRowResultData + iCurPlace) == 0xfe && *(unsigned char *)(pszRowResultData + iCurPlace + 1) == 0xff){
                        iCurPlace+=2;
                        isadd = 1;
                    }else{
                        iCurPlace+=1;
                    }
                    unsigned short len = *(unsigned char *)(pszRowResultData + iCurPlace) * 256 + *(unsigned char *)(pszRowResultData + iCurPlace + 1);
                    if(len > iRowResultLen || len < 100)
                        break;
                    iCurPlace += 2;
                    iCurPlace += 100;
                    len -= 100;
                    for(i = 0 ; i < len ; i ++){
                        if(*(pszRowResultData + iCurPlace + i) != 0x00)
                            printreslut("%c", *(pszRowResultData + iCurPlace + i));
                    }
                    iCurPlace += len;
                    if(isadd == 1){
                        iCurPlace += 2;
                    }
                    printreslut("|");
                }
            }else{
                iRet = ParseStrbyLen(pszRowResultData + iCurPlace, iRowResultLen - sizeof (DATA0601) - iCurPlace, pszRowStr[nCurCol], &iRowStrLen);
                if (iRet < 0) {
                    return -1;
                }
                PrintData(pszFieldType[nCurCol], pszRowStr[nCurCol], iRowStrLen);
                pRowStrLen[nCurCol] = iRowStrLen<1024?iRowStrLen:1024;
                //printf("%s\t", pszRowStr[nCurCol]); 

                szOldCol[(nCurCol / 8)] = szOldCol[(nCurCol / 8)] | (1 << (nCurCol % 8));
                iCurPlace += iRowStrLen + iRet;
            }

            nCurCol++;
            nRealCol++;
        }

        //printreslut("#");
        if(strlen(sql_reslut)>0){
                DBOracleReslut(sql_reslut,pProxyInfo);
            }

        //memset(szOldCol, 0, sizeof(szOldCol));
        //memcpy(szOldCol, szCol, nColLen);
row_split:
        nRowNum++;
        if (iCurPlace + sizeof (ROWSPLIT) > iRowResultLen) {
            return -1;
        }

        ROWSPLIT *pRowSplit = (ROWSPLIT*) (pszRowResultData + iCurPlace);

        if (pRowSplit->uId != 0x15 || pRowSplit->uColNum > nGlobMaxColNum) {
            break;
        }
        iCurPlace += sizeof (ROWSPLIT);


        nColLen = GetColStrLen(nGlobMaxColNum);
        memset(szCol, 0, sizeof (szCol));
        memcpy(szCol, pszRowResultData + iCurPlace, nColLen);
        if (0 != pRowSplit->uColNum) {
            uColNum = pRowSplit->uColNum;
        }
        //if(nColLen == 1 && *(pszRowResultData + iCurPlace + 1) == 0x00){
        //    iCurPlace++;
        //}
        //iCurPlace += nColLen + 1;
        iCurPlace+=nColLen;
        while(*(pszRowResultData + iCurPlace) != 0x07){
            iCurPlace+=1;
            if(iCurPlace>iRowResultLen)
                return iCurPlace;
        }
        iCurPlace+=1;

    }

    return iCurPlace;
}
int DecodeTnsData0602(char *pszRowResultData, int iRowResultLen, PPROXYINFO pProxyInfo) {
      //    char    pszRowStr[20][1024];        //行字符串
    int iRowStrLen = 0; //当前列的程度
    unsigned short uColNum = 0; //当前行的列数
    unsigned int iCurPlace = 0; //当前内存中的位置
    unsigned char szCol[128]; //当前行的列标志字符串
    //    unsigned char   szOldCol[128];              //上一行的列标志字符串
    unsigned int nColLen = 0; //标志位的字符串长度
    unsigned int nCurCol = 0; //当前处理的列
    unsigned int nRealCol = 0; //实际发送的行
    //    unsigned int    nMaxColNum = 0;             
    int iRet;
    unsigned int nRowNum = 0; //行数
    DATA0602 *pData0602 = (DATA0602 *) pszRowResultData;
    if(nMaxColNum>64)
        return -1;

    uColNum = pData0602->nColNum;
    if (uColNum < 0) {
        return -1;
    }else if(uColNum == 0){
        memset(pszRowStr, 0, sizeof (pszRowStr));
        iCurPlace = sizeof (DATA0602);
        memset(szOldCol, 0, sizeof (szOldCol));
        goto row_split;
    }

    if(nGlobMaxColNum == 0){
        nGlobMaxColNum = uColNum;
    }
    memset(pszRowStr, 0, sizeof (pszRowStr));
    iCurPlace = sizeof (DATA0602);
    memset(szOldCol, 0, sizeof (szOldCol));
    
    //nMaxColNum = 0;
    while (1) {
        nCurCol = 0;
        nRealCol = 0;

        if (nMaxColNum < uColNum) {
            nMaxColNum = uColNum;
        }else if(nRowNum==0){
            nCurCol = nMaxColNum-uColNum;
        }

        while (nCurCol < nMaxColNum) {
            if (iCurPlace > iRowResultLen) {
                return -2;
            }

            if (nRealCol >= uColNum) {
                break;
            }

            //nCurCol++;
            /*
             *	判断数据是否包含第nCurCol列,如果该行没有数据，但是其上n行有数据则需要显示，否则不显示数据
             */
            /*if (nRowNum > 0 && (szCol[(nCurCol / 8)]&(1 << (nCurCol % 8))) == 0) {

                //如果上一行nCurCol列有数据而本行nCurCol列没有数据，则按上一行数据显示
                if ((szOldCol[(nCurCol / 8)]&(1 << (nCurCol % 8))) != 0) {
                    PrintData(pszFieldType[nCurCol], pszRowStr[nCurCol], iRowStrLen);
                } else {
                    printreslut("\t");
                }

                nCurCol++;
                continue;
            }*/

            

            if (nCurCol < 0) {
                return -1;
            }else if(nCurCol > 64){
                return -1;
            }

            memset(pszRowStr[nCurCol], 0, sizeof (pszRowStr[nCurCol]));

            iRet = ParseStrbyLen(pszRowResultData + iCurPlace, iRowResultLen - sizeof (DATA0601) - iCurPlace, pszRowStr[nCurCol], &iRowStrLen);
            if (iRet < 0) {
                return -1;
            }
            if(pszFieldType[nCurCol] == 0x71){
                
            }else{
                PrintData(pszFieldType[nCurCol], pszRowStr[nCurCol], iRowStrLen);
            }
            //printf("%s\t", pszRowStr[nCurCol]); 

            szOldCol[(nCurCol / 8)] = szOldCol[(nCurCol / 8)] | (1 << (nCurCol % 8));
            iCurPlace += iRowStrLen + iRet;

            
            nCurCol++;
            nRealCol++;
            if(*(pszRowResultData + iCurPlace) == 0x15)
                break;
            if(*(pszRowResultData + iCurPlace) == 0x04 && (*(pszRowResultData + iCurPlace + 1) == 0x01||*(pszRowResultData + iCurPlace + 1) == 0x02))
                break;
        }

        //printreslut("#");
        if(strlen(sql_reslut)>0){
                DBOracleReslut(sql_reslut,pProxyInfo);
            }

        //memset(szOldCol, 0, sizeof(szOldCol));
        //memcpy(szOldCol, szCol, nColLen);
        row_split:
        nRowNum++;
        if (iCurPlace + sizeof (ROWSPLIT) > iRowResultLen) {
            return -1;
        }

        ROWSPLIT *pRowSplit = (ROWSPLIT*) (pszRowResultData + iCurPlace);

        if (pRowSplit->uId != 0x15 || pRowSplit->uColNum > nGlobMaxColNum) {
            break;
        }
        iCurPlace += sizeof (ROWSPLIT);
        iCurPlace +=2;


        /*nColLen = GetColStrLen(pRowSplit->uColNum);
        memset(szCol, 0, sizeof (szCol));
        memcpy(szCol, pszRowResultData + iCurPlace, nColLen);
        if (0 != pRowSplit->uColNum) {
            uColNum = pRowSplit->uColNum;
        }

        iCurPlace += nColLen + 1;*/


    }

    return iCurPlace;
}

int DecodeTnsData0602_64(char *pszRowResultData, int iRowResultLen,PPROXYINFO pProxyInfo) {
      //    char    pszRowStr[20][1024];        //行字符串
    int iRowStrLen = 0; //当前列的程度
    unsigned short uColNum = 0; //当前行的列数
    unsigned int iCurPlace = 0; //当前内存中的位置
    unsigned char szCol[128]; //当前行的列标志字符串
    //    unsigned char   szOldCol[128];              //上一行的列标志字符串
    unsigned int nColLen = 0; //标志位的字符串长度
    unsigned int nCurCol = 0; //当前处理的列
    unsigned int nRealCol = 0; //实际发送的行
    int isColCode = 0;
    //    unsigned int    nMaxColNum = 0;             
    int iRet;
    unsigned int nRowNum = 0; //行数
    DATA0602_64 *pData0602 = (DATA0602_64 *) pszRowResultData;
    if(nMaxColNum>64)
        return -1;

    uColNum = pData0602->nColNum;
    if (uColNum < 0) {
        return -1;
    }else if(uColNum == 0){
        memset(pszRowStr, 0, sizeof (pszRowStr));
        iCurPlace = sizeof (DATA0602_64);
        memset(szOldCol, 0, sizeof (szOldCol));
        goto row_split;
    }

    if(nGlobMaxColNum == 0){
        nGlobMaxColNum = uColNum;
    }
    memset(pszRowStr, 0, sizeof (pszRowStr));
    iCurPlace = sizeof (DATA0602_64);
    memset(szOldCol, 0, sizeof (szOldCol));
    
    if(bitversion == 64 && iversion == 9){
        if(pData0602->flag == 0x0f){
            iCurPlace -= 2;
        }else{
            iCurPlace -= 4;
        }
    }
    
    //nMaxColNum = 0;
     while (1) {
        nCurCol = 0;
        nRealCol = 0;

        if (nMaxColNum < uColNum) {
            nMaxColNum = uColNum;
        }

        while (nCurCol < nMaxColNum ) {
            if (iCurPlace > iRowResultLen) {
                return -2;
            }

            //nCurCol++;
            /*
             *	判断数据是否包含第nCurCol列,如果该行没有数据，但是其上n行有数据则需要显示，否则不显示数据
             */
            if (nRowNum > 0 && (szCol[(nCurCol / 8)]&(1 << (nCurCol % 8))) == 0 && (istoad != 1||isColCode==1)) {

                //如果上一行nCurCol列有数据而本行nCurCol列没有数据，则按上一行数据显示
                if ((szOldCol[(nCurCol / 8)]&(1 << (nCurCol % 8))) != 0) {
                    PrintData(pszFieldType[nCurCol], pszRowStr[nCurCol], pRowStrLen[nCurCol]);
                } else {
                    printreslut("NULL|");
                }

                nCurCol++;
                continue;
            }


            if (nRealCol >= uColNum) {
                break;
            }

            if (nCurCol < 0) {
                return -1;
            }else if(nCurCol >= 64){
                return -1;
            }
            memset(pszRowStr[nCurCol], 0, sizeof (pszRowStr[nCurCol]));

            if(pszFieldType[nCurCol]==0x71){
                if(iversion == 9 && istoad==1 && bitversion== 64){
                    unsigned int len = 0;
                    
                    //iCurPlace+=4;
                    memcpy(&len,(pszRowResultData + iCurPlace),sizeof(len));
                    iCurPlace+=4;
                    if(*(unsigned char *)(pszRowResultData + iCurPlace) == 0xfe && *(unsigned char *)(pszRowResultData + iCurPlace + 1) == 0xff){
                            iCurPlace+=10;
                            iCurPlace+=6;
                     }
                    if(len > 0){
                        iCurPlace+=1;
                    }else if(len == 0){
                        
                    }else{
                        break;
                    }
                    iCurPlace+=len;
                    printreslut("blob|");
                }else{
                    int i = 0;
                    int isadd = 0;
                    unsigned int temp = 0;
                    memcpy(&temp,pszRowResultData + iCurPlace,4);
                    if(temp == 0){
                        printreslut("NULL");
                        iCurPlace += 4;
                    }else{
                        iCurPlace += 4;
                        if(*(unsigned char *)(pszRowResultData + iCurPlace) == 0xfe && *(unsigned char *)(pszRowResultData + iCurPlace + 1) == 0xff){
                            iCurPlace+=2;
                            isadd = 1;
                        }else{
                            iCurPlace+=1;
                        }
                        unsigned short len = *(unsigned char *)(pszRowResultData + iCurPlace) * 256 + *(unsigned char *)(pszRowResultData + iCurPlace + 1);
                        if(len > iRowResultLen || len <= 100){
                            iCurPlace += 5;
                            break;
                        }
                        iCurPlace += 2;
                        iCurPlace += 100;
                        len -= 100;
                        for(i = 0 ; i < len ; i ++){
                            printreslut("%02x", *(pszRowResultData + iCurPlace + i));
                        }
                        iCurPlace += len;
                        if(isadd == 1){
                            iCurPlace += 2;
                        }
                    }
                    printreslut("|");
                }
            }else if(pszFieldType[nCurCol]==0x70){//clob nclob
                if(iversion == 11 || (iversion == 9 && istoad==1 && bitversion== 64)){
                    unsigned int len = 0;
                    
                    //iCurPlace+=4;
                    memcpy(&len,(pszRowResultData + iCurPlace),sizeof(len));
                    iCurPlace+=4;
                    if(*(unsigned char *)(pszRowResultData + iCurPlace) == 0xfe && *(unsigned char *)(pszRowResultData + iCurPlace + 1) == 0xff){
                            iCurPlace+=10;
                            iCurPlace+=6;
                     }
                    if(len > 0){
                        iCurPlace+=1;
                    }else if(len == 0){
                        
                    }else{
                        break;
                    }
                    iCurPlace+=len;
                    printreslut("clob|");
                }else{
                    int i = 0;
                    int isadd = 0;
                    unsigned int temp = 0;
                    if(*(unsigned char *)(pszRowResultData + iCurPlace) == 0xff && *(unsigned char *)(pszRowResultData + iCurPlace + 1) == 0xff){//is nclob
                        iCurPlace+=4;
                    }
                    memcpy(&temp,pszRowResultData + iCurPlace,4);
                    if(temp == 0){
                        printreslut("NULL|");
                        iCurPlace += 4;
                    }else{
                        iCurPlace += 4;
                        if(*(unsigned char *)(pszRowResultData + iCurPlace) == 0xfe && *(unsigned char *)(pszRowResultData + iCurPlace + 1) == 0xff){
                            iCurPlace+=2;
                            isadd = 1;
                        }else{
                            iCurPlace+=1;
                        }
                        
                        unsigned short len = *(unsigned char *)(pszRowResultData + iCurPlace) * 256 + *(unsigned char *)(pszRowResultData + iCurPlace + 1);
                        if(len > iRowResultLen || len <= 100)
                            break;
                        iCurPlace += 2;
                        iCurPlace += 100;
                        len -= 100;
                        for(i = 0 ; i < len ; i ++){
                            if(*(pszRowResultData + iCurPlace + i) != 0x00)
                                printreslut("%c", *(pszRowResultData + iCurPlace + i));
                        }
                        iCurPlace += len;
                        if(isadd == 1){
                            iCurPlace += 5;
                        }
                        
                        printreslut("|");
                    }
                }
            }
            else if(pszFieldType[nCurCol]==0x72)//bfile
            {
                int len = 0;
                int offset = 0;
                iCurPlace ++;
                
                memcpy(((char*)&len),pszRowResultData + iCurPlace+3,1);
                memcpy(((char*)&len)+1,pszRowResultData + iCurPlace+2,1);
                memcpy(((char*)&len)+2,pszRowResultData + iCurPlace+1,1);
                memcpy(((char*)&len)+3,pszRowResultData + iCurPlace,1);
                
                iCurPlace += 4;
                
                offset = 16;
                
                short dirlen = 0;
                short namelen = 0;
                
                memcpy(((char*)&dirlen),pszRowResultData + iCurPlace + offset +1,1);
                memcpy(((char*)&dirlen)+1,pszRowResultData + iCurPlace + offset ,1);
                offset += 2;
                if(dirlen >0){
                    char *dir = (char*)malloc(dirlen+1);
                    memset(dir,0,dirlen+1);
                    memcpy(dir,pszRowResultData + iCurPlace + offset,dirlen);
                    offset += dirlen;
                    printreslut("%s//", dir);
                    free(dir);
                    
                    memcpy(((char*)&namelen),pszRowResultData + iCurPlace + offset +1,1);
                    memcpy(((char*)&namelen)+1,pszRowResultData + iCurPlace + offset ,1);
                    offset += 2;
                    if(namelen > 0){
                        char *name = (char*)malloc(namelen+1);
                        memset(name,0,namelen+1);
                        memcpy(name,pszRowResultData + iCurPlace + offset,namelen);
                        offset += namelen;
                        printreslut("%s", name);
                        free(name);
                    }
                    printreslut("|");
                }
                
                iCurPlace+=len;
                
                
                //dir
                //name
            }
            else{
                iRet = ParseStrbyLen(pszRowResultData + iCurPlace, iRowResultLen - sizeof (DATA0601) - iCurPlace, pszRowStr[nCurCol], &iRowStrLen);
                if (iRet < 0) {
                    return -1;
                }
                if ((pszFieldType[nCurCol] == 0x0b) && bitversion == 64 && pszRowResultData + iCurPlace != 0x0e){
                    iRowStrLen = 14;
                    iRet = 0;
                } 
                if ((pszFieldType[nCurCol] == 0xd0) && bitversion == 64 && pszRowResultData + iCurPlace != 0x0e){
                    iRowStrLen = 9;
                    iRet = 0;
                } 
                PrintData(pszFieldType[nCurCol], pszRowStr[nCurCol], iRowStrLen);
                pRowStrLen[nCurCol] = iRowStrLen<1024?iRowStrLen:1024;
               // printf("%s\t", sql_reslut); 
                if (pszFieldType[nCurCol] == 0x08 || pszFieldType[nCurCol] == 0x18) {
                    iCurPlace += 4;
                }

                szOldCol[(nCurCol / 8)] = szOldCol[(nCurCol / 8)] | (1 << (nCurCol % 8));
                iCurPlace += iRowStrLen + iRet;
                
            }

            nCurCol++;
            nRealCol++;
        }
 //       printf("\n");
        //printreslut("#");
        if(strlen(sql_reslut)>0){
            DBOracleReslut(sql_reslut,pProxyInfo);
        }

        //memset(szOldCol, 0, sizeof(szOldCol));
        //memcpy(szOldCol, szCol, nColLen);
row_split:
        nRowNum++;
        if (iCurPlace + sizeof (ROWSPLIT) > iRowResultLen) {
            return -1;
        }
        if(istoad == 1 && *(pszRowResultData + iCurPlace) == 0x07){
            iCurPlace+=1;
            
            
            if (uColNum <= 0) {
                return -1;
            }else if(uColNum > 64){
                return -1;
            }
            continue;
        }

        ROWSPLIT *pRowSplit = (ROWSPLIT*) (pszRowResultData + iCurPlace);

        if (pRowSplit->uId != 0x15 || pRowSplit->uColNum > nGlobMaxColNum) {
            break;
        }
        iCurPlace += sizeof (ROWSPLIT);//下一行


        nColLen = GetColStrLen(nGlobMaxColNum);
        memset(szCol, 0, sizeof (szCol));
        memcpy(szCol, pszRowResultData + iCurPlace, nColLen);
        if (0 != pRowSplit->uColNum) {
            uColNum = pRowSplit->uColNum;
        }
        //if(nColLen == 1 && *(pszRowResultData + iCurPlace + 1) == 0x00){
        //    iCurPlace++;
        //}
        //iCurPlace += nColLen + 1;
        iCurPlace+=nColLen;
        while(*(pszRowResultData + iCurPlace) != 0x07){
            iCurPlace+=1;
            if(iCurPlace>iRowResultLen)
                return iCurPlace;
        }
        iCurPlace+=1;
        isColCode = 1;
    }


    return iCurPlace;
}

int DecodeTnsData061a(char *pszRowResultData, int iRowResultLen,PPROXYINFO pProxyInfo) {
      //    char    pszRowStr[20][1024];        //行字符串
    int iRowStrLen = 0; //当前列的程度
    unsigned short uColNum = 0; //当前行的列数
    unsigned int iCurPlace = 0; //当前内存中的位置
    unsigned char szCol[128]; //当前行的列标志字符串
    //    unsigned char   szOldCol[128];              //上一行的列标志字符串
    unsigned int nColLen = 0; //标志位的字符串长度
    unsigned int nCurCol = 0; //当前处理的列
    unsigned int nRealCol = 0; //实际发送的行
    int isColCode = 0;
    //    unsigned int    nMaxColNum = 0;             
    int iRet;
    int isoffset = 0;
    unsigned int nRowNum = 0; //行数
    DATA0602_64 *pData0602 = (DATA0602_64 *) pszRowResultData;
    if(nMaxColNum>64)
        return -1;

    uColNum = pData0602->nColNum;
    if (uColNum < 0) {
        return -1;
    }else if(uColNum == 0){
        memset(pszRowStr, 0, sizeof (pszRowStr));
        iCurPlace = sizeof (DATA0602_64);
        memset(szOldCol, 0, sizeof (szOldCol));
        goto row_split;
    }

    if(nGlobMaxColNum == 0){
        nGlobMaxColNum = uColNum;
    }
    memset(pszRowStr, 0, sizeof (pszRowStr));
    iCurPlace = sizeof (DATA0602_64);
    memset(szOldCol, 0, sizeof (szOldCol));
    
    if(istoad == 1){
            if(*(unsigned char *)(pszRowResultData + iCurPlace) == 0xfe && *(unsigned char *)(pszRowResultData + iCurPlace+1) == 0xff){
                iCurPlace+=4;
            }else{
                iCurPlace+=3;
            }
            
            
            uColNum = *(pszRowResultData + iCurPlace);
            iCurPlace ++;
    }
   
    
    
    //nMaxColNum = 0;
     while (1) {
        nCurCol = 0;
        nRealCol = 0;

        if (nMaxColNum < uColNum) {
            nMaxColNum = uColNum;
        }

        while (nCurCol < nMaxColNum ) {
            if (iCurPlace > iRowResultLen) {
                return -2;
            }

            //nCurCol++;
            /*
             *	判断数据是否包含第nCurCol列,如果该行没有数据，但是其上n行有数据则需要显示，否则不显示数据
             */
            if (nRowNum > 0 && (szCol[(nCurCol / 8)]&(1 << (nCurCol % 8))) == 0 && (istoad != 1||isColCode==1)) {

                //如果上一行nCurCol列有数据而本行nCurCol列没有数据，则按上一行数据显示
                if ((szOldCol[(nCurCol / 8)]&(1 << (nCurCol % 8))) != 0) {
                    PrintData(pszFieldType[nCurCol], pszRowStr[nCurCol], pRowStrLen[nCurCol]);
                } else {
                    printreslut("NULL|");
                }

                nCurCol++;
                continue;
            }


            if (nRealCol >= uColNum) {
                break;
            }

            if (nCurCol < 0) {
                return -1;
            }else if(nCurCol > 64){
                return -1;
            }
            memset(pszRowStr[nCurCol], 0, sizeof (pszRowStr[nCurCol]));

            if(pszFieldType[nCurCol]==0x71){
                if(iversion == 9 && istoad==1 && bitversion== 64){
                    unsigned int len = 0;
                    
                    //iCurPlace+=4;
                    memcpy(&len,(pszRowResultData + iCurPlace),sizeof(len));
                    iCurPlace+=4;
                    if(*(unsigned char *)(pszRowResultData + iCurPlace) == 0xfe && *(unsigned char *)(pszRowResultData + iCurPlace + 1) == 0xff){
                            iCurPlace+=10;
                            iCurPlace+=6;
                     }
                    if(len > 0){
                        iCurPlace+=1;
                    }else if(len == 0){
                        
                    }else{
                        break;
                    }
                    iCurPlace+=len;
                    printreslut("clob|");
                }else{
                    int i = 0;
                    int isadd = 0;
                    unsigned int temp = 0;
                    memcpy(&temp,pszRowResultData + iCurPlace,4);
                    if(temp == 0){
                        printreslut("NULL");
                        iCurPlace += 4;
                    }else{
                        iCurPlace += 4;
                        if(*(unsigned char *)(pszRowResultData + iCurPlace) == 0xfe && *(unsigned char *)(pszRowResultData + iCurPlace + 1) == 0xff){
                            iCurPlace+=2;
                            isadd = 1;
                        }else{
                            iCurPlace+=1;
                        }
                        unsigned short len = *(unsigned char *)(pszRowResultData + iCurPlace) * 256 + *(unsigned char *)(pszRowResultData + iCurPlace + 1);
                        if(len > iRowResultLen || len <= 100){
                            iCurPlace += 5;
                            break;
                        }
                        iCurPlace += 2;
                        iCurPlace += 100;
                        len -= 100;
                        for(i = 0 ; i < len ; i ++){
                            printreslut("%02x", *(pszRowResultData + iCurPlace + i));
                        }
                        iCurPlace += len;
                        if(isadd == 1){
                            iCurPlace += 2;
                        }
                    }
                    printreslut("|");
                }
            }else if(pszFieldType[nCurCol]==0x70){//clob nclob
                if(iversion == 11 || (iversion == 9 && istoad==1 && bitversion== 64)){
                    unsigned int len = 0;
                    
                    //iCurPlace+=4;
                    memcpy(&len,(pszRowResultData + iCurPlace),sizeof(len));
                    iCurPlace+=4;
                    if(*(unsigned char *)(pszRowResultData + iCurPlace) == 0xfe && *(unsigned char *)(pszRowResultData + iCurPlace + 1) == 0xff){
                            iCurPlace+=10;
                            iCurPlace+=6;
                     }
                    if(len > 0){
                        iCurPlace+=1;
                    }else if(len == 0){
                        
                    }else{
                        break;
                    }
                    iCurPlace+=len;
                    printreslut("clob|");
               
                }else{
                    int i = 0;
                    int isadd = 0;
                    unsigned int temp = 0;
                    if(*(unsigned char *)(pszRowResultData + iCurPlace) == 0xff && *(unsigned char *)(pszRowResultData + iCurPlace + 1) == 0xff){//is nclob
                        iCurPlace+=4;
                    }
                    memcpy(&temp,pszRowResultData + iCurPlace,4);
                    if(temp == 0){
                        printreslut("NULL|");
                        iCurPlace += 4;
                    }else{
                        iCurPlace += 4;
                        if(*(unsigned char *)(pszRowResultData + iCurPlace) == 0xfe && *(unsigned char *)(pszRowResultData + iCurPlace + 1) == 0xff){
                            iCurPlace+=2;
                            isadd = 1;
                        }else{
                            iCurPlace+=1;
                        }
                        
                        unsigned short len = *(unsigned char *)(pszRowResultData + iCurPlace) * 256 + *(unsigned char *)(pszRowResultData + iCurPlace + 1);
                        if(len > iRowResultLen || len <= 100)
                            break;
                        iCurPlace += 2;
                        iCurPlace += 100;
                        len -= 100;
                        for(i = 0 ; i < len ; i ++){
                            if(*(pszRowResultData + iCurPlace + i) != 0x00)
                                printreslut("%c", *(pszRowResultData + iCurPlace + i));
                        }
                        iCurPlace += len;
                        if(isadd == 1){
                            iCurPlace += 5;
                        }
                        
                        printreslut("|");
                    }
                }
            }
            else if(pszFieldType[nCurCol]==0x72)//bfile
            {
                int len = 0;
                int offset = 0;
                iCurPlace ++;
                
                memcpy(((char*)&len),pszRowResultData + iCurPlace+3,1);
                memcpy(((char*)&len)+1,pszRowResultData + iCurPlace+2,1);
                memcpy(((char*)&len)+2,pszRowResultData + iCurPlace+1,1);
                memcpy(((char*)&len)+3,pszRowResultData + iCurPlace,1);
                
                iCurPlace += 4;
                
                offset = 16;
                
                short dirlen = 0;
                short namelen = 0;
                
                memcpy(((char*)&dirlen),pszRowResultData + iCurPlace + offset +1,1);
                memcpy(((char*)&dirlen)+1,pszRowResultData + iCurPlace + offset ,1);
                offset += 2;
                if(dirlen >0){
                    char *dir = (char*)malloc(dirlen+1);
                    memset(dir,0,dirlen+1);
                    memcpy(dir,pszRowResultData + iCurPlace + offset,dirlen);
                    offset += dirlen;
                    printreslut("%s//", dir);
                    free(dir);
                    
                    memcpy(((char*)&namelen),pszRowResultData + iCurPlace + offset +1,1);
                    memcpy(((char*)&namelen)+1,pszRowResultData + iCurPlace + offset ,1);
                    offset += 2;
                    if(namelen > 0){
                        char *name = (char*)malloc(namelen+1);
                        memset(name,0,namelen+1);
                        memcpy(name,pszRowResultData + iCurPlace + offset,namelen);
                        offset += namelen;
                        printreslut("%s", name);
                        free(name);
                    }
                    printreslut("|");
                }
                
                iCurPlace+=len;
                
                
                //dir
                //name
             
            }
            else{
                iRet = ParseStrbyLen(pszRowResultData + iCurPlace, iRowResultLen - sizeof (DATA0601) - iCurPlace, pszRowStr[nCurCol], &iRowStrLen);
                if (iRet < 0) {
                    return -1;
                }
                if ((pszFieldType[nCurCol] == 0x0b) && bitversion == 64 && pszRowResultData + iCurPlace != 0x0e){
                    iRowStrLen = 14;
                    iRet = 0;
                } 
                if ((pszFieldType[nCurCol] == 0xd0) && bitversion == 64 && pszRowResultData + iCurPlace != 0x0e){
                    iRowStrLen = 9;
                    iRet = 0;
                } 
                PrintData(pszFieldType[nCurCol], pszRowStr[nCurCol], iRowStrLen);
                pRowStrLen[nCurCol] = iRowStrLen<1024?iRowStrLen:1024;
               // printf("%s\t", sql_reslut); 
                if (pszFieldType[nCurCol] == 0x08 || pszFieldType[nCurCol] == 0x18) {
                    iCurPlace += 4;
                }
                if(pszFieldType[nCurCol]==96){
                    int i = 0 ;
                    for(i = iRet ; i < iRowStrLen ; i ++ ){
                        if(*(unsigned char *)(pszRowResultData + iCurPlace + i) == 0x9f || *(unsigned char *)(pszRowResultData + iCurPlace + i) == 0xff){
                            iRowStrLen ++;
                            break;
                        }
                    }
                    
                    
                }

                szOldCol[(nCurCol / 8)] = szOldCol[(nCurCol / 8)] | (1 << (nCurCol % 8));
                iCurPlace += iRowStrLen + iRet;
                
            }

            nCurCol++;
            nRealCol++;
        }
 //       printf("\n");
        //printreslut("#");
        if(strlen(sql_reslut)>0){
            DBOracleReslut(sql_reslut,pProxyInfo);
        }
        
        //memset(szOldCol, 0, sizeof(szOldCol));
        //memcpy(szOldCol, szCol, nColLen);
row_split:
        nRowNum++;
        isoffset = 0;
        if (iCurPlace + sizeof (ROWSPLIT) > iRowResultLen) {
            return -1;
        }
        if(istoad == 1 && *(unsigned char *)(pszRowResultData + iCurPlace) == 0x00){
            iCurPlace ++;
        }
        if(istoad == 1 && *(unsigned char *)(pszRowResultData + iCurPlace) == 0x07){
            iCurPlace ++;
            if(*(unsigned char *)(pszRowResultData + iCurPlace) == 0xfe && *(unsigned char *)(pszRowResultData + iCurPlace+1) == 0xff){
                iCurPlace+=4;
            }else{
                iCurPlace+=3;
            }
            
            
            uColNum = *(pszRowResultData + iCurPlace);
            iCurPlace ++;
            if (uColNum <= 0) {
                return -1;
            }else if(uColNum > 64){
                return -1;
            }
            continue;
        }

        ROWSPLIT *pRowSplit = (ROWSPLIT*) (pszRowResultData + iCurPlace);

        if (pRowSplit->uId != 0x15 || pRowSplit->uColNum > nGlobMaxColNum) {
            break;
        }
        iCurPlace += sizeof (ROWSPLIT);


        nColLen = GetColStrLen(nGlobMaxColNum);
        memset(szCol, 0, sizeof (szCol));
        memcpy(szCol, pszRowResultData + iCurPlace, nColLen);
        if (0 != pRowSplit->uColNum) {
            uColNum = pRowSplit->uColNum;
        }
        //if(nColLen == 1 && *(pszRowResultData + iCurPlace + 1) == 0x00){
        //    iCurPlace++;
        //}
        //iCurPlace += nColLen + 1;
        iCurPlace+=nColLen;
        while(*(pszRowResultData + iCurPlace) != 0x07){
            iCurPlace+=1;
            if(iCurPlace>iRowResultLen)
                return iCurPlace;
        }
        iCurPlace+=1;
        isColCode = 1;
    }


    return iCurPlace;
}

/***************************************************************
Function:       DecodeTnsData0601
Description:    解析0x0601结构包
Input:                      
                1. PACKET_INFO *pPacketInfo     包相关信息
                2. char *pszRowResultData       数据流指针
                3. int iRowResultLen            数据长度
Output:   
                返回行结果信息
Return:  
                >0   成功,返回包长度
                <0  失败
Others:         
 ***************************************************************/
int DecodeTnsData0601(unsigned char *pszRowResultData, unsigned int iRowResultLen, PPROXYINFO pProxyInfo) {
    //    char    pszRowStr[20][1024];        //行字符串
    int iRowStrLen = 0; //当前列的程度
    unsigned short uColNum = 0; //当前行的列数
    unsigned int iCurPlace = 0; //当前内存中的位置
    unsigned char szCol[128]; //当前行的列标志字符串
    //    unsigned char   szOldCol[128];              //上一行的列标志字符串
    unsigned int nColLen = 0; //标志位的字符串长度
    unsigned int nCurCol = 0; //当前处理的列
    unsigned int nRealCol = 0; //实际发送的行
    //    unsigned int    nMaxColNum = 0;             
    int iRet;
    int shortData = 0;
    unsigned int nRowNum = 0; //行数
    int isColCode = 0;
    DATA0601 *pData0601 = (DATA0601 *) pszRowResultData;

    uColNum = pData0601->nColNum;
    if (uColNum <= 0) {
        return -1;
    }else if(uColNum > 64){
        return -1;
    }
    memset(pszRowStr, 0, sizeof (pszRowStr));

    if(nGlobMaxColNum == 0){
        nGlobMaxColNum = uColNum;
    }
    iCurPlace = sizeof (DATA0601);
    if(iversion == 10 && istoad != 1)
    {
        if(pData0601->szMagic1[0] == 0x0a){
            iCurPlace+=4;
        }
        //iCurPlace+=4;
    }else if(iversion == 9){
        iCurPlace -= 4;
    }else if(iversion == 11 && istoad == 1){
        iCurPlace -= 1;
        if(*(pszRowResultData + iCurPlace) == 0x07){
            iCurPlace+=1;
            //nColLen = GetColStrLen(nMaxColNum);
            if(pData0601->szMagic1[0] == 0x1a){
                memset(szCol, 0, sizeof (szCol));
                memcpy(szCol, pszRowResultData + iCurPlace, 1);
                iCurPlace += 1;
                iCurPlace += 2;
                uColNum = *(pszRowResultData + iCurPlace);
                iCurPlace += 1;
            }
        }
    }
    memset(szOldCol, 0, sizeof (szOldCol));
    //nMaxColNum = 0;
    while (1) {
        nCurCol = 0;
        nRealCol = 0;

        if (nMaxColNum < uColNum) {
            nMaxColNum = uColNum;
        }

        while (nCurCol < nMaxColNum ) {
            if (iCurPlace > iRowResultLen) {
                return -2;
            }

            //nCurCol++;
            /*
             *	判断数据是否包含第nCurCol列,如果该行没有数据，但是其上n行有数据则需要显示，否则不显示数据
             */
            if (nRowNum > 0 && (szCol[(nCurCol / 8)]&(1 << (nCurCol % 8))) == 0 && (istoad != 1||isColCode==1)) {

                //如果上一行nCurCol列有数据而本行nCurCol列没有数据，则按上一行数据显示
                if ((szOldCol[(nCurCol / 8)]&(1 << (nCurCol % 8))) != 0) {
                    PrintData(pszFieldType[nCurCol], pszRowStr[nCurCol], pRowStrLen[nCurCol]);
                } else {
                    printreslut("NULL|");
                }

                nCurCol++;
                continue;
            }


            if (nRealCol >= uColNum) {
                break;
            }

            if (nCurCol < 0) {
                return -1;
            }else if(nCurCol > 64){
                return -1;
            }
            memset(pszRowStr[nCurCol], 0, sizeof (pszRowStr[nCurCol]));

            if(pszFieldType[nCurCol]==0x71){
                int i = 0;
                int isadd = 0;
                unsigned int temp = 0;
                memcpy(&temp,pszRowResultData + iCurPlace,4);
                if(temp == 0){
                    printreslut("NULL");
                    iCurPlace += 4;
                }else{
                    iCurPlace += 4;
                    if(*(unsigned char *)(pszRowResultData + iCurPlace) == 0xfe && *(unsigned char *)(pszRowResultData + iCurPlace + 1) == 0xff){
                        iCurPlace+=2;
                        isadd = 1;
                    }else{
                        iCurPlace+=1;
                    }
                    unsigned short len = *(unsigned char *)(pszRowResultData + iCurPlace) * 256 + *(unsigned char *)(pszRowResultData + iCurPlace + 1);
                    if(len > iRowResultLen  || len <= 100)
                        break;
                    iCurPlace += 2;
                    iCurPlace += 100;
                    len -= 100;
                    for(i = 0 ; i < len ; i ++){
                        printreslut("%02x", *(pszRowResultData + iCurPlace + i));
                    }
                    iCurPlace += len;
                    if(isadd == 1){
                        iCurPlace += 2;
                    }
                }
                printreslut("|");
            }else if(pszFieldType[nCurCol]==0x70){//clob nclob
                if(iversion == 11){
                    unsigned int len = 0;
                    
                    //iCurPlace+=4;
                    memcpy(&len,(pszRowResultData + iCurPlace),sizeof(len));
                    iCurPlace+=4;
                    if(*(unsigned char *)(pszRowResultData + iCurPlace) == 0xfe && *(unsigned char *)(pszRowResultData + iCurPlace + 1) == 0xff){
                            iCurPlace+=10;
                            iCurPlace+=6;
                     }
                    if(len > 0){
                        iCurPlace+=1;
                    }else if(len == 0){
                        
                    }else{
                        break;
                    }
                    iCurPlace+=len;
                    
                    
                    printreslut("clob|");
                }else{
                    int i = 0;
                    int isadd = 0;
                    unsigned int temp = 0;
                    if(*(unsigned char *)(pszRowResultData + iCurPlace) == 0xff && *(unsigned char *)(pszRowResultData + iCurPlace + 1) == 0xff){//is nclob
                        iCurPlace+=4;
                    }
                    memcpy(&temp,pszRowResultData + iCurPlace,4);
                    if(temp == 0){
                        printreslut("NULL|");
                        iCurPlace += 4;
                    }else{
                        iCurPlace += 4;
                        if(*(unsigned char *)(pszRowResultData + iCurPlace) == 0xfe && *(unsigned char *)(pszRowResultData + iCurPlace + 1) == 0xff){
                            iCurPlace+=2;
                            isadd = 1;
                        }else{
                            iCurPlace+=1;
                        }
                        unsigned short len = *(unsigned char *)(pszRowResultData + iCurPlace) * 256 + *(unsigned char *)(pszRowResultData + iCurPlace + 1);
                        if(len > iRowResultLen || len < 100)
                            break;
                        iCurPlace += 2;
                        iCurPlace += 100;
                        len -= 100;
                        for(i = 0 ; i < len ; i ++){
                            if(*(pszRowResultData + iCurPlace + i) != 0x00)
                                printreslut("%c", *(pszRowResultData + iCurPlace + i));
                        }
                        iCurPlace += len;
                        if(isadd == 1){
                            iCurPlace += 2;
                        }
                        printreslut("|");
                    }
                }
            }else{
                iRet = ParseStrbyLen(pszRowResultData + iCurPlace, iRowResultLen - sizeof (DATA0601) - iCurPlace, pszRowStr[nCurCol], &iRowStrLen);
                if (iRet < 0) {
                    return -1;
                }

                PrintData(pszFieldType[nCurCol], pszRowStr[nCurCol], iRowStrLen);
                pRowStrLen[nCurCol] = iRowStrLen<1024?iRowStrLen:1024;
               // printf("%s\t", sql_reslut); 
                if (pszFieldType[nCurCol] == 0x08 || pszFieldType[nCurCol] == 0x18) {
                    iCurPlace += 4;
                }

                szOldCol[(nCurCol / 8)] = szOldCol[(nCurCol / 8)] | (1 << (nCurCol % 8));
                iCurPlace += iRowStrLen + iRet;
                
            }

            nCurCol++;
            nRealCol++;
        }

        //printreslut("#");
        if(strlen(sql_reslut)>0){
                DBOracleReslut(sql_reslut,pProxyInfo);
        }

        //memset(szOldCol, 0, sizeof(szOldCol));
        //memcpy(szOldCol, szCol, nColLen);

        nRowNum++;
        if (iCurPlace + sizeof (ROWSPLIT) > iRowResultLen) {
            return -1;
        }
        
        if(iversion == 11 && istoad == 1 && *(pszRowResultData + iCurPlace) == 0x07){
            iCurPlace+=1;
            //nColLen = GetColStrLen(nMaxColNum);
            
            if( pData0601->szMagic1[0] == 0x02 && pData0601->szMagic1[1] == 0x00){
                
            }else{
                memset(szCol, 0, sizeof (szCol));
                memcpy(szCol, pszRowResultData + iCurPlace, 1);
                iCurPlace += 1;
                iCurPlace += 2;
                uColNum = *(pszRowResultData + iCurPlace);
                iCurPlace += 1;
            }
           
            
            if (uColNum <= 0) {
                return -1;
            }else if(uColNum > 64){
                return -1;
            }
            continue;
        }else if(iversion == 11 && istoad == 1 && pData0601->szMagic1[0] == 0x02&& *(pszRowResultData + iCurPlace)!= 0x15){
                if(*(pszRowResultData + iCurPlace + 4) == 0x07){
                    iCurPlace += 5;
                    continue;
                }else{
                    break;
                }
        }else if(iversion == 10 && istoad != 1 && pData0601->szMagic1[0] == 0x0a&& *(pszRowResultData + iCurPlace)== 0x07){
            iCurPlace += 5;
            nRowNum = 0;
            continue;
        }

        ROWSPLIT *pRowSplit = (ROWSPLIT*) (pszRowResultData + iCurPlace);

        if (pRowSplit->uId != 0x15 || pRowSplit->uColNum > nGlobMaxColNum) {
            break;
        }
        iCurPlace += sizeof (ROWSPLIT);


        nColLen = GetColStrLen(nGlobMaxColNum);
        memset(szCol, 0, sizeof (szCol));
        memcpy(szCol, pszRowResultData + iCurPlace, nColLen);
        if (0 != pRowSplit->uColNum) {
            uColNum = pRowSplit->uColNum;
        }
        //if(nColLen == 1 && *(pszRowResultData + iCurPlace + 1) == 0x00){
        //    iCurPlace++;
        //}
        //iCurPlace += nColLen + 1;
        iCurPlace+=nColLen;
        while(*(pszRowResultData + iCurPlace) != 0x07){
            iCurPlace+=1;
            if(iCurPlace>iRowResultLen)
                return iCurPlace;
        }
        iCurPlace+=1;
        isColCode = 1;
    }

    return iCurPlace;
}

/***************************************************************
Function:       DecodeTnsData1019
Description:    解析0x1019结构包
Input:                      
                1. PACKET_INFO *pPacketInfo     包相关信息
                2. char *pszTnsFieldInfo       数据流指针
                3. int iTnsFieldInfolen            数据长度
Output:   
                输出数据的字段信息
Return:  
                >0   成功,返回包长度
                <0  失败
Others:         
 ***************************************************************/
int DecodeTnsData1019(char *pszTnsFieldInfo, int iTnsFieldInfolen, PPROXYINFO pProxyInfo) {
    char pszField[1024];
    int iFieldLen;
    int iFieldNum = 0;
    unsigned int iCurPlace = 0;
    int iRet;
    int iCurFieldNum = 0;
    int iNotFindSplit = 0;

    if (iTnsFieldInfolen < sizeof (DATA1019)) {
        return 0;
    }
    nMaxColNum = 0;

    if(iversion == 11 && istoad == 1){
        DATA1019_TOAD *pData1019 = (DATA1019_TOAD *) pszTnsFieldInfo;

        memset(pszFieldType, 0, sizeof (pszFieldType));

        iFieldNum = pData1019->nFieldNum;
        nMaxColNum = iFieldNum;

        iCurPlace = sizeof (DATA1019_TOAD);
    }else if(iversion != 9){
        DATA1019 *pData1019 = (DATA1019 *) pszTnsFieldInfo;

        memset(pszFieldType, 0, sizeof (pszFieldType));

        iFieldNum = pData1019->nFieldNum;
        nMaxColNum = iFieldNum;

        iCurPlace = sizeof (DATA1019);
    }else if(iversion == 9 && bitversion == 64){
        DATA1019_9i_64 *pData1019 = (DATA1019_9i_64 *) pszTnsFieldInfo;

        memset(pszFieldType, 0, sizeof (pszFieldType));

        iFieldNum = pData1019->nFieldNum;
        iCurPlace = sizeof (DATA1019_9i_64);
        if(iFieldNum == 0x4b){
            iFieldNum = *(&(pData1019->nFieldNum)-1);
            iCurPlace --;
        }
        nMaxColNum = iFieldNum;

        
    }else {
        DATA1019_9i *pData1019 = (DATA1019_9i *) pszTnsFieldInfo;

        memset(pszFieldType, 0, sizeof (pszFieldType));

        iFieldNum = pData1019->nFieldNum;
        nMaxColNum = iFieldNum;

        iCurPlace = sizeof (DATA1019_9i);
    }
    
    while (iFieldNum > 0) {
        if(bitversion == 32){
            FIELD_INFO *pFieldInfo;
            pFieldInfo = (FIELD_INFO*) (pszTnsFieldInfo + iCurPlace);
            pszFieldType[iCurFieldNum] = pFieldInfo->uType;
            iCurPlace += sizeof (FIELD_INFO);
        }else{
            if(iversion != 9){
                FIELD_INFO_64 *pFieldInfo;
                pFieldInfo = (FIELD_INFO_64*) (pszTnsFieldInfo + iCurPlace);
                pszFieldType[iCurFieldNum] = pFieldInfo->uType;
                iCurPlace += sizeof (FIELD_INFO_64);
            }else{
                FIELD_INFO_64_9i *pFieldInfo;
                pFieldInfo = (FIELD_INFO_64_9i*) (pszTnsFieldInfo + iCurPlace);
                pszFieldType[iCurFieldNum] = pFieldInfo->uType;
                iCurPlace += sizeof (FIELD_INFO_64_9i);
                if(iNotFindSplit == 1){
                    iNotFindSplit = 0;
                    iCurPlace += 4;
                }
                
                if(*(pszTnsFieldInfo + iCurPlace) == *(pszTnsFieldInfo + iCurPlace+2) && *(pszTnsFieldInfo + iCurPlace) == *(pszTnsFieldInfo + iCurPlace+3)){
                    iCurPlace += 3;
                }
            }
        }
        if (iCurPlace > iTnsFieldInfolen) {
                return iCurPlace;
        }
        memset(pszField, 0, sizeof (pszField));
        iRet = ParseStrbyLen(pszTnsFieldInfo + iCurPlace, iTnsFieldInfolen - iCurPlace, pszField, &iFieldLen);

        printreslut("%s|", pszField);

        if(iversion != 9 &&isjdbc==0){
            if(iversion == 11 && bitversion== 64 )
                //iCurPlace+=4;   modify by lijy at 2015-10-27
                iCurPlace += iFieldLen + iRet + 10;  
        }else if(isjdbc==1){
            iCurPlace += iFieldLen + iRet + 12;
        }else{
            if(bitversion== 64 ){
                iCurPlace += iFieldLen + iRet + 2;
                if(*(unsigned char *)(pszTnsFieldInfo + iCurPlace) == 0x02 &&
                   *(unsigned char *)(pszTnsFieldInfo + iCurPlace + 1) == 0x00 &&
                   *(unsigned char *)(pszTnsFieldInfo + iCurPlace + 2) == 0x00 &&
                   *(unsigned char *)(pszTnsFieldInfo + iCurPlace + 3) == 0x81 
                ){
                    iCurPlace += 4;
                }else{
                    iNotFindSplit = 1;
                }
                
            }else{
                iCurPlace += iFieldLen + iRet + 8;
            }   
        }
        iFieldNum--;
        iCurFieldNum++;
    }
    //printreslut("#");
	if(strlen(sql_reslut)>0){
                DBOracleReslut(sql_reslut,pProxyInfo);
            }
    // 07 00 00 00 07 78 6a 05 1a 0f 0b 0c
    if(iversion != 9){
        iCurPlace += 28;
        if(istoad == 1 && iversion == 11){
            iCurPlace += 4;
        }
    }else{
        iCurPlace += 12;
    }
    return iCurPlace;
}
int DecodeTnsData1019_jdbc(char *pszTnsFieldInfo, int iTnsFieldInfolen, PPROXYINFO pProxyInfo) {
    char pszField[1024];
    int iFieldLen;
    int iFieldNum = 0;
    unsigned int iCurPlace = 0;
    int iRet;
    int iCurFieldNum = 0;

    if (iTnsFieldInfolen < sizeof (DATA1019)) {
        return 0;
    }
    nMaxColNum = 0;

    {
        //DATA1019_idbc *pData1019 = (DATA1019_idbc *) pszTnsFieldInfo;
        while(*(pszTnsFieldInfo+iCurPlace)!=0x01 || *(pszTnsFieldInfo+iCurPlace+2)!=0x39){
            if (iCurPlace > iTnsFieldInfolen) {
                return iCurPlace;
            }
            iCurPlace++;
        }
        iCurPlace++;
        iFieldNum = *(pszTnsFieldInfo+iCurPlace);
        iCurPlace+=2;
        memset(pszFieldType, 0, sizeof (pszFieldType));

        //iFieldNum = pData1019->nFieldNum;
        nMaxColNum = iFieldNum;

        //iCurPlace = sizeof (DATA1019_idbc);
    }
    while (iFieldNum > 0) {
        
            FIELD_INFO_JDBC *pFieldInfo;
            pFieldInfo = (FIELD_INFO_JDBC*) (pszTnsFieldInfo + iCurPlace);
            pszFieldType[iCurFieldNum] = *(pszTnsFieldInfo + iCurPlace);
            iCurPlace += sizeof (FIELD_INFO_JDBC);
            if(pFieldInfo->uType == 1){
                iCurPlace+=3;
            }else if(pFieldInfo->uType == 0x71){
                iCurPlace+=1;
            }
        
        if (iCurPlace > iTnsFieldInfolen) {
                return iCurPlace;
        }
        memset(pszField, 0, sizeof (pszField));
        iRet = ParseStrbyLen(pszTnsFieldInfo + iCurPlace, iTnsFieldInfolen - iCurPlace, pszField, &iFieldLen);

        printreslut("%s|", pszField);

        
        iCurPlace += iFieldLen + iRet + 2;
        if(*(pszTnsFieldInfo + iCurPlace) == 0x00){
            iCurPlace+=1;
        }else if(*(pszTnsFieldInfo + iCurPlace) == 0x01){
            iCurPlace+=2;
        }
        
        iFieldNum--;
        iCurFieldNum++;
    }
    //printreslut("#");
    if(strlen(sql_reslut)>0){
                DBOracleReslut(sql_reslut,pProxyInfo);
            }
    // 07 00 00 00 07 78 6a 05 1a 0f 0b 0c
    if(iversion != 9){
        iCurPlace += 28;
    }else{
        iCurPlace += 12;
    }
    return iCurPlace;
}

/***************************************************************
Function:       DecodeTnsData0401
Description:    解析0x0401结构包
Input:                      
                1. PACKET_INFO *pPacketInfo         包相关信息
                2. char *pszTnsAckInfo              数据流指针
                3. int iTnsTnsAckInfoLen            数据长度
Output:   
                
Return:  
                0   成功
                -1  失败
Others:         
 ***************************************************************/
int DecodeTnsData0401(char *pszTnsAckInfo, int iTnsTnsAckInfoLen,char *pszQueryStr) {
    if(isjdbc==0){
        int iQueryStrLen=0;
        int data0401Len = 0;
        int uErrMsgLen = 0;
        if(bitversion == 64)
        {
            if(iversion != 9)
            {
                DATA0401_X64* pData0401 = (DATA0401_X64*)pszTnsAckInfo;
                data0401Len = sizeof(DATA0401_X64);
                uErrMsgLen = pData0401->uErrMsgLen;
            }
            else
            {
                DATA0401_X64_9I* pData0401 = (DATA0401_X64_9I*)pszTnsAckInfo;
                data0401Len = sizeof(DATA0401_X64_9I);
                uErrMsgLen = pData0401->uErrMsgLen;
            }
            if(uErrMsgLen <= 0)
            {
               DATA0401_X64_Ex* pData0401Ex = (DATA0401_X64_Ex*)pszTnsAckInfo;
               data0401Len = sizeof(DATA0401_X64_Ex);
               if(iversion == 9)
                   data0401Len-=2;
               uErrMsgLen = pData0401Ex->uErrMsgLen;
                if(strstr(pszTnsAckInfo + data0401Len,"ORA-01465") == NULL)//ORA-01465
                {
                    uErrMsgLen = 0;
                }
            }
        }
        else 
        {
            DATA0401*pData0401 = (DATA0401*)pszTnsAckInfo;
            data0401Len = sizeof(DATA0401);
            uErrMsgLen = pData0401->uErrMsgLen;
        }
        //DATA0401 *pData0401 = (DATA0401 *) pszTnsAckInfo;
        //printf("type: %d\t, Record Num: %d\n", pData0401->uType, pData0401->nRecordNum);

        memset(pszQueryStr, 0, sizeof (pszQueryStr));
        iQueryStrLen = uErrMsgLen;
        //memcpy(&iQueryStrLen , pszTnsAckInfo + sizeof (DATA0401) + 4,4);
        memcpy(pszQueryStr,pszTnsAckInfo + data0401Len,iQueryStrLen);
        //ParseStrbyLen(pszTnsAckInfo + sizeof (DATA0401), iTnsTnsAckInfoLen - sizeof (DATA0401), pszQueryStr, &iQueryStrLen);
        
        if(*pszQueryStr < 0x20 || *pszQueryStr > 0x80 || uErrMsgLen == 0){
            return -1;
        }
        if(strlen(sql_reslut)==0)
                print2log("Result: %s", pszQueryStr);
        char *data = pszQueryStr;
        while(*data != 0){
            if(*data == '\r' || *data == '\n')
                *data=' ';
            data ++;
        }
    }else{
        int iQueryStrLen=*(pszTnsAckInfo+30);
        memcpy(pszQueryStr,pszTnsAckInfo + 31,iQueryStrLen);
        if(*pszQueryStr < 0x20 || *pszQueryStr > 0x80){
            return -1;
        }
        print2log("Result: %s", pszQueryStr);
    }
    return 0;
}

/***************************************************************
Function:       DecodeTnsData
Description:    解析数据类型结构(0x06)
Input:                      
                1. PACKET_INFO *pPacketInfo         包相关信息
                2. char *pszTnsAckInfo              数据流指针
                3. int iTnsTnsAckInfoLen            数据长度
                4. TCPRULELIST *pTcpRuleList        规则链表
Output:   

Return:  
                0   成功
                -1  失败
Others:                
 ***************************************************************/
int iscolumn = 0;

int DecodeTnsData(char *pszTnsDataHeader, int iTnsDataHeaderlen, PPROXYINFO pProxyInfo, int iIsPorxy, int isclient) {
    TNS_DATA_HEADER *pTnsDataHeader = (TNS_DATA_HEADER *) pszTnsDataHeader;//2

    char *pszSelectStr = NULL;

    if (iTnsDataHeaderlen < 0) {
        return -1;
    } else if (iTnsDataHeaderlen < sizeof (TNS_DATA_HEADER)) {
        return 0;
    }

    char *pszTnsData = pszTnsDataHeader + sizeof (TNS_DATA_HEADER);
    unsigned int iTnsDataLen = iTnsDataHeaderlen - sizeof (TNS_DATA_HEADER);

    char pszQueryStr[1024];
    int iQueryStrLen = 0;
    unsigned int nCurPacketSize = 0;

    memset(pszQueryStr, 0, sizeof (pszQueryStr));


    
    if (isclient) {
              // printf("Request: Command = %02x, Sub Command = %02x,sid= %02x\n", 
              //      pTnsDataHeader->command, pTnsDataHeader->sub_command, (unsigned char)*(pszTnsData));    
        if (pTnsDataHeader->command == 0x03 && pTnsDataHeader->sub_command == 0x02) {//???
            DecodeTnsData(pszTnsData + sizeof (DATA0302), iTnsDataLen - sizeof (DATA0302), pProxyInfo, iIsPorxy, isclient);
        } else if (pTnsDataHeader->command == 0x03 && pTnsDataHeader->sub_command == 0x03) {//???
            //nCurPacketSize = DecodeTnsData0303(pszTnsData, iTnsDataLen,pnethead);            
            nCurPacketSize = DecodeTnsData035e(pszTnsData, iTnsDataLen, pProxyInfo);
            return 0;
        } else if (pTnsDataHeader->command == 0x03 && pTnsDataHeader->sub_command == 0x09) {
            //关闭系统
            print2log("client ask to close\n");
            UserLogout(pProxyInfo);
            if(strlen(sql_reslut)>0){
                DBOracleReslut(sql_reslut,pProxyInfo);
            }
            nCurPacketSize = sizeof (DATA0309);
        } else if (pTnsDataHeader->command == 0x03 && pTnsDataHeader->sub_command == 0x73) {
            return 0;
        } else if (pTnsDataHeader->command == 0x03 && pTnsDataHeader->sub_command == 0x76) {
            nCurPacketSize = DecodeTnsData0376(pszTnsData, iTnsDataLen, pProxyInfo);
            return 0;
        } else if (pTnsDataHeader->command == 0x03 && pTnsDataHeader->sub_command == 0x3b) {
            nCurPacketSize = sizeof (DATA033b);
        } else if (pTnsDataHeader->command == 0x03 && pTnsDataHeader->sub_command == 0x47) {
            nCurPacketSize = DecodeTnsData035e(pszTnsData, iTnsDataLen, pProxyInfo);
            return 0;
        } else if (pTnsDataHeader->command == 0x03 && pTnsDataHeader->sub_command == 0x5e) {  
            nCurPacketSize = DecodeTnsData035e(pszTnsData, iTnsDataLen, pProxyInfo);
            return 0;
        } else if (pTnsDataHeader->command == 0x11 && pTnsDataHeader->sub_command == 0x69) {
            int find_035e = 0;
            int i = 0 ;
            for(i = 0 ; i < iTnsDataLen-1 ; i ++){
                if(pszTnsData[i] == 0x03 && pszTnsData[i+1] == 0x5e){
                    nCurPacketSize = i;
                    find_035e = 1;
                }
            }
            if(find_035e == 0)
                nCurPacketSize = sizeof (DATA1169);
            
        } else if (pTnsDataHeader->command == 0x11 && pTnsDataHeader->sub_command == 0x6b) {
            nCurPacketSize = sizeof (DATA116b);
        } else if (pTnsDataHeader->command == 0x11 && pTnsDataHeader->sub_command == 0x78) {
            //???
            nCurPacketSize = sizeof (DATA1178);
        } else {
            return 0;
        }

    } else {
        //printf("Response: Command = %02x, Sub Command = %02x\n",
         //       pTnsDataHeader->command, pTnsDataHeader->sub_command); /**/
        
        
        if (pTnsDataHeader->command == 0x04 && pTnsDataHeader->sub_command == 0x01) {
            char pszQueryStr[1024];
            memset(pszQueryStr,0,sizeof(pszQueryStr));
            if(DecodeTnsData0401(pszTnsData, iTnsDataLen,pszQueryStr) == -1)
                return 0;
            //printf("%s\n",sql_reslut);
            if(strlen(sql_reslut)>0)
                DBOracleReslut(sql_reslut,pProxyInfo);
            else
            {
                if(strstr(pszQueryStr,"ORA-01403") == NULL)
                    DBOracleReslut(pszQueryStr,pProxyInfo);
            }
            return 0;
        } else if (pTnsDataHeader->command == 0x04 && pTnsDataHeader->sub_command == 0x02) {
            char pszQueryStr[1024];
            memset(pszQueryStr,0,sizeof(pszQueryStr));
            if(DecodeTnsData0401(pszTnsData, iTnsDataLen,pszQueryStr) == -1)
                return 0;
            //printf("%s\n",sql_reslut);
             if(strlen(sql_reslut)>0)
                DBOracleReslut(sql_reslut,pProxyInfo);
            else
            {
                if(strstr(pszQueryStr,"ORA-01403") == NULL)
                    DBOracleReslut(pszQueryStr,pProxyInfo);
            }
            return 0;
        } else if (pTnsDataHeader->command == 0x04 && pTnsDataHeader->sub_command == 0x05) {
            char pszQueryStr[1024];
            memset(pszQueryStr,0,sizeof(pszQueryStr));
            if(DecodeTnsData0401(pszTnsData, iTnsDataLen,pszQueryStr) == -1)
                return 0;
            //printf("%s\n",sql_reslut);
            if(strlen(sql_reslut)>0)
                DBOracleReslut(sql_reslut,pProxyInfo);
            else
            {
                if(strstr(pszQueryStr,"ORA-01403") == NULL)
                    DBOracleReslut(pszQueryStr,pProxyInfo);
            }
            return 0;
        } else if (pTnsDataHeader->command == 0x06 && pTnsDataHeader->sub_command == 0x00) {
            if(iversion == 11&& istoad == 1){
                nCurPacketSize = DecodeTnsData0601(pszTnsData, iTnsDataLen,pProxyInfo);
            }else{
                nCurPacketSize = DecodeTnsData0600(pszTnsData, iTnsDataLen,pProxyInfo);
            }
        } else if (pTnsDataHeader->command == 0x06 && pTnsDataHeader->sub_command == 0x01) {
            nCurPacketSize = DecodeTnsData0601(pszTnsData, iTnsDataLen,pProxyInfo);
        } else if (pTnsDataHeader->command == 0x06 &&(pTnsDataHeader->sub_command == 0x02 || pTnsDataHeader->sub_command == 0x0a  || pTnsDataHeader->sub_command == 0x22)) {
            if(bitversion == 64){
                nCurPacketSize = DecodeTnsData0602_64(pszTnsData, iTnsDataLen,pProxyInfo);
            }else if(iversion == 11&& istoad == 1){
                nCurPacketSize = DecodeTnsData0601(pszTnsData, iTnsDataLen,pProxyInfo);
            }else{
                nCurPacketSize = DecodeTnsData0602(pszTnsData, iTnsDataLen,pProxyInfo);
            }
        } else if (pTnsDataHeader->command == 0x06 && pTnsDataHeader->sub_command == 0x1a){
            nCurPacketSize = DecodeTnsData061a(pszTnsData, iTnsDataLen,pProxyInfo);
        } else if (pTnsDataHeader->command == 0x08 && pTnsDataHeader->sub_command == 0x05) {
            nCurPacketSize = sizeof (DATA0805);
        } else if (pTnsDataHeader->command == 0x08 && (pTnsDataHeader->sub_command == 0x01 || pTnsDataHeader->sub_command == 0xa2 || pTnsDataHeader->sub_command == 0x7a || pTnsDataHeader->sub_command == 0x94 || pTnsDataHeader->sub_command == 0x9c || pTnsDataHeader->sub_command == 0x80 || pTnsDataHeader->sub_command == 0x9a)) {
            ParseStrbyLen(pszTnsData + 1, iTnsDataLen - 1, pszQueryStr, &iQueryStrLen);
            
            if(strstr(pszQueryStr,"Oracle Database 10g") != NULL){
                iversion = 10;
            }else if(strstr(pszQueryStr,"Oracle Database 11g") != NULL){
                iversion = 11;
            }else if(strstr(pszQueryStr,"Oracle9i") != NULL){
                iversion = 9;
            }
            
            if(strstr(pszQueryStr, "64bit Production") != NULL){
                bitversion = 64;
            }
            printf("Db Version: %s\n", pszQueryStr);
            return 0;
        } else if (pTnsDataHeader->command == 0x09 && pTnsDataHeader->sub_command == 0x01) {

            //关闭系统
            //printf("关闭成功\n");
            return 0;
        } /*else if (pTnsDataHeader->command == 0x10 && (pTnsDataHeader->sub_command == 0x17 ||pTnsDataHeader->sub_command == 0x19||pTnsDataHeader->sub_command == 0x1d)) {
            
            memset(sql_reslut,0,sizeof(sql_reslut));
            if(isjdbc==0)
                nCurPacketSize = DecodeTnsData1019(pszTnsData, iTnsDataLen,pProxyInfo);
            else
                nCurPacketSize = DecodeTnsData1019_jdbc(pszTnsData, iTnsDataLen,pProxyInfo);
            iscolumn =1;
        }*/else {
            return 0;
        }
        if (iQueryStrLen > 0) {
            print2log("Query:%s\n", pszQueryStr);
        }
    }

    //解析下一命令
    if(nCurPacketSize < 0){
        return 0;
    }
    if (iTnsDataLen > nCurPacketSize) {
        DecodeTnsData(pszTnsData + nCurPacketSize, iTnsDataLen - nCurPacketSize, pProxyInfo, iIsPorxy, isclient);
    }
    return 0;
}

#pragma pack(1)

typedef struct stConnVer {
    struct stConnVer* pNext;
    PROXYINFO Conn;
    unsigned char ucVer[2]; //10g: 0x01 0x39 ,11g: 0x01 0x3a
} STCONNVER, *PSTCONNVER;
#pragma pack()

PSTCONNVER g_pConnVerHead = NULL;

int AddConn(PPROXYINFO pProxyInfo, unsigned char* pucVer) {
    if (NULL == FindVer(pProxyInfo)) {
        PSTCONNVER pst = (PSTCONNVER) malloc(sizeof (STCONNVER));
        memset(pst, 0, sizeof (STCONNVER));
        pst->Conn = *pProxyInfo;
        memcpy(pst->ucVer, pucVer, 2);
        if (g_pConnVerHead == NULL) {
            g_pConnVerHead = pst;
        } else {
            pst->pNext = g_pConnVerHead;
            g_pConnVerHead = pst;
        }
    }
    return 1;
}

int DelConn(PPROXYINFO pProxyInfo) {
    PSTCONNVER pst = g_pConnVerHead;
    PSTCONNVER pPre = g_pConnVerHead;
    if (pst == NULL) {
        return 0;
    }

    while (pst) {
        if (pst == g_pConnVerHead) {
            g_pConnVerHead = g_pConnVerHead->pNext;
            free(pst);
            return 1;
        } else {
            pPre->pNext = pst->pNext;
            free(pst);
            return 1;
        }

        if (pPre != pst) {
            pPre = pPre->pNext;
        }
        pst = pst->pNext;
    }
    return 0;
}

unsigned char* FindVer(PPROXYINFO pProxyInfo) {
    PSTCONNVER pst = g_pConnVerHead;
    if (pst == NULL) {
        return NULL;
    }

    while (pst) {
        if (0 == memcmp((char*) (&(pst->Conn)), (char*) pProxyInfo, sizeof (PROXYINFO))) {
            return pst->ucVer;
        }
    }
    return NULL;
}
int issplit = 0;
int issplit_07db = 0;
int issplit_07db_split = 0;
int totalLen = 0;
int totalOffset = 0;
char totalBuffer[BUFSIZE];
unsigned char lastucType=0;

char sznewHost[256] = {0};
int inewPort = 0;

int GetNewConnet(char * host, int * port){
    if(inewPort <= 0){
        return 0;
    }
    strcpy(host,sznewHost);
    *port = inewPort;
    return 1;
}

int OracleFitler(PPROXYINFO pProxyInfo ,unsigned char *indata, const  int inlen, int iIsPorxy, int isclient, char *outdata ,int *outlen) {
    PSTORACLEHEADER pstHeader = NULL;
    pstHeader = (PSTORACLEHEADER) indata;
    if (inlen == sizeof (STORACLEHEADER)) {
        return 0;
    }
    int len = htons(pstHeader->usLength);
    
    /*if(issplit == 0 && (len > inlen || (*indata==0x07 && *(indata+1) == 0xdb) && inlen == 0x07db)){
        memset(totalBuffer,0,sizeof(totalBuffer));
        memcpy(totalBuffer,indata,inlen);
        totalOffset = inlen;
        totalLen = len;
        issplit = 1;
        if((*indata==0x07 && *(indata+1) == 0xdb) && inlen == 0x07db)
           issplit_07db = 1;
        lastucType=pstHeader->ucType;
        return 0;
    }else if(issplit == 1 && totalOffset+inlen < BUFSIZE){
        if(issplit_07db == 1){
            if((*indata==0x07 && *(indata+1) == 0xdb) && inlen == 0x07db){
                indata = indata + 10;
                memcpy(totalBuffer+totalOffset,indata,inlen-10);
                totalOffset += inlen-10;
                return 0;
            }else if(len > inlen && issplit_07db_split == 0){
                indata = indata + 10;
                memcpy(totalBuffer+totalOffset,indata,inlen-10);
                totalOffset += inlen-10;
                issplit_07db_split = 1;
                return 0;
            }else if(issplit_07db_split == 1){
                issplit_07db_split = 0;
                issplit_07db = 2;
                indata = indata;
                memcpy(totalBuffer+totalOffset,indata,inlen);
                totalOffset += inlen;
                
            }else{
            
                issplit_07db = 2;
                indata = indata + 10;
                memcpy(totalBuffer+totalOffset,indata,inlen-10);
                totalOffset += inlen-10;
            }
            indata = totalBuffer;
            len = totalOffset;
            issplit = 0;
            pstHeader->ucType=lastucType;
        }else{
            memcpy(totalBuffer+totalOffset,indata,inlen);
            totalOffset += inlen;
            if(totalOffset >= totalLen){
                indata = totalBuffer;
                len = totalLen;
                issplit = 0;
                pstHeader->ucType=lastucType;
            }else{
                
                return 0;
            }
        }
    }
    totalOffset=0;
    totalLen=0;
    char inBuffer[BUFSIZE];
    int bufferLen = 0;
    memset(inBuffer,0,sizeof(inBuffer));
    if(*indata==0x07 && *(indata+1) == 0xdb && issplit_07db == 0){
        memcpy(inBuffer+bufferLen,indata,0x07db);
        indata = indata + 0x07db;
        bufferLen +=0x07db;
        while(*indata==0x07 && *(indata+1) == 0xdb){
            indata = indata + 10;
            if(bufferLen+0x07db - 10 > BUFSIZE && bufferLen+0x07db - 10 > inlen)
                break;
            
            memcpy(inBuffer+bufferLen,indata,0x07db-10);
            indata = indata + 0x07db - 10;
            bufferLen = bufferLen + 0x07db - 10;
        }
        unsigned int templen = *indata * 256 + *(indata+1);
        
        if(templen > 10 && bufferLen+templen - 10 < BUFSIZE && bufferLen+templen - 10 < inlen){
            indata = indata + 10;
            memcpy(inBuffer+bufferLen,indata,templen-10);
            indata = indata + templen - 10;
            bufferLen = bufferLen + templen - 10;
        }
        indata = inBuffer;
        len = bufferLen;
    }else{
        issplit_07db = 0;
    }*/
    switch (pstHeader->ucType) {
        case TNS_TYPE_DATA:
        {
            
            int ret = DecodeTnsData(indata + sizeof (STORACLEHEADER) + 2, len - sizeof (STORACLEHEADER) - 2, pProxyInfo, iIsPorxy, isclient);
                
        }
            break;
        case TNS_TYPE_CONNECT:
        {
            TNS_CONNECT_HEADER *psTnsConnHeader = (TNS_CONNECT_HEADER *) (pstHeader + 1);
            char *connect = (char*) (psTnsConnHeader + 1);
            printf("connect:%s\n", connect);
            //if(strstr(connect,"HOST=__jdbc__")){
            //    isjdbc = 1;
            //}
            if(strstr(connect,"Toad")){
                istoad = 1;
            }
            else if(strstr(connect,"plsqldev"))
            {
                isplsql = 1;
            }
        }
        break;
        case TNS_TYPE_REDIRECT:
        {
            char connect[1024] = {0};
            char oldconent[1024] = {0};
            char *pData = pstHeader + 1;
            unsigned short len = 0;
            unsigned short oldlen = 0;
            char *p = NULL;
            print2log("TNS_TYPE_REDIRECT\n");
            
            memcpy(((char*)&oldlen),pData+1,1);
            memcpy(((char*)&oldlen+1),pData,1);
            
            if(oldlen > htons(pstHeader->usLength)){
                PREDIRECT_DATA predirect_data = (PREDIRECT_DATA)indata;
                
                if (predirect_data->headData.ucType == TNS_TYPE_DATA) {
                    pData = predirect_data + 1;
                    strcpy(oldconent,pData);
                    pData += strlen(oldconent)+1;
                    
                    if(strstr(oldconent,"(HOST=") != NULL){
                        p = strstr(oldconent,"(HOST=");
                        p += 6;
                        strcpy(sznewHost,p);
                        p = strstr(sznewHost,")");
                        if(p != NULL){
                            *p = 0;
                        }
                        print2log("HOST=%s\n",sznewHost);
                    }
                    if(strstr(oldconent,"(PORT=") != NULL){
                        char szPort[256] = {0};
                        p = strstr(oldconent,"(PORT=");
                        p += 6;
                        strcpy(szPort,p);
                        p = strstr(szPort,")");
                        if(p != NULL){
                            *p = 0;
                        }
                        inewPort = atoi(szPort);
                        print2log("PORT=%s\n",szPort);
                    }

                    pProxyInfo->clientPort = getNewProt();
                    sprintf(connect,"(ADDRESS=(PROTOCOL=tcp)(HOST=%s)(PORT=%d))",pProxyInfo->auth.pr_proxyHost,pProxyInfo->clientPort);
                    len = strlen(connect);
                    print2log("connect=%s\n",connect);
                    
                    *outlen = sizeof(REDIRECT_DATA) + len + 1 + strlen(pData);
                    predirect_data->headRedtrect.usLength =ntohs(10);
                    predirect_data->usLengthRedtrect = htons(len + 1 + strlen(pData));
                    predirect_data->headData.usLength = htons(len + 1 + strlen(pData) + 10);
                    predirect_data->usLengthData = htons(0x40);
                    p = outdata;
                    memcpy(p,predirect_data,sizeof(REDIRECT_DATA));    
                    p += sizeof(REDIRECT_DATA);
                    memcpy(p,connect,len);
                    p+= len;
                    *p = 0;
                    p ++;
                    memcpy(p,pData,strlen(pData));
                    return 1;
                }
                
                return 0;
            }
            
            pData += sizeof(unsigned short);
            memcpy(oldconent,pData,oldlen);
            
            if(strstr(oldconent,"(HOST=") != NULL){
                p = strstr(oldconent,"(HOST=");
                p += 6;
                strcpy(sznewHost,p);
                p = strstr(sznewHost,")");
                if(p != NULL){
                    *p = 0;
                }
            }
            if(strstr(oldconent,"(PORT=") != NULL){
                char szPort[256] = {0};
                p = strstr(oldconent,"(PORT=");
                p += 6;
                strcpy(szPort,p);
                p = strstr(szPort,")");
                if(p != NULL){
                    *p = 0;
                }
                inewPort = atoi(szPort);
            }
            
            pProxyInfo->clientPort = getNewProt();
            sprintf(connect,"(ADDRESS=(PROTOCOL=tcp)(HOST=%s)(PORT=%d))",pProxyInfo->auth.pr_proxyHost,pProxyInfo->clientPort);
            len = strlen(connect);
            
            *outlen = sizeof(STORACLEHEADER) + sizeof(unsigned short) + len;
            pstHeader->usLength =ntohs(*outlen);
            memcpy(outdata,pstHeader,sizeof(STORACLEHEADER));          
            memcpy(outdata+sizeof(STORACLEHEADER),((char*)&len+1),1);
            memcpy(outdata+sizeof(STORACLEHEADER)+1,((char*)&len),1);
            memcpy(outdata+sizeof(STORACLEHEADER)+sizeof(unsigned short),connect,len);
            
            return 1;
        }
       
        default:
            break;
    }
    return 0;
}

int DBOracleOperate(char* szSQL, PPROXYINFO pProxyInfo) {
    isquery = 1;
    int i = 0;
    print2log("SQL:%s\n",szSQL);
    for(i = 0 ; i < strlen(szSQL) ; i ++){
        if(szSQL[i] == '\r' || szSQL[i]=='\n'){
            szSQL[i] = ' ';
        }
    }
    strcpy(pProxyInfo->lineOutBuffer,szSQL);
    InsertUseractivitylog(pProxyInfo);
    
    return 0;
}

int DBOracleReslut(char* szData, PPROXYINFO pProxyInfo) {
    print2log("SQL Reslut:%s\n",szData);
    printf("%s\n",szData);
    int i = 0;
    for(i = 0 ; i < strlen(szData) ; i ++){
        if(szData[i] == '\r' || szData[i]=='\n'){
            szData[i] = ' ';
        }
    }
    strcpy(pProxyInfo->lineOutBuffer,szData);
    InsertUseractivityoutlog(pProxyInfo);
    memset(sql_reslut,0,sizeof(sql_reslut));
    return 0;
}

int print2log(const char* format, ...) {
    static char sprint_buf[1024 * 20];
    va_list args;
    int n;
    FILE *fa;

    fa = fopen("/etc/oracleproxy.log", "a+");
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
int getNewProt() {
    int prot = 10500;
    while (1) {
        int conn_socket;
        struct sockaddr_in server;
        conn_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (conn_socket < 0) {
            return -1;
        }
        memset(&server, 0, sizeof (struct sockaddr_in));
        server.sin_addr.s_addr = inet_addr("127.0.0.1");
        server.sin_family = AF_INET;
        server.sin_port = htons(prot);
        if (connect(conn_socket, (struct sockaddr*) &server, sizeof (server)) == -1) {
            break;
        }
        close(conn_socket);
        prot++;
    }
    return prot;
}