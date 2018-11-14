// TestViewDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "TestView.h"
#include "TestViewDlg.h"
#include <string>
#include <time.h>
#include <stdio.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTestViewDlg 对话框




CTestViewDlg::CTestViewDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTestViewDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTestViewDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CTestViewDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON1, &CTestViewDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CTestViewDlg::OnBnClickedButton2)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CTestViewDlg 消息处理程序

BOOL CTestViewDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CTestViewDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CTestViewDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
typedef void (*pOnRecord)(char *szRec);
typedef void (*pOnStop)();

static char* getTime()
{
    time_t timep;
    time (&timep);
    char tmp[64];
    strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S",localtime(&timep) );
    return tmp;
}

void CTestViewDlg::OnBnClickedButton1()
{
	HMODULE hmodule = LoadLibrary(_T("recordview.dll"));
	pOnRecord OnRecord = (pOnRecord)GetProcAddress(hmodule,"OnRecord");
	(*OnRecord)("c:\\test1.avi");
}

void CTestViewDlg::OnBnClickedButton2()
{
	HMODULE hmodule = LoadLibrary("recordview.dll");
	pOnStop OnStop = (pOnStop)GetProcAddress(hmodule,"OnStop");
	(*OnStop)();
}

DWORD static WINAPI CloseRecord(LPVOID lparam)
{
	OutputDebugString(getTime());
	OutputDebugString("   2--stop record...");
	OutputDebugString("\r\n");
	HMODULE hmodule = LoadLibrary("recordview.dll");
	pOnStop OnStop = (pOnStop)GetProcAddress(hmodule,"OnStop");
	(*OnStop)();
	OutputDebugString(getTime());
	OutputDebugString("   2--stop record end.");
	OutputDebugString("\r\n");
	return 0;
}

void CTestViewDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	DWORD  dwThreadId1;	
	HMODULE hmodule = LoadLibrary("recordview.dll");
	pOnStop OnStop = (pOnStop)GetProcAddress(hmodule,"OnStop");

	CreateThread(NULL,NULL,CloseRecord,NULL,0,&dwThreadId1);

	OutputDebugString(getTime());
	OutputDebugString("   1--stop record...");
	(*OnStop)();
	OutputDebugString(getTime());
	OutputDebugString("   1--stop record end.");
	CDialog::OnClose();
}