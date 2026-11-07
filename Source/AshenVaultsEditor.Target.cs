using UnrealBuildTool;
using System.Collections.Generic;

public class AshenVaultsEditorTarget : TargetRules
{
    public AshenVaultsEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V2;
        ExtraModuleNames.Add("AshenVaults");
    }
}
