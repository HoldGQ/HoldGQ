#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Toolkits/IToolkit.h" // Required for FExtensibilityManager / FSpawnTabArgs
#include "Logging/LogMacros.h" // For DECLARE_LOG_CATEGORY_EXTERN

DECLARE_LOG_CATEGORY_EXTERN(LogModolsAssetPlacer, Log, All);

class FToolBarBuilder;
class FMenuBuilder;
class SDockTab; // Forward declaration

class FModolsAssetPlacerCPPEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    // Method to summon the plugin window
    void PluginButtonClicked();

private:
    void RegisterMenus();
    TSharedRef<SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

    TSharedPtr<class FUICommandList> PluginCommands;
    static const FName ModolsAssetPlacerTabName; // Changed to ModolsAssetPlacerWindowTabId in .cpp, ensure consistency
};
