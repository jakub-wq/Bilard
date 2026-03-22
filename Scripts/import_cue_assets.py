import unreal


DESTINATION_PATH = "/Game/Billiards/Cues"


def build_import_options() -> unreal.FbxImportUI:
    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = False
    options.import_materials = True
    options.import_textures = False
    options.import_animations = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    options.static_mesh_import_data.combine_meshes = False
    options.static_mesh_import_data.transform_vertex_to_absolute = False
    options.static_mesh_import_data.bake_pivot_in_vertex = True
    options.static_mesh_import_data.generate_lightmap_u_vs = True
    options.static_mesh_import_data.auto_generate_collision = False
    return options


def import_cue_meshes() -> None:
    project_dir = unreal.Paths.project_dir()
    cue_sources = [
        unreal.Paths.combine([project_dir, "SourceArt", "Exports", "CueStick_A_Source.fbx"]),
        unreal.Paths.combine([project_dir, "SourceArt", "Exports", "CueStick_B_Source.fbx"]),
    ]

    if not unreal.EditorAssetLibrary.does_directory_exist(DESTINATION_PATH):
        unreal.EditorAssetLibrary.make_directory(DESTINATION_PATH)

    tasks = []
    for cue_source in cue_sources:
        task = unreal.AssetImportTask()
        task.filename = cue_source
        task.destination_path = DESTINATION_PATH
        task.automated = True
        task.replace_existing = True
        task.replace_existing_settings = True
        task.save = True
        task.options = build_import_options()
        tasks.append(task)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    unreal.EditorAssetLibrary.save_directory(DESTINATION_PATH, only_if_is_dirty=False, recursive=True)
    unreal.log(f"Imported cue meshes into {DESTINATION_PATH}")
    for task in tasks:
        for asset_path in task.imported_object_paths:
            unreal.log(f"  {asset_path}")


if __name__ == "__main__":
    import_cue_meshes()
