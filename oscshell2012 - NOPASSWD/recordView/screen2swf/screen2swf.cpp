// screen2swf.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include <WinDef.h>
#include <windowsx.h>
#include <MMSystem.h>
#include <Vfw.h>
#include "screen2swf.h"
//#include "hook.h"
#include "./fister/SoundFile.h"
//#include "../Idle/IdleUI.h"

#include <wtsapi32.h>
#include <TlHelp32.h>


//#define ACTIVERECORD 1
//#define SWFRECORD 1 不在录像程序里做转换

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DLLEXPORT __declspec(dllexport)


#ifdef SWFRECORD
	extern   "C" __declspec(dllimport) bool __stdcall Avi2Swf(char *pAvi, char * pSwf);
#else
	bool Avi2Swf(char *pAvi, char * pSwf){return 0;}
#endif


//
//	Note! 
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

/////////////////////////////////////////////////////////////////////////////
// CScreen2swfApp

BEGIN_MESSAGE_MAP(CScreen2swfApp, CWinApp)
	//{{AFX_MSG_MAP(CScreen2swfApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScreen2swfApp construction

CScreen2swfApp::CScreen2swfApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CScreen2swfApp object

CScreen2swfApp theApp;

/*
BOOL CScreen2swfApp::InitInstance()
{
//	m_bIdleLoad = IdleUIInit();
//	if (!m_bIdleLoad)
//	{
//		return FALSE;
//	}
//	this->m_hInstance;
//	this->m_lp
	return TRUE;
}

int CScreen2swfApp::ExitInstance() 
{
//	if (m_bIdleLoad)
//		IdleUITerm();
	return CWinApp::ExitInstance();
}
*/

int recordstate=0;
int recordpaused=0;
UINT interruptkey = 0;
int tdata=0,tdataPause=0;
DWORD initialtime=0;
int initcapture = 0;
int irsmallcount=0;

//Report variables
int nActualFrame=0;
int nCurrFrame=0;
float fRate=0.0;
float fActualRate=0.0;
float fTimeLength=0.0;
CString strCodec("MS Video 1"); 
int actualwidth=0;
int actualheight=0;

/////////////////////////////////////////////////////////
//Variables/Options requiring interface
/////////////////////////////////////////////////////////
int bits = 16; 
int launchPlayer=3;
int minimizeOnStart=0;
int MouseCaptureMode = 2;
int DefineMode = 0; //set only in FixedRegion.cpp
int capturewidth=320;
int captureheight=240;

//ver 1.5
int captureleft=100;
int capturetop=100;
int fixedcapture=0;

int maxxScreen;
int maxyScreen;

//Vars used for selecting fixed /variableregion
RECT   rcOffset;
RECT   rcClip; 
RECT   rcUse; 
RECT   old_rcClip;
BOOL   bCapturing=FALSE;
POINT  ptOrigin;

//state vars
BOOL AllowNewRecordStartKey=TRUE;
int doneOnce=0;
int savesettings=1;

int threadPriority = THREAD_PRIORITY_NORMAL;
//Path to temporary avi file
CString tempfilepath;

//这个值太大 播放太快看不清
//int timelapse=100;
//
//
////这个值越小图像越好、文件越大、默认200
//int frames_per_second = 250;
//int keyFramesEvery = 250;

//int timelapse=40;
int timelapse=200;


//这个值越小图像越好、文件越大、默认200
//int frames_per_second = 25;
//int keyFramesEvery = 25;

int frames_per_second = 3;
int keyFramesEvery = 20;


HWND hWndGlobal = NULL;
//static UINT WM_USER_SAVECURSOR = ::RegisterWindowMessage(WM_USER_SAVECURSOR_MSG);

//AVI functions  and #defines
#define AVIIF_KEYFRAME	0x00000010L // this frame is a key frame.
#define BUFSIZE 260
#define LPLPBI	LPBITMAPINFOHEADER *
#define N_FRAMES	50
#define TEXT_HEIGHT	20

//int compquality = 10000;
int compquality = 7000;
DWORD compfccHandler = 0;
ICINFO * compressor_info = NULL;
int num_compressor =0;
int selected_compressor = -1;

//Autopan
int autopan=0;
int maxpan = 20;
RECT panrect_current;
RECT panrect_dest;

//ver 2.26 Vscap Interface
#define ModeAVI 0
#define ModeFlash 1
int RecordingMode = 1;

//Ver 1.2
//Video Compress Parameters
LPVOID pVideoCompressParams = NULL; 
DWORD CompressorStateIsFor = 0;
DWORD CompressorStateSize = 0;
int useMCI = 0;
MCI_OPEN_PARMS mop;
MCI_SAVE_PARMS msp;
PSTR strFile;
int isMciRecordOpen= 0;
int alreadyMCIPause=0;

WAVEFORMATEX m_FormatSpeaker;
DWORD waveinselected_Speaker = WAVE_FORMAT_4S16;
int audio_bits_per_sample_Speaker = 16;
int audio_num_channels_Speaker = 2;
int audio_samples_per_seconds_Speaker = 44100 ;

HWAVEIN m_hRecord;
WAVEFORMATEX m_Format;
DWORD m_ThreadID;
int m_QueuedBuffers=0;
int	 m_BufferSize = 1000;	// number of samples
CSoundFile *m_pFile = NULL;

LPVOID pParamsUse = NULL; 
void FreeParamsUse();

//Audio Formats Dialog
DWORD waveinselected = WAVE_FORMAT_2S16;
int audio_bits_per_sample = 16;
int audio_num_channels = 2;
int audio_samples_per_seconds = 22050 ;
BOOL bAudioCompression = TRUE;

//Path to temporary wav file
CString tempaudiopath;
int recordaudio=0;
UINT AudioDeviceID = WAVE_MAPPER;
int audioTimeInitiated = 0;
int sdwSamplesPerSec = 22050;
int sdwBytesPerSec = 44100;
int presettime = 60;
int recordpreset = 0;
UINT keyRecordStart = VK_F8;
UINT keyRecordEnd = VK_F9;
UINT keyRecordCancel = VK_F10;
//Files Directory
CString savedir("");
CString cursordir("");

int autonaming = 0;
int restrictVideoCodecs = 0;
//Path to temporary wav file


int vanWndCreated = 0;
int launchPropPrompt = 0;
int launchHTMLPlayer = 1;
int deleteAVIAfterUse = 1;
int captureTrans=1;
int versionOp = 0;
//Cursor variables
HCURSOR g_loadcursor = NULL;
HCURSOR g_customcursor = NULL;
int g_customsel = 0;
int g_recordcursor=1; 
int g_cursortype=0; 
int g_highlightcursor=0;
int g_highlightsize=64;
int g_highlightshape=0;
COLORREF g_highlightcolor = RGB(255,255,125);
HCURSOR hSavedCursor = NULL;

char *g_szRec = NULL;


//Audio Options Dialog
LPWAVEFORMATEX      pwfx = NULL;
DWORD               cbwfx;
#define  MILLISECONDS 0
#define  FRAMES 1
BOOL interleaveFrames = TRUE;
int  interleaveFactor = 100;
int  interleaveUnit = MILLISECONDS;
RECT rc;
#define NUMSTREAMS   2

#define USE_WINDOWS_TEMP_DIR 0
//version 1.6
#define USE_INSTALLED_DIR 1
#define USE_USER_SPECIFIED_DIR 2
int tempPath_Access  = USE_WINDOWS_TEMP_DIR;
CString specifieddir;



CRect newRect;
int newRegionUsed = 0;
int readingRegion = 0;
int writingRegion = 0;


int timeshift = 100;
int frameshift = 0;
int shiftType = 0; // 0 : no shift, 1 : delayAudio, 2: delayVideo

static int freeTimes=0;

#define SWAP(x,y)   ((x)^=(y)^=(x)^=(y))
void NormalizeRect(LPRECT prc);
LRESULT OnRecordStart();
LRESULT OnRecordInterrupted();

HANDLE hFinishEvent=NULL ;

DWORD dwProcessId = 0;
HWND  hCurrentWnd = NULL;

extern void SetHooks(void);
extern void RemoveHooks(void);



extern int printdblog(const char* format, ...) {
	return 0;
	FILE* logFile = NULL;
	char logName[MAX_PATH];
	char timeNow[MAX_PATH];
	char newLogPostfix[MAX_PATH];
	memset(newLogPostfix,0,sizeof(newLogPostfix));
	memset(logName,0,sizeof(logName));
	memset(timeNow,0,sizeof(timeNow));

	time_t now;
	time(&now);

	char prePath[MAX_PATH];
	char logPath[MAX_PATH];
	char aviName[MAX_PATH];

	memset(logPath,0,sizeof(logPath));
	memset(prePath,0,sizeof(prePath));
	memset(aviName,0,sizeof(aviName));

	char *p = strrchr(g_szRec, '\\');
	if(p == NULL){
		return 0;
	}
	strcpy(aviName,p+1);

	::GetModuleFileName(NULL, prePath, _MAX_PATH);
	if(strlen(prePath) <= 0)
		return 0;
	char *slash = strrchr(prePath,static_cast<int>('\\'));
	if(slash != NULL)
		*slash = 0;
	strftime(timeNow, 10 , "%Y%m%d", localtime(&now));
	sprintf(newLogPostfix,"recodrd%s.log",aviName);
	sprintf(logName,"%s\\log\\%s",prePath,newLogPostfix);
	sprintf(logPath,"%s\\log",prePath);

	logFile = fopen(logName,"a+");
	if(logFile == NULL)
		return 0;
	char sprintBuf[1024 * 20];
	memset(sprintBuf,0,sizeof(sprintBuf));
	va_list args;
	int n;
	time_t now_sec = time(NULL);
	char timestr[24];
	strftime(timestr, _countof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&now_sec));
	va_start(args, format);
	n = vsprintf(sprintBuf, format, args);
	va_end(args);
	fprintf(logFile,"%s:%s\n",timestr,sprintBuf);
	fclose(logFile);
	return 0;
}



int GetProcessIdByName()
{
	UINT nProcessID = 0;
	PROCESSENTRY32 pe = {sizeof(PROCESSENTRY32)};



	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
	if (hSnapshot != INVALID_HANDLE_VALUE)
	{
		if (Process32First(hSnapshot, &pe))
		{
			while (Process32Next(hSnapshot, &pe))
			{
				if (pe.th32ParentProcessID == ::GetCurrentProcessId())
				{
					if (lstrcmpi("AuditMonitor.exe", pe.szExeFile) != 0)
					{
						nProcessID = pe.th32ProcessID;
						break;
					}
				}

			}
		}

		CloseHandle(hSnapshot);
	}

	return nProcessID;
}
HWND GetWindowHandleByID(DWORD dwProcessID){
	if (dwProcessID == 0)
	{
		return NULL;
	}
	HWND h=GetTopWindow(0);
	while (h)
	{
		DWORD pid = 0;
		DWORD dwTheardId = GetWindowThreadProcessId( h,&pid);
		if (dwTheardId != 0)
		{
			if ( pid == dwProcessID)
			{

				HWND hwnd = GetParent(h);
				while (hwnd != NULL)
				{
					h = hwnd;
					hwnd = GetParent(h);
				}
				return h;
			}
		}
		h = ::GetNextWindow(h, GW_HWNDNEXT);
	}
	return NULL;
}

extern   "C" DLLEXPORT void __stdcall OnStop() 
{
	printdblog("[OnStop]");
	//Version 1.1
	if (recordstate==0) return;
	if (recordpaused) {
		recordpaused=0;
	}


	//设置无信号
	ResetEvent(hFinishEvent);

	OnRecordInterrupted();

	//在这里等待信号 最多等1分钟
	if( WaitForSingleObject(hFinishEvent, 1000*60) == WAIT_TIMEOUT)
		printdblog("\n等待录像生成超时");

	//关闭事件
	CloseHandle(hFinishEvent);		
	hFinishEvent = NULL;
	printdblog("[OnStop] end");
}

bool checkSessionStatus(void){
	char *ppBuffer=NULL;
	DWORD pBytesReturned;
	bool isActive=false;
	if(WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE,
		WTS_CURRENT_SESSION,
		WTSConnectState,
		&ppBuffer,
		&pBytesReturned))
	{
		if(*((INT *)ppBuffer) == WTSActive){
			isActive=true;
		}
	}
	WTSFreeMemory( ppBuffer );
	if(!isActive)
	{
	   OnStop();
	}
	return isActive;
}

extern   "C" DLLEXPORT void initEvent()
{
    SetHooks();
}
extern   "C" DLLEXPORT void freeEvent()
{
    RemoveHooks();
}

extern   "C" DLLEXPORT void setFreeTimes(int timeOut)
{
    freeTimes=timeOut;    
}

extern   "C" DLLEXPORT void __stdcall OnRecordEx(char *szRec,int timeOut, DWORD processId)
{
	int count = 0;
	dwProcessId = processId;
	/*while (hCurrentWnd == NULL && count < 10)
	{
		hCurrentWnd = GetWindowHandleByID(dwProcessId);
		if (hCurrentWnd != NULL)
		{
			break;
		}else{
			Sleep(1000);
		}
		count ++;
	}*/
	setFreeTimes(timeOut);
    OnRecord(szRec);
}


extern   "C" DLLEXPORT void __stdcall OnRecord(char *szRec)
{

	hFinishEvent = CreateEvent(NULL, TRUE, FALSE, NULL);//手动设置事件，初始化为无信息号状态
	//Version 1.1

	g_szRec = szRec;
	if (recordpaused) {
		recordpaused = 0;
		//ver 1.8
		//if (recordaudio==2) 
		//	mciRecordResume(tempaudiopath);
		SetEvent(hFinishEvent);	//设置录像完成信号
		printdblog("设置录像完成信号");
		return;	
	}
	setFreeTimes(10);
	recordpaused = 0;
	nActualFrame=0;
	nCurrFrame=0;
	fRate=0.0;
	fActualRate=0.0;
	fTimeLength=0.0;

	//MouseCaptureMode = 0;
	fixedcapture = 1;


	initEvent();
	HDC hScreenDC=::GetDC(hCurrentWnd);
	maxxScreen = GetDeviceCaps(hScreenDC,HORZRES);
	maxyScreen = GetDeviceCaps(hScreenDC,VERTRES);
	::ReleaseDC(NULL,hScreenDC);		

	if (MouseCaptureMode==0) {
		if (fixedcapture) {
			/*rc.top=capturetop;
			rc.left=captureleft;
			rc.right=captureleft+capturewidth-1;
			rc.bottom=capturetop+captureheight-1;	

			if (rc.top<0) rc.top=0;
			if (rc.left<0) rc.left=0;
			if (rc.right>maxxScreen-1) rc.right=maxxScreen-1;
			if (rc.bottom>maxyScreen-1) rc.bottom=maxyScreen-1;

			//using protocols for MouseCaptureMode==0
			rcClip = rc;
			old_rcClip=rcClip;
			NormalizeRect(&old_rcClip);
			CopyRect(&rcUse, &old_rcClip);*/
			GetWindowRect(hCurrentWnd,&rcUse);

			//开始录屏
			OnRecordStart();
		}
		else {
			rc.top=0;
			rc.left=0;
			rc.right=capturewidth-1;
			rc.bottom=captureheight-1;	
		}
	}
	//全屏幕录像
	else if (MouseCaptureMode==2) {
		rcUse.left=0;
		rcUse.top=0;
		rcUse.right= maxxScreen;
		rcUse.bottom= maxyScreen;
		//开始录屏
		OnRecordStart();
	}else{
		printdblog("退出录屏");
	}
}


extern "C" DLLEXPORT void OnPause() 
{
	// TODO: Add your command handler code here
	//return if not current recording or already in paused state
	//if ((recordstate==0) || (recordpaused==1)) return;
	//recordpaused=1;

	//ver 1.8 
	//if (recordaudio==2) 
	//	mciRecordPause(tempaudiopath);
}

extern "C" DLLEXPORT void OnResume() 
{
	//if ((recordstate==0) || (recordpaused==0)) return;
	//recordpaused=0;
}

void FixRectSizePos(LPRECT prc,int maxxScreen, int maxyScreen);
DWORD WINAPI RecordAVIThread(LPVOID pParam);

extern void checkFreeEvent(int s);

DWORD WINAPI CheckFreeTimesThread(LPVOID pParam)
{
	printdblog("CheckFreeTimesThread");
    while(recordstate)
	{
		if(freeTimes>0){
           checkFreeEvent(freeTimes);
		}
		Sleep(50);
	}
	printdblog("CheckFreeTimesThread  end");
	return 0;
}

LRESULT OnRecordStart()
{
	//Check validity of rc and fix it		
	HDC hScreenDC = ::GetDC(hCurrentWnd);	
	maxxScreen = GetDeviceCaps(hScreenDC,HORZRES);
	maxyScreen = GetDeviceCaps(hScreenDC,VERTRES);		
	FixRectSizePos(&rc,maxxScreen, maxyScreen);		
	::ReleaseDC(NULL,hScreenDC);	

	//InstallMyHook(WM_USER_SAVECURSOR);	
	printdblog("OnRecordStart");
	recordstate=1;	
	interruptkey = 0;
	//CWinThread * pThread = AfxBeginThread(RecordAVIThread, &tdata);	
	//Ver 1.3
	//if (pThread)
		//pThread->SetThreadPriority(threadPriority);
	HANDLE hThread = CreateThread(NULL,0,RecordAVIThread,(LPVOID)&tdata,0,NULL);
	if (hThread)
	{
		::SetThreadPriority(hThread,threadPriority);
		CloseHandle(hThread);
	}

	HANDLE hThreadPause = CreateThread(NULL,0,CheckFreeTimesThread,(LPVOID)&tdataPause,0,NULL);
	if (hThreadPause)
	{
		::SetThreadPriority(hThreadPause,threadPriority);
		CloseHandle(hThreadPause);
	}


	//Ver 1.2
	AllowNewRecordStartKey = TRUE; //allow this only after recordstate is set to 1
	return 0;	
}

LRESULT OnRecordInterrupted () {
	//if (recordstate==0) return;
	//UninstallMyHook();	
	//Ver 1.1	
	if (recordpaused) {
		
		recordpaused = 0;
	}
	recordstate=0;
	//Store the interrupt key in case this function is triggered by a keypress
	interruptkey = 0;	
	return 0;
}

int RecordVideo(int top,int left,int width,int height,int fps,const char *szFileName);
CString GetTempPath();

DWORD WINAPI RecordAVIThread(LPVOID pParam) {

	int top=rcUse.top;
	int left=rcUse.left;
	int width=rcUse.right-rcUse.left;
	int height=rcUse.bottom - rcUse.top;
	int fps=frames_per_second;

	//CString fileName("\\~temp.avi");
	//tempfilepath = GetTempPath () + fileName;
	printdblog("RecordAVIThread");
	tempfilepath = g_szRec;

	//将这个替换成指定的AVI

	//Test the validity of writing to the file
	//Make sure the file to be created is currently not used by another application
	/*int fileverified = 0;
	while (!fileverified) 
	{
		OFSTRUCT ofstruct;	
		HFILE fhandle = OpenFile( tempfilepath, &ofstruct, OF_SHARE_EXCLUSIVE | OF_WRITE  | OF_CREATE );  
		if (fhandle != HFILE_ERROR) {
			fileverified = 1;
			CloseHandle( (HANDLE) fhandle );
			DeleteFile(tempfilepath);
		}	 
		else {
			printdblog("OpenFile filed:%s",tempfilepath);
			fileverified = 0;
			srand( (unsigned)time( NULL ) );
			int randnum = rand();
			char numstr[50];
			sprintf(numstr,"%d",randnum);

			CString cnumstr(numstr);
			CString fxstr("\\~temp");
			CString exstr(".avi");
			tempfilepath = GetTempPath () + fxstr + cnumstr + exstr;
		}
	} 	*/
	RecordVideo(top,left,width,height,fps,tempfilepath);
	printdblog("RecordAVIThread end");
	return 0;
}

HCURSOR FetchCursorHandle() {
	if (g_cursortype == 0) {
		//if (hSavedCursor == NULL) 
		hSavedCursor = GetCursor();
		//hSavedCursor = IdleUIGetCursor();
		if (hSavedCursor == NULL)
		{
			hSavedCursor = LoadCursor(NULL,IDC_HAND);
		}
		return hSavedCursor;
	}
	else if (g_cursortype == 1) {
		return g_customcursor ;
	}
	else {
		return g_loadcursor;
	}
}

void InsertHighLight(HDC hdc,int xoffset, int yoffset)
{
	CSize fullsize;
	fullsize.cx=128;
	fullsize.cy=128;
	int highlightsize = g_highlightsize;
	COLORREF highlightcolor = g_highlightcolor;
	int highlightshape = g_highlightshape;

	double x1,x2,y1,y2;

	//OffScreen Buffer	
	HBITMAP hbm = NULL;
	HBITMAP old_bitmap;
	HDC hdcBits = ::CreateCompatibleDC(hdc);
	hbm = (HBITMAP) ::CreateCompatibleBitmap(hdc,fullsize.cx,fullsize.cy);    	
	old_bitmap = (HBITMAP) ::SelectObject(hdcBits,hbm);		

	if ((highlightshape == 0) || (highlightshape == 2)) { //circle and square

		x1 = (fullsize.cx - highlightsize)/2.0;
		x2 = (fullsize.cx + highlightsize)/2.0;
		y1 = (fullsize.cy - highlightsize)/2.0;
		y2 = (fullsize.cy + highlightsize)/2.0;
	}
	else if ((highlightshape == 1) || (highlightshape == 3)) { //ellipse and rectangle

		x1 = (fullsize.cx - highlightsize)/2.0;
		x2 = (fullsize.cx + highlightsize)/2.0;
		y1 = (fullsize.cy - highlightsize/2.0)/2.0;
		y2 = (fullsize.cy + highlightsize/2.0)/2.0;
	}
	HBRUSH ptbrush = (HBRUSH) ::GetStockObject(WHITE_BRUSH);
	HPEN nullpen = CreatePen(PS_NULL,0,0); 
	HBRUSH hlbrush = CreateSolidBrush( highlightcolor);
	HBRUSH oldbrush = (HBRUSH)  ::SelectObject(hdcBits,ptbrush);	
	HPEN oldpen = (HPEN) ::SelectObject(hdcBits,nullpen);			
	::Rectangle(hdcBits, 0,0,fullsize.cx+1,fullsize.cy+1);		
	//Draw the highlight
	::SelectObject(hdcBits,hlbrush);				

	if ((highlightshape == 0)  || (highlightshape == 1)) { //circle and ellipse
		::Ellipse(hdcBits,(int) x1,(int) y1,(int) x2,(int) y2);
	}
	else if ((highlightshape == 2) || (highlightshape == 3)) { //square and rectangle
		::Rectangle(hdcBits,(int) x1,(int) y1,(int) x2,(int) y2);
	}
	::SelectObject(hdcBits,oldbrush);		
	::SelectObject(hdcBits,oldpen);	
	DeleteObject(hlbrush);  	
	DeleteObject(nullpen);		

	//OffScreen Buffer		
	BitBlt(hdc, xoffset, yoffset, fullsize.cx, fullsize.cy, hdcBits, 0, 0, SRCAND);  	
	SelectObject(hdcBits, old_bitmap);        
	DeleteObject(hbm);
	DeleteDC(hdcBits);
}

HANDLE  Bitmap2Dib( HBITMAP hbitmap, UINT bits )
{
	HANDLE               hdib ;
	HDC                 hdc ;
	BITMAP              bitmap ;
	UINT                wLineLen ;
	DWORD               dwSize ;
	DWORD               wColSize ;
	LPBITMAPINFOHEADER  lpbi ;
	LPBYTE              lpBits ;

	GetObject(hbitmap,sizeof(BITMAP),&bitmap) ;

	//
	// DWORD align the width of the DIB
	// Figure out the size of the colour table
	// Calculate the size of the DIB
	//
	wLineLen = (bitmap.bmWidth*bits+31)/32 * 4;
	wColSize = sizeof(RGBQUAD)*((bits <= 8) ? 1<<bits : 0);
	dwSize = sizeof(BITMAPINFOHEADER) + wColSize +
		(DWORD)(UINT)wLineLen*(DWORD)(UINT)bitmap.bmHeight;

	//
	// Allocate room for a DIB and set the LPBI fields
	//
	hdib = GlobalAlloc(GHND,dwSize);
	if (!hdib)
		return hdib ;

	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hdib) ;

	lpbi->biSize = sizeof(BITMAPINFOHEADER) ;
	lpbi->biWidth = bitmap.bmWidth ;
	lpbi->biHeight = bitmap.bmHeight ;
	lpbi->biPlanes = 1 ;
	lpbi->biBitCount = (WORD) bits ;
	lpbi->biCompression = BI_RGB ;
	lpbi->biSizeImage = dwSize - sizeof(BITMAPINFOHEADER) - wColSize ;
	lpbi->biXPelsPerMeter = 0 ;
	lpbi->biYPelsPerMeter = 0 ;
	lpbi->biClrUsed = (bits <= 8) ? 1<<bits : 0;
	lpbi->biClrImportant = 0 ;

	//
	// Get the bits from the bitmap and stuff them after the LPBI
	//
	lpBits = (LPBYTE)(lpbi+1)+wColSize ;

	hdc = CreateCompatibleDC(NULL) ;

	GetDIBits(hdc,hbitmap,0,bitmap.bmHeight,lpBits,(LPBITMAPINFO)lpbi, DIB_RGB_COLORS);

	lpbi->biClrUsed = (bits <= 8) ? 1<<bits : 0;

	DeleteDC(hdc) ;
	GlobalUnlock(hdib);
	return hdib ;
}

LPBITMAPINFOHEADER captureScreenFrame(int left,int top,int width, int height,int tempDisableRect)
{
	//hCurrentWnd = GetWindowHandleByID(dwProcessId);
	HDC hScreenDC =GetDC(NULL);
	HDC hMemDC = ::CreateCompatibleDC(hScreenDC);     
	HBITMAP hbm;
	int widdes = width;
	int heides = height;

	// 位图句柄 
	int nX, nY, nX2, nY2; 
	// 选定区域坐标 
	int nWidth, nHeight; 
	// 位图宽度和高度 
	int xScrn, yScrn; 

	GetWindowRect(hCurrentWnd,&rcUse);

	
	// 获得选定区域坐标 
	nX = rcUse.left; 
	nY = rcUse.top; 
	nX2 = rcUse.right; 
	nY2 = rcUse.bottom; 
	// 获得屏幕分辨率 
	xScrn = GetDeviceCaps(hScreenDC, HORZRES); 
	yScrn = GetDeviceCaps(hScreenDC, VERTRES); 
	//确保选定区域是可见的 
	if (nX<0) 
		nX = 0; 
	if (nY<0) 
		nY = 0; 
	if (nX2>xScrn) 
		nX2 = xScrn; 
	if (nY2>yScrn) 
		nY2 = yScrn; 
	nWidth = nX2 - nX; 
	nHeight = nY2 - nY; 
	if (nWidth%2==1)
	{
		nWidth ++;
	}
	if (nHeight%2==1)
	{
		nHeight ++;
	}
	widdes = nWidth;
	heides = nHeight;

	

	//int widdes = width/4*3;
	//int heides = height/4*3;
	hbm = CreateCompatibleBitmap(hScreenDC, width, height);
	HBITMAP oldbm = (HBITMAP) SelectObject(hMemDC, hbm);	 
	//BitBlt(hMemDC, 0, 0, width, height, hScreenDC, left, top, SRCCOPY);	 	
	//ver 1.6
	DWORD bltFlags = SRCCOPY;
	if ((captureTrans) && (versionOp>4))
		//if (captureTrans)
		bltFlags |= CAPTUREBLT;
	BitBlt(hMemDC, 0, 0, nWidth, nHeight, 
		hScreenDC, nX, nY, bltFlags);	 
	
	//StretchBlt( 
	//	hMemDC, // 目标设备环境句柄 
	//	0, // 目标矩形的坐标原点 
	//	0, 
	//	widdes, // 目标矩形的长度和宽度 
	//	heides, 
	//	hScreenDC, // 源设备环境句柄 
	//	left, // 源矩形的坐标原点 
	//	top, 
	//	width, // 源矩形的长度和宽度 
	//	height, 
	//	bltFlags // 光栅操作标志 
	//	); 

	//Get Cursor Pos
	POINT xPoint; 
	GetCursorPos( &xPoint ); 
	HCURSOR hcur= FetchCursorHandle();
	xPoint.x-=left;
	xPoint.y-=top;
	//Draw the HighLight	
	if (g_highlightcursor==1) {	
		POINT highlightPoint; 		
		highlightPoint.x = xPoint.x -64 ;
		highlightPoint.y = xPoint.y -64 ;		
		InsertHighLight( hMemDC, highlightPoint.x, highlightPoint.y);
	}
	//Draw the Cursor	
	if (g_recordcursor==1) {
		ICONINFO iconinfo ;	
		BOOL ret;
		ret	= GetIconInfo( hcur,  &iconinfo ); 
		if (ret) {
			xPoint.x -= iconinfo.xHotspot;
			xPoint.y -= iconinfo.yHotspot;
			//need to delete the hbmMask and hbmColor bitmaps
			//otherwise the program will crash after a while after running out of resource
			if (iconinfo.hbmMask) DeleteObject(iconinfo.hbmMask);
			if (iconinfo.hbmColor) DeleteObject(iconinfo.hbmColor);
		}		
		::DrawIcon( hMemDC,  xPoint.x,  xPoint.y, hcur); 							
	}

	static HFONT hf = NULL;
	if(hf == NULL)
	{
		LOGFONT   lf; 
		memset(&lf,0,sizeof(LOGFONT));
		lf.lfHeight=40; 
		lf.lfWidth=20; 
		lf.lfEscapement=0; 
		hf=CreateFontIndirect(&lf); 
	}
	HFONT hfold = (HFONT)SelectObject(hMemDC,hf);  
	
	//绘制时间
	CTime time(CTime::GetCurrentTime());
	CString szTime = time.Format("%Y%m%d %H:%M:%S"); 	
	//CRect rect(left+width/2-200,0,left+width/2+200,60); //正上方中间
	//CRect rect(width-400-50,height-60-10,width-50,height-10);   //右下角
	CRect rect(width-400-50,height-60-10,width-50,height-10);   //右下角
	::SetBkMode(hMemDC,   TRANSPARENT); 
	//::SetTextColor(hMemDC,RGB(0,0,0));
	::SetTextColor(hMemDC,RGB(255,0,0));//红色
	DrawText(hMemDC,szTime,szTime.GetLength(),rect,DT_CENTER | DT_SINGLELINE | DT_VCENTER);
	
	SelectObject(hMemDC,hfold); 
	SelectObject(hMemDC,oldbm);    			
	//LPBITMAPINFOHEADER pBM_HEADER = (LPBITMAPINFOHEADER)GlobalLock(Bitmap2Dib(hbm, bits));	
	LPBITMAPINFOHEADER pBM_HEADER = (LPBITMAPINFOHEADER)GlobalLock(Bitmap2Dib(hbm, 24));	
	if (pBM_HEADER == NULL) { 	
		printdblog("截取屏幕图像失败");
	
		exit(1);
	}    
	if (hbm != NULL)
	{
		DeleteObject(hbm);
	}
	if (hMemDC!=NULL)
	{
		DeleteDC(hMemDC);	
	}
		
	if (hScreenDC != NULL)
	{
		ReleaseDC(NULL,hScreenDC) ;	
	}
	printdblog("captureScreenFrame");
	return pBM_HEADER;
}

void FreeFrame(LPBITMAPINFOHEADER alpbi)
{
	if (!alpbi)
		return ;
	GlobalFreePtr(alpbi);
	//GlobalFree(alpbi);
	alpbi = NULL;
}

BOOL MakeCompressParamsCopy(DWORD paramsSize, LPVOID pOrg) {
	if (pParamsUse) {		
		//Free Existing
		FreeParamsUse();
	}	
	pParamsUse = (LPVOID) GlobalAllocPtr(GHND, paramsSize);
	if (NULL == pParamsUse)
	{		
		return FALSE;
	}
	memcpy(pParamsUse,pOrg,paramsSize);	
	return TRUE;
}

void FreeParamsUse() {
	if (pParamsUse) {
		GlobalFreePtr(pParamsUse);
		pParamsUse = NULL;
	}
}

//ver 1.6 Capture waveout
//MCI functions
void mciRecordOpen()
{
	//mop.dwCallback = (DWORD)hWndGlobal;
	mop.lpstrDeviceType = (LPCSTR)MCI_DEVTYPE_WAVEFORM_AUDIO;
	mop.lpstrElementName = TEXT("");
	mciSendCommand(0 , MCI_OPEN , 
		MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID |
		MCI_OPEN_ELEMENT , (DWORD)&mop);
	isMciRecordOpen = 1;
}

void mciRecordStart()
{
	DWORD dwReturn;
	char  buffer[300];
	if (dwReturn = mciSendCommand(mop.wDeviceID , MCI_RECORD , 0 , 0))
	{
		mciGetErrorString(dwReturn, buffer, sizeof (buffer));
	}	
}

void SuggestSpeakerRecordingFormat(int i) {
	//Ordered in preference of choice
	switch (i) {
		case 0: {
			audio_bits_per_sample_Speaker = 16;
			audio_num_channels_Speaker = 2;
			audio_samples_per_seconds_Speaker = 44100;				
			waveinselected_Speaker = WAVE_FORMAT_4S16;
			break;
				}
		case 1: {
			audio_bits_per_sample_Speaker = 16;
			audio_num_channels_Speaker = 2;
			audio_samples_per_seconds_Speaker = 22050;				
			waveinselected_Speaker = WAVE_FORMAT_2S16;
			break;
				}
		case 2: {
			audio_bits_per_sample_Speaker = 8;
			audio_num_channels_Speaker = 2;
			audio_samples_per_seconds_Speaker = 44100;				
			waveinselected_Speaker = WAVE_FORMAT_4S08;
			break;
				}
		case 3: {
			audio_bits_per_sample_Speaker = 8;
			audio_num_channels_Speaker = 2;
			audio_samples_per_seconds_Speaker = 22050;				
			waveinselected_Speaker = WAVE_FORMAT_2S08;
			break;
				}
		case 4: {
			audio_bits_per_sample_Speaker = 16;
			audio_num_channels_Speaker = 1;
			audio_samples_per_seconds_Speaker = 44100;				
			waveinselected_Speaker = WAVE_FORMAT_4M16;
			break;
				}
		case 5: {
			audio_bits_per_sample_Speaker = 8;
			audio_num_channels_Speaker = 1;
			audio_samples_per_seconds_Speaker = 44100;				
			waveinselected_Speaker = WAVE_FORMAT_4M08;
			break;
				}
		case 6: {
			audio_bits_per_sample_Speaker = 16;
			audio_num_channels_Speaker = 1;
			audio_samples_per_seconds_Speaker = 22050;				
			waveinselected_Speaker = WAVE_FORMAT_2M16;
			break;
				}		
		case 7: {
			audio_bits_per_sample_Speaker = 16;
			audio_num_channels_Speaker = 2;
			audio_samples_per_seconds_Speaker = 11025;				
			waveinselected_Speaker = WAVE_FORMAT_1S16;
			break;
				}
		case 8: {
			audio_bits_per_sample_Speaker = 8;
			audio_num_channels_Speaker = 1;
			audio_samples_per_seconds_Speaker = 22050;				
			waveinselected_Speaker = WAVE_FORMAT_2M08;
			break;
				}		
		case 9: {
			audio_bits_per_sample_Speaker = 8;
			audio_num_channels_Speaker = 2;
			audio_samples_per_seconds_Speaker = 11025;				
			waveinselected_Speaker = WAVE_FORMAT_1S08;
			break;
				}
		default : {
			audio_bits_per_sample_Speaker = 8;
			audio_num_channels_Speaker = 1;
			audio_samples_per_seconds_Speaker = 11025;				
			waveinselected_Speaker = WAVE_FORMAT_1M08;
				  }
	}
}

void BuildSpeakerRecordingFormat() {
	m_FormatSpeaker.wFormatTag	= WAVE_FORMAT_PCM;		
	m_FormatSpeaker.wBitsPerSample = audio_bits_per_sample_Speaker;
	m_FormatSpeaker.nSamplesPerSec = audio_samples_per_seconds_Speaker;
	m_FormatSpeaker.nChannels = audio_num_channels_Speaker;
	m_FormatSpeaker.nBlockAlign = m_FormatSpeaker.nChannels * (m_FormatSpeaker.wBitsPerSample/8);
	m_FormatSpeaker.nAvgBytesPerSec = m_FormatSpeaker.nSamplesPerSec * m_FormatSpeaker.nBlockAlign;
	m_FormatSpeaker.cbSize = 0;
}

//The setting/suggesting of format for recording Speakers is deferred until recording starts
//Default is to use best settings avalable
void mciSetWaveFormat()
{	
	MCI_WAVE_SET_PARMS set_parms;
	DWORD dwReturn;
	char  buffer[128];

	//Suggest 10 formats
	dwReturn=-1;
	for (int i=0;((i<10) && (dwReturn!=0));i++) {
		SuggestSpeakerRecordingFormat(i);
		BuildSpeakerRecordingFormat();
		// Set PCM format of recording.
		set_parms.wFormatTag = m_FormatSpeaker.wFormatTag;
		set_parms.wBitsPerSample = m_FormatSpeaker.wBitsPerSample;
		set_parms.nChannels = m_FormatSpeaker.nChannels;
		set_parms.nSamplesPerSec = m_FormatSpeaker.nSamplesPerSec;
		set_parms.nAvgBytesPerSec =  m_FormatSpeaker.nAvgBytesPerSec;
		set_parms.nBlockAlign = m_FormatSpeaker.nBlockAlign;

		if (dwReturn = mciSendCommand( mop.wDeviceID, MCI_SET, MCI_WAIT |
			MCI_WAVE_SET_FORMATTAG |
			MCI_WAVE_SET_BITSPERSAMPLE |
			MCI_WAVE_SET_CHANNELS |
			MCI_WAVE_SET_SAMPLESPERSEC |

			MCI_WAVE_SET_AVGBYTESPERSEC |
			MCI_WAVE_SET_BLOCKALIGN,
			(DWORD)(LPVOID)&set_parms))
		{
			//mciGetErrorString(dwReturn, buffer, sizeof(buffer));
		}
	}
	if (dwReturn) {
		mciGetErrorString(dwReturn, buffer, sizeof(buffer));
	}   
}

//Build Recording Format to m_Format
void BuildRecordingFormat() {
	m_Format.wFormatTag	= WAVE_FORMAT_PCM;		
	m_Format.wBitsPerSample = audio_bits_per_sample;
	m_Format.nSamplesPerSec = audio_samples_per_seconds;
	m_Format.nChannels = audio_num_channels;
	m_Format.nBlockAlign = m_Format.nChannels * (m_Format.wBitsPerSample/8);
	m_Format.nAvgBytesPerSec = m_Format.nSamplesPerSec * m_Format.nBlockAlign;
	m_Format.cbSize = 0;
}

//Delete the m_pFile variable and close existing audio file
void ClearAudioFile()	
{
	if(m_pFile) {
		//will close output file
		delete m_pFile;
		m_pFile = NULL;
	}
}

//Initialize the tempaudiopath variable with a valid temporary path
void GetTempWavePath() {
	CString fileName("\\~temp001.wav");
	tempaudiopath = GetTempPath () + fileName;

	//Test the validity of writing to the file
	int fileverified = 0;
	while (!fileverified) 
	{
		OFSTRUCT ofstruct;	
		HFILE fhandle = OpenFile( tempaudiopath, &ofstruct, OF_SHARE_EXCLUSIVE | OF_WRITE  | OF_CREATE );  
		if (fhandle != HFILE_ERROR) {
			fileverified = 1;
			CloseHandle( (HANDLE) fhandle );
			DeleteFile(tempaudiopath);
		}	 
		else {
			srand( (unsigned)time( NULL ) );
			int randnum = rand();
			char numstr[50];
			sprintf(numstr,"%d",randnum);

			CString cnumstr(numstr);
			CString fxstr("\\~temp");
			CString exstr(".wav");
			tempaudiopath = GetTempPath () + fxstr + cnumstr + exstr;
			//MessageBox(NULL,tempaudiopath,"Uses Temp File",MB_OK);
			//fileverified = 1;
			//Try choosing another temporary filename
		}
	} 	
}

BOOL InitAudioRecording()
{
	m_ThreadID = ::GetCurrentThreadId();
	m_QueuedBuffers = 0;
	m_hRecord = NULL;
	m_BufferSize		= 1000;  // samples per callback

	BuildRecordingFormat();
	ClearAudioFile();			
	//Create temporary wav file for audio recording
	GetTempWavePath();	
	m_pFile = new CSoundFile(tempaudiopath, &m_Format);	
	return TRUE;
}

int AddInputBufferToQueue()
{
	MMRESULT mmReturn = 0;
	// create the header
	LPWAVEHDR pHdr = new WAVEHDR;
	if(pHdr == NULL) return NULL;
	ZeroMemory(pHdr, sizeof(WAVEHDR));

	// new a buffer
	CBuffer buf(m_Format.nBlockAlign*m_BufferSize, false);
	pHdr->lpData = buf.ptr.c;
	pHdr->dwBufferLength = buf.ByteLen;

	// prepare it
	mmReturn = ::waveInPrepareHeader(m_hRecord,pHdr, sizeof(WAVEHDR));
	if(mmReturn)
	{
		return m_QueuedBuffers;
	}

	// add the input buffer to the queue
	mmReturn = ::waveInAddBuffer(m_hRecord, pHdr, sizeof(WAVEHDR));
	if(mmReturn)
	{
		return m_QueuedBuffers;
	}
	// no error
	// increment the number of waiting buffers
	return m_QueuedBuffers++;
}

BOOL StartAudioRecording(WAVEFORMATEX* format)
{
	MMRESULT mmReturn = 0;	

	if (format != NULL)
		m_Format = *format;

	// open wavein device 
	mmReturn = ::waveInOpen( &m_hRecord, AudioDeviceID, &m_Format,(DWORD) hWndGlobal, NULL, CALLBACK_WINDOW  ); //use on message to map.....
	if(mmReturn)
	{
		return FALSE;
	}
	else
	{
		// make several input buffers and add them to the input queue
		for(int i=0; i<3; i++)
		{
			AddInputBufferToQueue();
		}

		// start recording
		mmReturn = ::waveInStart(m_hRecord);
		if(mmReturn )
		{
			return FALSE;
		}

		audioTimeInitiated = 1;
		sdwSamplesPerSec = ((LPWAVEFORMAT) &m_Format)->nSamplesPerSec;
		sdwBytesPerSec = ((LPWAVEFORMAT) &m_Format)->nAvgBytesPerSec;
	}
	return TRUE;
}

void mciRecordStop(CString strFile)
{
	//msp.dwCallback = (DWORD)hWndGlobal;
	msp.lpfilename = LPCTSTR(strFile);
	DWORD dwReturn;
	//char  buffer[300];
	dwReturn = mciSendCommand(mop.wDeviceID , MCI_STOP , MCI_WAIT , 0);
	/*
	if (dwReturn)
	{
	mciGetErrorString(dwReturn, buffer, sizeof (buffer));
	MessageBox( NULL, buffer, "MCI_RECORD",MB_ICONEXCLAMATION | MB_OK);
	}
	*/
	dwReturn = mciSendCommand(mop.wDeviceID , MCI_SAVE ,
		MCI_WAIT | MCI_SAVE_FILE , (DWORD)&msp);	
	/*
	if (dwReturn)
	{
	mciGetErrorString(dwReturn, buffer, sizeof (buffer));
	}
	*/
}

void mciRecordClose()
{
	mciSendCommand(mop.wDeviceID , MCI_CLOSE , 0 , 0);
	isMciRecordOpen = 0;
}

void mciRecordPause(CString strFile)
{	
	//can call this only in the same thread as the one opening the device?
	if (isMciRecordOpen==0) return;
	//msp.dwCallback = (DWORD)hWndGlobal;
	msp.lpfilename = LPCTSTR(strFile);	
	DWORD dwReturn;
	//char  buffer[300];
	dwReturn = mciSendCommand(mop.wDeviceID , MCI_PAUSE , MCI_WAIT , 0);	
	//dwReturn = mciSendCommand(mop.wDeviceID , MCI_STOP , MCI_WAIT , 0);
	/*
	if (dwReturn)
	{
	mciGetErrorString(dwReturn, buffer, sizeof (buffer));

	}
	*/
}

void mciRecordResume(CString strFile)
{
	//can call this only in the same thread as the one opening the device?
	//msp.dwCallback = (DWORD)hWndGlobal;
	msp.lpfilename = LPCTSTR(strFile);
	if (isMciRecordOpen==0) return;
	//char  buffer[300];
	DWORD dwReturn = mciSendCommand(mop.wDeviceID , MCI_RESUME , MCI_WAIT , 0);	
	/*
	if (dwReturn)
	{

	mciGetErrorString(dwReturn, buffer, sizeof (buffer));

	}
	*/
}

void StopAudioRecording()
{
	MMRESULT mmReturn = MMSYSERR_NOERROR;
	mmReturn = ::waveInReset(m_hRecord);
	if(mmReturn)
	{		
		return;
	}
	else
	{		
		Sleep(500);
		mmReturn = ::waveInStop(m_hRecord);
		mmReturn = ::waveInClose(m_hRecord);
	}
	//if(m_QueuedBuffers != 0) ErrorMsg("Still %d buffers in waveIn queue!", m_QueuedBuffers);
	if(m_QueuedBuffers != 0) 
		//MessageBox(NULL,"Audio buffers still in queue!","note", MB_OK);
	audioTimeInitiated = 0;
}

CString GetProgPath()
{
	// locals
	TCHAR    szTemp[300];
	CFile converter;
	CString result;
	// get root
	GetModuleFileName( NULL, szTemp, 300 );
	CString path=(CString)szTemp;
	path=path.Left(path.ReverseFind('\\'));
	return path;	
}

BOOL WinYield(void)
{
	//Process 3 messages, then return false
	MSG msg;
	for (int i=0;i<3; i++) {
		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return FALSE;	
}

BOOL CALLBACK SaveCallback(int iProgress)
{
	return WinYield();
}

// Ver 1.1
// ========================================
// Merge Audio and Video File Function
// ========================================
//
// No recompression is applied to the Video File
// Optional Recompression is applied to the Audio File	
// Assuming audio_recompress_format is compatible with the existing format of the audio file
//
// If recompress audio is set to FALSE,  both audio_recompress_format and audio_format_size can be NULL
// ========================================
int Merge_Video_And_Sound_File(CString input_video_path, CString input_audio_path, CString output_avi_path, BOOL recompress_audio, LPWAVEFORMATEX audio_recompress_format, DWORD  audio_format_size, BOOL bInterleave, int interleave_factor, int interleave_unit) {	    

	PAVISTREAM            AviStream[NUMSTREAMS];      // the editable streams
	AVICOMPRESSOPTIONS    gaAVIOptions[NUMSTREAMS];   // compression options
	LPAVICOMPRESSOPTIONS  galpAVIOptions[NUMSTREAMS];	
	PAVIFILE pfileVideo = NULL;
	AVIFileInit();	    
	//Open Video and Audio Files	
	HRESULT hr = AVIFileOpen(&pfileVideo, LPCTSTR(input_video_path), OF_READ | OF_SHARE_DENY_NONE, 0L);
	if (hr != 0)
	{		
		return 1;
	}

	//Get Video Stream from Video File and Audio Stream from Audio File        
	// ==========================================================
	// Important Assumption
	// Assume stream 0 is the correct stream in the files
	// ==========================================================
	if (pfileVideo) {
		PAVISTREAM  pavi;		
		if (AVIFileGetStream(pfileVideo, &pavi, streamtypeVIDEO , 0) != AVIERR_OK)
		{
			AVIFileRelease(pfileVideo);	   	
			return 1;
		}
		//Set editable stream number as 0
		if (CreateEditableStream(&AviStream[0], pavi) != AVIERR_OK) {
			AVIStreamRelease(pavi);
			AVIFileRelease(pfileVideo);
			return 1;
		}
		AVIStreamRelease(pavi);
		AVIFileRelease(pfileVideo);
		pfileVideo = NULL;	
	}

	// =============================
	// Getting Audio Stream
	// =============================
	{
		PAVISTREAM  pavi;		
		if (AVIStreamOpenFromFile(&pavi,input_audio_path,streamtypeAUDIO,0,OF_READ | OF_SHARE_DENY_NONE,NULL)!=AVIERR_OK)
		{
			AVIStreamRelease(AviStream[0]);
			return 2;
		}
		//Set editable stream number as 1
		if (CreateEditableStream(&AviStream[1], pavi) != AVIERR_OK) {
			AVIStreamRelease(pavi);
			AVIStreamRelease(AviStream[0]);
			return 2;
		}		
		AVIStreamRelease(pavi);
	}	

	//Verifying streams are of the correct type
	AVISTREAMINFO     avis[NUMSTREAMS];
	AVIStreamInfo(AviStream[0], &avis[0], sizeof(avis[0]));
	AVIStreamInfo(AviStream[1], &avis[1], sizeof(avis[1]));
	//Assert that the streams we are going to work with are correct in our assumption
	//such that stream 0 is video and stream 1 is audio
	if (avis[0].fccType != streamtypeVIDEO) {
		AVIStreamRelease(AviStream[0]);
		AVIStreamRelease(AviStream[1]);
		return 3;	
	}

	if (avis[1].fccType != streamtypeAUDIO) {
		AVIStreamRelease(AviStream[0]);
		AVIStreamRelease(AviStream[1]);
		return 4;	
	}
	//
	// AVISaveV code takes a pointer to compression opts
	//
	galpAVIOptions[0] = &gaAVIOptions[0];
	galpAVIOptions[1] = &gaAVIOptions[1];
	//
	// clear options structure to zeroes
	//
	_fmemset(galpAVIOptions[0], 0, sizeof(AVICOMPRESSOPTIONS));
	_fmemset(galpAVIOptions[1], 0, sizeof(AVICOMPRESSOPTIONS));

	//=========================================
	//Set Video Stream Compress Options
	//=========================================
	//No Recompression
	galpAVIOptions[0]->fccType = streamtypeVIDEO;
	galpAVIOptions[0]->fccHandler = 0;
	galpAVIOptions[0]->dwFlags = AVICOMPRESSF_VALID | AVICOMPRESSF_KEYFRAMES | AVICOMPRESSF_DATARATE;
	if (bInterleave) 
		galpAVIOptions[0]->dwFlags = galpAVIOptions[0]->dwFlags | AVICOMPRESSF_INTERLEAVE; 
	galpAVIOptions[0]->dwKeyFrameEvery = (DWORD) -1;
	galpAVIOptions[0]->dwQuality = (DWORD)ICQUALITY_DEFAULT;
	galpAVIOptions[0]->dwBytesPerSecond = 0;

	if (interleave_unit==FRAMES)
		galpAVIOptions[0]->dwInterleaveEvery = interleave_factor;
	else {
		double interfloat = (((double) interleaveFactor) * ((double) frames_per_second))/1000.0;
		int interint = (int) interfloat;
		if (interint<=0)
			interint = 1;	
		galpAVIOptions[0]->dwInterleaveEvery = interint;
	}
	//galpAVIOptions[0]->cbParms = 0;
	//galpAVIOptions[0]->cbFormat = 0;

	//=========================================
	//Set Audio Stream Compress Options
	//=========================================
	//Recompression may be applied
	//
	//Audio Compress Options seems to be specified by the audio format in avicompressoptions
	galpAVIOptions[1]->fccType = streamtypeAUDIO;
	galpAVIOptions[1]->fccHandler = 0;
	galpAVIOptions[1]->dwFlags = AVICOMPRESSF_VALID;
	if (bInterleave) 
		galpAVIOptions[1]->dwFlags = galpAVIOptions[1]->dwFlags | AVICOMPRESSF_INTERLEAVE; 
	galpAVIOptions[1]->dwKeyFrameEvery = 0;
	galpAVIOptions[1]->dwQuality = 0;
	galpAVIOptions[1]->dwBytesPerSecond = 0;

	if (interleave_unit==FRAMES)
		galpAVIOptions[1]->dwInterleaveEvery = interleave_factor;
	else {
		//back here
		double interfloat = (((double) interleaveFactor) * ((double) frames_per_second))/1000.0;
		int interint = (int) interfloat;
		if (interint<=0)
			interint = 1;
		galpAVIOptions[1]->dwInterleaveEvery = interint;
	}
	//galpAVIOptions[1]->dwInterleaveEvery = interleave_factor;
	//galpAVIOptions[1]->cbParms = 0;
	if (recompress_audio) {
		galpAVIOptions[1]->cbFormat = audio_format_size;
		galpAVIOptions[1]->lpFormat = GlobalAllocPtr(GHND, audio_format_size); 	
		memcpy( (void *) galpAVIOptions[1]->lpFormat,  (void *) audio_recompress_format, audio_format_size );
	}
	else {
		LONG lTemp;
		AVIStreamReadFormat(AviStream[1], AVIStreamStart(AviStream[1]), NULL, &lTemp);
		galpAVIOptions[1]->cbFormat = lTemp;
		if (lTemp)  galpAVIOptions[1]->lpFormat = GlobalAllocPtr(GHND, lTemp);		
		// Use existing format as compress format
		if (galpAVIOptions[1]->lpFormat)     
			AVIStreamReadFormat(AviStream[1],	AVIStreamStart(AviStream[1]),galpAVIOptions[1]->lpFormat, &lTemp);
	}

	// ============================
	// Do the work! Merging
	// ============================

	//Save 	fccHandlers 
	DWORD fccHandler[NUMSTREAMS]; 
	fccHandler[0] = galpAVIOptions[0]->fccHandler;
	fccHandler[1] = galpAVIOptions[1]->fccHandler;

	hr = AVISaveV(LPCTSTR(output_avi_path),  NULL, (AVISAVECALLBACK) SaveCallback, NUMSTREAMS, AviStream, galpAVIOptions);	
	//hr = AVISaveV(LPCTSTR(output_avi_path),  NULL, (AVISAVECALLBACK) NULL, NUMSTREAMS, AviStream, galpAVIOptions);
	if (hr != AVIERR_OK) {

		//Error merging with audio compress options, retry merging with default audio options (no recompression)
		if (recompress_audio) {
			AVISaveOptionsFree(NUMSTREAMS,galpAVIOptions);

			galpAVIOptions[0] = &gaAVIOptions[0];
			galpAVIOptions[1] = &gaAVIOptions[1];

			//Resetting Compress Options
			_fmemset(galpAVIOptions[0], 0, sizeof(AVICOMPRESSOPTIONS));
			_fmemset(galpAVIOptions[1], 0, sizeof(AVICOMPRESSOPTIONS));

			galpAVIOptions[0]->fccType = streamtypeVIDEO;
			galpAVIOptions[0]->fccHandler = 0;
			galpAVIOptions[0]->dwFlags = AVICOMPRESSF_VALID | AVICOMPRESSF_KEYFRAMES | AVICOMPRESSF_DATARATE;
			if (bInterleave) 
				galpAVIOptions[0]->dwFlags = galpAVIOptions[0]->dwFlags | AVICOMPRESSF_INTERLEAVE; 
			galpAVIOptions[0]->dwKeyFrameEvery = (DWORD) -1;
			galpAVIOptions[0]->dwQuality = (DWORD)ICQUALITY_DEFAULT;
			galpAVIOptions[0]->dwBytesPerSecond = 0;
			galpAVIOptions[0]->dwInterleaveEvery = interleave_factor;

			galpAVIOptions[1]->fccType = streamtypeAUDIO;
			galpAVIOptions[1]->fccHandler = 0;
			galpAVIOptions[1]->dwFlags = AVICOMPRESSF_VALID;
			if (bInterleave) 
				galpAVIOptions[1]->dwFlags = galpAVIOptions[1]->dwFlags | AVICOMPRESSF_INTERLEAVE; 
			galpAVIOptions[1]->dwKeyFrameEvery = 0;
			galpAVIOptions[1]->dwQuality = 0;
			galpAVIOptions[1]->dwBytesPerSecond = 0;
			galpAVIOptions[1]->dwInterleaveEvery = interleave_factor;	

			//Use default audio format
			LONG lTemp;
			AVIStreamReadFormat(AviStream[1], AVIStreamStart(AviStream[1]), NULL, &lTemp);
			galpAVIOptions[1]->cbFormat = lTemp;			
			if (lTemp)  galpAVIOptions[1]->lpFormat = GlobalAllocPtr(GHND, lTemp);		
			// Use existing format as compress format
			if (galpAVIOptions[1]->lpFormat)     
				AVIStreamReadFormat(AviStream[1],	AVIStreamStart(AviStream[1]),galpAVIOptions[1]->lpFormat, &lTemp);

			//Do the Work .... Merging
			hr = AVISaveV(LPCTSTR(output_avi_path),  NULL, (AVISAVECALLBACK) NULL, NUMSTREAMS, AviStream, galpAVIOptions);

			if (hr != AVIERR_OK) {		

				AVISaveOptionsFree(NUMSTREAMS,galpAVIOptions);						
				AVIStreamRelease(AviStream[0]);
				AVIStreamRelease(AviStream[1]);	    		
				return 5;
			}

			//Succesful Merging, but with no audio recompression
		} // if recompress audio retry
		else {

			AVISaveOptionsFree(NUMSTREAMS,galpAVIOptions);						
			AVIStreamRelease(AviStream[0]);
			AVIStreamRelease(AviStream[1]);	    		
			//MessageBox(NULL,"Unable to audio and video merge streams (2).","Note",MB_OK | MB_ICONEXCLAMATION);	
			return 5;
		}
	}
	// Restore fccHandlers
	galpAVIOptions[0]->fccHandler = fccHandler[0];
	galpAVIOptions[1]->fccHandler = fccHandler[1];
	
	AVISaveOptionsFree(NUMSTREAMS,galpAVIOptions);		
	// Free Editable Avi Streams
	for (int i=0;i<	NUMSTREAMS;i++) {

		if (AviStream[i]) {
			AVIStreamRelease(AviStream[i]);
			AviStream[i]=NULL;		
		}
	}	
	AVIFileExit();
	return 0;
}



LRESULT OnUserGeneric() 
{
	//ver 1.2
	if (interruptkey==keyRecordCancel) {
		//if (interruptkey==VK_ESCAPE) {

		//Perform processing for cancel operation
		DeleteFile(tempfilepath);
		if (recordaudio) DeleteFile(tempaudiopath);
		return 0;
	}
	//Normal thread exit	
	//Prompt the user for the filename
	if (savedir=="") 
		savedir=GetProgPath();
	CString m_newfile;
	CString m_newfileTitle;
	CString szRecord;
	if ((RecordingMode == ModeAVI)	&&  (autonaming > 0))
	{	
		savedir=GetProgPath();
		//ErrMsg("hereh");
		time_t osBinaryTime;  // C run-time time (defined in <time.h>)
		time( &osBinaryTime ); 
		CTime ctime(osBinaryTime);	
		int day = ctime.GetDay();
		int month = ctime.GetMonth();
		int year = ctime.GetYear();
		int hour = ctime.GetHour() ;
		int minutes = ctime.GetMinute() ;
		int second = ctime.GetSecond() ;
		CString filestr;
		//filestr.Format("%d%d%d_%d%d",day,month,year,hour,minutes);
		filestr.Format("%d%d%d_%d%d_%d",day,month,year,hour,minutes,second);
		m_newfile = savedir + "\\" + filestr + ".avi";
		m_newfileTitle = savedir + "\\" + filestr + ".avi";
		m_newfileTitle=m_newfileTitle.Left(m_newfileTitle.ReverseFind('\\'));
	}
	else 
	{
		//保存文件
		m_newfile = CString(g_szRec);
		m_newfileTitle = m_newfile;
		m_newfileTitle=m_newfileTitle.Left(m_newfileTitle.ReverseFind('\\'));		
		savedir = m_newfileTitle;
	}
	szRecord = m_newfile;

	//ver 2.26
	/*if (RecordingMode == ModeFlash)
	{	
		int lenx = m_newfile.GetLength();
		if (((m_newfile.GetAt(lenx-1) == 'f') || (m_newfile.GetAt(lenx-1) == 'F')) &&
			((m_newfile.GetAt(lenx-2) == 'w') || (m_newfile.GetAt(lenx-2) == 'W')) &&
			((m_newfile.GetAt(lenx-3) == 's') || (m_newfile.GetAt(lenx-3) == 'S')) &&
			(m_newfile.GetAt(lenx-4) == '.'))
		{
			m_newfile.SetAt(lenx-1,'i');
			m_newfile.SetAt(lenx-2,'v');
			m_newfile.SetAt(lenx-3,'a');
			m_newfile.SetAt(lenx-4,'.');
		}
		else
		{
			m_newfile += ".avi";
		}
	}*/
	//Ver 1.1
	if (recordaudio) {	
		//Check if file exists  and if so, does it allow overwite		
		HANDLE hfile = CreateFile(
			m_newfile,          // pointer to name of the file
			GENERIC_WRITE,       // access (read-write) mode
			0,           // share mode
			NULL,        // pointer to security attributes
			OPEN_EXISTING,  // how to create
			FILE_ATTRIBUTE_NORMAL,  // file attributes
			NULL        // handle to file with attributes to                            // copy
			);

		if (hfile == INVALID_HANDLE_VALUE) {
			OnUserGeneric();
			return 0;
		}
		else {			
			CloseHandle(hfile);
			//DeleteFile(m_newfile);
		}
		
		int result;
		//Mergefile video with audio		
		//if (recordaudio==2) {
		//if ((recordaudio==2) || (useMCI)) {	
		//ver 2.26 ...overwrite audio settings for Flash Moe recording ... no compression used...
		if ((recordaudio==2) || (useMCI) || (RecordingMode == ModeFlash)) {	
			result = Merge_Video_And_Sound_File(tempfilepath, tempaudiopath, m_newfile, FALSE, pwfx, cbwfx,interleaveFrames,interleaveFactor, interleaveUnit);
		}
		else if (recordaudio==1) {
			result = Merge_Video_And_Sound_File(tempfilepath, tempaudiopath, m_newfile, bAudioCompression, pwfx, cbwfx,interleaveFrames,interleaveFactor, interleaveUnit);
		}

		//Check Results : Attempt Recovery on error		
		if (result==0) {
			//Successful
			DeleteFile(tempfilepath);
			DeleteFile(tempaudiopath);
		}
		else if (result==1) { //video file broken
			//Unable to recover
			DeleteFile(tempfilepath);
			DeleteFile(tempaudiopath);
		}
		else if (result==3) { //this case is rare
			//Unable to recover
			DeleteFile(tempfilepath);
			DeleteFile(tempaudiopath);
		}
		else if ((result==2) || (result==4)) { //recover video file
			//video file is ok, but not audio file
			//so copy the video file as avi	and ignore the audio
			if (!CopyFile( tempfilepath,m_newfile,FALSE)) {
				//Although there is error copying, the temp file still remains in the temp directory and is not deleted, in case user wants a manual recover
				return 0;	
			}
			else {
				DeleteFile(tempfilepath);
				DeleteFile(tempaudiopath);
			} 	
		}
		else if (result == 5) { //recover both files, but as separate files
			CString m_audiofile;
			CString m_audioext(".wav");
			m_audiofile = m_newfile + m_audioext;
			if (!CopyFile( tempfilepath,m_newfile,FALSE)) {
				return 0;					
			}		
			else {
				DeleteFile(tempfilepath);				
			} 	
			if (!CopyFile(tempaudiopath,m_audiofile,FALSE)) {
				return 0;					
			}		
			else {
				DeleteFile(tempaudiopath);
			} 								
		} //if result
	}	   //if record audio
	else { //no audio, just do a plain copy of temp avi to final avi
		//if (!CopyFile( tempfilepath,m_newfile,FALSE)) {
		//	//Ver 1.1
		//	//DeleteFile(m_newfile);
		//	//Repeat this function until success
		//	OnUserGeneric();	
		//	return 0;
		//}
		//else {
		//	DeleteFile(tempfilepath);
		//	if (recordaudio) DeleteFile(tempaudiopath);
		//} 	
	} 
	//ver 2.26
	if (RecordingMode == ModeAVI)
	{	
		//Launch the player
		if (launchPlayer == 1) {
			//调用avi2swf进行转换
			char *pAvi = m_newfile.GetBuffer();
			char *pSwf = szRecord.GetBuffer();
			m_newfile.ReleaseBuffer();
			szRecord.ReleaseBuffer();
			if (Avi2Swf(pAvi,pSwf))
			{
				//删除原始avi文件
			}
		}
		else if (launchPlayer == 3) {
			//调用avi2swf进行转换
			char *pAvi = m_newfile.GetBuffer();
			char *pSwf = szRecord.GetBuffer();
			m_newfile.ReleaseBuffer();
			szRecord.ReleaseBuffer();
			if (Avi2Swf(pAvi,pSwf))
			{
				//删除原始avi文件
			}
		}
	}
	else
	{
		/*
		CString swfname;
		swfname = m_newfile;
		int lenx = m_newfile.GetLength();
		if (((m_newfile.GetAt(lenx-1) == 'i') || (m_newfile.GetAt(lenx-1) == 'I')) &&
		((m_newfile.GetAt(lenx-2) == 'v') || (m_newfile.GetAt(lenx-2) == 'V')) &&
		((m_newfile.GetAt(lenx-3) == 'a') || (m_newfile.GetAt(lenx-3) == 'A')) &&
		(m_newfile.GetAt(lenx-4) == '.'))
		{

		m_newfile.SetAt(lenx-1,'f');
		m_newfile.SetAt(lenx-2,'w');
		m_newfile.SetAt(lenx-3,'s');
		m_newfile.SetAt(lenx-4,'.');

		}
		*/

		//调用avi2swf进行转换
		char *pAvi = m_newfile.GetBuffer();
		char *pSwf = szRecord.GetBuffer();
		m_newfile.ReleaseBuffer();
		szRecord.ReleaseBuffer();
		if (Avi2Swf(pAvi,pSwf))
		{
			//删除原始avi文件
		}
		//ver 2.26
		//SaveProducerCommand();
	}
	return 0;
}
// The main function used in the recording of video
// Includes opening/closing avi file, initializing avi settings, capturing frames, applying cursor effects etc.
int RecordVideo(int top,int left,int width,int height,int fps,const char *szFileName) 
{
	LPBITMAPINFOHEADER alpbi;
	AVISTREAMINFO strhdr;
	PAVIFILE pfile = NULL;
	PAVISTREAM ps = NULL, psCompressed = NULL;
	AVICOMPRESSOPTIONS opts;
	AVICOMPRESSOPTIONS FAR * aopts[1] = {&opts};
	HRESULT hr;
	actualwidth=width;
	actualheight=height;	
	alpbi = NULL;
	char msg[100] = {0};

	if(width%2 == 1){
		width ++;
	}
	if(height%2 == 1){
		height ++;
	}
	sprintf(msg,"%d*%d*%d*%d",left,top,width, height);
	printdblog(msg);

	alpbi=captureScreenFrame(left,top,width, height,1);

	sprintf(msg,"%d*%d",alpbi->biWidth,alpbi->biHeight);
	printdblog(msg);

	//IV50
	if (compfccHandler==mmioFOURCC('I', 'V', '5', '0'))	
	{ //Still Can't Handle Indeo 5.04
		compfccHandler = mmioFOURCC('M', 'S', 'V', 'C');	
		strCodec = CString("MS Video 1");
	}
	////////////////////////////////////////////////
	// INIT AVI USING FIRST FRAME
	////////////////////////////////////////////////
	printdblog("[RecordVideo][AVIFileInit]");
	AVIFileInit();
	//
	// Open the movie file for writing....
	//
	printdblog("[RecordVideo][AVIFileOpen]");
	hr = AVIFileOpen(&pfile, szFileName, OF_WRITE | OF_CREATE, NULL);	
	if (hr != AVIERR_OK) goto error;

	_fmemset(&strhdr, 0, sizeof(strhdr));
	strhdr.fccType                = streamtypeVIDEO;// stream type
	strhdr.fccHandler = 0;
	
	strhdr.dwScale                = 1;
	strhdr.dwRate                 = fps;		    
	strhdr.dwSuggestedBufferSize  = alpbi->biSizeImage;
	SetRect(&strhdr.rcFrame, 0, 0,		    // rectangle for stream
		(int) alpbi->biWidth,
		(int) alpbi->biHeight);

	// And create the stream;
	hr = AVIFileCreateStream(pfile,	&ps, &strhdr);
	if (hr != AVIERR_OK){ 	
		printdblog("[RecordVideo][AVIFileCreateStream error]");
		goto error; 
	}

	memset(&opts, 0, sizeof(opts)); 
	aopts[0]->fccType			 = streamtypeVIDEO;
	aopts[0]->fccHandler		 = compfccHandler;
	aopts[0]->dwKeyFrameEvery	   = keyFramesEvery;		// keyframe rate 
	//控制视频质量的
	aopts[0]->dwQuality		 = compquality;        // compress quality 0-10,000 
	aopts[0]->dwBytesPerSecond	         = 0;		// bytes per second 
	aopts[0]->dwFlags			 = AVICOMPRESSF_VALID | AVICOMPRESSF_KEYFRAMES;    // flags 		
	aopts[0]->lpFormat			 = 0x0;                         // save format 
	aopts[0]->cbFormat			 = 0;
	aopts[0]->dwInterleaveEvery = 0;			// for non-video streams only 

	//ver 2.26
	if (RecordingMode == ModeFlash)
	{
		printdblog("[RecordVideo][ver 2.26]");
		HIC hic = NULL;		
		DWORD fccHandler = mmioFOURCC('X', 'V', 'I', 'D');
		hic = ICOpen(ICTYPE_VIDEO, fccHandler, ICMODE_QUERY);
		if (hic) 
		{
			printdblog("[RecordVideo][xvid]");
			ICClose(hic);
		}	
		else 
		{
			fccHandler = mmioFOURCC('M', 'S', 'V', 'C');	
			strCodec = CString("MS Video 1");
			printdblog("[RecordVideo][msvc]");
		}
		aopts[0]->fccHandler = fccHandler;
	}

	printdblog("[RecordVideo]AVIMakeCompressedStream]");

	hr = AVIMakeCompressedStream(&psCompressed, ps, &opts, NULL);
	if (hr != AVIERR_OK)  	{
		printdblog("[RecordVideo][AVIMakeCompressedStream] error");
		goto error; 
	}

	hr = AVIStreamSetFormat(psCompressed, 0, 
		alpbi,				// stream format
		alpbi->biSize +		// format size
		alpbi->biClrUsed * sizeof(RGBQUAD));
	if (hr != AVIERR_OK) {
		printdblog("[RecordVideo][AVIStreamSetFormat] error");
		goto error;	
	}

	FreeFrame(alpbi);	
	alpbi=NULL;

	DWORD timeexpended, frametime, oldframetime;		
	initialtime = timeGetTime();		
	initcapture = 1;	
	oldframetime = 0;
	nCurrFrame = 0;
	nActualFrame = 0;

	long divx, oldsec;
	divx=0;
	oldsec=0;
	int actcount = 0;
	frametime = 0;
	timeexpended = 0;			
	initcapture = 0;
	int error = 0;
	

	while (recordstate) {  //repeatedly loop		
		printdblog("[RecordVideo][while]");

		// edited by Breeze at 20181029, don't record at free time
		if(recordpaused == 0){
			alpbi=captureScreenFrame(left,top,width, height,0);	
		
			//写入文件，但没有刷新
			hr = AVIStreamWrite(psCompressed,	// stream pointer
				frametime,						// time of this frame
				1,								// number to write
				(LPBYTE) alpbi +				// pointer to data
				alpbi->biSize +
				alpbi->biClrUsed * sizeof(RGBQUAD),
				alpbi->biSizeImage,				// size of this frame
				//AVIIF_KEYFRAME,				// flags....
				0,								//Dependent n previous frame, not key frame
				NULL,
				NULL);
			
			if (hr != AVIERR_OK){
				printdblog("[RecordVideo][AVIStreamWrite]error");
				error = 1;
				break;		
			}

			FreeFrame(alpbi);
			alpbi=NULL;

			Sleep(timelapse);
			timeexpended = timeGetTime() - initialtime;		
			if (timelapse>1000)
				frametime++;
			else
				frametime = (DWORD) (((double) timeexpended /1000.0 ) * (double) (1000.0/timelapse));
		}
		else{
			Sleep(timelapse);
			initialtime += timelapse;
		}		
	} //for loop
error:	
	printdblog("[RecordVideo][end]");
	if ((compfccHandler == CompressorStateIsFor) && (compfccHandler != 0)) {
		aopts[0]->lpParms			 = 0;    
		aopts[0]->cbParms			 = 0;		
	}
	AVISaveOptionsFree(1,(LPAVICOMPRESSOPTIONS FAR *) &aopts);	
	

	
	if (pfile)
		AVIFileClose(pfile);
	if (ps)
		AVIStreamClose(ps);

	//关闭视频流，文件突然变大
	if (psCompressed)
		AVIStreamClose(psCompressed);
	AVIFileExit();	
	printdblog("[RecordVideo][AVIFileExit]");
	if (error == 1) 	{
		OnRecordInterrupted();
		
		if (compfccHandler != mmioFOURCC('M', 'S', 'V', 'C'))	{
				compfccHandler = mmioFOURCC('M', 'S', 'V', 'C');
				strCodec = "MS Video 1";
				//重新录屏幕
				printdblog("[RecordVideo][重新录屏幕]");
				OnRecordStart();
		}
		
		//函数执行完毕
		printdblog("[RecordVideo][设置录像完成信号1]");
		return 0;
	}else{
		//Save the file on success
		OnUserGeneric();
		
		//函数执行完毕
		SetEvent(hFinishEvent);	//设置录像完成信号
		printdblog("[RecordVideo][设置录像完成信号2]");
	}
	return 0;
}

void NormalizeRect(LPRECT prc)
{
	if (prc->right  < prc->left) SWAP(prc->right,  prc->left);
	if (prc->bottom < prc->top)  SWAP(prc->bottom, prc->top);
}

void FixRectSizePos(LPRECT prc,int maxxScreen, int maxyScreen)
{
	NormalizeRect(prc);
	int width=((prc->right)-(prc->left))+1;
	int height=((prc->bottom)-(prc->top))+1;

	if (width>maxxScreen) {
		prc->left=0;
		prc->right=maxxScreen-1;
	}
	if (height>maxyScreen) {
		prc->top=0;
		prc->bottom=maxyScreen-1;
	}
	if (prc->left <0) {
		prc->left= 0;
		prc->right=width-1;
	}
	if (prc->top <0) {
		prc->top= 0;
		prc->bottom=height-1;
	}
	if (prc->right > maxxScreen-1 ) {

		prc->right = maxxScreen-1;
		prc->left= maxxScreen-width;
	}
	if (prc->bottom > maxyScreen-1 ) {

		prc->bottom = maxyScreen-1;
		prc->top= maxyScreen-height;
	}
}

//ver 1.6
CString GetTempPath()
{
	if (tempPath_Access == USE_WINDOWS_TEMP_DIR) {
		char dirx[300];
		GetWindowsDirectory(dirx,300);
		CString tempdir(dirx);
		tempdir = tempdir + "\\temp";			
		//Verify the chosen temp path is valid

		WIN32_FIND_DATA wfd;
		memset(&wfd, 0, sizeof (wfd));	
		HANDLE hdir = FindFirstFile(LPCTSTR(tempdir), &wfd);
		if (!hdir) {
			return GetProgPath();
		}	
		FindClose(hdir);	
		//If valid directory, return Windows\temp as temp directory
		if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
			return tempdir;
		//else return program path as temp directory	
		tempPath_Access = USE_INSTALLED_DIR;
		return GetProgPath();	
	}
	else if (tempPath_Access == USE_USER_SPECIFIED_DIR)
	{
		CString tempdir;
		tempdir = specifieddir;			
		//Verify the chosen temp path is valid		
		WIN32_FIND_DATA wfd;
		memset(&wfd, 0, sizeof (wfd));	
		HANDLE hdir = FindFirstFile(LPCTSTR(tempdir), &wfd);
		if (!hdir) {
			return GetProgPath();
		}	
		FindClose(hdir);	
		//If valid directory, return Windows\temp as temp directory
		if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
			return tempdir;
		//set temp path to installed directory
		tempPath_Access = USE_INSTALLED_DIR;
		return GetProgPath();	
	}
	else  {
		return GetProgPath();	
	}
}

LRESULT OnSaveCursor (UINT wParam, LONG lParam){	
	hSavedCursor = (HCURSOR) wParam;		
	return 0;
}
