// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PantherJamGame : ModuleRules
{
	public PantherJamGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"PantherJamGame",
			"PantherJamGame/Variant_Platforming",
			"PantherJamGame/Variant_Combat",
			"PantherJamGame/Variant_Combat/AI",
			"PantherJamGame/Variant_SideScrolling",
			"PantherJamGame/Variant_SideScrolling/Gameplay",
			"PantherJamGame/Variant_SideScrolling/AI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
