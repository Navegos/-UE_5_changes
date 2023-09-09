// Copyright Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class RigLogicLib : ModuleRules
	{
		public RigLogicLib(ReadOnlyTargetRules Target) : base(Target)
		{
			IWYUSupport = IWYUSupport.None;

			if ((Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Mac || Target.Platform == UnrealTargetPlatform.Linux || Target.Platform == UnrealTargetPlatform.Android) && Target.Architecture == UnrealArch.X64)
			{
				if (Target.MinCpuArchX64 >= MinimumCpuArchitectureX64.AVX)
				{
					PublicDefinitions.AddRange(
						new string[]
						{
							"RL_BUILD_WITH_AVX=1",
							"RL_BUILD_WITH_SSE=1",
							"RL_USE_HALF_FLOATS=1", // This on safe to use all AVX2 CPU's has support for F16C.
							"TRIMD_ENABLE_AVX=1",
							"TRIMD_ENABLE_SSE=1",
							"TRIMD_ENABLE_F16C=1", // This on safe to use all AVX2 CPU's has support for F16C.
						}
					);
				}
				else
				{
					PublicDefinitions.AddRange(
						new string[]
						{
							"RL_BUILD_WITH_SSE=1",
							//"RL_USE_HALF_FLOATS=1", // This on SSE revamps minimal DefaultCPU version to AMD(R) BobcatV2|+ or AMD(R) BulldozerV2|+, Intel(R) Ivybridge or Intel(R) Haswell|+.
							"TRIMD_ENABLE_SSE=1",
							//"TRIMD_ENABLE_F16C=1", // This on SSE revamps minimal DefaultCPU version to AMD(R) BobcatV2|+ or AMD(R) BulldozerV2|+, Intel(R) Ivybridge or Intel(R) Haswell|+.
						}
					);
				}
			}

			if (Target.Architecture == UnrealArch.Arm64)
			{
				if (Target.WindowsPlatform.Compiler.IsMSVC())
				{
					PublicDefinitions.AddRange(
						new string[]
						{
							"RL_BUILD_WITH_NEON=1",
							"TRIMD_ENABLE_NEON=1",
							// MSVC misses FP16 instructions.
						}
					);
				}
				else
				{
					PublicDefinitions.AddRange(
						new string[]
						{
							"RL_BUILD_WITH_NEON=1",
							"TRIMD_ENABLE_NEON=1",
							"RL_USE_HALF_FLOATS=1",
							"TRIMD_ENABLE_NEON_FP16=1",
						}
					);
				}
			}

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
						"Core",
						"CoreUObject",
						"Engine",
						"ControlRig"
				}
			);

			if (Target.Type == TargetType.Editor)
			{
				PublicDependencyModuleNames.Add("UnrealEd");
				PublicDependencyModuleNames.Add("EditorFramework");
			}

			Type = ModuleType.CPlusPlus;

			if (Target.LinkType != TargetLinkType.Monolithic)
			{
				PrivateDefinitions.Add("RL_BUILD_SHARED=1");
				PublicDefinitions.Add("RL_SHARED=1");
			}

			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				PrivateDefinitions.Add("TRIO_WINDOWS_FILE_MAPPING_AVAILABLE=1");
				PrivateDefinitions.Add("TRIO_CUSTOM_WINDOWS_H=\"WindowsPlatformUE.h\"");
			}

			if (Target.Platform == UnrealTargetPlatform.Linux || Target.Platform == UnrealTargetPlatform.LinuxArm64)
			{
				PrivateDefinitions.Add("TRIO_MREMAP_AVAILABLE=1");
			}

			if (Target.Platform == UnrealTargetPlatform.Linux ||
					Target.Platform == UnrealTargetPlatform.LinuxArm64 ||
					Target.Platform == UnrealTargetPlatform.Mac)
			{
				PrivateDefinitions.Add("TRIO_MMAP_AVAILABLE=1");
			}

			PrivateDefinitions.Add("RL_AUTODETECT_SSE=1");
			PublicDefinitions.Add("RL_BUILD_WITH_ML_EVALUATOR=1");
		}
	}
}
