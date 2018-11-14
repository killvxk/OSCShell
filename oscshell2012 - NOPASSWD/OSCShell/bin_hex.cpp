//bin_hex.cpp
#include <windows.h>
#include <stdio.h>


BOOL BinaryToString(PBYTE pbData, DWORD cbData, LPWSTR pszData, DWORD dwLen)
{
	if (dwLen < (cbData*2) + sizeof(WCHAR))	//for the trailing NULL
		return FALSE;

	LPWSTR pszCur = pszData;
	WCHAR  digits[] = L"0123456789ABCDEF";
	for (DWORD i=0; i<cbData; i++)
	{
		*pszCur++ = digits[(pbData[i] >> 4) & 0x0F];
		*pszCur++ = digits[pbData[i] & 0x0F];
	}
	*pszCur++ = L'\0';
	return TRUE;
}

//======================================================
DWORD binToHexW(BYTE* pByte, DWORD dwLen, TCHAR* szHex){
	ZeroMemory(szHex, dwLen*2);
	for(UINT k=0; k<dwLen; k++){
		TCHAR tch[3];
		wsprintf(tch, L"%02x", pByte[k]);
		wcscat(szHex, tch);
	}
	return 0;
}

DWORD hexWToBin(TCHAR* szHex, BYTE* pByte){
	DWORD dwLen = wcslen(szHex); //add 2 for terminating \0?
	if(pByte==NULL)
		pByte = new BYTE[dwLen/2];
	TCHAR* nCh=new TCHAR[3];
	TCHAR* hCh=new TCHAR[5];
	BYTE* bPointer=pByte;
	TCHAR* cPointer=szHex;
	UINT i=0; 
	BYTE buf;
	while(i<dwLen/2){
		//get first two digits
		wcsncpy(nCh, cPointer, 2);
		swscanf(nCh, L"%x", &buf);
		bPointer[i++]=buf;			// save byte and increment pointer

		//DEBUGMSG(1, (L"nCh=%s\tbuf=%02x\n", nCh, buf));
		
		//wsprintf(hCh, L"\\x%s", nCh);
		//bPointer[0]=(BYTE)_wtoi(nCh);
		
		cPointer+=2;
		//bPointer++;
		//i++;
	}
	return 0;
}

//======================================================
#define Char_IsNumeric(C)	((C >= '0') && (C < = '9'))
#define Char_IsAlphabetic(C)	(((C >= 'a') && (C < = 'z')) || ((C >= 'A') && (C < = 'Z')))
#define Char_IsAlphaNumeric(C)	(Char_IsNumeric(C) || Char_IsAlphabetic(C))
#define Char_IsLower(C)		((BYTE)(C - 'a') < 26)
#define Char_IsUpper(C)		((BYTE)(C - 'A') < 26)
#define	Char_Upper(C)		(Char_IsLower(C) ? (C + ('A' - 'a')) : C)
#define Char_Lower(C)		(Char_IsUpper(C) ? (C + ('a' - 'A')) : C)
#define BASE16SYM           ("0123456789ABCDEF")
#define BASE16VAL           ("\x0\x1\x2\x3\x4\x5\x6\x7\x8\x9\xA\xB\xC\xD\xE\xF")
#define BASE16_ENCODELO(b)      (BASE16SYM[((BYTE)(b)) >> 4])
#define BASE16_ENCODEHI(b)      (BASE16SYM[((BYTE)(b)) & 0xF])
#define BASE16_DECODELO(b)      (BASE16VAL[ Char_Upper(b) - '0'] << 4) 
#define BASE16_DECODEHI(b)      (BASE16VAL[ Char_Upper(b) - '0'])


DWORD bin2hex(BYTE* pByte, DWORD dSize, char* szHex){
	
	UINT i=0;
	while (i<dSize)// *szHex != '\0')
	{
		szHex[0] = BASE16_ENCODELO(*pByte);
		szHex[1] = BASE16_ENCODEHI(*pByte);

		szHex += 2;
		pByte += 1;
		i++;
	}
	*szHex='\0';
	return 0;
}

DWORD hex2bin(char* szEncoded, BYTE* pByte, DWORD *pSize){
	BYTE* TargetPtr = pByte;
	while (*szEncoded != '\0')
	{
		TargetPtr[0]  = BASE16_DECODELO(szEncoded[0]);
		TargetPtr[0] |= BASE16_DECODEHI(szEncoded[1]);

		TargetPtr += 1;	//increment pointer
		szEncoded += 2;	//increment pointer
	}
	return 0;
}
/* 
			DECODE
		TargetPtr = Target;
		 
		while (*Source != '\0')
		   {
		   TargetPtr[0]  = BASE16_DECODELO(Souce[0]);
		   TargetPtr[0] |= BASE16_DECODEHI(Souce[1]);
		 
		   TargetPtr += 1;
		   Source += 2;
		   }
		 
		*TargetPtr = '\0';

			ENCODE
		TargetPtr = Target;
		 
		while (*Source != '\0')
		   {
		   TargetPtr[0] = BASE16_ENCODELO(*Souce);
		   TargetPtr[1] = BASE16_ENCODEHI(*Souce);
		 
		   TargetPtr += 2;
		   Source += 1;
		   }
*/

DWORD hex2bin(TCHAR* szEncodedPwd, BYTE* pByte, DWORD* pSize){
	char* szEncodedPwdA=new char[*pSize];
	wcstombs(szEncodedPwdA, szEncodedPwd, *pSize);
	hex2bin(szEncodedPwdA, pByte, pSize);
	return 0;
}

//convert a byte array to a hex string
DWORD bin2hex(BYTE* pByte, DWORD dSize, TCHAR* szHex){
	TCHAR* myHex= new TCHAR[(dSize+1)*2];
	ZeroMemory(myHex, (dSize+1)*2);
	wsprintf(myHex, L"");
	TCHAR* tCh=new TCHAR(3);
	for(UINT i=0; i<dSize; i++){
		wsprintf(tCh, L"%02x", pByte[i]);
		wcscat(myHex, tCh);
		wcscat(myHex, L"\0"); // terminate string
	}
	wcscpy(szHex, myHex);
	return 0;
}