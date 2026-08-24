// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class NavigationGrid : ModuleRules
{
	public NavigationGrid(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", 
			"CoreUObject", 
			"Engine",
			"InputCore",
			"UnrealEd",
			"AIModule",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
		});

	}
}
