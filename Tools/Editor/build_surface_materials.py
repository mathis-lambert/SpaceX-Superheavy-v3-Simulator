"""Rebuild industrial and vehicle materials from their canonical definitions."""
import sys, json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from surface_materials import surface_state, industrial_surfaces, vehicle_surfaces, site_lamp, road_surface
from project_paths import SAVED_ROOT

collection=surface_state()
paths=industrial_surfaces(collection)+vehicle_surfaces()+[site_lamp(),road_surface('M_ServiceRoad').get_path_name(),road_surface('M_ServiceShoulder',True).get_path_name()]
(SAVED_ROOT/'visual-renewal-surfaces.json').write_text(json.dumps(
    dict(success=True,collection=collection.get_path_name(),materials=paths),indent=2))
print('VISUAL_RENEWAL_SURFACES_READY',len(paths),flush=True)
