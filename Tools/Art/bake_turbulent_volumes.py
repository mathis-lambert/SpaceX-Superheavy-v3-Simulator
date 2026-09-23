"""Bake an original Mantaflow gas roll as sparse OpenVDB; run Blender headlessly.

This is authored visual flow, not a rocket plume CFD calculation. Keep measured
flight forces independent. The cache records a reproducible flow and obstacle.
"""
import sys, json, hashlib, os
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import ART_ROOT
import bpy

OUT=ART_ROOT/'Effects/TurbulentDeluge';OUT.mkdir(parents=True,exist_ok=True)
os.chdir(OUT) # Mantaflow's wavelet cache belongs with the source artwork.
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene;scene.render.fps=24;scene.frame_start=1;scene.frame_end=64
scene.gravity=(0,0,-9.81)
def box(name,location,dimensions):
    bpy.ops.mesh.primitive_cube_add(size=1,location=location)
    obj=bpy.context.object;obj.name=name;obj.dimensions=dimensions
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    return obj

domain=box('DelugeDomain',(0,0,6),(20,16,12))
mod=domain.modifiers.new('Mantaflow','FLUID');mod.fluid_type='DOMAIN'
bpy.context.view_layer.update();settings=mod.domain_settings
settings.domain_type='GAS';settings.resolution_max=64;settings.cache_frame_start=1;settings.cache_frame_end=64
settings.cache_directory=str(OUT/'Cache');settings.cache_type='ALL';settings.cache_data_format='OPENVDB'
settings.use_noise=True;settings.noise_scale=2;settings.cache_noise_format='OPENVDB'
settings.vorticity=1.4;settings.alpha=-.12;settings.beta=.45;settings.time_scale=.8
settings.use_adaptive_domain=False;settings.timesteps_max=6;settings.timesteps_min=1
settings.use_dissolve_smoke=True;settings.dissolve_speed=70
flow=box('WarmWaterVapor',(-6,0,1.6),(2,3,2.2))
fm=flow.modifiers.new('VaporFlow','FLUID');fm.fluid_type='FLOW';bpy.context.view_layer.update()
fs=fm.flow_settings;fs.flow_type='SMOKE';fs.flow_behavior='INFLOW';fs.density=1;fs.temperature=.7
fs.use_initial_velocity=True;fs.velocity_coord=(6,0,1.5);fs.surface_distance=1.5
for name,location,dimensions in [('Ground',(0,0,-.4),(20,16,.8)),('Deflector',(1,0,1.7),(1.4,4,3.4))]:
    obj=box(name,location,dimensions);eff=obj.modifiers.new('SolidBoundary','FLUID');eff.fluid_type='EFFECTOR'
    bpy.context.view_layer.update();eff.effector_settings.surface_distance=.001
bpy.context.view_layer.objects.active=domain
bpy.ops.object.select_all(action='DESELECT');domain.select_set(True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'TurbulentDeluge.blend'))
result=bpy.ops.fluid.bake_all();assert 'FINISHED' in result,result
files=sorted((OUT/'Cache').rglob('*.vdb'));assert files,'No VDB simulation cache was written'
manifest=dict(author='Original simulator volumetric flow',solver='Blender Mantaflow',blender=bpy.app.version_string,
    units='metres',domain_m=[20,16,12],base_resolution=64,noise_scale=2,frames=64,fps=24,time_scale=.8,
    description='Warm inflow rolls around a low solid deflector; offline visual simulation, not vehicle CFD.',
    files=[dict(file=str(p.relative_to(OUT)),bytes=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in files])
(OUT/'sources.json').write_text(json.dumps(manifest,indent=2));print('TURBULENT_VDB_READY',len(files),flush=True)
