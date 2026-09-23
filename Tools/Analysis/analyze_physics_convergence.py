"""Compare timestamped solver kinematics; a successful catch alone is insufficient."""
import argparse
import bisect
import csv
import json
import math
from pathlib import Path


def load_flight(path):
    report = json.loads(path.read_text(encoding="utf-8-sig"))
    cadence = json.loads(path.with_name(path.stem + "-cadence.json").read_text(encoding="utf-8-sig"))
    samples = {}
    fields = ["solver_" + axis + "_m" for axis in "xyz"] + ["solver_v" + axis + "_mps" for axis in "xyz"]
    with path.with_suffix(".csv").open(encoding="utf-8-sig", newline="") as stream:
        for row in csv.DictReader(stream):
            time = float(row["solver_sample_time_s"])
            if time > 0:
                value = tuple(float(row[field]) for field in fields)
                if not all(math.isfinite(v) for v in (time, *value)):
                    raise ValueError(f"Non-finite solver sample: {path}")
                samples[time] = value
    if len(samples) < 2:
        raise ValueError(f"Missing flight samples: {path}")
    ordered = sorted(samples)
    dt = cadence["configured_fixed_step_s"]
    if dt <= 0 or not cadence["async_physics"] or cadence["substepping"]:
        raise ValueError(f"Expected fixed asynchronous clock: {path}")
    for name in ("solver_steps", "inner_dynamics_steps", "flight_guidance_steps", "upper_stage_steps"):
        step = cadence[name]
        if step["count"] <= 0 or max(abs(step[k] - dt) for k in ("min_s", "max_s")) > 1e-8:
            raise ValueError(f"Variable/missing clock in {name}: {path}")
    return {"path": str(path), "report": report, "hz": 1 / dt,
            "game_hz": 1 / cadence["game_steps"]["mean_s"],
            "times": ordered, "samples": [samples[t] for t in ordered]}


def interpolate(flight, time):
    times = flight["times"]
    if time < times[0] or time > times[-1]:
        raise ValueError(f"Flight does not cover {time} s")
    right = bisect.bisect_left(times, time)
    if times[right] == time:
        return flight["samples"][right]
    left = right - 1
    weight = (time - times[left]) / (times[right] - times[left])
    return tuple(a + (b - a) * weight for a, b in zip(flight["samples"][left], flight["samples"][right]))


def difference(coarse, fine, times):
    rows = []
    for time in times:
        a, b = interpolate(coarse, time), interpolate(fine, time)
        rows.append({"time_s": time,
                     "position_error_m": math.dist(a[:3], b[:3]),
                     "velocity_error_mps": math.dist(a[3:], b[3:])})
    return {"coarse_hz": coarse["hz"], "fine_hz": fine["hz"], "samples": rows,
            "position_rms_m": math.sqrt(sum(r["position_error_m"] ** 2 for r in rows) / len(rows)),
            "velocity_rms_mps": math.sqrt(sum(r["velocity_error_mps"] ** 2 for r in rows) / len(rows))}


def compare(flights):
    flights = sorted(flights, key=lambda f: f["hz"])
    if len(flights) < 3 or any(abs(b["hz"] / a["hz"] - 2) > 1e-5 for a, b in zip(flights, flights[1:])):
        raise ValueError("Provide at least three successively doubled physical rates")
    if max(f["game_hz"] for f in flights) - min(f["game_hz"] for f in flights) > 1e-4:
        raise ValueError("Keep the game cadence identical during physical-step refinement")
    result = {"schema_version": 1, "flights": [f["path"] for f in flights],
              "sample_kind": "Incoming solver origin and COM velocity, interpolated only for comparison",
              "captures_pass": all(f["report"]["success"] and f["report"]["physical_capture"] for f in flights)}
    for name, times in {"ascent": list(range(10, 126, 5)), "recovery": [180, 210, 240, 270, 300, 330, 360, 390]}.items():
        differences = [difference(a, b, times) for a, b in zip(flights, flights[1:])]
        refinement = all(b[key] < a[key] for a, b in zip(differences, differences[1:])
                         for key in ("position_rms_m", "velocity_rms_mps"))
        result[name] = {"differences": differences, "errors_decrease": refinement}
    result["success"] = result["captures_pass"] and result["ascent"]["errors_decrease"] and result["recovery"]["errors_decrease"]
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reports", nargs="+", type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    result = compare([load_flight(path) for path in args.reports])
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({k: v for k, v in result.items() if k not in ("ascent", "recovery")}, indent=2))
    for name in ("ascent", "recovery"):
        print(name, "errors_decrease=", result[name]["errors_decrease"],
              [(d["position_rms_m"], d["velocity_rms_mps"]) for d in result[name]["differences"]])
    return 0 if result["success"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
