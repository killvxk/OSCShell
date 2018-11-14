// StringHelper.cpp: implementation of the CStringHelper class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "StringHelper.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

bool StringHelper::WildcardCompareNoCase (LPCTSTR strWild, LPCTSTR strText)
{
	int cp = 0, mp = 0;
	int idxWild = 0, idxString = 0;
	int nStrLen = _tcslen (strText);   //from page
	int nWildLen = _tcslen (strWild);  //from input

	while (idxString < nStrLen)
	{
		if (idxWild >= nWildLen)
			break;
		if (strWild [idxWild] == _T ('*'))
			break;
		
		if ((toupper (strWild [idxWild]) != toupper (strText [idxString]) && (strWild [idxWild] != _T ('?'))))
			return false;
		
		++ idxWild;
		++ idxString;
	}
	
	while (idxString < nStrLen) 
	{
		if (idxWild >= nWildLen)
			break;
		
		if (strWild [idxWild] == _T ('*')) 
		{
			++ idxWild;
			if (idxWild >= nWildLen)
				return true;
			
			mp = idxWild;
			cp = idxString + 1;
		} 
		else if ((toupper (strWild [idxWild]) == toupper (strText [idxString])) || (strWild [idxWild] == _T ('?'))) 
		{
			++ idxWild;
			++ idxString;
		}
		else 
		{
			idxWild = mp;
			idxString = cp ++;
		}
	}
	
	while (idxWild < nWildLen)
	{
		if (strWild [idxWild] != _T ('*'))
			break;
		++ idxWild;
	}
	
	if (idxWild < nWildLen)
		return false;
	else
		return true;
}

bool StringHelper::WildcardCompare (LPCTSTR strWild, LPCTSTR strText)
{
	int cp = 0, mp = 0;
	int idxWild = 0, idxString = 0;
	int nStrLen = _tcslen (strText);
	int nWildLen = _tcslen (strWild);
	
	while (idxString < nStrLen)
	{
		if (idxWild >= nWildLen)
			break;
		if (strWild [idxWild] == _T ('*'))
			break;
		
		if ((strWild [idxWild] != strText [idxString] && (strWild [idxWild] != _T ('?'))))
			return false;
		
		++ idxWild;
		++ idxString;
	}
	
	while (idxString < nStrLen) 
	{
		if (idxWild >= nWildLen)
			break;
		
		if (strWild [idxWild] == _T ('*')) 
		{
			++ idxWild;
			if (idxWild >= nWildLen)
				return true;
			
			mp = idxWild;
			cp = idxString + 1;
		} 
		else if ((strWild [idxWild] == strText [idxString]) || (strWild [idxWild] == _T ('?'))) 
		{
			++ idxWild;
			++ idxString;
		}
		else 
		{
			idxWild = mp;
			idxString = cp ++;
		}
	}
	
	while (idxWild < nWildLen)
	{
		if (strWild [idxWild] != _T ('*'))
			break;
		++ idxWild;
	}
	
	if (idxWild < nWildLen)
		return false;
	else
		return true;
}
