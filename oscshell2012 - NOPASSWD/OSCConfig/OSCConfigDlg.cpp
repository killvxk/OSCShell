// OSCConfigDlg.cpp : 实现文件
//

#include "stdafx.h"
#include <io.h>
#include "OSCConfig.h"
#include "OSCConfigDlg.h"
#include "tinyxml.h"
#include <string>
#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


char audit[]				="audit";
char audit_ip[]				= "ip";
char audit_user[]			= "user";
char audit_pwd[]			= "pwd";
char audit_port[]			= "port";
char audit_dbname[]			= "dbname";
char audit_dbtype[]			= "dbtype";

char osc[]					= "osc";
char osc_ip[]				= "ip";
char osc_user[]				= "user";
char osc_pwd[]				= "pwd";
char osc_port[]				= "port";
char osc_dbname[]			= "dbname";
char osc_dbtype[]			= "dbtype";

char node[]					= "node";
char node_code[]			= "code";
char node_level[]			= "level";
char node_monitorPort[]		= "monitorPort";
char node_videoPath[]		= "videoPath";
char node_warningTimes_1[]	= "warningTimes_1";
char node_warningTimes_2[]	= "warningTimes_2";
char node_warningTimes_3[]	= "warningTimes_3";

char WMI[]					= "WMIservice";
char WMI_licenseport[]		= "licenseport";
char WMI_wmiport[]			= "wmiport";
char WMI_tinterval[]		= "tinterval";
char WMI_dPrinter[]			= "dPrinter";

char NFS[]					= "NFS";
char NFS_cmd[]				= "cmd";



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


// COSCConfigDlg 对话框




COSCConfigDlg::COSCConfigDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COSCConfigDlg::IDD, pParent)
	, m_strAuditIp(_T(""))
	, m_strAuditUser(_T(""))
	, m_strAuditPwd(_T(""))
	, m_strAuditPort(_T(""))
	, m_strAuditDbname(_T(""))
	, m_strAuditDbtype(_T(""))
	, m_strOscip(_T(""))
	, m_strOscUser(_T(""))
	, m_strOscPwd(_T(""))
	, m_strOscPort(_T(""))
	, m_strOscDbname(_T(""))
	, m_strOscDbtype(_T(""))
	, m_strNodeCode(_T(""))
	, m_strNodeLevel(_T(""))
	, m_strNodeMonitorPort(_T(""))
	, m_strNodeVideoPath(_T(""))
	, m_strNodeWarningTime1(_T(""))
	, strNodeWarningTime2(_T(""))
	, strNodeWarningTime3(_T(""))
	, m_strWMIPort(_T(""))
	, m_strWMITinterval(_T(""))
	, m_strWMIDPrinter(_T(""))
	, m_strNFSCmd(_T(""))
	, m_strLicport(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	isinstall = 0;
}

void COSCConfigDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_AUDIT_IP, m_strAuditIp);
	DDX_Text(pDX, IDC_AUDIT_USER, m_strAuditUser);
	DDX_Text(pDX, IDC_AUDIT_PWD, m_strAuditPwd);
	DDX_Text(pDX, IDC_AUDIT_PORT, m_strAuditPort);
	DDX_Text(pDX, IDC_AUDIT_DBNAME, m_strAuditDbname);
	DDX_Text(pDX, IDC_COMBO1, m_strAuditDbtype);
	DDX_Text(pDX, IDC_OSC_IP, m_strOscip);
	DDX_Text(pDX, IDC_OSC_USER, m_strOscUser);
	DDX_Text(pDX, IDC_OSC_PWD, m_strOscPwd);
	DDX_Text(pDX, IDC_OSC_PORT, m_strOscPort);
	DDX_Text(pDX, IDC_OSC_DBNAME, m_strOscDbname);
	DDX_Text(pDX, IDC_COMBO2, m_strOscDbtype);
	DDX_Text(pDX, IDC_NODE_CODE, m_strNodeCode);
	DDX_Text(pDX, IDC_NODE_LEVEL, m_strNodeLevel);
	DDX_Text(pDX, IDC_NODE_MONITORPORT, m_strNodeMonitorPort);
	DDX_Text(pDX, IDC_NODE_VIDEOPATH, m_strNodeVideoPath);
	DDX_Text(pDX, IDC_NODE_WARNINGTIME_1, m_strNodeWarningTime1);
	DDX_Text(pDX, IDC_NODE_WARNINGTIME_2, strNodeWarningTime2);
	DDX_Text(pDX, IDC_NODE_WARNINGTIME_3, strNodeWarningTime3);
	DDX_Text(pDX, IDC_WMI_PORT, m_strWMIPort);
	DDX_Text(pDX, IDC_WMI_TINTERVAL, m_strWMITinterval);
	DDX_Text(pDX, IDC_WMI_DPRINTER, m_strWMIDPrinter);
	DDX_Text(pDX, IDC_NFS_CMD, m_strNFSCmd);
	DDX_Text(pDX, IDC_WMI_LICPORT, m_strLicport);
}

BEGIN_MESSAGE_MAP(COSCConfigDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_SAVE, &COSCConfigDlg::OnBnClickedSave)
END_MESSAGE_MAP()


// COSCConfigDlg 消息处理程序

BOOL COSCConfigDlg::OnInitDialog()
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

	OnUpdateConfig();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void COSCConfigDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void COSCConfigDlg::OnPaint()
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
HCURSOR COSCConfigDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void COSCConfigDlg::OnBnClickedSave()
{
	UpdateData(TRUE);

	char cfgPath[_MAX_PATH] = {0};
	char basePath[_MAX_PATH] = {0};
	char dbTypePath[_MAX_PATH] = {0};
	char oscDbCfgPath[_MAX_PATH] = {0};
	char auditDbCfgPath[_MAX_PATH] = {0};
	::GetModuleFileName(NULL, basePath, _MAX_PATH);
	LPTSTR tszSlash = _tcsrchr(basePath, static_cast<int>('\\'));
	if (tszSlash)
	{
		*++tszSlash = '\0';
	}
	strcpy(cfgPath, basePath);
	strcat(cfgPath,"cfg.ini");

	sprintf(dbTypePath, "%sConfig\\DBConfigure.xml",basePath);

	//WritePrivateProfileString(audit,audit_ip,m_strAuditIp.GetBuffer(),szFilePath);
	//WritePrivateProfileString(audit,audit_user,m_strAuditUser.GetBuffer(),szFilePath);
	//WritePrivateProfileString(audit,audit_pwd,m_strAuditPwd.GetBuffer(),szFilePath);
	//WritePrivateProfileString(audit,audit_port,m_strAuditPort.GetBuffer(),szFilePath);
	//WritePrivateProfileString(audit,audit_dbname,m_strAuditDbname.GetBuffer(),szFilePath);
	//WritePrivateProfileString(audit,audit_dbtype,m_strAuditDbtype.GetBuffer(),szFilePath);

	//WritePrivateProfileString(osc,osc_ip,m_strOscip.GetBuffer(),szFilePath);
	//WritePrivateProfileString(osc,osc_user,m_strOscUser.GetBuffer(),szFilePath);
	//WritePrivateProfileString(osc,osc_pwd,m_strOscPwd.GetBuffer(),szFilePath);
	//WritePrivateProfileString(osc,osc_port,m_strOscPort.GetBuffer(),szFilePath);
	//WritePrivateProfileString(osc,osc_dbname,m_strOscDbname.GetBuffer(),szFilePath);
	//WritePrivateProfileString(osc,osc_dbtype,m_strOscDbtype.GetBuffer(),szFilePath);

	WritePrivateProfileString(node,node_code,m_strNodeCode.GetBuffer(),cfgPath);
	WritePrivateProfileString(node,node_level,m_strNodeLevel.GetBuffer(),cfgPath);
	WritePrivateProfileString(node,node_monitorPort,m_strNodeMonitorPort.GetBuffer(),cfgPath);
	WritePrivateProfileString(node,node_videoPath,m_strNodeVideoPath.GetBuffer(),cfgPath);
	WritePrivateProfileString(node,node_warningTimes_1,m_strNodeWarningTime1.GetBuffer(),cfgPath);
	WritePrivateProfileString(node,node_warningTimes_2,strNodeWarningTime2.GetBuffer(),cfgPath);
	WritePrivateProfileString(node,node_warningTimes_3,strNodeWarningTime3.GetBuffer(),cfgPath);

	WritePrivateProfileString(WMI,WMI_licenseport,m_strLicport.GetBuffer(),cfgPath);
	WritePrivateProfileString(WMI,WMI_wmiport,m_strWMIPort.GetBuffer(),cfgPath);
	WritePrivateProfileString(WMI,WMI_tinterval,m_strWMITinterval.GetBuffer(),cfgPath);
	WritePrivateProfileString(WMI,WMI_dPrinter,m_strWMIDPrinter.GetBuffer(),cfgPath);

	WritePrivateProfileString(NFS,NFS_cmd,m_strNFSCmd.GetBuffer(),cfgPath);
	
	bool exist = _access(dbTypePath, 4) == 0;
	if (exist)
	{
		TiXmlDocument doc(dbTypePath);
		bool loadOk = doc.LoadFile();
		if (loadOk)
		{
			TiXmlElement *root = doc.RootElement();
			TiXmlNode *type = root->FirstChild("dbtype");
			//type->ToText()->SetValue(m_strOscDbtype.GetBuffer());
			type->Clear();
			TiXmlText *value = new TiXmlText(m_strOscDbtype.GetBuffer());
			type->LinkEndChild(value);
			doc.SaveFile();
		}
	}

	sprintf(oscDbCfgPath, "%sConfig\\%s.cfg.xml",basePath, m_strOscDbtype.GetBuffer());
	exist = _access(oscDbCfgPath, 4) == 0;
	if (exist)
	{
		TiXmlDocument doc(oscDbCfgPath);
		bool loadOk = doc.LoadFile();
		if (loadOk)
		{
			TiXmlElement *root = doc.RootElement();
			TiXmlNode *factory = root->FirstChild();
			TiXmlNode *child = factory->FirstChild("property");
			TiXmlNode *dbInfo = factory->IterateChildren(child);
			if (m_strOscDbtype.Compare("MySql") == 0)
			{
				char info[200] = {0};
				sprintf(info, "Database=%s;Data Source=%s;Port=%s;User Id=%s;Password=%s",m_strOscDbname.GetBuffer(),m_strOscip.GetBuffer(), m_strOscPort.GetBuffer(), m_strOscUser.GetBuffer(), m_strOscPwd.GetBuffer());
				dbInfo->Clear();
				TiXmlText *value = new TiXmlText(info);
				dbInfo->LinkEndChild(value);
				doc.SaveFile();
			}
			else if (m_strOscDbtype.Compare("MsSql") == 0)
			{
				char info[200] = {0};
				sprintf(info, "server=%s;database=%s;port=%s;uid=%s;pwd=%s;",m_strOscip.GetBuffer(),m_strOscDbname.GetBuffer(), m_strOscPort.GetBuffer(), m_strOscUser.GetBuffer(), m_strOscPwd.GetBuffer());
				dbInfo->Clear();
				TiXmlText *value = new TiXmlText(info);
				dbInfo->LinkEndChild(value);
				doc.SaveFile();
			}
			else
			{
			}
		}
	}

	sprintf(auditDbCfgPath, "%sConfig\\%sAudit.cfg.xml",basePath, m_strAuditDbtype.GetBuffer());
	exist = _access(auditDbCfgPath, 4) == 0;
	if (exist)
	{
		TiXmlDocument doc(auditDbCfgPath);
		bool loadOk = doc.LoadFile();
		if (loadOk)
		{
			TiXmlElement *root = doc.RootElement();
			TiXmlNode *factory = root->FirstChild();
			TiXmlNode *child = factory->FirstChild("property");
			TiXmlNode *dbInfo = factory->IterateChildren(child);
			if (m_strAuditDbtype.Compare("MySql") == 0)
			{
				char info[200] = {0};
				sprintf(info, "Database=%s;Data Source=%s;Port=%s;User Id=%s;Password=%s",m_strAuditDbname.GetBuffer(),m_strAuditIp.GetBuffer(), m_strAuditPort.GetBuffer(), m_strAuditUser.GetBuffer(), m_strAuditPwd.GetBuffer());
				dbInfo->Clear();
				TiXmlText *value = new TiXmlText(info);
				dbInfo->LinkEndChild(value);
				doc.SaveFile();
			}
			else if (m_strAuditDbtype.Compare("MsSql") == 0)
			{
				char info[200] = {0};
				sprintf(info, "server=%s;database=%s;port=%s;uid=%s;pwd=%s;",m_strAuditIp.GetBuffer(),m_strAuditDbname.GetBuffer(), m_strAuditPort.GetBuffer(), m_strAuditUser.GetBuffer(), m_strAuditPwd.GetBuffer());
				dbInfo->Clear();
				TiXmlText *value = new TiXmlText(info);
				dbInfo->LinkEndChild(value);
				doc.SaveFile();
			}
			else
			{
			}
		}
	}

	if (MessageBox("配置完成","info",MB_OK) == IDOK && this->isinstall == 1)
	{
		TCHAR szFilePath[_MAX_PATH];
		::GetModuleFileName(NULL, szFilePath, _MAX_PATH);
		LPTSTR tszSlash = _tcsrchr(szFilePath, static_cast<int>('\\'));
		if (tszSlash)
		{
			*++tszSlash = '\0';
		}
		strcat(szFilePath,"NFS.exe");
		ShellExecute(NULL,"open",szFilePath,"-Service","", SW_SHOWNORMAL);

		WinExec("bcdedit /set testsigning on",SW_HIDE);

		TCHAR szCmd[_MAX_PATH]={0};
		/*::GetModuleFileName(NULL, szFilePath, _MAX_PATH);
		tszSlash = _tcsrchr(szFilePath, static_cast<int>('\\'));
		if (tszSlash)
		{
			*++tszSlash = '\0';
		}
		wsprintf(szCmd,"InfDefaultInstall.exe \"%scancelSafe.inf\"",szFilePath);

		WinExec(szCmd,SW_HIDE);*/

		
		::GetModuleFileName(NULL, szFilePath, _MAX_PATH);
		tszSlash = _tcsrchr(szFilePath, static_cast<int>('\\'));
		if (tszSlash)
		{
			*++tszSlash = '\0';
		}
		wsprintf(szCmd,"regedit.exe \"%sxvidc.reg\"",szFilePath);

		WinExec(szCmd,SW_SHOW);
		OnOK();

		/*if (MessageBox("安装完成，是否立即重启系统？","提示",MB_YESNO|MB_ICONQUESTION) == IDYES)
		{
			ExitWindowsEx(EWX_REBOOT,EWX_FORCE);
		}*/

	}
	//OnOK();

}
void COSCConfigDlg::OnUpdateConfig()
{
	char basePath[_MAX_PATH] = {0};
	char cfgFilePath[_MAX_PATH] = {0};
	char dbCfgFilePath[_MAX_PATH] = {0};
	char dbtype[10] = {0};
	::GetModuleFileName(NULL, basePath, _MAX_PATH);
	LPTSTR tszSlash = _tcsrchr(basePath, static_cast<int>('\\'));
	if (tszSlash)
	{
		*++tszSlash = '\0';
	}
	sprintf(cfgFilePath, "%scfg.ini", basePath);
	sprintf(dbCfgFilePath, "%sConfig\\DBConfigure.xml",basePath);
	bool exist = _access(dbCfgFilePath, 4) == 0;
	if (exist)
	{
		TiXmlDocument doc(dbCfgFilePath);
		bool loadOk = doc.LoadFile();
		if (loadOk)
		{
			TiXmlElement *root = doc.RootElement();
			TiXmlNode *type = root->FirstChild("dbtype");
			strcpy(dbtype, type->ToElement()->GetText());
		}
	}
	sprintf(dbCfgFilePath, "%sConfig\\%s.cfg.xml",basePath, dbtype);
	exist = _access(dbCfgFilePath, 4) == 0;
	if (exist)
	{
		TiXmlDocument doc(dbCfgFilePath);
		bool loadOk = doc.LoadFile();
		if (loadOk)
		{
			TiXmlElement *root = doc.RootElement();
			TiXmlNode *factory = root->FirstChild("session-factory");
			TiXmlNode *child = factory->FirstChild("property");
			TiXmlNode *dbInfo = factory->IterateChildren(child);
			char info[200] = {0};
			strcpy(info, dbInfo->ToElement()->GetText());

			std::string str = info;
			std::vector<std::string> result;
			result = split(str, ";");
			for(size_t i = 0; i < result.size(); i++)
			{
				std::string element = result[i];
				if (element.find("Database=") != std::string::npos || element.find("database=") != std::string::npos)
				{
					element = result[i].substr(9);
					m_strOscDbname = element.c_str();
				}
				else if (element.find("Data Source=") != std::string::npos || element.find("server=") != std::string::npos)
				{
					if (element.find("Data Source=") != std::string::npos)
					{
						element = result[i].substr(12);
						m_strOscip = element.c_str();
					}
					else
					{
						element = result[i].substr(7);
						m_strOscip = element.c_str();
					}
				}
				else if (element.find("Port=") != std::string::npos)
				{
					element = result[i].substr(5);
					m_strOscPort = element.c_str();
				}
				else if (element.find("User Id=") != std::string::npos || element.find("uid=") != std::string::npos)
				{
					if (element.find("User Id=") != std::string::npos)
					{
						element = result[i].substr(8);
						m_strOscUser = element.c_str();
					}
					else
					{
						element = result[i].substr(4);
						m_strOscUser = element.c_str();
					}
				}
				else if (element.find("Password=") != std::string::npos || element.find("pwd=") != std::string::npos)
				{
					if (element.find("Password=") != std::string::npos)
					{
						element = result[i].substr(9);
						m_strOscPwd = element.c_str();
					}
					else
					{
						element = result[i].substr(4);
						m_strOscPwd = element.c_str();
					}

				}
				else
				{

				}
			}
		}
	}

	sprintf(dbCfgFilePath, "%sConfig\\%sAudit.cfg.xml",basePath, dbtype);
	exist = _access(dbCfgFilePath, 4) == 0;
	if (exist)
	{
		TiXmlDocument doc(dbCfgFilePath);
		bool loadOk = doc.LoadFile();
		if (loadOk)
		{
			TiXmlElement *root = doc.RootElement();
			TiXmlNode *factory = root->FirstChild("session-factory");
			TiXmlNode *child = factory->FirstChild("property");
			TiXmlNode *dbInfo = factory->IterateChildren(child);
			char info[200] = {0};
			strcpy(info, dbInfo->ToElement()->GetText());

			std::string str = info;
			std::vector<std::string> result;
			result = split(str, ";");
			for(size_t i = 0; i < result.size(); i++)
			{
				std::string element = result[i];
				if (element.find("Database=") != std::string::npos || element.find("database=") != std::string::npos)
				{
					element = result[i].substr(9);
					m_strAuditDbname = element.c_str();
				}
				else if (element.find("Data Source=") != std::string::npos || element.find("server=") != std::string::npos)
				{
					if (element.find("Data Source=") != std::string::npos)
					{
						element = result[i].substr(12);
						m_strAuditIp = element.c_str();
					}
					else
					{
						element = result[i].substr(7);
						m_strAuditIp = element.c_str();
					}
				}
				else if (element.find("Port=") != std::string::npos)
				{
					element = result[i].substr(5);
					m_strAuditPort = element.c_str();
				}
				else if (element.find("User Id=") != std::string::npos || element.find("uid=") != std::string::npos)
				{
					if (element.find("User Id=") != std::string::npos)
					{
						element = result[i].substr(8);
						m_strAuditUser = element.c_str();
					}
					else
					{
						element = result[i].substr(4);
						m_strAuditUser = element.c_str();
					}
				}
				else if (element.find("Password=") != std::string::npos || element.find("pwd=") != std::string::npos)
				{
					if (element.find("Password=") != std::string::npos)
					{
						element = result[i].substr(9);
						m_strAuditPwd = element.c_str();
					}
					else
					{
						element = result[i].substr(4);
						m_strAuditPwd = element.c_str();
					}

				}
				else
				{

				}
			}
		}
	}
	char param[1024] = {0};
	/*GetPrivateProfileString(audit,audit_ip,"",param,1024,cfgFilePath);
	m_strAuditIp = param;
	GetPrivateProfileString(audit,audit_user,"",param,1024,cfgFilePath);
	m_strAuditUser = param;
	GetPrivateProfileString(audit,audit_pwd,"",param,1024,cfgFilePath);
	m_strAuditPwd = param;
	GetPrivateProfileString(audit,audit_port,"",param,1024,cfgFilePath);
	m_strAuditPort = param;
	GetPrivateProfileString(audit,audit_dbname,"",param,1024,cfgFilePath);
	m_strAuditDbname = param;
	GetPrivateProfileString(audit,audit_dbtype,"",param,1024,cfgFilePath);
	m_strAuditDbtype = param;

	GetPrivateProfileString(osc,osc_ip,"",param,1024,cfgFilePath);
	m_strOscip = param;
	GetPrivateProfileString(osc,osc_user,"",param,1024,cfgFilePath);
	m_strOscUser = param;
	GetPrivateProfileString(osc,osc_pwd,"",param,1024,cfgFilePath);
	m_strOscPwd = param;
	GetPrivateProfileString(osc,osc_port,"",param,1024,cfgFilePath);
	m_strOscPort = param;
	GetPrivateProfileString(osc,osc_dbname,"",param,1024,cfgFilePath);
	m_strOscDbname = param;
	GetPrivateProfileString(osc,osc_dbtype,"",param,1024,cfgFilePath);
	m_strOscDbtype = param;*/
	m_strAuditDbtype = "MySql";
	m_strOscDbtype = "MySql";

	GetPrivateProfileString(node,node_code,"",param,1024,cfgFilePath);
	m_strNodeCode = param;
	GetPrivateProfileString(node,node_level,"",param,1024,cfgFilePath);
	m_strNodeLevel = param;
	GetPrivateProfileString(node,node_monitorPort,"",param,1024,cfgFilePath);
	m_strNodeMonitorPort = param;
	GetPrivateProfileString(node,node_videoPath,"",param,1024,cfgFilePath);
	m_strNodeVideoPath = param;
	GetPrivateProfileString(node,node_warningTimes_1,"",param,1024,cfgFilePath);
	m_strNodeWarningTime1 = param;
	GetPrivateProfileString(node,node_warningTimes_2,"",param,1024,cfgFilePath);
	strNodeWarningTime2 = param;
	GetPrivateProfileString(node,node_warningTimes_3,"",param,1024,cfgFilePath);
	strNodeWarningTime3 = param;

	GetPrivateProfileString(WMI,WMI_licenseport,"",param,1024,cfgFilePath);
	m_strLicport = param;
	GetPrivateProfileString(WMI,WMI_wmiport,"",param,1024,cfgFilePath);
	m_strWMIPort = param;
	GetPrivateProfileString(WMI,WMI_tinterval,"",param,1024,cfgFilePath);
	m_strWMITinterval = param;
	GetPrivateProfileString(WMI,WMI_dPrinter,"",param,1024,cfgFilePath);
	m_strWMIDPrinter = param;

	GetPrivateProfileString(NFS,NFS_cmd,"",param,1024,cfgFilePath);
	m_strNFSCmd = param;

	
	UpdateData(FALSE);
}

//字符串分割函数
std::vector<std::string> COSCConfigDlg::split(std::string str,std::string pattern) 
{ 
	std::string::size_type pos; 
	std::vector<std::string> result; 
	str+=pattern;//扩展字符串以方便操作 
	int size=str.size(); 

	for(int i=0; i<size; i++) 
	{ 
		pos=str.find(pattern,i); 
		if(pos<size) 
		{ 
			std::string s=str.substr(i,pos-i); 
			result.push_back(s); 
			i=pos+pattern.size()-1; 
		} 
	} 
	return result; 
} 