#include "SModolsAssetPlacerWindow.h"
#include "ModolsAssetPlacerCPPEditorModule.h" // For LogModolsAssetPlacer
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SScrollBox.h" // If content might overflow
#include "EditorStyleSet.h" // For FEditorStyle
#include "Styling/AppStyle.h" // For FAppStyle for icons etc.
#include "AssetRegistry/AssetRegistryModule.h" // For IAssetRegistry
#include "ARFilter.h" // For FARFilter
#include "Engine/StaticMesh.h" // For UStaticMesh::StaticClass()
#include "Editor.h" // For GEditor
#include "ActorEditorUtils.h" // For FActorEditorUtils::CreateOrGetActorFolder
#include "Folder.h" // For FFolder::GetWorldRootFolder
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h" // For UStaticMeshComponent
#include "EngineUtils.h" // For TActorIterator
// Note: EditorLevelUtils.h or EditorScriptingUtilities is not strictly needed if using World->SpawnActor directly.
// Editor.h is already included, which is needed for GEditor and FScopedTransaction

#define LOCTEXT_NAMESPACE "SModolsAssetPlacerWindow"

const FName SModolsAssetPlacerWindow::SpawnedActorTag = TEXT("ModolsAssetPlacerSpawned");

void SModolsAssetPlacerWindow::Construct(const FArguments& InArgs)
{
    RootPath = TEXT("/Game/Modols/"); // Default path

    ChildSlot
    [
        SNew(SVerticalBox)
        // --- Root Path Input --- (Uncommented and enabled)
        +SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            SNew(SHorizontalBox)
            +SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(0,0,5,0)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock).Text(LOCTEXT("RootPathLabel", "Root Path:"))
            ]
            +SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                SAssignNew(RootPathEditableTextBox, SEditableTextBox)
                .Text(FText::FromString(RootPath))
                .OnTextCommitted(this, &SModolsAssetPlacerWindow::OnRootPathTextCommitted)
                .ToolTipText(LOCTEXT("RootPathTooltip", "Enter the root path to search for Modols, e.g., /Game/Modols/ (Press Enter to commit)"))
            ]
        ]

        // --- Process Button ---
        +SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            SNew(SButton)
            .HAlign(HAlign_Center)
            .Text(LOCTEXT("ProcessButtonText", "Process Modols Folder"))
            .OnClicked(this, &SModolsAssetPlacerWindow::OnProcessButtonClicked)
            .ToolTipText(LOCTEXT("ProcessButtonTooltip", "Iterates assets in the Modols folder, creates Outliner structure, and places meshes."))
        ]

        // --- Clear Button --- (Uncommented and enabled)
        +SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            SNew(SButton)
            .HAlign(HAlign_Center)
            .Text(LOCTEXT("ClearButtonText", "Clear Spawned Modols Actors"))
            .OnClicked(this, &SModolsAssetPlacerWindow::OnClearActorsButtonClicked)
            .ToolTipText(LOCTEXT("ClearButtonTooltip", "Removes actors previously spawned by this tool (identified by a specific tag)."))
        ]
        
        // --- Status Message Area ---
        +SVerticalBox::Slot()
        .FillHeight(1.0f) // Make it fill available space
        .Padding(5)
        [
            SNew(SScrollBox) // In case messages are long
            +SScrollBox::Slot()
            [
                SAssignNew(StatusTextBlock, STextBlock)
                .Text(LOCTEXT("InitialStatus", "Ready."))
                .AutoWrapText(true)
            ]
        ]
    ];
}

FReply SModolsAssetPlacerWindow::OnProcessButtonClicked()
{
    // Clear previous status first
    SetStatusMessage(LOCTEXT("Processing", "Processing... Please wait."), false);
    
    // It's good practice to yield for a frame to allow Slate to update the UI (e.g. show "Processing...")
    // This can be done by scheduling the work for the next tick, but for simplicity now, direct call.
    PerformAssetProcessing(); 

    return FReply::Handled();
}

TArray<FAssetData> SModolsAssetPlacerWindow::GetStaticMeshAssets(const FString& PathToSearch)
{
    TArray<FAssetData> OutAssetData;
    // SetStatusMessage(FText::Format(LOCTEXT("SearchingAssets", "Searching for Static Meshes in: {0}..."), FText::FromString(PathToSearch))); 
    // This message might be too quick, better to set status before calling this, and after it returns.

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    FARFilter Filter;
    Filter.PackagePaths.Add(FName(*PathToSearch));
    Filter.bRecursivePaths = true;
    // Using ClassPaths with FTopLevelAssetPath for more robust filtering in UE5+
    Filter.ClassPaths.Add(UStaticMesh::StaticClass()->GetClassPathName());

    // Good practice to check if registry is still discovering assets, warn if so.
    if (AssetRegistry.IsLoadingAssets())
    {
        UE_LOG(LogModolsAssetPlacer, Warning, TEXT("Asset Registry is still discovering assets. Results from GetAssets might be incomplete."));
        // Optionally: SetStatusMessage(LOCTEXT("AssetRegistryBusy", "Asset Registry is busy. Results may be incomplete."), true);
        // Depending on desired behavior, could even defer processing or return empty. For now, proceed and log.
    }
    AssetRegistry.GetAssets(Filter, OutAssetData);

    UE_LOG(LogModolsAssetPlacer, Log, TEXT("AssetRegistry Search: Found %d static meshes in '%s'."), OutAssetData.Num(), *PathToSearch);
    // Storing in member variable as per header file change
    FoundStaticMeshAssets = OutAssetData; // This member is updated here.
         
    return OutAssetData; // Return local copy, member is also updated.
}

void SModolsAssetPlacerWindow::PerformAssetProcessing()
{
    UE_LOG(LogModolsAssetPlacer, Log, TEXT("PerformAssetProcessing called. Root path: '%s'"), *RootPath);
    SetStatusMessage(FText::Format(LOCTEXT("ProcessingPath", "Processing path: {0}..."), FText::FromString(RootPath)), false);
    
    if (RootPath.IsEmpty())
    {
        SetStatusMessage(LOCTEXT("Error_RootPathEmpty", "Error: Root path cannot be empty. Please specify a path (e.g., /Game/Modols/)."), true);
        UE_LOG(LogModolsAssetPlacer, Error, TEXT("Root path is empty. Aborting processing."));
        return;
    }
    // Basic validation for typical content paths
    if (!RootPath.StartsWith(TEXT("/Game/")) && !RootPath.StartsWith(TEXT("/Engine/")) && !RootPath.StartsWith(TEXT("/Plugin/")))
    {
		SetStatusMessage(LOCTEXT("Error_RootPathInvalid", "Error: Root path must start with /Game/, /Engine/, or /Plugin/."), true);
		UE_LOG(LogModolsAssetPlacer, Error, TEXT("Root path '%s' is invalid. Aborting processing."), *RootPath);
		return;
    }

    // Clear previously found assets from any prior run. FoundStaticMeshAssets is a member.
    FoundStaticMeshAssets.Empty(); 
    GetStaticMeshAssets(RootPath); // This will populate FoundStaticMeshAssets

    if (FoundStaticMeshAssets.Num() == 0)
    {
        // If Asset Registry was busy, a warning might have been set. Otherwise, it's just "0 found".
        if (!StatusTextBlock->GetColorAndOpacity().IsColorSet() || StatusTextBlock->GetColorAndOpacity().Get().ToFColor(false) != FLinearColor::Red)
        {
            SetStatusMessage(FText::Format(LOCTEXT("NoAssetsFoundToProcess", "No Static Meshes found in '{0}'."), FText::FromString(RootPath)), false);
        }
        UE_LOG(LogModolsAssetPlacer, Warning, TEXT("No Static Meshes found in '%s' to process further."), *RootPath);
        return;
    }
    
    UE_LOG(LogModolsAssetPlacer, Log, TEXT("Assets to be processed for folder creation and placement (%d total):"), FoundStaticMeshAssets.Num());
    for (const FAssetData& Asset : FoundStaticMeshAssets)
    {
        UE_LOG(LogModolsAssetPlacer, Log, TEXT("  - %s"), *Asset.GetObjectPathString());
    }
    
    CreateOutlinerFolders(FoundStaticMeshAssets); 
    PlaceStaticMeshActors(FoundStaticMeshAssets); 

    SetStatusMessage(FText::Format(LOCTEXT("ProcessingFullyComplete", "Processing complete. Processed {0} assets. Folders created and actors placed. See log for details."), FText::AsNumber(FoundStaticMeshAssets.Num())), false);
    UE_LOG(LogModolsAssetPlacer, Log, TEXT("Modols Asset Placer processing finished for path '%s'."), *RootPath);
}

void SModolsAssetPlacerWindow::CreateOutlinerFolders(const TArray<FAssetData>& AssetDataList)
{
    if (!GEditor)
    {
        SetStatusMessage(LOCTEXT("Error_NoGEditor_Folders", "Error: GEditor is not available. Cannot create Outliner folders."), true);
        UE_LOG(LogModolsAssetPlacer, Error, TEXT("GEditor is null. Cannot create Outliner folders."));
        return;
    }
    UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
    if (!EditorWorld)
    {
        SetStatusMessage(LOCTEXT("Error_NoWorld_Folders", "Error: Editor world is not available. Cannot create Outliner folders."), true);
        UE_LOG(LogModolsAssetPlacer, Error, TEXT("EditorWorld is null. Cannot create Outliner folders."));
        return;
    }
    
    UE_LOG(LogModolsAssetPlacer, Log, TEXT("Starting Outliner folder creation..."));
    // SetStatusMessage(LOCTEXT("CreatingFoldersLog", "Creating Outliner folders..."), false); // This might be too quick / overwritten by next status.
    TSet<FName> CreatedFolderPathsSet; 
    int FoldersProcessedCount = 0; 

    FString CleanRootPath = RootPath; // RootPath is already validated by PerformAssetProcessing
    // Ensure CleanRootPath ends with a slash for consistent path manipulation.
    // RootPath is validated by PerformAssetProcessing, so it's not empty here.
    if (!CleanRootPath.EndsWith(TEXT("/"))) 
    {
        CleanRootPath += TEXT("/");
    }

    for (const FAssetData& Asset : AssetDataList)
    {
        FString AssetPackagePath = Asset.PackagePath.ToString(); 
        
        if (!AssetPackagePath.StartsWith(CleanRootPath))
        {
            UE_LOG(LogModolsAssetPlacer, Warning, TEXT("Asset package path '%s' is not under the specified RootPath '%s'. Skipping folder creation for it."), *AssetPackagePath, *CleanRootPath);
            continue;
        }

        FString RelativeAssetFolderPath = AssetPackagePath;
        RelativeAssetFolderPath.RightChopInline(CleanRootPath.Len()); 
        
        if (RelativeAssetFolderPath.IsEmpty())
        {
            UE_LOG(LogModolsAssetPlacer, Log, TEXT("Asset '%s' is directly under RootPath. No sub-folder needed in Outliner from this asset's package path."), *Asset.GetObjectPathString());
            continue; 
        }

        RelativeAssetFolderPath.TrimCharInline(TEXT('/'), &RelativeAssetFolderPath);
        if (RelativeAssetFolderPath.IsEmpty()){ continue; }

        FName FolderPathFName(*RelativeAssetFolderPath);
        if (!CreatedFolderPathsSet.Contains(FolderPathFName))
        {
            // TODO: Consider if CreateOrGetActorFolder should be wrapped in a try-catch for robustness, though it's generally safe.
            bool bFolderCreatedOrExisted = FActorEditorUtils::CreateOrGetActorFolder(EditorWorld, FolderPathFName, FFolder::GetWorldRootFolder(EditorWorld));

            if (bFolderCreatedOrExisted)
            {
                UE_LOG(LogModolsAssetPlacer, Log, TEXT("Ensured Outliner folder exists: %s"), *RelativeAssetFolderPath);
                CreatedFolderPathsSet.Add(FolderPathFName);
                FoldersProcessedCount++; 
            }
            else
            {
                UE_LOG(LogModolsAssetPlacer, Warning, TEXT("Failed to create or get Outliner folder: %s. Subsequent actors might not be placed in the intended folder."), *RelativeAssetFolderPath);
                // Optionally update UI to reflect this specific failure, though the summary at the end might be enough.
            }
        }
    }
    // Update status message at the end of this specific operation.
    // The overall success/failure is handled by PerformAssetProcessing.
    UE_LOG(LogModolsAssetPlacer, Log, TEXT("Outliner folder creation process finished. Processed %d unique folder paths."), FoldersProcessedCount);
    // SetStatusMessage(FText::Format(LOCTEXT("FoldersCreatedSummary", "Folder creation: {0} unique paths processed."), FText::AsNumber(FoldersProcessedCount)), false); // This would be too quick
}

void SModolsAssetPlacerWindow::PlaceStaticMeshActors(const TArray<FAssetData>& AssetDataList)
{
    if (!GEditor)
    {
        SetStatusMessage(LOCTEXT("Error_NoGEditor_Placement", "Error: GEditor is not available. Cannot place actors."), true);
        UE_LOG(LogModolsAssetPlacer, Error, TEXT("GEditor is null. Cannot place actors."));
        return;
    }
    UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
    if (!EditorWorld)
    {
        SetStatusMessage(LOCTEXT("Error_NoWorld_Placement", "Error: Editor world is not available. Cannot place actors."), true);
        UE_LOG(LogModolsAssetPlacer, Error, TEXT("EditorWorld is null. Cannot place actors."));
        return;
    }

    int ActorsPlacedCount = 0;
    FVector NextSpawnLocation = FVector::ZeroVector; 
    // Consider adding a small configurable offset for each actor if placing many, e.g., from UI.
    // float SpawnOffsetIncrement = 100.0f; // Example value

    UE_LOG(LogModolsAssetPlacer, Log, TEXT("Starting actor placement process..."));
    // SetStatusMessage(LOCTEXT("PlacingActorsLog", "Placing Static Mesh actors into the level..."), false); // This might be too quick / overwritten by next status.

    for (const FAssetData& Asset : AssetDataList)
    {
        UObject* LoadedObject = Asset.GetAsset(); 
        UStaticMesh* StaticMeshToPlace = Cast<UStaticMesh>(LoadedObject);

        if (!StaticMeshToPlace)
        {
            UE_LOG(LogModolsAssetPlacer, Warning, TEXT("Asset.GetAsset() for '%s' did not return a UStaticMesh. Attempting explicit LoadObject."), *Asset.GetObjectPathString());
            StaticMeshToPlace = LoadObject<UStaticMesh>(nullptr, *Asset.GetObjectPathString());
            
            if (!StaticMeshToPlace) {
               UE_LOG(LogModolsAssetPlacer, Warning, TEXT("Failed to load StaticMesh '%s' after explicit LoadObject. Skipping this asset."), *Asset.GetObjectPathString());
               SetStatusMessage(FText::Format(LOCTEXT("Error_LoadAssetFailed", "Failed to load asset: {0}. Skipping."), FText::FromString(Asset.AssetName.ToString())), true); // Update UI for this specific failure
               continue;
            }
        }

        FActorSpawnParameters SpawnParams;
        SpawnParams.Name = MakeUniqueObjectName(EditorWorld, AStaticMeshActor::StaticClass(), StaticMeshToPlace->GetFName()); // Ensure unique name
        SpawnParams.NameMode = FActorSpawnParameters::ESpawnNameMode::Requested; // Try to use the name if possible

        AStaticMeshActor* SpawnedActor = EditorWorld->SpawnActor<AStaticMeshActor>(NextSpawnLocation, FRotator::ZeroRotator, SpawnParams);
        if (!SpawnedActor)
        {
            UE_LOG(LogModolsAssetPlacer, Error, TEXT("Failed to spawn AStaticMeshActor for StaticMesh: %s"), *StaticMeshToPlace->GetPathName());
            SetStatusMessage(FText::Format(LOCTEXT("Error_SpawnActorFailed", "Failed to spawn actor for: {0}. Skipping."), FText::FromString(StaticMeshToPlace->GetName())), true);
            continue;
        }
        
        UStaticMeshComponent* SMComponent = SpawnedActor->GetStaticMeshComponent();
        if (!SMComponent)
        {
            UE_LOG(LogModolsAssetPlacer, Error, TEXT("Spawned AStaticMeshActor '%s' has no StaticMeshComponent. Destroying actor."), *SpawnedActor->GetActorLabel());
            SpawnedActor->Destroy(); // Clean up malformed actor
            SetStatusMessage(FText::Format(LOCTEXT("Error_NoSMComponent", "Actor for {0} missing component. Skipping."), FText::FromString(StaticMeshToPlace->GetName())), true);
            continue;
        }

        SMComponent->SetStaticMesh(StaticMeshToPlace);
        SMComponent->SetMobility(EComponentMobility::Static); 
        SpawnedActor->SetActorLabel(StaticMeshToPlace->GetName()); 
        SpawnedActor->Tags.Add(SpawnedActorTag); // Add the tag
        UE_LOG(LogModolsAssetPlacer, Log, TEXT("Tagged actor '%s' with '%s'"), *SpawnedActor->GetActorLabel(), *SpawnedActorTag.ToString());

        FString AssetPackagePath = Asset.PackagePath.ToString();
        FString RelativeAssetFolderPath = AssetPackagePath; 
        
        FString CleanedRootPath = RootPath; // RootPath already validated by PerformAssetProcessing
        if (!CleanedRootPath.EndsWith(TEXT("/"))) { CleanedRootPath += TEXT("/"); }

        if (AssetPackagePath.StartsWith(CleanedRootPath))
        {
            RelativeAssetFolderPath.RightChopInline(CleanedRootPath.Len());
        }
        else 
        {
            // This case should ideally not happen if GetStaticMeshAssets only returns assets under RootPath.
            // However, if it does, or if RootPath logic changes, this is a fallback.
            RelativeAssetFolderPath.Empty();
            UE_LOG(LogModolsAssetPlacer, Warning, TEXT("Asset '%s' (package path '%s') is not directly under the cleaned RootPath '%s'. Actor will be placed at Outliner root."), *Asset.GetObjectPathString(), *AssetPackagePath, *CleanedRootPath);
        }
        
        RelativeAssetFolderPath.TrimCharInline(TEXT('/'), &RelativeAssetFolderPath);

        if (!RelativeAssetFolderPath.IsEmpty())
        {
            // TODO: Consider if SetFolderPath should be wrapped in try-catch, though it's generally safe.
            SpawnedActor->SetFolderPath(FName(*RelativeAssetFolderPath));
            UE_LOG(LogModolsAssetPlacer, Log, TEXT("Placed actor '%s' into Outliner folder: '%s'"), *SpawnedActor->GetActorLabel(), *RelativeAssetFolderPath);
        }
        else
        {
            SpawnedActor->SetFolderPath(NAME_None); 
            UE_LOG(LogModolsAssetPlacer, Log, TEXT("Placed actor '%s' at Outliner root."), *SpawnedActor->GetActorLabel());
        }
        
        ActorsPlacedCount++;
        // NextSpawnLocation.X += SpawnOffsetIncrement; // Example for offsetting multiple actors
    }
    // Update status message at the end of this specific operation.
    // The overall success/failure is handled by PerformAssetProcessing.
    UE_LOG(LogModolsAssetPlacer, Log, TEXT("Actor placement process finished. Placed %d actors."), ActorsPlacedCount);
    // SetStatusMessage(FText::Format(LOCTEXT("ActorsPlacedSummary", "Actor placement: {0} actors placed."), FText::AsNumber(ActorsPlacedCount)), false); // This would be too quick
}


// void SModolsAssetPlacerWindow::OnRootPathTextCommitted(const FText& InText, ETextCommit::Type InCommitType)
// { // Uncommented and enabled
//     FString NewPath = InText.ToString();
//     if (!NewPath.StartsWith(TEXT("/Game/")) && !NewPath.StartsWith(TEXT("/Engine/")) && !NewPath.StartsWith(TEXT("/Plugin/")))
//     {
//         // Basic validation - could be more robust, e.g. disallow empty paths or paths with spaces
//         SetStatusMessage(FText::Format(LOCTEXT("Error_InvalidPathPrefix", "Error: Path '{0}' must start with /Game/, /Engine/, or /Plugin/."), FText::FromString(NewPath)), true);
//         UE_LOG(LogModolsAssetPlacer, Warning, TEXT("Invalid root path prefix entered: %s"), *NewPath);
//         if(RootPathEditableTextBox.IsValid()) // Revert to old valid path
//         {
//             RootPathEditableTextBox->SetText(FText::FromString(RootPath));
//         }
//         return;
//     }
//     RootPath = NewPath;
//     SetStatusMessage(FText::Format(LOCTEXT("PathUpdated", "Root path updated to: {0}"), FText::FromString(RootPath)), false);
//     UE_LOG(LogModolsAssetPlacer, Log, TEXT("Root path updated to: %s by user input."), *RootPath);
// }
 
FReply SModolsAssetPlacerWindow::OnClearActorsButtonClicked()
{
    if (!GEditor || !GEditor->GetEditorWorldContext().World())
    {
        SetStatusMessage(LOCTEXT("Error_NoWorld_Clear", "Error: Cannot access editor world to clear actors."), true);
        UE_LOG(LogModolsAssetPlacer, Error, TEXT("Cannot access GEditor or its world for actor clearing."));
        return FReply::Handled();
    }

    UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
    int ClearedActorsCount = 0;

    SetStatusMessage(LOCTEXT("ClearingActors", "Searching for and clearing tagged actors..."), false);
    UE_LOG(LogModolsAssetPlacer, Log, TEXT("Clear Spawned Actors button clicked. Searching for actors with tag: %s"), *SpawnedActorTag.ToString());

    // It's good practice to use an undo transaction for deletions
    const FScopedTransaction Transaction(LOCTEXT("ClearModolsActorsTransaction", "Clear Modols Placed Actors"));

    for (TActorIterator<AActor> It(EditorWorld); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor && Actor->ActorHasTag(SpawnedActorTag))
        {
            UE_LOG(LogModolsAssetPlacer, Log, TEXT("Found tagged actor: %s. Deleting."), *Actor->GetActorLabel());
            EditorWorld->DestroyActor(Actor);
            ClearedActorsCount++;
        }
    }

    if (ClearedActorsCount > 0)
    {
        // Refresh editor viewports after deletion
        if(GEditor) GEditor->RedrawLevelEditingViewports(true);
        SetStatusMessage(FText::Format(LOCTEXT("ActorsCleared", "Successfully cleared {0} tagged actors."), FText::AsNumber(ClearedActorsCount)));
        UE_LOG(LogModolsAssetPlacer, Log, TEXT("Cleared %d actors with tag %s."), ClearedActorsCount, *SpawnedActorTag.ToString());
    }
    else
    {
        SetStatusMessage(LOCTEXT("NoActorsToClear", "No actors found with the specified tag to clear."));
        UE_LOG(LogModolsAssetPlacer, Log, TEXT("No actors found with tag %s."), *SpawnedActorTag.ToString());
    }
    
    return FReply::Handled();
}

void SModolsAssetPlacerWindow::SetStatusMessage(const FText& Message, bool bIsError)
{
    if (StatusTextBlock.IsValid())
    {
        StatusTextBlock->SetText(Message);
        StatusTextBlock->SetColorAndOpacity(bIsError ? FSlateColor(FLinearColor::Red) : FSlateColor(FLinearColor::White));
    }
}

#undef LOCTEXT_NAMESPACE
