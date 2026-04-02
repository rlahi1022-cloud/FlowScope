// skeletonclientDlg.cpp
#include "pch.h"
#include "framework.h"
#include "skeletonclient.h"
#include "skeletonclientDlg.h"
#include "afxdialogex.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ============================================
// Colors
// ============================================
namespace Colors
{
	// flow box background
	const COLORREF GRAY_BG = RGB(0xF2, 0xF3, 0xF4);
	const COLORREF TEAL_BG = RGB(0xD1, 0xF2, 0xEB);
	const COLORREF PURPLE_BG = RGB(0xE8, 0xDA, 0xEF);
	const COLORREF CORAL_BG = RGB(0xFA, 0xDB, 0xD8);
	const COLORREF AMBER_BG = RGB(0xFE, 0xF9, 0xE7);
	const COLORREF PINK_BG = RGB(0xFC, 0xE4, 0xEC);

	// flow box text
	const COLORREF GRAY_TEXT = RGB(0x5D, 0x6D, 0x7E);
	const COLORREF TEAL_TEXT = RGB(0x0E, 0x66, 0x55);
	const COLORREF PURPLE_TEXT = RGB(0x6C, 0x34, 0x83);
	const COLORREF CORAL_TEXT = RGB(0xA9, 0x32, 0x26);
	const COLORREF AMBER_TEXT = RGB(0xB9, 0x77, 0x0E);
	const COLORREF PINK_TEXT = RGB(0xC2, 0x18, 0x5B);

	// active step
	const COLORREF ACTIVE_GREEN = RGB(0x1D, 0x9E, 0x75);
	const COLORREF ARROW_COLOR = RGB(0x5D, 0xCA, 0xA5);

	// server buttons (different from flow box colors)
	const COLORREF BTN_NORMAL_BG = RGB(0xE8, 0xE8, 0xE8);
	const COLORREF BTN_NORMAL_TEXT = RGB(0x55, 0x55, 0x55);
	const COLORREF BTN_ACTIVE_BG = RGB(0x2E, 0x86, 0xC1);
	const COLORREF BTN_ACTIVE_TEXT = RGB(0xFF, 0xFF, 0xFF);
}

static const int DEFAULT_PORTS[SERVER_COUNT] = { 9001, 9002, 9003, 9004 };

#define TIMER_FLOW_REPAINT 1001

// ============================================
// AboutBox
// ============================================
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg() : CDialogEx(IDD_ABOUTBOX) {}
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif
protected:
	virtual void DoDataExchange(CDataExchange* pDX) override { CDialogEx::DoDataExchange(pDX); }
	DECLARE_MESSAGE_MAP()
};
BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// ============================================
// Message map
// ============================================
BEGIN_MESSAGE_MAP(CskeletonclientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BTN_CONNECT, &CskeletonclientDlg::OnBtnConnect)
	ON_BN_CLICKED(IDC_BTN_DISCONNECT, &CskeletonclientDlg::OnBtnDisconnect)
	ON_BN_CLICKED(IDC_BTN_SEND, &CskeletonclientDlg::OnBtnSend)
	ON_BN_CLICKED(IDC_BTN_CLEAR, &CskeletonclientDlg::OnBtnClear)
	ON_BN_CLICKED(IDC_BTN_SERVER1, &CskeletonclientDlg::OnBtnServer1)
	ON_BN_CLICKED(IDC_BTN_SERVER2, &CskeletonclientDlg::OnBtnServer2)
	ON_BN_CLICKED(IDC_BTN_SERVER3, &CskeletonclientDlg::OnBtnServer3)
	ON_BN_CLICKED(IDC_BTN_SERVER4, &CskeletonclientDlg::OnBtnServer4)
	ON_CBN_SELCHANGE(IDC_COMBO_SERVER, &CskeletonclientDlg::OnComboServerChange)
END_MESSAGE_MAP()

// ============================================
// Constructor
// ============================================
CskeletonclientDlg::CskeletonclientDlg(CWnd* pParent)
	: CDialogEx(IDD_SKELETONCLIENT_DIALOG, pParent)
	, m_hIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME))
	, m_pSocket(nullptr)
	, m_bConnected(false)
	, m_currentServer(SERVER_EPOLL)
	, m_nCurrentStep(-1)
{
}

// ============================================
// DDX
// ============================================
void CskeletonclientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_SERVER, m_comboServer);
	DDX_Control(pDX, IDC_EDIT_IP, m_editIP);
	DDX_Control(pDX, IDC_EDIT_PORT, m_editPort);
	DDX_Control(pDX, IDC_STATIC_STATUS, m_staticStatus);
	DDX_Control(pDX, IDC_STATIC_FLOW, m_staticFlow);
	DDX_Control(pDX, IDC_LIST_CHAT, m_listChat);
	DDX_Control(pDX, IDC_EDIT_MSG, m_editMsg);
	DDX_Control(pDX, IDC_LIST_LOG, m_listLog);
}

// ============================================
// OnInitDialog
// ============================================
BOOL CskeletonclientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// about menu
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);
	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		CString strAboutMenu;
		BOOL bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);
	SetWindowText(_T("FlowScope Client"));

	// -- combo init --
	m_comboServer.AddString(_T("1. Request-Response"));
	m_comboServer.AddString(_T("2. epoll event loop"));
	m_comboServer.AddString(_T("3. EventBus pub/sub"));
	m_comboServer.AddString(_T("4. Hybrid"));
	m_comboServer.SetCurSel(1);

	m_editIP.SetWindowText(_T("127.0.0.1"));
	SetDefaultPort();
	m_staticStatus.SetWindowText(_T("Disconnected"));

	// -- create server buttons dynamically (inside flow area) --
	CString btnLabels[SERVER_COUNT] = {
		_T("1.Req-Res"), _T("2.epoll"), _T("3.EventBus"), _T("4.Hybrid")
	};
	UINT btnIDs[SERVER_COUNT] = {
		IDC_BTN_SERVER1, IDC_BTN_SERVER2, IDC_BTN_SERVER3, IDC_BTN_SERVER4
	};

	for (int i = 0; i < SERVER_COUNT; i++)
	{
		m_btnServer[i].Create(btnLabels[i],
			WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
			CRect(0, 0, 80, 22), this, btnIDs[i]);
		m_btnServer[i].SetFont(GetFont());
	}

	// -- flow init --
	m_currentServer = SERVER_EPOLL;
	BuildFlowSteps(m_currentServer);
	UpdateServerButtons();

	// -- remove SS_OWNERDRAW from flow static (causes MFC assert) --
	// we draw manually in OnPaint instead
	if (m_staticFlow.GetSafeHwnd())
	{
		LONG style = ::GetWindowLong(m_staticFlow.GetSafeHwnd(), GWL_STYLE);
		style &= ~SS_OWNERDRAW;
		style |= SS_SIMPLE;
		::SetWindowLong(m_staticFlow.GetSafeHwnd(), GWL_STYLE, style);
	}

	// -- button state --
	GetDlgItem(IDC_BTN_DISCONNECT)->EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_SEND)->EnableWindow(FALSE);

	// -- layout --
	CRect rc;
	GetClientRect(&rc);
	LayoutControls(rc.Width(), rc.Height());

	// -- timer for flow repaint (delay until window fully ready) --
	SetTimer(TIMER_FLOW_REPAINT, 300, NULL);

	return TRUE;
}

// ============================================
// OnTimer - repaint flow area
// ============================================
void CskeletonclientDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TIMER_FLOW_REPAINT)
	{
		KillTimer(TIMER_FLOW_REPAINT);
		UpdateFlowDisplay();
	}
	CDialogEx::OnTimer(nIDEvent);
}

// ============================================
// OnSysCommand
// ============================================
void CskeletonclientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

HCURSOR CskeletonclientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// ============================================
// OnDestroy
// ============================================
void CskeletonclientDlg::OnDestroy()
{
	if (m_pSocket)
	{
		m_pSocket->Close();
		delete m_pSocket;
		m_pSocket = nullptr;
	}
	m_bConnected = false;
	CDialogEx::OnDestroy();
}

// ============================================
// Server button clicks
// ============================================
void CskeletonclientDlg::OnBtnServer1() { SelectServer(SERVER_REQUEST_RESPONSE); }
void CskeletonclientDlg::OnBtnServer2() { SelectServer(SERVER_EPOLL); }
void CskeletonclientDlg::OnBtnServer3() { SelectServer(SERVER_EVENTBUS); }
void CskeletonclientDlg::OnBtnServer4() { SelectServer(SERVER_HYBRID); }

void CskeletonclientDlg::SelectServer(ServerType type)
{
	m_currentServer = type;
	m_nCurrentStep = -1;
	m_comboServer.SetCurSel(static_cast<int>(type));
	SetDefaultPort();
	BuildFlowSteps(type);
	UpdateServerButtons();
	UpdateFlowDisplay();
}

void CskeletonclientDlg::UpdateServerButtons()
{
	for (int i = 0; i < SERVER_COUNT; i++)
	{
		if (!m_btnServer[i].GetSafeHwnd()) continue;
		// selected button gets bold look via redraw (MFC basic buttons dont support color easily)
		// we use a simple approach: disable/enable style or font trick
		// For now just invalidate - the visual difference comes from DrawFlowDiagram title
		m_btnServer[i].Invalidate();
	}
}

// ============================================
// Connection
// ============================================
void CskeletonclientDlg::OnBtnConnect()
{
	if (m_bConnected) return;

	CString strIP, strPort;
	m_editIP.GetWindowText(strIP);
	m_editPort.GetWindowText(strPort);

	if (strIP.IsEmpty() || strPort.IsEmpty())
	{
		AfxMessageBox(_T("IP / Port required"));
		return;
	}

	int nPort = _ttoi(strPort);

	if (m_pSocket)
	{
		m_pSocket->Close();
		delete m_pSocket;
		m_pSocket = nullptr;
	}

	m_pSocket = new CClientSocket();
	m_pSocket->SetConnectCallback([this](bool ok) { OnSocketConnected(ok); });
	m_pSocket->SetDisconnectCallback([this]() { OnSocketDisconnected(); });
	m_pSocket->SetPacketCallback([this](const std::string& json) { OnPacketReceived(json); });

	if (!m_pSocket->Create())
	{
		AfxMessageBox(_T("Socket create failed"));
		delete m_pSocket;
		m_pSocket = nullptr;
		return;
	}

	m_staticStatus.SetWindowText(_T("Connecting..."));
	m_pSocket->Connect(strIP, nPort);

	m_currentServer = static_cast<ServerType>(m_comboServer.GetCurSel());
	BuildFlowSteps(m_currentServer);
	UpdateFlowDisplay();
}

void CskeletonclientDlg::OnBtnDisconnect()
{
	if (m_pSocket)
	{
		m_pSocket->Close();
		delete m_pSocket;
		m_pSocket = nullptr;
	}
	m_bConnected = false;
	m_nCurrentStep = -1;
	UpdateStatusDisplay();
	UpdateFlowDisplay();

	GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(TRUE);
	GetDlgItem(IDC_BTN_DISCONNECT)->EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_SEND)->EnableWindow(FALSE);

	AddChatMessage(_T("[System] Disconnected"));
}

void CskeletonclientDlg::OnSocketConnected(bool success)
{
	if (success)
	{
		m_bConnected = true;
		UpdateStatusDisplay();
		GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(FALSE);
		GetDlgItem(IDC_BTN_DISCONNECT)->EnableWindow(TRUE);
		GetDlgItem(IDC_BTN_SEND)->EnableWindow(TRUE);

		CString strPort;
		m_editPort.GetWindowText(strPort);
		CString msg;
		msg.Format(_T("[System] Connected to Server %d (port %s)"),
			m_comboServer.GetCurSel() + 1, (LPCTSTR)strPort);
		AddChatMessage(msg);
	}
	else
	{
		m_bConnected = false;
		m_staticStatus.SetWindowText(_T("Connect Failed"));
		AddChatMessage(_T("[System] Connection failed"));
		if (m_pSocket)
		{
			m_pSocket->Close();
			delete m_pSocket;
			m_pSocket = nullptr;
		}
	}
}

void CskeletonclientDlg::OnSocketDisconnected()
{
	m_bConnected = false;
	m_nCurrentStep = -1;
	UpdateStatusDisplay();
	UpdateFlowDisplay();
	GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(TRUE);
	GetDlgItem(IDC_BTN_DISCONNECT)->EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_SEND)->EnableWindow(FALSE);
	AddChatMessage(_T("[System] Server disconnected"));
}

// ============================================
// Send / Receive
// ============================================
void CskeletonclientDlg::OnBtnSend()
{
	if (!m_bConnected || !m_pSocket) return;

	CString strMsg;
	m_editMsg.GetWindowText(strMsg);
	if (strMsg.IsEmpty()) return;

	CT2A msgA(strMsg, CP_UTF8);
	std::string msgStr(msgA);
	std::string json = "{\"type\":\"CHAT_MSG\",\"body\":{\"message\":\"" + msgStr + "\"}}";

	if (m_pSocket->SendPacket(json))
	{
		CString chatLine;
		chatLine.Format(_T("[Me] %s"), (LPCTSTR)strMsg);
		AddChatMessage(chatLine);
		m_editMsg.SetWindowText(_T(""));
		m_editMsg.SetFocus();
	}
	else
	{
		AddChatMessage(_T("[System] Send failed"));
	}
}

void CskeletonclientDlg::OnPacketReceived(const std::string& json)
{
	CA2T wideJson(json.c_str(), CP_UTF8);
	CString strJson(wideJson);

	CString chatLine;
	chatLine.Format(_T("[Server] %s"), (LPCTSTR)strJson);
	AddChatMessage(chatLine);
	AddLogEntry(strJson);

	// TODO: parse json -> update m_nCurrentStep -> UpdateFlowDisplay()
}

// ============================================
// Combo / Status / Clear
// ============================================
void CskeletonclientDlg::OnComboServerChange()
{
	int sel = m_comboServer.GetCurSel();
	SelectServer(static_cast<ServerType>(sel));
}

void CskeletonclientDlg::SetDefaultPort()
{
	int sel = m_comboServer.GetCurSel();
	if (sel >= 0 && sel < SERVER_COUNT)
	{
		CString port;
		port.Format(_T("%d"), DEFAULT_PORTS[sel]);
		m_editPort.SetWindowText(port);
	}
}

void CskeletonclientDlg::UpdateStatusDisplay()
{
	m_staticStatus.SetWindowText(m_bConnected ? _T("Connected") : _T("Disconnected"));
}

void CskeletonclientDlg::OnBtnClear()
{
	m_listLog.ResetContent();
}

// ============================================
// UI helpers
// ============================================
void CskeletonclientDlg::AddChatMessage(const CString& msg)
{
	m_listChat.AddString(msg);
	// auto-scroll: use raw HWND to bypass MFC assert on LBS_NOSEL
	int count = (int)::SendMessage(m_listChat.GetSafeHwnd(), LB_GETCOUNT, 0, 0);
	if (count > 0)
		::SendMessage(m_listChat.GetSafeHwnd(), LB_SETTOPINDEX, count - 1, 0);
}

void CskeletonclientDlg::AddLogEntry(const CString& log)
{
	m_listLog.AddString(log);
	// auto-scroll: use raw HWND to bypass MFC assert on LBS_NOSEL
	int count = (int)::SendMessage(m_listLog.GetSafeHwnd(), LB_GETCOUNT, 0, 0);
	if (count > 0)
		::SendMessage(m_listLog.GetSafeHwnd(), LB_SETTOPINDEX, count - 1, 0);
}

// ============================================
// Flow steps
// ============================================
void CskeletonclientDlg::BuildFlowSteps(ServerType type)
{
	m_flowSteps.clear();

	FlowStep clientReq = { _T("Client request"), Colors::GRAY_BG, Colors::GRAY_TEXT };

	switch (type)
	{
	case SERVER_REQUEST_RESPONSE:
		m_flowSteps.push_back(clientReq);
		m_flowSteps.push_back({ _T("Accept / Read"),  Colors::TEAL_BG,   Colors::TEAL_TEXT });
		m_flowSteps.push_back({ _T("Dispatcher"),      Colors::PURPLE_BG, Colors::PURPLE_TEXT });
		m_flowSteps.push_back({ _T("Service"),          Colors::CORAL_BG,  Colors::CORAL_TEXT });
		m_flowSteps.push_back({ _T("Write / Send"),    Colors::TEAL_BG,   Colors::TEAL_TEXT });
		break;

	case SERVER_EPOLL:
		m_flowSteps.push_back(clientReq);
		m_flowSteps.push_back({ _T("epoll detect"),    Colors::TEAL_BG,   Colors::TEAL_TEXT });
		m_flowSteps.push_back({ _T("Dispatcher"),      Colors::PURPLE_BG, Colors::PURPLE_TEXT });
		m_flowSteps.push_back({ _T("Service"),          Colors::CORAL_BG,  Colors::CORAL_TEXT });
		m_flowSteps.push_back({ _T("epoll write"),     Colors::TEAL_BG,   Colors::TEAL_TEXT });
		break;

	case SERVER_EVENTBUS:
		m_flowSteps.push_back(clientReq);
		m_flowSteps.push_back({ _T("epoll detect"),    Colors::TEAL_BG,   Colors::TEAL_TEXT });
		m_flowSteps.push_back({ _T("EventBus"),        Colors::AMBER_BG,  Colors::AMBER_TEXT });
		m_flowSteps.push_back({ _T("Service"),          Colors::CORAL_BG,  Colors::CORAL_TEXT });
		m_flowSteps.push_back({ _T("epoll write"),     Colors::TEAL_BG,   Colors::TEAL_TEXT });
		break;

	case SERVER_HYBRID:
		m_flowSteps.push_back(clientReq);
		m_flowSteps.push_back({ _T("epoll detect"),    Colors::TEAL_BG,   Colors::TEAL_TEXT });
		m_flowSteps.push_back({ _T("Dispatcher"),      Colors::PURPLE_BG, Colors::PURPLE_TEXT });
		m_flowSteps.push_back({ _T("EventBus"),        Colors::AMBER_BG,  Colors::AMBER_TEXT });
		m_flowSteps.push_back({ _T("JobQueue"),        Colors::PINK_BG,   Colors::PINK_TEXT });
		m_flowSteps.push_back({ _T("Worker"),           Colors::CORAL_BG,  Colors::CORAL_TEXT });
		m_flowSteps.push_back({ _T("epoll write"),     Colors::TEAL_BG,   Colors::TEAL_TEXT });
		break;
	}
}

void CskeletonclientDlg::UpdateFlowDisplay()
{
	if (m_staticFlow.GetSafeHwnd())
	{
		CClientDC dc(&m_staticFlow);
		CRect rect;
		m_staticFlow.GetClientRect(&rect);
		DrawFlowDiagram(&dc, rect);
	}
}

// ============================================
// OnPaint
// ============================================
void CskeletonclientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();

		// draw flow diagram
		if (m_staticFlow.GetSafeHwnd())
		{
			CClientDC flowDC(&m_staticFlow);
			CRect flowRect;
			m_staticFlow.GetClientRect(&flowRect);
			DrawFlowDiagram(&flowDC, flowRect);
		}
	}
}

// ============================================
// DrawFlowDiagram - GDI+ rendering
// ============================================
void CskeletonclientDlg::DrawFlowDiagram(CDC* pDC, CRect rect)
{
	Gdiplus::Graphics graphics(pDC->GetSafeHdc());
	graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

	// background
	Gdiplus::SolidBrush whiteBrush(Gdiplus::Color(255, 255, 255, 255));
	graphics.FillRectangle(&whiteBrush, 0, 0, rect.Width(), rect.Height());

	if (m_flowSteps.empty()) return;

	int stepCount = static_cast<int>(m_flowSteps.size());
	int boxWidth = 160;
	int boxHeight = 36;
	int arrowGap = 16;
	int startX = (rect.Width() - boxWidth) / 2;
	int cornerRadius = 14;

	Gdiplus::FontFamily fontFamily(L"Arial");
	Gdiplus::Font titleFont(&fontFamily, 12, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
	Gdiplus::Font stepFont(&fontFamily, 11, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);

	// -- server title --
	CString serverNames[] = {
		_T("Request-Response"), _T("epoll event loop"),
		_T("EventBus pub/sub"), _T("Hybrid")
	};

	int startY = 4;
	if (m_currentServer >= 0 && m_currentServer < SERVER_COUNT)
	{
		Gdiplus::SolidBrush titleBrush(Gdiplus::Color(255, 0x1B, 0x4F, 0x72));
		CT2W wTitle(serverNames[m_currentServer]);
		Gdiplus::StringFormat sf;
		sf.SetAlignment(Gdiplus::StringAlignmentCenter);
		Gdiplus::RectF titleRect(0, (Gdiplus::REAL)startY, (Gdiplus::REAL)rect.Width(), 20);
		graphics.DrawString(wTitle, -1, &titleFont, titleRect, &sf, &titleBrush);
		startY = 28;
	}

	// -- draw each step --
	for (int i = 0; i < stepCount; i++)
	{
		int y = startY + i * (boxHeight + arrowGap);
		const FlowStep& step = m_flowSteps[i];

		BYTE bgR = GetRValue(step.bgColor);
		BYTE bgG = GetGValue(step.bgColor);
		BYTE bgB = GetBValue(step.bgColor);
		BYTE txR = GetRValue(step.textColor);
		BYTE txG = GetGValue(step.textColor);
		BYTE txB = GetBValue(step.textColor);

		// rounded rect path
		Gdiplus::GraphicsPath path;
		int x = startX;
		int r = cornerRadius;
		path.AddArc(x, y, r * 2, r * 2, 180, 90);
		path.AddArc(x + boxWidth - r * 2, y, r * 2, r * 2, 270, 90);
		path.AddArc(x + boxWidth - r * 2, y + boxHeight - r * 2, r * 2, r * 2, 0, 90);
		path.AddArc(x, y + boxHeight - r * 2, r * 2, r * 2, 90, 90);
		path.CloseFigure();

		// fill
		Gdiplus::SolidBrush fillBrush(Gdiplus::Color(255, bgR, bgG, bgB));
		graphics.FillPath(&fillBrush, &path);

		// border
		Gdiplus::Pen borderPen(Gdiplus::Color(255, txR, txG, txB), 1.0f);
		graphics.DrawPath(&borderPen, &path);

		// active step highlight
		if (i == m_nCurrentStep)
		{
			Gdiplus::Pen activePen(Gdiplus::Color(255, 0x1D, 0x9E, 0x75), 2.5f);
			graphics.DrawPath(&activePen, &path);

			Gdiplus::SolidBrush greenBrush(Gdiplus::Color(255, 0x1D, 0x9E, 0x75));
			graphics.FillEllipse(&greenBrush, x - 16, y + (boxHeight - 10) / 2, 10, 10);
		}

		// text
		Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, txR, txG, txB));
		CT2W wText(step.name);
		Gdiplus::StringFormat sf;
		sf.SetAlignment(Gdiplus::StringAlignmentCenter);
		sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
		Gdiplus::RectF textRect((Gdiplus::REAL)x, (Gdiplus::REAL)y, (Gdiplus::REAL)boxWidth, (Gdiplus::REAL)boxHeight);
		graphics.DrawString(wText, -1, &stepFont, textRect, &sf, &textBrush);

		// arrow (except last)
		if (i < stepCount - 1)
		{
			int arrowX = startX + boxWidth / 2;
			int arrowY1 = y + boxHeight;
			int arrowY2 = arrowY1 + arrowGap;

			Gdiplus::Pen arrowPen(Gdiplus::Color(255, 0x5D, 0xCA, 0xA5), 1.5f);
			graphics.DrawLine(&arrowPen, arrowX, arrowY1, arrowX, arrowY2 - 4);

			Gdiplus::SolidBrush arrowBrush(Gdiplus::Color(255, 0x5D, 0xCA, 0xA5));
			Gdiplus::Point head[] = {
				Gdiplus::Point(arrowX - 4, arrowY2 - 6),
				Gdiplus::Point(arrowX + 4, arrowY2 - 6),
				Gdiplus::Point(arrowX, arrowY2)
			};
			graphics.FillPolygon(&arrowBrush, head, 3);
		}
	}
}

// ============================================
// Layout
// ============================================
void CskeletonclientDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	LayoutControls(cx, cy);
}

void CskeletonclientDlg::LayoutControls(int cx, int cy)
{
	if (!m_comboServer.GetSafeHwnd()) return;

	int margin = 4;
	int topH = 22;
	int logH = 150;
	int editH = 20;
	int btnH = 20;
	int serverBtnH = 24;

	// -- top connection bar --
	int x = margin;
	int y = margin;

	GetDlgItem(IDC_STATIC_SERVER_LABEL)->MoveWindow(x, y + 2, 30, 14);
	x += 32;
	m_comboServer.MoveWindow(x, y, 150, 200);
	x += 154;

	GetDlgItem(IDC_STATIC_IP_LABEL)->MoveWindow(x, y + 2, 14, 14);
	x += 16;
	m_editIP.MoveWindow(x, y, 110, editH);
	x += 114;

	GetDlgItem(IDC_STATIC_PORT_LABEL)->MoveWindow(x, y + 2, 22, 14);
	x += 24;
	m_editPort.MoveWindow(x, y, 46, editH);
	x += 50;

	GetDlgItem(IDC_BTN_CONNECT)->MoveWindow(x, y, 58, btnH);
	x += 62;
	GetDlgItem(IDC_BTN_DISCONNECT)->MoveWindow(x, y, 68, btnH);
	x += 72;

	m_staticStatus.MoveWindow(x, y + 2, 120, 14);

	// -- middle area --
	int midY = margin + topH + margin + 2;
	int midH = cy - midY - logH - margin * 3;

	// left: flow area (60%)
	int flowW = (int)((cx - margin * 3) * 0.6);

	// server buttons at top of flow area
	int btnW = (flowW - margin * 3) / 4;
	for (int i = 0; i < SERVER_COUNT; i++)
	{
		if (m_btnServer[i].GetSafeHwnd())
		{
			m_btnServer[i].MoveWindow(
				margin + i * (btnW + margin), midY,
				btnW, serverBtnH);
		}
	}

	// flow static below server buttons
	int flowY = midY + serverBtnH + 2;
	int flowH = midH - serverBtnH - 2;
	m_staticFlow.MoveWindow(margin, flowY, flowW, flowH);

	// right: chat
	int chatX = margin + flowW + margin;
	int chatW = cx - chatX - margin;

	GetDlgItem(IDC_STATIC_CHAT_LABEL)->MoveWindow(chatX, midY, chatW, 16);

	int chatListY = midY + 18;
	int chatListH = midH - 18 - editH - margin;
	m_listChat.MoveWindow(chatX, chatListY, chatW, chatListH);

	int msgY = chatListY + chatListH + margin;
	int sendW = 50;
	m_editMsg.MoveWindow(chatX, msgY, chatW - sendW - margin, editH);
	GetDlgItem(IDC_BTN_SEND)->MoveWindow(chatX + chatW - sendW, msgY, sendW, editH);

	// -- bottom: log --
	int logY = cy - logH - margin;

	GetDlgItem(IDC_STATIC_LOG_LABEL)->MoveWindow(margin, logY, 60, 16);
	GetDlgItem(IDC_BTN_CLEAR)->MoveWindow(cx - margin - 46, logY, 46, 16);
	m_listLog.MoveWindow(margin, logY + 18, cx - margin * 2, logH - 20);

	// repaint flow
	UpdateFlowDisplay();
}