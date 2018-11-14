//#include <windows.h>
//#include <stdio.h>
#include "stdafx.h"
#include <windows.h>
#define INITGUID
#include <initguid.h>
#include <exdisp.h>
#include <memory.h>

extern "C" __declspec(dllimport) bool __stdcall initLoad(char *url,LPDWORD pid,HANDLE *process,int timeout,char *title[]);
extern "C" __declspec(dllimport) bool __stdcall setPassword(char *id,char *name,char *value);
extern "C" __declspec(dllimport) bool __stdcall clickButton(char *id,char *type,char *name,char *value);
extern "C" __declspec(dllimport) bool __stdcall setCheck(char *id,char *type,char *name,bool value);
extern "C" __declspec(dllimport) bool __stdcall setText(char *id,char *name,char *value);
extern "C" __declspec(dllimport) bool __stdcall clickAnchor(char *id,char *name,char *title,char *href);
extern "C" __declspec(dllimport) void __stdcall unLoad();

#define TYPE_TEXT        "0"
#define TYPE_USERNAME    "1"
#define TYPE_PASSWORD    "2"
#define TYPE_BUTTON      "3"
#define TYPE_ANCHOR      "4"

#define MAXDATALISTNUM   50
#define IDSPLIT          "id:"
#define NAMESPLIT        "name:"
#define VALUESPLIT       "value:"
#define TYPESPLIT        "type:"
#define FLAGSPLIT        "flag:"


typedef struct{
   char id[50];
   char name[50];
   char value[50];
   char type[50];
   char flag[5];
}STRUCT_DATALIST;

void parseParam(char * param,char *split,char *buff)
{
	 char *preOp=param;
	 char *currOp=param;
     currOp=strstr(param,split);
     if(currOp!=NULL)
	 {
		 preOp=currOp+strlen(split);
		 currOp=strchr(preOp,';'); //冒号			 
	     if(currOp!=NULL && currOp!=preOp)
		 {
			 memcpy(buff,preOp,currOp-preOp);
		 }else			   
		 {
			strcpy(buff,preOp);
		 }
	}
}

//id:xxx;name:xxx;value:xxx;type:text,id:xxx;name:xxx;value:xxx;type:password,id:xxx name:xxx;type:button

void PostData(char *url,char *userName,char *password,char *param,LPDWORD pid, HANDLE *process,char *title[])
{
	 char tmpBuf[1024]={0};
	 char buff[512]={0};
	 int i=0,j=0,num=0;
	 int loadTimeOut=0;
	 char *preOp=NULL;
	 char *currOp=NULL;
	 if(param!=NULL)
	 {
  //   loadTimeOut=0;
	   strcpy(tmpBuf,param);
	   preOp=tmpBuf;
	   currOp=tmpBuf;
	 }
	 STRUCT_DATALIST dataList[MAXDATALISTNUM];
	 memset(dataList,0,sizeof(STRUCT_DATALIST)*MAXDATALISTNUM);
	 while(currOp!=NULL)
	 {
		  memset(buff,0,sizeof(buff));
		  preOp=strchr(currOp,',');
		  if(preOp!=NULL)
		  {
			 *preOp=0;
			 preOp++;
		  }
		  if(currOp==preOp)
		  {
			  break;
		  }
		  strcpy(buff,currOp);
		  parseParam(buff,IDSPLIT,dataList[num].id);
		  parseParam(buff,NAMESPLIT,dataList[num].name);
		  parseParam(buff,VALUESPLIT,dataList[num].value);
		  parseParam(buff,TYPESPLIT,dataList[num].type);
		  num++;
		  if(num>=MAXDATALISTNUM)
		  {
			 break;
		  }
		  currOp=preOp;
	 }

	 initLoad(url,pid,process,loadTimeOut,title);
	 if(title!=NULL)
	 {
		 unLoad();
		 return;
	 }
	 j = -1;
     for(i=0;i<num;i++)
	 {
		 if(strcmp(dataList[i].type,"anchor")==0 ||strcmp(dataList[i].type,"button")==0||strcmp(dataList[i].type,"submit")==0)
		 {
			j=i;
			continue;
		 }

		 char *id=NULL;
		 char *name=NULL;
		 if(strlen(dataList[i].id)>0)
		 {
		   id=dataList[i].id;
		 }
		 if(strlen(dataList[i].name)>0)
		 {
		   name=dataList[i].name;
		 }
		 if(strcmp(dataList[i].type,"password")==0)
		// if(strnicmp(dataList[i].type,TYPE_PASSWORD,strlen(TYPE_PASSWORD))==0)
		 {
			setPassword(id,name,password);
		 }else
		 if(strcmp(dataList[i].type,"text")==0)
		 {
			if(strlen(dataList[i].value)==0)
			{
			   setText(id,name,userName);
			}else
			{
			   setText(id,name,dataList[i].value);
			}
		 }else
		 {
			setText(id,name,dataList[i].value);
		 }
	 }
	 //最后处理click
	 if(j!= -1)
	 {
		 char *id=NULL;
		 char *name=NULL;
		 char *value=NULL;
		 if(strlen(dataList[j].id)>0)
		 {
			 id=dataList[j].id;
		 }
		 if(strlen(dataList[j].name)>0)
		 {
		     name=dataList[j].name;
		 }
		 if(strlen(dataList[j].value)>0)
		 {
		     value=dataList[j].value;
		 }
		 if(strcmp(dataList[j].type,"anchor")==0)  //value--->title
		 {
			clickAnchor(id,name,value,NULL);
		 }else
		 {
			clickButton(id,dataList[j].type,name,value);
		 }
	 }
	 unLoad();
/*
//	 initLoad("http://192.168.1.246/paraOsc",&pid,&process);
	 initLoad("http://192.168.1.246/paraOsc",&pid,&process);
	 setText("j_username",NULL,"admin");
     setPassword("password",NULL,"admin");
     clickButton("longin_btn",NULL);
*/
}

void PostDataTest()
{
	 DWORD pid=0;
	 HANDLE process=NULL;

//	 PostData("http://teamlab.paraview.cn","wulh@paraview.cn","paraview","id:loginButton;type:button,id:pwd;name:pwd;type:password,id:login;name:login;type:button",&pid,&process);
//	 PostData("http://192.168.1.246/paraOsc","admin","admin","id:j_username;type:1,id:password;type:2,id:longin_btn;type:3",&pid,&process);
}

HRESULT GetPostData(LPVARIANT pvPostData);

void PostData1()
{
     HRESULT hr;
	 IWebBrowserApp* m_pWebBrowser = NULL; // Derived from IWebBrowser

     BSTR bstrURL = NULL, bstrHeaders = NULL;
     VARIANT vFlags = {0},
     vTargetFrameName = {0},
     vPostData = {0},
     vHeaders = {0};

     if (FAILED(hr = CoInitialize(NULL)))
     {
        return;
     }

	 if (FAILED(hr = CoCreateInstance(CLSID_InternetExplorer,
            NULL,
            CLSCTX_SERVER,
            IID_IWebBrowserApp,
            (LPVOID*)&m_pWebBrowser)))
     {
          goto Error;
     }

	 bstrURL = SysAllocString(L"http://localhost:8080/paraOsc/login.do");
     if (!bstrURL)
     {
        goto Error;
     }

	 bstrHeaders = SysAllocString(L"Content-Type: application/x-www-form-urlencoded\r\n");
     if (!bstrHeaders)
     {
        goto Error;
     }

     V_VT(&vHeaders) = VT_BSTR;
     V_BSTR(&vHeaders) = bstrHeaders;

     hr = GetPostData(&vPostData);
     hr = m_pWebBrowser->Navigate(bstrURL, &vFlags,&vTargetFrameName, &vPostData, &vHeaders);

/*
	 VARIANT_BOOL bool_ret;
	 pWBApp->get_Busy(&bool_ret)	
	 while(bool_ret)
	 {
		 Sleep(500);
	     pWBApp->get_Busy(&bool_ret)
	 }
*/
//	 IDispatch *pdisp;
//	 pWBApp->get_Document(&pdisp);


     m_pWebBrowser->put_Visible(VARIANT_TRUE);

Error:
     if (bstrURL) SysFreeString(bstrURL);
     if (bstrHeaders) SysFreeString(bstrHeaders);
     VariantClear(&vPostData);
     if (m_pWebBrowser) m_pWebBrowser->Release();
     CoUninitialize();
}

      // Pack some data into a SAFEARRAY of BYTEs. Return in a VARIANT
HRESULT GetPostData(LPVARIANT pvPostData)
{
	//AdId=33435571079336&currentMenuName=&j_password=33435571079336_admin&
    HRESULT hr;
    LPSAFEARRAY psa;
    LPCTSTR cszPostData = "AdId=33435571079336&currentMenuName=&j_password=33435571079336_admin&j_username=admin&password=admin";
    UINT cElems = lstrlen(cszPostData);
    LPSTR pPostData;

    if (!pvPostData)
    {
       return E_POINTER;
    }

	VariantInit(pvPostData);
    psa = SafeArrayCreateVector(VT_UI1, 0, cElems);
    if (!psa)
    {
       return E_OUTOFMEMORY;
    }

    hr = SafeArrayAccessData(psa, (LPVOID*)&pPostData);
    memcpy(pPostData, cszPostData, cElems);
    hr = SafeArrayUnaccessData(psa);

    V_VT(pvPostData) = VT_ARRAY | VT_UI1;         
    V_ARRAY(pvPostData) = psa;
    return NOERROR;
}