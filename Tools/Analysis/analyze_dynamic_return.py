"""Gate faster physical returns against the previous alpha's flight reports."""
import argparse
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('reports', type=Path, nargs='+')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--baseline-prefix', type=Path, required=True, help='Prefix of explicitly selected baseline reports (outside the source tree)')
    args = parser.parse_args()
    results = []
    for path in args.reports:
        report = json.loads(path.read_text(encoding='utf-8-sig'))
        baseline_path = Path(f"{args.baseline_prefix}_{report['scenario']}_60.json")
        baseline = json.loads(baseline_path.read_text(encoding='utf-8-sig'))
        checks = {
            'physical_support': report['success'] and report['solver_support_mask'] == 3 and report['contact_engine_shutdown'],
            'clear_structure': report['structural_contacts'] == 0,
            'front_ingress': report['front_ingress_verified'] and report['front_min_mast_clearance_m'] >= 0 and report['front_min_corridor_margin_m'] >= 0,
            'unpowered_coast': not report['unpowered_thrust_violation'] and report['unpowered_seconds'] > baseline['unpowered_seconds'],
            'shorter_burn': report['landing_burn_seconds'] < min(45, baseline['landing_burn_seconds'] * .65),
            'soft_first_contact': report['first_contact_time_s'] > report['landing_ignition_time_s'] and 0 <= report['first_contact_speed_mps'] < 1.5,
            'no_slow_approach': report['low_slow_approach_seconds'] < 8,
            'bounded_hardware': report['peak_engine_force_ratio'] <= 1.000001 and report['peak_gimbal_deg'] <= 8.001,
            'propellant_ledger': abs(report['propellant_balance_error_kg']) < .1,
            'passive_support': report['restraint_drift_m'] < .25 and report['final_speed_mps'] < .1,
        }
        results.append({
            'report': path.as_posix(), 'baseline': baseline_path.as_posix(), 'checks': checks,
            'success': all(checks.values()), 'scenario': report['scenario'],
            'ignition_altitude_m': report['landing_ignition_altitude_m'],
            'ignition_speed_mps': report['landing_ignition_speed_mps'],
            'burn_s': report['landing_burn_seconds'], 'baseline_burn_s': baseline['landing_burn_seconds'],
            'additional_unpowered_s': report['unpowered_seconds'] - baseline['unpowered_seconds'],
            'first_contact_speed_mps': report['first_contact_speed_mps'],
            'low_slow_approach_s': report['low_slow_approach_seconds'],
        })
    output = {'success': bool(results) and all(r['success'] for r in results), 'flights': results}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(output, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(output, indent=2))
    raise SystemExit(0 if output['success'] else 1)


if __name__ == '__main__':
    main()
