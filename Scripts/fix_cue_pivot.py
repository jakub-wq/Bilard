import os
from pathlib import Path

import bpy
import mathutils


PROJECT_ROOT = Path(os.environ.get("PROJECT_ROOT", Path(__file__).resolve().parents[1]))
BLEND_PATH = PROJECT_ROOT / "SourceArt" / "Billard_split.blend"
EXPORT_DIR = PROJECT_ROOT / "SourceArt" / "Exports"


def move_origin_to_local_z_end(obj: bpy.types.Object, use_max: bool = True) -> None:
    if obj.type != "MESH" or not obj.data.vertices:
        return

    target_z = max(vertex.co.z for vertex in obj.data.vertices) if use_max else min(vertex.co.z for vertex in obj.data.vertices)
    local_offset = mathutils.Vector((0.0, 0.0, target_z))

    # Shift mesh data so the object origin lands on the chosen cue end while
    # keeping the visible cue in the same world-space position.
    obj.data.transform(mathutils.Matrix.Translation(-local_offset))
    obj.matrix_world.translation = obj.matrix_world @ local_offset


def export_object(filepath: Path, obj: bpy.types.Object) -> None:
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(
        filepath=str(filepath),
        use_selection=True,
        object_types={"MESH"},
        apply_unit_scale=True,
        bake_space_transform=False,
        add_leaf_bones=False,
        mesh_smooth_type="FACE",
        path_mode="AUTO",
    )


EXPORT_DIR.mkdir(parents=True, exist_ok=True)

for object_name, export_name in (
    ("CueStick_A", "CueStick_A_Source.fbx"),
    ("CueStick_B", "CueStick_B_Source.fbx"),
):
    cue = bpy.data.objects[object_name]
    move_origin_to_local_z_end(cue, use_max=True)
    export_object(EXPORT_DIR / export_name, cue)
    print(f"Updated pivot and exported {export_name}")

bpy.ops.wm.save_mainfile(filepath=str(BLEND_PATH))
print(f"Saved updated blend to {BLEND_PATH}")
