// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class StolenOath : ModuleRules
{
	public StolenOath(ReadOnlyTargetRules Target) : base(Target)
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
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"StolenOath",
			"StolenOath/Variant_Platforming",
			"StolenOath/Variant_Platforming/Animation",
			"StolenOath/Variant_Combat",
			"StolenOath/Variant_Combat/AI",
			"StolenOath/Variant_Combat/Animation",
			"StolenOath/Variant_Combat/Gameplay",
			"StolenOath/Variant_Combat/Interfaces",
			"StolenOath/Variant_Combat/UI",
			"StolenOath/Variant_SideScrolling",
			"StolenOath/Variant_SideScrolling/AI",
			"StolenOath/Variant_SideScrolling/Gameplay",
			"StolenOath/Variant_SideScrolling/Interfaces",
			"StolenOath/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
