#include "stdafx.h"
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

#include <windows.h>
#include <winuser.h>


#define DLL_EXPORT __declspec(dllexport)

#ifdef __GNUC__
#define SHARED __attribute__((section ("SHAREDDATA"), shared))
#else
#define SHARED
#endif

// Shared DATA
#pragma data_seg ( "SHAREDDATA" )
   //int g_event_flag SHARED = 0;
   CTime g_time SHARED = NULL;

// blocks for locally generated events
/*HWND g_block_move_hwnd SHARED = NULL;
unsigned int g_block_move_serial SHARED = 0;
RECT g_block_move SHARED = { 0, 0, 0, 0 };

unsigned int g_blocked_zchange_serial SHARED = 0;
HWND g_blocked_zchange[2] SHARED = { NULL, NULL };

unsigned int g_blocked_focus_serial SHARED = 0;
HWND g_blocked_focus SHARED = NULL;

unsigned int g_blocked_state_serial SHARED = 0;
HWND g_blocked_state_hwnd SHARED = NULL;
int g_blocked_state SHARED = -1;
*/
#pragma data_seg ()

#pragma comment(linker, "/section:SHAREDDATA,rws")

static HHOOK     g_cbt_hook = NULL;
static HHOOK     g_wndproc_hook = NULL;
static HHOOK     g_wndprocret_hook = NULL;
static HHOOK     g_keyboard_hook = NULL;
static HHOOK     g_mouse_hook = NULL;

//static HANDLE    g_PauseEvent=NULL ;

static HANDLE   g_mutex = NULL;
static CTime    g_PausedTime = NULL;

extern int recordstate;
extern int recordpaused;
int pid = 0;

static BOOL is_toplevel(HWND hwnd)
{
	BOOL toplevel;
	HWND parent;
	parent = GetAncestor(hwnd, GA_PARENT);

	toplevel = (!parent || parent == GetDesktopWindow());
	return toplevel;
}

/* Returns true if a window is a menu window. */
static BOOL is_menu(HWND hwnd)
{

	LONG exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);
	return (exstyle & (WS_EX_TOOLWINDOW | WS_EX_TOPMOST));
}

/* Determine the "parent" field for the CREATE response. */
static HWND get_parent(HWND hwnd)
{
	HWND result;
	HWND owner;
	LONG exstyle;

	owner = GetWindow(hwnd, GW_OWNER);
	exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);
	if (!owner && !(exstyle & WS_EX_TOOLWINDOW))
	{
		/* display taskbar icon */
		result = NULL;
	}
	else
	{
		/* no taskbar icon */
		if (owner)
			result = owner;
		else
			result = (HWND) - 1;
	}

	return result;
}

static void wirteStrlog(char *str,int len)
{
	    FILE* f1;
        f1=fopen("c://report.txt","a+");    
		if(f1 == NULL)
			return;
		char sprintBuf[100];
		memset(sprintBuf,0,sizeof(sprintBuf));
		time_t now_sec = time(NULL);
		char timestr[24]={0};
		strftime(timestr, _countof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&now_sec));
		char s[2]={0};
		memcpy(s,str,len);
		fprintf(f1,"%s:%s\n",timestr,s);
		fclose(f1);
}

static void wirtelog(char ch)
{
    wirteStrlog(&ch,1);
}

static void wirteNumLog(int data)
{
	char str[200]={0};
	sprintf(str,"|%d|",data);
    wirteStrlog(str,strlen(str));
}


static LRESULT CALLBACK wndproc_hook_proc(int code, WPARAM cur_thread, LPARAM details)
{
	HWND hwnd;
	UINT msg;
	LONG style;
	wirtelog('3');

	if (code < 0)
		goto end;

	hwnd = ((CWPSTRUCT *) details)->hwnd;
	msg = ((CWPSTRUCT *) details)->message;
	style = GetWindowLong(hwnd, GWL_STYLE);
	if(!(style & WS_VISIBLE) || (style & WS_MINIMIZE))
	{
		goto end;
	}

	switch (msg)
	{
		case WM_WINDOWPOSCHANGED:
		case WM_SETICON:
		case WM_SIZE:
		case WM_MOVE:
		case WM_DESTROY:
			WaitForSingleObject(g_mutex, INFINITE);
			g_time=CTime::GetCurrentTime();
wirtelog('W');
//wirteNumLog(msg);
			ReleaseMutex(g_mutex);
			break;
		default:
			break;
	}
    end:
	return CallNextHookEx(g_wndproc_hook, code, cur_thread, details);
}

static LRESULT CALLBACK wndprocret_hook_proc(int code, WPARAM cur_thread, LPARAM details)
{
	HWND hwnd;
	UINT msg;
	LONG style;
	wirtelog('4');

	if (code < 0)
		goto end;

	hwnd = ((CWPRETSTRUCT *) details)->hwnd;
	msg = ((CWPRETSTRUCT *) details)->message;

	style = GetWindowLong(hwnd, GWL_STYLE);
    if(!(style & WS_VISIBLE) || (style & WS_MINIMIZE))
	{
		goto end;        
	}
	
	switch (msg)
	{
		case WM_WINDOWPOSCHANGED:
		case WM_SETTEXT:
		case WM_SETICON:
			WaitForSingleObject(g_mutex, INFINITE);
			g_time=CTime::GetCurrentTime();
wirtelog('P');
//wirteNumLog(msg);
			ReleaseMutex(g_mutex);
			break;		
		default:
			break;
	}
    end:
	return CallNextHookEx(g_wndprocret_hook, code, cur_thread, details);
}


static LRESULT CALLBACK keyboard_hook_proc(int code, WPARAM cur_thread, LPARAM details)
{
	HWND hwnd;
	UINT msg;
	LONG style;
	wirtelog('1');

	if (code < 0)
		goto end;

	PMSG pmsg = (PMSG)details;

	hwnd = pmsg->hwnd;
	msg =  pmsg->message;

	style = GetWindowLong(hwnd, GWL_STYLE);
    if(!(style & WS_VISIBLE) || (style & WS_MINIMIZE))
	{
		goto end;        
	}
	
	switch (msg)
	{
		case WM_MOUSEWHEEL:
		//case WM_MOUSEMOVE:
		case WM_RBUTTONDOWN:
		case WM_LBUTTONDOWN:
		case WM_IME_COMPOSITION:
		case WM_KEYDOWN:
		    WaitForSingleObject(g_mutex, INFINITE);
		    g_time=CTime::GetCurrentTime();
wirtelog('K');
//wirteNumLog(msg);
		    ReleaseMutex(g_mutex);
			break;		
		default:
			break;
	}
    end:
	return CallNextHookEx(g_keyboard_hook, code, cur_thread, details);
}

static LRESULT CALLBACK mouse_hook_proc(int code, WPARAM cur_thread, LPARAM details)
{
	HWND hwnd;
	LONG style;
	wirtelog('2');
	if (code < 0)
		goto end;
	
	LPMOUSEHOOKSTRUCT pMouseHook=(MOUSEHOOKSTRUCT FAR *) details;

	hwnd = pMouseHook->hwnd;

	//style = GetWindowLong(hwnd, GWL_STYLE);
    //if(!(style & WS_VISIBLE) || (style & WS_MINIMIZE))
	//{
	//	goto end;        
	//}
	DWORD pid = 0;
	DWORD dwTheardId = GetWindowThreadProcessId( hwnd,&pid);
	if (dwTheardId != 0)
	{
		if(pid !=::GetCurrentProcessId())
		{
			goto end;
		}
	}else{
		goto end;
	}
	switch (cur_thread)
	{
		case WM_LBUTTONDOWN:
//		case WM_LBUTTONUP:
//		case WM_MOUSEMOVE:
		case WM_MOUSEWHEEL:
		case WM_RBUTTONDOWN:
//		case WM_RBUTTONUP:
		    WaitForSingleObject(g_mutex, INFINITE);
		    g_time=CTime::GetCurrentTime();
wirtelog('M');
//wirteNumLog(msg);
		    ReleaseMutex(g_mutex);
			break;		
		default:
			break;
	}
    end:
	return CallNextHookEx(g_mouse_hook, code, cur_thread, details);
}


static LRESULT CALLBACK cbt_hook_proc(int code, WPARAM wparam, LPARAM lparam)
{
	HWND hwnd;
	LONG style;
	wirtelog('c');
	if (code < 0)
		goto end;

     hwnd = (HWND) wparam;
	 style = GetWindowLong(hwnd, GWL_STYLE);
	 if (!(style & WS_VISIBLE))
	    goto end;

	switch (code)
	{
		case HCBT_MOVESIZE:      //size 		
		//case HCBT_CLICKSKIPPED:  //mouse
		case HCBT_KEYSKIPPED:    //keyboard
		case HCBT_MINMAX:
			WaitForSingleObject(g_mutex, INFINITE);
			g_time=CTime::GetCurrentTime();
			
//wirtelog('C');
//wirteNumLog(code);

			ReleaseMutex(g_mutex);
		   break;
		default:
			break;
	}

    end:
	return CallNextHookEx(g_cbt_hook, code, wparam, lparam);
}
extern int printdblog(const char* format, ...);
void checkFreeEvent(int s)
{
	printdblog("checkFreeEvent return");
	//return;		//edited by Breeze at 20181029, don't record at free time
	CTime tm;
	CTimeSpan tm_left;
    WaitForSingleObject(g_mutex, INFINITE);
    tm=CTime::GetCurrentTime();
    if(recordpaused==0)
	{	
		 if(g_time!=NULL)
		 {
	        tm_left = tm - g_time;
		    if(tm_left.GetTotalSeconds()>=s){
			  g_PausedTime=CTime::GetCurrentTime();
			  printdblog("recordpaused=1");
		      recordpaused=1; 
			  OutputDebugString("checkFreeEvent == 1\n");
		    }
		 }
	}else{
         if(g_PausedTime==NULL){
		     recordpaused=0;
		 }else{
             tm_left = g_time - g_PausedTime;
		     if(tm_left.GetTotalSeconds()>0){
			 	printdblog("recordpaused=0");
		       recordpaused=0; 
			   OutputDebugString("checkFreeEvent == 0\n");
		     }	
		 }
	}
	ReleaseMutex(g_mutex);
}

void SetHooks(void)
{	
	//return ;
	g_mutex = CreateMutex(NULL, FALSE, NULL);
	
	if (!g_keyboard_hook)
	{
		g_keyboard_hook = SetWindowsHookEx(WH_GETMESSAGE, keyboard_hook_proc, AfxGetApp()->m_hInstance, 0);
		if(g_keyboard_hook == NULL)	
			wirtelog('0');
	}
	/*if (!g_keyboard_hook)
		{
			g_keyboard_hook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_hook_proc, NULL, GetCurrentThreadId());
			if(g_keyboard_hook == NULL)	
				wirtelog('0');
		}*/
	if (!g_mouse_hook)
		g_mouse_hook = SetWindowsHookEx(WH_MOUSE_LL, mouse_hook_proc, AfxGetApp()->m_hInstance, 0);

	if (!g_cbt_hook)
	{
		g_cbt_hook = SetWindowsHookEx(WH_CBT, cbt_hook_proc, AfxGetApp()->m_hInstance, 0);
		if (g_cbt_hook == NULL)
		{
			wirtelog('9');
		}
	}

	if (!g_wndproc_hook)
	{
		g_wndproc_hook = SetWindowsHookEx(WH_CALLWNDPROC, wndproc_hook_proc, AfxGetApp()->m_hInstance, 0);
		if (g_wndproc_hook == NULL)
		{
			wirtelog('8');
		}
	}

	if (!g_wndprocret_hook)
	{
		g_wndprocret_hook = SetWindowsHookEx(WH_CALLWNDPROCRET, wndprocret_hook_proc, AfxGetApp()->m_hInstance, 0);
		if (g_wndprocret_hook == NULL)
		{
			wirtelog('7');
		}
	}

}

void RemoveHooks(void)
{
	if (g_keyboard_hook)
		UnhookWindowsHookEx(g_keyboard_hook);
		
	if (g_mouse_hook)
		UnhookWindowsHookEx(g_mouse_hook);

	if (g_cbt_hook)
		UnhookWindowsHookEx(g_cbt_hook);

	if (g_wndproc_hook)
		UnhookWindowsHookEx(g_wndproc_hook);

	if (g_wndprocret_hook)
		UnhookWindowsHookEx(g_wndprocret_hook);

	  CloseHandle(g_mutex);		
}
