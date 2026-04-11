#pragma once

// ------------------------------------------------
// dispatch4.h
// server4 디스패처 (동기/비동기/이벤트 분기)
//
// 책임:
//   - cmd를 분석해 처리 경로 결정 (sync / async / event)
//   - 각 경로별로 다른 처리 수행
//
// 처리 경로:
//   sync (echo, ping, debug_status, ui_*)
//     → 즉시 핸들러 호출
//   async (flow_*)
//     → jobqueue4 push
//   event (ai_keyword)
//     → eventbus4 publish
// ------------------------------------------------

#include <string>
#include "../../common/protocol.h"

// ------------------------------------------------
// dispatch_type
// 요청의 처리 경로 유형
// ------------------------------------------------
enum class dispatch_type {
    sync,       // 즉시 처리 (echo, ping, debug_status, ui_*)
    async,      // JobQueue → Worker (flow_*)
    event,      // EventBus (ai_keyword)
    unknown     // 분기 불가
};

// ------------------------------------------------
// dispatch4
// 프로토콜 → 처리 경로 변환
// ------------------------------------------------
class dispatch4 {
public:
    // ------------------------------------------------
    // determine
    // proto를 분석해 처리 경로 결정
    // ------------------------------------------------
    static dispatch_type determine(internal_protocol proto);

    // ------------------------------------------------
    // is_sync
    // sync 경로인지 확인 (echo, ping, debug_status, ui_*)
    // ------------------------------------------------
    static bool is_sync(internal_protocol proto);

    // ------------------------------------------------
    // is_async
    // async 경로인지 확인 (flow_*)
    // ------------------------------------------------
    static bool is_async(internal_protocol proto);

    // ------------------------------------------------
    // is_event
    // event 경로인지 확인 (ai_keyword)
    // ------------------------------------------------
    static bool is_event(internal_protocol proto);
};
