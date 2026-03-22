import unreal


BP_PATH = "/Game/BP_MyCharacter"

bp = unreal.load_asset(BP_PATH)
if not bp:
    unreal.log_error(f"Could not load {BP_PATH}")
    raise SystemExit(1)

subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)
if not handles:
    unreal.log_error(f"No subobject handles found for {BP_PATH}")
    raise SystemExit(1)

context_handle = handles[0]
deleted = False

for handle in handles[1:]:
    data = subsystem.k2_find_subobject_data_from_handle(handle)
    obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, bp)
    obj_name = obj.get_name() if obj else None
    unreal.log(f"Inspecting subobject {obj_name}")

    if obj_name == "Kij_GEN_VARIABLE":
        result = subsystem.delete_subobject(context_handle, handle, bp)
        unreal.log(f"delete_subobject(Kij_GEN_VARIABLE) -> {result}")
        deleted = True

if not deleted:
    unreal.log_warning("Did not find Kij_GEN_VARIABLE to delete.")

unreal.BlueprintEditorLibrary.compile_blueprint(bp)
saved = unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
unreal.log(f"Saved {BP_PATH}: {saved}")
