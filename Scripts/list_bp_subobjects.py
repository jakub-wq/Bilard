import unreal


bp = unreal.load_asset("/Game/BP_MyCharacter")
subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)

unreal.log(f"Found {len(handles)} subobject handles")
for handle in handles:
    data = subsystem.k2_find_subobject_data_from_handle(handle)
    obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, bp)
    owner_bp = unreal.SubobjectDataBlueprintFunctionLibrary.get_blueprint(data)
    flags = {
        "is_component": unreal.SubobjectDataBlueprintFunctionLibrary.is_component(data),
        "is_scene_component": unreal.SubobjectDataBlueprintFunctionLibrary.is_scene_component(data),
        "is_root": unreal.SubobjectDataBlueprintFunctionLibrary.is_root_component(data),
        "is_native": unreal.SubobjectDataBlueprintFunctionLibrary.is_native_component(data),
        "is_inherited": unreal.SubobjectDataBlueprintFunctionLibrary.is_inherited_component(data),
    }
    unreal.log(f"handle={handle} obj={obj} owner={owner_bp} flags={flags}")
