// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using EpicGames.Core;

namespace UnrealBuildTool.Rules
{
	public class RigLogicLibTest : ModuleRules
	{
		public RigLogicLibTest(ReadOnlyTargetRules Target) : base(Target)
		{
			//bUseUnity = false; // A windows include is preprocessing some method names causing compile failures.
			//bDisableStaticAnalysis = true;
			
			if ((Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Mac || Target.Platform == UnrealTargetPlatform.Linux || Target.Platform == UnrealTargetPlatform.Android) && Target.Architecture == UnrealArch.X64)
			{
				if (Target.MinCpuArchX64 >= MinimumCpuArchitectureX64.AVX)
				{
					PublicDefinitions.AddRange(
						new string[]
						{
							"RL_BUILD_WITH_AVX=1",
							"RL_BUILD_WITH_SSE=1",
							//"RL_USE_HALF_FLOATS=1", // This on safe to use all AVX2 CPU's has support for F16C.
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

			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				PrivateDefinitions.Add("RL_BUILD_WITH_SSE=1");
				PublicDefinitions.Add("GTEST_OS_WINDOWS=1");
			}

			string RigLogicLibPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../RigLogicLib"));

			if (Target.LinkType == TargetLinkType.Monolithic)
			{
				PublicDependencyModuleNames.Add("RigLogicLib");
				PrivateIncludePaths.Add(Path.Combine(RigLogicLibPath, "Private"));
			}
			else
			{
				PrivateDefinitions.Add("RIGLOGIC_MODULE_DISCARD");
				ConditionalAddModuleDirectory(new DirectoryReference(RigLogicLibPath));
			}

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"GoogleTest"
				}
			);
		}
	}
}
