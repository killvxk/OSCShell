#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>

#include "oracleproxy.h"

int InitOracleProxy(){
    return 0;
}
/*
 * 处理接受到服务器的数据，并返回给客户端
 * 返回值：
 * 0 不处理直接返回给客户端
 * 1 outdata返回给客户端
 */
int RecvOracleDBServer(PPROXYINFO pProxyInfo ,char *indata, int inlen, char *outdata ,int *outlen){
    return OracleFitler(pProxyInfo,indata,inlen,1,0,outdata,outlen);
}

/*
 * 处理接收到客户端的数据，并返回给服务器
 * 返回值：
 * 0 不处理直接返回给服务器
 * 1 outdata返回给服务器
 */
int RecvOracleDBClient(PPROXYINFO pProxyInfo ,char *indata, int inlen, char *outdata ,int *outlen){
    OracleFitler(pProxyInfo,indata,inlen,1,1,outdata,outlen);
    return 0;
}
