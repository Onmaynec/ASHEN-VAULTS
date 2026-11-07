using UnrealBuildTool;
using System.Collections.Generic;

public class AshenVaultsTarget : TargetRules
{
    public AshenVaultsTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V2;
        ExtraModuleNames.Add("AshenVaults");
    }
}
