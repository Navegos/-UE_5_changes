// Copyright(c) 2020, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#pragma once
#include "CoreMinimal.h"
#include "Engine/Texture2DDynamic.h"

class MDL_API IDynamicMipsLoader
{
public:
	virtual ~IDynamicMipsLoader()
	{
	}

	virtual void GetMipData(UTexture2DDynamic* Texture2DDynamic, int32 FirstMipToLoad, void** OutMipData) = 0;

	virtual bool GetUncompressedMipData(UTexture2DDynamic* Texture2DDynamic, int32 MipIndex, TArray64<uint8>& OutMipData, ETextureSourceFormat& OutFormat) = 0;
};

