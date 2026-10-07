#include <stdbool.h>
#include <stdint.h>

/* ==================== 와이퍼 상태 ==================== */

typedef enum {
    WIPER_SWITCH_OFF,
    WIPER_SWITCH_AUTO,
    WIPER_SWITCH_LOW,
    WIPER_SWITCH_HIGH
} WiperSwitch;

typedef enum {
    WIPER_STOP,
    WIPER_INTERMITTENT,
    WIPER_LOW,
    WIPER_HIGH
} WiperState;

typedef enum {
    SENSOR_NORMAL,
    SENSOR_FAULT
} SensorState;

/* ==================== 입력 구조체 ==================== */

typedef struct {
    bool engineOn;
    WiperSwitch wiperSwitch;
    int rainAmount;
    int speed;

    bool vehicleStateValid;
    SensorState rainSensorFault;
} WiperInput;

/* ==================== 출력 구조체 ==================== */

typedef struct {
    WiperState wiper;
    SensorState rainSensorFault;
} WiperOutput;

/* ==================== 와이퍼 제어 ==================== */

WiperOutput Wiper_Control(WiperInput input)
{
    WiperOutput output = {
        WIPER_STOP,
        input.rainSensorFault
    };
  /* C2: 강우량 범위 검사 */
if (input.rainAmount < 0 || input.rainAmount > 100) {
    output.rainSensorFault = SENSOR_FAULT;
}
else {
    output.rainSensorFault = input.rainSensorFault;
}

    /* W1 + C4: 시동 OFF 또는 차량 상태 수신 중단 */
    if (!input.engineOn || !input.vehicleStateValid) {
        return output;
    }

    /* W4: 수동 스위치 */
    switch (input.wiperSwitch) {
        case WIPER_SWITCH_OFF:
            output.wiper = WIPER_STOP;
            return output;

        case WIPER_SWITCH_LOW:
            output.wiper = WIPER_LOW;
            return output;

        case WIPER_SWITCH_HIGH:
            output.wiper = WIPER_HIGH;
            return output;

        case WIPER_SWITCH_AUTO:
            break;

        default:
            return output;
    }



/* W5: 강우 센서 고장 */
if (output.rainSensorFault == SENSOR_FAULT) {
    output.wiper = WIPER_INTERMITTENT;
    return output;
}

    /* W2: 강우량에 따른 와이퍼 동작 */
    if (input.rainAmount < 10) {
        output.wiper = WIPER_STOP;
    }
    else if (input.rainAmount < 40) {
        output.wiper = WIPER_INTERMITTENT;
    }
    else if (input.rainAmount < 70) {
        output.wiper = WIPER_LOW;
    }
    else {
        output.wiper = WIPER_HIGH;
    }

    /* W3: 유효한 차량 속도가 0이면 한 단계 감소 */
    if (input.speed == 0 && input.vehicleStateValid) {
        if (output.wiper > WIPER_STOP) {
            output.wiper--;
        }
    }

    return output;
}
