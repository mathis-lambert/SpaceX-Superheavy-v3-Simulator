"""Blender source models for generic, unbranded Starbase service equipment."""
import sys,math
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import ART_ROOT
import bpy
from mathutils import Vector

root=ART_ROOT/'Starbase/Service';root.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
materials={}
for name,col in [('Paint',(.63,.67,.7)),('Rubber',(.014,.017,.021)),('Glass',(.035,.065,.08)),('Metal',(.24,.27,.29)),('Lamp',(.95,.74,.3))]:
    m=bpy.data.materials.new(name);m.diffuse_color=(*col,1);materials[name]=m
parts=[]
def finish(obj,mat,bevel=0):
    obj.data.materials.append(materials[mat]);parts.append(obj)
    if bevel:
        bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
        mod=obj.modifiers.new('Manufactured edge','BEVEL');mod.width=bevel;mod.segments=2
        bpy.ops.object.modifier_apply(modifier=mod.name)
    return obj
def box(pos,size,mat='Paint',bevel=.015):
    bpy.ops.mesh.primitive_cube_add(size=1,location=pos);o=bpy.context.object;o.scale=size
    return finish(o,mat,bevel)
def axle(pos,radius,width,mat):
    bpy.ops.mesh.primitive_cylinder_add(vertices=32,radius=radius,depth=width,location=pos,rotation=(math.pi/2,0,0));return finish(bpy.context.object,mat,.012)
def panel(name,verts,faces,mat,bevel=0):
    mesh=bpy.data.meshes.new(name);mesh.from_pydata(verts,[],faces);mesh.update()
    obj=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(obj)
    bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
    return finish(obj,mat,bevel)
def wheel(x,y):
    axle((x,y,.43),.43,.30,'Rubber');axle((x,y+math.copysign(.17,y),.43),.26,.035,'Metal')
    axle((x,y+math.copysign(.20,y),.43),.11,.04,'Rubber')
    for n in range(6):
        a=n*math.tau/6;axle((x+math.sin(a)*.17,y+math.copysign(.20,y),.43+math.cos(a)*.17),.021,.02,'Metal')
    for n in range(24):
        a=n*math.tau/24;o=box((x+math.sin(a)*.426,y,.43+math.cos(a)*.426),(.075,.31,.025),'Rubber',.003);o.rotation_euler.y=a
def export(name):
    bpy.ops.object.select_all(action='DESELECT')
    for o in parts:o.select_set(True)
    bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();obj=bpy.context.object;obj.name=name
    bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.export_scene.fbx(filepath=str(root/(name+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE')
    print('SERVICE_MODEL',name,len(obj.data.polygons),flush=True);parts.clear()

box((0,0,.6),(5.25,1.8,.24),'Rubber');body=box((0,0,.92),(5.4,2.05,.60),bevel=.075)
for x in (-1.77,1.71):
    bpy.ops.mesh.primitive_cylinder_add(vertices=48,radius=.54,depth=2.5,location=(x,0,.43),rotation=(math.pi/2,0,0));cutter=bpy.context.object
    bpy.context.view_layer.objects.active=body;mod=body.modifiers.new('Wheel clearance','BOOLEAN');mod.operation='DIFFERENCE';mod.object=cutter
    bpy.ops.object.modifier_apply(modifier=mod.name);bpy.data.objects.remove(cutter,do_unlink=True)
box((1.65,0,1.24),(1.9,2,.18),bevel=.055)
panel('Cab',[(-.85,-.955,1.175),(1.05,-.955,1.175),(1.05,.955,1.175),(-.85,.955,1.175),(-.82,-.90,2.405),(.70,-.90,2.405),(.70,.90,2.405),(-.82,.90,2.405)],[(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)],'Paint',.045)
panel('Windshield',[(.735,-.79,2.29),(.735,.79,2.29),(.969,.84,1.51),(.969,-.84,1.51)],[(0,1,2,3)],'Glass')
box((-.86,0,1.91),(.03,1.68,.60),'Glass',.02)
for side in (-1,1):
    panel('Side glazing',[(-.71,side*.923,2.28),(.65,side*.923,2.28),(.89,side*.958,1.53),(-.74,side*.958,1.53)],[(0,1,2,3)] if side<0 else [(3,2,1,0)],'Glass')
    box((.05,side*.99,1.37),(1.82,.022,.015),'Rubber',.002)
    box((-.48,side*1.01,1.50),(.18,.035,.04),'Metal',.01)
    box((.82,side*1.17,1.74),(.24,.20,.18),'Rubber',.03)
    box((-1.70,side*.985,1.39),(1.8,.11,.40),bevel=.025)
    box((.02,side*1.15,.60),(2.0,.20,.10),'Metal',.02)
box((-1.76,0,1.105),(1.80,1.82,.08),'Rubber');box((-2.65,0,1.36),(.10,1.86,.38))
for x in (-1.77,1.71):
    for y in (-1.02,1.02):wheel(x,y)
for side in (-1,1):box((2.72,side*.74,1.20),(.035,.43,.22),'Lamp',.012)
box((2.73,0,.98),(.04,.92,.27),'Rubber');box((2.76,0,.71),(.16,2.12,.19),'Metal',.03)
for n in range(7):box((2.76,-.40+n*.133,.98),(.04,.025,.23),'Metal',.002)
box((.12,0,2.43),(.67,.25,.09),'Lamp',.015)
export('SM_ServicePickup')

box((0,0,.6),(3.6,1.55,.20),'Metal');box((0,0,1.45),(3.1,1.45,1.5),'Paint',.08)
for side in (-1,1):
    for n in range(18):box((-.96+n*.07,side*.737,1.48),(.035,.022,.72),'Rubber',.001)
    box((.78,side*.74,1.5),(.85,.018,1.13),'Metal',.015)
    box((.82,side*.76,1.79),(.5,.03,.26),'Glass',.01)
    axle((-.60,side*.88,.43),.43,.27,'Rubber');axle((-.60,side*1.03,.43),.23,.035,'Metal')
box((2.12,0,.57),(1.15,.17,.16),'Metal');box((2.65,0,.58),(.24,.23,.16),'Rubber')
box((2.10,0,.31),(.09,.09,.57),'Metal');box((2.10,0,.045),(.34,.26,.07),'Metal')
box((.8,.30,2.35),(.11,.11,.47),'Rubber');box((.8,.30,2.58),(.18,.16,.04),'Metal')
export('SM_TowableGenerator')
# Keep the source library readable while preserving each exported local origin.
bpy.data.objects['SM_ServicePickup'].location.y=-2
bpy.data.objects['SM_TowableGenerator'].location.y=2
bpy.ops.wm.save_as_mainfile(filepath=str(root/'SiteServiceProps.blend'))
print('SITE_SERVICE_PROPS_READY')
