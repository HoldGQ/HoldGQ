using UnrealBuildTool;

public class ModolsAssetPlacerCPPEditor : ModuleRules
{
    public ModolsAssetPlacerCPPEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(new string[] { /* ... */ });
        PrivateIncludePaths.AddRange(new string[] { /* ... */ });

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "Projects" // Needed for IPluginManager
                // ... add other public dependencies as needed
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "InputCore", // Often needed with Slate
                "UnrealEd",  // For editor functionalities, GEditor
                "ToolMenus", // For adding menu items
                "EditorStyle", // For standard editor styles/icons
                "AssetRegistry",
                "LevelEditor", // For FLevelEditorModule, if used for menu extension
                "EditorScriptingUtilities" // If using UEditorLevelLibrary etc.
                // ... add other private dependencies as needed
            }
        );

        DynamicallyLoadedModuleNames.AddRange(new string[] { /* ... */ });
    }
}
