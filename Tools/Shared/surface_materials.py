"""Canonical industrial and vehicle surfaces. No graph-patching passes.

All fine patterns are footprint filtered. Surface state is shared through one
collection, so deluge residue needs no per-object tick or duplicated materials.
"""
from unreal_materials import u, A, E, prop, material, expression as ex, custom, sample, constant, color, scalar, vector, connect, save

ROOT = '/Game/Starbase'
STATE = ROOT + '/Materials/MPC_SurfaceState'


def surface_state():
    collection = u.load_asset(STATE)
    if not collection:
        collection = u.AssetToolsHelpers.get_asset_tools().create_asset(
            'MPC_SurfaceState', ROOT+'/Materials', u.MaterialParameterCollection,
            u.MaterialParameterCollectionFactoryNew())
    # Preserve existing parameter IDs across rebuilds.
    scalars = list(collection.get_editor_property('scalar_parameters'))
    names = {str(p.get_editor_property('parameter_name')) for p in scalars}
    for name in ('PadWetness', 'PadResidue'):
        if name not in names:
            p = u.CollectionScalarParameter()
            prop(p, 'parameter_name', name); prop(p, 'default_value', 0.)
            scalars.append(p)
    prop(collection, 'scalar_parameters', scalars)
    vectors = list(collection.get_editor_property('vector_parameters'))
    if not any(str(p.get_editor_property('parameter_name')) == 'PadOrigin' for p in vectors):
        p = u.CollectionVectorParameter(); prop(p, 'parameter_name', 'PadOrigin')
        prop(p, 'default_value', u.LinearColor(2400, 0, 0, 0)); vectors.append(p)
    prop(collection, 'vector_parameters', vectors)
    assert A.save_loaded_asset(collection, False)
    return collection


def state_parameter(m, collection, name):
    node = ex(m, u.MaterialExpressionCollectionParameter)
    prop(node, 'collection', collection); prop(node, 'parameter_name', name)
    return node


def industrial_surfaces(collection):
    results = []
    for name, tint, metal, rough in (
        ('M_Concrete', (.29,.285,.265), 0., .84),
        ('M_Cladding', (.47,.49,.50), .78, .38),
        ('M_Graphite', (.055,.065,.073), .75, .43),
        ('M_SafetyAmber', (.68,.25,.025), .12, .48)):
        m = material(ROOT+'/Materials/'+name)
        prop(m, 'used_with_instanced_static_meshes', True)
        p = ex(m, u.MaterialExpressionWorldPosition)
        normal = ex(m, u.MaterialExpressionVertexNormalWS)
        # Blend projections rather than stretching XY detail up vertical walls.
        uv = custom(m, {'P':p, 'N':normal}, '''
float3 n=abs(N); float3 w=pow(n,8); w/=max(dot(w,1),.0001);
return (P.yz*w.x+P.xz*w.y+P.xy*w.z)/420;
''', u.CustomMaterialOutputType.CMOT_FLOAT2)
        grain = sample(m, '/Game/ThirdParty/MWLandscapeAutoMaterial/Textures/Ground/TEX_MWAM_SandA_col', uv)
        wet = state_parameter(m, collection, 'PadWetness')
        residue = state_parameter(m, collection, 'PadResidue')
        origin = state_parameter(m, collection, 'PadOrigin')
        contact = custom(m, {'P':p,'O':origin,'N':normal,'W':wet}, '''
float footprint=exp(-dot((P.xy-O.xy)/11500,(P.xy-O.xy)/11500));
float horizontal=smoothstep(.65,.95,N.z);
float nearGround=1-smoothstep(100,550,abs(P.z-O.z));
return W*footprint*horizontal*nearGround;
''', u.CustomMaterialOutputType.CMOT_FLOAT1)
        base = custom(m, {'P':p,'D':(grain,'RGB'),'Base':color(m,tint),'Wet':contact,'R':residue,'O':origin}, '''
float aggregate=clamp(dot(D,float3(.299,.587,.114))*2.2,.65,1.35);
float broad=.95+.04*sin(P.x*.0007+sin(P.y*.00047))+.025*sin(P.y*.0011);
float stain=R*exp(-dot((P.xy-O.xy)/4200,(P.xy-O.xy)/4200))*(1-smoothstep(80,300,abs(P.z-O.z)));
return Base*lerp(.92,1.08,aggregate*.5)*broad*(1-.28*Wet)*(1-.22*stain);
''')
        r = custom(m, {'D':(grain,'RGB'),'Wet':contact,'R':constant(m,rough)},
                   'return clamp(lerp(R+(D.r-.5)*.10,.23,Wet*.8),.2,.95);',
                   u.CustomMaterialOutputType.CMOT_FLOAT1)
        connect(m,base,u.MaterialProperty.MP_BASE_COLOR)
        connect(m,r,u.MaterialProperty.MP_ROUGHNESS)
        connect(m,constant(m,metal),u.MaterialProperty.MP_METALLIC)
        save(m); results.append(m.get_path_name())
    return results


def vehicle_surfaces():
    results = []
    # Reuse the actual mesh UVs and authored textures; channel copies are linear.
    m = material(ROOT+'/Materials/M_BoosterFlight')
    uv = ex(m,u.MaterialExpressionTextureCoordinate)
    albedo = sample(m,ROOT+'/Vehicle/Textures/Superheavy-V3_BaseColor',uv)
    normal = sample(m,ROOT+'/Vehicle/Textures/Superheavy-V3_Normal',uv,normal=True)
    def linear_mask(name):
        texture=u.load_asset(ROOT+'/Textures/'+name); assert texture,name
        prop(texture,'srgb',False); assert A.save_loaded_asset(texture,False)
        sampler=(u.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE
                 if texture.get_editor_property('compression_settings')==u.TextureCompressionSettings.TC_GRAYSCALE
                 else u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
        return sample(m,texture,uv,sampler=sampler)
    rough_tex = linear_mask('T_BoosterRoughLinear')
    metal_tex = linear_mask('T_BoosterMetalLinear')
    connect(m,albedo,u.MaterialProperty.MP_BASE_COLOR,'RGB')
    connect(m,normal,u.MaterialProperty.MP_NORMAL,'RGB')
    connect(m,custom(m,{'R':(rough_tex,'R')},'return clamp(R,.29,.86);',u.CustomMaterialOutputType.CMOT_FLOAT1),u.MaterialProperty.MP_ROUGHNESS)
    connect(m,metal_tex,u.MaterialProperty.MP_METALLIC,'R'); save(m); results.append(m.get_path_name())

    for name, tint, rough in (('M_StainlessFlight',(.56,.59,.62),.32),
                               ('M_GridFinAlloy',(.21,.23,.24),.46),
                               ('M_FlightMechanisms',(.065,.075,.085),.46),
                               ('M_FlightNozzles',(.23,.25,.27),.39)):
        m=material(ROOT+'/Materials/'+name); uv=ex(m,u.MaterialExpressionTextureCoordinate)
        steel=name=='M_StainlessFlight'
        base=custom(m,{'UV':uv,'C':color(m,tint),'Steel':constant(m,1 if steel else 0)},'''
float seamPhase=UV.y*54/1.82*3.14159265;
float weld=pow(abs(cos(seamPhase)),55)*(1-smoothstep(.1,.8,fwidth(seamPhase)));
float brushPhase=UV.x*900;
float brush=sin(brushPhase)*.004*(1-smoothstep(.2,2,fwidth(brushPhase)));
return C*(1-Steel*weld*.14+brush);
''')
        r=custom(m,{'UV':uv,'R':constant(m,rough)},'''
float2 q=UV*float2(24,72); float footprint=length(fwidth(q));
return R+.025*sin(q.x)*sin(q.y)*(1-smoothstep(.1,1.5,footprint));
''',u.CustomMaterialOutputType.CMOT_FLOAT1)
        connect(m,base,u.MaterialProperty.MP_BASE_COLOR); connect(m,r,u.MaterialProperty.MP_ROUGHNESS)
        connect(m,constant(m,.95 if steel else .85),u.MaterialProperty.MP_METALLIC)
        save(m); results.append(m.get_path_name())
    tile=material(ROOT+'/Materials/M_StarshipHeatShield')
    uv=ex(tile,u.MaterialExpressionTextureCoordinate)
    tile_color=custom(tile,{'UV':uv},'''
float2 p=UV*float2(160,300);
float footprint=max(length(ddx(p)),length(ddy(p)));
p.x+=fmod(floor(p.y),2)*.5;
float2 q=abs(frac(p)-.5);
float d=max(q.x*.866+q.y*.5,q.y);
float edge=1-smoothstep(.445-fwidth(d),.465+fwidth(d),d);
float n=frac(sin(dot(floor(p),float2(12.9898,78.233)))*43758.5453);
float detailed=lerp(.32,.80+n*.35,edge);
return float3(.024,.027,.031)*lerp(detailed,.78,smoothstep(.4,1.2,footprint));
''')
    connect(tile,tile_color,u.MaterialProperty.MP_BASE_COLOR)
    connect(tile,constant(tile,.88),u.MaterialProperty.MP_ROUGHNESS)
    save(tile);results.append(tile.get_path_name())
    return results


def site_lamp():
    m=material(ROOT+'/Materials/M_SiteLamp')
    prop(m,'shading_model',u.MaterialShadingModel.MSM_UNLIT)
    connect(m,custom(m,{'C':vector(m,'Color',(1,.8,.55,1)),'I':scalar(m,'Intensity',300)},
                     'return C*I;'),u.MaterialProperty.MP_EMISSIVE_COLOR)
    save(m)
    return m.get_path_name()


def road_surface(name,gravel=False):
    m=material(ROOT+'/Materials/Starbase/'+name)
    p=ex(m,u.MaterialExpressionWorldPosition);uv=ex(m,u.MaterialExpressionTextureCoordinate)
    coords=custom(m,{'P':p},'return P.xy/350;',u.CustomMaterialOutputType.CMOT_FLOAT2)
    grain=sample(m,'/Game/ThirdParty/MWLandscapeAutoMaterial/Textures/Ground/TEX_MWAM_SandA_col',coords)
    normal=sample(m,'/Game/ThirdParty/MWLandscapeAutoMaterial/Textures/Ground/TEX_MWAM_SandA_nrm',coords,normal=True)
    base=custom(m,{'P':p,'UV':uv,'D':(grain,'RGB')},('''
float aggregate=.7+dot(D,float3(.299,.587,.114))*.65;
return float3(.24,.205,.16)*aggregate;
''' if gravel else '''
float aggregate=.85+dot(D,float3(.299,.587,.114))*.35;
float aa=max(fwidth(UV.x),.002);
float lane=1-smoothstep(.005-aa,.005+aa,abs(UV.x-.5));
float phase=UV.y*.62831853;
float dash=smoothstep(.25-fwidth(phase),.25+fwidth(phase),cos(phase));
float edge=1-smoothstep(.005-aa,.005+aa,abs(abs(UV.x-.5)-.44));
float wear=.75+.25*sin(P.x*.013+sin(P.y*.009)*2);
return lerp(float3(.035,.039,.043)*aggregate,float3(.58,.56,.49),max(lane*dash,edge)*wear);
'''))
    connect(m,base,u.MaterialProperty.MP_BASE_COLOR);connect(m,normal,u.MaterialProperty.MP_NORMAL,'RGB')
    connect(m,constant(m,.93 if gravel else .84),u.MaterialProperty.MP_ROUGHNESS);save(m);return m
