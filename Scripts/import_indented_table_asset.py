import unreal


DESTINATION_PATH = "/Game"
DESTINATION_NAME = "Billard"


def build_import_options() -> unreal.FbxImportUI:
    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = False
    options.import_materials = True
    options.import_textures = False
    options.import_animations = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    options.static_mesh_import_data.combine_meshes = True
    options.static_mesh_import_data.generate_lightmap_u_vs = True
    options.static_mesh_import_data.auto_generate_collision = False
    return options


def import_table() -> None:
    source_fbx = unreal.Paths.combine([unreal.Paths.project_dir(), "SourceArt", "Exports", "Billard_Table_Indented.fbx"])

    task = unreal.AssetImportTask()
    task.filename = source_fbx
    task.destination_path = DESTINATION_PATH
    task.destination_name = DESTINATION_NAME
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = True
    task.options = build_import_options()

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    unreal.EditorAssetLibrary.save_asset("/Game/Billard", only_if_is_dirty=False)
    unreal.log(f"Imported updated table mesh from {source_fbx} to /Game/Billard")


if __name__ == "__main__":
    import_table()
