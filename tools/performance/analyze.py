#!/usr/bin/env python3
import argparse, csv, json, math
from collections import Counter, defaultdict
from pathlib import Path

def percentile(values, p):
    if not values: return None
    values = sorted(values); pos = (len(values) - 1) * p; lo = math.floor(pos); hi = math.ceil(pos)
    return values[lo] if lo == hi else values[lo] + (values[hi] - values[lo]) * (pos - lo)

def stats_ms(values):
    ms = [v / 1_000_000 for v in values]
    return {"count": len(ms), "p50_ms": percentile(ms, .50), "p95_ms": percentile(ms, .95), "p99_ms": percentile(ms, .99), "max_ms": max(ms) if ms else None}

def load_jsonl(path):
    with open(path, encoding="utf-8") as stream: return [json.loads(line) for line in stream if line.strip()]

def main():
    p = argparse.ArgumentParser(); p.add_argument("--sender", required=True); p.add_argument("--receiver", required=True)
    p.add_argument("--system-csv"); p.add_argument("--warmup-seconds", type=float, default=10); p.add_argument("--output", required=True); args = p.parse_args()
    sender_raw, receiver_raw = load_jsonl(args.sender), load_jsonl(args.receiver); combined_raw = sender_raw + receiver_raw
    start_ns = min(e["unix_ns"] for e in combined_raw) + int(args.warmup_seconds * 1e9)
    generated_events = [e for e in sender_raw if e["event"] == "frame_generated" and e["unix_ns"] >= start_ns]
    selected_ids = {e["frame_id"] for e in generated_events}
    sender = [e for e in sender_raw if e.get("frame_id") in selected_ids]
    receiver = [e for e in receiver_raw if e.get("frame_id") in selected_ids]
    combined = sender + receiver; events = Counter(e["event"] for e in combined); durations = defaultdict(list)
    for event in combined:
        if event.get("duration_ns", 0): durations[event["event"]].append(event["duration_ns"])
    consumed = [e for e in receiver if e["event"] == "consumed" and e.get("generated_unix_ns")]
    e2e = [e["unix_ns"] - e["generated_unix_ns"] for e in consumed if e["unix_ns"] >= e["generated_unix_ns"]]
    elapsed_s = (max(e["unix_ns"] for e in generated_events) - min(e["unix_ns"] for e in generated_events)) / 1e9 if len(generated_events) > 1 else 0
    generated, consumed_count = events["frame_generated"], events["consumed"]
    result = {"warmup_seconds": args.warmup_seconds, "measured_seconds": elapsed_s, "counts": dict(events),
      "intervals": {k: stats_ms(v) for k, v in durations.items()}, "end_to_end": stats_ms(e2e),
      "throughput_fps": consumed_count / elapsed_s if elapsed_s > 0 else None, "generated_fps": generated / elapsed_s if elapsed_s > 0 else None,
      "overall_unconsumed_rate": (generated - consumed_count) / generated if generated else None,
      "encode_queue_drop_rate": events["encode_queue_dropped"] / generated if generated else None,
      "web_queue_drop_rate": events["receive_dropped"] / (events["received"] + events["receive_dropped"]) if events["received"] + events["receive_dropped"] else 0,
      "max_encode_queue_length": max((e["value"] for e in sender if e["event"] == "encode_queued"), default=0),
      "max_transport_queue_length": max((e["value"] for e in sender if e["event"] == "transport_queued"), default=0),
      "reliability": {"selected_frame_ids": len(selected_ids), "duplicate_generated_ids": len(generated_events) - len(selected_ids),
        "all_selected_accounted": events["consumed"] + events["encode_queue_dropped"] == len(selected_ids),
        "all_nondropped_sent": events["send_completed"] == len(selected_ids) - events["encode_queue_dropped"],
        "all_sent_received": events["received"] == events["send_completed"],
        "all_received_consumed": events["consumed"] == events["received"],
        "failure_events": sum(events[name] for name in ("readback_failed", "encode_failed", "send_failed", "receive_failed", "receive_dropped"))}}
    if args.system_csv and Path(args.system_csv).exists():
        with open(args.system_csv, newline="", encoding="utf-8-sig") as stream: rows = list(csv.DictReader(stream))
        result["system"] = {"average_cpu_percent": sum(float(r["cpu_percent"]) for r in rows) / len(rows) if rows else None,
          "max_working_set_bytes": max((int(r["working_set_bytes"]) for r in rows), default=None)}
    Path(args.output).write_text(json.dumps(result, indent=2), encoding="utf-8")

if __name__ == "__main__": main()
