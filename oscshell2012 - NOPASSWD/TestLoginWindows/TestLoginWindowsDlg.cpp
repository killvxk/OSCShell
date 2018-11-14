// TestLoginWindowsDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "TestLoginWindows.h"
#include "TestLoginWindowsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CTestLoginWindowsDlg 对话框




CTestLoginWindowsDlg::CTestLoginWindowsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTestLoginWindowsDlg::IDD, pParent)
	, m_strHost(_T(""))
	, m_strUser(_T(""))
	, m_strPwd(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTestLoginWindowsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_IP, m_strHost);
	DDX_Text(pDX, IDC_USER, m_strUser);
	DDX_Text(pDX, IDC_PWD, m_strPwd);
}

BEGIN_MESSAGE_MAP(CTestLoginWindowsDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON1, &CTestLoginWindowsDlg::OnBnClickedBtnlogin)
END_MESSAGE_MAP()


// CTestLoginWindowsDlg 消息处理程序

BOOL CTestLoginWindowsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CTestLoginWindowsDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CTestLoginWindowsDlg::OnPaint()
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
HCURSOR CTestLoginWindowsDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

typedef struct
{
	HWND hWnd;
	DWORD dwProcessId;
	CHAR szClass[256];
	CHAR szTitle[256];
}EnumFunArg;
bool isFindString(unsigned char *src, unsigned char *dest){
	if (strlen((char*)src) == 0)
	{
		return true;
	}
	else
	{
		if (strstr((char*)src,"*") == NULL)
		{
			if (strcmp((char*)src,(char*)dest) == 0)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			unsigned char szTmpext[256]={0};
			strcpy((char*)szTmpext,(char*)src);
			char *p = strstr((char*)szTmpext,"*");
			*p=0;

			if(strstr((char*)dest,(char*)szTmpext) != NULL){
				return true;
			}else{
				return false;
			}

		}
	}
}

static BOOL CALLBACK lpEnumFunc(HWND hwnd, LPARAM lParam)
{

	EnumFunArg *pArg = reinterpret_cast<EnumFunArg *> (lParam);
	DWORD  dwProcessId;
	GetWindowThreadProcessId(hwnd, &dwProcessId);    
	if( dwProcessId == pArg->dwProcessId  || pArg->dwProcessId == 0)
	{
		CHAR szClass[256]={0};
		CHAR szTitle[256]={0};

		GetClassNameA(hwnd,szClass,256);
		GetWindowTextA(hwnd, szTitle, 256); 


		if(isFindString((unsigned char *)pArg->szClass,(unsigned char *)szClass) == true && isFindString((unsigned char *)pArg->szTitle,(unsigned char *)szTitle) == true)
		{
			pArg->hWnd = hwnd;
			return FALSE;
		}
	}
	return TRUE;
}
static BOOL CALLBACK lpSubEnumFunc(HWND hwnd, LPARAM lParam)
{

	EnumFunArg *pArg = reinterpret_cast<EnumFunArg *> (lParam);
	DWORD  dwProcessId;
	GetWindowThreadProcessId(hwnd, &dwProcessId);    
	if( dwProcessId == pArg->dwProcessId  || pArg->dwProcessId == 0)
	{
		EnumChildWindows(hwnd,lpEnumFunc,lParam);
	}
	return TRUE;
}
HWND GetSubWindowByProcessId(DWORD dwProcessId,CHAR *className,CHAR *titleName,int timeOut){
	int pid_count=0;
	EnumFunArg arg;
	memset(&arg,0,sizeof(EnumFunArg));
	arg.dwProcessId = dwProcessId;
	arg.hWnd = 0;
	if(className!=NULL)
	{
		strcpy(arg.szClass,className); 
	}
	if(className!=NULL)
	{
		strcpy(arg.szTitle,titleName); 
	}

	EnumWindows(lpSubEnumFunc,reinterpret_cast<LPARAM>(&arg));
	while(arg.hWnd==NULL && timeOut>0)
	{
		Sleep(1000);
		pid_count++;
		if(pid_count>timeOut)
		{
			break; 
		}
		EnumWindows(lpSubEnumFunc,reinterpret_cast<LPARAM>(&arg));
	}
	return arg.hWnd;
}
HWND GetWindowByProcessId(DWORD dwProcessId,CHAR *className,int timeOut)
{
	int pid_count=0;
	EnumFunArg arg;
	memset(&arg,0,sizeof(EnumFunArg));
	arg.dwProcessId = dwProcessId;
	arg.hWnd = 0;
	if(className!=NULL)
	{
		strcpy(arg.szClass,className);
	}
	EnumWindows(lpEnumFunc,reinterpret_cast<LPARAM>(&arg));
	while(arg.hWnd==NULL && timeOut>0)
	{
		Sleep(1000);
		pid_count++;
		if(pid_count>timeOut)
		{
			//			 MessageBox(NULL,L"Not Found Process Window!",L"Info",MB_OK);
			break; 
		}
		EnumWindows(lpEnumFunc,reinterpret_cast<LPARAM>(&arg));
	}
	return arg.hWnd;
}
int findShiftKey(char key){

	if(key == '~'){
		return 1;
	}else if(key == '!'){
		return 1;
	}else if(key == '@'){
		return 1;
	}else if(key == '#'){
		return 1;
	}else if(key == '$'){
		return 1;
	}else if(key == '%'){
		return 1;
	}else if(key == '^'){
		return 1;
	}else if(key == '&'){
		return 1;
	}else if(key == '*'){
		return 1;
	}else if(key == '('){
		return 1;
	}else if(key == ')'){
		return 1;
	}else if(key == '_'){
		return 1;
	}else if(key == '+'){
		return 1;
	}else if(key == '{'){
		return 1;
	}else if(key == '}'){
		return 1;
	}else if(key == '|'){
		return 1;
	}else if(key == ':'){
		return 1;
	}else if(key == '\"'){
		return 1;
	}else if(key == '<'){
		return 1;
	}else if(key == '>"'){
		return 1;
	}else if(key == '?'){
		return 1;
	}
	return 0;
}
DWORD XenCenterSSO(LPVOID lparam)
{
	char *ip="192.168.3.22";
	char *user="root";
	char *password="spspHZ!@1017.info";

	HWND hWnd=(HWND)lparam;
	HWND hTopWnd = NULL;
	while(hTopWnd != hWnd){
		::SetForegroundWindow(hWnd);//先将窗口设置为顶级窗口,以便接收到按键响应
		EnableWindow(hWnd,TRUE);
		hTopWnd = GetForegroundWindow();
		Sleep(100);
	}
	/*SendMessageA(hWnd,WM_SYSCOMMAND,SC_KEYMENU,0);
	Sleep(100);
	keybd_event('S',0,0,0);//再按下s键
	Sleep(100);
	keybd_event('S',0,KEYEVENTF_KEYUP,0);//先放开s键
	Sleep(100);
	keybd_event('A',0,0,0);//再按下s键
	Sleep(100);
	keybd_event('A',0,KEYEVENTF_KEYUP,0);//先放开s键

	Sleep(100);
	HWND aWnd = NULL;
	while(aWnd == NULL){
		aWnd = ::FindWindowA("WindowsForms10.Window.8.app.0.378734a","Add New Server");
		Sleep(100);
	}*/

	for(int i = 0 ; i < strlen(ip) ; i ++){
		int vk = VkKeyScan(ip[i]);
		keybd_event(vk,0,0,0);
		Sleep(100);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
	}

	keybd_event(VK_TAB,0,0,0);
	Sleep(100);
	keybd_event(VK_TAB,0,KEYEVENTF_KEYUP,0);

	for(int i = 0 ; i < strlen(user) ; i ++){		
		if (isupper(user[i]) || findShiftKey(password[i])==1)
		{
			keybd_event(VK_SHIFT,0,0,0);
			Sleep(100);
		}
		int vk = VkKeyScan(user[i]);
		keybd_event(vk,0,0,0);
		Sleep(100);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
		if (isupper(user[i]) || findShiftKey(password[i])==1)
		{
			keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
		}
	}

	keybd_event(VK_TAB,0,0,0);
	Sleep(100);
	keybd_event(VK_TAB,0,KEYEVENTF_KEYUP,0);

	for(int i = 0 ; i < strlen(password) ; i ++){
		SendMessage(hWnd,WM_CHAR,password[i],0);
		/*if (isupper(password[i]) || findShiftKey(password[i])==1)
		{
			keybd_event(VK_SHIFT,0,0,0);
			Sleep(100);
		}
		int vk = VkKeyScan(password[i]);
		keybd_event(vk,0,0,0);
		Sleep(100);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
		if (isupper(password[i]) || findShiftKey(password[i])==1)
		{
			keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
		}*/
	}

	keybd_event(VK_RETURN,0,0,0);
	Sleep(100);
	keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);

	return 0;

}
DWORD IPMIView(LPVOID lparam)
{
	/*char *ip="192.168.3.22";
	char *user="administrator";
	char *password="Parav1ew";

	HWND hWnd=(HWND)lparam;
	HWND hTopWnd = NULL;
	while(hTopWnd != hWnd){
		::SetForegroundWindow(hWnd);//先将窗口设置为顶级窗口,以便接收到按键响应
		EnableWindow(hWnd,TRUE);
		hTopWnd = GetForegroundWindow();
		Sleep(100);
	}
	Sleep(3000);
	//HWND hWnd = GetForegroundWindow();
	//SendMessageA(hWnd,WM_SYSCOMMAND,SC_KEYMENU,0);
	Sleep(100);
	keybd_event(18,0,0,0);
	Sleep(100);
	keybd_event(18,0,KEYEVENTF_KEYUP,0);

	Sleep(100);
	keybd_event(VK_RETURN,0,0,0);
	Sleep(100);
	keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);
	Sleep(100);
	keybd_event(VK_DOWN,0,0,0);
	Sleep(100);
	keybd_event(VK_DOWN,0,KEYEVENTF_KEYUP,0);
	Sleep(100);
	keybd_event(VK_RETURN,0,0,0);
	Sleep(100);
	keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);
	Sleep(100);
	keybd_event(VK_DOWN,0,0,0);
	Sleep(100);
	keybd_event(VK_DOWN,0,KEYEVENTF_KEYUP,0);
	Sleep(100);
	keybd_event(VK_RETURN,0,0,0);
	Sleep(100);
	keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);

	keybd_event(VK_TAB,0,0,0);
	Sleep(100);
	keybd_event(VK_TAB,0,KEYEVENTF_KEYUP,0);

	keybd_event(VK_CONTROL,0,0,0);
	keybd_event('A',0,0,0);
	keybd_event('A',0,KEYEVENTF_KEYUP,0);
	keybd_event(VK_CONTROL,0,KEYEVENTF_KEYUP,0);
	keybd_event(VK_BACK,0,0,0);
	keybd_event(VK_BACK,0,KEYEVENTF_KEYUP,0);

	Sleep(100);

	for(int i = 0 ; i < strlen(ip) ; i ++){
		int vk = VkKeyScan(ip[i]);
		keybd_event(vk,0,0,0);
		Sleep(50);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
	}

	Sleep(100);
	keybd_event(VK_RETURN,0,0,0);
	Sleep(100);
	keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);
	Sleep(2000);
	for(int i = 0 ; i < 12 ; i ++){
		keybd_event(VK_TAB,0,0,0);
		Sleep(10);
		keybd_event(VK_TAB,0,KEYEVENTF_KEYUP,0);
	}
	Sleep(100);
	keybd_event(VK_RETURN,0,0,0);
	Sleep(100);
	keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);
	Sleep(500);

	hWnd=GetWindowByProcessId(0,L"Confirm",1);
	while(hWnd == NULL){
		keybd_event(VK_TAB,0,0,0);
		Sleep(10);
		keybd_event(VK_TAB,0,KEYEVENTF_KEYUP,0);
		Sleep(100);
		keybd_event(VK_RETURN,0,0,0);
		Sleep(100);
		keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);
		Sleep(500);
		hWnd=GetWindowByProcessId(0,L"Confirm",1);
	}

	keybd_event(VK_SPACE,0,0,0);
	Sleep(100);
	keybd_event(VK_SPACE,0,KEYEVENTF_KEYUP,0);
	Sleep(1000);
	for(int i = 0 ; i < strlen(user) ; i ++){		
		if (isupper(user[i]) || findShiftKey(password[i])==1)
		{
			keybd_event(VK_SHIFT,0,0,0);
			Sleep(50);
		}
		int vk = VkKeyScan(user[i]);
		keybd_event(vk,0,0,0);
		Sleep(100);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
		if (isupper(user[i]) || findShiftKey(password[i])==1)
		{
			keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
		}
	}

	keybd_event(VK_TAB,0,0,0);
	Sleep(100);
	keybd_event(VK_TAB,0,KEYEVENTF_KEYUP,0);

	for(int i = 0 ; i < strlen(password) ; i ++){
		
		if (isupper(password[i]) || findShiftKey(password[i])==1)
		{
			keybd_event(VK_SHIFT,0,0,0);
			Sleep(100);
		}
		int vk = VkKeyScan(password[i]);
		keybd_event(vk,0,0,0);
		Sleep(50);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
		if (isupper(password[i]) || findShiftKey(password[i])==1)
		{
			keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
		}
	}

	keybd_event(VK_RETURN,0,0,0);
	Sleep(100);
	keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);
*/
	return 0;

}
DWORD WindowsSecurity(LPVOID lparam){
	/*char *user="admin";
	char *password="admin";

	HWND hWnd=(HWND)lparam;
	HWND hTopWnd = NULL;
	while(hTopWnd != hWnd){
		::SetForegroundWindow(hWnd);//先将窗口设置为顶级窗口,以便接收到按键响应
		EnableWindow(hWnd,TRUE);
		hTopWnd = GetForegroundWindow();
		Sleep(100);
	}

	for(int i = 0 ; i < strlen(user) ; i ++){		
		if (isupper(user[i]) || findShiftKey(password[i])==1)
		{
			keybd_event(VK_SHIFT,0,0,0);
			Sleep(50);
		}
		int vk = VkKeyScan(user[i]);
		keybd_event(vk,0,0,0);
		Sleep(100);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
		if (isupper(user[i]) || findShiftKey(password[i])==1)
		{
			keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
		}
	}

	keybd_event(VK_TAB,0,0,0);
	Sleep(100);
	keybd_event(VK_TAB,0,KEYEVENTF_KEYUP,0);

	for(int i = 0 ; i < strlen(password) ; i ++){

		if (isupper(password[i]) || findShiftKey(password[i])==1)
		{
			keybd_event(VK_SHIFT,0,0,0);
			Sleep(100);
		}
		int vk = VkKeyScan(password[i]);
		keybd_event(vk,0,0,0);
		Sleep(50);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
		if (isupper(password[i]) || findShiftKey(password[i])==1)
		{
			keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
		}
	}

	keybd_event(VK_RETURN,0,0,0);
	Sleep(100);
	keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);*/
	return 0;
}
DWORD RadminSSO(LPVOID lparam)
{

	char *user="test";
	char *password="123456";

	HWND hWnd=(HWND)lparam;
	HWND hTopWnd = NULL;
	while(hTopWnd != hWnd){
		::SetForegroundWindow(hWnd);//先将窗口设置为顶级窗口,以便接收到按键响应
		EnableWindow(hWnd,TRUE);
		hTopWnd = GetForegroundWindow();
		Sleep(100);
	}


	for(int i = 0 ; i < strlen(user) ; i ++){		
		if (isupper(user[i]) || findShiftKey(user[i])==1)
		{
			keybd_event(VK_SHIFT,0,0,0);
			Sleep(100);
		}
		int vk = VkKeyScan(user[i]);
		keybd_event(vk,0,0,0);
		Sleep(100);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
		if (isupper(user[i]) || findShiftKey(user[i])==1)
		{
			keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
		}
	}

	keybd_event(VK_TAB,0,0,0);
	Sleep(100);
	keybd_event(VK_TAB,0,KEYEVENTF_KEYUP,0);

	for(int i = 0 ; i < strlen(password) ; i ++){
		if (isupper(password[i]) || findShiftKey(password[i])==1)
		{
			keybd_event(VK_SHIFT,0,0,0);
			Sleep(100);
		}
		int vk = VkKeyScan(password[i]);
		keybd_event(vk,0,0,0);
		Sleep(100);
		keybd_event(vk,0,KEYEVENTF_KEYUP,0);
		if (isupper(password[i]) || findShiftKey(password[i])==1)
		{
			keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
		}
	}

	keybd_event(VK_RETURN,0,0,0);
	Sleep(100);
	keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);

	return 0;

}
#define CLASS_XenCenter     L"IPMI View V2.9.6 (build 120316) - Super Micro Computer, Inc."
#define TITLE_RADMIN		L"Radmin 安全性:*"

int count = 0;
BOOL CALLBACK EnumChildProc(HWND hWnd,LPARAM lParam)
{
	CHAR szBuf[256]={0};
	GetClassNameA(hWnd,szBuf,256);
	if (strstr(szBuf,"Edit"))
	{
		count ++;
	}
	
	return true;
}
void SSOPostpresql(char * ip,int port,char *user,char *pwd,char *dbname)
{
	char szPath[1024] = {0};
	char regData[10240] = {0};
	char regRealData[10240] = {0};
	memset(szPath,0,sizeof(szPath));
	memset(regData,0,sizeof(regData));
	memset(regRealData,0,sizeof(regRealData));

	GetModuleFileNameA(NULL,szPath,MAX_PATH);
	char* slash = strrchr(szPath, static_cast<int>('\\'));
	if (slash)
	{
		*(slash) = '\0';
	}
	strcat(szPath,"\\pgadmin.cfg");
	FILE * fp = fopen(szPath,"r");
	if (fp != NULL)
	{
		fread(regData,1,1024,fp);
		fclose(fp);
	}else{
		return;
	}
	sprintf(regRealData,regData,ip,ip,port,dbname,user);

	memset(szPath,0,sizeof(szPath));
	GetModuleFileNameA(NULL,szPath,MAX_PATH);
	slash = strrchr(szPath, static_cast<int>('\\'));
	if (slash)
	{
		*(slash) = '\0';
	}
	strcat(szPath,"\\pgadmin.reg");
	fp = fopen(szPath,"w");
	if (fp != NULL)
	{
		fwrite(regRealData,1,strlen(regRealData),fp);
		fclose(fp);
	}else{
		return;
	}

	char runParam[1024] = {0};
	sprintf(runParam,"/s \"%s\"",szPath);

	ShellExecuteA(NULL,("open"),"regedit",runParam,NULL,SW_HIDE);

	char autoPwd[1024] = {0};
	memset(autoPwd,0,sizeof(autoPwd));
	sprintf(autoPwd,"%s:%d:*:%s:%s",ip,port,user,pwd);

	memset(szPath,0,sizeof(szPath));
	SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, NULL, szPath);
	strcat(szPath,"\\postgresql\\pgpass.conf");

	fp = fopen(szPath,"w");
	if (fp != NULL)
	{
		fwrite(autoPwd,1,strlen(autoPwd),fp);
		fclose(fp);
	}else{
		return;
	}
}
int FindVideoFormat(char *path)
{
	SECURITY_ATTRIBUTES sa; 
	HANDLE hRead,hWrite; 
	char szPath[1024] = {0};
	char szParam[1024] = {0};
	memset(szPath,0,sizeof(szPath));
	memset(szParam,0,sizeof(szParam));

	GetModuleFileNameA(NULL,szPath,MAX_PATH);
	char* slash = strrchr(szPath, static_cast<int>('\\'));
	if (slash)
	{
		*(slash) = '\0';
	}
	strcat(szPath,"\\ffmpeg.exe");
	sprintf(szParam," -i %s",path);

	sa.nLength = sizeof(SECURITY_ATTRIBUTES); 
	sa.lpSecurityDescriptor = NULL; 
	sa.bInheritHandle = TRUE; 
	if (!CreatePipe(&hRead,&hWrite,&sa,0)) { 
		printf("Error On CreatePipe()\n"); 
		return false; 
	} 
	PROCESS_INFORMATION pi; 
	STARTUPINFOA si; 
	si.cb = sizeof(STARTUPINFO); 
	GetStartupInfoA(&si); 
	si.hStdError = hWrite; 
	si.hStdOutput = hWrite; 
	si.wShowWindow = SW_HIDE; 
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES; 
	if (!CreateProcessA(szPath, szParam, 
		NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi)) { 
			printf("Error on CreateProcess()\n"); 
			return false;   
	} 
	CloseHandle(hWrite); 
	const int BufferLength = 1024; 
	CStringA showedMsg = ""; 
	char buffer[BufferLength + 1] = {0}; 
	memset(buffer,0,sizeof(buffer));
	DWORD bytesRead; 
	while (ReadFile(hRead,buffer,BufferLength,&bytesRead,NULL)) { 
		showedMsg.Append(buffer); 
		memset(buffer,0,sizeof(buffer));
		Sleep(200); 
	} 
	CloseHandle(hRead);
	if (showedMsg.Find("Video: h264") > 0) { 
		
		return 1; 
	}else if (showedMsg.Find("Video: mpeg4") > 0) { 

		return 2; 
	}

	return 0;
}
int GetFileOpeDate(char * path)
{

	HANDLE     hFile;
	FILETIME   filetime;
	FILETIME   localtime;
	SYSTEMTIME systemtime;

	
	hFile = CreateFileA(path, GENERIC_READ | GENERIC_WRITE,0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		GetFileTime(hFile, NULL, NULL, &filetime);      //取得UTC文件时间
		FileTimeToLocalFileTime(&filetime, &localtime); //换成本地时间
		//FileTimeToSystemTime(&localtime, &systemtime);  //换成系统时间格式


		/*TRACE("%04d-%02d-%02d %02d:%02d:%02d/r/n",
			systemtime.wYear, systemtime.wMonth, systemtime.wDay,
			systemtime.wHour, systemtime.wMinute, systemtime.wSecond);*/

		CloseHandle(hFile);

		CTime ftime(localtime);

		//取得当前时间
		time_t t;
		time(&t);
		CTime ctime(t);	
		CTimeSpan span = ctime - ftime;
		int days = span.GetDays();

		//int day = ctime.GetDay();
		//int month = ctime.GetMonth();
		//int year = ctime.GetYear();

		

		if (days <= 1/*day == systemtime.wDay && month == systemtime.wMonth && year == systemtime.wYear*/)
		{
			return 1;
		}



	}
	return 0;
}
void CompareFile(char * path)
{
	if(FindVideoFormat(path) != 2)
	{
		return;
	}
	char cmpCmd[1024] = {0};
	char cmpPath[1024] = {0};
	char bakPath[1024] = {0};
	sprintf(cmpPath,"%s.cmp.avi",path);
	sprintf(bakPath,"%s.bak",path);
	sprintf(cmpCmd," -i %s -vcodec libx264 %s",path,cmpPath);


	SECURITY_ATTRIBUTES sa; 
	HANDLE hRead,hWrite; 
	char szPath[1024] = {0};
	memset(szPath,0,sizeof(szPath));

	GetModuleFileNameA(NULL,szPath,MAX_PATH);
	char* slash = strrchr(szPath, static_cast<int>('\\'));
	if (slash)
	{
		*(slash) = '\0';
	}
	strcat(szPath,"\\ffmpeg.exe");

	sa.nLength = sizeof(SECURITY_ATTRIBUTES); 
	sa.lpSecurityDescriptor = NULL; 
	sa.bInheritHandle = TRUE; 
	if (!CreatePipe(&hRead,&hWrite,&sa,0)) { 
		printf("Error On CreatePipe()\n"); 
		return ; 
	} 
	PROCESS_INFORMATION pi; 
	STARTUPINFOA si; 
	si.cb = sizeof(STARTUPINFO); 
	GetStartupInfoA(&si); 
	si.hStdError = hWrite; 
	si.hStdOutput = hWrite; 
	si.wShowWindow = SW_HIDE; 
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES; 
	if (!CreateProcessA(szPath, cmpCmd, 
		NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi)) { 
			printf("Error on CreateProcess()\n"); 
			return ;   
	} 
	CloseHandle(hWrite); 
	const int BufferLength = 1024; 
	CStringA showedMsg = ""; 
	char buffer[BufferLength + 1] = {0}; 
	memset(buffer,0,sizeof(buffer));
	DWORD bytesRead; 
	while (ReadFile(hRead,buffer,BufferLength,&bytesRead,NULL)) { 
		showedMsg.Append(buffer); 
		memset(buffer,0,sizeof(buffer));
		Sleep(200); 
	} 

	if (showedMsg.Find("libx264 @") < 0) { 
	
			return ; 
	}
	if(MoveFileA(path,bakPath) == FALSE)
		return;
	if (MoveFileA(cmpPath,path) == FALSE)
	{
		MoveFileA(bakPath,path);
	}
	DeleteFileA(bakPath);

}
/*CStringA CStrW2CStrA(const CStringW &cstrSrcW)
{
	int len = WideCharToMultiByte(CP_ACP, 0, LPCWSTR(cstrSrcW), -1, NULL, 0, NULL, NULL);
	char *str = new char[len];
	memset(str, 0, len);
	WideCharToMultiByte(CP_ACP, 0, LPCWSTR(cstrSrcW), -1, str, len, NULL, NULL);
	CStringA cstrDestA = str;
	delete[] str;

	return cstrDestA;
}
CStringW CStrA2CStrW(const CStringA &cstrSrcA)
{
	int len = MultiByteToWideChar(CP_ACP, 0, LPCSTR(cstrSrcA), -1, NULL, 0);
	wchar_t *wstr = new wchar_t[len];
	memset(wstr, 0, len*sizeof(wchar_t));
	MultiByteToWideChar(CP_ACP, 0, LPCSTR(cstrSrcA), -1, wstr, len);
	CStringW cstrDestW = wstr;
	delete[] wstr;

	return cstrDestW;
}*/
void CompareDir(char * dir,bool isnow)
{
	char szFind[MAX_PATH];
    char szFile[MAX_PATH];

    WIN32_FIND_DATAA FindFileData;

    strcpy(szFind,dir);
    strcat(szFind,"*.*");

    HANDLE hFind=::FindFirstFileA(szFind,&FindFileData);
    if(INVALID_HANDLE_VALUE == hFind)    return;
    while(TRUE)
    {
        if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
        }
        else
		{      
			char filepath[300] = {0};
			memset(filepath,0,sizeof(filepath));
			sprintf(filepath,"%s%s",dir,FindFileData.cFileName);
			if (isnow)
			{
				if(GetFileOpeDate(filepath) != 1)
				{
					continue;
				}
			}
			CompareFile(filepath);
        }
        if(!FindNextFileA(hFind,&FindFileData))
            break;
    }
    FindClose(hFind); 

	/*CStringA strName;
	strName.Format("%s*.*",dir);//strDir:要查找的目录
	char chFile[MAX_PATH];
	char chTemp[MAX_PATH];
	CFileFind find;
	BOOL bFinished=find.FindFile(CStrA2CStrW(strName));

	while(bFinished)
	{
		bFinished =find.FindNextFile();
		if (!find.IsDots())
		{
			if(!find.IsDirectory())   
			{
				CStringA filepath = CStrW2CStrA(find.GetFilePath());
				if (isnow)
				{
					if(GetFileOpeDate(filepath.GetBuffer()) != 1)
					{
						continue;
					}
				}
				CompareFile(filepath.GetBuffer());
			}
			

		}
	}*/
}

BOOL DirectoryExist(CString Path)
{
	WIN32_FIND_DATA fd;
	BOOL ret = FALSE;
	if (Path.Right(1) == "\\")
		Path = Path.Left(Path.GetLength()-1);

	HANDLE hFind = FindFirstFile(Path, &fd);
	if ((hFind != INVALID_HANDLE_VALUE) && (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{  //目录存在
		ret = TRUE;    
	}
	FindClose(hFind);
	return ret;
}


DWORD WINAPI RefalshSCPTitleThread(LPVOID lparam)
{
	DWORD dwProcessId = *(DWORD *)lparam;
	HWND hWnd=GetWindowByProcessId(9144,"TScpCommanderForm",60);
	if(hWnd==NULL)
	{
		return 0;
	}
	char puttyTitle[1024] = {0};
	sprintf(puttyTitle,"%s@%s","root","192.168.1.1");
	while(true){
		char title[1024] = {0};
		GetWindowTextA(hWnd,title,1024);
		char *p = strstr(title," - ");
		if (p != NULL)
		{
			p += 3;
			*p = 0;
		}
		strcat(title,puttyTitle);
		SetWindowTextA(hWnd , title );
		Sleep(2000);
	}

}
void CTestLoginWindowsDlg::OnBnClickedBtnlogin()
{

	char szPath[MAX_PATH] = {0};
	SHGetSpecialFolderPathA(NULL,szPath,CSIDL_APPDATA,FALSE);
	WritePrivateProfileStringA("LogonHistory",NULL,NULL,"C:\\Users\\Administrator\\Desktop\\user.prefs");  
	/*HKEY hkey;
	LONG res; 
	DWORD datatype=REG_SZ; 
	char szvalue[_MAX_PATH];
	strcpy((char*)szvalue,"");

	res =::RegOpenKeyExA(HKEY_CURRENT_USER, 
		"Software\\SimonTatham\\PuTTY\\Sessions\\Default%20Settings", 0, 
		KEY_WRITE|KEY_READ, &hkey);

	if(res==ERROR_SUCCESS)
	{

		res = ::RegSetValueExA(hkey, "LogFileName", 0, datatype, (const BYTE *)szvalue, strlen(szvalue));
		RegCloseKey(hkey);
	}*/
	
	
	//DWORD dwProcessId = 9144;
	//CreateThread(NULL,NULL,RefalshSCPTitleThread,(LPVOID)&dwProcessId,0,0);
	//return;
//	DirectoryExist(L"c:\\log\\");
//	CompareDir("e:\\video\\",true);

	/*SECURITY_ATTRIBUTES sa; 
	HANDLE hRead,hWrite; 
	sa.nLength = sizeof(SECURITY_ATTRIBUTES); 
	sa.lpSecurityDescriptor = NULL; 
	sa.bInheritHandle = TRUE; 
	if (!CreatePipe(&hRead,&hWrite,&sa,0)) { 
		printf("Error On CreatePipe()\n"); 
		return ; 
	} 
	PROCESS_INFORMATION pi; 
	STARTUPINFOA si; 
	si.cb = sizeof(STARTUPINFO); 
	GetStartupInfoA(&si); 
	si.hStdError = hWrite; 
	si.hStdOutput = hWrite; 
	si.wShowWindow = SW_HIDE; 
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES; 
	if (!CreateProcessA("E:\\work\\ffmpeg-win64-static\\ffmpeg\\bin\\ffmpeg.exe", " -i e:\\2.avi", 
		NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi)) { 
			printf("Error on CreateProcess()\n"); 
			return ;   
	} 
	CloseHandle(hWrite); 
	const int BufferLength = 1024; 
	CStringA showedMsg = ""; 
	char buffer[BufferLength + 1] = {0}; 
	DWORD bytesRead; 
	while (ReadFile(hRead,buffer,BufferLength,&bytesRead,NULL)) { 
		showedMsg.Append(buffer); 
		Sleep(200); 
	} 
	if (showedMsg.Find("find Bugs") > 0) { 
		printf("Error \n"); 
		return ; 
	}*/
	//char dbname[10] = {0};

	//SSOPostpresql("192.168.1.95",5432,"postgres","Parav1ew",dbname);
	//HWND hWnd=GetSubWindowByProcessId(3692,"IHWindowClass","Input Capture Window",30);
	//if(hWnd==NULL)
	//{
	//	return;
	//}
	//::SetWindowTextA(hWnd , "testuser@192.168.1.1" );
	/*STARTUPINFO   StartInfo;   
	PROCESS_INFORMATION     ProceInfo;   
	ZeroMemory(&StartInfo,sizeof(StartInfo));   
	ZeroMemory(&ProceInfo,sizeof(ProceInfo));   
	StartInfo.cb=sizeof(StartInfo); 
	StartInfo.dwFlags =STARTF_USESHOWWINDOW;
	StartInfo.wShowWindow = SW_MAXIMIZE;
	CreateProcess(NULL, 
		L"\"C:\\osc3.5\\tools\\Microsoft SQL Server\\100\Tools\\Binn\\VSShell\\Common7\\IDE\\Ssms.exe",   //lpApplicationName:   PChar   
		 
		NULL,   //lpProcessAttributes:   PSecurityAttributes   
		NULL,   //lpThreadAttributes:   PSecurityAttributes   
		//	TRUE,   //bInheritHandles:   BOOL   
		FALSE,
		CREATE_NEW_CONSOLE|NORMAL_PRIORITY_CLASS,
		//	CREATE_NEW_CONSOLE,   
		NULL,   
		NULL,   
		&StartInfo,   
		&ProceInfo); 

	HWND hWnd=GetWindowByProcessId(ProceInfo.dwProcessId,TITLE_RADMIN,30);
	if(hWnd==NULL)
	{
		return;
	}*/
	//::EnumChildWindows(hWnd,EnumChildProc,0);
	//while(count==0){
	//	Sleep(1000);
	//}
	//Sleep(1000);
	//DeleteFile(L"IPMIView.properties");
	//UpdateData(TRUE);
	//RadminSSO(hWnd);
	/*char * szCmdline = "http://localhost:8080/manager/html";

	SHELLEXECUTEINFOA ShExecInfo = {0};
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL;
	ShExecInfo.lpFile = "iexplore.exe";
	ShExecInfo.lpParameters =szCmdline;
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_SHOW;
	ShExecInfo.hInstApp = NULL;
	ShellExecuteExA(&ShExecInfo);
	//ProceInfo.hProcess = ShExecInfo.hProcess;
	//ProceInfo.dwProcessId = GetProcessId(ShExecInfo.hProcess);

	HWND hWnd=GetWindowByProcessId(0,L"Windows Security",2);
	if(hWnd == NULL){
		hWnd=GetWindowByProcessId(0,L"Windows 安全",2);
		if (hWnd == NULL)
		{
			TCHAR title_msg[100] = {0};
			wsprintf(title_msg,L"连接到 %s",L"192.168.1.1");
			hWnd=GetWindowByProcessId(0,title_msg,2);
			if (hWnd != NULL)
			{
				WindowsSecurity(hWnd);
			}
		}else{
			WindowsSecurity(hWnd);
		}
	}else{
		WindowsSecurity(hWnd);
	}*/

}

