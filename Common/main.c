/* =========================================================
 * 차체 편의 기능 시뮬레이터 (VBC-SRS-001 v1.0)
 *
 * input/input.csv 의 차량 상태(3.1)를 한 줄씩 읽어
 * 각 모듈 함수를 호출하고, 3.2 출력 항목을 output 로그(txt)에 기록한다.
 *
 * 빌드 (프로젝트 루트에서):
 *   gcc -std=c99 -Wall -Wextra -I Common -o vehicle_sim \
 *       Common/main.c HeadLight/headLight.c Wiper/Wiper.c Door/Door.c \
 *       sensor/sensor.c "Dashboard Warning/ClusterWarning.c"
 *
 * 실행:
 *   ./vehicle_sim [입력 csv] [출력 로그]
 *   기본값: input/input.csv, output/simulation_log_YYYYMMDD_HHMMSS.txt (실행 시각)
 *   - 실행할 때마다 새 로그 파일을 만든다. 같은 초에 다시 실행하면 _2, _3 … 을 붙인다.
 *   - 출력 로그 경로를 인자로 주면 그 파일에 (덮어)쓴다.
 *   종료 코드: 0 정상, 1 파일 오류, 2 입력에 건너뛴 행/잘못된 값 있음
 *
 * Windows (Visual Studio 개발자 명령 프롬프트, 프로젝트 루트에서):
 *   cl /nologo /W3 /utf-8 /I Common /Fe:vehicle_sim.exe Common\main.c HeadLight\headLight.c ^
 *      Wiper\Wiper.c Door\Door.c sensor\sensor.c "Dashboard Warning\ClusterWarning.c"
 *   - 이 파일은 UTF-8(BOM 포함)로 저장되어 있어 /utf-8 없이도 한글 문자열이 깨지지 않는다.
 *     편집기에서 다시 저장할 때 BOM 을 지우지 말 것.
 *   - Visual Studio 프로젝트로 빌드할 때는 작업 디렉터리를 프로젝트 루트(two_four)로 설정하거나
 *     입력/출력 경로를 인자로 넘긴다.
 *
 * 입력 CSV 형식:
 *   time_ms,speed,gear,engineState,illuminance,rainAmount,
 *   doorFL,doorFR,doorRL,doorRR,lightSwitch,wiperSwitch
 *   - 빈 칸은 "이번 주기에 값이 들어오지 않음"으로 처리한다 (C2, C4 시험용).
 *   - time_ms 는 증가해야 하며 100ms 간격을 전제로 한다.
 *     간격이 100ms 보다 크면 빠진 주기를 "아무 값도 들어오지 않은 주기"로 채워 처리한다
 *     (로그에 '~' 표시). 100ms 의 배수가 아닌 간격은 경고한다.
 *
 * main 의 역할 (모듈이 맡지 않는 부분):
 *   - 0.5초 미수신 판정 (C2, C4)
 *   - 유효한 속도 판정 (C3) : 수신 중단이거나 범위 밖이면 INVALID_SPEED 전달
 *   - C1 시간 카운트 조건 : L2·L3 은 시동 ON + AUTO + 조도 센서 정상일 때만,
 *                          L6 은 시동 ON 이고 와이퍼가 작동하는 동안만 센다
 *   - 상태 보관 (전조등, 도어 잠금, 각종 연속 시간)
 * ========================================================= */
#ifdef _MSC_VER
/* fopen 등의 C4996 경고 방지. Visual Studio 기본 설정(/sdl)에서는 이 경고가 오류가 된다 */
#define _CRT_SECURE_NO_WARNINGS
#endif

#if defined(_MSC_VER) && !defined(__clang__)
/* 실행 문자 집합을 UTF-8 로 고정 (기본은 시스템 코드 페이지 CP949 라 로그가 CP949 로 저장됨) */
#pragma execution_character_set("utf-8")
#endif

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "vehicle_control.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#define CYCLE_MS            100
#define RX_TIMEOUT_MS       500
/* 헤더(vehicle_control.h)로 옮겨질 경우를 대비해 중복 정의를 피한다 */
#ifndef INVALID_SPEED
#define INVALID_SPEED       (-1)
#endif
#define CSV_LINE_MAX        512
#define FIELD_COUNT         12

/* 입력 없이 이만큼 주기가 지나면 모든 주기 입력이 수신 중단(0.5초)되어 출력이 더 바뀌지 않는다.
 * 그 이후의 빈 구간은 로그에 한 줄로 요약한다 (큰 시간 점프로 로그가 수 GB 가 되는 것 방지). */
#define MAX_FILL_CYCLES     (RX_TIMEOUT_MS / CYCLE_MS + 1)

#define DEFAULT_INPUT_PATH  "input/input.csv"
#define DEFAULT_OUTPUT_DIR  "output"
#define LOG_PATH_MAX        260     /* Windows MAX_PATH */
#define MAX_LOG_SUFFIX      99

/* =========================
 * 입력 문자열 → 값 변환
 * ========================= */
typedef struct {
    const char *name;
    int value;
} EnumName;

static const EnumName GEAR_NAMES[] = {
    { "GEAR_P", GEAR_P }, { "GEAR_R", GEAR_R }, { "GEAR_N", GEAR_N }, { "GEAR_D", GEAR_D },
    { NULL, 0 }
};

static const EnumName ENGINE_NAMES[] = {
    { "ENGINE_OFF", ENGINE_OFF }, { "ENGINE_ON", ENGINE_ON },
    { NULL, 0 }
};

static const EnumName DOOR_NAMES[] = {
    { "DOOR_CLOSED", DOOR_CLOSED }, { "DOOR_OPEN", DOOR_OPEN },
    { NULL, 0 }
};

static const EnumName LIGHT_SWITCH_NAMES[] = {
    { "LIGHT_AUTO", LIGHT_AUTO }, { "LIGHT_OFF", LIGHT_OFF }, { "LIGHT_ON", LIGHT_ON },
    { NULL, 0 }
};

static const EnumName WIPER_SWITCH_NAMES[] = {
    { "WIPER_SWITCH_OFF", WIPER_SWITCH_OFF }, { "WIPER_SWITCH_AUTO", WIPER_SWITCH_AUTO },
    { "WIPER_SWITCH_LOW", WIPER_SWITCH_LOW }, { "WIPER_SWITCH_HIGH", WIPER_SWITCH_HIGH },
    { NULL, 0 }
};

/* 열 순서와 이름. table 이 NULL 이면 정수 열 */
enum {
    COL_TIME, COL_SPEED, COL_GEAR, COL_ENGINE, COL_ILLUMINANCE, COL_RAIN,
    COL_DOOR_FL, COL_DOOR_FR, COL_DOOR_RL, COL_DOOR_RR, COL_LIGHT_SWITCH, COL_WIPER_SWITCH
};

typedef struct {
    const char *name;
    const EnumName *table;
} Column;

static const Column COLUMNS[FIELD_COUNT] = {
    { "time_ms", NULL },           { "speed", NULL },            { "gear", GEAR_NAMES },
    { "engineState", ENGINE_NAMES }, { "illuminance", NULL },    { "rainAmount", NULL },
    { "doorFL", DOOR_NAMES },      { "doorFR", DOOR_NAMES },     { "doorRL", DOOR_NAMES },
    { "doorRR", DOOR_NAMES },      { "lightSwitch", LIGHT_SWITCH_NAMES },
    { "wiperSwitch", WIPER_SWITCH_NAMES }
};

/* 성공 시 1, 알 수 없는 값이면 0 */
static int ParseEnum(const char *text, const EnumName *table, int *out)
{
    int i;

    for (i = 0; table[i].name != NULL; i++)
    {
        if (strcmp(text, table[i].name) == 0)
        {
            *out = table[i].value;
            return 1;
        }
    }

    return 0;
}

/* 성공 시 1, 숫자가 아니면 0.
 * int 범위를 넘는 값은 INT_MIN/INT_MAX 로 포화시켜
 * 형변환 오버플로로 유효 범위 안의 값이 되는 일을 막는다 (범위 밖 → 센서 고장 유지). */
static int ParseInt(const char *text, int *out)
{
    char *end;
    long value;

    errno = 0;
    value = strtol(text, &end, 10);
    if (end == text || *end != '\0')
    {
        return 0;
    }

    if (errno == ERANGE || value > INT_MAX)
    {
        value = INT_MAX;
    }
    else if (value < INT_MIN)
    {
        value = INT_MIN;
    }

    *out = (int)value;
    return 1;
}

/* 앞뒤 공백·개행 제거 */
static char *Trim(char *text)
{
    char *end;

    while (*text == ' ' || *text == '\t')
    {
        text++;
    }

    end = text + strlen(text);
    while (end > text && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n'))
    {
        *--end = '\0';
    }

    return text;
}

/* 쉼표로 분리. 실제 필드 수를 반환 (maxFields 를 넘으면 maxFields + 1) */
static int SplitCsv(char *line, char *fields[], int maxFields)
{
    int count = 0;
    int i;
    char *p = line;

    fields[count++] = p;
    for (; *p != '\0'; p++)
    {
        if (*p != ',')
        {
            continue;
        }

        if (count == maxFields)
        {
            return maxFields + 1;
        }

        *p = '\0';
        fields[count++] = p + 1;
    }

    for (i = 0; i < count; i++)
    {
        fields[i] = Trim(fields[i]);
    }

    return count;
}

/* =========================
 * 한 행의 입력
 * ========================= */
typedef struct {
    long timeMs;
    int present[FIELD_COUNT];   /* 이번 행에 값이 들어왔는지 */
    int value[FIELD_COUNT];
} RowInput;

/* 성공 시 1. 잘못된 값이 있는 칸은 미수신으로 처리하고 *badValues 를 늘린다 */
static int ParseRow(char *line, int lineNo, RowInput *row, int *badValues)
{
    char *fields[FIELD_COUNT];
    int count = SplitCsv(line, fields, FIELD_COUNT);
    int col;

    if (count != FIELD_COUNT)
    {
        fprintf(stderr, "%d행: 필드 수가 %d개여서 건너뜁니다 (필요: %d개)\n",
                lineNo, count, FIELD_COUNT);
        return 0;
    }

    for (col = 0; col < FIELD_COUNT; col++)
    {
        const char *text = fields[col];
        int ok;

        row->present[col] = 0;
        if (text[0] == '\0')
        {
            continue;
        }

        ok = (COLUMNS[col].table == NULL)
             ? ParseInt(text, &row->value[col])
             : ParseEnum(text, COLUMNS[col].table, &row->value[col]);
        if (ok)
        {
            row->present[col] = 1;
        }
        else
        {
            fprintf(stderr, "%d행: %s 값 '%s' 를 해석할 수 없어 미수신으로 처리합니다\n",
                    lineNo, COLUMNS[col].name, text);
            (*badValues)++;
        }
    }

    if (!row->present[COL_TIME])
    {
        fprintf(stderr, "%d행: time_ms 가 없어 건너뜁니다\n", lineNo);
        return 0;
    }

    /* ParseInt 는 범위를 넘는 값을 포화시키므로, 시간은 원문으로 범위를 다시 확인한다 */
    errno = 0;
    row->timeMs = strtol(fields[COL_TIME], NULL, 10);
    if (errno == ERANGE || row->timeMs < 0 || row->timeMs > INT_MAX)
    {
        fprintf(stderr, "%d행: time_ms '%s' 가 0~%d 범위를 벗어나 건너뜁니다\n",
                lineNo, fields[COL_TIME], INT_MAX);
        return 0;
    }

    return 1;
}

/* =========================
 * 주기 입력 수신 관리 (C2, C4)
 * ========================= */
typedef struct {
    int value;
    int everReceived;
    long lastRxTime;    /* 처음 값이 오기 전에는 시작 시점 */
} PeriodicInput;

static void ReceivePeriodic(PeriodicInput *in, const RowInput *row, int col)
{
    if (row->present[col])
    {
        in->value = row->value[col];
        in->everReceived = 1;
        in->lastRxTime = row->timeMs;
    }
}

/* =========================
 * 모듈이 맡지 않는 규칙 (특이사항.md 3번)
 *
 * 각 규칙을 함수 하나로 모아 두었다. 모듈로 옮길 때는 함수 단위로 옮기면 된다.
 * 이름에 Sim 을 붙여 모듈에 추가될 함수(CheckRxTimeout, GetValidSpeed 등)와 겹치지 않게 했다.
 * ========================= */

/* C2·C4 미수신: 마지막 수신(처음에는 시작 시점) 후 0.5초가 지나면 RX_TIMEOUT
 * → 옮길 곳: sensor.c UpdateRxElapsedTime / CheckRxTimeout */
static RxState SimRxState(const PeriodicInput *in, long now)
{
    return (now - in->lastRxTime >= RX_TIMEOUT_MS) ? RX_TIMEOUT : RX_NORMAL;
}

/* C2 센서 고장: 미수신이거나 유효 범위 밖
 * → 옮길 곳: sensor.c CheckLightSensorFault / CheckRainSensorFault 에 RxState 인자 추가 */
static SensorState SimSensorState(const PeriodicInput *in, long now,
                                  SensorState (*checkRange)(int value))
{
    if (SimRxState(in, now) == RX_TIMEOUT)
    {
        return SENSOR_FAULT;
    }

    return checkRange(in->value);
}

/* C3 유효한 속도: 차량 상태 3개 모두 0.5초 이내 수신 + 0~250, 아니면 INVALID_SPEED
 * 한 번도 들어오지 않은 항목은 "마지막으로 들어온" 시각이 없으므로 유효하지 않다.
 * (시작 후 0.5초 안에는 vehicleRx 가 RX_NORMAL 이어도 초기값 속도 0·기어 P 를 쓰지 않는다)
 * → 옮길 곳: sensor.c GetValidSpeed */
static int SimValidSpeed(const PeriodicInput *speed, const PeriodicInput *gear,
                         const PeriodicInput *engine, RxState vehicleRx)
{
    if (vehicleRx == RX_NORMAL &&
        speed->everReceived && gear->everReceived && engine->everReceived &&
        speed->value >= 0 && speed->value <= 250)
    {
        return speed->value;
    }

    return INVALID_SPEED;
}

/* C1 L2·L3 시간 조건: 시동 ON + 스위치 AUTO + 조도 센서 정상일 때만 센다
 * (조도 값을 한 번도 받지 못했으면 셀 값이 없으므로 세지 않는다)
 * → 옮길 곳: headLight.c UpdateLow/HighIlluminanceTime 에 조건 인자 추가 */
static int SimLightTimerEnabled(EngineState engineState, LightSwitch lightSwitch,
                                SensorState lightSensor, int illuminanceReceived)
{
    return engineState == ENGINE_ON &&
           lightSwitch == LIGHT_AUTO &&
           lightSensor == SENSOR_NORMAL &&
           illuminanceReceived;
}

/* =========================
 * 시뮬레이터 상태와 한 주기 처리
 * ========================= */
typedef struct {
    HeadLightState headLight;
    SensorState lightSensor;
    WiperState wiper;
    SensorState rainSensor;
    DoorLockState doorLock;
    ClusterWarning warning;
} Outputs;

typedef struct {
    int started;
    long prevTime;

    /* 주기 입력: 처음 값이 오기 전에는 C5 초기 상태(정지·시동 OFF·P)로 본다 */
    PeriodicInput speed;
    PeriodicInput gear;
    PeriodicInput engine;
    PeriodicInput illuminance;
    PeriodicInput rain;

    /* 이벤트 입력: 빈 칸이면 직전 값 유지 */
    DoorState doors[4];
    LightSwitch lightSwitch;
    WiperSwitch wiperSwitch;

    /* C1 연속 시간 */
    unsigned int lowIlluminanceTime;
    unsigned int highIlluminanceTime;
    unsigned int wiperActiveTime;

    Outputs out;
} Simulator;

static void InitSimulator(Simulator *sim)
{
    memset(sim, 0, sizeof(*sim));
    sim->gear.value = GEAR_P;
    sim->engine.value = ENGINE_OFF;
    sim->doors[0] = sim->doors[1] = sim->doors[2] = sim->doors[3] = DOOR_CLOSED;
    sim->lightSwitch = LIGHT_OFF;
    sim->wiperSwitch = WIPER_SWITCH_OFF;

    /* C5: 모두 꺼짐, 정지, 잠금 해제, 경고 없음 */
    sim->out.headLight = HEADLIGHT_OFF;
    sim->out.lightSensor = SENSOR_NORMAL;
    sim->out.wiper = WIPER_STOP;
    sim->out.rainSensor = SENSOR_NORMAL;
    sim->out.doorLock = DOOR_UNLOCKED;
    sim->out.warning = WARNING_NONE;
}

/* 처음 행이면 시작 시점을 정한다. 시간이 증가하지 않는 행이면 0 (건너뜀) */
static int AcceptTime(Simulator *sim, const RowInput *row, int lineNo)
{
    long delta;

    if (!sim->started)
    {
        /* C2, C4: 처음 값이 들어오기 전에는 시작 시점부터 0.5초를 센다 */
        sim->speed.lastRxTime = row->timeMs;
        sim->gear.lastRxTime = row->timeMs;
        sim->engine.lastRxTime = row->timeMs;
        sim->illuminance.lastRxTime = row->timeMs;
        sim->rain.lastRxTime = row->timeMs;
        sim->started = 1;
        return 1;
    }

    delta = row->timeMs - sim->prevTime;
    if (delta <= 0)
    {
        fprintf(stderr, "%d행: time_ms(%ld)가 직전 행(%ld)보다 크지 않아 건너뜁니다\n",
                lineNo, row->timeMs, sim->prevTime);
        return 0;
    }

    if (delta % CYCLE_MS != 0)
    {
        fprintf(stderr, "%d행: 간격 %ldms 가 %dms 의 배수가 아닙니다 (연속 시간은 주기당 %dms 로 셈)\n",
                lineNo, delta, CYCLE_MS, CYCLE_MS);
    }

    return 1;
}

static void StepSimulator(Simulator *sim, const RowInput *row)
{
    long now = row->timeMs;
    RxState vehicleRx;
    EngineState engineState;
    int validSpeed;
    int lightTimerEnabled;
    int i;
    DoorOpenWarningState doorOpenWarning;
    Outputs *out = &sim->out;

    /* ---------- 입력 수신 ---------- */
    ReceivePeriodic(&sim->speed, row, COL_SPEED);
    ReceivePeriodic(&sim->gear, row, COL_GEAR);
    ReceivePeriodic(&sim->engine, row, COL_ENGINE);
    ReceivePeriodic(&sim->illuminance, row, COL_ILLUMINANCE);
    ReceivePeriodic(&sim->rain, row, COL_RAIN);

    for (i = 0; i < 4; i++)
    {
        if (row->present[COL_DOOR_FL + i])
        {
            sim->doors[i] = (DoorState)row->value[COL_DOOR_FL + i];
        }
    }
    if (row->present[COL_LIGHT_SWITCH])
    {
        sim->lightSwitch = (LightSwitch)row->value[COL_LIGHT_SWITCH];
    }
    if (row->present[COL_WIPER_SWITCH])
    {
        sim->wiperSwitch = (WiperSwitch)row->value[COL_WIPER_SWITCH];
    }

    /* ---------- C4 차량 상태 수신 중단 ---------- */
    vehicleRx = CheckVehicleStateRx(SimRxState(&sim->speed, now),
                                    SimRxState(&sim->gear, now),
                                    SimRxState(&sim->engine, now));

    /* C4: 수신 중단이면 조명·와이퍼는 시동 OFF 로 본다 (C1 시간 조건에도 적용) */
    engineState = (vehicleRx == RX_TIMEOUT) ? ENGINE_OFF : (EngineState)sim->engine.value;

    /* C3 유효한 속도 */
    validSpeed = SimValidSpeed(&sim->speed, &sim->gear, &sim->engine, vehicleRx);

    /* ---------- C2 센서 고장 ---------- */
    out->lightSensor = SimSensorState(&sim->illuminance, now, CheckLightSensorFault);
    out->rainSensor = SimSensorState(&sim->rain, now, CheckRainSensorFault);

    /* ---------- 와이퍼 (W1 > W4 > W5 > W2·W3) ---------- */
    out->wiper = ControlWiper(vehicleRx, engineState, sim->wiperSwitch,
                              out->rainSensor, sim->rain.value, validSpeed);

    /* C1: L6 시간은 시동 ON 이고 와이퍼가 작동하는 동안 센다.
     *     시동 OFF·수신 중단이면 ControlWiper 가 WIPER_STOP(W1, C4)을 내므로
     *     UpdateWiperActiveTime 만으로 0 이 된다 (별도 시동 조건 불필요). */
    sim->wiperActiveTime = UpdateWiperActiveTime(out->wiper, sim->wiperActiveTime);

    /* C1: L2·L3 시간 조건 */
    lightTimerEnabled = SimLightTimerEnabled(engineState, sim->lightSwitch, out->lightSensor,
                                             sim->illuminance.everReceived);
    sim->lowIlluminanceTime = lightTimerEnabled
                              ? UpdateLowIlluminanceTime(sim->illuminance.value, sim->lowIlluminanceTime)
                              : 0;
    sim->highIlluminanceTime = lightTimerEnabled
                               ? UpdateHighIlluminanceTime(sim->illuminance.value, sim->highIlluminanceTime)
                               : 0;

    /* ---------- 전조등 (L1 > L5 > L4 > L6 > L2·L3) ---------- */
    out->headLight = ControlHeadLight(vehicleRx, engineState, out->lightSensor, sim->lightSwitch,
                                      sim->wiperActiveTime, sim->lowIlluminanceTime,
                                      sim->highIlluminanceTime, out->headLight);

    /* ---------- 도어 잠금 (D1 > D2 > D3) ---------- */
    out->doorLock = ControlDoorLock(out->doorLock, (EngineState)sim->engine.value,
                                    (Gear)sim->gear.value, validSpeed, vehicleRx);

    /* ---------- 경고 (D4, D5) ---------- */
    doorOpenWarning = CheckDoorOpenWhileDriving(validSpeed, sim->doors[0], sim->doors[1],
                                                sim->doors[2], sim->doors[3]);
    out->warning = DetermineClusterWarning(doorOpenWarning, out->lightSensor, out->rainSensor);
}

/* =========================
 * 로그 출력 (3.2 출력 항목)
 * ========================= */
static const char *HeadLightText(HeadLightState s)
{
    return (s == HEADLIGHT_ON) ? "켜짐" : "꺼짐";
}

static const char *SensorText(SensorState s)
{
    return (s == SENSOR_FAULT) ? "고장" : "정상";
}

static const char *WiperText(WiperState s)
{
    switch (s)
    {
        case WIPER_STOP:         return "정지";
        case WIPER_INTERMITTENT: return "간헐";
        case WIPER_LOW:          return "저속";
        case WIPER_HIGH:         return "고속";
    }

    return "?";
}

static const char *DoorLockText(DoorLockState s)
{
    return (s == DOOR_LOCKED) ? "잠김" : "풀림";
}

static const char *WarningText(ClusterWarning w)
{
    switch (w)
    {
        case WARNING_NONE:               return "없음";
        case WARNING_DOOR_OPEN:          return "주행 중 도어 열림";
        case WARNING_LIGHT_SENSOR_FAULT: return "조도 센서 고장";
        case WARNING_RAIN_SENSOR_FAULT:  return "강우 센서 고장";
    }

    return "?";
}

#define LOG_COLUMN_COUNT 7

/* 로그 열 너비 (화면 폭 기준, 마지막 열은 패딩 없음) */
static const int LOG_WIDTHS[LOG_COLUMN_COUNT] = { 9, 7, 10, 7, 10, 10, 0 };

/* 화면 폭 기준으로 칸을 맞춘다 (UTF-8 한글은 3바이트, 폭 2) */
static void PrintPadded(FILE *fp, const char *text, int width)
{
    int display = 0;
    const unsigned char *p = (const unsigned char *)text;

    while (*p != '\0')
    {
        if (*p < 0x80)       { display += 1; p += 1; }
        else if (*p >= 0xF0) { display += 2; p += 4; }
        else if (*p >= 0xE0) { display += 2; p += 3; }
        else                 { display += 1; p += 2; }
    }

    fputs(text, fp);
    while (display++ < width)
    {
        fputc(' ', fp);
    }
}

static void PrintLogLine(FILE *fp, const char *cells[LOG_COLUMN_COUNT])
{
    int i;

    for (i = 0; i < LOG_COLUMN_COUNT; i++)
    {
        if (i > 0)
        {
            fputs("| ", fp);
        }
        PrintPadded(fp, cells[i], LOG_WIDTHS[i]);
    }
    fputc('\n', fp);
}

/* 같은 이름의 로그가 이미 있으면 _2, _3 … 을 붙여 겹치지 않는 경로를 만든다.
 * 성공 시 1, 빈 이름을 찾지 못하면 0 */
static int MakeLogPath(char *path, const char *stamp)
{
    int suffix;

    for (suffix = 1; suffix <= MAX_LOG_SUFFIX; suffix++)
    {
        FILE *existing;

        if (suffix == 1)
        {
            sprintf(path, "%s/simulation_log_%s.txt", DEFAULT_OUTPUT_DIR, stamp);
        }
        else
        {
            sprintf(path, "%s/simulation_log_%s_%d.txt", DEFAULT_OUTPUT_DIR, stamp, suffix);
        }

        existing = fopen(path, "r");
        if (existing == NULL)
        {
            return 1;
        }
        fclose(existing);
    }

    return 0;
}

static void PrintLogHeader(FILE *fp, const char *inputPath, const char *runTime)
{
    static const char *titles[LOG_COLUMN_COUNT] = {
        "time_ms", "전조등", "조도 센서", "와이퍼", "강우 센서", "도어 잠금", "계기판 경고"
    };

    fprintf(fp, "# 차체 편의 기능 시뮬레이션 로그 (VBC-SRS-001 v1.0, 3.2 출력)\n");
    fprintf(fp, "# 실행 시각: %s\n", runTime);
    fprintf(fp, "# 입력: %s\n", inputPath);
    fprintf(fp, "# 주기: %d ms, 수신 중단 판정: %d ms\n", CYCLE_MS, RX_TIMEOUT_MS);
    fprintf(fp, "# 변경 표시(*): 직전 행과 출력이 달라진 행\n");
    fprintf(fp, "# 보간 표시(~): 입력 행이 없어 \"아무 값도 들어오지 않은 주기\"로 채운 행\n");
    fprintf(fp, "#\n");
    PrintLogLine(fp, titles);
    fputs("---------+--------+-----------+--------+-----------+-----------+------------------\n", fp);
}

static void PrintLogRow(FILE *fp, long timeMs, const Outputs *out, int changed, int filled)
{
    char timeText[24];
    const char *cells[LOG_COLUMN_COUNT];

    /* snprintf 는 C89·VS2013 이하에 없으므로 sprintf 사용 (long 최대 20자 + "~*" + NUL = 23자) */
    sprintf(timeText, "%ld%s%s", timeMs, filled ? "~" : "", changed ? "*" : "");
    cells[0] = timeText;
    cells[1] = HeadLightText(out->headLight);
    cells[2] = SensorText(out->lightSensor);
    cells[3] = WiperText(out->wiper);
    cells[4] = SensorText(out->rainSensor);
    cells[5] = DoorLockText(out->doorLock);
    cells[6] = WarningText(out->warning);
    PrintLogLine(fp, cells);
}

static int SameOutputs(const Outputs *a, const Outputs *b)
{
    return a->headLight == b->headLight &&
           a->lightSensor == b->lightSensor &&
           a->wiper == b->wiper &&
           a->rainSensor == b->rainSensor &&
           a->doorLock == b->doorLock &&
           a->warning == b->warning;
}

/* =========================
 * 시뮬레이션
 * ========================= */
int main(int argc, char *argv[])
{
    const char *inputPath = (argc > 1) ? argv[1] : DEFAULT_INPUT_PATH;
    const char *outputPath;
    char logPath[LOG_PATH_MAX];
    char stamp[32];
    char runTime[32];
    time_t startTime = time(NULL);
    struct tm *local = localtime(&startTime);
    FILE *in;
    FILE *out;
    char line[CSV_LINE_MAX];
    int lineNo = 0;
    int rowCount = 0;
    int skippedRows = 0;
    int badValues = 0;
    int filledRows = 0;
    int writeFailed;
    Simulator sim;
    Outputs prev;

#ifdef _WIN32
    /* 콘솔(기본 CP949)에서 한글 안내 메시지가 깨지지 않게 한다 */
    SetConsoleOutputCP(CP_UTF8);
#endif

    if (local == NULL ||
        strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", local) == 0 ||
        strftime(runTime, sizeof(runTime), "%Y-%m-%d %H:%M:%S", local) == 0)
    {
        strcpy(stamp, "unknown");
        strcpy(runTime, "unknown");
    }

    if (argc > 2)
    {
        outputPath = argv[2];
    }
    else if (MakeLogPath(logPath, stamp))
    {
        outputPath = logPath;
    }
    else
    {
        fprintf(stderr, "%s 에 같은 시각의 로그가 너무 많습니다\n", DEFAULT_OUTPUT_DIR);
        return 1;
    }

    in = fopen(inputPath, "r");
    if (in == NULL)
    {
        fprintf(stderr, "입력 파일을 열 수 없습니다: %s\n", inputPath);
        return 1;
    }

    out = fopen(outputPath, "w");
    if (out == NULL)
    {
        fprintf(stderr, "출력 파일을 열 수 없습니다: %s\n", outputPath);
        fclose(in);
        return 1;
    }

    InitSimulator(&sim);
    prev = sim.out;
    PrintLogHeader(out, inputPath, runTime);

    while (fgets(line, sizeof(line), in) != NULL)
    {
        RowInput row;
        int gapCycles;
        char *text = line;

        lineNo++;

        /* 버퍼보다 긴 줄은 나머지를 버리고 건너뛴다 */
        if (strchr(line, '\n') == NULL && !feof(in))
        {
            int c;
            while ((c = fgetc(in)) != '\n' && c != EOF)
            {
            }
            fprintf(stderr, "%d행: %d자를 넘는 줄이어서 건너뜁니다\n", lineNo, CSV_LINE_MAX - 1);
            skippedRows++;
            continue;
        }

        if (lineNo == 1)
        {
            /* UTF-8 BOM 과 헤더 */
            if (strncmp(text, "\xEF\xBB\xBF", 3) == 0)
            {
                text += 3;
            }
            if (strncmp(text, "time_ms", 7) == 0)
            {
                continue;
            }
        }

        if (Trim(text)[0] == '\0')
        {
            continue;
        }

        if (!ParseRow(text, lineNo, &row, &badValues))
        {
            skippedRows++;
            continue;
        }

        if (!AcceptTime(&sim, &row, lineNo))
        {
            skippedRows++;
            continue;
        }

        /* 빠진 주기는 아무 값도 들어오지 않은 주기로 채운다 (C2, C4 미수신 판정) */
        gapCycles = 0;
        while (rowCount > 0 && row.timeMs - sim.prevTime > CYCLE_MS)
        {
            RowInput empty;

            if (gapCycles == MAX_FILL_CYCLES)
            {
                /* 모든 입력이 수신 중단 상태라 출력이 그대로이므로 나머지는 건너뛴다 */
                fprintf(out, "# %ld~%ld ms: 입력 없음 지속 (출력 변화 없음, 생략)\n",
                        sim.prevTime + CYCLE_MS, row.timeMs - CYCLE_MS);
                sim.prevTime = row.timeMs - CYCLE_MS;
                break;
            }

            memset(&empty, 0, sizeof(empty));
            empty.timeMs = sim.prevTime + CYCLE_MS;
            StepSimulator(&sim, &empty);
            PrintLogRow(out, empty.timeMs, &sim.out, !SameOutputs(&sim.out, &prev), 1);
            prev = sim.out;
            sim.prevTime = empty.timeMs;
            filledRows++;
            gapCycles++;
        }

        StepSimulator(&sim, &row);
        PrintLogRow(out, row.timeMs, &sim.out, rowCount > 0 && !SameOutputs(&sim.out, &prev), 0);
        prev = sim.out;
        sim.prevTime = row.timeMs;
        rowCount++;
    }

    writeFailed = ferror(out);
    if (fclose(out) != 0)
    {
        writeFailed = 1;
    }
    fclose(in);

    if (writeFailed)
    {
        fprintf(stderr, "로그 파일 쓰기에 실패했습니다: %s\n", outputPath);
        return 1;
    }

    printf("시뮬레이션 완료: %d행 처리, 보간 주기 %d, 건너뛴 행 %d, 잘못된 값 %d, 로그: %s\n",
           rowCount, filledRows, skippedRows, badValues, outputPath);
    return (skippedRows > 0 || badValues > 0) ? 2 : 0;
}
