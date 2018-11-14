/* 
 * File:   main.cpp
 * Author: Administrator
 *
 * Created on 2014年5月19日, 下午1:50
 */
#ifndef SYSLOGEXH
#define SYSLOGEXH

#include <stdio.h>
#include <stdarg.h>
#include <sys/socket.h>
#include<arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

/*
 * 
 */
/******************************************************************************
 * syslog
 *
 * Generate a log message using FMT string and option arguments.
 */
#define	LOG_KERN	(0<<3)	/* kernel messages */
#define	LOG_USER	(1<<3)	/* random user-level messages */
#define	LOG_MAIL	(2<<3)	/* mail system */
#define	LOG_DAEMON	(3<<3)	/* system daemons */
#define	LOG_AUTH	(4<<3)	/* security/authorization messages */
#define	LOG_SYSLOG	(5<<3)	/* messages generated internally by syslogd */
#define	LOG_LPR		(6<<3)	/* line printer subsystem */
#define	LOG_NEWS	(7<<3)	/* network news subsystem */
#define	LOG_UUCP	(8<<3)	/* UUCP subsystem */
#define	LOG_CRON	(9<<3)	/* clock daemon */
#define	LOG_AUTHPRIV	(10<<3)	/* security/authorization messages (private) */
#define	LOG_FTP		(11<<3)	/* ftp daemon */

/*
 * priorities/facilities are encoded into a single 32-bit quantity, where the
 * bottom 3 bits are the priority (0-7) and the top 28 bits are the facility
 * (0-big number).  Both the priorities and the facilities map roughly
 * one-to-one to strings in the syslogd(8) source code.  This mapping is
 * included in this file.
 *
 * priorities (these are ordered)
 */
#define	LOG_EMERG	0	/* system is unusable */
#define	LOG_ALERT	1	/* action must be taken immediately */
#define	LOG_CRIT	2	/* critical conditions */
#define	LOG_ERR		3	/* error conditions */
#define	LOG_WARNING	4	/* warning conditions */
#define	LOG_NOTICE	5	/* normal but significant condition */
#define	LOG_INFO	6	/* informational */
#define	LOG_DEBUG	7	/* debug-level messages */
extern int printdblog(const char* format, ...);
void vsyslog(int pri,char* fmt, va_list ap );
void syslogEx(int pri,char* fmt, ... )
{
    va_list ap;

    va_start( ap, fmt );
    vsyslog(pri,fmt, ap );
    va_end( ap );
}

/******************************************************************************
 * vsyslog
 *
 * Generate a log message using FMT and using arguments pointed to by AP.
 */
void  getSyslogInfo(char* ip,int *port)
{
    char host[256]={0};
    FILE *fd = fopen("/etc/syslog.host","r");
    if(fd == NULL)
    	{
    	    printdblog("/etc/syslog.host open failed(%d)",errno);
            return;
    	}
    char* p=NULL;
     if( NULL == fgets( host, sizeof(host), fd ) )
     {
         printdblog("get sysloginfo failed(%d)",errno);
         return;
     }
    else
    {
        p = strchr( host, '\n' );
        if( p )
            *p = 0;
        p = strchr( host, '\r' );
        if( p )
            *p = 0;
    }
    fclose( fd );
    p = strchr( host, ':' );
    if( p )
        *p++ = 0;
    memcpy(ip,host,strlen(host));
    if( p )
        *port=strtoul( p, NULL, 0 );
    else
        *port=514;
}
void vsyslog(int pri,char* fmt, va_list ap )
{
    static char ip[16]={0};
    static int port=0;
	if(strlen(ip) <= 0 || port <= 0)
	    getSyslogInfo(ip,&port);
    if(strlen(ip) <= 3 || port == 0)
    {
    	printdblog("syslog off!");
        return;
    }
    char datagramm[ 1024 ]={0};
	int len = sprintf( datagramm, "<%d>",pri);
    vsnprintf( datagramm + len, 1024 - len, fmt, ap );
    //vsnprintf(datagramm,1024, fmt, ap );
    int socket_desc;    
    socket_desc = socket(AF_INET , SOCK_DGRAM , 0);
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip);;
    server.sin_port = htons(port);
    
    
    if(connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0 )
    {
        printdblog("syslog server connect failed(%s[%d])\n",strerror(errno),errno);
		close(socket_desc);
        return;
    }
    if(send(socket_desc, datagramm, strlen(datagramm), 0) < 0)
    {
        printdblog("syslog info send failed(%s[%d])\n",strerror(errno),errno);
    }
	close(socket_desc);
}
#endif