#include "vehicle_control.h"
/* =========================================================
 * 센서 / 차량 상태 수신 판단
 *
 * 0.5초 미수신 처리는 main의 수신 관리 로직에서 처리한다.
 * 이 파일의 함수는 전달받은 값(또는 RxState)만으로 판단하며,
 * 수신 시각을 알 수 없으므로 미수신 여부를 직접 판단하지 않는다.
 * ========================================================= */

/*
 * C2: 조도 센서 고장 판단
 * 조도 유효 범위: 0 ~ 60000 lux
 *
 * 유효 범위를 벗어나면 SENSOR_FAULT,
 * 유효 범위이면 SENSOR_NORMAL을 반환한다.
 */
SensorState CheckLightSensorFault(int illuminance)
{
    if (illuminance < 0 || illuminance > 60000) {
        return SENSOR_FAULT;
    }

    return SENSOR_NORMAL;
}

/*
 * C2: 강우 센서 고장 판단
 * 강우량 유효 범위: 0 ~ 100 %
 *
 * 유효 범위를 벗어나면 SENSOR_FAULT,
 * 유효 범위이면 SENSOR_NORMAL을 반환한다.
 */
SensorState CheckRainSensorFault(int rainAmount)
{
    if (rainAmount < 0 || rainAmount > 100) {
        return SENSOR_FAULT;
    }

    return SENSOR_NORMAL;
}

/*
 * C3: 차량 상태(speed, gear, engine) 수신 판단
 *
 * 각 항목의 0.5초 미수신 판정 결과(RxState)를 전달받아
 * 하나라도 RX_TIMEOUT이면 RX_TIMEOUT,
 * 모두 RX_NORMAL이면 RX_NORMAL을 반환한다.
 *
 * C4(수신 중단 시 시동 OFF 간주) fail-safe는 이번 범위에서 제외되어
 * 이 함수에서는 결과만 반환하고 다른 기능에 연결하지 않는다.
 */
RxState CheckVehicleStateRx(RxState speedRxState, RxState gearRxState, RxState engineRxState)
{
    if (speedRxState == RX_TIMEOUT ||
        gearRxState == RX_TIMEOUT ||
        engineRxState == RX_TIMEOUT) {
        return RX_TIMEOUT;
    }

    return RX_NORMAL;
}
