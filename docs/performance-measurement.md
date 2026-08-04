# RobotPal 성능 측정 기준

성능값은 `StreamingSystemModule`에서 생성되어 PBO Readback, 용량 6 인코딩 큐, JPEG 품질 70 인코딩, 무제한 TCP 송신 큐, Python TCP 수신, OpenCV 디코딩·반전·224×224 리사이즈를 거쳐 `latest_image`에 기록되는 실제 경로에서만 얻는다. 인코딩 큐가 가득 차면 가장 오래된 프레임 1개를 폐기하고 최신 프레임을 적재한다.

## 비교 및 실행 조건

동일 Release 바이너리에서 `ROBOTPAL_ENCODE_WORKERS=1`을 기준, `ROBOTPAL_ENCODE_WORKERS=4`를 후보로 사용한다. 바뀌는 변수는 워커 수 하나이며 측정 전에는 후보를 개선 구현이라고 부르지 않는다. 동일 장비·네트워크·장면·해상도·입력 FPS·JPEG 품질·프로토콜·Python 소비 코드를 유지한다.

`x64-release`로 빌드하고 각 조건을 5회 이상 실행한다. 권장 순서는 `1,4,4,1,1,4,4,1,1,4`이다. 워밍업 10초는 제외한다. 실행마다 커밋 SHA, `git status --short`, 명령, 설정, CPU/GPU/RAM/OS를 보관한다.

송신 프로그램에는 `ROBOTPAL_BENCHMARK_LOG=<sender.jsonl>`, Python 수신기에는 `ROBOTPAL_RECEIVER_BENCHMARK_LOG=<receiver.jsonl>`을 설정한다. 계측 모드에서만 패킷에 프레임 ID와 생성 시각이 추가되며 두 조건 모두 같은 모드를 사용한다.

```powershell
tools/performance/sample-process.ps1 -ProcessId <RobotPalPID>,<PythonPID> -Output system.csv -DurationSeconds 70
python tools/performance/analyze.py --sender sender.jsonl --receiver receiver.jsonl --system-csv system.csv --warmup-seconds 10 --output summary.json
python tools/performance/compare.py --baseline <worker1-summary 5개 이상> --candidate <worker4-summary 5개 이상> --output comparison.json
```

## 고정된 지표 정의

- 처리량: `consumed / 측정 초`
- 전체 미소비율: `(frame_generated - consumed) / frame_generated`
- 인코딩 큐 폐기율: `encode_queue_dropped / frame_generated`; 폐기 원인은 `drop_oldest_capacity_6`으로 기록한다.
- WebSocket 큐 폐기율: `receive_dropped / (received + receive_dropped)`; TCP 경로에는 bounded queue나 큐 초과 폐기가 없다.
- 구간 시간과 종단 간 지연: p50·p95·p99·최대. 종단 간은 생성부터 `latest_image` 갱신 직후까지이며 동일 호스트에서만 확정한다.
- 최대 큐 길이: 적재 직후 관측한 인코딩 큐 및 TCP 송신 큐 길이의 최대.
- 실패: `readback_failed`, `encode_failed`, `send_failed`, `receive_failed`, `receive_dropped`를 원인별로 집계한다. `encode_queue_dropped`는 의도된 정책 결과이므로 실패와 분리한다.
- CPU: 표본 간 프로세스 CPU 시간 증가량/표본 간격. 메모리: 두 프로세스 Working Set 합계의 최대.

렌더 스케줄 자체가 생성하지 못한 프레임은 현재 코드에서 직접 식별할 수 없으므로 확정하지 않는다. TCP 단일 `send()`의 부분 전송은 `partial_send`로 기록하되 애플리케이션 동작을 바꾸지 않기 위해 재전송하지 않는다.

## 결과 보관

`results/<commit>/<run-id>/`에 sender/receiver JSONL, system CSV, summary JSON, 환경 정보와 실행 순서를 보관한다. 절댓값과 `(후보-기준)/기준*100`을 함께 보고한다. 원본 로그, 조건별 5회, 동일 조건, 재현 명령 중 하나라도 없거나 변동이 크면 README·포트폴리오·이력서 수치로 사용하지 않는다. 측정 후 계산식을 변경하지 않는다.
