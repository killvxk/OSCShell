// IEHelper.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "IEHelper.h"
#include "IEHelperUnit.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

using namespace std;

static CMyInternetExplorer *ieHandler=NULL;

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		_tprintf(_T("Fatal Error: MFC initialization failed\n"));
		nRetCode = 1;
	}
	else
	{
		// TODO: code your application's behavior here.
	}

	return nRetCode;
}
extern "C" __declspec(dllexport) bool __stdcall initLoad(char *url,LPDWORD pid,HANDLE *process,int timeout,char *title[])
{
	if(ieHandler!=NULL)
	{
		delete ieHandler;
		ieHandler=NULL;
	}
	ieHandler=new CMyInternetExplorer("");
	ieHandler->CreateNewInstance();
	*pid=ieHandler->getPID();
	*process=ieHandler->getProcessHandle();
    ieHandler->Navigate(url,0,NULL,"Content-Type: application/x-www-form-urlencoded\r\n");
	bool rtn = ieHandler->WaitTillLoaded(timeout,title);
	ieHandler->ClickHttps();
	::Sleep(1000);
	ieHandler->WaitTillLoaded(timeout,title);
	return rtn;
}

extern "C" __declspec(dllexport) void __stdcall unLoad()
{
	if(ieHandler!=NULL)
	{
		delete ieHandler;
		ieHandler=NULL;
	}
}
extern "C" __declspec(dllexport) bool __stdcall runScript(char *js){
	if(ieHandler==NULL)
	{
		return false;
	}
	ieHandler->RunScript(js);
	return true;
}
extern "C" __declspec(dllexport) bool __stdcall setPassword(char *id,char *name,char *value,int index = 1)
{
	if(ieHandler==NULL)
	{
		return false;
	}
   bool bClick=FALSE;
   bool bSelect=FALSE;
   bool bChangeValue=FALSE;
   bool bSetCheck=FALSE;
   bool bId=FALSE;
   bool bType=TRUE;   //
   bool bName=FALSE;
   bool bValue=FALSE;
   bool bNewCheckValue=FALSE;

   char *sValueToLook=NULL;
   char *sNewValue=value;
   char *sNameToLook=name;
   char *sTypeToLook="password"; //
   char *sId=id;
   if(sId!=NULL)
   {
	  bId=TRUE;
   }
   
   if(name!=NULL)
   {
	  bName=TRUE;
   }
   if(value!=NULL)
   {
      bChangeValue=TRUE; 
   }
   return ieHandler->FindInput(bClick,bSelect,bChangeValue,bSetCheck,bId,bType,bName,bValue,sId,sTypeToLook,sNameToLook,sValueToLook,bNewCheckValue,sNewValue,index);
}

extern "C" __declspec(dllexport) bool __stdcall clickButton(char *id,char *type,char *name,char *value,int index = 1)
{
   if(ieHandler==NULL)
   {
	  return false;
   }
   bool bClick=TRUE;
   bool bSelect=FALSE;
   bool bChangeValue=FALSE;
   bool bSetCheck=FALSE;
   bool bId=FALSE;
   bool bType=TRUE;  //
   bool bName=FALSE;
   bool bValue=FALSE;
   bool bNewCheckValue=FALSE;

   char *sValueToLook=value;
   char *sNewValue=NULL;
   char *sNameToLook=name;
   char *sTypeToLook=type;
   char *sId=id;
   if(sId!=NULL)
   {
	  bId=TRUE;
   }   
   if(sValueToLook!=NULL)
   {
	  bValue=TRUE;
   }
   if(sNameToLook!=NULL)
   {
	  bName=TRUE;
   }
   return ieHandler->FindInput(bClick,bSelect,bChangeValue,bSetCheck,bId,bType,bName,bValue,sId,sTypeToLook,sNameToLook,sValueToLook,bNewCheckValue,sNewValue,index);
}

extern "C" __declspec(dllexport) bool __stdcall setCheck(char *id,char *type,char *name,bool value,int index = 1)
{
   if(ieHandler==NULL)
   {
	return false;
   }
   bool bClick=TRUE;
   bool bSelect=FALSE;
   bool bChangeValue=FALSE;
   bool bSetCheck=TRUE;   //
   bool bId=FALSE;
   bool bType=FALSE;  
   bool bName=FALSE;
   bool bValue=FALSE;
   bool bNewCheckValue=FALSE;

   char *sValueToLook=NULL;
   char *sNewValue=NULL;
   char *sNameToLook=name;
   char *sTypeToLook=type;
   char *sId=id;
   if(sId!=NULL)
   {
	  bId=TRUE;
   }
   
   if(name!=NULL)
   {
	  bName=TRUE;
   }
   if(type!=NULL)
   {
	  bType=TRUE;
   }
   return ieHandler->FindInput(bClick,bSelect,bChangeValue,bSetCheck,bId,bType,bName,bValue,sId,sTypeToLook,sNameToLook,sValueToLook,bNewCheckValue,sNewValue,index);
}

extern "C" __declspec(dllexport) bool __stdcall setText(char *id,char *name,char *value,int index = 1)
{
	if(ieHandler==NULL)
	{
		return false;
	}
   bool bClick=TRUE;
   bool bSelect=FALSE;
   bool bChangeValue=FALSE;
   bool bSetCheck=FALSE;   
   bool bId=FALSE;
   bool bType=TRUE;     //
   bool bName=FALSE;
   bool bValue=FALSE;
   bool bNewCheckValue=FALSE;


   char *sValueToLook=NULL;
   char *sNewValue=value;
   char *sNameToLook=name;
   char *sTypeToLook="text"; //
   char *sId=id;

   if(id!=NULL)
   {
	  bId=TRUE;
   }
   if(name!=NULL)
   {
	  bName=TRUE;
   }
   if(value!=NULL)
   {
      bChangeValue=TRUE; 
   }
   return ieHandler->FindInput(bClick,bSelect,bChangeValue,bSetCheck,bId,bType,bName,bValue,sId,sTypeToLook,sNameToLook,sValueToLook,bNewCheckValue,sNewValue,index);
}



extern "C" __declspec(dllexport) bool __stdcall clickAnchor(char *id,char *name,char *title,char *href)
{
	bool bClick=TRUE;
	if(ieHandler==NULL)
	{
		return false;
	}
	bool bId=FALSE;
	bool bTooltip=FALSE;
	bool bName=FALSE;
	bool bURL=FALSE;
	bool bOuterText=FALSE;
	char *sOuterText=NULL;
	char *sId=id;
	if(id!=NULL){
	    bId=TRUE;
	}

	if(name!=NULL){
	    bName=TRUE;
	}
	if(title!=NULL){
	    bTooltip=TRUE;
	}
	if(href!=NULL){
	    bURL=TRUE;
	}
	return ieHandler->FindAnchor(bClick,FALSE,bId,bName,bOuterText,bTooltip,bURL,sId,name,sOuterText,title,href);
}



