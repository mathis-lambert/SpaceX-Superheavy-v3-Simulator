"""Analyze Unreal timings, slow frames, phase coverage and actual RHI memory.

GPU passes may overlap; they are not an additive budget. Missing counters are
unavailable, never zero. Original CSV files retain all startup measurements.
"""
import argparse
import csv
import json
import math
import statistics
from pathlib import Path

PHASES = ('Ready', 'Countdown', 'Ascent', 'Separation', 'Boostback', 'Coast',
          'Entry', 'LandingBurn', 'Capture', 'Captured', 'Aborted')
TIMINGS = {'FrameTime', 'GameThreadTime', 'RenderThreadTime', 'GPUTime',
           'RHIThreadTime', 'InputLatencyTime'}
MEMORY = ('GPUMem/LocalUsedMB', 'GPUMem/LocalBudgetMB', 'GPUMem/SystemUsedMB',
          'TextureStreaming/StreamingPool', 'TextureStreaming/WantedMips',
          'TextureStreaming/CachedMips', 'TextureStreaming/NonStreamingMips',
          'TextureStreaming/PendingStreamInData', 'RenderTargetPool/PeakUsedMB',
          'Shaders/ShaderMemoryMB')


def number(value):
    try:
        result = float(value)
        return result if math.isfinite(result) else None
    except (ValueError, TypeError):
        return None


def percentile(ordered, fraction):
    """Linear interpolation, including phases containing a single frame."""
    index = (len(ordered) - 1) * fraction
    low, high = math.floor(index), math.ceil(index)
    return ordered[low] + (ordered[high] - ordered[low]) * (index - low)


def distribution(values, unit):
    ordered = sorted(values)
    metrics = {'mean': statistics.fmean(values), 'p50': percentile(ordered, .5),
               'p95': percentile(ordered, .95), 'p99': percentile(ordered, .99),
               'p999': percentile(ordered, .999), 'max': ordered[-1]}
    return {'samples': len(values), **{f'{k}_{unit}': round(v, 4) for k, v in metrics.items()}}


def summarize(rows):
    timings, memory, misses, unavailable = {}, {}, {}, []
    for key in rows[0]:
        values = [r[key] for r in rows if r.get(key) is not None]
        if not values:
            continue
        if key in TIMINGS or key in MEMORY or key.startswith(('GPU/', 'PSO/')):
            values = [value for value in values if value >= 0]
            if not values:
                unavailable.append(key)
                continue
        if key in TIMINGS or key.startswith('GPU/'):
            timings[key] = distribution(values, 'ms')
        if key in MEMORY:
            memory[key] = distribution(values, 'mb')
        if key.startswith('PSO/'):
            misses[key] = {'sum': sum(values), 'peak_frame': max(values)}
    durations = [r['FrameTime'] for r in rows]
    worst = sorted(durations, reverse=True)[:max(1, math.ceil(len(rows) * .01))]
    stalls = {}
    for threshold in (1000 / 60, 1000 / 30, 50, 100):
        current = longest = count = 0
        for value in durations:
            if value > threshold:
                current += 1
                count += 1
                longest = max(longest, current)
            else:
                current = 0
        stalls[f'over_{threshold:.3f}_ms'] = {
            'frames': count, 'percent': round(count * 100 / len(rows), 3),
            'longest_consecutive_frames': longest}
    return {'frames_measured': len(rows),
            'fps_from_mean': round(1000 / statistics.fmean(durations), 2),
            'fps_from_worst_one_percent_mean': round(1000 / statistics.fmean(worst), 2),
            'timings': dict(sorted(timings.items(), key=lambda item: -item[1]['mean_ms'])),
            'stalls': stalls, 'memory': memory, 'pso_counters': misses,
            'unavailable_counters': unavailable}


def analyze(path, warmup_frames=300):
    if warmup_frames < 0:
        raise ValueError('Warm-up frame count must not be negative')
    rows, rejected = [], 0
    with Path(path).open(encoding='utf-8-sig', newline='') as stream:
        reader = csv.DictReader(stream)
        if not reader.fieldnames or 'FrameTime' not in reader.fieldnames:
            raise ValueError('Not an Unreal performance CSV: FrameTime is missing')
        for row in reader:
            duration = number(row.get('FrameTime'))
            if duration is None or duration <= 0:
                rejected += 1
                continue
            rows.append({key: number(value) for key, value in row.items() if key is not None})
    measured = rows[warmup_frames:]
    if not measured:
        raise ValueError('Capture is shorter than the warm-up exclusion')
    phase_rows = {}
    for row in measured:
        phase = row.get('Recovery/Phase')
        if phase is not None:
            index = int(phase)
            label = PHASES[index] if 0 <= index < len(PHASES) else f'Unknown_{index}'
            phase_rows.setdefault(label, []).append(row)
    result = {'schema_version': 2, 'capture': str(Path(path).resolve()),
              'valid_frames': len(rows), 'metadata_or_invalid_rows': rejected,
              'warmup_frames_excluded': warmup_frames, **summarize(measured),
              'phases': {label: summarize(group) for label, group in phase_rows.items()},
              'notes': ['GPU pass timings can overlap and are not an additive budget.',
                        'RHI LocalUsedMB is observed local memory; a streaming pool is a budget.',
                        'Missing counters are unavailable. A benchmark is not a visual-quality test.']}
    if warmup_frames:
        result['startup'] = summarize(rows[:warmup_frames])
    result['coverage'] = {}
    for key in ('Recovery/MissionTimeS', 'Recovery/AltitudeM'):
        values = [r[key] for r in measured if r.get(key) is not None]
        if values:
            result['coverage'][key] = {'min': min(values), 'max': max(values)}
    result['slowest_frames'] = [
        {'valid_frame_index': index + warmup_frames, 'frame_ms': row['FrameTime'],
         'mission_time_s': row.get('Recovery/MissionTimeS'),
         'phase': row.get('Recovery/Phase'), 'altitude_m': row.get('Recovery/AltitudeM')}
        for index, row in sorted(enumerate(measured), key=lambda pair: -pair[1]['FrameTime'])[:20]]
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    parser.add_argument('--warmup-frames', type=int, default=300)
    args = parser.parse_args()
    result = analyze(args.capture, args.warmup_frames)
    target = args.capture.with_suffix('.summary.json')
    target.write_text(json.dumps(result, indent=2, allow_nan=False), encoding='utf-8')
    print(json.dumps({'summary': str(target), 'frames': result['frames_measured'],
                     'fps': result['fps_from_mean'], 'frame_time': result['timings']['FrameTime'],
                     'phases': list(result['phases']), 'memory': result['memory'],
                     'top_passes': dict(list(result['timings'].items())[:12])}, indent=2))


if __name__ == '__main__':
    main()
