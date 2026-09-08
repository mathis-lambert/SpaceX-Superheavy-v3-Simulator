"""Procedural supporting art in metres; preserves the user's source booster."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT

import bpy, math, os
from mathutils import Vector
OUT=str(ART_ROOT/'Flight')
os.makedirs(OUT,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
def export(name,verts,faces,uvs=None):
    mesh=bpy.data.meshes.new(name);mesh.from_pydata(verts,[],faces);mesh.update()
    obj=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(obj)
    for p in mesh.polygons:p.use_smooth=True
    if uvs:
        layer=mesh.uv_layers.new(name='UVMap')
        for poly in mesh.polygons:
            for j in poly.loop_indices:layer.data[j].uv=uvs[mesh.loops[j].vertex_index]
    bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT,name+'.fbx'),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE')
    return obj
R=6360000.
# Curved ocean patch reaches beyond the 100 km horizon; dense rings near the site.
verts=[(0,0,-3.5)];faces=[]
N=512;K=140
for i in range(K):
    r=60*math.exp(i*math.log(2000000/60)/(K-1))
    for j in range(N):
        a=j*math.tau/N;verts.append((r*math.cos(a),r*math.sin(a),math.sqrt(R*R-r*r)-R-3.5))
for j in range(N):faces.append((0,1+j,1+(j+1)%N))
for i in range(K-1):
    for j in range(N):
        a=1+i*N+j;b=1+i*N+(j+1)%N;faces.append((a,a+N,b+N,b))
export('SM_CurvedOcean',verts,faces)
# Broad coastal plain, generic coastline rather than a claim of surveyed Starbase geography.
verts=[];faces=[];N=256;K=180
for j in range(N+1):
    y=math.copysign(abs(2*j/N-1)**2.5*1600000,2*j/N-1)
    shore=600+500*math.sin(y/12000)+900*math.sin(y/27000)
    for i in range(K+1):
        x=shore-2100000*(i/K)**2.8
        inland=shore-x
        z=2+math.sin(x/8000)*math.cos(y/7000)*25*min(1,inland/15000)
        if abs(x)<2000 and abs(y)<2000:z=-0.35
        z+=math.sqrt(R*R-x*x-y*y)-R
        verts.append((x,y,z))
for j in range(N):
    for i in range(K):
        a=j*(K+1)+i;faces.append((a,a+K+1,a+K+2,a+1))
export('SM_CoastalPlain',verts,faces)
# UV-mapped exhaust envelope: local -Z, normalized dimensions for runtime pressure scaling.
verts=[];faces=[];uv=[];N=40;K=36
for i in range(K+1):
    t=i/K;r=(0.47+0.12*math.sin(t*math.pi))*max(0.006,1-t*t)**0.55
    for j in range(N+1):
        a=j*math.tau/N;verts.append((r*math.cos(a),r*math.sin(a),-t));uv.append((j/N,1-t))
for i in range(K):
    for j in range(N):
        a=i*(N+1)+j;faces.append((a,a+N+1,a+N+2,a+1))
export('SM_ExhaustEnvelope',verts,faces,uv)
# Upper-stage silhouette to make the carried mass and separation visually explicit.
verts=[];faces=[];uv=[];N=96;K=64
for i in range(K+1):
    z=54*i/K
    r=4.5 if z<=40 else 4.5*math.sqrt(max(0.0001,1-((z-40)/14)**1.5))
    for j in range(N+1):
        a=j*math.tau/N;verts.append((r*math.cos(a),r*math.sin(a),z));uv.append((j/N,z/54))
for i in range(K):
    for j in range(N):
        a=i*(N+1)+j;faces.append((a,a+1,a+N+2,a+N+1))
for sign in [-1,1]:
    for z,height,span in [(4,13,5.0),(39,8,3.2)]:
        start=len(verts)
        for y in [-0.22,0.22]:
            for x,zz in [(4.1,z),(4.1,z+height),(4.5+span,z+height*0.6),(4.5+span,z+height*0.12)]:
                verts.append((x*sign,y,zz));uv.append((0.25 if sign<0 else 0.75,zz/54))
        for f in [(0,1,2,3),(7,6,5,4),(0,4,5,1),(1,5,6,2),(2,6,7,3),(3,7,4,0)]:faces.append(tuple(start+i for i in f))
ship=export('SM_UpperStageProxy',verts,faces,uv)
# Open launch table with a clear exhaust passage, circular ring and eight legs.
verts=[];faces=[]
def ring(ro,ri,z0,z1,n=96):
    start=len(verts)
    for z,r in [(z0,ro),(z0,ri),(z1,ro),(z1,ri)]:
        verts.extend((r*math.cos(j*math.tau/n),r*math.sin(j*math.tau/n),z) for j in range(n))
    for j in range(n):
        k=(j+1)%n
        for face in [(j,k,2*n+k,2*n+j),(n+k,n+j,3*n+j,3*n+k),(2*n+j,2*n+k,3*n+k,3*n+j),(k,j,n+j,n+k)]:faces.append(tuple(start+a for a in face))
def beam(a,b,r,n=12):
    a,b=Vector(a),Vector(b);axis=(b-a).normalized();side=axis.cross(Vector((0,0,1)))
    if side.length<0.1:side=axis.cross(Vector((0,1,0)))
    side.normalize();other=axis.cross(side);start=len(verts)
    for p in [a,b]:
        verts.extend(tuple(p+r*(side*math.cos(i*math.tau/n)+other*math.sin(i*math.tau/n))) for i in range(n))
    for i in range(n):faces.append((start+i,start+(i+1)%n,start+(i+1)%n+n,start+i+n))
    faces.append(tuple(start+i for i in reversed(range(n))));faces.append(tuple(start+n+i for i in range(n)))
ring(8.8,4.55,12.8,15.15);ring(9.3,8.7,12.2,12.8)
for j in range(8):
    angle=j*math.tau/8;c,s=math.cos(angle),math.sin(angle)
    beam((8*c,8*s,0),(7.4*c,7.4*s,13),0.65)
    beam((8*c,8*s,4),(4.9*c,4.9*s,14.1),0.23)
    beam((7.4*c,7.4*s,9),(7.4*math.cos(angle+math.tau/8),7.4*math.sin(angle+math.tau/8),13),0.18)
export('SM_LaunchMount',verts,faces)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT,'RecoveryFlightEnvironment.blend'))
print('FLIGHT_ART_READY')
