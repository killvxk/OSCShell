// screen2swf.h : main header file for the SCREEN2SWF DLL
//

#if !defined(AFX_SCREEN2SWF_H__8B5DEFE3_02D9_407E_8D60_B4076184945F__INCLUDED_)
#define AFX_SCREEN2SWF_H__8B5DEFE3_02D9_407E_8D60_B4076184945F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

extern   "C" __declspec(dllimport) void __stdcall OnRecordEx(char *szRec,int timeOut, DWORD processId);

extern   "C" __declspec(dllimport) void __stdcall OnRecord(char *szRec);

extern   "C" __declspec(dllimport) void __stdcall OnStop();

extern   "C" __declspec(dllimport) void OnPause() ;

extern   "C" __declspec(dllimport) void setFreeTimes(int timeOut);

extern   "C" __declspec(dllimport) void OnResume() ;

extern   "C" __declspec(dllimport) void initEvent() ;

extern   "C" __declspec(dllimport) void freeEvent() ;
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCREEN2SWF_H__8B5DEFE3_02D9_407E_8D60_B4076184945F__INCLUDED_)
