#include "vehicle_control.h"

/* =========================
 * 연속 시간 계산 (100ms 주기 호출)
 * 조건이 맞으면 currentTime + 100, 아니면 0
 * ========================= */

/* 조도 0 이상 1000 lux 미만 */
unsigned int UpdateLowIlluminanceTime(int illuminance, unsigned int currentTime)
{
    if (illuminance >= 0 && illuminance < 1000)
    {
        return currentTime + 100;
    }

    return 0;
}

/* 조도 1500 lux 초과 60000 이하 */
unsigned int UpdateHighIlluminanceTime(int illuminance, unsigned int currentTime)
{
    if (illuminance > 1500 && illuminance <= 60000)
    {
        return currentTime + 100;
    }

    return 0;
}

/* 와이퍼 작동 */
unsigned int UpdateWiperActiveTime(WiperState wiperState, unsigned int currentTime)
{
    if (wiperState != WIPER_STOP)
    {
        return currentTime + 100;
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

    WiperState wiperState = WIPER_STOP;
    int illuminance = 0;

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
