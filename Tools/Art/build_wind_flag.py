"""Author a subdivided, UV-pinned unbranded cloth flag for wind animation."""
import bpy,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import ART_ROOT
out=ART_ROOT/'Starbase/Activity';out.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
nx,nz=32,16
verts=[(x/nx*2.4,0,z/nz*1.15) for z in range(nz+1) for x in range(nx+1)]
faces=[(z*(nx+1)+x,z*(nx+1)+x+1,(z+1)*(nx+1)+x+1,(z+1)*(nx+1)+x) for z in range(nz) for x in range(nx)]
mesh=bpy.data.meshes.new('WindCloth');mesh.from_pydata(verts,[],faces);mesh.update()
uv=mesh.uv_layers.new()
for face in mesh.polygons:
    for loop in face.loop_indices:
        v=mesh.vertices[mesh.loops[loop].vertex_index].co
        uv.data[loop].uv=(v.x/2.4,v.z/1.15)
obj=bpy.data.objects.new('SM_WindFlag',mesh);bpy.context.collection.objects.link(obj)
obj.select_set(True);bpy.context.view_layer.objects.active=obj
bpy.ops.export_scene.fbx(filepath=str(out/'SM_WindFlag.fbx'),use_selection=True,axis_forward='-Y',axis_up='Z',apply_unit_scale=True,mesh_smooth_type='FACE')
bpy.ops.wm.save_as_mainfile(filepath=str(out/'WindFlag.blend'))
print('WIND_FLAG_SOURCE_READY')
