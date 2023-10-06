add_requires("skyrim-commonlib")

target("SkyUnit.Plugin")
    add_files("SkyUnit.Plugin.cpp")
    add_deps("SkyUnit", "SkyrimScripting.Plugin")
    add_packages("skyrim-commonlib")
    add_rules("@skyrim-commonlib/plugin", {
        mod_folders = os.getenv("SKYRIM_SCRIPTING_MOD_FOLDERS")
    })
