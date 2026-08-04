#!/usr/bin/env python3
import argparse, json, statistics
from pathlib import Path

METRICS = ("throughput_fps", "generated_fps", "overall_unconsumed_rate", "encode_queue_drop_rate")

def mean_metric(runs, key):
    values = [run[key] for run in runs if run.get(key) is not None]
    return statistics.mean(values) if values else None

def nested_values(runs, *keys):
    values = []
    for run in runs:
        value = run
        for key in keys: value = value.get(key) if isinstance(value, dict) else None
        if value is not None: values.append(value)
    return values

def compare_values(values_before, values_after):
    before = statistics.mean(values_before) if values_before else None
    after = statistics.mean(values_after) if values_after else None
    return {"baseline_absolute": before, "candidate_absolute": after,
            "relative_change_percent": ((after - before) / before * 100) if before not in (None, 0) and after is not None else None,
            "baseline_cv_percent": statistics.pstdev(values_before) / before * 100 if before and len(values_before) > 1 else None,
            "candidate_cv_percent": statistics.pstdev(values_after) / after * 100 if after and len(values_after) > 1 else None}

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--baseline", nargs="+", required=True, help="worker=1 summary JSON files")
    parser.add_argument("--candidate", nargs="+", required=True, help="worker=4 summary JSON files")
    parser.add_argument("--output", required=True); args = parser.parse_args()
    if len(args.baseline) < 5 or len(args.candidate) < 5:
        parser.error("at least five runs per condition are required")
    load = lambda paths: [json.loads(Path(path).read_text(encoding="utf-8")) for path in paths]
    baseline, candidate = load(args.baseline), load(args.candidate); comparison = {}
    for key in METRICS:
        comparison[key] = compare_values(nested_values(baseline, key), nested_values(candidate, key))
    for section in ("end_to_end",):
        for metric in ("p50_ms", "p95_ms", "p99_ms", "max_ms"):
            comparison[f"{section}.{metric}"] = compare_values(nested_values(baseline, section, metric), nested_values(candidate, section, metric))
    for event in ("readback_completed", "encode_dequeued", "encode_completed", "send_completed", "consumed"):
        for metric in ("p50_ms", "p95_ms", "p99_ms", "max_ms"):
            comparison[f"{event}.{metric}"] = compare_values(nested_values(baseline, "intervals", event, metric), nested_values(candidate, "intervals", event, metric))
    comparison["average_cpu_percent"] = compare_values(nested_values(baseline, "system", "average_cpu_percent"), nested_values(candidate, "system", "average_cpu_percent"))
    comparison["max_working_set_bytes"] = compare_values(nested_values(baseline, "system", "max_working_set_bytes"), nested_values(candidate, "system", "max_working_set_bytes"))
    output = {"baseline_runs": len(baseline), "candidate_runs": len(candidate),
              "all_runs_reliable": all(r["reliability"]["all_selected_accounted"] and r["reliability"]["all_nondropped_sent"] and r["reliability"]["all_sent_received"] and r["reliability"]["all_received_consumed"] and r["reliability"]["failure_events"] == 0 for r in baseline + candidate),
              "comparison": comparison}
    Path(args.output).write_text(json.dumps(output, indent=2), encoding="utf-8")

if __name__ == "__main__": main()
