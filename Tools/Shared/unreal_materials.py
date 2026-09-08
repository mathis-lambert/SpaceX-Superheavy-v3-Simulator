"""Small, shared MaterialEditingLibrary helpers with checked graph connections."""
import unreal as u

E=u.MaterialEditingLibrary
A=u.EditorAssetLibrary

def prop(obj,key,value):
    obj.set_editor_property(key,value)

def expression(material,cls):
    return E.create_material_expression(material,cls)

def constant(material,value):
    node=expression(material,u.MaterialExpressionConstant);prop(node,'r',value);return node

def scalar(material,name,value):
    node=expression(material,u.MaterialExpressionScalarParameter)
    prop(node,'parameter_name',name);prop(node,'default_value',value);return node

def vector(material,name,value):
    node=expression(material,u.MaterialExpressionVectorParameter)
    prop(node,'parameter_name',name);prop(node,'default_value',u.LinearColor(*value));return node

def color(material,value):
    node=expression(material,u.MaterialExpressionConstant3Vector)
    prop(node,'constant',u.LinearColor(*value,1));return node

def custom(material,inputs,code,kind=u.CustomMaterialOutputType.CMOT_FLOAT3):
    node=expression(material,u.MaterialExpressionCustom);pins=[]
    for name in inputs:
        pin=u.CustomInput();prop(pin,'input_name',name);pins.append(pin)
    prop(node,'inputs',pins);prop(node,'code',code);prop(node,'output_type',kind)
    for name,value in inputs.items():
        source,output=value if isinstance(value,tuple) else (value,'')
        assert E.connect_material_expressions(source,output,node,name),name
    return node

def connect(material,node,property_name,pin=''):
    assert E.connect_material_property(node,pin,property_name),str(property_name)

def sample(material,texture,uv=None,normal=False,sampler=None):
    node=expression(material,u.MaterialExpressionTextureSample)
    prop(node,'texture',u.load_asset(texture) if isinstance(texture,str) else texture)
    if normal:prop(node,'sampler_type',u.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    if sampler is not None:prop(node,'sampler_type',sampler)
    if uv:assert E.connect_material_expressions(uv,'',node,E.get_material_expression_input_names(node)[0])
    return node

def material(path):
    name=path.rsplit('/',1)[-1];folder=path.rsplit('/',1)[0]
    result=u.load_asset(path) if A.does_asset_exist(path) else u.AssetToolsHelpers.get_asset_tools().create_asset(name,folder,u.Material,u.MaterialFactoryNew())
    assert result,path
    E.delete_all_material_expressions(result)
    return result

def save(material):
    if material.get_editor_property('material_domain')==u.MaterialDomain.MD_SURFACE and material.get_editor_property('blend_mode')==u.BlendMode.BLEND_OPAQUE:
        prop(material,'used_with_nanite',True)
    E.recompile_material(material)
    assert A.save_loaded_asset(material,False),material.get_path_name()
