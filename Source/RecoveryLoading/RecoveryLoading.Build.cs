using UnrealBuildTool;
public class RecoveryLoading : ModuleRules
{
    public RecoveryLoading(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]{"Core","Slate","SlateCore"});
        PrivateDependencyModuleNames.AddRange(new[]{"PreLoadScreen","CoreUObject"});
    }
}
