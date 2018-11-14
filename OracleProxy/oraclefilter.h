/* 
 * File:   oraclefilter.h
 * Author: Administrator
 *
 * Created on 2012年2月20日, 上午9:40
 */

#ifndef ORACLEFILTER_H
#define	ORACLEFILTER_H
#include "../DBRecord/dbrecord.h"

//extern int InitMysqlFitler(int port);
int OracleFitler(PPROXYINFO pProxyInfo ,unsigned char *indata, const int inlen,int iIsPorxy,int isclient, char *outdata ,int *outlen);

#pragma pack(1)

#define TNS_TYPE_CONNECT 1
#define TNS_TYPE_ACCEPT 2
#define TNS_TYPE_ACK 3
#define TNS_TYPE_REFUSE 4
#define TNS_TYPE_REDIRECT 5
#define TNS_TYPE_DATA 6
#define TNS_TYPE_NULL 7
#define TNS_TYPE_ABORT 9
#define TNS_TYPE_RESEND 11
#define TNS_TYPE_MARKER 12
#define TNS_TYPE_ATTENTION 13
#define TNS_TYPE_CONTROL 14
#define TNS_TYPE_MAX 19

typedef struct stOracleHeader
{
    unsigned short usLength;
    unsigned short usPacketChecksum;
    unsigned char  ucType;
    unsigned char  ucFlag;
    unsigned short usHeadChecksum;
}STORACLEHEADER,*PSTORACLEHEADER;

typedef struct _tnstype			//定义子协议映射表
{
    int  iTypeNum;
    char pszTypeText[36];
}TNSTYPE;

typedef struct _tnsREDIRECT_DATA{
    STORACLEHEADER headRedtrect;
    unsigned short usLengthRedtrect;
    STORACLEHEADER headData;
    unsigned short usLengthData;
}REDIRECT_DATA,*PREDIRECT_DATA;

typedef struct TNS_header{
    unsigned short length;
    unsigned char p_chksum[2];
    unsigned char type;
    /********packet type*************
    = 0x01(connect)
    = 0x02(accept)
    = 0x05(redirect)
    = 0x06(data)
    ********************************/
    unsigned char flag;
    unsigned char h_chksum[2];
}TNS_HEADER; 

typedef struct TNS_CONNECT_header{
    unsigned char NS_ver[2];
    unsigned char Compatible_ver[2];
    unsigned char ser_opt1;
    unsigned char ser_opt2;
    unsigned char SDU_size[2];
    unsigned char TDU_size[2];
    unsigned char NT_protocol_ch1;
    unsigned char NT_protocol_ch2;
    unsigned char Max_packets[2];
    unsigned char Hardware_1[2];
    unsigned char data_length[2];       //连接字符的长度
    unsigned char Offset[2];            //连接字符的位置,从TNS协议开始
    unsigned char Max_data[4];
    unsigned char flag0;
    unsigned char flag1;

    /*java thin interface has no items and id,
    if it is not jdbc thin, use offset to override these*/
    unsigned char item1[4];
    unsigned char item2[4];
    unsigned char ID[8];
    unsigned char unknown[8];
    /*followed by decode_des_conn_data */
}TNS_CONNECT_HEADER; 


typedef struct TNS_REFUSE_header{
    unsigned char uUserRefuseReason;
    unsigned char uSysRefuseReason;
    unsigned short int data_length;

}TNS_REFUSE_HEADER; 


typedef struct TNS_ACCEPT_header{
    unsigned char NS_ver[2];
    unsigned char ser_opt1;
    unsigned char ser_opt2;
    unsigned char SDU_size[2];
    unsigned char TDU_size[2];
    unsigned char Hardware_1[2];
    unsigned char data_length[2];
    unsigned char Offset[2];
    unsigned char flag0;
    unsigned char flag1;
    unsigned char unknown[8];
    /*no following data*/
}TNS_ACCEPT_HEADER; 

typedef struct TNS_DATA_header{
//    unsigned char data_flag[2];
    unsigned char command;
    unsigned char sub_command;
    /*follow by 
    net8_data       or
    SNS data        or
    end of file     
    */
}TNS_DATA_HEADER;

typedef struct TNS_REDIRECT_header{
    unsigned char data_length[2];
}TNS_REDIRECT_HEADER; 



/*
       01 01 03 00 00 08 00 00 00 00 00 00 00 10 00 00 
       00 00 00 00 00 00 00 00 00 54 03 01 00 00 00 00 
       00 07 08 53 51 4c 2a 50 6c 75 73	
 */


/*
                                        01 02 03 00 00  ................
0100   16 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0110   00 00 00 00 00 00 00 00 00 00 00 00              ............

 */

/*
0040   01 06 05 04 03 02 01 00 49 42 4d 50 43 2f 57 49  ........IBMPC/WI
0050   4e 5f 4e 54 2d 38 2e 31 2e 30 00                 N_NT-8.1.0.	
 */



/*
SQL OPEN
03 02 1c 48 e7 12 00 00 00 00 00
03 02 13 10 e4 12 00 00 00 00 00
*/
typedef struct _data0302
{    
    unsigned char sid;
    unsigned char data[8];
}DATA0302;

/*
QUERY
03 03 08 01 00 00 00 0c de 49 00 16 00 00 00 16  .........I......
53 45 4c 45 43 54 20 55 53 45 52 20 46 52 4f 4d  SELECT USER FROM
20 44 55 41 4c 00                                 DUAL.

03 03 1d 01 00 00 00 b0 d7 49 00 06 00 00 00 06  .........I......
63 6f 6d 6d 69 74                                commit

03 03 14 01 00 00 00 54 dd 49 00 b7 00 00 00 fe  .......T.I......
40 53 45 4c 45 43 54 20 43 48 41 52 5f 56 41 4c  @SELECT CHAR_VAL
55 45 20 46 52 4f 4d 20 53 59 53 54 45 4d 2e 50  UE FROM SYSTEM.P
52 4f 44 55 43 54 5f 50 52 49 56 53 20 57 48 45  RODUCT_PRIVS WHE
52 45 20 20 20 28 55 50 50 45 52 28 27 53 51 4c  RE   (UPPER('SQL
2a 40 50 6c 75 73 27 29 20 4c 49 4b 45 20 55 50  *@Plus') LIKE UP
50 45 52 28 50 52 4f 44 55 43 54 29 29 20 41 4e  PER(PRODUCT)) AN
44 20 20 20 28 28 55 50 50 45 52 28 55 53 45 52  D   ((UPPER(USER
29 20 4c 49 4b 45 20 55 53 45 52 49 44 29 20 4f  ) LIKE USERID) O
52 20 37 28 55 53 45 52 49 44 20 3d 20 27 50 55  R 7(USERID = 'PU
42 4c 49 43 27 29 29 20 41 4e 44 20 20 20 28 55  BLIC')) AND   (U
50 50 45 52 28 41 54 54 52 49 42 55 54 45 29 20  PPER(ATTRIBUTE) 
3d 20 27 52 4f 4c 45 53 27 29 00                 = 'ROLES').
*/
typedef struct _data0303
{
    unsigned char sid;
    unsigned char magic[12];      //需要分析
//    unsigned char udatalen;
}DATA0303;

/*
QUERY_SECOND
03 04 1e 01 00 00 00 01 00 00 00 00 00 00 00
 */
typedef struct _data0304
{
    unsigned char sid;
    unsigned char data[12];
}DATA0304;

/*
FETCH_MORE
功能1: 查询数据时请求下一页
03 05 02 01 00 00 00 0f 00 00 00                 ...........

03 05 16 01 00 00 00 01 00 00 00
 */
typedef struct _data0305
{
    unsigned char sid;
    unsigned char szMagic1[4];
    unsigned char szMagic2[4];
}DATA0305;


/*
HANDSHAKE7
03 08 1f 01 00 00 00
 */
typedef struct _data0308
{
    unsigned char sid;
    unsigned char data[4];
}DATA0308;

/*
    DISCONNECT
*/
typedef struct _data0309
{    
    unsigned char sid;
}DATA0309;

/*
 *	SET_LANG
 0040   03 27 06 01 00 00 00 a2 09 e0 0a 08 01 00 00 41  .'.............A
 0050   4c 54 45 52 20 53 45 53 53 49 4f 4e 20 53 45 54  LTER SESSION SET
 0060   20 4e 4c 53 5f 4c 41 4e 47 55 41 47 45 3d 20 27   NLS_LANGUAGE= '
 0070   53 49 4d 50 4c 49 46 49 45 44 20 43 48 49 4e 45  SIMPLIFIED CHINE
 0080   53 45 27 20 4e 4c 53 5f 54 45 52 52 49 54 4f 52  SE' NLS_TERRITOR
 0090   59 3d 20 27 43 48 49 4e 41 27 20 4e 4c 53 5f 43  Y= 'CHINA' NLS_C
 00a0   55 52 52 45 4e 43 59 3d 20 27 52 4d 42 27 20 4e  URRENCY= 'RMB' N
 00b0   4c 53 5f 49 53 4f 5f 43 55 52 52 45 4e 43 59 3d  LS_ISO_CURRENCY=
 00c0   20 27 43 48 49 4e 41 27 20 4e 4c 53 5f 4e 55 4d   'CHINA' NLS_NUM
 00d0   45 52 49 43 5f 43 48 41 52 41 43 54 45 52 53 3d  ERIC_CHARACTERS=
 00e0   20 27 2e 2c 27 20 4e 4c 53 5f 43 41 4c 45 4e 44   '.,' NLS_CALEND
 00f0   41 52 3d 20 27 47 52 45 47 4f 52 49 41 4e 27 20  AR= 'GREGORIAN' 
 0100   4e 4c 53 5f 44 41 54 45 5f 46 4f 52 4d 41 54 3d  NLS_DATE_FORMAT=
 0110   20 27 44 44 2d 4d 4f 4e 2d 52 52 27 20 4e 4c 53   'DD-MON-RR' NLS
 0120   5f 44 41 54 45 5f 4c 41 4e 47 55 41 47 45 3d 20  _DATE_LANGUAGE= 
 0130   27 53 49 4d 50 4c 49 46 49 45 44 20 43 48 49 4e  'SIMPLIFIED CHIN
 0140   45 53 45 27 20 4e 4c 53 5f 53 4f 52 54 3d 20 27  ESE' NLS_SORT= '
 0150   42 49 4e 41 52 59 27                             BINARY'

 */
typedef struct _data0327
{
    unsigned char sid;
}DATA0327;

/*
 *	DESC_COLS
 */
typedef struct _data032b
{
    unsigned char sid;

}DATA032b;

/*
DB VERSION
03 3b 05 b4 e4 12 00 f4 01 00 00 80 e4 12 00 7c  .;.............|
e4 12 00                                         ...
03 3b 05 e8 f9 95 11 00 08 00 00 50 f9 95 11 4c 
f9 95 11                                            ..

 */
typedef struct _data033b
{
    unsigned char sid;
    unsigned char data[16];
}DATA033b;

/*
FETCH
0040   03 47 09 30 80 00 00 01 00 00 00 00 00 00 00 00  .G.0............
0050   00 00 00 00 00 00 00 00 00 00 00 00 7c 33 01 07  ............|3..
0060   00 00 00 58 69 38 01 02 00 00 00 00 00 00 00 a0  ...Xi8..........
0070   68 38 01 01 00 00 00 00 00 00 00 00 00 00 00 00  h8..............
0080   00 00 00 01 00 00 00 00 00 00 00 00 00 00 00 00  ................
0090   00 00 00 00 00 00 00 00 00 00 00 01 01 00 00 00  ................
00a0   1d 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
00b0   00 00 00 00 54 03 00 00 00 00 00 00              ....T.......

0040   03 47 15 30 80 00 00 01 00 00 00 00 00 00 00 00  .G.0............
0050   00 00 00 00 00 00 00 00 00 00 00 00 7c 33 01 07  ............|3..
0060   00 00 00 58 69 38 01 02 00 00 00 00 00 00 00 a0  ...Xi8..........
0070   68 38 01 01 00 00 00 00 00 00 00 00 00 00 00 00  h8..............
0080   00 00 00 01 00 00 00 00 00 00 00 00 00 00 00 00  ................
0090   00 00 00 00 00 00 00 00 00 00 00 01 01 00 00 00  ................
00a0   f9 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
00b0   00 00 00 00 54 03 00 00 00 00 00 00              ....T.......
 */
typedef struct _data0347
{
    unsigned char sid;
    unsigned char data[121];
}DATA0347;

/*
 *	USER_PASSWD
 0040   03 51 03 48 2b dc 0a 05 00 00 00 c2 0c e0 0a 20  .Q.H+.......... 
 0050   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
 0060   00 00 00 f2 0d e0 0a 0e 00 00 00 31 0f e0 0a 19  ...........1....
 0070   00 00 00 f1 0e e0 0a 07 00 00 00 30 11 00 00 b1  ...........0....
 0080   0f e0 0a 09 00 00 00 c1 0f e0 0a 08 00 00 00 00  ................
 0090   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
 00a0   00 00 00 00 00 00 00 00 00 00 00 73 63 6f 74 74  ...........scott
 00b0   31 45 33 44 34 33 45 34 33 35 41 32 44 33 43 46  1E3D43E435A2D3CF
 00c0   34 45 30 33 38 46 39 42 36 34 43 37 44 31 37 30  4E038F9B64C7D170
 00d0   54 4f 50 53 45 43 2d 4d 59 41 4e 47 4a 50 57 4f  TOPSEC-MYANGJPWO
 00e0   52 4b 47 52 4f 55 50 5c 54 4f 50 53 45 43 2d 4d  RKGROUP\TOPSEC-M
 00f0   59 41 4e 47 4a 50 00 6d 79 61 6e 67 6a 70 33 30  YANGJP.myangjp30
 0100   38 30 3a 32 33 33 36 6a 72 65 77 2e 65 78 65     80:2336jrew.exe

 */
typedef struct _data0351
{
    unsigned char   sid;
    unsigned char   pszMagic1[4];
    unsigned char   uUserNameLen;
    unsigned char   pszMagic2[99];
    //unsigned char pszUserName[uUserNameLen];
    //unsigned char pszUserName[32];
}DATA0351;

/*
 *	CLIENT_ID
 0040   03 52 02 48 2b dc 0a 05 00 00 00 00 00 00 00 00  .R.H+...........
 0050   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
 0060   00 00 00 f2 0d e0 0a 0e 00 00 00 31 0f e0 0a 19  ...........1....
 0070   00 00 00 f1 0e e0 0a 07 00 00 00 30 11 00 00 b1  ...........0....
 0080   0f e0 0a 09 00 00 00 c1 0f e0 0a 08 00 00 00 00  ................
 0090   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 82  ................
 00a0   09 e0 0a 21 00 00 00 80 09 e0 0a 73 63 6f 74 74  ...!.......scott
 00b0   54 4f 50 53 45 43 2d 4d 59 41 4e 47 4a 50 57 4f  TOPSEC-MYANGJPWO
 00c0   52 4b 47 52 4f 55 50 5c 54 4f 50 53 45 43 2d 4d  RKGROUP\TOPSEC-M
 00d0   59 41 4e 47 4a 50 00 6d 79 61 6e 67 6a 70 33 30  YANGJP.myangjp30
 00e0   38 30 3a 32 33 33 36 6a 72 65 77 2e 65 78 65     80:2336jrew.exe

 */
typedef struct _data0352
{
    unsigned char   sid;
    unsigned char   pszMagic1[4];
    unsigned char   uUserNameLen;
    unsigned char   pszMagic2[99];
    //unsigned char pszUserName[uUserNameLen];
}DATA0352;

/*
HANDSHAKE4
03 54 06 f4 41 33 01 f8 41 33 01 01 00 00 00 00  .T..A3..A3......
00 00 00 00 00 00 00 00 00 00 00 d4 e5 12 00 09  ................
00 00 00 09 00 34 6e 84 00                       .....4n..
03 54 04 f4 41 33 01 f8 41 33 01 01 00 00 00 00  .T..A3..A3......
00 00 00 00 00 00 00 00 00 00 00 70 e3 12 00 09  ...........p....
00 00 00 09 00 34 6e 84 00                       .....4n..
 */
typedef struct _data0354
{
    unsigned char sid;
    unsigned char data[38];
}DATA0354;


/*
SQL:一般的sql语句都由这个结构封装    
                                                 03 5e  it<>W..........^
0060   75 61 81 00 00 00 00 00 00 18 b1 09 1b f6 00 00  ua..............
0070   00 e8 0b 58 12 0c 00 00 00 00 00 00 00 18 0c 58  ...X...........X
0080   12 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0090   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
00a0   00 00 00 00 00 1a 0c 58 12 dc b1 09 1b 00 00 00  .......X........
00b0   00 fe 40 2f 2a 20 4f 72 61 63 6c 65 4f 45 4d 20  ..@// OracleOEM 
00c0   2a 2f 20 53 45 4c 45 43 54 20 63 6f 75 6e 74 20  // SELECT count 
00d0   46 52 4f 4d 20 73 79 73 2e 64 62 61 5f 75 6e 75  FROM sys.dba_unu
00e0   73 65 64 5f 63 6f 6c 5f 74 61 62 73 20 57 48 45  sed_col_tabs WHE
00f0   52 45 20 3b 6f 77 6e 65 72 20 3d 20 27 51 53 5f  RE ;owner = 'QS_
0100   4f 53 27 20 41 4e 44 20 74 61 62 6c 65 5f 6e 61  OS' AND table_na
0110   6d 65 20 3d 20 27 41 51 24 5f 51 53 5f 4f 53 5f  me = 'AQ$_QS_OS_
0120   4f 52 44 45 52 53 5f 4d 51 54 41 42 5f 49 27 00  ORDERS_MQTAB_I'.
0130   01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0140   00 00 00 00 00 00 00 00 00 00 00 00 01 00 00 00  ................
0150   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0160  

0040   03 5e 18 29 04 04 00 00 00 00 00 54 42 34 01 36  .^.).......TB4.6
0050   00 00 00 00 7c 33 01 0c 00 00 00 00 00 00 00 30  ....|3.........0
0060   7c 33 01 00 00 00 00 01 00 00 00 00 00 00 00 3c  |3.............<
0070   46 34 01 01 00 00 00 00 00 00 00 00 00 00 00 00  F4..............
0080   00 00 00 00 00 00 00 32 7c 33 01 3c 46 34 01 00  .......2|3.<F4..
0090   00 00 00 36 42 45 47 49 4e 20 44 42 4d 53 5f 41  ...6BEGIN DBMS_A
00a0   50 50 4c 49 43 41 54 49 4f 4e 5f 49 4e 46 4f 2e  PPLICATION_INFO.
00b0   53 45 54 5f 4d 4f 44 55 4c 45 28 3a 31 2c 4e 55  SET_MODULE(:1,NU
00c0   4c 4c 29 3b 20 45 4e 44 3b 00 01 00 00 00 01 00  LL); END;.......
00d0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
00e0   00 00 00 00 00 00 08 00 00 00 00 00 00 00 00 00  ................
00f0   00 00 00 00 00 00 00 00 00 00 

0040   03 5e 14 40 00 00 00 01 00 00 00 00 00 00 00 00  .^.@............
0050   00 00 00 48 1d 40 12 0c 00 00 00 00 00 00 00 78  ...H.@.........x
0060   1d 40 12 00 00 00 00 00 00 00 00 c2 00 00 00 00  .@..............
0070   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0080   00 00 00 00 00 00 00 7a 1d 40 12 00 ce 4f 12 00  .......z.@...O..
0090   00 00 00 00 00 00 00 fa 00 00 00 00 00 00 00 00  ................
00a0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 01  ................
00b0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
00c0   00 00 00
                                
 */
typedef struct _data035e
{
//    unsigned char sid;
    //unsigned char szMagic1[80];
    unsigned char szMagic1[41];
    // unsigned char uDataLen;
    // char          *pszData[uDataLen];
    //unsigned char szMagic2[48];
}DATA035e;

/*
AUTH2 用户名和密码
03 73 03 2c 3f 33 01 06 00 00 00 01 01 00 00 1c  .s.,?3..........
e3 12 00 07 00 00 00 d4 df 12 00 60 e5 12 00 06  ...........`....
79 61 6e 67 6a 70 0d 00 00 00 0d 41 55 54 48 5f  yangjp.....AUTH_
50 41 53 53 57 4f 52 44 20 00 00 00 20 36 38 41  PASSWORD ... 68A
41 34 46 30 31 43 32 46 31 32 30 32 45 30 33 36  A4F01C2F1202E036
33 32 36 42 39 34 45 39 43 36 43 45 41 00 00 00  326B94E9C6CEA...
00 0d 00 00 00 0d 41 55 54 48 5f 54 45 52 4d 49  ......AUTH_TERMI
4e 41 4c 0e 00 00 00 0e 54 4f 50 53 45 43 2d 4d  NAL.....TOPSEC-M
59 41 4e 47 4a 50 00 00 00 00 0f 00 00 00 0f 41  YANGJP.........A
55 54 48 5f 50 52 4f 47 52 41 4d 5f 4e 4d 0c 00  UTH_PROGRAM_NM..
00 00 0c 73 71 6c 70 6c 75 73 77 2e 65 78 65 00  ...sqlplusw.exe.
00 00 00 0c 00 00 00 0c 41 55 54 48 5f 4d 41 43  ........AUTH_MAC
48 49 4e 45 19 00 00 00 19 57 4f 52 4b 47 52 4f  HINE.....WORKGRO
55 50 5c 54 4f 50 53 45 43 2d 4d 59 41 4e 47 4a  UP\TOPSEC-MYANGJ
50 00 00 00 00 00 08 00 00 00 08 41 55 54 48 5f  P..........AUTH_
50 49 44 08 00 00 00 08 32 31 34 34 3a 32 33 36  PID.....2144:236
00 00 00 00 08 00 00 00 08 41 55 54 48 5f 41 43  .........AUTH_AC
4c 04 00 00 00 04 34 34 30 30 00 00 00 00 12 00  L.....4400......
00 00 12 41 55 54 48 5f 41 4c 54 45 52 5f 53 45  ...AUTH_ALTER_SE
53 53 49 4f 4e f0 01 00 00 fe 40 41 4c 54 45 52  SSION.....@ALTER
20 53 45 53 53 49 4f 4e 20 53 45 54 20 4e 4c 53   SESSION SET NLS
5f 4c 41 4e 47 55 41 47 45 3d 20 27 53 49 4d 50  _LANGUAGE= 'SIMP
4c 49 46 49 45 44 20 43 48 49 4e 45 53 45 27 20  LIFIED CHINESE' 
4e 4c 53 5f 54 45 52 52 49 54 4f 40 52 59 3d 20  NLS_TERRITO@RY= 
27 43 48 49 4e 41 27 20 4e 4c 53 5f 43 55 52 52  'CHINA' NLS_CURR
45 4e 43 59 3d 20 27 52 4d 42 27 20 4e 4c 53 5f  ENCY= 'RMB' NLS_
49 53 4f 5f 43 55 52 52 45 4e 43 59 3d 20 27 43  ISO_CURRENCY= 'C
48 49 4e 41 27 20 4e 4c 53 5f 4e 55 40 4d 45 52  HINA' NLS_NU@MER
49 43 5f 43 48 41 52 41 43 54 45 52 53 3d 20 27  IC_CHARACTERS= '
2e 2c 27 20 4e 4c 53 5f 43 41 4c 45 4e 44 41 52  .,' NLS_CALENDAR
3d 20 27 47 52 45 47 4f 52 49 41 4e 27 20 4e 4c  = 'GREGORIAN' NL
53 5f 44 41 54 45 5f 46 4f 52 4d 41 54 40 3d 20  S_DATE_FORMAT@= 
27 44 44 2d 4d 4f 4e 2d 52 52 27 20 4e 4c 53 5f  'DD-MON-RR' NLS_
44 41 54 45 5f 4c 41 4e 47 55 41 47 45 3d 20 27  DATE_LANGUAGE= '
53 49 4d 50 4c 49 46 49 45 44 20 43 48 49 4e 45  SIMPLIFIED CHINE
53 45 27 20 20 4e 4c 53 5f 53 4f 52 54 3d 40 20  SE'  NLS_SORT=@ 
27 42 49 4e 41 52 59 27 20 54 49 4d 45 5f 5a 4f  'BINARY' TIME_ZO
4e 45 3d 20 27 2b 30 38 3a 30 30 27 20 4e 4c 53  NE= '+08:00' NLS
5f 44 55 41 4c 5f 43 55 52 52 45 4e 43 59 20 3d  _DUAL_CURRENCY =
20 27 52 4d 42 27 20 4e 4c 53 5f 54 49 4d 45 40   'RMB' NLS_TIME@
5f 46 4f 52 4d 41 54 20 3d 20 27 48 48 2e 4d 49  _FORMAT = 'HH.MI
2e 53 53 58 46 46 20 41 4d 27 20 4e 4c 53 5f 54  .SSXFF AM' NLS_T
49 4d 45 53 54 41 4d 50 5f 46 4f 52 4d 41 54 20  IMESTAMP_FORMAT 
3d 20 27 44 44 2d 4d 4f 4e 2d 52 52 20 48 48 2e  = 'DD-MON-RR HH.
40 4d 49 2e 53 53 58 46 46 20 41 4d 27 20 4e 4c  @MI.SSXFF AM' NL
53 5f 54 49 4d 45 5f 54 5a 5f 46 4f 52 4d 41 54  S_TIME_TZ_FORMAT
20 3d 20 27 48 48 2e 4d 49 2e 53 53 58 46 46 20   = 'HH.MI.SSXFF 
41 4d 20 54 5a 52 27 20 4e 4c 53 5f 54 49 4d 45  AM TZR' NLS_TIME
53 30 54 41 4d 50 5f 54 5a 5f 46 4f 52 4d 41 54  S0TAMP_TZ_FORMAT
20 3d 20 27 44 44 2d 4d 4f 4e 2d 52 52 20 48 48   = 'DD-MON-RR HH
2e 4d 49 2e 53 53 58 46 46 20 41 4d 20 54 5a 52  .MI.SSXFF AM TZR
27 00 00 00 00 00 00                             '......
 */
typedef struct _data0373
{
    unsigned char sid;
    unsigned char magic[28];
    unsigned char usernamelen;    
    unsigned char *username;
    unsigned char magic1[4];
    unsigned char passwdlen;    
    unsigned char *passwd;
    unsigned char magic2[4];
    unsigned char passwdvaluelen;    
    unsigned char *passwdvalue;
    unsigned char magic3[8];
    unsigned char terminallen;    
    unsigned char *terminal;
    unsigned char magic4[4];
    unsigned char terminalvaluelen;    
    unsigned char *terminalvalue;
    unsigned char magic5[4];
    unsigned char programlen;    
    unsigned char *program;
    unsigned char magic6[4];
    unsigned char programvaluelen;    
    unsigned char *programvalue;
    unsigned char magic7[8];
    unsigned char hostlen;    
    unsigned char *host;
    unsigned char magic8[4];
    unsigned char machinevaluelen;    
    unsigned char *machinevalue;
    unsigned char magic9[8];
    unsigned char authpidlen;    
    unsigned char *authpid;
    unsigned char magic10[4];
    unsigned char authpidvaluelen;    
    unsigned char *authpidvalue;
    unsigned char magic11[8];
    unsigned char authacllen;    
    unsigned char *authacl;
    unsigned char magic12[4];
    unsigned char authaclvaluelen;    
    unsigned char *authaclvalue;
    unsigned char magic13[8];
    unsigned char authalterlen;    
    unsigned char *authalter;
    unsigned char magic14[4];
    unsigned char authalterlvaluelen;    
    unsigned char *authaltervalue;
    unsigned char magic15[4];
}DATA0373;

/*
AUTH1 发送用户名
03 76 02 2c 3f 33 01 06 00 00 00 01 00 00 00 00  .v.,?3..........
d4 12 00 04 00 00 00 d0 d1 12 00 c0 d5 12 00 06  ................
79 61 6e 67 6a 70 0d 00 00 00 0d 41 55 54 48 5f  yangjp.....AUTH_
54 45 52 4d 49 4e 41 4c 0e 00 00 00 0e 54 4f 50  TERMINAL.....TOP
53 45 43 2d 4d 59 41 4e 47 4a 50 00 00 00 00 0f  SEC-MYANGJP.....
00 00 00 0f 41 55 54 48 5f 50 52 4f 47 52 41 4d  ....AUTH_PROGRAM
5f 4e 4d 0c 00 00 00 0c 73 71 6c 70 6c 75 73 77  _NM.....sqlplusw
2e 65 78 65 00 00 00 00 0c 00 00 00 0c 41 55 54  .exe.........AUT
48 5f 4d 41 43 48 49 4e 45 19 00 00 00 19 57 4f  H_MACHINE.....WO
52 4b 47 52 4f 55 50 5c 54 4f 50 53 45 43 2d 4d  RKGROUP\TOPSEC-M
59 41 4e 47 4a 50 00 00 00 00 00 08 00 00 00 08  YANGJP..........
41 55 54 48 5f 50 49 44 08 00 00 00 08 32 31 34  AUTH_PID.....214
34 3a 32 33 36 00 00 00 00                       4:236....
 */
typedef struct _data0376
{
    unsigned char sid;
    unsigned char magic[28];
    unsigned char usernamelen;    
    unsigned char *username;
    unsigned char magic1[4];
    unsigned char terminallen;    
    unsigned char *terminal;
    unsigned char magic2[4];
    unsigned char terminalvaluelen;    
    unsigned char *terminalvalue;
    unsigned char magic3[4];
    unsigned char programlen;    
    unsigned char *program;
    unsigned char magic4[4];
    unsigned char programvaluelen;    
    unsigned char *programvalue;
    unsigned char magic5[8];
    unsigned char hostlen;    
    unsigned char *host;
    unsigned char magic6[4];
    unsigned char machinevaluelen;    
    unsigned char *machinevalue;
    unsigned char magic7[8];
    unsigned char authpidlen;    
    unsigned char *authpid;
    unsigned char magic8[4];
    unsigned char authpidvaluelen;    
    unsigned char *authpidvalue;
    unsigned char magic9[4];
}DATA0376;

/*
0040   03 77 25 fd 04 34 01 09 00 00 00 00 00 00 00 00  .w%..4..........
0050   00 00 00 00 02 00 00 00 09 74 65 73 74 74 61 62  .........testtab
0060   6c 65                                            le
0040   03 77 26 fd 04 34 01 0a 00 00 00 00 00 00 00 00  .w&..4..........
0050   00 00 00 00 02 00 00 00 0a 74 65 73 74 74 61 62  .........testtab
0060   6c 65 31                                         le1
0040   03 77 20 f5 6f 07 07 02 00 00 00 00 00 00 00 00  .w .o...........
0050   00 00 00 00 02 00 00 00 02 74 74                 .........tt
0040   03 77 21 f5 6f 07 07 02 00 00 00 00 00 00 00 00  .w!.o...........
0050   00 00 00 00 02 00 00 00 02 74 74                 .........tt
0040   03 77 22 f5 6f 07 07 07 00 00 00 00 00 00 00 00  .w".o...........
0050   00 00 00 00 02 00 00 00 07 74 65 73 74 63 6f 6c  .........testcol
0060  
desc table;
 */
typedef struct _data0377
{
    unsigned char sid;
    unsigned char szMagic1[4];
    unsigned char uTableNameLen;
    unsigned char szMagic2[16];
//    unsigned char uTableNameLen1;
}DATA0377;

/*
0x1169 + 0x035e
11 69 1b 40 dc 32 01 01 00 00 00 01 00 00 00  
11 69 00 01 01 01 01 02           .............
*/
typedef struct _data1169
{
    unsigned char sid;
    unsigned char data[12];
}DATA1169;

/*
0x116b + 0x035e
11 6b 0c 09 00 00 00 09 00 00 00 01 00 00 00 
*/
typedef struct _data116b
{
    unsigned char sid;
    unsigned char data[12];
}DATA116b;

/*
0x1178 + 0x1169 + 0x035e
0040   11 78 36 04 2d dc 11 01 00 00 00 01 00 00 00 11  .x6.-...........
0050   69 37 04 2d dc 11 01 00 00 00 01 00 00 00 03 5e  i7.-...........^
*/
typedef struct _data1178
{
    unsigned char sid;
    unsigned char data[12];
}DATA1178;

/*
ACK 返回
0040   04 01 00 00 00 01 00 00 00 00 45 07 00 00 00 00  ..........E.....
0050   01 00 44 00 02 00 00 00 00 00 ff ff ff ff 00 00  ..D.............
0060   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 28  ...............(
0070   00 00 00 00 00 00 36 01 00 00 00 00 00 00 24 43  ......6.......$C
0080   21 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  !...............
0090   00 00 00 00 00 00 00 00 00 00 00 00 00 00 22 4f  .............."O
00a0   52 41 2d 30 31 38 36 31 3a 20 ce c4 d7 d6 d3 eb  RA-01861: ......
00b0   b8 f1 ca bd d7 d6 b7 fb b4 ae b2 bb c6 a5 c5 e4  ................
00c0   0a                                               .
0040   04 01 00 00 00 01 00 00 00 00 79 05 00 00 00 00  ..........y.....
0050   01 00 35 00 02 00 00 00 00 00 ff ff ff ff 00 00  ..5.............
0060   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 26  ...............&
0070   00 00 00 00 00 00 36 01 00 00 00 00 00 00 24 43  ......6.......$C
0080   21 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  !...............
0090   00 00 00 00 00 00 00 00 00 00 00 00 00 00 1e 4f  ...............O
00a0   52 41 2d 30 31 34 30 31 3a 20 b2 e5 c8 eb b5 c4  RA-01401: ......
00b0   d6 b5 b6 d4 d3 da c1 d0 b9 fd b4 f3 0a           .............

 */
typedef struct _data0401
{
    unsigned char   szMagic1[4]; //always be 0x00 0x00 0x00 0x01
    unsigned short    nRecordNum; 
    unsigned short  nErrorNum;
    unsigned char   szMagic2[8];
    unsigned char   uType;  //0x00 non op
    //0x01 create table, 0x02 insert,
    //0x06 update,
    //0x07 delete, 0x03 select,
    //0x0c drop table, 0x0f alter table
    //0x2c commit,0x2f begin
    unsigned char   szMagic3[2];
    unsigned short  nObjId;
    unsigned char   szMagic4[19];
    unsigned char   uAckId;
    unsigned char   szMagic5[46+7];
    unsigned char uErrMsgLen;
    // char          *pszMsg[uErrMsgLen];
}DATA0401;
typedef struct _data0401_x64
{
    unsigned char   szMagic1[4]; //always be 0x00 0x00 0x00 0x01
    unsigned short    nRecordNum; 
    unsigned short  nErrorNum;
    unsigned char   szMagic2[8];
    unsigned char   uType;  //0x00 non op
    //0x01 create table, 0x02 insert,
    //0x06 update,
    //0x07 delete, 0x03 select,
    //0x0c drop table, 0x0f alter table
    //0x2c commit,0x2f begin
    unsigned char   szMagic3[2];
    unsigned short  nObjId;
    unsigned char   szMagic4[19];
    unsigned char   uAckId;
    unsigned char   szMagic5[23];
    unsigned char uErrMsgLen;
    // char          *pszMsg[uErrMsgLen];
}DATA0401_X64;
typedef struct _data0401_x64_9i
{
    unsigned char   szMagic1[4]; //always be 0x00 0x00 0x00 0x01
    unsigned short    nRecordNum; 
    unsigned short  nErrorNum;
    unsigned char   szMagic2[8];
    unsigned char   uType;  //0x00 non op
    //0x01 create table, 0x02 insert,
    //0x06 update,
    //0x07 delete, 0x03 select,
    //0x0c drop table, 0x0f alter table
    //0x2c commit,0x2f begin
    unsigned char   szMagic3[2];
    unsigned short  nObjId;
    unsigned char   szMagic4[19];
    unsigned char   uAckId;
    unsigned char   szMagic5[21];
    unsigned char uErrMsgLen;
    // char          *pszMsg[uErrMsgLen];
}DATA0401_X64_9I;
typedef struct _data0401_x64_Ex
{
    unsigned char   szMagic1[4]; //always be 0x00 0x00 0x00 0x01
    unsigned short    nRecordNum; 
    unsigned short  nErrorNum;
    unsigned char   szMagic2[8];
    unsigned char   uType;  //0x00 non op
    //0x01 create table, 0x02 insert,
    //0x06 update,
    //0x07 delete, 0x03 select,
    //0x0c drop table, 0x0f alter table
    //0x2c commit,0x2f begin
    unsigned char   szMagic3[2];
    unsigned short  nObjId;
    unsigned char   szMagic4[19];
    unsigned char   uAckId;
    unsigned char   szMagic5[23+14];
    unsigned char uErrMsgLen;
    // char          *pszMsg[uErrMsgLen];
}DATA0401_X64_Ex;
 /*
ACK 返回
 0040   04 02 00 00 00 01 00 00 00 00 ae 03 00 00 00 00  ................
 0050   01 00 0e 00 00 00 00 00 00 00 10 77 00 00 0b 00  ...........w....
 0060   00 01 35 01 00 00 01 00 00 00 00 00 00 00 00 51  ..5............Q
 0070   00 00 01 00 00 00 36 01 00 00 00 00 00 00 0c 13  ......6.........
 0080   22 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  "...............
 0090   00 00 00 00 00 00 00 00 00 00 00 00 00 00 1a 4f  ...............O
 00a0   52 41 2d 30 30 39 34 32 3a 20 b1 ed bb f2 ca d3  RA-00942: ......
 00b0   cd bc b2 bb b4 e6 d4 da 0a                       .........
  */

typedef struct _data0402
{
    unsigned char   szMagic1[4]; //always be 0x00 0x00 0x00 0x01
    unsigned int    nRecordNum; 
    unsigned short  nErrorNum;
    unsigned char   szMagic2[8];
    unsigned char   uType;  //0x00 non op
                    //0x01 create table, 0x02 insert,
                    //0x06 update,
                    //0x07 delete, 0x03 select,
                    //0x0c drop table, 0x0f alter table
                    //0x2c commit,0x2f begin
    unsigned char   szMagic3[2];
    unsigned short  nObjId;
    unsigned char   szMagic4[19];
    unsigned char   uAckId;
    unsigned char   szMagic5[46];
    // unsigned char uErrMsgLen;
    // char          *pszMsg[uErrMsgLen];
}DATA0402;


/*
ACK 返回
0040   04 05 00 00 00 01 00 00 00 00 86 03 00 00 00 00  ................
0050   01 00 21 00 01 00 00 00 00 00 2d 77 00 00 0b 00  ..!.......-w....
0060   00 00 00 01 00 00 00 00 00 00 00 00 00 00 00 43  ...............C
0070   00 00 00 00 00 00 36 01 00 00 00 00 00 00 64 be  ......6.......d.
0080   20 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ...............
0090   00 00 00 00 00 00 00 00 00 00 00 00 00 00 18 4f  ...............O
00a0   52 41 2d 30 30 39 30 32 3a 20 ce de d0 a7 ca fd  RA-00902: ......
00b0   be dd c0 e0 d0 cd 0a                             .......
 */
typedef struct _data0405
{
    unsigned char   szMagic1[4]; //always be 0x00 0x00 0x00 0x01
    unsigned int    nRecordNum; 
    unsigned short  nErrorNum;
    unsigned char   szMagic2[8];
    unsigned char   uType;  //0x00 non op
    //0x01 create table, 0x02 insert,
    //0x06 update,
    //0x07 delete, 0x03 select,
    //0x0c drop table, 0x0f alter table
    //0x2c commit,0x2f begin
    unsigned char   szMagic3[2];
    unsigned short  nObjId;
    unsigned char   szMagic4[19];
    unsigned char   uAckId;
    unsigned char   szMagic5[46];
    // unsigned char uErrMsgLen;
    // char          *pszMsg[uErrMsgLen];
}DATA0405;


/*  
Result More
0040   06 00 02 e3 04 00 00 00 0f 00 00 00 20 00 3c 0f  ............ .<.
0050   01 03 01 00 00 00 00 00 00 00 00 00 00 00 0f 07  ................
0060   0a 32 32 32 32 32 20 20 20 20 20 02 c1 03 02 c1  .22222     .....
0070   4f 04 6c 69 61 6f 15 03 00 07 07 0a 33 33 33 33  O.liao......3333
0080   33 20 20 20 20 20 02 c1 04 02 c1 54              3     .....T   
 */
typedef struct _data0600
{
    unsigned char       szMagic1[2];        //什么标志
    unsigned short      nColNum;            //字段个数
    unsigned char       szMagic2[30];   
//  unsigned char       szCol[uColNum/8];     //最大为2的列数次方-1，即当返回该第n列时第n为为1
//  unsigned char       uMagic2;      //always is 0x07
// unsigned char        uRowLen;
// char          *pszRow[uRowLen];
}DATA0600;

typedef struct _data0602
{
    unsigned char       szMagic1[1];        //什么标志
    unsigned short      nColNum;            //字段个数
    unsigned char       szMagic2[6];   
//  unsigned char       szCol[uColNum/8];     //最大为2的列数次方-1，即当返回该第n列时第n为为1
//  unsigned char       uMagic2;      //always is 0x07
// unsigned char        uRowLen;
// char          *pszRow[uRowLen];
}DATA0602;
typedef struct _data0602_64
{
    unsigned short      nColNum;            //字段个数
    unsigned char       szMagic1[2];  
    unsigned short      flag;            
    unsigned char       szMagic2[15];   
//  unsigned char       szCol[uColNum/8];     //最大为2的列数次方-1，即当返回该第n列时第n为为1
//  unsigned char       uMagic2;      //always is 0x07
// unsigned char        uRowLen;
// char          *pszRow[uRowLen];
}DATA0602_64;
typedef struct _data0602_9i64
{
    unsigned short      nColNum;            //字段个数
    unsigned char       szMagic2[15];   
//  unsigned char       szCol[uColNum/8];     //最大为2的列数次方-1，即当返回该第n列时第n为为1
//  unsigned char       uMagic2;      //always is 0x07
// unsigned char        uRowLen;
// char          *pszRow[uRowLen];
}DATA0602_9i64;

typedef struct _RowSplit
{
    unsigned char       uId; //总是为15
    unsigned short      uColNum;  //该行列数
//    unsigned char     szCol[uFieldNum/8];
//    unsigned char       uMagic2;      //always is 0x07
}ROWSPLIT;

/*
Result First
0380   06 01 02 e3 0e 00 00 00 01 00 00 00 20 00 00 00  ............ ...
0390   00 00 00 00 00 00 00 00 00 00 00 00 00 00 07 02  ................
03a0   c1 03 0c 31 31 31 31 31 31 20 20 20 20 20 20 03  ...111111      .
03b0   c2 43 43 0c 65 6e 67 69 6e 65 6d 61 63 20 20 20  .CC.enginemac   
03c0   00 00 00 00 00 00 00 00 00 00 

37 77表示表ID
             06 01 02 e3 04 00 00 00 01 00 00 00 20 00  .............. .
0150   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0160   07 0a 31 31 31 31 31 20 20 20 20 20 02 c1 02 02  ..11111     ....
0170   c1 63 06 79 61 6e 67 6a 70 
 */
typedef struct _data0601
{
    unsigned char       szMagic1[2];    //什么标志
    unsigned short      nColNum;        //字段个数
    unsigned char       szMagic2[29];
    // unsigned char    uDataLen;
    // char     *pszMsg[uDataLen];
}DATA0601;

/*  
-->033B
0040   08 01 00 4f 00 01 20 09 09 01 00 00 00           ...O.. ......
-->0302
0040   08 01 00 00 00 09 01 00 00 00                    ..........

--->0376
0040   08 01 00 0c 00 00 00 0c 41 55 54 48 5f 53 45 53  ........AUTH_SES
0050   53 4b 45 59 20 00 00 00 20 45 37 39 37 44 33 34  SKEY ... E797D34
0060   32 43 39 41 38 30 44 35 31 34 35 45 43 35 43 41  2C9A80D5145EC5CA
0070   46 35 39 33 37 41 30 34 42 00 00 00 00            ...........
-->0377
0040   08 01 00 01 00 00 00 1b 01 07 00 00 00 07 78 6a  ..............xj
0050   05 19 10 21 34 00 00 00 00 06 00 00 00 06 59 41  ...!4.........YA
0060   4e 47 4a 50 02 00 00 00 02 54 54 2d 77 00 00 00  NGJP.....TT-w...
0070   00 00 00 00 00 00 00 00 01 00 00 00 43 2d 77 00  ............C-w.
0080   00 00 00 00 00 01 00 00 00 4c 01 00 00 00 1b 0b  .........L......
0090   07 00 00 00 07 78 6a 05 19 10 21 34 00 00 00 00  .....xj...!4....
00a0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
00b0   00 01 00 00 00 07 01 07 00 00 00 4c 01 00 00 00  ...........L....
00c0   1b 00 07 00 00 00 07 78 6a 05 19 10 21 34 02 00  .......xj...!4..
00d0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
00e0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
00f0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0100   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0110   00 00 00 00 00 00 00 01 00 00 00 1b 09 07 00 00  ................
0120   00 07 78 6a 05 19 10 21 34 02 00 00 00 00 00 00  ..xj...!4.......
0130   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0140   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0150   00 00 00 00 00 00 00 00 00 00 01 00 00 00 39 16  ..............9.
0160   00 02 00 02 00 00 00 02 41 41 26 00 01 00 00 00  ........AA&.....
0170   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0180   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0190   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
01a0   01 00 00 00 1b 09 07 00 00 00 07 78 6a 05 19 10  ...........xj...
01b0   21 34 02 00 00 00 00 00 00 00 00 00 00 00 00 00  !4..............
01c0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
01d0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
01e0   00 00 00 01 00 00 00 39 16 00 02 00 02 00 00 00  .......9........
01f0   02 42 42 26 00 01 00 00 00 00 00 00 00 00 00 00  .BB&............
0200   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0210   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0220   00 00 00 00 00 00 00 00 00 01 00 00 00 1b 09 07  ................
0230   00 00 00 07 78 6a 05 19 10 21 34 02 00 00 00 00  ....xj...!4.....
0240   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0250   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0260   00 00 00 00 00 00 00 00 00 00 00 00 01 00 00 00  ................
0270   39 16 00 02 00 02 00 00 00 02 43 43 26 00 01 00  9.........CC&...
0280   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0290   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
02a0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
02b0   00 00 01 00 00 00 1b 09 07 00 00 00 07 78 6a 05  .............xj.
02c0   19 10 21 34 02 00 00 00 00 00 00 00 00 00 00 00  ..!4............
02d0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
02e0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
02f0   00 00 00 00 00 01 00 00 00 39 16 00 02 00 02 00  .........9......
0300   00 00 02 44 44 26 00 01 00 00 00 00 00 00 00 00  ...DD&..........
0310   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0320   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0330   00 00 00 00 00 00 00 00 00 00 00 01 00 00 00 1b  ................
0340   09 07 00 00 00 07 78 6a 05 19 10 21 34 02 00 00  ......xj...!4...
0350   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0360   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0370   00 00 00 00 00 00 00 00 00 00 00 00 00 00 01 00  ................
0380   00 00 39 16 00 02 00 02 00 00 00 02 45 45 26 00  ..9.........EE&.
0390   01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
03a0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
03b0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
03c0   00 00 00 00 01 00 00 00 1b 09 07 00 00 00 07 78  ...............x
03d0   6a 05 19 10 21 34 02 00 00 00 00 00 00 00 00 00  j...!4..........
03e0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
03f0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0400   00 00 00 00 00 00 00 01 00 00 00 39 16 00 02 00  ...........9....
0410   02 00 00 00 02 46 46 26 00 01 00 00 00 00 00 00  .....FF&........
0420   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0430   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0440   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0450   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0460   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0470   00 00 00 00 00 00 00 00 00 00 00 00 00 06 00 0c  ................
0480   00 00 00 f3 00 c0 02 00 00 00 00 09 00 00 00 00  ................
0490   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
04a0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
04b0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
04c0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
04d0   00 00 04 05 00 00 00 01 00 00 00 00 00 00 00 00  ................
04e0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
04f0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0500   00 23 00 00 01 00 00 00 36 01 00 00 00 00 00 00  .#......6.......
0510   04 35 21 00 00 00 00 00 00 00 00 00 00 00 00 00  .5!.............
0520   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0530  
--->0x0377
0040   08 01 00 01 00 00 00 1b 01 07 00 00 00 07 78 6a  ..............xj
0050   05 1e 0e 24 39 00 00 00 00 06 00 00 00 06 59 41  ...$9.........YA
0060   4e 47 4a 50 06 00 00 00 06 4d 59 54 45 53 54 37  NGJP.....MYTEST7
0070   77 00 00 00 00 00 00 00 00 00 00 00 01 00 00 00  w...............
0080   43 37 77 00 00 00 00 00 00 01 00 00 00 4c 01 00  C7w..........L..
0090   00 00 1b 0b 07 00 00 00 07 78 6a 05 1e 0e 24 39  .........xj...$9
00a0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
00b0   00 00 00 00 00 01 00 00 00 07 01 05 00 00 00 4c  ...............L
00c0   01 00 00 00 1b 00 07 00 00 00 07 78 6a 05 1e 0e  ...........xj...
00d0   24 39 02 00 00 00 00 00 00 00 00 00 00 00 00 00  $9..............
00e0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
00f0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0100   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0110   00 00 00 00 00 00 00 00 00 00 00 01 00 00 00 1b  ................
0120   09 07 00 00 00 07 78 6a 05 1e 0e 24 39 02 00 00  ......xj...$9...
0130   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0140   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0150   00 00 00 00 00 00 00 00 00 00 00 00 00 00 01 00  ................
0160   00 00 39 0a 00 60 00 04 00 00 00 04 4e 41 4d 45  ..9..`......NAME
0170   00 0a 01 00 00 00 00 00 00 00 00 00 00 00 00 54  ...............T
0180   03 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0190   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
01a0   00 00 00 00 00 00 01 00 00 00 1b 09 07 00 00 00  ................
01b0   07 78 6a 05 1e 0e 24 39 02 00 00 00 00 00 00 00  .xj...$9........
01c0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
01d0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
01e0   00 00 00 00 00 00 00 00 00 01 00 00 00 39 16 00  .............9..
01f0   02 00 06 00 00 00 06 53 54 55 5f 49 44 26 00 01  .......STU_ID&..
0200   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0210   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0220   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0230   00 00 00 01 00 00 00 1b 09 07 00 00 00 07 78 6a  ..............xj
0240   05 1e 0e 24 39 02 00 00 00 00 00 00 00 00 00 00  ...$9...........
0250   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0260   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0270   00 00 00 00 00 00 01 00 00 00 39 16 00 02 00 04  ..........9.....
0280   00 00 00 04 4d 41 52 4b 26 00 01 00 00 00 00 00  ....MARK&.......
0290   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
02a0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
02b0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 01 00  ................
02c0   00 00 1b 09 07 00 00 00 07 78 6a 05 1e 0e 24 39  .........xj...$9
02d0   02 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
02e0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
02f0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0300   00 01 00 00 00 39 2c 01 01 00 04 00 00 00 04 4d  .....9,........M
0310   45 4d 4f 01 2c 01 00 00 00 00 00 00 00 00 00 00  EMO.,...........
0320   00 00 54 03 01 00 00 00 00 00 00 00 00 00 00 00  ..T.............
0330   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0340   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0350   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0360   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0370   00 00 00 00 00 00 00 00 00 04 00 0c 00 00 00 23  ...............#
0380   00 c0 02 00 00 00 00 09 00 00 00 00 00 00 00 00  ................
0390   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
03a0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
03b0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
03c0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 
 */
typedef struct _data0801
{
    unsigned char data[1025]; 
}DATA0801;

/*
成功返回(create table,drop table,Select) + 0401
len 103
0040   08 05 00 2c 7e 0e 00 00 00 00 00 01 00 00 00 00  ...,~...........
0050   00 00 00 00 00 00 00 00 00 00 00                     .........

       08 05 00 52 23 10 00 00 00 00 00 01 00 00 00 00 
       00 00 00 00 00 00 00 00 00 00 00 
 */
typedef struct _data0805
{
    unsigned char   szMagic[23];
    unsigned short  nValueNum;
    /*
     *	DATA0805Value value[nValueNum]
     */
}DATA0805;

typedef struct _data0805Value
{
    unsigned char szMagic1[4];
    unsigned char szValueNameLen[4];
    unsigned char uValueNameLen;
    /*
     *	unsigned char szValueName[uValueLen];
     *  unsigned char szValue[2];
     */
}DATA0805Value;

/*
 --0x0373
 0040   08 08 00 13 00 00 00 13 41 55 54 48 5f 56 45 52  ........AUTH_VER
 0050   53 49 4f 4e 5f 53 54 52 49 4e 47 0c 00 00 00 0c  SION_STRING.....
 0060   2d 20 50 72 6f 64 75 63 74 69 6f 6e 00 00 00 00  - Production....
 0070   10 00 00 00 10 41 55 54 48 5f 56 45 52 53 49 4f  .....AUTH_VERSIO
 0080   4e 5f 53 51 4c 02 00 00 00 02 31 38 00 00 00 00  N_SQL.....18....
 0090   13 00 00 00 13 41 55 54 48 5f 58 41 43 54 49 4f  .....AUTH_XACTIO
 00a0   4e 5f 54 52 41 49 54 53 01 00 00 00 01 33 00 00  N_TRAITS.....3..
 00b0   00 00 0f 00 00 00 0f 41 55 54 48 5f 56 45 52 53  .......AUTH_VERS
 00c0   49 4f 4e 5f 4e 4f 09 00 00 00 09 31 35 33 30 39  ION_NO.....15309
 00d0   32 33 35 32 00 00 00 00 13 00 00 00 13 41 55 54  2352.........AUT
 00e0   48 5f 56 45 52 53 49 4f 4e 5f 53 54 41 54 55 53  H_VERSION_STATUS
 00f0   01 00 00 00 01 30 00 00 00 00 15 00 00 00 15 41  .....0.........A
 0100   55 54 48 5f 43 41 50 41 42 49 4c 49 54 59 5f 54  UTH_CAPABILITY_T
 0110   41 42 4c 45 00 00 00 00 00 00 00 00 0f 00 00 00  ABLE............
 0120   0f 41 55 54 48 5f 53 45 53 53 49 4f 4e 5f 49 44  .AUTH_SESSION_ID
 0130   02 00 00 00 02 31 30 00 00 00 00 0f 00 00 00 0f  .....10.........
 0140   41 55 54 48 5f 53 45 52 49 41 4c 5f 4e 55 4d 03  AUTH_SERIAL_NUM.
 0150   00 00 00 03 32 38 32 00 00 00 00 04 01 00 00 00  ....282.........
 0160   01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
 0170   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
 0180   00 00 00 00 00 00 00 00 00 00 03 00 00 00 00 00  ................
 0190   00 36 01 00 00 00 00 00 00 74 4f 23 00 00 00 00  .6.......tO#....
 01a0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
 01b0   00 00 00 00 00 00 00 00 00                       .........

 */
typedef struct _data0808
{
    unsigned char data[1]; 
}DATA0808;

/*
 *	-->0x0354
 0040   08 09 00 00 00 97 00 00 00 00 00 
 0050  
 */
typedef struct _data0809
{
    unsigned char data[9]; 
}DATA0809;

/*
返回session ID
-->0x0352
0040   08 20 00 45 30 42 36 39 42 34 41 35 43 33 35 35  . .E0B69B4A5C355
0050   44 42 46 36 42 43 36 33 32 45 44 42 42 46 41 38  DBF6BC632EDBBFA8
0060   31 41 34                                         1A4 
 */
typedef struct _data0820
{
    unsigned char uMagic; 
    unsigned char   pszSessionId[32];
}DATA0820;

/*
0040   08 9c 00 9c 4f 72 61 63 6c 65 39 69 20 45 6e 74  ....Oracle9i Ent
0050   65 72 70 72 69 73 65 20 45 64 69 74 69 6f 6e 20  erprise Edition 
0060   52 65 6c 65 61 73 65 20 39 2e 32 2e 30 2e 31 2e  Release 9.2.0.1.
0070   30 20 2d 20 50 72 6f 64 75 63 74 69 6f 6e 0a 57  0 - Production.W
0080   69 74 68 20 74 68 65 20 50 61 72 74 69 74 69 6f  ith the Partitio
0090   6e 69 6e 67 2c 20 4f 4c 41 50 20 61 6e 64 20 4f  ning, OLAP and O
00a0   72 61 63 6c 65 20 44 61 74 61 20 4d 69 6e 69 6e  racle Data Minin
00b0   67 20 6f 70 74 69 6f 6e 73 0a 4a 53 65 72 76 65  g options.JServe
00c0   72 20 52 65 6c 65 61 73 65 20 39 2e 32 2e 30 2e  r Release 9.2.0.
00d0   31 2e 30 20 2d 20 50 72 6f 64 75 63 74 69 6f 6e  1.0 - Production
00e0   00 01 20 
*/
typedef struct _data089c
{
    unsigned char uMagic; 
    //unsigned char uDataLen;
    //unsigned char pszData[uDataLen];
    //unsigned char pszData[uDataLen];
}DATA089c;


/*
数据段头
0040   10 19 21 7f 9c a9 a8 c9 83 66 00 00 78 6a 05 19  ..!......f..xj..
0050   0c 17 09 ed 04 00 00 00 00 00 00 2a 00 00 00 03  ...........*....
0060   00 00 00 37 01 02 00 26 00 16 00 00 00 00 00 00  ...7...&........
0070   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0080   00 00 00 00 00 01 07 07 00 00 00 07 49 44 30 31  ............ID01
0090   32 33 34 00 00 00 00 00 00 00 00 01 60 80 00 00  234.........`...
00a0   0a 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
00b0   00 00 00 00 54 03 01 00 0a 00 00 00 01 0a 0a 00  ....T...........
00c0   00 00 0a 4e 41 4d 45 41 42 43 44 45 46 00 00 00  ...NAMEABCDEF...
00d0   00 00 00 00 00 01 01 80 00 00 0a 00 00 00 00 00  ................
00e0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 54 03  ..............T.
00f0   01 00 0a 00 00 00 01 0c 0c 00 00 00 0c 52 45 4d  .............REM
0100   41 52 4b 41 42 43 44 45 46 00 00 00 00 00 00 00  ARKABCDEF.......
0110   00 07 00 00 00 07 78 6a 05 19 0e 29 3a           .........

0040   10 19 e8 ea 54 65 9c 77 56 66 00 00 78 6a 05 19  ....Te.wVf..xj..
0050   0c 27 1b 67 05 00 00 00 00 00 00 20 00 00 00 02  .'.g....... ....
0060   00 00 00 37 01 02 00 26 00 16 00 00 00 00 00 00  ...7...&........
0070   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0080   00 00 00 00 00 01 04 04 00 00 00 04 46 49 4e 54  ............FINT
0090   00 00 00 00 00 00 00 00 01 60 80 00 00 0a 00 00  .........`......
00a0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
00b0   00 54 03 01 00 0a 00 00 00 01 07 07 00 00 00 07  .T..............
00c0   46 43 48 41 52 31 30 00 00 00 00 00 00 00 00 07  FCHAR10.........
00d0   00 00 00 07 78 6a 05 19 0e 32 25 

0040   10 19 6d 48 a0 13 e0 79 4f 66 00 00 78 6a 05 19  ..mH...yOf..xj..
0050   0f 06 0b a6 05 00 00 00 00 00 00 20 00 00 00 03  ........... ....
0060   00 00 00 37 01 60 80 00 00 05 00 00 00 00 00 00  ...7.`..........
0070   00 00 00 00 00 00 00 00 00 00 00 00 00 54 03 01  .............T..
0080   00 05 00 00 00 01 04 04 00 00 00 04 41 41 41 41  ............AAAA
0090   00 00 00 00 00 00 00 00 01 02 00 26 00 16 00 00  ...........&....
00a0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
00b0   00 00 00 00 00 00 00 00 00 01 04 04 00 00 00 04  ................
00c0   42 42 42 42 00 00 00 00 00 00 00 00 01 60 80 00  BBBB.........`..
00d0   00 05 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
00e0   00 00 00 00 00 54 03 01 00 05 00 00 00 01 04 04  .....T..........
00f0   00 00 00 04 43 43 43 43 00 00 00 00 00 00 00 00  ....CCCC........
0100   07 00 00 00 07 78 6a 05 19 0f 06 0b 

0040   10 19 f8 fc b7 43 48 08 a5 66 00 00 78 6a 05 1a  .....CH..f..xj..
0050   0e 3b 37 66 04 00 00 00 00 00 00 68 00 00 00 04  .;7f.......h....
0060   00 00 00 37 01 60 80 00 00 0a 00 00 00 00 00 00  ...7.`..........
0070   00 00 00 00 00 00 00 00 00 00 00 00 00 54 03 01  .............T..
0080   00 0a 00 00 00 01 04 04 00 00 00 04 4e 41 4d 45  ............NAME
0090   00 00 00 00 00 00 00 00 01 02 00 26 00 16 00 00  ...........&....
00a0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
00b0   00 00 00 00 00 00 00 00 00 01 06 06 00 00 00 06  ................
00c0   53 54 55 5f 49 44 00 00 00 00 00 00 00 00 01 02  STU_ID..........
00d0   00 26 00 16 00 00 00 00 00 00 00 00 00 00 00 00  .&..............
00e0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 01  ................
00f0   04 04 00 00 00 04 4d 41 52 4b 00 00 00 00 00 00  ......MARK......
0100   00 00 01 01 80 00 00 32 00 00 00 00 00 00 00 00  .......2........
0110   00 00 00 00 00 00 00 00 00 00 00 54 03 01 00 32  ...........T...2
0120   00 00 00 01 04 04 00 00 00 04 4d 45 4d 4f 00 00  ..........MEMO..
0130   00 00 00 00 00 00 07 00 00 00 07 78 6a 05 1a 0f  ...........xj...
0140   0b 0c 
 */


typedef struct _data1019
{
#define DATA1019PRELEN  32
    unsigned char   szMagic1[27];
    unsigned short  nFieldNum;
    unsigned char   szMagic2[3]; //always be 00 00 37
//  FIELD_INFO   Fields[];
//  unsigned char   szMagic3[12];// 07 00 00 00 07 78 6a 05 1a 0f 0b 0c
}DATA1019;
typedef struct _data1019_toad
{
#define DATA1019PRELEN  32
    unsigned char   szMagic1[30];
    unsigned short  nFieldNum;
    unsigned char   szMagic2[3]; //always be 00 00 37
//  FIELD_INFO   Fields[];
//  unsigned char   szMagic3[12];// 07 00 00 00 07 78 6a 05 1a 0f 0b 0c
}DATA1019_TOAD;

typedef struct _data1019_jdbc
{
#define DATA1019PRELEN_JDBC  34
    unsigned char   szMagic1[26];
    unsigned char  nFieldNum;
    unsigned char   szMagic2[1]; //always be 00 00 37
//  FIELD_INFO   Fields[];
//  unsigned char   szMagic3[12];// 07 00 00 00 07 78 6a 05 1a 0f 0b 0c
}DATA1019_idbc;

typedef struct _data1019_9i
{
    unsigned char   szMagic1[29];
    unsigned short  nFieldNum;
    unsigned char   szMagic2[3]; //always be 00 00 37
//  FIELD_INFO   Fields[];
//  unsigned char   szMagic3[12];// 07 00 00 00 07 78 6a 05 1a 0f 0b 0c
}DATA1019_9i;
typedef struct _data1019_9i_64
{
    unsigned char   szMagic1[32];
    unsigned char  nFieldNum;
    unsigned char   szMagic2[5]; //always be 00 00 37
//  FIELD_INFO   Fields[];
//  unsigned char   szMagic3[12];// 07 00 00 00 07 78 6a 05 1a 0f 0b 0c
}DATA1019_9i_64;

typedef struct _Field_Info 
{
    unsigned char   umagic1;
    unsigned char   uType;
    unsigned char   uMagic2;
    unsigned char   uPrecision;
    unsigned char   uScale;
    unsigned char   szMagic3[20];
    unsigned short  nCharsetId;
    unsigned short  nCharsetForm;
    unsigned short  nSpare3;
    unsigned short  nMagic4;
    unsigned char   uIsNull;
    unsigned char   uLength;
    unsigned int    iLength;
    //unsigned char   uLength;
    //unsigned char   pszField[uLength];
}FIELD_INFO;

typedef struct _Field_Info_jdbc 
{
    unsigned char   szMagic1[14];
    unsigned char   uType;
    unsigned char   szMagic3[2];
}FIELD_INFO_JDBC;

typedef struct _Field_Info_64 
{
    unsigned char   uType;
    unsigned char   szMagic1[34];
}FIELD_INFO_64;
typedef struct _Field_Info_6_9i 
{
    unsigned char   uType;
    unsigned char   szMagic1[12];
}FIELD_INFO_64_9i;

#pragma pack()



#endif	/* ORACLEFILTER_H */

