# two_four

> 두번째 프로젝트 - 4조
> **가상 차량 차체 편의 기능 시뮬레이터** (VBC-SRS-001 v1.0)

고객 요구사항 사양서(SRS) `VBC-SRS-001 v1.0`을 근거로, 가상 차량의 **조명 · 와이퍼 · 도어 잠금 · 계기판 경고** 기능을 C로 구현한 프로젝트입니다.
CSV로 주어진 차량 상태를 100ms 주기로 한 줄씩 읽어 각 제어 모듈을 호출하고, 그 결과를 텍스트 로그로 남깁니다. 입력 CSV만 바꾸면 출력이 어떻게 달라지는지 바로 확인할 수 있습니다.

## 목차

- [주요 기능](#주요-기능)
- [디렉터리 구조](#디렉터리-구조)
- [빌드와 실행](#빌드와-실행)
- [입력 형식](#입력-형식)
- [출력 로그](#출력-로그)
- [구조](#구조)
- [요구사항 요약](#요구사항-요약)
- [테스트 케이스](#테스트-케이스)
- [알려진 제약](#알려진-제약)

## 주요 기능

| 블록 | 받는 정보 | 내보내는 정보 |
| --- | --- | --- |
| 조명 | 시동, 조도, 조명 스위치, 와이퍼 동작 상태 | 전조등 켜짐/꺼짐, 조도 센서 고장 표시 |
| 와이퍼 | 시동, 강우량, 속도, 와이퍼 스위치 | 와이퍼 단계(정지·간헐·저속·고속), 강우 센서 고장 표시 |
| 도어·잠금·경고 | 시동, 기어, 속도, 도어 4개, 센서 고장 표시 | 잠금/해제, 계기판 경고(한 번에 하나) |

SRS의 필수 요구사항(C · L · W · D)을 구현 범위로 합니다. 선택 요구사항인 주행(R)과 팀 추가 기능(X)은 포함되어 있지 않으며, 속도·기어·시동은 CSV로 직접 입력합니다.

## 디렉터리 구조

```
two_four/
├── Common/
│   ├── main.c               # 시뮬레이터: CSV 입력, 수신 관리, 모듈 호출, 로그 출력
│   └── vehicle_control.h    # 공용 enum 타입과 모듈 함수 선언
├── HeadLight/
│   └── headLight.c          # 전조등 제어, 연속 시간 계산 (L1~L6, C1)
├── Wiper/
│   └── Wiper.c              # 와이퍼 제어 (W1~W5)
├── Door/
│   └── Door.c               # 도어 잠금, 주행 중 도어 열림 판단 (D1~D4)
├── Dashboard Warning/
│   └── ClusterWarning.c     # 계기판 경고 우선순위 결정 (D5)
├── sensor/
│   └── sensor.c             # 센서 범위 검사, 차량 상태 수신 판단 (C2, C4)
├── input/
│   └── input.csv            # 시뮬레이션 입력 시나리오
├── output/                  # 실행할 때마다 생성되는 시뮬레이션 로그
├── testcase/
│   └── testcase.xlsx        # 함수별 테스트 케이스
└── Plan/
    └── 고객_요구사항_사양서.pdf  # SRS (VBC-SRS-001 v1.0)
```

## 빌드와 실행

모든 명령은 **프로젝트 루트**에서 실행합니다. 외부 라이브러리 의존성은 없습니다.

### Linux / macOS (gcc)

```bash
gcc -std=c99 -Wall -Wextra -I Common -o vehicle_sim \
    Common/main.c HeadLight/headLight.c Wiper/Wiper.c Door/Door.c \
    sensor/sensor.c "Dashboard Warning/ClusterWarning.c"
```

### Windows (Visual Studio 개발자 명령 프롬프트)

```bat
cl /nologo /W3 /utf-8 /I Common /Fe:vehicle_sim.exe Common\main.c HeadLight\headLight.c ^
   Wiper\Wiper.c Door\Door.c sensor\sensor.c "Dashboard Warning\ClusterWarning.c"
```

- `main.c`는 UTF-8(BOM 포함)로 저장되어 있습니다. 편집기에서 다시 저장할 때 BOM을 지우지 마세요.
- Visual Studio 프로젝트로 빌드한다면 작업 디렉터리를 프로젝트 루트로 설정하거나, 입력/출력 경로를 인자로 넘기세요.

### 실행

```bash
./vehicle_sim [입력 csv] [출력 로그]
```

| 인자 | 기본값 | 설명 |
| --- | --- | --- |
| 입력 csv | `input/input.csv` | 차량 상태 시나리오 |
| 출력 로그 | `output/simulation_log_YYYYMMDD_HHMMSS.txt` | 생략하면 실행 시각으로 새 파일을 만듭니다. 같은 초에 다시 실행하면 `_2`, `_3` …이 붙습니다. 경로를 직접 주면 그 파일에 덮어씁니다. |

| 종료 코드 | 의미 |
| --- | --- |
| `0` | 정상 |
| `1` | 파일 오류 (입력/출력 파일을 열 수 없음, 쓰기 실패) |
| `2` | 입력에 건너뛴 행이나 해석할 수 없는 값이 있음 |

실행이 끝나면 처리한 행 수와 로그 경로가 콘솔에 출력됩니다.

```
시뮬레이션 완료: 833행 처리, 보간 주기 0, 건너뛴 행 0, 잘못된 값 0, 로그: output/simulation_log_20261008_111248.txt
```

## 입력 형식

첫 줄은 헤더이고, 한 행이 한 주기(100ms)입니다.

```csv
time_ms,speed,gear,engineState,illuminance,rainAmount,doorFL,doorFR,doorRL,doorRR,lightSwitch,wiperSwitch
700,0,GEAR_P,ENGINE_OFF,20000,0,DOOR_CLOSED,DOOR_CLOSED,DOOR_CLOSED,DOOR_CLOSED,LIGHT_AUTO,WIPER_SWITCH_AUTO
```

| 열 | 유효한 값 | 비고 |
| --- | --- | --- |
| `time_ms` | 0 이상의 정수 | 증가해야 하며 100ms 간격을 전제로 합니다 |
| `speed` | 0 ~ 250 (km/h) | 범위 밖은 무효 |
| `gear` | `GEAR_P` / `GEAR_R` / `GEAR_N` / `GEAR_D` | |
| `engineState` | `ENGINE_OFF` / `ENGINE_ON` | |
| `illuminance` | 0 ~ 60000 (lux) | 범위 밖은 조도 센서 고장 |
| `rainAmount` | 0 ~ 100 (%) | 범위 밖은 강우 센서 고장 |
| `doorFL` `doorFR` `doorRL` `doorRR` | `DOOR_CLOSED` / `DOOR_OPEN` | 앞 왼쪽, 앞 오른쪽, 뒤 왼쪽, 뒤 오른쪽 |
| `lightSwitch` | `LIGHT_AUTO` / `LIGHT_OFF` / `LIGHT_ON` | |
| `wiperSwitch` | `WIPER_SWITCH_OFF` / `WIPER_SWITCH_AUTO` / `WIPER_SWITCH_LOW` / `WIPER_SWITCH_HIGH` | |

입력 처리 규칙:

- **빈 칸**은 "이번 주기에 값이 들어오지 않음"으로 처리합니다. 수신 중단·센서 고장 시험(C2, C4)에 사용합니다.
  - 주기 입력(속도·기어·시동·조도·강우량)은 0.5초 동안 비면 수신 중단 또는 센서 고장이 됩니다.
  - 이벤트 입력(도어·스위치)은 빈 칸이면 직전 값을 유지합니다.
- 행 사이 간격이 100ms보다 크면, 빠진 주기를 "아무 값도 들어오지 않은 주기"로 채워서 처리합니다.
- 해석할 수 없는 값은 미수신으로 처리하고, 필드 수가 맞지 않거나 시간이 증가하지 않는 행은 건너뜁니다. 두 경우 모두 표준 오류로 경고를 출력하고 종료 코드 `2`를 반환합니다.

## 출력 로그

SRS 3.2의 출력 항목을 주기마다 한 줄씩 기록합니다.

```
# 차체 편의 기능 시뮬레이션 로그 (VBC-SRS-001 v1.0, 3.2 출력)
# 실행 시각: 2026-10-08 11:12:48
# 입력: input/input.csv
# 주기: 100 ms, 수신 중단 판정: 500 ms
#
time_ms  | 전조등 | 조도 센서 | 와이퍼 | 강우 센서 | 도어 잠금 | 계기판 경고
---------+--------+-----------+--------+-----------+-----------+------------------
400      | 꺼짐   | 정상      | 정지   | 정상      | 풀림      | 없음
500*     | 꺼짐   | 고장      | 정지   | 고장      | 풀림      | 조도 센서 고장
600      | 꺼짐   | 고장      | 정지   | 고장      | 풀림      | 조도 센서 고장
700*     | 꺼짐   | 정상      | 정지   | 정상      | 풀림      | 없음
```

| 표시 | 의미 |
| --- | --- |
| `*` | 직전 행과 출력이 달라진 행 |
| `~` | 입력 행이 없어 빈 주기로 채운 행 |

입력이 오래 끊겨 출력이 더 바뀌지 않는 구간은 한 줄로 요약합니다.

## 구조

제어 모듈은 모두 **내부 상태가 없는 순수 함수**입니다. 입력만으로 결과가 정해지므로 함수 단위로 테스트할 수 있습니다. 시간과 상태에 관한 처리는 `main.c`가 맡습니다.

| 담당 | 내용 |
| --- | --- |
| `main.c` | 0.5초 미수신 판정(C2, C4), 유효한 속도 판정(C3), 연속 시간 카운트 조건(C1), 상태 보관(전조등, 도어 잠금, 연속 시간) |
| 모듈 함수 | 전달받은 값과 상태만으로 출력 결정 |

한 주기의 처리 순서:

1. 입력 수신
2. 차량 상태 수신 중단 판정 (C4) → 유효한 속도 계산 (C3)
3. 센서 고장 판정 (C2)
4. 와이퍼 제어 → 와이퍼 작동 시간 갱신
5. 조도 연속 시간 갱신 → 전조등 제어
6. 도어 잠금 제어
7. 주행 중 도어 열림 판단 → 계기판 경고 결정

와이퍼를 전조등보다 먼저 계산하는 이유는 L6(와이퍼 10초 연속 작동 시 전조등 켜기)이 와이퍼 결과에 의존하기 때문입니다.

### 모듈 함수

| 함수 | 파일 | 역할 |
| --- | --- | --- |
| `ControlHeadLight` | `HeadLight/headLight.c` | 전조등 켜짐/꺼짐 결정 |
| `UpdateLowIlluminanceTime` | `HeadLight/headLight.c` | 조도 1000 lux 미만 연속 시간 |
| `UpdateHighIlluminanceTime` | `HeadLight/headLight.c` | 조도 1500 lux 초과 연속 시간 |
| `UpdateWiperActiveTime` | `HeadLight/headLight.c` | 와이퍼 연속 작동 시간 |
| `ControlWiper` | `Wiper/Wiper.c` | 와이퍼 단계 결정 |
| `ControlDoorLock` | `Door/Door.c` | 도어 잠금/해제 결정 |
| `CheckDoorOpenWhileDriving` | `Door/Door.c` | 주행 중 도어 열림 판단 |
| `DetermineClusterWarning` | `Dashboard Warning/ClusterWarning.c` | 표시할 경고 하나 선택 |
| `CheckLightSensorFault` | `sensor/sensor.c` | 조도 값 범위 검사 |
| `CheckRainSensorFault` | `sensor/sensor.c` | 강우량 값 범위 검사 |
| `CheckVehicleStateRx` | `sensor/sensor.c` | 속도·기어·시동 수신 상태 종합 |

## 요구사항 요약

전체 내용은 `Plan/`의 사양서를 참고하세요.

### 공통 (C)

| ID | 내용 |
| --- | --- |
| C1 | "N초 연속"은 조건이 끊기지 않고 N초 동안 성립해야 하며, 깨지면 처음부터 다시 셉니다 |
| C2 | 센서 값이 0.5초 동안 들어오지 않거나 유효 범위 밖이면 센서 고장. 정상 값이 다시 들어오면 풀립니다 |
| C3 | 속도·기어·시동이 모두 0.5초 이내에 들어왔고 속도가 0~250일 때만 유효한 속도 |
| C4 | 속도·기어·시동 중 하나라도 0.5초 동안 들어오지 않으면 차량 상태 수신 중단. 조명·와이퍼는 시동이 꺼진 것으로 봅니다 |
| C5 | 시작 상태는 모두 꺼짐, 정지, 잠금 해제, 경고 없음 |

### 조명 (L) — 우선순위 L1 > L5 > L4 > L6 > L2·L3

| ID | 내용 |
| --- | --- |
| L1 | 시동이 꺼져 있으면 전조등을 끈다 |
| L5 | 조도 센서가 고장이면 켜고, 고장을 표시한다 |
| L4 | 스위치 ON이면 켜고, OFF이면 끈다 |
| L6 | AUTO: 와이퍼가 10초 연속 작동하면 켜고, 작동하는 동안 켜 둔다 |
| L2 | AUTO: 조도 1000 lux 미만이 1초 연속이면 켠다 |
| L3 | AUTO: 조도 1500 lux 초과가 3초 연속이면 끈다. 어느 규칙에도 해당하지 않으면 직전 상태 유지 |

### 와이퍼 (W) — 우선순위 W1 > W4 > W5 > W2·W3

| ID | 내용 |
| --- | --- |
| W1 | 시동이 꺼져 있으면 정지한다 |
| W4 | 스위치 OFF는 정지, LOW는 저속, HIGH는 고속 |
| W5 | AUTO에서 강우 센서가 고장이면 간헐로 동작한다 |
| W2 | AUTO: 강우량 10% 미만 정지, 10~40% 간헐, 40~70% 저속, 70% 이상 고속 |
| W3 | AUTO이고 유효한 속도가 0이면 W2 결과를 한 단계 낮춘다 |

### 도어·잠금·경고 (D)

| ID | 내용 |
| --- | --- |
| D1 | 차량 상태 수신 중단이면 잠금 상태를 그대로 둔다 |
| D2 | 그 외에 시동이 꺼져 있거나 기어가 P이면 잠금을 푼다 |
| D3 | 그 외에 유효한 속도가 15 km/h 이상이면 잠근다. 해당 없으면 직전 상태 유지 |
| D4 | 유효한 속도가 0보다 크고 문이 하나라도 열려 있으면 "주행 중 도어 열림" 경고 |
| D5 | 경고는 하나만 표시: 주행 중 도어 열림 > 조도 센서 고장 > 강우 센서 고장 |

## 테스트 케이스

`testcase/testcase.xlsx`에 함수별 테스트 케이스가 시트 하나씩 정리되어 있습니다. 각 행은 입력 값과 기대 출력으로 이루어지며, 경계값(예: 연속 시간 999 / 1000 / 1001ms, 강우량 9 / 10 / 11%)을 포함합니다.

| 시트 (대상 함수) | 케이스 수 |
| --- | ---: |
| `ControlHeadLight` | 15 |
| `ControlWiper` | 24 |
| `ControlDoorLock` | 30 |
| `CheckDoorOpenWhileDriving` | 17 |
| `CheckLightSensorFault` | 6 |
| `CheckRainSensorFault` | 6 |
| `DetermineClusterWarning` | 4 |
| `CheckVehicleStateRx` | 8 |
| **합계** | **110** |

이 표는 테스트 설계 문서이며, 자동으로 실행하는 테스트 코드는 저장소에 포함되어 있지 않습니다. 통합 동작은 `input/input.csv` 시나리오를 실행한 뒤 `output/`의 로그로 확인합니다.

## 알려진 제약

- 주기는 100ms로 고정되어 있습니다. 행 간격이 100ms의 배수가 아니면 경고를 출력하고, 연속 시간은 주기당 100ms로 셉니다.
- 0.5초 미수신 판정, 유효한 속도 판정, 연속 시간 카운트 조건은 현재 `main.c`에 있습니다. 모듈로 옮길 수 있도록 `Sim` 접두어가 붙은 함수 단위로 분리해 두었습니다.
- `W3`의 단계 낮추기는 `WiperState` enum이 `STOP < INTERMITTENT < LOW < HIGH` 순서로 정의되어 있다는 전제에 의존합니다.
- CSV 한 줄은 최대 511자이며, 이를 넘는 줄은 건너뜁니다.
