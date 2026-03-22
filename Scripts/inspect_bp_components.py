import unreal


bp = unreal.EditorAssetLibrary.load_asset("/Game/BP_MyCharacter")
if not bp:
    unreal.log_error("Could not load /Game/BP_MyCharacter")
    raise SystemExit(1)

unreal.log(f"Blueprint type: {type(bp)}")

for prop_name in ("simple_construction_script", "component_templates"):
    try:
        value = bp.get_editor_property(prop_name)
        unreal.log(f"{prop_name}: {value}")
    except Exception as exc:
        unreal.log_warning(f"{prop_name}: {exc}")

try:
    scs = bp.get_editor_property("simple_construction_script")
    if scs:
        nodes = scs.get_all_nodes()
        unreal.log(f"SCS node count: {len(nodes)}")
        for node in nodes:
            try:
                component_template = node.get_editor_property("component_template")
            except Exception:
                component_template = None
            try:
                parent = node.get_editor_property("parent_component_or_variable_name")
            except Exception:
                parent = None
            try:
                attach_name = node.get_editor_property("attach_to_name")
            except Exception:
                attach_name = None
            unreal.log(
                f"node={node.get_editor_property('variable_name')} "
                f"template={component_template} "
                f"parent={parent} attach={attach_name}"
            )
except Exception as exc:
    unreal.log_error(f"Inspect SCS failed: {exc}")
