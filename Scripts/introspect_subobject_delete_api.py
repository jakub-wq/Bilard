import unreal


subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
for name in ("delete_subobject", "delete_subobjects", "detach_subobject", "reparent_subobject"):
    fn = getattr(subsystem, name)
    unreal.log(f"{name}: {fn}")
    try:
        unreal.log(f"{name} doc: {fn.__doc__}")
    except Exception as exc:
        unreal.log_warning(f"{name} doc unavailable: {exc}")
