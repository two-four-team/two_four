#!/usr/bin/env bash
# 빌드 후 시뮬레이터를 실행한다.
# 사용법: ./run.sh [입력 csv] [출력 로그]
#   인자를 생략하면 input/input.csv를 읽어 output/simulation_log_YYYYMMDD_HHMMSS.txt에 기록한다.
set -e

cd "$(dirname "$0")"

gcc -std=c99 -Wall -Wextra -I Common -o vehicle_sim \
    Common/main.c HeadLight/headLight.c Wiper/Wiper.c Door/Door.c \
    sensor/sensor.c "Dashboard Warning/ClusterWarning.c"

mkdir -p output

./vehicle_sim "$@"
