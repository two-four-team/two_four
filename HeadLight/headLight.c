#include <limits.h>
#include "vehicle_control.h"

/* =========================
 * 연속 시간 계산 (100ms 주기 호출)
 *
 * C1 "N초 연속": 조건이 처음 성립한 주기를 0ms로 보고, 그 뒤 주기마다 100ms씩 센다.
 * 반환값 = 경과 시간(ms) + 1
 * - 0은 "세지 않음(조건 불성립 또는 호출부 리셋)"으로 남겨 두어야 하므로,
 *   처음 성립한 주기에는 경과 0ms를 1로 표시한다.
 * - +1 덕분에 ControlHeadLight의 ">= 1000/3000/10000" 비교가
 *   정확히 1초/3초/10초가 지난 주기에 성립한다 (예: 9100ms 성립 → 10100ms에 1001).
 * - 조건이 깨지면 0을 반환해 처음부터 다시 센다.
 *
 * C1 호출부 계약: 아래 조건이 아니면 호출부가 시간을 0으로 저장해야 한다.
 * (이 함수들에는 시동/스위치/센서 상태 인자가 없어 직접 판단할 수 없다.)
 * - Low/HighIlluminanceTime: 시동 ON, 조명 스위치 AUTO, 조도 센서 정상(0.5초 미수신 포함)
 * - WiperActiveTime: ControlWiper() 결과를 넘기면 시동 OFF/수신 중단 시 STOP이므로 자동 충족
 * ========================= */

#define CYCLE_TIME          100u    /* 호출 주기 (ms) */
#define COUNT_STARTED       1u      /* 처음 성립한 주기: 경과 0ms + 1 */

/* 조건이 성립한 주기의 시간 갱신.
 * 누적 시간이 unsigned 범위를 넘으면 0 근처로 돌아가지 않도록 그대로 유지한다. */
static unsigned int AddTick(unsigned int currentTime)
{
    if (currentTime == 0)
    {
        return COUNT_STARTED;
    }

    if (currentTime > UINT_MAX - CYCLE_TIME)
    {
        return currentTime;
    }

    return currentTime + CYCLE_TIME;
}

/* 조도 0 이상 1000 lux 미만 */
unsigned int UpdateLowIlluminanceTime(int illuminance, unsigned int currentTime)
{
    if (illuminance >= 0 && illuminance < 1000)
    {
        return AddTick(currentTime);
    }

    return 0;
}

/* 조도 1500 lux 초과 60000 이하 */
unsigned int UpdateHighIlluminanceTime(int illuminance, unsigned int currentTime)
{
    if (illuminance > 1500 && illuminance <= 60000)
    {
        return AddTick(currentTime);
    }

    return 0;
}

/* 와이퍼 작동 */
unsigned int UpdateWiperActiveTime(WiperState wiperState, unsigned int currentTime)
{
    if (wiperState != WIPER_STOP)
    {
        return AddTick(currentTime);
    }

    return 0;
}

/* =========================
 * 전조등 제어
 * 우선순위: L1 > L5 > L4 > L6 > L2·L3
 * ========================= */
HeadLightState ControlHeadLight(
    RxState vehicleRxState,
    EngineState engineState,
    SensorState lightSensorState,
    LightSwitch lightSwitch,
    unsigned int wiperActiveTime,
    unsigned int lowIlluminanceTime,
    unsigned int highIlluminanceTime,
    HeadLightState currentHeadLight
)
{
    /* =========================
     * 1. 시동 OFF 또는 차량 상태 수신 중단 (L1, C4)
     * ========================= */
    if (vehicleRxState == RX_TIMEOUT || engineState == ENGINE_OFF)
    {
        return HEADLIGHT_OFF;
    }

    /* =========================
     * 2. 조도 센서 고장 (L5)
     * ========================= */
    if (lightSensorState == SENSOR_FAULT)
    {
        return HEADLIGHT_ON;
    }

    /* =========================
     * 3. 조명 스위치 ON / OFF (L4)
     * ========================= */
    if (lightSwitch == LIGHT_ON)
    {
        return HEADLIGHT_ON;
    }

    if (lightSwitch == LIGHT_OFF)
    {
        return HEADLIGHT_OFF;
    }

    /* =========================
     * 4. 조명 스위치 AUTO
     * ========================= */

    /* 와이퍼 10초 연속 작동 → ON (L6) */
    if (wiperActiveTime >= 10000)
    {
        return HEADLIGHT_ON;
    }

    /* 1000 lux 미만이 1초 연속 → ON (L2) */
    if (lowIlluminanceTime >= 1000)
    {
        return HEADLIGHT_ON;
    }

    /* 1500 lux 초과가 3초 연속 → OFF (L3) */
    if (highIlluminanceTime >= 3000)
    {
        return HEADLIGHT_OFF;
    }

    /* 어느 조건도 아니면 기존 상태 유지 (L3) */
    return currentHeadLight;
}
