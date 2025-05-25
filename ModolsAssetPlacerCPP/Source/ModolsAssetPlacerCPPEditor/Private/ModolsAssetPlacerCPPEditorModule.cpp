#include "ModolsAssetPlacerCPPEditorModule.h"
#include "SModolsAssetPlacerWindow.h" // Our new Slate widget
#include "ToolMenus.h"
#include "LevelEditor.h" // For FLevelEditorModule for menu
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h" // For FMenuBuilder
// #include "Interfaces/IMainFrameModule.h" // If docking to main frame - Not strictly needed for NomadTab
#include "WorkspaceMenuStructure.h" // For placement in Window menu
#include "WorkspaceMenuStructureModule.h" // For FWorkspaceItem
#include "Styling/AppStyle.h" // For FSlateIcon / FAppStyle

// DEFINE_LOG_CATEGORY_STATIC(LogModolsAssetPlacer, Log, All); // Alternative if only used in this .cpp
// If LogModolsAssetPlacer is used across multiple .cpp files in your module, define it once (e.g. here)
// and DECLARE_LOG_CATEGORY_EXTERN in the .h file as done.
#include "ModolsAssetPlacerCPPEditorModule.h" // For the LogModolsAssetPlacer declaration
DEFINE_LOG_CATEGORY(LogModolsAssetPlacer);


#define LOCTEXT_NAMESPACE "FModolsAssetPlacerCPPEditorModule"

// Define TabName (consistent with .h)
const FName FModolsAssetPlacerCPPEditorModule::ModolsAssetPlacerTabName = TEXT("ModolsAssetPlacerTab");

// Unique ID for the tab spawner
static const FName ModolsAssetPlacerWindowTabId("ModolsAssetPlacerWindow");

void FModolsAssetPlacerCPPEditorModule::StartupModule()
{
    UE_LOG(LogModolsAssetPlacer, Log, TEXT("ModolsAssetPlacerCPPEditor module has started."));

    PluginCommands = MakeShareable(new FUICommandList);
    // No commands to map yet, but good to have the list

    // Register the tab spawner
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ModolsAssetPlacerWindowTabId, FOnSpawnTab::CreateRaw(this, &FModolsAssetPlacerCPPEditorModule::OnSpawnPluginTab))
        .SetDisplayName(LOCTEXT("ModolsAssetPlacerTabTitle", "Modols Asset Placer"))
        .SetMenuType(ETabSpawnerMenuType::Enabled) // Changed to Enabled to show in Window menu by default if not explicitly added
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.PlaceActors")) // Example icon
        .SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory());

    RegisterMenus();
}

void FModolsAssetPlacerCPPEditorModule::ShutdownModule()
{
    UE_LOG(LogModolsAssetPlacer, Log, TEXT("ModolsAssetPlacerCPPEditor module has shut down."));
    
    UToolMenus::UnregisterOwner(this); // Unregister menu owner
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ModolsAssetPlacerWindowTabId);
    PluginCommands.Reset();
}

TSharedRef<SDockTab> FModolsAssetPlacerCPPEditorModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab) // Can be docked anywhere
        [
            SNew(SModolsAssetPlacerWindow) // Our custom widget
        ];
}

void FModolsAssetPlacerCPPEditorModule::PluginButtonClicked()
{
    FGlobalTabmanager::Get()->TryInvokeTab(ModolsAssetPlacerWindowTabId);
}

void FModolsAssetPlacerCPPEditorModule::RegisterMenus()
{
    // Owner will be used for cleanup in ShutdownModule
    FToolMenuOwnerScoped OwnerScoped(this);

    // Add menu entry to "Tools" menu
    UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
    {
        FToolMenuSection& Section = ToolsMenu->AddSection("ModolsAssetPlacerTools", LOCTEXT("ModolsAssetPlacerToolsSection", "Modols Tools"));
        Section.AddMenuEntryWithCommandList(
            FUIAction(FExecuteAction::CreateRaw(this, &FModolsAssetPlacerCPPEditorModule::PluginButtonClicked)),
            PluginCommands, // No actual commands in list yet, but FUIAction needs it
            LOCTEXT("ModolsAssetPlacerMenuEntry", "Modols Asset Placer"),
            LOCTEXT("ModolsAssetPlacerMenuEntry_ToolTip", "Open the Modols Asset Placer tool window."),
            FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.PlaceActors") // Example icon
        );
    }

    // Also add to Window -> Developer Tools menu for discoverability
    // The tab spawner registration with .SetMenuType(ETabSpawnerMenuType::Enabled) and .SetGroup() 
    // should automatically add it to the Window menu under the specified group.
    // Explicitly adding it again might be redundant or provide finer control if needed.
    // For now, relying on the tab spawner's group registration.
    // If it doesn't show up, uncomment and adapt this:
    /*
    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
    {
        FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout"); // Or a more appropriate section
        Section.AddMenuEntry(
            "ModolsAssetPlacerWindowEntry", // Unique name for this menu entry
            LOCTEXT("ModolsAssetPlacerWindowMenuEntry", "Modols Asset Placer"),
            LOCTEXT("ModolsAssetPlacerWindowMenuEntry_ToolTip", "Opens the Modols Asset Placer window."),
            FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.PlaceActors"),
            FUIAction(FExecuteAction::CreateRaw(this, &FModolsAssetPlacerCPPEditorModule::PluginButtonClicked))
        );
    }
    */
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FModolsAssetPlacerCPPEditorModule, ModolsAssetPlacerCPPEditor)
