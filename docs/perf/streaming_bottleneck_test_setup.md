# Streaming Bottleneck Test Environment

> **Supplemental benchmark only**
>
> 현재 RobotPal 카메라 스트리밍 성능의 공식 수치와 해석은 [../streaming-performance-result.md](../streaming-performance-result.md)를 사용한다. 이 문서는 JPEG/producer-consumer 구조를 별도로 압박하는 마이크로벤치마크의 실행 방법을 설명하기 위한 보조 문서이며, 여기서 얻은 수치를 실제 1232×832 애플리케이션의 성과로 사용하지 않는다.

## 1. 목적

`tests/streaming_frame_drop_benchmark.cpp`는 실제 렌더링·GPU Readback을 포함하지 않고 다음 CPU 처리 구간을 독립적으로 압박한다.

- RGBA → RGB conversion
- JPEG encoding
- producer/consumer queue pressure
- single worker와 multi worker 비교

따라서 이 벤치마크는 JPEG 처리 구조의 상대적인 특성을 확인하는 용도다. 최종 Simulation FPS나 Camera Send FPS를 대체하지 않는다.

## 2. 빌드

C++17, pthread, libjpeg 개발 환경이 필요하다.

```bash
c++ -O2 -std=c++17 \
  tests/streaming_frame_drop_benchmark.cpp \
  RobotPal/src/Util/JpegEncoder_libjpeg.cpp \
  -I./RobotPal/include -pthread -ljpeg \
  -o tests/streaming_frame_drop_benchmark
```

## 3. 실행 형식

```text
./tests/streaming_frame_drop_benchmark \
  <width> <height> <input_fps> <duration_sec> [quality] [queue_size]
```

비교 시 worker 전략 외의 입력 조건은 동일하게 유지한다.

## 4. 결과 해석

이 마이크로벤치마크에서 관찰한 drop rate나 worker 처리량은 실제 애플리케이션의 최신 성과값과 분리한다.

최신 end-to-end 실험은 **1232×832**, Streaming OFF/ON 각 **5회**, median 기준으로 Sync / PBO / PBO+4 Worker 구조를 비교했다. 그 결과 실제 주요 병목은 초기 예상과 달리 GPU Readback이 아니라 **main-thread JPEG compression**이었고, JPEG를 simulation critical path에서 분리했을 때 Simulation FPS ON과 Camera Send FPS가 개선됐다.

## 5. Cleanup

```bash
rm -f tests/streaming_frame_drop_benchmark
```
