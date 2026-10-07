#include "vehicle_control.h"

HeadLightState GetHeadLightState(EngineState engineState, LightSwitch lightSwitch, int illuminance, WiperState wiper)
{
    static int darkTime = 0;
    static int brightTime = 0;
    static int wiperTime = 0;

    HeadLightState state = HEADLIGHT_OFF;

    /* =========================
     * 1. 시동 OFF
     * ========================= */
    if (engineState == ENGINE_OFF)
    {
        darkTime = 0;
        brightTime = 0;
        wiperTime = 0;

        return HEADLIGHT_OFF;
    }

    /* =========================
     * 2. 조명 스위치 OFF
     * ========================= */
    if (lightSwitch == LIGHT_OFF)
    {
        darkTime = 0;
        brightTime = 0;
        wiperTime = 0;

        return HEADLIGHT_OFF;
    }

    /* =========================
     * 3. 조명 스위치 ON
     * ========================= */
    if (lightSwitch == LIGHT_ON)
    {
        darkTime = 0;
        brightTime = 0;
        wiperTime = 0;

        return HEADLIGHT_ON;
    }

    /* =========================
     * 4. 조명 스위치 AUTO
     * ========================= */

    /* 와이퍼가 작동 중인지 확인 */
    if (wiper != WIPER_STOP)
    {
        wiperTime++;

        /* 와이퍼 10초 연속 작동 */
        if (wiperTime >= 10)
        {
            state = HEADLIGHT_ON;
        }
    }
    else
    {
        wiperTime = 0;
    }

    /* -------------------------
     * 조도 조건
     * ------------------------- */

    /* 어두움: 1000 lux 미만이 1초 연속 */
    if (illuminance < 1000)
    {
        darkTime++;
        brightTime = 0;

        if (darkTime >= 1)
        {
            state = HEADLIGHT_ON;
        }
    }

    /* 밝음: 1500 lux 초과가 3초 연속 */
    else if (illuminance > 1500)
    {
        brightTime++;
        darkTime = 0;

        if (brightTime >= 3)
        {
            state = HEADLIGHT_OFF;
        }
    }

    /* 1000 ~ 1500 lux는 기존 상태 유지 */

    return state;
}