"""Original procedural tower detailing, authored in metres. Run with Blender --background."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT

import bpy, math, os
from mathutils import Vector, Matrix

OUT = str(ART_ROOT/'Flight')
os.makedirs(OUT, exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)

def material(name, color, metal=0, rough=0.5):
    m=bpy.data.materials.new(name); m.diffuse_color=(*color,1); m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF'); p.inputs['Base Color'].default_value=(*color,1)
    p.inputs['Metallic'].default_value=metal; p.inputs['Roughness'].default_value=rough
    return m
steel=material('Graphite steel',(0.055,0.08,0.1),0.8,0.32)
panel=material('Ceramic cladding',(0.48,0.57,0.61),0.6,0.4)
orange=material('Safety amber',(0.95,0.24,0.035),0.35,0.4)

parts=[]
def box(name,loc,size,mat):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc)
    o=bpy.context.object; o.name=name; o.dimensions=size
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    o.data.materials.append(mat); parts.append(o); return o
def beam(name,a,b,w,mat):
    a,b=Vector(a),Vector(b); o=box(name,(a+b)/2,(w,w,(b-a).length),mat)
    o.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler(); return o
def export(name,normalize=None):
    bpy.ops.object.select_all(action='DESELECT')
    for o in parts:o.select_set(True)
    bpy.context.view_layer.objects.active=parts[0]; bpy.ops.object.join()
    o=bpy.context.object; o.name=name
    bpy.context.scene.cursor.location=(0,0,0); bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    if normalize:o.data.transform(Matrix.Diagonal((*normalize,1)))
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT,name+'.fbx'),use_selection=True,object_types={'MESH'},apply_unit_scale=True,global_scale=1,axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE')
    return o

box('Elevator housing',(-1,0,51),(4,5,102),steel)
for floor in range(10):
    z=4+floor*10
    box('Service deck',(-1,0,z),(11,11,0.2),steel)
    for y in [-5,5]:
        beam('Rail',(-5,y,z+1.1),(5,y,z+1.1),0.07,panel)
        for x in [-5,-3,-1,1,3,5]:beam('Post',(x,y,z),(x,y,z+1.1),0.07,panel)
    box('Equipment panel',(-3.06,0,z+4),(0.12,4,6.5),panel)
    box('Deck edge',(-1,-5.55,z),(10,0.13,0.22),orange)
    for step in range(28):
        box('Stair',(3.5,-4+step*0.28,z+step*0.35),(1.5,0.31,0.1),panel)
    beam('Stair rail',(4.4,-4,z+1),(4.4,3.85,z+10.8),0.06,orange)
box('Roof',(0,0,104),(14,14,0.4),steel)
for y in [-5,5]:beam('Lightning mast',(0,y,104),(0,y,114),0.13,panel)
detail=export('SM_TowerServiceCore')
parts=[]
for y in [-0.55,0.55]:
    for z in [-0.85,0.85]:beam('Arm chord',(-13,y,z),(13,y,z),0.18,steel)
    for i in range(13):
        x=-13+i*2
        beam('Arm diagonal',(x,y,-0.85),(x+2,y,0.85),0.13,panel)
        beam('Arm diagonal',(x,y,0.85),(x+2,y,-0.85),0.13,panel)
# The load-bearing rail and hydraulic struts are independent moving components.
# Baking the rail into the truss would leave a second, motionless rail underneath.
box('Carriage attachment',(-12,0,0),(2,1.4,2.2),steel)
arm=export('SM_CaptureArm',(1/26,1/1.1,1/1.7))

# A softly sloping, irregular coastal shelf instead of a rectangular land block.
verts=[(0,0,2)]; faces=[]; count=128; rings=20
for ring in range(1,rings+1):
    r=ring/rings
    for i in range(count):
        a=i*math.tau/count; coast=1+0.04*math.sin(3*a)+0.025*math.sin(7*a+1)
        z=2-max(0,(r-0.78)/0.22)**1.4*9
        verts.append((550*r*math.cos(a)*coast,350*r*math.sin(a)*coast,z))
for i in range(count):faces.append((0,1+i,1+(i+1)%count))
for ring in range(rings-1):
    for i in range(count):
        a=1+ring*count+i; b=1+ring*count+(i+1)%count
        faces.append((a,a+count,b+count,b))
mesh=bpy.data.meshes.new('CoastalShelf');mesh.from_pydata(verts,[],faces);mesh.update()
terrain=bpy.data.objects.new('SM_CoastalTerrain',mesh);bpy.context.collection.objects.link(terrain)
for polygon in mesh.polygons:polygon.use_smooth=True
parts=[terrain]; export('SM_CoastalTerrain')
detail.hide_set(False)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT,'RecoveryTower.blend'))
print('RECOVERY_MESHES_READY',OUT)
