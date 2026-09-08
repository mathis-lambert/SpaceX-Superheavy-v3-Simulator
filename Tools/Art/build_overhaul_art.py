"""Author a detailed 54 m ship and attitude-control housings in Blender.
Dimensions and nozzle arrangement are visual estimates; original booster untouched.
"""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT

import bpy,math
from pathlib import Path
from mathutils import Vector
OUT=ART_ROOT/'VehicleDetails'
OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
materials=[]
for name,color,metal,rough in [('Steel',(.48,.51,.54,1),1,.28),('Tiles',(.018,.02,.024,1),0,.8),('Mechanisms',(.07,.085,.10,1),.8,.45),('Nozzles',(.17,.20,.22,1),.85,.34)]:
    m=bpy.data.materials.new(name);m.diffuse_color=color;m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=color;p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=rough;materials.append(m)
parts=[]
def mesh(name,v,f,uv=None,mi=None):
    me=bpy.data.meshes.new(name);me.from_pydata(v,[],f);me.update()
    ob=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(ob)
    for m in materials:me.materials.append(m)
    for i,p in enumerate(me.polygons):p.use_smooth=True;p.material_index=mi[i] if mi else 0
    if uv:
        layer=me.uv_layers.new(name='UVMap')
        for p in me.polygons:
            for j in p.loop_indices:layer.data[j].uv=uv[me.loops[j].vertex_index]
    parts.append(ob);return ob
def lathe(name,rings,material=0,n=96,origin=(0,0,0)):
    v=[];uv=[];f=[];indices=[]
    for k,(z,r) in enumerate(rings):
        for i in range(n+1):
            a=i*math.tau/n;v.append((origin[0]+r*math.cos(a),origin[1]+r*math.sin(a),origin[2]+z));uv.append((i/n,z/54))
    for k in range(len(rings)-1):
        for i in range(n):
            a=k*(n+1)+i;f.append((a,a+1,a+n+2,a+n+1));indices.append(1 if material==-1 and math.sin((i+.5)*math.tau/n)<-.08 else max(0,material))
    return mesh(name,v,f,uv,indices)
def box(name,loc,size,mat=0,rotation=0):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc);ob=bpy.context.object;ob.name=name;ob.dimensions=size;ob.rotation_euler[2]=rotation
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    for m in materials:ob.data.materials.append(m)
    for p in ob.data.polygons:p.material_index=mat
    bevel=ob.modifiers.new('Machined edges','BEVEL');bevel.width=.035;bevel.segments=2
    bpy.context.view_layer.objects.active=ob;bpy.ops.object.modifier_apply(modifier=bevel.name);parts.append(ob);return ob
def export(name):
    bpy.ops.object.select_all(action='DESELECT')
    for p in parts:p.select_set(True)
    bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();ob=bpy.context.object;ob.name=name
    bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE')
    return ob
rings=[]
for i in range(181):
    z=54*i/180
    radius=4.5 if z<38.5 else max(.015,4.5*math.sqrt(max(0,1-((z-38.5)/15.5)**1.48)))
    rings.append((z,radius))
lathe('Pressure hull',rings,-1,192)
for z in [i*1.82 for i in range(1,22)]:lathe('Weld ring',[(z-.012,4.501),(z,4.516),(z+.012,4.501)],0,192)
lathe('Aft skirt lip',[(0,4.5),(.04,4.55),(.25,4.55),(.29,4.5)],0,192)
lathe('Engine bay inner wall',[(0,4.40),(1.8,4.4)],2,128)
# Four articulated flaps: thick tapered profiles on the heat-shield shoulders.
for angle in [-145,-35]:
    a=math.radians(angle);radial=Vector((math.cos(a),math.sin(a),0));tangent=Vector((-math.sin(a),math.cos(a),0))
    for z,h,span in [(2.6,12.5,4.2),(39,7.7,2.6)]:
        root=4.35 if z<38 else 4.0
        corners=[(root,z),(root,z+h),(root+span*.75,z+h*.77),(root+span,z+h*.20)]
        vertices=[]
        for thickness in [-.19,.19]:
            vertices.extend(tuple(radial*x+tangent*thickness+Vector((0,0,zz))) for x,zz in corners)
        faces=[(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)]
        ob=mesh('Articulated flap',vertices,faces,mi=[1,1,0,0,0,0])
        for p in ob.data.polygons:p.use_smooth=False
        # The cylinder is the exposed hinge, not a flat silhouette on the hull.
        lathe('Flap hinge',[(z,.16),(z+h,.16)],2,20,tuple(radial*(root-.12)))
        for dz in [.6,h*.5,h-.6]:box('Hinge bracket',tuple(radial*(root-.14)+Vector((0,0,z+dz))),(.44,.52,.3),2,a)
for i in range(6):
    a=i*math.tau/6;vac=i%2==0;r=2.85 if vac else 1.25
    x,y=r*math.cos(a),r*math.sin(a);exit_r=1.05 if vac else .55;length=2.1 if vac else 1.35
    lathe('Raptor vacuum' if vac else 'Raptor sea level',[(-.05,exit_r),(.08,exit_r*1.01),(length*.5,exit_r*.54),(length,.26),(length+.65,.29)],3,48,(x,y,0))
    lathe('Bell interior',[(-.04,exit_r-.035),(length*.5,exit_r*.5),(length,.225)],2,48,(x,y,0))
for i in range(12):
    a=i*math.tau/12;box('Skirt reinforcement',(4.2*math.cos(a),4.2*math.sin(a),1.2),(.11,.22,2.3),2,a)
for z in [14,24,34]:
    for a in [math.radians(35),math.radians(145)]:box('Service panel',(4.47*math.cos(a),4.47*math.sin(a),z),(.035,1.0,.65),0,a)
ship=export('SM_StarshipDetailed');parts=[];ship.hide_set(True)
# Common housing aligned local +X outward. Four ports for estimated attitude jets.
box('Thruster housing',(0,0,0),(.45,1.15,1.3),0)
box('Thermal base',(-.24,0,0),(.08,1.35,1.5),2)
for y,z in [(-.32,0),(.32,0),(0,-.4),(0,.4)]:
    nozzle=lathe('Gas nozzle',[(0,.18),(.1,.185),(.3,.09),(.45,.07)],3,32)
    nozzle.rotation_euler[1]=-math.pi/2;nozzle.location=(.5,y,z)
pod=export('SM_RCSBlock')
ship.hide_set(False)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'StarshipAndRCS.blend'))
print('OVERHAUL_ART_READY',len(ship.data.vertices),'ship vertices',len(pod.data.vertices),'pod vertices')
