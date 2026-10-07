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
    RxState vehicleRxState,
    EngineState engineState,
    WiperSwitch wiperSwitch,
    SensorState rainSensorState,
    int rainAmount,
    int speed
)
{
    WiperState wiper = WIPER_STOP;

    /* W1 + C4:
     * 시동 OFF 또는 차량 상태 수신 중단이면 와이퍼 정지 */
    if (engineState == ENGINE_OFF ||
        vehicleRxState == RX_TIMEOUT) {
        return WIPER_STOP;
    }

    /* W4: 수동 스위치 제어 */
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

    /* W5:
     * AUTO에서 강우 센서 고장이면 간헐 동작.
     * W5가 W3보다 우선하므로 정차 중이어도 단계 감소하지 않음. */
    if (rainSensorState == SENSOR_FAULT) {
        return WIPER_INTERMITTENT;
    }

    /* W2: 강우량에 따른 AUTO 와이퍼 단계 결정 */
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

    /* W3:
     * 유효한 속도가 0일 때 W2 결과를 한 단계 감소.
     * 유효 속도 범위는 0 ~ 250.
     *
     * WiperState enum이
     * STOP < INTERMITTENT < LOW < HIGH
     * 순서로 정의되어 있다는 전제에 의존함.
     *
     * 속도가 유효 범위를 벗어난 경우에는
     * W3를 적용하지 않고 W2 결과를 유지함.
     */
    if ((speed >= 0 && speed <= 250) &&
        speed == 0 &&
        wiper > WIPER_STOP) {

        wiper = (WiperState)(wiper - 1);
    }

    return wiper;
}