import unreal


DESTINATION_PATH = "/Game/Billiards/Balls"


def build_import_options() -> unreal.FbxImportUI:
    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = False
    options.import_materials = True
    options.import_textures = False
    options.import_animations = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    options.static_mesh_import_data.combine_meshes = False
    options.static_mesh_import_data.generate_lightmap_u_vs = True
    options.static_mesh_import_data.auto_generate_collision = False
    return options


def import_ball_meshes() -> None:
    project_fbx = unreal.Paths.combine([unreal.Paths.project_dir(), "SourceArt", "Exports", "PoolBalls_Source.fbx"])

    if not unreal.EditorAssetLibrary.does_directory_exist(DESTINATION_PATH):
        unreal.EditorAssetLibrary.make_directory(DESTINATION_PATH)

    task = unreal.AssetImportTask()
    task.filename = project_fbx
    task.destination_path = DESTINATION_PATH
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = True
    task.options = build_import_options()

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    imported = [path for path in task.imported_object_paths if path.endswith("_Source")]
    unreal.EditorAssetLibrary.save_directory(DESTINATION_PATH, only_if_is_dirty=False, recursive=True)
    unreal.log(f"Imported {len(imported)} pool ball meshes from {project_fbx} into {DESTINATION_PATH}")
    for asset_path in imported:
        unreal.log(f"  {asset_path}")


if __name__ == "__main__":
    import_ball_meshes()
