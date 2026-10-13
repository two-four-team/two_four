#include "vehicle_control.h"

/* =========================
 * 연속 시간 계산
 *
 * C1 "N초 연속": 조건이 처음 성립한 시각을 기억하고, 경과 시간 = 현재 시각 - 시작 시각.
 * - 입력 간격이 몇 ms든 실제 경과 시간으로 판정한다 (100ms 주기를 전제하지 않는다).
 * - 처음 성립한 시각의 경과 시간은 0ms 이다.
 * - 조건이 깨지면 시작 시각을 TIMER_NOT_STARTED 로 되돌려 처음부터 다시 센다.
 *
 * C1 호출부 계약: 아래 조건이 아니면 호출부가 시작 시각을 TIMER_NOT_STARTED 로 두어야 한다.
 * (이 함수들에는 시동/스위치/센서 상태 인자가 없어 직접 판단할 수 없다.)
 * - Low/HighIlluminanceTime: 시동 ON, 조명 스위치 AUTO, 조도 센서 정상(0.5초 미수신 포함)
 * - WiperActiveTime: ControlWiper() 결과를 넘기면 시동 OFF/수신 중단 시 STOP이므로 자동 충족
 * ========================= */

/* 조건이 성립하면 시작 시각부터의 경과 시간(ms), 아니면 0 */
static unsigned int UpdateElapsedTime(int condition, long *startTime, long now)
{
    if (!condition)
    {
        *startTime = TIMER_NOT_STARTED;
        return 0;
    }

    if (*startTime == TIMER_NOT_STARTED)
    {
        *startTime = now;
    }

    return (unsigned int)(now - *startTime);
}

/* 조도 0 이상 1000 lux 미만 */
unsigned int UpdateLowIlluminanceTime(int illuminance, long *startTime, long now)
{
    return UpdateElapsedTime(illuminance >= 0 && illuminance < 1000, startTime, now);
}

/* 조도 1500 lux 초과 60000 이하 */
unsigned int UpdateHighIlluminanceTime(int illuminance, long *startTime, long now)
{
    return UpdateElapsedTime(illuminance > 1500 && illuminance <= 60000, startTime, now);
}

/* 와이퍼 작동 */
unsigned int UpdateWiperActiveTime(WiperState wiperState, long *startTime, long now)
{
    return UpdateElapsedTime(wiperState != WIPER_STOP, startTime, now);
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
    if (wiperActiveTime >= WIPER_ACTIVE_MS)
    {
        return HEADLIGHT_ON;
    }

    /* 1000 lux 미만이 1초 연속 → ON (L2) */
    if (lowIlluminanceTime >= LOW_ILLUMINANCE_MS)
    {
        return HEADLIGHT_ON;
    }

    /* 1500 lux 초과가 3초 연속 → OFF (L3) */
    if (highIlluminanceTime >= HIGH_ILLUMINANCE_MS)
    {
        return HEADLIGHT_OFF;
    }

    /* 어느 조건도 아니면 기존 상태 유지 (L3) */
    return currentHeadLight;
}
