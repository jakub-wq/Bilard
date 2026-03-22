import unreal


def log_mesh_info(label: str, mesh: unreal.StaticMesh) -> None:
    if not mesh:
        unreal.log_warning(f"{label}: no mesh")
        return

    box = mesh.get_bounding_box()
    center = (box.min + box.max) * 0.5
    extent = (box.max - box.min) * 0.5
    unreal.log(f"{label}: path={mesh.get_path_name()} center={center} extent={extent}")


def log_component_info(label: str, component) -> None:
    if not component:
        unreal.log_warning(f"{label}: no component")
        return

    try:
        mesh = component.get_editor_property("static_mesh")
    except Exception:
        mesh = None

    unreal.log(
        f"{label}: hidden_in_game={component.get_editor_property('hidden_in_game')} "
        f"visible={component.get_editor_property('visible')} "
        f"relative_location={component.get_editor_property('relative_location')} "
        f"relative_rotation={component.get_editor_property('relative_rotation')} "
        f"relative_scale3d={component.get_editor_property('relative_scale3d')}"
    )
    log_mesh_info(f"{label}.mesh", mesh)


def main() -> None:
    native_class = unreal.load_class(None, "/Script/Billard.MyCharacter")
    native_cdo = unreal.get_default_object(native_class) if native_class else None
    if native_cdo:
        unreal.log("Native MyCharacter defaults:")
        log_component_info("NativeCueMesh", native_cdo.get_editor_property("CueMesh"))
        for name in ("CueDistanceFromBall", "CuePullbackDistance", "CueSideOffset", "CueHeightOffset", "CueAimRotationOffset"):
            unreal.log(f"Native {name} = {native_cdo.get_editor_property(name)}")

    bp = unreal.EditorAssetLibrary.load_asset("/Game/BP_MyCharacter")
    if not bp:
        unreal.log_error("Could not load /Game/BP_MyCharacter")
        return

    generated_class = bp.generated_class()
    bp_cdo = unreal.get_default_object(generated_class) if generated_class else None
    if bp_cdo:
        unreal.log("Blueprint MyCharacter defaults:")
        log_component_info("BlueprintCueMesh", bp_cdo.get_editor_property("CueMesh"))
        for name in ("CueDistanceFromBall", "CuePullbackDistance", "CueSideOffset", "CueHeightOffset", "CueAimRotationOffset"):
            unreal.log(f"Blueprint {name} = {bp_cdo.get_editor_property(name)}")

    cue_mesh = unreal.load_asset("/Game/Billiards/Cues/CueStick_A_Source")
    log_mesh_info("ImportedCueAsset", cue_mesh)


if __name__ == "__main__":
    main()
