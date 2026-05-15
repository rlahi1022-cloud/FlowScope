# FlowScope 통합 테스트 & 벤치마크

FlowScope의 **5개 서버(중앙 게이트웨이 + server1~4)를 실제로 빌드·실행**해서, 클라이언트
입장에서 검증하는 pytest 스위트와 4구조 성능 벤치마크입니다. 서버의 패킷 포맷
(4바이트 빅엔디언 길이 + JSON 바디)을 파이썬 표준 라이브러리만으로 구현했기 때문에
`protobuf` 같은 외부 패키지 없이 동작합니다.

## 핵심 아이디어

FlowScope는 **하나의 프로토콜을 네 가지 서버 구조로 구현**한 프로젝트입니다. 이 스위트는
같은 검증을 5개 서버 모두에 동일하게 돌려서 **"구조가 달라도 동작은 동일하다"**를 코드로
증명하고, 벤치마크로 **각 구조의 성능 트레이드오프를 수치화**합니다.

## 준비물

- Python 3.10+
- `cmake`, `g++` (서버 빌드용) — Linux 전용 (epoll 기반)
- `pip install -r tests/requirements.txt`

## 실행 방법

저장소 루트에서:

```bash
pip install -r tests/requirements.txt
cd tests

# 기능 검증 스위트 (서버 5개 자동 빌드 → 실행 → 검증 → 종료)
python -m pytest

# 4구조 성능 벤치마크 (수치 비교표 출력 + .md / .csv 저장)
python benchmark.py
```

`pytest`는 처음 실행 시 5개 서버를 CMake로 빌드하고 세션 동안 띄운 뒤 자동으로 정리합니다.
5개 포트(9000~9004)가 이미 떠 있으면 그 서버를 그대로 재사용합니다.

### 빌드 위치

기본값은 `tests/_build/` 입니다. 저장소가 CMake가 싫어하는 파일시스템(예: 윈도우 마운트)에
있으면 네이티브 경로로 바꿔 주세요:

```bash
FLOWSCOPE_BUILD_DIR=/tmp/fsbuild python -m pytest
```

## 벤치마크

```bash
python benchmark.py                                  # 100 connections, 5s load
python benchmark.py --connections 50 --duration 3    # 더 짧게
python benchmark.py --servers server1,server4        # 일부만
```

각 서버를 **하나씩 단독으로** 띄워 측정하므로 메모리·CPU 수치가 그 구조만의 값입니다.
측정 지표: 처리량(req/s), p95 지연(ms), 메모리(MB, peak RSS), CPU(%). 결과는 콘솔 표 +
`benchmark_results.md` + `benchmark_results.csv`로 저장됩니다.

## 파일 구성

| 파일 | 내용 |
|------|------|
| `protocol.py` | 패킷 프레이밍 + JSON 인코딩/디코딩 + 5개 서버 레지스트리 |
| `serverctl.py` | 서버 빌드(CMake) + 시작/종료 관리 — pytest와 벤치마크가 공유 |
| `client.py` | `FlowScopeClient` — TCP 접속, 요청/응답 헬퍼 |
| `conftest.py` | 5개 서버 빌드·실행 픽스처, `connect()` 팩토리 |
| `test_equivalence.py` | **4구조 동등성** — 같은 검증을 5개 서버에 동일 적용 (echo/ping/unknown/대용량/동시접속), 그리고 "같은 요청 → 5개 서버 동일 응답" 증명 |
| `test_per_server.py` | 구조별 의도된 차이 (server 태그, traceid, UI 이벤트 계약) |
| `test_central_routing.py` | 중앙 게이트웨이의 target 기반 포워딩 |
| `benchmark.py` | 4구조 성능 벤치마크 → 수치 비교표 |

## 이 스위트가 잡은 실제 버그

이 테스트 스위트를 구성하는 과정에서 **server3(EventBus 구조)가 모든 요청에 무응답**인
버그를 발견했습니다. `server3/main.cpp`의 EventBus "request" 토픽 구독 콜백이
`payload.find(":\"cmd\"")`로 cmd를 탐색했는데, 실제 payload(`fd:traceid:{"cmd":...}`)에는
`:"cmd"` 문자열이 존재하지 않아 핸들러가 한 번도 호출되지 않았습니다. (검색 문자열을
`"\"cmd\""`로 수정 → 정상화.) 테스트가 없었다면 "4개 구조를 비교 구현했다"는 전제 자체가
조용히 깨져 있었을 부분입니다.
