# FlowScope — epoll-based Event Flow Visualization Server

> 서버 흐름 도식화 — 동일 요청을 4종 서버 구조로 처리해 흐름을 시각적으로 비교

## 프로젝트 개요

**같은 요청도 서버 구조에 따라 다른 흐름이 되는가?**

이 질문에서 출발한 프로젝트이다.
동일한 비즈니스 로직을 **서로 다른 서버 아키텍처 4종**으로 구현하고,
각 구조의 요청 처리 흐름을 실시간으로 시각화하여 비교할 수 있도록 만든
**서버 아키텍처 학습/비교 프로젝트**이다.

단순 기능 구현이 아니라,
**"서버 구조 선택이 처리 흐름·성능·확장성에 어떤 차이를 만드는가"**를
코드 수준에서 직접 증명하는 것이 목표이다.

## 프로젝트 정보

- 개발 인원: 1인 (전체 설계·구현)
- 운영 체제: Linux (Ubuntu)
- 분류: 개인 프로젝트 (포트폴리오)

---

## 기획 동기

이전 팀 프로젝트(3LOUD)에서 서버 구조를 설명할 때
**팀원 간 구조에 대한 이해 속도 차이**가 발생하는 경험을 했다.

> "단순한 설명만으로는 구조를 전달하는 데 한계가 있다.
> 서버 구조는 결국 본인만 아는 영역으로 남는다."

그래서 FlowScope는 단순한 기능 구현이 아니라,
**서버의 처리 흐름과 구조를 시각적으로 표현하는 것**을 목표로 설계했다.

네 가지 구조를 직접 구현해보며,
같은 기능이라도 흐름의 모양에 따라 다른 시스템이 된다는 것을 체감하는 과정이다.

---

## 왜 이 구조인가

프로젝트 목적이 **"아키텍처 비교"**라는 점이 모든 구조 결정의 기준이 되었다.
일반 서비스 개발과는 다른 우선순위로 다음 4가지를 결정했다.

### 1. EventBus 도입 시점 (단위 테스트 완료 전)

* **결정**: 단위 테스트가 완료되기 전에 EventBus 구조를 먼저 도입
* **고려한 대안**: 기능 검증을 끝낸 뒤 안정된 상태에서 EventBus로 전환
* **선택 이유**:
  - 프로젝트 목적이 기능 구현이 아니라 **아키텍처 비교**
  - EventBus 없이 테스트 먼저 진행했다면 나중에 구조 전체를 뒤집어야 했음
  - 뒤집는 비용 > 지금 도입 비용
* **결과**: 4종 구조의 일관된 기준 위에서 구현 진행. 단위 테스트 가시성은 떨어졌으나 구조 일관성을 우선

### 2. 공통 코드 = protocol/constants 만 공유

* **결정**: `protocol.h`, `constants.h`만 4종 서버가 공유. 패킷 구조체는 각 서버/클라이언트가 자체 정의
* **고려한 대안**: 모든 공통 코드를 라이브러리화하여 4종 서버가 동일한 구현을 공유
* **선택 이유**:
  - 이 프로젝트의 핵심 메시지는 **"같은 JSON 패킷을 다른 흐름으로 처리"**
  - 구현까지 공유하면 차이가 흐려짐 → 의미는 공유, 구현은 분리하는 원칙
  - 클라이언트가 서버 내부 구조를 모르고도 동일 요청을 보낼 수 있어야 함
* **결과**: `cmd`/`traceId`/`data` 3필드만 고정. 4종 서버에 같은 요청을 보내고 trace 로그로 흐름 차이를 시각적으로 비교 가능

### 3. 클라이언트 범위 = 송수신 / 응답 출력까지

* **결정**: 클라이언트 완성 범위를 연결·해제, 패킷 송수신, 응답 화면 출력까지로 한정
* **고려한 대안**: 풀 UI 디자인 + 전체 예외 처리 + 사용자 편의 기능 포함
* **선택 이유**:
  - 클라이언트는 서버 흐름을 **눈으로 확인하는 도구** 역할
  - UI 완성도와 클라이언트 예외 처리는 본 프로젝트의 목적 밖
  - 범위를 분명히 하지 않으면 서버 작업이 정체되고 우선순위가 흔들림
* **결과**: 클라이언트 완성도 정의 명확화. 서버 작업에 집중할 수 있는 환경 확보

### 4. Git 브랜치 전략 (혼자 프로젝트에도 분리 운영)

* **결정**: `main` / `dev` / `feat/server1~4` / `feat/client` 로 브랜치 분리
* **고려한 대안**: 단일 `main` 브랜치에서 모든 작업 진행
* **선택 이유**:
  - 4종 서버를 각자 다른 흐름으로 발전시키는 프로젝트 특성상 작업 격리 필요
  - 혼자 하는 프로젝트라도 **커밋 히스토리 자체가 설계 과정의 기록**
  - 서버별 결정 시점과 그 근거가 브랜치 단위로 남으면, 나중에 흐름을 되짚을 수 있음
* **결과**: 서버별 의사결정 흐름이 브랜치에 남아 회고와 비교 분석이 가능

---

## 시스템 구성

```
        ┌─────────────────────────┐
        │  MFC Client (Windows)   │  서버 흐름 시각화 UI
        └────────────┬────────────┘
                     │ TCP/JSON
                     ▼
        ┌─────────────────────────┐
        │  Central Server :9000   │  요청 라우팅 / 트레이스 수집
        └────┬────┬────┬────┬─────┘
             │    │    │    │
             ▼    ▼    ▼    ▼
          :9001 :9002 :9003 :9004
        ┌──────┬──────┬──────┬──────┐
        │ Srv1 │ Srv2 │ Srv3 │ Srv4 │
        │ Fwd  │epoll │Event │Hybrid│
        └──────┴──────┴──────┴──────┘
```

### 구성 요소

- **Central Server (port 9000)**
  - 클라이언트 요청을 받아 4개 서버 중 선택된 구조로 분기
  - 각 서버의 처리 흐름(trace)을 수집하여 클라이언트로 전달

- **Server 1 — Forward (port 9001)**
  - 요청 → Handler → Service → Response (직선 흐름)
  - 동기 처리 / 단일 흐름 기반 구조

- **Server 2 — epoll 기반 (port 9002)**
  - epoll → 이벤트 발생 → 처리 → 반복
  - non-blocking I/O / 이벤트 루프 구조

- **Server 3 — EventBus 기반 (port 9003)**
  - Dispatcher → Router → Handler → Service → EventBus → Subscriber
  - 책임 분리 / Pub-Sub 기반 구조

- **Server 4 — Hybrid (port 9004)**
  - epoll → Dispatcher → Handler → EventBus → JobQueue → WorkerPool
  - 비동기 + 병렬 / 대규모 서비스 대응 구조

- **MFC Client**
  - 동일한 JSON 요청을 4개 서버에 보내고
  - 각 서버의 처리 흐름을 순서도/타임라인 형태로 시각화

---

## 4종 서버 구조 비교

각 구조는 **이전 구조의 한계를 해결하며** 점진적으로 발전한다.

| 구분 | Server1 (Forward) | Server2 (epoll) | Server3 (EventBus) | Server4 (Hybrid) |
|------|------|------|------|------|
| **구조 형태** | 직선 처리 | 이벤트 기반 | 계층 구조 | 분산 / 비동기 |
| **처리 방식** | 동기 | 비동기 (이벤트) | 동기 (구조화) | 비동기 + 병렬 |
| **동시성** | 낮음 | 높음 | 중간 | 매우 높음 |
| **확장성** | 낮음 | 중간 | 중간 | 높음 |
| **결합도** | 높음 | 중간 | 낮음 | 매우 낮음 |
| **디버깅** | 쉬움 | 어려움 | 쉬움 | 어려움 |
| **적합 도메인** | 단순 API, 관리자 툴 | 실시간 채팅, 알림 | 업무 시스템, 웹 백엔드 | 대규모 서비스, 커머스 |

### 같은 요청 → 다른 흐름

```
요청: {"cmd": "ECHO", "traceId": "abc-123", "data": "hello"}

[Server1] Client → Handler → Service → Response
[Server2] epoll_wait → 이벤트 분기 → 처리 → epoll 복귀
[Server3] Dispatcher → Router → Handler → Service → EventBus → Subscriber
[Server4] epoll → Dispatcher → Handler → EventBus → JobQueue → Worker → Response
```

동일한 echo 요청을 4개 서버에 보내면,
**처리 단계·스레드 모델·지연 특성이 완전히 다른 흐름**으로 나타난다.

---

## 핵심 설계 특징

### 1. 공통 인터페이스로 4종 서버 비교 가능

서버 4종이 **같은 JSON 패킷을 받아 다른 흐름으로 처리**하는 것을 보여주기 위해,
프로토콜과 패킷 포맷을 공통화했다.

- `protocol.h`, `constants.h`는 4종 서버 공유
- 패킷 구조체는 서버/클라 각자 정의 (의미 공유, 구현 분리 원칙)
- JSON 포맷 고정: `cmd`, `traceId`, `data` 세 필드

이를 통해 클라이언트는 **서버 내부 구조 차이를 모르고도** 동일한 요청을 보낼 수 있다.

### 2. trace 기반 흐름 시각화

각 서버는 요청을 처리하면서 단계별 trace 로그를 남긴다.

```
[traceId=abc-123]
  ├─ 09:12:01.001 [epoll] event received
  ├─ 09:12:01.002 [dispatcher] dispatched
  ├─ 09:12:01.003 [router] routed → EchoHandler
  ├─ 09:12:01.004 [handler] processing
  ├─ 09:12:01.005 [service] business logic
  └─ 09:12:01.006 [response] sent
```

이 trace는 Central Server를 통해 클라이언트로 전달되어
**MFC UI에서 단계별 진행 상태와 지연시간**을 확인할 수 있다.

### 3. 빅엔디안 헤더 + JSON 본문 프레이밍

TCP는 스트림 기반이므로 메시지 경계를 명시해야 한다.

```
[4 byte big-endian length] + [JSON body]
```

길이 헤더 4바이트 뒤에 실제 JSON 본문이 따라오는 구조로,
서버는 헤더를 먼저 읽고 본문 길이만큼 누적 수신한다.

### 4. ET 모드 epoll + non-blocking I/O

Server 2~4는 **Edge-Triggered epoll**을 사용한다.

- ET 모드는 이벤트가 한 번만 발생하므로 **non-blocking 반복 read** 필수
- 불필요한 이벤트 발생을 줄여 고성능 처리 구조 확보
- 단, 데이터가 남아있어도 추가 이벤트가 안 오는 특성을 고려한 처리 필요

---

## 통신 프로토콜

### 요청 포맷

```json
{
  "cmd": "ECHO",
  "traceId": "abc-123",
  "data": "hello"
}
```

### 응답 포맷

```json
{
  "cmd": "ECHO_RES",
  "traceId": "abc-123",
  "result": "ok",
  "trace": [
    {"step": "dispatcher", "ts": 1709000000.001},
    {"step": "handler",    "ts": 1709000000.002},
    {"step": "service",    "ts": 1709000000.003},
    {"step": "response",   "ts": 1709000000.004}
  ]
}
```

### 패킷 프레이밍

```
+----------------+--------------------+
| length (4 BE)  | JSON body (UTF-8)  |
+----------------+--------------------+
```

---

## 디렉터리 구조

```
flowScope/
├── common/         # 4종 서버 공유 헤더
│   ├── protocol.h  # 프로토콜 상수 정의 (cmd 코드 등)
│   └── constants.h # 공통 상수
├── server/         # Central Server (9000) — 요청 라우팅 / trace 수집
├── server1/        # Forward 구조 (9001) — 직선 동기 처리
│   ├── infra/      # 소켓·연결 관리
│   ├── router/     # 요청 분기
│   └── service/    # 비즈니스 로직
├── server2/        # epoll 기반 (9002) — non-blocking 이벤트 루프
├── server3/        # EventBus (9003) — Dispatcher → Router → Handler → Service → EventBus
├── server4/        # Hybrid (9004) — epoll + EventBus + JobQueue + WorkerPool
└── client/         # MFC 시각화 클라이언트 (Windows)
```

상세 구조는 각 서버 폴더의 README 참고.

---

## 실행 방법

### 사전 요구사항

- **서버**: Linux (Ubuntu) + CMake + C++17 지원 컴파일러
- **클라이언트**: Windows + Visual Studio (MFC 포함)
- **포트**: 9000 ~ 9004 (서버 5개 동시 사용)

### 서버 빌드 (Linux)

```bash
cd flowScope
mkdir build && cd build
cmake ..
cmake --build .
```

### 서버 실행

각 서버를 독립 프로세스로 실행한다. 5개 모두 띄워야 trace 비교가 가능하다.

```bash
./server  &    # Central Server (9000) — 진입점
./server1 &    # Forward         (9001)
./server2 &    # epoll           (9002)
./server3 &    # EventBus        (9003)
./server4 &    # Hybrid          (9004)
```

### 서버 종료

```bash
pkill -f server   # 모든 서버 프로세스 종료
```

### 클라이언트 실행 (Windows)

Visual Studio에서 `client/FlowScope_Client.sln` 열고 빌드 후 실행.
실행 후 Central Server(`<server-ip>:9000`)에 연결하면 4종 서버 흐름을 비교할 수 있다.

---

## 기술 스택

- **Language**: C++17
- **Server OS**: Linux (Ubuntu)
- **Client**: Windows / MFC
- **I/O Model**: epoll (Edge-Triggered, non-blocking)
- **Communication**: TCP + JSON
- **Build**: CMake
- **Version Control**: Git + GitHub
  - Branch: `main` / `dev` / `feat/server1~4` / `feat/client`

---

## 한계

본 프로젝트는 **구조 비교·학습** 목적이므로,
운영 서버 수준의 예외 처리와 안정성 확보는 의도적으로 후순위로 두었다.

### 현재 미구현

- JSON 형식 검증 (스키마 단계 검증 없음)
- 클라이언트별 수신 타임아웃
- 동시 접속 수 제한
- 헤더만 도착하고 본문이 안 오는 경우의 버퍼 누적 정리

### 구조적 한계

- EventBus 도입 시점이 단위 테스트 완료 전이었음.
  구조 일관성을 위해 유지했으나, 테스트 가시성은 떨어졌다.
- 4종 서버 모두 단일 머신에서 포트만 분리하여 실행.
  실제 분산 환경(다중 노드)에서의 비교는 범위 밖.
- 시각화는 trace 기반 후처리.

---

## 회고

가장 어려웠던 결정은 **EventBus를 단위 테스트 완료 전에 도입할 것인가**였다.
당시에는 기능 검증을 먼저 해야 한다는 의문이 있었지만,
프로젝트의 목적이 **아키텍처 비교**임을 다시 확인한 뒤
"나중에 구조를 뒤집는 비용 > 지금 도입 비용" 으로 결정했다.

같은 echo 요청을 Forward → epoll → EventBus → Hybrid로 구현해보며,
각 구조가 이전 구조의 한계를 해결하기 위한 **필연적 선택**임을 trace 로그로 직접 확인할 수 있었고,
서버 개발의 본질이 기능 구현이 아니라 **흐름 설계**라는 것을 체감한 프로젝트였다.

---

> 이 프로젝트는 단순한 결과물이 아니라,
> 초기에 제시되었던 **"서버에 대해 스스로 고민해보라"**는 질문에 대한 답이다.
> 구현을 통해 구조를 이해하고,
> 그 구조를 다시 설명할 수 있는 수준까지 도달하는 것이 최종 목표이다.
