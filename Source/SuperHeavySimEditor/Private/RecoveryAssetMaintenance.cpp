#include "RecoveryAssetMaintenance.h"
#include "EditorReimportHandler.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl,SuperHeavySimEditor)

bool URecoveryAssetMaintenance::ReimportAsset(UObject* Asset, const FString& SourceFile)
{
    if (!Asset || !FPaths::FileExists(SourceFile)) return false;
    return FReimportManager::Instance()->Reimport(Asset, false, false,
        FPaths::ConvertRelativePathToFull(SourceFile), nullptr, INDEX_NONE, false, true);
}
