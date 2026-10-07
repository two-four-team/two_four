#include "vehicle_control.h"
/* =========================================================
 * 강우 센서 상태 판단
 * ========================================================= */
/*
 * C2: 강우 센서 고장 판단
 * 강우량 유효 범위: 0 ~ 100 %
 *
 * 유효 범위를 벗어나면 SENSOR_FAULT,
 * 유효 범위이면 SENSOR_NORMAL을 반환한다.
 *
 * 0.5초 미수신 처리는 main의 수신 관리 로직에서 처리한다.
 */
SensorState CheckRainSensorFault(int rainAmount)
{
    if (rainAmount < 0 || rainAmount > 100) {
        return SENSOR_FAULT;
    }

    return SENSOR_NORMAL;
}