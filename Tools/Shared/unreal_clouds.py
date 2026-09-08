"""Cloud shader features shared by initial scene authoring and targeted rebuilds."""
from unreal_materials import u, E, prop, expression, constant, save


def configure_storm_feature(material, instance):
    # A uniform StormClouds=0 still evaluates its texture samples. A static
    # feature removes these branches from clear-weather shader permutations;
    # the enabled permutation retains the original continuous storm control.
    nodes = list(E.get_material_expressions(material))
    switches = [node for node in nodes if isinstance(node, u.MaterialExpressionStaticSwitchParameter)
                and str(node.get_editor_property('parameter_name')) == 'EnableStormClouds']
    assert len(switches) <= 1, 'Duplicate storm feature switches'
    if not switches:
        strengths = [node for node in nodes if isinstance(node, u.MaterialExpressionScalarParameter)
                     and str(node.get_editor_property('parameter_name')) == 'StormClouds']
        assert strengths, 'Expected authored storm strength'
        switch = expression(material, u.MaterialExpressionStaticSwitchParameter)
        prop(switch, 'parameter_name', 'EnableStormClouds')
        prop(switch, 'default_value', False)
        prop(switch, 'desc', 'Compile storm and lightning only for storm weather profiles')
        assert E.connect_material_expressions(strengths[0], '', switch, 'True')
        assert E.connect_material_expressions(constant(material, 0.), '', switch, 'False')
        for consumer in nodes:
            pins = E.get_material_expression_input_names(consumer)
            sources = E.get_inputs_for_material_expression(material, consumer)
            assert len(pins) == len(sources), consumer.get_name()
            for pin, source in zip(pins, sources):
                if source in strengths:
                    assert E.connect_material_expressions(switch, '', consumer, pin), pin
        # The engine graph repeated this same named parameter six times.
        for duplicate in strengths[1:]:
            E.delete_material_expression(material, duplicate)
    enabled = E.get_material_instance_scalar_parameter_value(instance, 'StormClouds') != 0.
    save(material)
    # UE 5.8's setter applies the change but always returns false. Verify the
    # actual parameter override and resolved value, not that erroneous result.
    E.set_material_instance_static_switch_parameter_value(instance, 'EnableStormClouds', enabled)
    assert E.is_material_instance_parameter_overridden(instance, 'EnableStormClouds')
    assert E.get_material_instance_static_switch_parameter_value(instance, 'EnableStormClouds') == enabled
    assert u.EditorAssetLibrary.save_loaded_asset(instance, False)
    return enabled
