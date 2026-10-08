#include "vehicle_control.h"
/* =========================================================
 * 와이퍼 제어
 * 우선순위 : W1 > W4 > W5 > W2/W3
 * ========================================================= */

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

    /*
     * W1 + C4
     *
     * 실제 시동이 OFF이거나,
     * CheckVehicleStateRx()에서 차량 상태 수신 중단으로
     * 판단된 경우 와이퍼를 정지한다.
     */
    if (engineState == ENGINE_OFF ||
        vehicleRxState == RX_TIMEOUT) {

        return WIPER_STOP;
    }


    /*
     * W4
     * 수동 스위치는 AUTO 제어보다 우선한다.
     */
    switch (wiperSwitch) {

        case WIPER_SWITCH_OFF:
            return WIPER_STOP;

        case WIPER_SWITCH_LOW:
            return WIPER_LOW;

        case WIPER_SWITCH_HIGH:
            return WIPER_HIGH;

        case WIPER_SWITCH_AUTO:
            /* AUTO인 경우 아래 W5, W2, W3 수행 */
            break;

        default:
            /* 정의되지 않은 스위치 값에 대한 방어 처리 */
            return WIPER_STOP;
    }
    /*
     * W5
     * AUTO 상태에서 강우 센서 고장이면 간헐 동작.
     *
     * W5가 W3보다 우선하므로
     * speed == 0이어도 STOP으로 낮추지 않는다.
     */
    if (rainSensorState == SENSOR_FAULT) {
        return WIPER_INTERMITTENT;
    }
    /*
     * W2
     * AUTO + 정상 강우 센서 상태에서
     * 강우량에 따라 기본 와이퍼 단계를 결정한다.
     */
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
    /*
     * W3
     * 유효한 속도가 0이면
     * W2 결과를 한 단계 낮춘다.
     *
     * 유효 속도 범위 : 0 ~ 250
     *
     * WiperState enum이
     * STOP(0) < INTERMITTENT(1) < LOW(2) < HIGH(3)
     * 순서로 정의되어 있다는 전제에 의존한다.
     *
     * 범위를 벗어난 속도는 W3 조건으로 사용하지 않는다.
     */
    if (speed >= 0 && speed <= 250) {

        if (speed == 0 && wiper > WIPER_STOP) {
            wiper = (WiperState)(wiper - 1);
        }
    }

    return wiper;
}
