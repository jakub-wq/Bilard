import os
from pathlib import Path

import bpy
import mathutils


PROJECT_ROOT = Path(os.environ.get("PROJECT_ROOT", Path(__file__).resolve().parents[1]))
EXPORT_DIR = PROJECT_ROOT / "SourceArt" / "Exports"
BLEND_OUTPUT = PROJECT_ROOT / "SourceArt" / "Billard_split.blend"


def ensure_collection(name: str, parent: bpy.types.Collection | None = None) -> bpy.types.Collection:
    collection = bpy.data.collections.get(name)
    if collection is None:
        collection = bpy.data.collections.new(name)
    target_parent = parent or bpy.context.scene.collection
    if collection.name not in target_parent.children:
        target_parent.children.link(collection)
    return collection


def unlink_from_all(obj: bpy.types.Object) -> None:
    for collection in list(obj.users_collection):
        collection.objects.unlink(obj)


def move_to_collection(obj: bpy.types.Object, collection: bpy.types.Collection) -> None:
    unlink_from_all(obj)
    collection.objects.link(obj)


def export_objects(filepath: Path, objects: list[bpy.types.Object]) -> None:
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.export_scene.fbx(
        filepath=str(filepath),
        use_selection=True,
        object_types={"MESH", "EMPTY"},
        apply_unit_scale=True,
        bake_space_transform=False,
        add_leaf_bones=False,
        mesh_smooth_type="FACE",
        path_mode="AUTO",
    )


def create_empty(name: str, location: mathutils.Vector, collection: bpy.types.Collection) -> bpy.types.Object:
    existing = bpy.data.objects.get(name)
    if existing is None:
        empty = bpy.data.objects.new(name, None)
        empty.empty_display_type = "SPHERE"
        empty.empty_display_size = 0.04
    else:
        empty = existing
    empty.location = location
    move_to_collection(empty, collection)
    return empty


def felt_extents(table: bpy.types.Object) -> tuple[float, float, float]:
    mesh = table.data
    top_z = max((table.matrix_world @ vertex.co).z for vertex in mesh.vertices)
    x_values: list[float] = []
    y_values: list[float] = []
    for polygon in mesh.polygons:
        material = table.material_slots[polygon.material_index].material
        if material is None or material.name != "Tapis":
            continue
        for vertex_index in polygon.vertices:
            world = table.matrix_world @ mesh.vertices[vertex_index].co
            if world.z >= top_z - 0.01:
                x_values.append(world.x)
                y_values.append(world.y)
    if not x_values or not y_values:
        bbox = [table.matrix_world @ mathutils.Vector(corner) for corner in table.bound_box]
        x_values = [point.x for point in bbox]
        y_values = [point.y for point in bbox]
    return max(abs(value) for value in x_values), max(abs(value) for value in y_values), top_z


EXPORT_DIR.mkdir(parents=True, exist_ok=True)
BLEND_OUTPUT.parent.mkdir(parents=True, exist_ok=True)

root = ensure_collection("BillardAssets")
tables = ensure_collection("Table", root)
balls = ensure_collection("Balls", root)
cues = ensure_collection("Cues", root)
props = ensure_collection("Props", root)
helpers = ensure_collection("GameplayHelpers", root)
reference = ensure_collection("ReferenceScene", root)

table = bpy.data.objects["Cube"]
table.name = "PoolTable"
move_to_collection(table, tables)

cue_a = bpy.data.objects["Cylindre.001"]
cue_a.name = "CueStick_A"
move_to_collection(cue_a, cues)

cue_b = bpy.data.objects["Cylindre.002"]
cue_b.name = "CueStick_B"
move_to_collection(cue_b, cues)

rack = bpy.data.objects["Plan"]
rack.name = "RackTriangle"
move_to_collection(rack, props)

ball_names = [
    "CueBall_Source",
    "Ball_01_Source",
    "Ball_02_Source",
    "Ball_03_Source",
    "Ball_04_Source",
    "Ball_05_Source",
    "Ball_06_Source",
    "Ball_07_Source",
    "Ball_08_Source",
    "Ball_09_Source",
    "Ball_10_Source",
    "Ball_11_Source",
    "Ball_12_Source",
    "Ball_13_Source",
    "Ball_14_Source",
    "Ball_15_Source",
]

for original_name, target_name in zip(
    [
        "Sphère",
        "Sphère.001",
        "Sphère.002",
        "Sphère.003",
        "Sphère.004",
        "Sphère.005",
        "Sphère.006",
        "Sphère.007",
        "Sphère.008",
        "Sphère.009",
        "Sphère.010",
        "Sphère.011",
        "Sphère.012",
        "Sphère.013",
        "Sphère.014",
        "Sphère.017",
    ],
    ball_names,
):
    obj = bpy.data.objects[original_name]
    obj.name = target_name
    move_to_collection(obj, balls)

for original_name in [
    "Cube.001",
    "Plan.001",
    "Plan.002",
    "Plan.003",
    "Plan.004",
    "Plan.005",
    "Plan.006",
    "Plan.007",
]:
    obj = bpy.data.objects.get(original_name)
    if obj is not None:
        move_to_collection(obj, reference)

felt_half_x, felt_half_y, felt_top_z = felt_extents(table)
pocket_inset_x = felt_half_x * 0.965
pocket_inset_y = felt_half_y * 0.965

create_empty("Pocket_Corner_NW", mathutils.Vector((-pocket_inset_x, pocket_inset_y, felt_top_z)), helpers)
create_empty("Pocket_Corner_NE", mathutils.Vector((pocket_inset_x, pocket_inset_y, felt_top_z)), helpers)
create_empty("Pocket_Corner_SW", mathutils.Vector((-pocket_inset_x, -pocket_inset_y, felt_top_z)), helpers)
create_empty("Pocket_Corner_SE", mathutils.Vector((pocket_inset_x, -pocket_inset_y, felt_top_z)), helpers)
create_empty("Pocket_Side_W", mathutils.Vector((-felt_half_x * 0.988, 0.0, felt_top_z)), helpers)
create_empty("Pocket_Side_E", mathutils.Vector((felt_half_x * 0.988, 0.0, felt_top_z)), helpers)

create_empty("CueBallSpawn_Helper", mathutils.Vector((0.0, -felt_half_y * 0.37, felt_top_z)), helpers)
create_empty("RackCenter_Helper", mathutils.Vector((0.0, felt_half_y * 0.18, felt_top_z)), helpers)

table_export_objects = [table] + [obj for obj in helpers.objects if obj.name.startswith("Pocket_")]
export_objects(EXPORT_DIR / "PoolTable_Source.fbx", table_export_objects)
export_objects(EXPORT_DIR / "CueStick_A_Source.fbx", [cue_a])
export_objects(EXPORT_DIR / "CueStick_B_Source.fbx", [cue_b])
export_objects(EXPORT_DIR / "RackTriangle_Source.fbx", [rack])
export_objects(EXPORT_DIR / "PoolBalls_Source.fbx", [bpy.data.objects[name] for name in ball_names])

bpy.ops.wm.save_as_mainfile(filepath=str(BLEND_OUTPUT))
print(f"Saved organized source blend to {BLEND_OUTPUT}")
