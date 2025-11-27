using UnrealBuildTool;

public class UEGSRuntime : ModuleRules
{
    public UEGSRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Projects" ,"RenderCore" ,"Renderer" ,"RHICore","RHI","ExtendedGraphicsProgramming" });

		PrivateIncludePaths.AddRange(new string[] {
			System.IO.Path.Combine(GetModuleDirectory("Renderer"), "Private"),
		});
    }
}