#include "vehicle_control.h"

/* ==================== 강우 센서 고장 판단 ==================== */

/* C2: 강우량이 0 ~ 100 % 범위를 벗어나면 센서 고장 */
SensorState CheckRainSensorFault(int rainAmount)
{
    if (rainAmount < 0 || rainAmount > 100) {
        return SENSOR_FAULT;
    }

    return SENSOR_NORMAL;
}

/* ==================== 와이퍼 제어 ==================== */

WiperState ControlWiper(
    EngineState engineState,
    RxState vehicleRxState,
    WiperSwitch wiperSwitch,
    int rainAmount,
    SensorState rainSensorState,
    int speed
)
{
    WiperState wiper;

    /* W1 + C4: 시동 OFF 또는 차량 상태 수신 중단 */
    if (engineState == ENGINE_OFF || vehicleRxState == RX_TIMEOUT) {
        return WIPER_STOP;
    }

    /* W4: 수동 스위치 */
    switch (wiperSwitch) {
        case WIPER_SWITCH_OFF:
            return WIPER_STOP;

        case WIPER_SWITCH_LOW:
            return WIPER_LOW;

        case WIPER_SWITCH_HIGH:
            return WIPER_HIGH;

        case WIPER_SWITCH_AUTO:
            break;

        default:
            return WIPER_STOP;
    }

    /* W5: 강우 센서 고장 시 간헐 동작 */
    if (rainSensorState == SENSOR_FAULT) {
        return WIPER_INTERMITTENT;
    }

    /* W2: 강우량에 따른 와이퍼 동작 */
    if (rainAmount < 10) {
        wiper = WIPER_STOP;
    }
    else if (rainAmount < 40) {
        wiper = WIPER_INTERMITTENT;
    }
    else if (rainAmount < 70) {
        wiper = WIPER_LOW;
    }
    else {
        wiper = WIPER_HIGH;
    }

    /* W3: 정차 중(속도 0)이면 한 단계 감소
     * WiperState 가 STOP < INTERMITTENT < LOW < HIGH 순서임에 의존 */
    if (speed == 0 && wiper > WIPER_STOP) {
        wiper = (WiperState)(wiper - 1);
    }

    return wiper;
}
