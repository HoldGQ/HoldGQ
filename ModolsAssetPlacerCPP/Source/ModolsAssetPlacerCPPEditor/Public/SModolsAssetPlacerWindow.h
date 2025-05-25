#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SButton.h" // For FReply
#include "Widgets/Input/SEditableTextBox.h" // For SEditableTextBox (even if optional now)
#include "Widgets/Text/STextBlock.h" // For STextBlock
#include "AssetRegistry/AssetData.h" // For FAssetData
// #include "Engine/StaticMeshActor.h" // Forward declare or include if needed, but likely covered by .cpp
// #include "Engine/StaticMesh.h"      // Forward declare or include if needed, but likely covered by .cpp

class SModolsAssetPlacerWindow : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SModolsAssetPlacerWindow) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    // Handler for the process button click
    FReply OnProcessButtonClicked();
    
    // (Optional Enhancement) Handler for clear button
    FReply OnClearActorsButtonClicked();

    // (Optional Enhancement) Handler for path text committed
    void OnRootPathTextCommitted(const FText& InText, ETextCommit::Type InCommitType);
    
    void PerformAssetProcessing(); 
    TArray<FAssetData> GetStaticMeshAssets(const FString& Path);
    void CreateOutlinerFolders(const TArray<FAssetData>& AssetDataList);
    void PlaceStaticMeshActors(const TArray<FAssetData>& AssetDataList);

    // (Optional Enhancement) Root path for asset searching
    TSharedPtr<SEditableTextBox> RootPathEditableTextBox;
    FString RootPath; // Default to /Game/Modols/

    // Status message text block
    TSharedPtr<STextBlock> StatusTextBlock;

    // Helper to update status message
    void SetStatusMessage(const FText& Message, bool bIsError = false);

    // (Optional, but good for future steps) Add a member to store found assets:
    TArray<FAssetData> FoundStaticMeshAssets; 

    static const FName SpawnedActorTag; 
};
