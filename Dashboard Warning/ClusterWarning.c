#include "vehicle_control.h"

/* =========================
 * 계기판 경고 (ClusterWarning)
 *
 * 이 파일은 계기판에 표시할 경고의 우선순위 결정(D5)만 담당한다.
 * 센서 고장이나 도어 상태는 직접 판단하지 않고, 이미 판단된 결과를 입력받는다.
 *
 * 입력의 출처
 * - doorOpenWarningState : Door.c의 CheckDoorOpenWhileDriving() 결과 (D4)
 * - lightSensorState     : sensor.c의 CheckLightSensorFault() 결과 + 0.5초 미수신 (C2)
 * - rainSensorState      : sensor.c의 CheckRainSensorFault() 결과 + 0.5초 미수신 (C2)
 *   (두 센서 상태는 Common/main.c SimSensorState()에서 합성된다)
 *
 * 이 파일에서 하지 않는 것
 * - 센서값 범위 검사 (sensor.c 담당)
 * - 0.5초 미수신 판정 (Common/main.c SimRxState / SimSensorState 담당)
 * - 도어 상태 / 속도 검사 (Door.c 담당)
 * - C3·C4 처리 (수신 중단이면 main.c가 무효 속도(-1)를 넘겨 D4 경고가 꺼짐)
 *
 * 입력만으로 결과가 정해지는 순수 함수이며 내부 상태(static 변수)를 갖지 않는다.
 * 따라서 C5 초기 상태(경고 없음)는 호출부가 초기 입력
 * (DOOR_OPEN_WARNING_INACTIVE, SENSOR_NORMAL, SENSOR_NORMAL)을 넘기면
 * WARNING_NONE이 반환되는 것으로 만족한다.
 * ========================= */


/* =========================
 * [D5] 표시할 경고 결정
 * 경고는 하나만 표시하며, 우선순위는 다음과 같다.
 *   1. 주행 중 도어 열림   → WARNING_DOOR_OPEN
 *   2. 조도 센서 고장      → WARNING_LIGHT_SENSOR_FAULT
 *   3. 강우 센서 고장      → WARNING_RAIN_SENSOR_FAULT
 *   4. 이상 없음           → WARNING_NONE (C5 초기 상태: 경고 없음)
 * ========================= */
ClusterWarning DetermineClusterWarning(DoorOpenWarningState doorOpenWarningState,
                                       SensorState lightSensorState,
                                       SensorState rainSensorState)
{
    if (doorOpenWarningState == DOOR_OPEN_WARNING_ACTIVE)
    {
        return WARNING_DOOR_OPEN;
    }

    if (lightSensorState == SENSOR_FAULT)
    {
        return WARNING_LIGHT_SENSOR_FAULT;
    }

    if (rainSensorState == SENSOR_FAULT)
    {
        return WARNING_RAIN_SENSOR_FAULT;
    }

    return WARNING_NONE;
}
