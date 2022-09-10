// Copyright(c) 2020, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#pragma once
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Engine/EngineTypes.h"
#include "MDLSettings.generated.h"

struct MaterialData
{
	FString			MaterialName;
	TArray<FString>	ParameterNames;
	FString			Parameters;
	FString			Annotations;
};

struct ModuleData
{
	FString			Name;
	TSet<FString>	Imports;
};


UENUM()
enum class EDistillationTarget
{
	Diffuse,
	DiffuseGlossy,
	UE4
};

UCLASS(MinimalAPI, config = Engine, defaultconfig)
class UMDLSettings : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(config, EditAnywhere, Category = Settings)
	bool bImportOnlyFirstMaterialPerFile;

	UPROPERTY(config, EditAnywhere, Category = Settings)
	bool bAutoSaveMaterialFunctions;

// #if defined(USE_WORLD_ALIGNED_TEXTURES)
	UPROPERTY(config, EditAnywhere, Category = Settings)
	bool bWorldAlignedTextures;
// #endif

	UPROPERTY(config, EditAnywhere, Category = Settings)
	bool bInstanceCompilation;

	UPROPERTY(config, EditAnywhere, Category = Settings)
	bool bDistillation;

	UPROPERTY(config, EditAnywhere, Category = Settings, meta = (editcondition = "bDistillation"))
	EDistillationTarget DistillationTarget;

	UPROPERTY(config, EditAnywhere, Category = Settings)
	float FluxDivider;

	UPROPERTY()
	bool bUseDisplayNameForParameter;

	UPROPERTY(config, EditAnywhere, Category = Settings, meta = (FilePathFilter = "mdl"))
	FFilePath WrapperMaterial;

	UPROPERTY(config, EditAnywhere, Category = Settings)
	int BakedTextureHeight;

	UPROPERTY(config, EditAnywhere, Category = Settings)
	int BakedTextureWidth;

	UPROPERTY(config, EditAnywhere, Category = Settings)
	int BakedTextureSamples;

	UPROPERTY(config, EditAnywhere, Category = Settings)
	float MetersPerSceneUnit;
};
