
// StringHelper.h: interface for the CStringHelper class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STRINGHELPER_H__AF93E88F_4BEF_4A22_A8E9_D6DECAF859A3__INCLUDED_)
#define AFX_STRINGHELPER_H__AF93E88F_4BEF_4A22_A8E9_D6DECAF859A3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class StringHelper  
{
public:
	static bool WildcardCompareNoCase	(LPCTSTR strWild, LPCTSTR strText);
	static bool WildcardCompare			(LPCTSTR strWild, LPCTSTR strText);
};

#endif // !defined(AFX_STRINGHELPER_H__AF93E88F_4BEF_4A22_A8E9_D6DECAF859A3__INCLUDED_)
