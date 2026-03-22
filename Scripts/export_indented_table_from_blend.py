import math
from pathlib import Path

import bpy


SOURCE_BLEND = Path("/Users/jakub/Downloads/source/Billard.blend")
EXPORT_FBX = Path("/Users/jakub/Bilard/SourceArt/Exports/Billard_Table_Indented.fbx")
OUTPUT_BLEND = Path("/Users/jakub/Bilard/SourceArt/Billard_table_indented.blend")


def find_table_object() -> bpy.types.Object:
    for candidate_name in ("Cube", "PoolTable", "Billard"):
        obj = bpy.data.objects.get(candidate_name)
        if obj is not None and obj.type == "MESH":
            return obj
    raise RuntimeError("Could not find the table mesh object in the source blend.")


def find_table_parts() -> list[bpy.types.Object]:
    part_names = [
        "Cube",
        "Cube.001",
        "Plan.001",
        "Plan.002",
        "Plan.003",
        "Plan.004",
        "Plan.005",
        "Plan.006",
        "Plan.007",
    ]
    parts: list[bpy.types.Object] = []
    for name in part_names:
        obj = bpy.data.objects.get(name)
        if obj is not None and obj.type == "MESH":
            parts.append(obj)
    if not parts:
        raise RuntimeError("Could not find any table parts to export.")
    return parts


def indent_felt(table: bpy.types.Object) -> None:
    mesh = table.data
    felt_material_indices = {
        index
        for index, slot in enumerate(table.material_slots)
        if slot.material and slot.material.name == "Tapis"
    }
    if not felt_material_indices:
        raise RuntimeError("Could not find the Tapis material on the table mesh.")

    felt_vertex_indices: set[int] = set()
    top_z = -10_000.0
    for polygon in mesh.polygons:
        if polygon.material_index not in felt_material_indices:
            continue
        for vertex_index in polygon.vertices:
            felt_vertex_indices.add(vertex_index)
            top_z = max(top_z, mesh.vertices[vertex_index].co.z)

    felt_vertices = [mesh.vertices[index] for index in felt_vertex_indices if mesh.vertices[index].co.z >= top_z - 0.05]
    if not felt_vertices:
        raise RuntimeError("No felt vertices were found near the top surface.")

    half_x = max(abs(vertex.co.x) for vertex in felt_vertices)
    half_y = max(abs(vertex.co.y) for vertex in felt_vertices)
    inner_half_x = max(1e-3, half_x * 0.84)
    inner_half_y = max(1e-3, half_y * 0.84)
    dish_depth = max(0.35, min(0.95, min(half_x, half_y) * 0.012))

    for vertex in felt_vertices:
        local_x = vertex.co.x / inner_half_x
        local_y = vertex.co.y / inner_half_y
        radial = math.sqrt(local_x * local_x + local_y * local_y)
        if radial >= 1.0:
            continue

        falloff = (1.0 - radial * radial) ** 2
        vertex.co.z = top_z - (dish_depth * falloff)

    mesh.update()


def export_table(table_parts: list[bpy.types.Object]) -> None:
    duplicates: list[bpy.types.Object] = []

    bpy.ops.object.select_all(action="DESELECT")
    for obj in table_parts:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = table_parts[0]
    bpy.ops.object.duplicate()
    duplicates = list(bpy.context.selected_objects)

    bpy.ops.object.select_all(action="DESELECT")
    for obj in duplicates:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = duplicates[0]
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    bpy.ops.object.join()

    joined = bpy.context.active_object
    joined.name = "Billard_Table_Joined"
    bpy.ops.object.origin_set(type="ORIGIN_GEOMETRY", center="BOUNDS")

    bpy.ops.export_scene.fbx(
        filepath=str(EXPORT_FBX),
        use_selection=True,
        object_types={"MESH"},
        apply_unit_scale=True,
        bake_space_transform=False,
        add_leaf_bones=False,
        mesh_smooth_type="FACE",
        path_mode="AUTO",
    )

    bpy.ops.object.delete()


def main() -> None:
    EXPORT_FBX.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_BLEND.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.open_mainfile(filepath=str(SOURCE_BLEND))
    table = find_table_object()
    table_parts = find_table_parts()
    indent_felt(table)
    export_table(table_parts)
    bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT_BLEND))
    print(f"Exported indented table mesh to {EXPORT_FBX}")


if __name__ == "__main__":
    main()
