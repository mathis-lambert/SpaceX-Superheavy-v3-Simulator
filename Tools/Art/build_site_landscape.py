"""Bake continuous service roads, drainage shoulders and dune relief."""
import sys,math,json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import ART_ROOT
from earth_geography import point,geo,height
from site_landscape import ROAD,dune_height,road_height
import bpy
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)

OUT=ART_ROOT/'Starbase/Landscape';OUT.mkdir(parents=True,exist_ok=True)
reports=[]
def mesh(name,vertices,faces,uv):
    data=bpy.data.meshes.new(name);data.from_pydata(vertices,[],faces);data.update()
    obj=bpy.data.objects.new(name,data);bpy.context.collection.objects.link(obj)
    layer=data.uv_layers.new(name='UVMap')
    for poly in data.polygons:
        poly.use_smooth=True
        for index in poly.loop_indices:layer.data[index].uv=uv[data.loops[index].vertex_index]
    bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
    bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE')
    reports.append(dict(name=name,vertices=len(vertices),faces=len(faces)))

def grid(name,nx,ny,fn):
    vertices=[];uv=[];faces=[]
    for j in range(ny+1):
        for i in range(nx+1):
            p,t=fn(i/nx,j/ny);vertices.append(p);uv.append(t)
    for j in range(ny):
        for i in range(nx):
            a=j*(nx+1)+i;faces.append((a,a+1,a+nx+2,a+nx+1))
    mesh(name,vertices,faces,uv)

# Replace the four central terrain tiles, rather than layering another ground
# mesh. Match the old coarser edge vertices at the outer perimeter exactly.
for j in (1,2):
    for i in (1,2):
        def terrain(u,v,i=i,j=j):
            x,y=-6000+3000*(i+u),-6000+3000*(j+v)
            if abs(x)==3000 or abs(y)==3000:
                axis=y if abs(x)==3000 else x
                a=math.floor((axis+3000)/(3000/256))*(3000/256)-3000;b=a+3000/256
                p0=point(*geo(x,a),height(x,a)) if abs(x)==3000 else point(*geo(a,y),height(a,y))
                p1=point(*geo(x,b),height(x,b)) if abs(x)==3000 else point(*geo(b,y),height(b,y))
                t=(axis-a)/(b-a)
                p=tuple(aa+(bb-aa)*t for aa,bb in zip(p0,p1))
            else:p=point(*geo(x,y),dune_height(x,y))
            return p,(u,v)
        grid(f'SM_BocaChica_{i}_{j}',512,512,terrain)

# Each segment shares a mitered boundary; shoulders carry the surface into gravel.
def ribbon(name,half_width,offset):
    vertices=[];uv=[];faces=[];distance=0
    for i,(x,y) in enumerate(ROAD):
        a=ROAD[i-1] if i>0 else ROAD[-2];b=ROAD[i+1] if i<len(ROAD)-1 else ROAD[1]
        dx,dy=b[0]-a[0],b[1]-a[1];length=math.hypot(dx,dy);nx,ny=-dy/length,dx/length
        if i:distance+=math.dist(ROAD[i-1],ROAD[i])
        for sign in (-1,1):
            px,py=x+nx*half_width*sign,y+ny*half_width*sign
            vertices.append((px,py,road_height(px,py)+offset));uv.append(((sign+1)/2,distance))
        if i:a=(i-1)*2;faces.append((a,a+2,a+3,a+1))
    # West access branch ends at the loop, with no overlapping road strips.
    a=len(vertices)
    for x,y in [(-305,-90-half_width),(-137,-90-half_width),(-137,-90+half_width),(-305,-90+half_width)]:
        vertices.append((x,y,road_height(x,y)+offset));uv.append(((y+90+half_width)/(2*half_width),x+305))
    faces.append((a,a+1,a+2,a+3))
    mesh(name,vertices,faces,uv)
ribbon('SM_ServiceRoadNetwork',5.5,0)
ribbon('SM_ServiceRoadShoulder',8.4,-.09)

bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'SiteLandscape.blend'))
(OUT/'geometry.json').write_text(json.dumps(dict(meshes=reports,road=ROAD,source='USGS 3DEP + authored sub-metre dune detail and service layout',sea_datum_m=-4.5),indent=2))
print('SITE_LANDSCAPE_READY',reports,flush=True)
