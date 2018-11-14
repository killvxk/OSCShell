// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the IEHELPER_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// IEHELPER_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef IEHELPER_EXPORTS
#define IEHELPER_API __declspec(dllexport)
#else
#define IEHELPER_API __declspec(dllimport)
#endif

// This class is exported from the IEHelper.dll
class IEHELPER_API CIEHelper {
public:
	CIEHelper(void);
	// TODO: add your methods here.
};

extern IEHELPER_API int nIEHelper;

IEHELPER_API int fnIEHelper(void);
