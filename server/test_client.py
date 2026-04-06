# ------------------------------------------------
# test_client.py
# FlowScope 서버 송수신 테스트용 Python 클라이언트
#
# 역할:
#   - 서버에 TCP 연결
#   - 4byte 빅엔디언 헤더 + JSON body 패킷 전송
#   - 서버 응답 수신 및 출력
#
# 사용법:
#   python3 test_client.py
# ------------------------------------------------

import socket
import struct
import json
import time

# ------------------------------------------------
# 서버 접속 정보
# ------------------------------------------------
HOST = "127.0.0.1"
PORT = 9000

# ------------------------------------------------
# build_packet
# JSON dict → 4byte 빅엔디언 헤더 + JSON body 바이트열 생성
# ------------------------------------------------
def build_packet(data: dict) -> bytes:
    body = json.dumps(data).encode("utf-8")
    header = struct.pack(">I", len(body))  # 빅엔디언 uint32
    return header + body

# ------------------------------------------------
# send_only
# 응답 없이 패킷만 전송 (unknown 등 응답 없는 cmd용)
# ------------------------------------------------
def send_only(sock: socket.socket, data: dict):
    packet = build_packet(data)
    sock.sendall(packet)
    print(f"[송신] {data}")
    print(f"[응답 없음 - 서버가 unknown cmd는 응답을 보내지 않음]")
    print()

# ------------------------------------------------
# send_and_recv
# 패킷 전송 후 응답 수신
# ------------------------------------------------
def send_and_recv(sock: socket.socket, data: dict):
    packet = build_packet(data)
    sock.sendall(packet)
    print(f"[송신] {data}")

    # 응답 헤더 4바이트 수신
    raw_header = b""
    while len(raw_header) < 4:
        chunk = sock.recv(4 - len(raw_header))
        if not chunk:
            print("[연결 종료]")
            return
        raw_header += chunk

    bodylen = struct.unpack(">I", raw_header)[0]

    # 응답 바디 수신
    raw_body = b""
    while len(raw_body) < bodylen:
        chunk = sock.recv(bodylen - len(raw_body))
        if not chunk:
            print("[연결 종료]")
            return
        raw_body += chunk

    response = json.loads(raw_body.decode("utf-8"))
    print(f"[수신] {response}")
    print()

# ------------------------------------------------
# main
# echo, ping, unknown_cmd 순서로 테스트 패킷 전송
# unknown_cmd는 서버가 응답을 보내지 않으므로 send_only 사용
# ------------------------------------------------
def main():
    print(f"[FlowScope 테스트 클라이언트]")
    print(f"서버 접속: {HOST}:{PORT}")
    print()

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect((HOST, PORT))
        print("[연결 성공]")
        print()

        # echo 테스트 (응답 있음)
        send_and_recv(sock, {"cmd": "echo", "data": "hello flowscope"})
        time.sleep(0.3)

        # ping 테스트 (응답 있음)
        send_and_recv(sock, {"cmd": "ping"})
        time.sleep(0.3)

        # unknown 테스트 (응답 없음 - send_only 사용)
        send_only(sock, {"cmd": "unknown_cmd"})
        time.sleep(0.3)

        print("[테스트 완료]")

if __name__ == "__main__":
    main()