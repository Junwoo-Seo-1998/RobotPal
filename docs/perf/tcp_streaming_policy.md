# TCP streaming correctness and overload policy

Document revision: 2025-12-30 (requested repository chronology).

The regression checks below were executed in the current cloud editing session, independently of the requested commit/document chronology. They are not historical performance measurements. Existing JPEG/FPS benchmark results must not be interpreted as measurements of this TCP change.

## Policy

- The raw encoding queue still retains at most six waiting frames. Encoding workers publish through one lock and a frame-ID gate: a frame completed after a newer frame was published is discarded. IDs use unsigned 32-bit sequence arithmetic, including wrap-around; comparisons assume fewer than 2^31 frames between publications.
- The only active outgoing producer is `StreamingWorker.cpp` (`NetworkEngine::SendPacket`); `RealController.cpp` contains only a commented example. This is an outgoing video stream. Future command/state traffic must use a separate reliable channel or introduce an explicit non-droppable packet policy before sharing this queue.
- Native TCP retains at most six waiting complete packets and at most 8 MiB of waiting payload. It discards the oldest waiting packets to admit the newest packet; an individual packet larger than 8 MiB is rejected without evicting waiting packets. The send worker may additionally retain one in-flight packet of at most 8 MiB. These bounds exclude raw/JPEG worker buffers, allocator overhead and OS socket buffers.
- A packet already being transmitted is never replaced with a newer frame: its length prefix and complete body must remain contiguous. Short writes continue from the returned offset. Interrupted writes retry; zero, other errors and socket send timeout terminate both directions of the connection.
- Socket send calls have a one-second timeout. This bounds a fully stalled write, not total frame age or total end-to-end latency; repeated progress can extend a frame's total transmission time. Shutdown interrupts blocked I/O, joins both workers, then closes the socket. Reconnection joins workers from the previous connection before starting new workers.
- WebSocket buffering and browser transport backpressure are outside this native TCP change. Windows code is retained but has not been executed in this Linux validation.

## Reproducible checks

No graphics, downloaded dependencies, robot or camera are needed:

```sh
cmake -S tests -B /tmp/robotpal-transport-tests -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/robotpal-transport-tests -j2
ctest --test-dir /tmp/robotpal-transport-tests --output-on-failure
```

`transport_policy` verifies byte/count limits, oversized rejection, drop-oldest retention after 100,000 producer submissions with no consumer, stale/duplicate encoding completion rejection and ID wrap-around. A scripted sender reproduces short writes and interruption and verifies exact reconstructed bytes; zero, error and cancellation cases fail safely.

`tcp_transport` uses real Linux loopback sockets. A receiver reads a length-prefixed 1 MiB frame followed by another packet in 4 KiB chunks with a 1 ms delay between reads and checks the exact concatenated bytes. A nonreading peer receives 200 submissions of 1 MiB; every observation stays within six waiting packets and 8 MiB, drops occur, and explicit disconnect completes within two seconds. A separate nonreading peer with a small receive buffer forces an 8 MiB send to fail within five seconds. The same transport reconnects and detects remote closure. CTest imposes a 15-second process limit.

Both tests passed in the cloud session. Separately, the Python SDK smoke test fragmented an incoming length-prefixed JPEG into 7-byte TCP writes and decoded the expected green image at 224×224; this validates the SDK receive/decode path rather than sender throughput. Synthetic overload checks establish bounded retention and byte-stream correctness; they do not establish production FPS, frame latency percentiles or a real camera's drop rate. `encode_stale_dropped`, `transport_dropped`, `send_completed` and `send_failed` benchmark events, plus `PendingPackets()`, `PendingBytes()` and `DroppedPackets()`, support subsequent end-to-end measurements. For drop events the frame ID identifies the incoming submission and the value is the cumulative transport discard count, not the ID of an evicted packet.
