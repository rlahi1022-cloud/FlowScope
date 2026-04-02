// skeletonclient.cpp
// ──────────────────────────────────────────────
// FlowScope MFC 애플리케이션 구현
// Winsock 초기화 + GDI+ 초기화 + 메인 다이얼로그 실행
// ──────────────────────────────────────────────
#include "pch.h"
#include "framework.h"
#include "skeletonclient.h"
#include "skeletonclientDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ──────────────────────────────────────────────
// 전역 앱 객체
// ──────────────────────────────────────────────
CskeletonclientApp theApp;

BEGIN_MESSAGE_MAP(CskeletonclientApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

// ──────────────────────────────────────────────
// 생성자
// ──────────────────────────────────────────────
CskeletonclientApp::CskeletonclientApp()
{
}

// ──────────────────────────────────────────────
// InitInstance
// 1) Winsock 초기화 (TCP 통신용)
// 2) GDI+ 초기화 (순서도 렌더링용)
// 3) 메인 다이얼로그 생성 및 실행
// ──────────────────────────────────────────────
BOOL CskeletonclientApp::InitInstance()
{
	// MFC 공통 컨트롤 초기화
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	// ── Winsock 초기화 ──
	// MFC 소켓 라이브러리 - TCP 연결에 필요
	if (!AfxSocketInit())
	{
		AfxMessageBox(_T("Winsock 초기화 실패"));
		return FALSE;
	}

	AfxEnableControlContainer();

	// ── GDI+ 초기화 ──
	// 순서도(Architecture flow) 렌더링에 사용
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken = 0;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	// ── 메인 다이얼로그 실행 ──
	CskeletonclientDlg dlg;
	m_pMainWnd = &dlg;

	INT_PTR nResponse = dlg.DoModal();

	// ── GDI+ 종료 ──
	Gdiplus::GdiplusShutdown(gdiplusToken);

	// 다이얼로그 종료 → 앱 종료
	return FALSE;
}