"""Read exported FBXs back into Blender and measure actual neighbouring edges."""
import sys,json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import ART_ROOT,SAVED_ROOT
import bpy,numpy as np
root=ART_ROOT/'Earth/LidarCoast/Meshes';edges={};counts={}
bpy.ops.wm.read_factory_settings(use_empty=True)
for j in range(4):
    for i in range(4):
        bpy.ops.import_scene.fbx(filepath=str(root/f'SM_BocaChica_{i}_{j}.fbx'))
        obj=next(o for o in bpy.context.selected_objects if o.type=='MESH');data=obj.data
        counts[f'{i}_{j}']=len(data.vertices)
        samples={key:{} for key in ('left','right','bottom','top')}
        for poly in data.polygons:
            for li in poly.loop_indices:
                uv=data.uv_layers.active.data[li].uv
                hits=[]
                if abs(uv.x)<1e-7:hits.append(('left',uv.y))
                if abs(uv.x-1)<1e-7:hits.append(('right',uv.y))
                if abs(uv.y)<1e-7:hits.append(('bottom',uv.x))
                if abs(uv.y-1)<1e-7:hits.append(('top',uv.x))
                if hits:
                    p=tuple(obj.matrix_world@data.vertices[data.loops[li].vertex_index].co)
                    for key,t in hits:samples[key][round(t,7)]=p
        edges[i,j]={key:sorted(value.items()) for key,value in samples.items()}
        bpy.data.objects.remove(obj,do_unlink=True);bpy.data.meshes.remove(data)
errors=[]
def compare(a,b):
    t=np.array(sorted(set(p[0] for p in a)|set(p[0] for p in b)))
    def interpolate(edge):
        uv=np.array([s[0] for s in edge]);xyz=np.array([s[1] for s in edge])
        return np.stack([np.interp(t,uv,xyz[:,k]) for k in range(3)],axis=-1)
    return float(np.linalg.norm(interpolate(a)-interpolate(b),axis=1).max())
for j in range(4):
    for i in range(4):
        if i<3:errors.append(dict(a=f'{i}_{j}/right',b=f'{i+1}_{j}/left',gap_m=compare(edges[i,j]['right'],edges[i+1,j]['left'])))
        if j<3:errors.append(dict(a=f'{i}_{j}/top',b=f'{i}_{j+1}/bottom',gap_m=compare(edges[i,j]['top'],edges[i,j+1]['bottom'])))
report=dict(success=max(e['gap_m'] for e in errors)<.003,maximum_gap_m=max(e['gap_m'] for e in errors),edges=errors,vertex_counts=counts)
(SAVED_ROOT/'lidar-mesh-seams.json').write_text(json.dumps(report,indent=2))
assert report['success'],report
print('LIDAR_MESH_SEAMS_PASS',report['maximum_gap_m'],flush=True)
