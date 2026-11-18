# DrawFight one-time setup. Runs at every editor start but only does work that is missing:
#  1) imports Data/*.csv as DataTables into /Game/DrawFight/Data
#  2) creates /Game/DrawFight/Maps/MainMenu with DFMenuGameMode
#  3) makes Lvl_ThirdPerson use DFArenaGameMode
import os
import unreal

DEST = "/Game/DrawFight/Data"
TABLES = {
    "DT_Characters": "DFCharacterRow", "DT_Skills": "DFSkillRow", "DT_WeaponClasses": "DFWeaponClassRow",
    "DT_Elements": "DFElementRow", "DT_Auras": "DFAuraRow", "DT_SecretClasses": "DFSecretClassRow",
    "DT_Passives": "DFPassiveRow", "DT_Perks": "DFPerkRow", "DT_EnemyKinds": "DFEnemyKindRow",
    "DT_Bosses": "DFBossRow", "DT_SignatureParts": "DFSignaturePartRow", "DT_SignatureKeywords": "DFSignatureKeywordRow",
}
MENU = "/Game/DrawFight/Maps/MainMenu"
ARENA = "/Game/ThirdPerson/Lvl_ThirdPerson"
log = lambda m: unreal.log("[DrawFight setup] " + m)


def import_tables():
    data_dir = os.path.join(unreal.Paths.project_dir(), "Data")
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    for name, struct in TABLES.items():
        path = DEST + "/" + name
        csv = os.path.join(data_dir, name + ".csv")
        if unreal.EditorAssetLibrary.does_asset_exist(path) or not os.path.exists(csv):
            continue
        row = unreal.load_object(None, "/Script/DrawFight." + struct)
        if not row:
            log("row struct missing: " + struct)
            continue
        settings = unreal.CSVImportSettings()
        settings.set_editor_property("import_row_struct", row)
        settings.set_editor_property("import_type", unreal.CSVImportType.ECSV_DATA_TABLE)
        factory = unreal.CSVImportFactory()
        factory.set_editor_property("automated_import_settings", settings)
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", csv)
        task.set_editor_property("destination_path", DEST)
        task.set_editor_property("destination_name", name)
        task.set_editor_property("automated", True)
        task.set_editor_property("save", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("factory", factory)
        tools.import_asset_tasks([task])
        log("imported " + name)


def set_game_mode(level_path, cls_path):
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not les.load_level(level_path):
        return False
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    gm = unreal.load_class(None, cls_path)
    ws = world.get_world_settings()
    if ws.get_editor_property("default_game_mode") != gm:
        ws.set_editor_property("default_game_mode", gm)
        les.save_current_level()
        log("game mode set on " + level_path)
    return True


def setup_levels():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not unreal.EditorAssetLibrary.does_asset_exist(MENU):
        if les.new_level(MENU):
            les.save_current_level()
            log("created " + MENU)
    set_game_mode(MENU, "/Script/DrawFight.DFMenuGameMode")
    set_game_mode(ARENA, "/Script/DrawFight.DFArenaGameMode")


_handle = None
_ticks = [0]


def _run(dt):
    global _handle
    _ticks[0] += 1
    if _ticks[0] < 120:  # let the editor finish loading
        return
    unreal.unregister_slate_post_tick_callback(_handle)
    try:
        import_tables()
        setup_levels()
        log("done")
    except Exception as e:
        unreal.log_error("[DrawFight setup] " + str(e))


_handle = unreal.register_slate_post_tick_callback(_run)
