"""Read-only graph inventory before changing the cloud's conservative bounds."""
import json
from pathlib import Path
import unreal as u

edit = u.MaterialEditingLibrary
material = u.load_asset('/Game/Starbase/Materials/M_CloudFlight')
instance = u.load_asset('/Game/Starbase/Materials/MI_CloudFlight')
assert material
nodes = []
for node in edit.get_material_expressions(material):
    item = {'name': node.get_name(), 'class': node.get_class().get_name(),
            'description': node.get_editor_property('desc'),
            'inputs': list(edit.get_material_expression_input_names(node)),
            'sources': [source.get_name() if source else None
                        for source in edit.get_inputs_for_material_expression(material, node)]}
    if isinstance(node, u.MaterialExpressionVolumetricAdvancedMaterialOutput):
        item['options'] = {key: node.get_editor_property(key) for key in (
            'gray_scale_material', 'ray_march_volume_shadow', 'ground_contribution',
            'per_sample_phase_evaluation', 'multi_scattering_approximation_octave_count')}
    if isinstance(node, u.MaterialExpressionMaterialFunctionCall):
        item['function'] = node.get_editor_property('material_function').get_path_name()
    if isinstance(node, u.MaterialExpressionScalarParameter):
        item['parameter'] = str(node.get_editor_property('parameter_name'))
        item['default'] = node.get_editor_property('default_value')
        item['instance'] = edit.get_material_instance_scalar_parameter_value(instance, item['parameter'])
    if isinstance(node, u.MaterialExpressionVectorParameter):
        item['parameter'] = str(node.get_editor_property('parameter_name'))
        value = edit.get_material_instance_vector_parameter_value(instance, item['parameter'])
        item['instance'] = [value.r, value.g, value.b, value.a]
    if isinstance(node, u.MaterialExpressionStaticSwitchParameter):
        item['parameter'] = str(node.get_editor_property('parameter_name'))
        item['default'] = node.get_editor_property('default_value')
    nodes.append(item)
target = Path(u.Paths.project_saved_dir())/'Recovery/cloud-material-graph.json'
target.parent.mkdir(parents=True, exist_ok=True)
roots = {}
for prop in (u.MaterialProperty.MP_BASE_COLOR, u.MaterialProperty.MP_SUBSURFACE_COLOR, u.MaterialProperty.MP_EMISSIVE_COLOR):
    source = edit.get_material_property_input_node(material, prop)
    roots[str(prop)] = source.get_name() if source else None
properties = {key: str(material.get_editor_property(key)) for key in ('material_domain', 'blend_mode', 'used_with_volumetric_cloud')}
target.write_text(json.dumps({'material': material.get_path_name(), 'properties': properties, 'roots': roots, 'nodes': nodes}, indent=2), encoding='utf-8')
u.log(f'CLOUD_GRAPH {len(nodes)} expressions: {target}')
