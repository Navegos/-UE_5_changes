// Copyright(c) 2020, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#pragma once

#if WITH_MDL_SDK

#include "MDLDependencies.h"
#include "MDLParameter.h"
#include "Engine/Texture.h"

typedef TFunction<void(class UTexture*&, const FString&, float Gamma, TextureCompressionSettings Compression)> FLoadTextureCallback;

class MDL_API IMDLMaterialImporter
{
public:

	virtual bool ImportMaterial(class UMaterial* Material, const mi::base::Handle<const mi::neuraylib::IMaterial_definition>& InMaterialDefinition, const mi::base::Handle<const mi::neuraylib::ICompiled_material>& InCompiledMaterial, class UMaterialExpressionClearCoatNormalCustomOutput*& OutClearCoatNormalCustomOutput, FLoadTextureCallback InCallback = nullptr) = 0;
	virtual bool ImportDistilledMaterial(class UMaterial* Material, const mi::base::Handle<const mi::neuraylib::IMaterial_definition>& InMaterialDefinition, const mi::base::Handle<const mi::neuraylib::ICompiled_material>& InCompiledMaterial, class UMaterialExpressionClearCoatNormalCustomOutput*& OutClearCoatNormalCustomOutput, FLoadTextureCallback InCallback = nullptr) = 0;
	virtual bool IsDistillOff(const mi::base::Handle<const mi::neuraylib::IMaterial_definition>& InMaterialDefinition) = 0;
	virtual const TArray<FString>& GetLastInvalidFunctionCalls() = 0;
	virtual ~IMDLMaterialImporter() {}
};

#endif
