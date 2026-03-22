import unreal


def describe_character_defaults(asset_path: str) -> None:
    blueprint = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not blueprint:
        unreal.log_error(f"Could not load blueprint asset: {asset_path}")
        return

    generated_class = blueprint.generated_class()
    cdo = unreal.get_default_object(generated_class)
    for name in (
        "CueDistanceFromBall",
        "CuePullbackDistance",
        "CueSideOffset",
        "CueHeightOffset",
        "CueAimRotationOffset",
    ):
        try:
            value = cdo.get_editor_property(name)
            unreal.log(f"{asset_path} {name} = {value}")
        except Exception as exc:
            unreal.log_warning(f"{asset_path} missing property {name}: {exc}")


if __name__ == "__main__":
    describe_character_defaults("/Game/BP_MyCharacter")
