using UnrealBuildTool;

public class AshenVaults : ModuleRules
{
    public AshenVaults(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "InputCore", "Paper2D"
        });
    }
}
