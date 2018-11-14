// OSCConfigDlg.h : 头文件
//
#include <vector>
#include <string>
#pragma once


// COSCConfigDlg 对话框
class COSCConfigDlg : public CDialog
{
// 构造
public:
	COSCConfigDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_OSCCONFIG_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CString m_strAuditIp;
	CString m_strAuditUser;
	CString m_strAuditPwd;
	CString m_strAuditPort;
	CString m_strAuditDbname;
	CString m_strAuditDbtype;
	CString m_strOscip;
	CString m_strOscUser;
	CString m_strOscPwd;
	CString m_strOscPort;
	CString m_strOscDbname;
	CString m_strOscDbtype;
	CString m_strNodeCode;
	CString m_strNodeLevel;
	CString m_strNodeMonitorPort;
	CString m_strNodeVideoPath;
	CString m_strNodeWarningTime1;
	CString strNodeWarningTime2;
	CString strNodeWarningTime3;
	CString m_strWMIPort;
	CString m_strWMITinterval;
	CString m_strWMIDPrinter;
	CString m_strNFSCmd;
	afx_msg void OnBnClickedSave();
	void OnUpdateConfig();
	std::vector<std::string> split(std::string str,std::string pattern);
	int isinstall;
	CString m_strLicport;
};
