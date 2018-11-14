//bin_hex.h

DWORD binToHexW(BYTE* pByte, DWORD dwLen, TCHAR* szHex);
DWORD hexWToBin(TCHAR* szHex, BYTE* pByte);
BOOL BinaryToString(PBYTE pbData, DWORD cbData, LPWSTR pszData, DWORD dwLen);

/*
DWORD bin2hex(BYTE* pByte, DWORD dSize, TCHAR* szHex);
DWORD bin2hex(BYTE* pByte, DWORD dSize, char* szHex);

DWORD hex2bin(TCHAR* szEncodedPwd, BYTE* pByte, DWORD* pSize);
DWORD hex2bin(char* szEncoded, BYTE* pByte, DWORD *pSize);
*/