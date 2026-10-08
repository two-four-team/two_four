#include "vehicle_control.h"

/* =========================
 * [D4] 주행 중 도어 열림 판단
 * - 유효한 속도(C3)가 0보다 크고 도어 4개(FL, FR, RL, RR) 중
 *   하나라도 열려 있으면 DOOR_OPEN_WARNING_ACTIVE
 * - 그 외에는 DOOR_OPEN_WARNING_INACTIVE
 *
 * 유효한 속도(C3) 중 이 함수에서 판단하는 것은 속도 범위(0 ~ 250 km/h)뿐이다.
 * "마지막 수신 후 0.5초 이내" 조건은 인자에 RxState가 없어 판단할 수 없으므로
 * 호출부에서 처리해야 한다.
 * (CheckVehicleStateRx() 결과가 RX_TIMEOUT이면 무효 속도(예: -1)를 넘기거나
 *  이 함수를 호출하지 않고 DOOR_OPEN_WARNING_INACTIVE로 처리한다.)
 * ========================= */
DoorOpenWarningState CheckDoorOpenWhileDriving(int speed,
                                               DoorState doorFL,
                                               DoorState doorFR,
                                               DoorState doorRL,
                                               DoorState doorRR)
{
    if (speed <= 0 || speed > 250)
    {
        return DOOR_OPEN_WARNING_INACTIVE;
    }

    if (doorFL == DOOR_OPEN ||
        doorFR == DOOR_OPEN ||
        doorRL == DOOR_OPEN ||
        doorRR == DOOR_OPEN)
    {
        return DOOR_OPEN_WARNING_ACTIVE;
    }

    return DOOR_OPEN_WARNING_INACTIVE;
}


DoorLockState ControlDoorLock(
    DoorLockState currentDoorLock,
    EngineState engineState,
    Gear gear,
    int speed,
    RxState vehicleRxState
)
{
    /* D1 차량 상태 수신 중단이면 그대로 둔다 */
    if (vehicleRxState == RX_TIMEOUT)
    {
        return currentDoorLock;
    }

    /* D2 시동 OFF 또는 기어 P 이면 푼다 */
    if (engineState == ENGINE_OFF || gear == GEAR_P)
    {
        return DOOR_UNLOCKED;
    }

    /* D3 유효한 속도(0~250)가 15 km/h 이상이면 잠근다 */
    if (speed >= 15 && speed <= 250)
    {
        return DOOR_LOCKED;
    }

    return currentDoorLock;
}