
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/"Tools"/"Shared"))
from project_paths import PROJECT_ROOT, ART_ROOT
import bpy, json, os
from mathutils import Vector
data=[]
for obj in bpy.data.objects:
    if obj.type=='MESH':
        points=[obj.matrix_world@Vector(v) for v in obj.bound_box]
        data.append(dict(name=obj.name,vertices=len(obj.data.vertices),min=[min(p[i] for p in points) for i in range(3)],max=[max(p[i] for p in points) for i in range(3)],materials=[m.name if m else '' for m in obj.data.materials]))
path=os.path.abspath('SuperHeavySim/Saved/Recovery/geometry-audit.json')
os.makedirs(os.path.dirname(path),exist_ok=True)
open(path,'w').write(json.dumps(data,indent=2))
print('GEOMETRY_AUDIT',path,len(data))
obj=bpy.data.objects.get('Superheavy v3')
adj=[[] for _ in obj.data.vertices]
for e in obj.data.edges:
    a,b=e.vertices;adj[a].append(b);adj[b].append(a)
seen=set();parts=[]
for v in obj.data.vertices:
    if v.index in seen:continue
    todo=[v.index];seen.add(v.index);part=[]
    while todo:
        i=todo.pop();part.append(obj.matrix_world@obj.data.vertices[i].co)
        for j in adj[i]:
            if j not in seen:seen.add(j);todo.append(j)
    lo=[min(p[i] for p in part) for i in range(3)];hi=[max(p[i] for p in part) for i in range(3)]
    if hi[2]>50 and lo[2]<65 and len(part)>10:parts.append(dict(n=len(part),min=lo,max=hi))
print('UPPER_BODY_ISLANDS',json.dumps(parts))
