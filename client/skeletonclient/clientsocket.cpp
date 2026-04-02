// ClientSocket.cpp
// ──────────────────────────────────────────────
// TCP 클라이언트 소켓 구현
// 비동기 수신 + 4바이트 길이 헤더 기반 패킷 조립
// ──────────────────────────────────────────────
#include "pch.h"
#include "ClientSocket.h"

#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

// ──────────────────────────────────────────────
// 생성자 / 소멸자
// ──────────────────────────────────────────────
CClientSocket::CClientSocket()
	: m_packetCb(nullptr)
	, m_connectCb(nullptr)
	, m_disconnectCb(nullptr)
{
}

CClientSocket::~CClientSocket()
{
	if (m_hSocket != INVALID_SOCKET)
	{
		Close();
	}
}

// ──────────────────────────────────────────────
// SendPacket
// JSON → [4바이트 Big-Endian 길이][JSON 페이로드] 전송
// ──────────────────────────────────────────────
bool CClientSocket::SendPacket(const std::string& jsonPayload)
{
	// 페이로드 길이를 Big-Endian 4바이트로 변환
	uint32_t length = static_cast<uint32_t>(jsonPayload.size());
	uint32_t netLength = htonl(length);

	// 전송 버퍼: [4바이트 헤더] + [JSON]
	std::vector<char> sendBuf(4 + length);
	memcpy(sendBuf.data(), &netLength, 4);
	memcpy(sendBuf.data() + 4, jsonPayload.c_str(), length);

	// 전체 전송 (부분 전송 처리)
	int totalSent = 0;
	int remaining = static_cast<int>(sendBuf.size());

	while (remaining > 0)
	{
		int sent = Send(sendBuf.data() + totalSent, remaining);

		if (sent == SOCKET_ERROR)
		{
			int err = GetLastError();
			if (err == WSAEWOULDBLOCK)
			{
				// 비동기 소켓 - 잠시 후 재시도
				Sleep(1);
				continue;
			}
			return false;
		}

		totalSent += sent;
		remaining -= sent;
	}

	return true;
}

// ──────────────────────────────────────────────
// OnConnect
// 비동기 Connect 완료 시 호출
// ──────────────────────────────────────────────
void CClientSocket::OnConnect(int nErrorCode)
{
	if (m_connectCb)
	{
		m_connectCb(nErrorCode == 0);
	}
	CAsyncSocket::OnConnect(nErrorCode);
}

// ──────────────────────────────────────────────
// OnReceive
// 데이터 수신 → 버퍼 누적 → 패킷 조립
// ──────────────────────────────────────────────
void CClientSocket::OnReceive(int nErrorCode)
{
	if (nErrorCode != 0)
	{
		CAsyncSocket::OnReceive(nErrorCode);
		return;
	}

	char tempBuf[4096];
	int received = Receive(tempBuf, sizeof(tempBuf));

	if (received > 0)
	{
		// 수신 데이터를 누적 버퍼에 추가
		m_recvBuffer.insert(m_recvBuffer.end(), tempBuf, tempBuf + received);
		// 완성된 패킷 추출
		ProcessBuffer();
	}
	else if (received == 0)
	{
		// 서버가 연결 종료
		if (m_disconnectCb)
		{
			m_disconnectCb();
		}
	}

	CAsyncSocket::OnReceive(nErrorCode);
}

// ──────────────────────────────────────────────
// OnClose
// 서버 연결 종료
// ──────────────────────────────────────────────
void CClientSocket::OnClose(int nErrorCode)
{
	if (m_disconnectCb)
	{
		m_disconnectCb();
	}
	CAsyncSocket::OnClose(nErrorCode);
}

// ──────────────────────────────────────────────
// ProcessBuffer
// 수신 버퍼에서 완성된 패킷 추출
//
// 패킷 구조:
//   [4바이트 Big-Endian 길이] [JSON 페이로드]
//
// TCP 스트림 특성상 패킷이 분할될 수 있으므로
// 충분한 데이터가 쌓일 때까지 대기
// ──────────────────────────────────────────────
void CClientSocket::ProcessBuffer()
{
	while (true)
	{
		// 최소 4바이트(길이 헤더)가 필요
		if (m_recvBuffer.size() < 4)
			break;

		// 길이 헤더 읽기 (Big-Endian → Host)
		uint32_t netLength = 0;
		memcpy(&netLength, m_recvBuffer.data(), 4);
		uint32_t payloadLength = ntohl(netLength);

		// 안전 검사: 10MB 초과 패킷 방지
		if (payloadLength > 10 * 1024 * 1024)
		{
			m_recvBuffer.clear();
			break;
		}

		// 헤더 + 페이로드 전체 도착 확인
		if (m_recvBuffer.size() < 4 + payloadLength)
			break;

		// JSON 페이로드 추출
		std::string jsonPacket(
			m_recvBuffer.data() + 4,
			m_recvBuffer.data() + 4 + payloadLength
		);

		// 처리한 데이터 제거
		m_recvBuffer.erase(
			m_recvBuffer.begin(),
			m_recvBuffer.begin() + 4 + payloadLength
		);

		// 콜백으로 전달
		if (m_packetCb)
		{
			m_packetCb(jsonPacket);
		}
	}
}