// Copyright 2023 Dev Levy. All Rights Reserved.

using UnrealBuildTool;

public class AdvancedOutlineSystem : ModuleRules
{
	public AdvancedOutlineSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"GameplayTags",
				"CoreUObject",
				"Engine"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				"NetCore",
			}
			);
		
	}
}