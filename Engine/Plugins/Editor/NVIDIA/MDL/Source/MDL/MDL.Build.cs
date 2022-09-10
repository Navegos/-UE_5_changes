// Copyright(c) 2020, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

using System;
using System.IO;

namespace UnrealBuildTool.Rules
{
    public class MDL : ModuleRules
    {
        public MDL(ReadOnlyTargetRules Target) : base(Target)
        {
            PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

            PrivateDependencyModuleNames.AddRange(
                new string[] {
					"Engine",
                    "Projects",
					"Core",
					"CoreUObject",
					"Engine",
					"RHI",
					"RenderCore",
					"InputCore",
					"UnrealEd",
					"MaterialEditor",
				}
            );


            //string MDLSDKPath = Environment.GetEnvironmentVariable("MDL_SDK_PATH");
            string ThirdPartyPath = Path.Combine(ModuleDirectory, "..", "ThirdParty");
            string MDLSDKPath = Path.Combine(ThirdPartyPath, "mdl_sdk");

            if (String.IsNullOrEmpty(MDLSDKPath))
            {
                PublicDefinitions.Add("WITH_MDL_SDK=0");
                return;
            }
            else
            {
                PublicDefinitions.Add("WITH_MDL_SDK=1");
            }
            //PublicDefinitions.Add("ADD_CLIP_MASK");
            PublicDefinitions.Add("USE_WORLD_ALIGNED_TEXTURES");
            PublicDefinitions.Add("USE_WAT_AS_SCALAR");

            PublicIncludePaths.Add(Path.Combine(MDLSDKPath, "include"));
			PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Shaders/Shared"));

			string PlatformExtension;

            if (Target.Platform == UnrealTargetPlatform.Win64)
            {
                PlatformExtension = ".dll";
            }
            else
            {
                return;
            }

            string MDLLibPath = Path.Combine(MDLSDKPath, "nt-x86-64/lib");
            RuntimeDependencies.Add(Path.Combine(MDLLibPath, "libmdl_sdk" + PlatformExtension), StagedFileType.NonUFS);
            RuntimeDependencies.Add(Path.Combine(MDLLibPath, "nv_freeimage" + PlatformExtension), StagedFileType.NonUFS);
            RuntimeDependencies.Add(Path.Combine(MDLLibPath, "mdl_distiller" + PlatformExtension), StagedFileType.NonUFS);
        }
    }
}
