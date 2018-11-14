/* 
 * File:   proxyinc.h
 * Author: r9canbo
 *
 * Created on July 30, 2012, 3:52 PM
 */

#ifndef PROXYINC_H
#define	PROXYINC_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct _NETHEAD{
    char serverIP[20];
    int serverPort;
    char serverMAC[50];
    char clientIP[20];
    int clientPort;
    char clientMAC[50];
}NETHEAD,*PNETHEAD;


#ifdef	__cplusplus
}
#endif

#endif	/* PROXYINC_H */

