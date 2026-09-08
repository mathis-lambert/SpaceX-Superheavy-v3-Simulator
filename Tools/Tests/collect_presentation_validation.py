"""Merge measured presentation reports without dropping the existing flight matrix."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT

import json
import re
from pathlib import Path

ROOT = PROJECT_ROOT
SAVED = ROOT / "Saved/Recovery"
DOCS = ROOT / "Docs"


def read(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def main():
    report = read(DOCS / "RECOVERY_VALIDATION.json")
    rendered = []
    for name in ("VFXFlight01", "VFXFlightFinal", "FrontendCrosswind", "MenuLightingFlight", "EarthFlight", "EarthFlightFinal"):
        path = SAVED / f"{name}.json"
        if path.exists():
            result = read(path)
            result["source_report"] = str(path.relative_to(ROOT)).replace("\\", "/")
            rendered.append(result)
    report["rendered_presentation_flights"] = rendered
    audit = read(SAVED / "InterfaceAudit/result.json")
    report["interface_audit"] = audit
    report["earth_camera_audit"] = read(SAVED / "EarthAudit/result.json")
    report["earth_asset_audit"] = read(SAVED / "earth-asset-audit.json")
    report["vfx_asset_audit"] = read(SAVED / "vfx-asset-audit.json")
    stack = read(SAVED / "niagara-stack-audit.json")
    report["niagara_stack"] = {key: stack[key] for key in ("numErrors", "numWarnings", "numInfos")}
    light_log = (SAVED / "menu-lighting-flight.log").read_text(encoding="utf-8", errors="replace")
    light_samples = [dict(zip(("configured", "expected", "active"), map(int, match)))
                     for match in re.findall(r"RECOVERY_LIGHTS configured=(\d+) expected=(\d+) active=(\d+)", light_log)]
    lights_passed = {s["active"] for s in light_samples} == {0, 3, 13, 33} and all(
        s["configured"] == 33 and s["active"] == s["expected"] for s in light_samples)
    report["engine_light_audit"] = {"success": lights_passed, "state_changes": light_samples}
    # Visual observations are supplied in the worklog, not inferred from exit codes.
    report["visual_final_review"] = "Rendered views inspected: home, graphics, display confirmation, pause, liftoff, trail, engine deck, high-altitude plume and secured catch; final Earth home, 14-view camera picker, 3 km spectator, Starbase panorama, complete globe, 93 km horizon and ascent over Boca Chica. See RECOVERY_WORKLOG.md."
    report["presentation_acceptance_passed"] = len(rendered) == 6 and all(r["success"] for r in rendered) and audit["success"] and lights_passed and report["vfx_asset_audit"]["success"] and stack["numErrors"] == 0 and stack["numWarnings"] == 0 and report["earth_camera_audit"]["success"] and report["earth_asset_audit"]["success"]
    (DOCS / "RECOVERY_VALIDATION.json").write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps({"rendered_flights": len(rendered), "interface_checks": len(audit["checks"]), "passed": report["presentation_acceptance_passed"]}))
    if not report["presentation_acceptance_passed"]:
        raise SystemExit("Presentation validation did not pass.")


if __name__ == "__main__":
    main()
