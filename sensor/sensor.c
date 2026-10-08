#include "vehicle_control.h"
/* =========================================================
 * 센서 / 차량 상태 수신 판단
 *
 * 0.5초 미수신 처리는 main의 수신 관리 로직에서 처리한다.
 * 이 파일의 함수는 전달받은 값(또는 RxState)만으로 판단하며,
 * 수신 시각을 알 수 없으므로 미수신 여부를 직접 판단하지 않는다.
 *
 * C2 호출부 계약: 조도/강우 센서가 0.5초 미수신(처음 값 전에는 시작 시점부터)이면
 * 호출부가 아래 함수 결과와 합성해 SENSOR_FAULT로 만든다.
 * → Common/main.c SimSensorState(): 미수신이면 SENSOR_FAULT,
 *   아니면 마지막 수신값으로 CheckLightSensorFault / CheckRainSensorFault 호출
 *
 * 수신 시각 판정(SimRxState), 유효 속도(SimValidSpeed)를 이 파일로 옮기려면
 * 새 함수나 인자가 필요해 vehicle_control.h 변경이 필요하므로 main.c에 둔다.
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
 * 각 항목의 0.5초 미수신 판정 결과(RxState, main.c SimRxState())를 전달받아
 * 하나라도 RX_TIMEOUT이면 RX_TIMEOUT,
 * 모두 RX_NORMAL이면 RX_NORMAL을 반환한다.
 *
 * C4: 이 결과를 ControlHeadLight/ControlWiper/ControlDoorLock의
 * vehicleRxState로 넘기면 조명·와이퍼는 시동 OFF로, 도어 잠금은 상태 유지(D1)로 처리된다.
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
