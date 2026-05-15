# FlowScope 4-Architecture Benchmark

- connections: **100**
- load: **4s** (warmup 1s)
- generated: 2026-05-15 02:45:13

테스트 통과/실패가 아닌 수치 표로 정리:

| 지표 | Server 1<br/>Thread-per-Connection | Server 2<br/>epoll + direct write | Server 3<br/>EventBus (pub/sub) | Server 4<br/>Hybrid (epoll + dispatcher + eventbus) |
|---|---|---|---|---|
| 처리량 (req/s) @ 100 connections | 10,382 | 14,280 | 11,250 | 10,427 |
| p95 지연 (ms) | 21.16 | 7.80 | 9.47 | 10.98 |
| 메모리 (MB) | 4.9 | 3.9 | 3.9 | 4.0 |
| CPU (%) | 32.2 | 95.2 | 97.0 | 97.7 |

