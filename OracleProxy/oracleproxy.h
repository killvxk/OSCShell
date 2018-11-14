/* 
 * File:   oracleproxy.h
 * Author: Administrator
 *
 * Created on 2012年1月14日, 下午2:51
 */

#ifndef ORACLEPROXY_H
#define	ORACLEPROXY_H
#include "../DBRecord/dbrecord.h"


extern int InitOracleProxy();
extern int RecvOracleDBServer(PPROXYINFO pProxyInfo ,char *indata, int inlen, char *outdata ,int *outlen);
extern int RecvOracleDBClient(PPROXYINFO pProxyInfo ,char *indata, int inlen, char *outdata ,int *outlen);
extern int GetNewConnet(char * host, int * port);

#endif	/* ORACLEPROXY_H */

