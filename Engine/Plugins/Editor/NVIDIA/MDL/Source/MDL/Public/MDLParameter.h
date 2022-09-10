// Copyright(c) 2020, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#pragma once
#include "CoreMinimal.h"
#include "MDLParameter.generated.h"

UENUM()
enum class EMdlValueType : uint8
{
	MDL_UNKNOWN,
	MDL_BOOL,
	MDL_INT,
	MDL_FLOAT,
	MDL_FLOAT2,
	MDL_FLOAT3,
	MDL_FLOAT4,
	MDL_COLOR,
	MDL_TEXTURE,
};

USTRUCT()
struct MDL_API FMDLParameterInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	EMdlValueType	ParameterType;

	UPROPERTY()
	FString ParameterName;

	UPROPERTY()
	FString DisplayName;
};

USTRUCT()
struct MDL_API FMDLParametersList
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FMDLParameterInfo> ParametersList;
};