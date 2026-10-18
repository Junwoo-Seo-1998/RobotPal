# RobotPal 카메라 스트리밍 성능 최적화

> **Canonical benchmark — 2026-10-08**
>
> 이 문서의 수치와 해석을 RobotPal 카메라 스트리밍 성능의 최신 기준으로 사용한다. 이전 문서·포트폴리오·경험 DB에 남아 있던 32.9→37.3fps, Readback 24.4→20.7ms, 큐 폐기 297→0 등의 탐색 측정 수치는 최신 성과 수치로 사용하지 않는다.

## 1. 목표

JETANK 번호판 인식에 필요한 영상 품질을 유지하기 위해 카메라 해상도를 **1232×832**로 고정하고, 실물 카메라 운용 수준을 고려해 스트리밍 상한을 **60 FPS**로 설정했다.

해상도를 낮추는 방식은 사용하지 않고 다음 두 항목을 개선 대상으로 삼았다.

- Camera Streaming Send FPS
- Streaming ON 상태의 Simulation FPS

수신·디코딩 성능은 이번 측정 범위에서 제외했다.

---

## 2. 실험 구성

세 단계의 구조를 동일한 환경에서 비교했다.

| 단계 | GPU Readback | JPEG |
|---|---|---|
| baseline-sync | 동기 `glReadPixels` | Main Thread |
| PBO | Non-blocking PBO + Fence | Main Thread |
| PBO-MT | Non-blocking PBO + Fence | 4 Worker Threads |

Streaming OFF/ON을 각각 5회 측정하고 median을 사용했다. Streaming-OFF FPS의 브랜치 간 차이는 **0.557%**로 sanity gate 5%를 통과했다.

---

## 3. 최종 결과

| 단계 | Simulation FPS OFF | Simulation FPS ON | Camera Send FPS | Streaming Penalty |
|---|---:|---:|---:|---:|
| Sync | 102.38 | 66.70 | 26.95 | 34.84% |
| PBO | 102.52 | 65.60 | 25.49 | 36.02% |
| PBO + MT | 102.95 | **96.01** | **40.05** | **6.74%** |

최종 PBO+MT 구조는 Sync 대비 Streaming ON Simulation FPS를 **약 43.9%**, Camera Send FPS를 **약 48.6%** 향상시켰다.

Streaming으로 인한 Simulation FPS 손실은 **34.84% → 6.74%**, 즉 **28.1%p 감소**했다.

---

## 4. 병목 분석

Tracy 계측 결과 동기 방식의 Readback p50은 **1.144 ms**였지만 JPEG 압축은 **16.406 ms**가 소요됐다.

| 단계 | 구간 | p50 | p95 | p99 |
|---|---|---:|---:|---:|
| Sync | Readback | 1.144 ms | 2.046 ms | 2.570 ms |
| Sync | JPEG | **16.406 ms** | 19.216 ms | 20.422 ms |
| PBO | Readback | 1.908 ms | 2.705 ms | 3.089 ms |
| PBO | JPEG | **17.601 ms** | 20.726 ms | 22.115 ms |
| PBO-MT | Readback | 2.415 ms | 3.860 ms | 4.261 ms |
| PBO-MT | JPEG Worker | 27.075 ms | 32.353 ms | 34.349 ms |
| PBO-MT | Main-thread enqueue | **0.017 ms** | 0.044 ms | 0.060 ms |

따라서 초기 가설과 달리 **GPU Readback보다 JPEG 압축이 훨씬 큰 main-thread 병목**이었다.

`stb_image_write` 자체도 구현 목표를 “compactness and simplicity”에 두며 최적 runtime performance를 목표로 하지 않는다고 명시한다.

---

## 5. PBO 단독 적용이 개선되지 않은 이유

PBO는 pixel transfer를 비동기화해 CPU와 GPU 작업을 겹치게 하기 위한 기법이다. 하지만 PBO를 사용한 뒤 데이터를 너무 빨리 접근하면 비동기화 이점을 얻기 어렵고, 전송 중 수행할 다른 작업이 있어야 효과가 있다.

이번 PBO 구현에서는 GPU readback을 기다리지 않도록 fence polling을 적용했지만, 완료된 PBO 데이터를 JPEG 입력으로 사용하려면 결국 CPU 메모리로 복사해야 한다.

1232×832 RGB 한 프레임은 약 **2.93 MiB**다.

```text
GPU Render Target
      ↓
PBO
      ↓
map
      ↓
약 2.93 MiB memcpy
      ↓
CPU JPEG
```

결과적으로 Sync의 단순 readback 비용을 줄여 얻는 이익보다 PBO 관리와 CPU 복사 비용이 더 크게 나타났고, PBO-only에서는 유의미한 전체 성능 개선이 발생하지 않았다.

따라서 이번 결과에서 중요한 것은 **“PBO가 느린 기술”이라는 결론이 아니라, 이 workload에서 PBO만으로 제거할 수 있는 병목 비중이 작았다**는 점이다.

---

## 6. 내장 GPU와 외장 GPU의 차이

Intel Iris Xe와 같은 iGPU는 별도의 VRAM 대신 CPU와 system memory를 공유한다. 따라서 이번 환경에서는 별도 VRAM을 가진 dGPU에 비해 GPU→CPU pixel transfer를 비동기화해서 얻을 수 있는 이점이 상대적으로 작을 수 있다.

다만 이것이 **iGPU에서 PBO나 double buffering이 무효라는 의미는 아니다.** 충분한 GPU/CPU 작업을 중첩할 수 있다면 iGPU에서도 PBO가 효과를 낼 수 있다.

이번 실험에서 확인된 것은 제한적이다.

> Iris Xe + 1232×832 + 즉시 CPU JPEG 처리라는 현재 workload에서는 PBO-only의 이점이 관측되지 않았으며, 메모리 복사와 CPU 처리 비용을 줄이는 것이 더 중요했다.

반대로 dedicated VRAM을 사용하는 dGPU에서는 framebuffer readback 시 GPU 메모리와 CPU 메모리 사이의 전송을 다른 작업과 중첩할 여지가 더 크기 때문에 PBO의 비동기 전송 구조가 보다 의미 있는 효과를 낼 가능성이 있다. 그러나 실제 효과는 GPU, 드라이버, format 및 pipeline 구조에 따라 별도 측정이 필요하다.

---

## 7. 멀티스레딩 효과

PBO-MT에서는 JPEG 단일 작업 시간이 오히려 **p50 27.075 ms**로 증가했다.

이는 JPEG 자체가 빨라진 것이 아니다. 여러 worker가 동시에 CPU 자원을 사용하면서 개별 job latency는 증가할 수 있다.

핵심은 **Main Thread가 JPEG 완료를 기다리지 않게 된 것**이다.

```text
Before

Simulation
 → Readback
 → JPEG 16~20 ms
 → Send
 → 다음 Simulation Frame


After

Simulation
 → Readback
 → Queue 0.017 ms
 → 다음 Simulation Frame

             └→ JPEG Worker ×4
                  → Send
```

그 결과 개별 JPEG latency가 증가했음에도 여러 worker가 병렬로 처리하면서 전체 Camera Send FPS는 **26.95 → 40.05 FPS**로 증가했고, Simulation FPS는 **66.70 → 96.01 FPS**로 회복됐다.

즉 최종 성능 향상의 핵심은 **JPEG 알고리즘의 고속화가 아니라 JPEG 작업을 simulation critical path에서 제거한 것**이다.

---

## 8. 결론

이번 최적화에서는 처음에 GPU Readback을 주요 병목으로 예상해 PBO 기반 비동기 readback을 적용했다. 그러나 실제 계측 결과 PBO 단독 적용은 성능을 개선하지 못했다.

Tracy 분석을 통해 Readback보다 JPEG 압축이 훨씬 큰 Main Thread 병목임을 확인했고, JPEG 작업을 worker thread로 분리했다.

최종적으로 **1232×832 번호판 인식 해상도를 유지하면서**

- **Simulation FPS**: `66.70 → 96.01 FPS`
- **Camera Send FPS**: `26.95 → 40.05 FPS`
- **Streaming Penalty**: `34.84% → 6.74%`

를 달성했다.

이번 결과의 핵심은 특정 최적화 기법을 적용한 것 자체가 아니라,

> **병목을 추정한 뒤 실제 데이터를 측정하고, 예상과 다른 결과가 나오자 다시 프로파일링하여 진짜 병목인 JPEG 압축을 찾아 구조적으로 분리한 과정**

에 있다.

현재 60 FPS 송신 목표에는 도달하지 못했으므로 향후 최적화 대상은 PBO보다 **JPEG 인코더 처리량, 불필요한 CPU 메모리 복사, worker 4개 처리 구조**가 우선이다.
