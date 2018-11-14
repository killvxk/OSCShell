
// IEHelper.h: interface for the IEHelper class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IEHELPER_H__022E25DB_3205_4310_991E_94EB80D064DB__INCLUDED_)
#define AFX_IEHELPER_H__022E25DB_3205_4310_991E_94EB80D064DB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <ExDisp.h>
#include <ExDispID.h>
#include <shlwapi.h>
#import <shdocvw.dll> 

#define __IHTMLControlElement_INTERFACE_DEFINED__
#include <msHtml.h>
#undef __IHTMLControlElement_INTERFACE_DEFINED__

class CMyInternetExplorer
{
public:

	CMyInternetExplorer (LPCTSTR sBrowserID);
	~CMyInternetExplorer ();
	
	CString GetBrowserID () const;
	
	bool IsValid ();
	
	bool CreateNewInstance ();

	bool FindUsingTitle (const CString & sTitle);

	CString GetFullName() const;
	CString GetType() const;
	CString GetLocationName() const;
	CString GetLocationURL() const;
	void	Navigate (LPCTSTR lpszURL, DWORD dwFlags = 0,
					  LPCTSTR lpszTargetFrameName = NULL,
					  LPCTSTR lpszHeaders = NULL, 
					  LPVOID lpvPostData = NULL,
					  DWORD dwPostDataLen = 0);
	
	void Refresh ();
	void Stop ();
	void GoBack ();
	void GoForward ();
	void Quit ();

	bool WaitTillLoaded (int nTimeout,char *title[]);

	bool GetBusy () const;
	READYSTATE GetReadyState () const;
	LPDISPATCH GetHtmlDocument() const;

	bool FindAnchor		(bool bClick, bool bFocus,bool bId,
						 bool bName, bool bOuterText, bool bTooltip, bool bURL,
						 LPCTSTR sId,LPCTSTR sName, LPCTSTR sOuterText, LPCTSTR sTooltip, LPCTSTR sURL);
	
	bool FindInput		(bool bClick, bool bSelect, bool bChangeValue, bool bSetCheck,
						 bool bId,bool bType, bool bName, bool bValue, 
						 LPCTSTR sId,LPCTSTR sTypeToLook, LPCTSTR sNameToLook, LPCTSTR sValueToLook,
						 bool bNewCheckValue, LPCTSTR sNewValue,int index=1);
	
	bool FindOption		(bool bClick, 
						 bool bValue, bool bText,
						 LPCTSTR sValue, LPCTSTR sText);

	bool FindElement	(bool bExactMatch, bool bWildcardMatch, bool bCaseSensitive, bool bClick,
						 bool bTagName, bool bOuterHTML, bool bInnerHTML, bool bTooltip,
						 LPCTSTR sTagNameOrg, LPCTSTR sOuterHTMLOrg, LPCTSTR sInnerHTMLOrg, LPCTSTR sTooltipOrg,
						 CString & sRetTagName, CString & sOuterHTML, CString & sRetInnerHTML, CString & sTooltip);
	DWORD getPID();
	HANDLE getProcessHandle();
//protected:
private:
	HANDLE hProcess;
    DWORD  dwProcessId;
	IWebBrowser2 * m_pWebBrowser;
	CString m_sBrowserID;
};


#endif // !defined(AFX_IEHELPER_H__022E25DB_3205_4310_991E_94EB80D064DB__INCLUDED_)
