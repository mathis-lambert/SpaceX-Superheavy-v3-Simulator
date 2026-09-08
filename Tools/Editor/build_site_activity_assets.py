"""Import source cloth and create lightweight vertex wind animation."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Shared'))
from project_paths import ART_ROOT
from unreal_materials import u,material,expression,scalar,vector,color,custom,constant,connect,prop,save
from unreal_imports import import_scenery_mesh
m=material('/Game/Starbase/Materials/Starbase/M_WindFlag')
prop(m,'two_sided',True)
uv=expression(m,u.MaterialExpressionTextureCoordinate)
t=scalar(m,'ActivityTime',0);wind=scalar(m,'WindSpeed',4)
normal=vector(m,'ClothNormal',(0,1,0,1))
offset=custom(m,{'UV':uv,'T':t,'W':wind,'N':normal},'''
float wave=sin(UV.x*11-T*(2+W*.12)+UV.y*3)*.65+sin(UV.x*23-T*4-UV.y*7)*.24;
return N.xyz*UV.x*UV.x*wave*clamp(W*4,5,45)+float3(0,0,-UV.x*UV.x*12);
''')
connect(m,offset,u.MaterialProperty.MP_WORLD_POSITION_OFFSET)
fabric=custom(m,{'UV':uv},'float weave=.96+.04*sin(UV.x*900)*sin(UV.y*450);return float3(.70,.73,.76)*weave;')
connect(m,fabric,u.MaterialProperty.MP_BASE_COLOR);connect(m,constant(m,.9),u.MaterialProperty.MP_ROUGHNESS);save(m)
mesh=import_scenery_mesh(ART_ROOT/'Starbase/Activity/SM_WindFlag.fbx','/Game/Starbase/Meshes/Starbase/SM_WindFlag')
mesh.set_material(0,m);u.EditorAssetLibrary.save_loaded_asset(mesh,False)
print('SITE_ACTIVITY_ASSETS_READY')
