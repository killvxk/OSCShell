#include "stdafx.h"
#include "pub.h"
#include <tchar.h>
#include <string.h>
//#include <windows.h>
#include "datasafe.h"
#include "db_table.h"
#include   <fstream> 
#include <time.h>
//#include <iostream>
//#include <atltime.h>
using namespace std;

STRUCT_SYSCONFIG sysconfig={0};

char *titleMsg[]={
"audit.dbtype=",\
"audit.ip=",\
"audit.user=",\
"audit.pwd=",\
"audit.dbname=",\
"audit.port=",\
"osc.dbtype=",\
"osc.ip=",\
"osc.user=",\
"osc.pwd=",\
"osc.dbname=",\
"osc.port=",\
"node.code=",\
"node.level=",\
"node.monitorPort=",\
"node.videoPath=",\
"node.auditPath=",\
"node.maxOutRows=",\
"node.maxThreads=",\
"node.maxCommitRows=",\
"node.warningTimes_1=",\
"node.warningTimes_2=",\
"node.warningTimes_3=",\
"node.proxyIp=",\
"node.proxyConnectPort=",\
NULL
};

unsigned int configFieldsLen[]={sizeof(sysconfig.auditConn.dbType),sizeof(sysconfig.auditConn.dbHost),sizeof(sysconfig.auditConn.dbUser),\
sizeof(sysconfig.auditConn.dbPwd),sizeof(sysconfig.auditConn.dbName),sizeof(sysconfig.auditConn.dbPort),\
sizeof(sysconfig.oscConn.dbType),sizeof(sysconfig.oscConn.dbHost),sizeof(sysconfig.oscConn.dbUser),\
sizeof(sysconfig.oscConn.dbPwd),sizeof(sysconfig.oscConn.dbName),sizeof(sysconfig.oscConn.dbPort),\
sizeof(sysconfig.code),sizeof(sysconfig.level),sizeof(sysconfig.monitorPort),\
sizeof(sysconfig.videoPath),sizeof(sysconfig.auditPath),sizeof(sysconfig.maxOutRows),\
sizeof(sysconfig.maxThreads),sizeof(sysconfig.maxCommitRows),sizeof(sysconfig.warningTimes_1),\
sizeof(sysconfig.warningTimes_2),sizeof(sysconfig.warningTimes_3),\
sizeof(sysconfig.proxyIp),sizeof(sysconfig.proxyConnectPort)};


unsigned __int64 GetNTime( void ) 
{
	LARGE_INTEGER nFrequency;
	LARGE_INTEGER nStartCounter;
	if(::QueryPerformanceFrequency(&nFrequency))
	{
		::QueryPerformanceCounter(&nStartCounter);
	}
	__int64 ret=0;
	//ret=nStartCounter.LowPart;
	//srand((int)time(0));//防止高并发访问情况下QuadPart值相同
	ret = nStartCounter.QuadPart+rand()%10000;
	return ret;
}

void get32Id(char *id)
{
	char buff[1024];
	char tmpId[33];
	int len=0;

	memset(tmpId,0,sizeof(tmpId));
//	sprintf(tmpId,"%d",GetTickCount());
	sprintf(tmpId,"%I64d",GetNTime());

	memset(buff,0,sizeof(buff));
	DataSafe.encrypt(tmpId,strlen(tmpId),buff);
	len=strlen(buff);
	if(len<32)
	{
		len=32;
	}
	strcpy(id,&buff[len-32]); 
}

int WharToMByte(wchar_t * wideChar,char* narrowChar) 
{
	int nLength = WideCharToMultiByte(CP_ACP,NULL,wideChar,-1,NULL,0,NULL,NULL);
	WideCharToMultiByte(CP_ACP,NULL,wideChar,-1,narrowChar,nLength,NULL,NULL);
	return 1 ;
}

BOOL MByteToWChar(LPCSTR lpcszStr, LPWSTR lpwszStr, DWORD dwSize)
{
	// Get the required size of the buffer that receives the Unicode 
	// string. 
	DWORD dwMinSize;
	dwMinSize = MultiByteToWideChar (CP_ACP, 0, lpcszStr, -1, NULL, 0);

	if(dwSize < dwMinSize)
	{
		return FALSE;
	}
	// Convert headers from ASCII to Unicode.
	MultiByteToWideChar (CP_ACP, 0, lpcszStr, -1, lpwszStr, dwMinSize);  
	return TRUE;
}

typedef struct _IPHeader        // 20字节的IP头   
{  
    UCHAR     iphVerLen;      // 版本号和头长度（各占4位）   
    UCHAR     ipTOS;          // 服务类型    
    USHORT    ipLength;       // 封包总长度，即整个IP报的长度   
    USHORT    ipID;           // 封包标识，惟一标识发送的每一个数据报   
    USHORT    ipFlags;        // 标志   
    UCHAR     ipTTL;          // 生存时间，就是TTL   
    UCHAR     ipProtocol;     // 协议，可能是TCP、UDP、ICMP等   
    USHORT    ipChecksum;     // 校验和   
    ULONG     ipSource;       // 源IP地址   
    ULONG     ipDestination;  // 目标IP地址   
} IPHeader, *PIPHeader;   

typedef struct icmp_hdr
{
    unsigned char   icmp_type;		// 消息类型
    unsigned char   icmp_code;		// 代码
    unsigned short  icmp_checksum;	// 校验和

	// 下面是回显头
    unsigned short  icmp_id;		// 用来惟一标识此请求的ID号，通常设置为进程ID
    unsigned short  icmp_sequence;	// 序列号
    unsigned long   icmp_timestamp;     // 时间戳
} ICMP_HDR, *PICMP_HDR;

BOOL SetTimeout(SOCKET s, int nTime, BOOL bRecv)
{
	int ret = ::setsockopt(s, SOL_SOCKET,bRecv ? SO_RCVTIMEO : SO_SNDTIMEO, (char*)&nTime, sizeof(nTime));
	return ret != SOCKET_ERROR;
}
USHORT checksum(USHORT* buff, int size)
{
	unsigned long cksum = 0;
	while(size>1)
	{
		cksum += *buff++;
		size -= sizeof(USHORT);
	}
	// 是奇数
	if(size)
	{
		cksum += *(UCHAR*)buff;
	}
	// 将32位的chsum高16位和低16位相加，然后取反
	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);			
	return (USHORT)(~cksum);
}

int getLocalAddrByPing(char *szDestIp,char *localAddr)
{
		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);
		SOCKET sRaw = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);  

		// 设置接收超时  
		SetTimeout(sRaw, 3000, TRUE);  
		struct in_addr sin_addr;  

		// 设置目的地址  
		SOCKADDR_IN dest;  
		dest.sin_family = AF_INET;  
		dest.sin_port = htons(0);  
		dest.sin_addr.S_un.S_addr = inet_addr(szDestIp);  

		// 创建ICMP封包  
		char buff[sizeof(ICMP_HDR) + 32];  
		ICMP_HDR* pIcmp = (ICMP_HDR*)buff;  

		// 填写ICMP封包数据，请求一个ICMP回显  
		pIcmp->icmp_type = 8;      
		pIcmp->icmp_code = 0;  
		pIcmp->icmp_id = (USHORT)GetCurrentProcessId();  
		pIcmp->icmp_checksum = 0;  
		pIcmp->icmp_sequence = 0;  

		// 填充数据部分，可以为任意  
		memset(&buff[sizeof(ICMP_HDR)], 'E', 32);  

		// 开始发送和接收ICMP封包  
		USHORT  nSeq = 0;  
		char recvBuf[1024];  
		SOCKADDR_IN from;  
		int nLen = sizeof(from);  
//		while(TRUE)  
		{  
//			static int nCount = 0;  
			int nRet;  

			// ping次数  
//			if(nCount++ == 1000)  
//				break;  

			pIcmp->icmp_checksum = 0;  
			pIcmp->icmp_timestamp = GetTickCount();  
			pIcmp->icmp_sequence = nSeq++;  
			pIcmp->icmp_checksum = checksum((USHORT*)buff, sizeof(ICMP_HDR) + 32);  
			nRet = sendto(sRaw, buff, sizeof(ICMP_HDR) + 32, 0, (SOCKADDR *)&dest, sizeof(dest));  
			if(nRet == SOCKET_ERROR)  
			{  
				printf(" sendto() failed: %d \n", ::WSAGetLastError());  
				closesocket(sRaw);
				return -1;  
			}
			nRet = recvfrom(sRaw, recvBuf, 1024, 0, (sockaddr*)&from, &nLen);
			if(nRet == SOCKET_ERROR)
			{  
				if(WSAGetLastError() == WSAETIMEDOUT)  
				{  
					printf(" timed out\n");  
//					continue;  
				}
				printf("recvfrom() failed: %d\n", WSAGetLastError());  
				closesocket(sRaw);
				return -1;
			}  

			// 下面开始解析接收到的ICMP封包
/*
			int nTick = ::GetTickCount();  
			if(nRet < sizeof(IPHeader) + sizeof(ICMP_HDR))  
			{  
				printf(" Too few bytes from %s \n", inet_ntoa(from.sin_addr));  
			}  
*/			
			if(localAddr!=NULL)
			{
				PIPHeader pHeader=(PIPHeader)recvBuf;
				
				sin_addr.S_un.S_addr=pHeader->ipSource;//ipDestination;
				
				sprintf(localAddr,"%d.%d.%d.%d",
						   sin_addr.S_un.S_un_b.s_b1,
						   sin_addr.S_un.S_un_b.s_b2,
						   sin_addr.S_un.S_un_b.s_b3,
						   sin_addr.S_un.S_un_b.s_b4);
			}

/*
			// 接收到的数据中包含IP头，IP头大小为20个字节，所以加20得到ICMP头  
			// (ICMP_HDR*)(recvBuf + sizeof(IPHeader));  
			ICMP_HDR* pRecvIcmp = (ICMP_HDR*)(recvBuf + 20);   
			if(pRecvIcmp->icmp_type != 0)    // 回显  
			{  
				printf(" nonecho type %d recvd \n", pRecvIcmp->icmp_type);  
				return -1;  
			}

			if(pRecvIcmp->icmp_id != GetCurrentProcessId())  
			{  
				printf(" someone else's packet! \n");  
				return -1;  
			}  

			printf("从 %s 返回 %d 字节:", inet_ntoa(from.sin_addr),nRet);  
			printf(" 数据包序列号 = %d. \t", pRecvIcmp->icmp_sequence);  
			printf(" 延时大小: %d ms", nTick - pRecvIcmp->icmp_timestamp);  
			printf(" \n");  
*/
			// 每一秒发送一次就行了  
//			Sleep(1000);
		}

		closesocket(sRaw);
		WSACleanup();
		return 0;
}

void getRemoteLogicDiskInfo(char *drivers)
{
	int DType;
	int si = 0;
	char DStr[1024];
	int DSLength = GetLogicalDriveStrings(0,NULL);//通过该函数获取所有驱动器字符串信息长度
	memset(DStr,0,sizeof(DStr));
	GetLogicalDriveStrings(DSLength,(LPTSTR)DStr);
	for(int i=0,j=0;i<DSLength/4;++i)
	{
		DType = GetDriveType((LPTSTR)DStr+i*4);//通过该函数获取磁盘的类型
		if(DType == DRIVE_REMOTE)
		{
			drivers[j++]=DStr[si];
		}
		si+=8;

		if(j>99)
		{
			break;
		}
	}
}

//extern "C" _declspec(dllexport)

int getLogicDiskInfo()
{
	int DType;
	int si = 0;
	BOOL result;
	unsigned _int64 i64FreeBytesToCaller;
	unsigned _int64 i64TotalBytes;
	unsigned _int64 i64FreeBytes;
	float totalSize;//总空间
	float usableSize;//可用空间
	char DStr[1024];
	int DSLength = GetLogicalDriveStrings(0,NULL);//通过该函数获取所有驱动器字符串信息长度
//	cout<<"长度为："<<DSLength<<endl;
//	ofstream in;//向文件写内容
//	in.open("D:\\disk\\info.txt",ios::trunc);//ios::trunc表示在打开文件前将文件清空，由于是写入操作，当文件不存在则创建
//	char* DStr = (char *)malloc(DSLength*sizeof(char)); //new char[DSLength];
	memset(DStr,0,sizeof(DStr));
	GetLogicalDriveStrings(DSLength,(LPTSTR)DStr);
	for(int i=0;i<DSLength/4;++i)
	{
		char dir[3] = {DStr[si],':','\\'};
		//cout<<"磁盘名称为："<<dir[0]<<dir[1]<<dir[2]<<endl;
		char str[3] = {0,0,0};
		str[0] = dir[0];
		str[1] = dir[1];
		string dirName = str;
		DType = GetDriveType((LPTSTR)DStr+i*4);//通过该函数获取磁盘的类型
		string driverType;
		if(DType == DRIVE_FIXED)
		{
			driverType = "本地磁盘";
			//cout<<driverType<<endl;
		}
		else if(DType == DRIVE_CDROM)
		{
			driverType = "光驱";
			//cout<<driverType<<endl;
		}
		else if(DType == DRIVE_REMOVABLE)
		{
			driverType = "可移动磁盘";
			//cout<<driverType<<endl;
		}
		else if(DType == DRIVE_REMOTE)
		{
			driverType = "网络磁盘";
			//cout<<driverType<<endl;
		}
		else if(DType == DRIVE_RAMDISK)
		{
			driverType = "虚拟RAM磁盘";
			//cout<<driverType<<endl;
		}
		else if(DType == DRIVE_UNKNOWN)
		{
			driverType = "未知设备";
			//cout<<driverType<<endl;
		}
		
		if(dirName.compare("C:")==0)//当磁盘为C盘时
		{
			result = GetDiskFreeSpaceEx(_T("C:"),(PULARGE_INTEGER)&i64FreeBytesToCaller,(PULARGE_INTEGER)&i64TotalBytes,(PULARGE_INTEGER)&i64FreeBytes);//获取磁盘的空间状态
			if(result)
			{
				totalSize = (float)i64TotalBytes/1024/1024/1024;
				usableSize = (float)i64FreeBytesToCaller/1024/1024/1024;
				//cout<<"总空间为："<<totalSize<<"GB"<<endl;
				//cout<<"可用空间为："<<usableSize<<"GB"<<endl;
				//cout<<"=============================================================="<<endl;
			}
			else
			{
				//cout<<"未检测到磁盘设备"<<endl;
			}
		}
		else if(dirName.compare("D:")==0)//当磁盘为D盘时
		{
			result = GetDiskFreeSpaceEx(_T("D:"),(PULARGE_INTEGER)&i64FreeBytesToCaller,(PULARGE_INTEGER)&i64TotalBytes,(PULARGE_INTEGER)&i64FreeBytes);//获取磁盘的空间状态
			if(result)
			{
				totalSize = (float)i64TotalBytes/1024/1024/1024;
				usableSize = (float)i64FreeBytesToCaller/1024/1024/1024;
				//cout<<"总空间为："<<totalSize<<"GB"<<endl;
				//cout<<"可用空间为："<<usableSize<<"GB"<<endl;
				//cout<<"=============================================================="<<endl;
			}
			else
			{
				//cout<<"未检测到磁盘设备"<<endl;
			}
		}
		else if(dirName.compare("E:")==0)//当磁盘为E盘时
		{
			result = GetDiskFreeSpaceEx(_T("E:"),(PULARGE_INTEGER)&i64FreeBytesToCaller,(PULARGE_INTEGER)&i64TotalBytes,(PULARGE_INTEGER)&i64FreeBytes);//获取磁盘的空间状态
			if(result)
			{
				totalSize = (float)i64TotalBytes/1024/1024/1024;
				usableSize = (float)i64FreeBytesToCaller/1024/1024/1024;
				//cout<<"总空间为："<<totalSize<<"GB"<<endl;
				//cout<<"可用空间为："<<usableSize<<"GB"<<endl;
				//cout<<"=============================================================="<<endl;
			}
			else
			{
				//cout<<"未检测到磁盘设备"<<endl;
			}
		}
		else if(dirName.compare("F:")==0)//当磁盘为F盘时
		{
			result = GetDiskFreeSpaceEx(_T("F:"),(PULARGE_INTEGER)&i64FreeBytesToCaller,(PULARGE_INTEGER)&i64TotalBytes,(PULARGE_INTEGER)&i64FreeBytes);//获取磁盘的空间状态
			if(result)
			{
				totalSize = (float)i64TotalBytes/1024/1024/1024;
				usableSize = (float)i64FreeBytesToCaller/1024/1024/1024;
				//cout<<"总空间为："<<totalSize<<"GB"<<endl;
				//cout<<"可用空间为："<<usableSize<<"GB"<<endl;
				//cout<<"=============================================================="<<endl;
			}
			else
			{
				//cout<<"未检测到磁盘设备"<<endl;
			}
		}
		else if(dirName.compare("G:")==0)//当磁盘为E盘时
		{
			result = GetDiskFreeSpaceEx(_T("G:"),(PULARGE_INTEGER)&i64FreeBytesToCaller,(PULARGE_INTEGER)&i64TotalBytes,(PULARGE_INTEGER)&i64FreeBytes);//获取磁盘的空间状态
			if(result)
			{
				totalSize = (float)i64TotalBytes/1024/1024/1024;
				usableSize = (float)i64FreeBytesToCaller/1024/1024/1024;
				//cout<<"总空间为："<<totalSize<<"GB"<<endl;
				//cout<<"可用空间为："<<usableSize<<"GB"<<endl;
				//cout<<"=============================================================="<<endl;
			}
			else
			{
				//cout<<"未检测到磁盘设备"<<endl;
			}
		}
		si+=8;
	}
	
	if(NULL != DType)
	{
		return DType;
	}
	return -1;
}



//wchar->byte(ansi)
DWORD WharToMByteANSI(wchar_t *wideChar,char* narrowChar)
{
	int nLength = WideCharToMultiByte(CP_ACP,NULL,wideChar,-1,NULL,0,NULL,NULL);
	return WideCharToMultiByte(CP_ACP,NULL,wideChar,-1,narrowChar,nLength,NULL,NULL);
}
//wchar->byte(utf8)
DWORD WharToMByteUTF8(wchar_t *wideChar,char* narrowChar)
{
	int nLength = WideCharToMultiByte(CP_UTF8,NULL,wideChar,-1,NULL,0,NULL,NULL);
	return WideCharToMultiByte(CP_UTF8,NULL,wideChar,-1,narrowChar,nLength,NULL,NULL);
}
//byte(ansi)->wchar
DWORD MByteANSIToWChar(char *lpcszStr, wchar_t *lpwszStr, DWORD dwSize)
{
	DWORD dwMinSize;
	dwMinSize = MultiByteToWideChar (CP_ACP, 0, lpcszStr, -1, NULL, 0);

	if(dwSize < dwMinSize)
	{
		return 0;
	}
	return MultiByteToWideChar (CP_ACP, 0, lpcszStr, -1, lpwszStr, dwMinSize);
}
//byte(utf8)->wchar
DWORD MByteUTF8ToWChar(LPCSTR lpcszStr, LPWSTR lpwszStr, DWORD dwSize)
{
	DWORD dwMinSize;
	dwMinSize = MultiByteToWideChar (CP_UTF8, 0, lpcszStr, -1, NULL, 0);

	if(dwSize < dwMinSize)
	{
		return 0;
	}
	return MultiByteToWideChar (CP_UTF8, 0, lpcszStr, -1, lpwszStr, dwMinSize);
}

//byte(ansi)->wchar->byte(utf8)
DWORD MByteANSIToMByteUTF8(char *lpcszStr, int len, char* utf8Char)
{
//	wchar_t lpwszStr[2048]={0};
    DWORD ret=0;
  	DWORD dwMinSize;
	wchar_t *lpwszStr=new wchar_t[2*len+1];
    memset(lpwszStr,0,sizeof(wchar_t)*(2*len+1));
    dwMinSize=MByteANSIToWChar(lpcszStr,lpwszStr,(sizeof(wchar_t)*(2*len+1))/sizeof(lpwszStr[0]));
    if(dwMinSize<(2*len+1))
	{
         ret=WharToMByteUTF8(lpwszStr,utf8Char);   //WChar to UTF8
	}
    delete lpwszStr;
	return ret;
}
//byte(utf8)->wchar->byte(ansi)
DWORD MByteUTF8ToMByteANSI(char* utf8Char,int len,char *lpcszStr)
{
//	wchar_t lpwszStr[2048]={0};
    DWORD ret=0;
    DWORD dwMinSize;
	wchar_t *lpwszStr=new wchar_t[2*len+1];
    memset(lpwszStr,0,sizeof(wchar_t)*(2*len+1));
    dwMinSize=MByteUTF8ToWChar(utf8Char,lpwszStr,(sizeof(wchar_t)*(2*len+1))/sizeof(lpwszStr[0]));
    if(dwMinSize<2*len+1)
	{
         ret= WharToMByteANSI(lpwszStr,lpcszStr);   //WChar to UTF8
	}
    delete lpwszStr;
	return ret;
}

void loadCfgFile(char *cfgFile)
{
	//避免多次执行loadCfgFile
	 if (strlen(sysconfig.auditConn.dbType) > 0)
	 {
		 return;
	 }
     char buf[1024];
	 char preSufix[50];
	 char preSufixBuf[2048];
	 memset(&sysconfig,0,sizeof(STRUCT_SYSCONFIG));
	 memset(preSufix,0,sizeof(preSufix));

	 ifstream infile(cfgFile);
     if(!infile) return;

     infile.seekg(0);
     while(!infile.eof()){
        memset(buf,0,sizeof(buf));
        infile.getline(buf,sizeof(buf)-1);        
		char *op=strstr(buf,"[audit]");
		if(op!=NULL)
		{
			strcpy(preSufix,"audit");
			continue;
		}
		op=strstr(buf,"[osc]");
		if(op!=NULL)
		{
			strcpy(preSufix,"osc");
			continue;
		}
		op=strstr(buf,"[node]");
		if(op!=NULL)
		{
			strcpy(preSufix,"node");
			continue;
		}

		int i=0;
		int offset=0;
		char *offsetOp=sysconfig.auditConn.dbType;
		memset(preSufixBuf,0,sizeof(preSufixBuf));
		sprintf(preSufixBuf,"%s.%s",preSufix,buf);
		while(titleMsg[i]!=NULL)
		{
			op=strstr(preSufixBuf,titleMsg[i]);
			if(op!=NULL)
			{
				op += strlen(titleMsg[i]);
				int len=strlen(op);
				if(len>=configFieldsLen[i])
				{
					len=configFieldsLen[i]-1;
				}
				memcpy((char *)(offsetOp+offset),op,len);            
				break;
			}
			offset += configFieldsLen[i];
			i++;
		}
     }
     infile.close();
}

