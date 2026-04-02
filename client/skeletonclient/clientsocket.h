// ClientSocket.h
// ──────────────────────────────────────────────
// TCP 클라이언트 소켓
// CAsyncSocket 기반 비동기 소켓
// 4바이트 Big-Endian 길이 헤더 + JSON 페이로드 프로토콜
// ──────────────────────────────────────────────
#pragma once

#include <afxsock.h>
#include <string>
#include <vector>
#include <functional>

// ──────────────────────────────────────────────
// 콜백 타입 정의
// ──────────────────────────────────────────────
using PacketCallback = std::function<void(const std::string& jsonPacket)>;
using ConnectCallback = std::function<void(bool success)>;
using DisconnectCallback = std::function<void()>;

// ──────────────────────────────────────────────
// CClientSocket
// - CAsyncSocket 파생 (비동기 이벤트 기반)
// - OnReceive에서 데이터 수신 → 패킷 조립
// - 완성된 패킷은 콜백으로 전달
// ──────────────────────────────────────────────
class CClientSocket : public CAsyncSocket
{
public:
	CClientSocket();
	virtual ~CClientSocket();

	// ── 콜백 설정 ──
	void SetPacketCallback(PacketCallback cb) { m_packetCb = cb; }
	void SetConnectCallback(ConnectCallback cb) { m_connectCb = cb; }
	void SetDisconnectCallback(DisconnectCallback cb) { m_disconnectCb = cb; }

	// ── 패킷 전송 ──
	// JSON 문자열을 [4바이트 길이 헤더] + [페이로드]로 전송
	bool SendPacket(const std::string& jsonPayload);

protected:
	// ── CAsyncSocket 가상 함수 오버라이드 ──
	virtual void OnConnect(int nErrorCode) override;
	virtual void OnReceive(int nErrorCode) override;
	virtual void OnClose(int nErrorCode)   override;

private:
	// ── 수신 버퍼 ──
	// TCP 스트림을 누적 후 완성된 패킷만 추출
	std::vector<char> m_recvBuffer;

	// ── 패킷 조립 ──
	void ProcessBuffer();

	// ── 콜백 ──
	PacketCallback     m_packetCb;
	ConnectCallback    m_connectCb;
	DisconnectCallback m_disconnectCb;
};