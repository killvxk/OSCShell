// TestIEHelperDlg.h : 头文件
//

#pragma once


// CTestIEHelperDlg 对话框
class CTestIEHelperDlg : public CDialog
{
// 构造
public:
	CTestIEHelperDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_TESTIEHELPER_DIALOG };

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
	CString m_strUsername;
	CString m_strPassword;
	CString m_strParam;
	afx_msg void OnBnClickedBtnok();
	CString m_strUrl;
};
