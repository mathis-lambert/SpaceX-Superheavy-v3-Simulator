"""Bake surveyed, curved terrain with exact nested-grid edge stitching."""
import sys,json,math
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import ART_ROOT
from earth_geography import heights,R,LAT,LON,E,N,U,LIDAR,LIDAR_WEIGHT
import bpy,numpy as np

assert LIDAR is not None and LIDAR_WEIGHT is not None,'Prepare the LiDAR and coverage first'
out=ART_ROOT/'Earth/LidarCoast/Meshes';out.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
reports=[]
def positions(x,y):
    la=np.radians(LAT)+y/R;lo=np.radians(LON)+x/(R*math.cos(math.radians(LAT)))
    h=heights(x,y)
    p=np.stack([np.cos(la)*np.cos(lo),np.cos(la)*np.sin(lo),np.sin(la)],axis=-1)*(R+h)[...,None]
    return np.stack([p@E,p@N,p@U-R],axis=-1)

for j in range(4):
    for i in range(4):
        fine=i in (1,2) and j in (1,2);n=768 if fine else 256
        u,v=np.meshgrid(np.linspace(0,1,n+1),np.linspace(0,1,n+1));x=-6000+3000*(i+u);y=-6000+3000*(j+v)
        p=positions(x,y)
        # The central grids share every third vertex with their coarse neighbours.
        if fine:
            for axis,index,is_boundary in [(0,0,i==1),(0,-1,i==2),(1,0,j==1),(1,-1,j==2)]:
                if not is_boundary:continue
                edge=p[:,index,:] if axis==0 else p[index,:,:]
                coarse=edge[::3].copy();t=np.arange(n+1)/3;k=np.minimum(t.astype(int),255);f=t-k
                edge[:]=coarse[k]*(1-f[:,None])+coarse[k+1]*f[:,None]
        vertices=p.reshape(-1,3);a=(np.arange(n)[:,None]*(n+1)+np.arange(n)[None,:]).ravel()
        faces=np.stack([a,a+1,a+n+2,a+n+1],axis=1)
        name=f'SM_BocaChica_{i}_{j}';data=bpy.data.meshes.new(name)
        data.vertices.add(len(vertices));data.vertices.foreach_set('co',vertices.astype(np.float32).ravel())
        data.loops.add(faces.size);data.loops.foreach_set('vertex_index',faces.ravel())
        data.polygons.add(len(faces));data.polygons.foreach_set('loop_start',np.arange(len(faces))*4);data.polygons.foreach_set('loop_total',np.full(len(faces),4))
        data.polygons.foreach_set('use_smooth',np.ones(len(faces),dtype=bool));data.update()
        uv=np.stack([u,v],axis=-1).reshape(-1,2);layer=data.uv_layers.new(name='UVMap');layer.data.foreach_set('uv',uv[faces.ravel()].astype(np.float32).ravel())
        obj=bpy.data.objects.new(name,data);bpy.context.collection.objects.link(obj);bpy.context.view_layer.objects.active=obj;obj.select_set(True)
        bpy.ops.export_scene.fbx(filepath=str(out/(name+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE')
        reports.append(dict(name=name,vertices=len(vertices),quads=len(faces),spacing_m=3000/n,source='USGS LiDAR with 3DEP fallback',z_range_m=[float(p[:,:,2].min()),float(p[:,:,2].max())]))
        bpy.data.objects.remove(obj,do_unlink=True);bpy.data.meshes.remove(data)
        print('LIDAR_TILE_READY',name,flush=True)
(out/'geometry.json').write_text(json.dumps(dict(success=True,tiles=reports,datum_m=-4.5,central_edge_ratio=3),indent=2))
print('LIDAR_TERRAIN_READY',flush=True)
