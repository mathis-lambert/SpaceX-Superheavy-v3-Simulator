using UnrealBuildTool;
public class SuperHeavySimEditor : ModuleRules
{
    public SuperHeavySimEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]{"Core","CoreUObject","Engine"});
        PrivateDependencyModuleNames.AddRange(new[]{"UnrealEd","BlueprintGraph","Kismet"});
    }
}
