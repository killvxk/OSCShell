// MouseHook.cpp : Defines the entry point for the DLL application.
//


#include "stdafx.h"
#define _COMPILING_44E531B1_14D3_11d5_A025_006067718D04
#include "Hook.h"
#include <stdio.h>
#include <mmsystem.h>

#pragma data_seg(".SHARE")
UINT nMsg = 0;
HHOOK hook = NULL;
unsigned long oldKeyMouseTime = 0;
#pragma data_seg()
#pragma comment(linker, "/section:.SHARE,rws")
HINSTANCE hInst;


UINT WM_USER_RECORDINTERRUPTED;
UINT WM_USER_SAVECURSOR;
UINT WM_USER_GENERIC;
UINT WM_USER_RECORDSTART;


static LRESULT CALLBACK hookproc(UINT nCode, WPARAM wParam, LPARAM lParam);


__declspec(dllexport) BOOL InstallMyHook(UINT message_to_call)
    {
     if(hook != NULL)
	return FALSE; // already hooked!
     hook = SetWindowsHookEx(WH_GETMESSAGE,
			    (HOOKPROC)hookproc,
			    hInst,
			    0);
     if(hook != NULL)
	{ /* success */
	 nMsg = message_to_call;
	 return TRUE;
	} /* success */
     return FALSE; // failed to set hook
    } // setMyHook

__declspec(dllexport) BOOL UninstallMyHook()
    {
     if(hook == NULL)
	return FALSE;
     BOOL unhooked = UnhookWindowsHookEx(hook);
     if(unhooked)
	hook = NULL;
     return unhooked;
    } // clearMyHook


LRESULT OnSaveCursor (UINT wParam, LONG lParam);
static LRESULT CALLBACK hookproc(UINT nCode, WPARAM wParam, LPARAM lParam)
{
    if(nCode < 0)
	{ /* pass it on */
	 CallNextHookEx(hook, nCode, wParam, lParam);
	 return 0;
	} /* pass it on */
     
	 
	 LPMSG msg = (LPMSG)lParam;     
	

	 //ver 1.2
	 if(msg->message == WM_MOUSEMOVE || msg->message == WM_NCMOUSEMOVE || msg->message == WM_LBUTTONDOWN || (msg->message == WM_LBUTTONUP)) {

		 unsigned long currentKeyMouseTime = timeGetTime();
		 unsigned long difftime = currentKeyMouseTime - oldKeyMouseTime;
		 if (difftime>20) {	//up to 50 frames per second	 

			HCURSOR hcur= GetCursor();		 		 
			//PostMessage(hWndServer, WM_USER_SAVECURSOR , (unsigned int) hcur, msg->message);
			OnSaveCursor ((unsigned int) hcur, msg->message);
			oldKeyMouseTime = currentKeyMouseTime;
		 
		 }
		 

	 } 	 	 
	 
     return CallNextHookEx(hook, nCode, wParam, lParam);    
    
} 
