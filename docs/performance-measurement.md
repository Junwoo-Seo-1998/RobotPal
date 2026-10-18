# RobotPal 카메라 스트리밍 성능 측정 기준

> **Canonical protocol — 2026-10-08**
>
> 최신 성능 수치와 해석은 [streaming-performance-result.md](./streaming-performance-result.md)를 기준으로 한다. 과거 1회 탐색 측정, 224×224/별도 해상도 마이크로벤치마크, 32.9→37.3fps 계열 수치는 최신 성과 수치로 사용하지 않는다.

## 1. 최적화 목표

JETANK 번호판 인식에 필요한 영상 품질을 유지하기 위해 해상도는 **1232×832**로 고정한다. 해상도를 낮춰 성능을 얻지 않는다.

스트리밍 상한은 **60 FPS**로 설정하며 다음 두 지표를 주 평가 대상으로 한다.

- Camera Streaming Send FPS
- Streaming ON 상태의 Simulation FPS

수신·디코딩 성능은 현재 최종 비교 범위에서 제외한다.

## 2. 비교 구조

같은 환경에서 아래 세 구조를 비교한다.

| 단계 | GPU Readback | JPEG |
|---|---|---|
| baseline-sync | 동기 `glReadPixels` | Main Thread |
| PBO | Non-blocking PBO + Fence | Main Thread |
| PBO-MT | Non-blocking PBO + Fence | 4 Worker Threads |

## 3. 반복 측정

- 각 구조에서 Streaming OFF와 Streaming ON을 각각 **5회** 실행한다.
- 대표값은 **median**을 사용한다.
- Streaming-OFF FPS의 브랜치 간 차이를 sanity check로 사용한다.
- 최신 실험에서는 브랜치 간 차이가 **0.557%**로 sanity gate **5%**를 통과했다.
- 해상도, 장면, 스트리밍 상한 등 비교 조건을 동일하게 유지한다.

## 4. 프로파일링 지표

Tracy로 최소 다음 구간을 분리해서 확인한다.

- GPU Readback
- JPEG compression / JPEG Worker
- Main-thread enqueue

최종 판단은 특정 함수 하나의 시간이 아니라 **Simulation FPS ON, Camera Send FPS, Streaming Penalty**와 함께 한다.

## 5. 해석 원칙

1. PBO 적용 자체를 성능 개선으로 간주하지 않는다.
2. PBO-only 결과가 기준보다 좋아지지 않으면 실제 병목 비중을 다시 측정한다.
3. JPEG worker의 개별 job latency와 전체 처리량을 구분한다.
4. 여러 worker에서 개별 JPEG 시간이 늘어도 main thread의 critical path에서 JPEG가 빠져 Simulation FPS와 Send FPS가 개선될 수 있다.
5. iGPU/dGPU 차이는 별도 측정 없이 일반화하지 않는다.
6. 60 FPS 목표에 도달하지 못한 사실과 남은 병목을 함께 기록한다.

## 6. 최신 기준 결과

| 단계 | Simulation FPS OFF | Simulation FPS ON | Camera Send FPS | Streaming Penalty |
|---|---:|---:|---:|---:|
| Sync | 102.38 | 66.70 | 26.95 | 34.84% |
| PBO | 102.52 | 65.60 | 25.49 | 36.02% |
| PBO + MT | 102.95 | 96.01 | 40.05 | 6.74% |

Sync 대비 PBO+MT에서 Streaming ON Simulation FPS는 약 **43.9%**, Camera Send FPS는 약 **48.6%** 향상됐다. Streaming Penalty는 **34.84% → 6.74%**로 **28.1%p 감소**했다.

세부 프로파일링 수치와 해석은 최신 결과 문서를 따른다.
