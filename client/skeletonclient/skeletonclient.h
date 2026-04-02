// skeletonclient.h
// ──────────────────────────────────────────────
// FlowScope MFC 애플리케이션 클래스
// CWinApp 파생 - 프로그램 진입점
// ──────────────────────────────────────────────
#pragma once

#ifndef __AFXWIN_H__
#error "PCH에 대해 이 파일을 포함하기 전에 'pch.h'를 포함합니다."
#endif

#include "resource.h"

// ──────────────────────────────────────────────
// CskeletonclientApp
// ──────────────────────────────────────────────
class CskeletonclientApp : public CWinApp
{
public:
	CskeletonclientApp();

	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};

extern CskeletonclientApp theApp;