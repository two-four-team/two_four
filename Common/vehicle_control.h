/* ==================== 입력 ==================== */

/* 기어 */
typedef enum {
    GEAR_P,
    GEAR_R,
    GEAR_N,
    GEAR_D
} Gear;

/* 시동 */
typedef enum {
    ENGINE_OFF,
    ENGINE_ON
} EngineState;

/* 도어 */
typedef enum {
    DOOR_CLOSED,
    DOOR_OPEN
} DoorState;

/* 조명 스위치 */
typedef enum {
    LIGHT_AUTO,
    LIGHT_OFF,
    LIGHT_ON
} LightSwitch;

/* 와이퍼 스위치 */
typedef enum {
    WIPER_SWITCH_OFF,
    WIPER_SWITCH_AUTO,
    WIPER_SWITCH_LOW,
    WIPER_SWITCH_HIGH
} WiperSwitch;

typedef enum {
    RX_NORMAL,
    RX_TIMEOUT
} RxState;

/* 입력 전역변수 */
int speed = 0;                   // 0 ~ 250 km/h
Gear gear = GEAR_P;
EngineState engineState = ENGINE_OFF;

int illuminance = 0;             // 0 ~ 60000 lux
int rainAmount = 0;              // 0 ~ 100 %

DoorState doorFL = DOOR_CLOSED;
DoorState doorFR = DOOR_CLOSED;
DoorState doorRL = DOOR_CLOSED;
DoorState doorRR = DOOR_CLOSED;

LightSwitch lightSwitch = LIGHT_AUTO;
WiperSwitch wiperSwitch = WIPER_SWITCH_OFF;

RxState speedRxState = RX_NORMAL;
RxState gearRxState = RX_NORMAL;
RxState engineRxState = RX_NORMAL;

/* ==================== 출력 ==================== */

/* 전조등 */
typedef enum {
    HEADLIGHT_OFF,
    HEADLIGHT_ON
} HeadLightState;

/* 센서 상태 */
typedef enum {
    SENSOR_NORMAL,
    SENSOR_FAULT
} SensorState;

/* 와이퍼 동작 */
typedef enum {
    WIPER_STOP,
    WIPER_INTERMITTENT,
    WIPER_LOW,
    WIPER_HIGH
} WiperState;

/* 도어 잠금 */
typedef enum {
    DOOR_UNLOCKED,
    DOOR_LOCKED
} DoorLockState;

/* 주행 중 도어 열림 상태 */
typedef enum {
    DOOR_OPEN_WARNING_INACTIVE,
    DOOR_OPEN_WARNING_ACTIVE
} DoorOpenWarningState;

/* 계기판 경고 */
typedef enum {
    WARNING_NONE,
    WARNING_DOOR_OPEN,
    WARNING_LIGHT_SENSOR_FAULT,
    WARNING_RAIN_SENSOR_FAULT
} ClusterWarning;


/* 출력 전역변수 */
HeadLightState headLight = HEADLIGHT_OFF;

SensorState lightSensorFault = SENSOR_NORMAL;
WiperState wiper = WIPER_STOP;
SensorState rainSensorFault = SENSOR_NORMAL;

DoorLockState doorLock = DOOR_UNLOCKED;

ClusterWarning clusterWarning = WARNING_NONE;

/* ==================== 함수 ==================== */

HeadLightState ControlHeadLight(
    EngineState engineState,
    RxState vehicleRxState,
    LightSwitch lightSwitch,
    int illuminance,
    SensorState lightSensorState,
    WiperState wiper
);

WiperState ControlWiper(
    EngineState engineState,
    RxState vehicleRxState,
    WiperSwitch wiperSwitch,
    int rainAmount,
    SensorState rainSensorState,
    int speed
);

DoorLockState ControlDoorLock(
    DoorLockState currentDoorLock,
    EngineState engineState,
    Gear gear,
    int speed,
    RxState vehicleRxState
);

DoorOpenWarningState CheckDoorOpenWhileDriving(
    int speed,
    DoorState doorFL,
    DoorState doorFR,
    DoorState doorRL,
    DoorState doorRR
);

SensorState CheckLightSensorFault(
    int illuminance
);

SensorState CheckRainSensorFault(
    int rainAmount
);

ClusterWarning CheckClusterWarning(
    DoorOpenWarningState doorOpenWarningState,
    SensorState lightSensorState,
    SensorState rainSensorState
);

RxState CheckVehicleStateRx(
    RxState speedRxState,
    RxState gearRxState,
    RxState engineRxState
);