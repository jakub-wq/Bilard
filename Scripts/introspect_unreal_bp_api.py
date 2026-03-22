import unreal


for cls_name in [
    "Blueprint",
    "BlueprintEditorLibrary",
    "SubobjectDataSubsystem",
    "SubobjectDataBlueprintFunctionLibrary",
    "KismetEditorUtilities",
]:
    try:
        cls = getattr(unreal, cls_name)
        unreal.log(f"{cls_name}: {cls}")
        members = [name for name in dir(cls) if "component" in name.lower() or "subobject" in name.lower() or "scs" in name.lower() or "blueprint" in name.lower() or "attach" in name.lower()]
        unreal.log(f"{cls_name} members: {members}")
    except Exception as exc:
        unreal.log_warning(f"{cls_name}: {exc}")
