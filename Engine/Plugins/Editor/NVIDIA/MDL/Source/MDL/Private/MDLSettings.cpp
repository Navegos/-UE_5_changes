// Copyright(c) 2020, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#include "MDLSettings.h"

UMDLSettings::UMDLSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bImportOnlyFirstMaterialPerFile(false)
	, bAutoSaveMaterialFunctions(true)
	, bWorldAlignedTextures(false)
	, bInstanceCompilation(false)
	, bDistillation(true)
	, DistillationTarget(EDistillationTarget::UE4)
	, FluxDivider(50.0f)
	, bUseDisplayNameForParameter(true)
	, BakedTextureHeight(1024)
	, BakedTextureWidth(1024)
	, BakedTextureSamples(16)
	, MetersPerSceneUnit(0.01f)
{}
