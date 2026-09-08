"""Blender: full Earth globe, Gulf regional shell and 16 USGS elevation tiles."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT

import bpy,sys,math,json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from earth_geography import *
OUTPUT=ROOT/'Continuity/Meshes';OUTPUT.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
def export(name,verts,faces,uvs):
    mesh=bpy.data.meshes.new(name);mesh.from_pydata(verts,[],faces);mesh.update()
    obj=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(obj)
    layer=mesh.uv_layers.new(name='UVMap')
    for poly in mesh.polygons:
        poly.use_smooth=True
        for k in poly.loop_indices:layer.data[k].uv=uvs[mesh.loops[k].vertex_index]
    bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
    bpy.ops.export_scene.fbx(filepath=str(OUTPUT/(name+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE')
    print('EXPORTED',name,flush=True)
def grid(name,n,k,fn):
    verts=[];uv=[];faces=[]
    for j in range(k+1):
        for i in range(n+1):
            p,t=fn(i/n,j/k);verts.append(p);uv.append(t)
    for j in range(k):
        for i in range(n):
            a=j*(n+1)+i;faces.append((a,a+1,a+n+2,a+n+1))
    export(name,verts,faces,uv)
# Northward V and Blender FBX UV conversion map equirectangular images correctly.
grid('SM_EarthGlobe',1024,512,lambda u,v:(point(-89.9998+179.9996*v,-180+360*u,-25),(u,v)))
grid('SM_GulfRegion',384,384,lambda u,v:(point(LAT-12+24*v,LON-12+24*u,-10),(u,v)))
grid('SM_BocaRegion',256,256,lambda u,v:(point(*geo(-60000+120000*u,-60000+120000*v),-7),(u,v)))
for j in range(4):
    for i in range(4):
        x0,y0=-6000+i*3000,-6000+j*3000
        def tile(u,v):
            x,y=x0+3000*u,y0+3000*v
            return point(*geo(x,y),height(x,y)),(u,v)
        grid(f'SM_BocaChica_{i}_{j}',256,256,tile)
bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT/'StarbaseEarth.blend'))
print('EARTH_ART_READY')
