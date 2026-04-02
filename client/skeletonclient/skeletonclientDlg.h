// skeletonclientDlg.h
#pragma once

#include "ClientSocket.h"

// -- flow step info --
struct FlowStep
{
	CString   name;
	COLORREF  bgColor;
	COLORREF  textColor;
};

// -- server type --
enum ServerType
{
	SERVER_REQUEST_RESPONSE = 0,
	SERVER_EPOLL = 1,
	SERVER_EVENTBUS = 2,
	SERVER_HYBRID = 3,
	SERVER_COUNT = 4
};

// -- main dialog --
class CskeletonclientDlg : public CDialogEx
{
public:
	CskeletonclientDlg(CWnd* pParent = nullptr);

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SKELETONCLIENT_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;
	virtual BOOL OnInitDialog() override;

	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnBtnConnect();
	afx_msg void OnBtnDisconnect();
	afx_msg void OnBtnSend();
	afx_msg void OnBtnClear();
	afx_msg void OnComboServerChange();
	afx_msg void OnBtnServer1();
	afx_msg void OnBtnServer2();
	afx_msg void OnBtnServer3();
	afx_msg void OnBtnServer4();
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	DECLARE_MESSAGE_MAP()

private:
	HICON m_hIcon;

	// -- controls --
	CComboBox m_comboServer;
	CEdit     m_editIP;
	CEdit     m_editPort;
	CStatic   m_staticStatus;
	CStatic   m_staticFlow;
	CListBox  m_listChat;
	CEdit     m_editMsg;
	CListBox  m_listLog;
	CButton   m_btnServer[SERVER_COUNT];

	// -- socket --
	CClientSocket* m_pSocket;
	bool           m_bConnected;

	// -- flow --
	ServerType            m_currentServer;
	int                   m_nCurrentStep;
	std::vector<FlowStep> m_flowSteps;

	// -- flow functions --
	void BuildFlowSteps(ServerType type);
	void DrawFlowDiagram(CDC* pDC, CRect rect);
	void UpdateFlowDisplay();
	void SelectServer(ServerType type);
	void UpdateServerButtons();

	// -- socket callbacks --
	void OnSocketConnected(bool success);
	void OnSocketDisconnected();
	void OnPacketReceived(const std::string& json);

	// -- UI helpers --
	void AddChatMessage(const CString& msg);
	void AddLogEntry(const CString& log);
	void UpdateStatusDisplay();
	void SetDefaultPort();
	void LayoutControls(int cx, int cy);
};