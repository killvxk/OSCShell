// screen2swf.h : main header file for the SCREEN2SWF DLL
//

#if !defined(AFX_SCREEN2SWF_H__8B5DEFE3_02D9_407E_8D60_B4076184945F__INCLUDED_)
#define AFX_SCREEN2SWF_H__8B5DEFE3_02D9_407E_8D60_B4076184945F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CScreen2swfApp
// See screen2swf.cpp for the implementation of this class
//

class CScreen2swfApp : public CWinApp
{
public:
	CScreen2swfApp();

public:
//	virtual BOOL InitInstance();
//	virtual int ExitInstance();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScreen2swfApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CScreen2swfApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	bool m_bIdleLoad;
};



extern   "C" __declspec(dllimport) 
void __stdcall OnRecord(char *szRec);


extern   "C" __declspec(dllimport) 
void __stdcall OnStop();


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCREEN2SWF_H__8B5DEFE3_02D9_407E_8D60_B4076184945F__INCLUDED_)
