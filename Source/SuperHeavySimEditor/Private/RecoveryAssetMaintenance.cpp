#include "RecoveryAssetMaintenance.h"
#include "EditorReimportHandler.h"
#include "Misc/Paths.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "K2Node_InputAction.h"
#include "K2Node_InputAxisEvent.h"
#include "K2Node_InputKey.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl,SuperHeavySimEditor)

int32 URecoveryAssetMaintenance::RemoveLegacyInputEvents(UBlueprint* Blueprint)
{
    if(!Blueprint)return 0;
    TArray<UEdGraph*> Graphs;Blueprint->GetAllGraphs(Graphs);int32 Removed=0;
    for(auto* Graph:Graphs)
    {
        const auto Nodes=Graph->Nodes;
        for(UEdGraphNode* Node:Nodes)
            if(Node && (Node->IsA<UK2Node_InputAction>() || Node->IsA<UK2Node_InputAxisEvent>() || Node->IsA<UK2Node_InputKey>()))
            {FBlueprintEditorUtils::RemoveNode(Blueprint,Node,true);++Removed;}
    }
    if(Removed)FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    return Removed;
}

bool URecoveryAssetMaintenance::ReimportAsset(UObject* Asset, const FString& SourceFile)
{
    if (!Asset || !FPaths::FileExists(SourceFile)) return false;
    return FReimportManager::Instance()->Reimport(Asset, false, false,
        FPaths::ConvertRelativePathToFull(SourceFile), nullptr, INDEX_NONE, false, true);
}
