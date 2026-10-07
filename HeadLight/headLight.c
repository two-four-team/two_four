#include "vehicle_control.h"

/* =========================
 * 시간 카운터
 * ========================= */
static int lowLightTime = 0;
static int highLightTime = 0;
static int wiperActiveTime = 0;

/* 현재 헤드라이트 상태 */
static HeadLightState headLightState = HEADLIGHT_OFF;


HeadLightState GetHeadLightState(EngineState engineState,LightSwitch lightSwitch,int illuminance,WiperState wiper)
{
    /* =========================
     * 1. 시동 OFF
     * ========================= */
    if (engineState == ENGINE_OFF)
    {
        lowLightTime = 0;
        highLightTime = 0;
        wiperActiveTime = 0;

        headLightState = HEADLIGHT_OFF;

        return headLightState;
    }


    /* =========================
     * 2. 조명 스위치 OFF
     * ========================= */
    if (lightSwitch == LIGHT_OFF)
    {
        lowLightTime = 0;
        highLightTime = 0;
        wiperActiveTime = 0;

        headLightState = HEADLIGHT_OFF;

        return headLightState;
    }


    /* =========================
     * 3. 조명 스위치 ON
     * ========================= */
    if (lightSwitch == LIGHT_ON)
    {
        lowLightTime = 0;
        highLightTime = 0;
        wiperActiveTime = 0;

        headLightState = HEADLIGHT_ON;

        return headLightState;
    }


    /* =========================
     * 4. 조명 스위치 AUTO
     * ========================= */

    /* -------------------------
     * 조도 조건
     * ------------------------- */

    /* 1000 lux 미만이 1초 연속 → ON */
    if (illuminance < 1000)
    {
        lowLightTime++;
        highLightTime = 0;

        if (lowLightTime >= 1)
        {
            headLightState = HEADLIGHT_ON;
        }
    }

    /* 1500 lux 초과가 3초 연속 → OFF */
    else if (illuminance > 1500)
    {
        highLightTime++;
        lowLightTime = 0;

        if (highLightTime >= 3)
        {
            headLightState = HEADLIGHT_OFF;
        }
    }

    /* 1000 ~ 1500 lux → 기존 상태 유지 */
    else
    {
        lowLightTime = 0;
        highLightTime = 0;
    }


    /* -------------------------
     * 와이퍼 조건
     * ------------------------- */

    /* 와이퍼가 정지 상태가 아니면 */
    if (wiper != WIPER_STOP)
    {
        wiperActiveTime++;

        /* 10초 연속 작동 → ON */
        if (wiperActiveTime >= 10)
        {
            headLightState = HEADLIGHT_ON;
        }
    }
    else
    {
        /* 와이퍼 정지 → 연속 작동 시간 초기화 */
        wiperActiveTime = 0;
    }


    return headLightState;
}