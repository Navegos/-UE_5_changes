// Copyright(c) 2020, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#include "TextureExporterDDS.h"
#include "DDSUtils.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureCube.h"
#include "Engine/Texture2DDynamic.h"

UTextureExporterDDS::UTextureExporterDDS(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UTexture::StaticClass();
	PreferredFormatIndex = 0;
	FormatExtension.Add(TEXT("dds"));
	FormatDescription.Add(TEXT("DirectDraw Surface"));
}

bool UTextureExporterDDS::SupportsObject(UObject* Object) const
{
	bool bSupportsObject = false;
	if (Super::SupportsObject(Object))
	{
		if (Object->IsA<UTexture2D>() || Object->IsA<UTextureCube>() || Object->IsA<UTexture2DDynamic>())
		{
			const EPixelFormat PixelFormat = Object->IsA<UTexture2D>() ? Cast<UTexture2D>(Object)->GetPixelFormat() : (Object->IsA<UTextureCube>() ? Cast<UTextureCube>(Object)->GetPixelFormat() : Cast<UTexture2DDynamic>(Object)->Format); 

			if (PixelFormat == PF_R8_UINT
			// G8
			|| PixelFormat == PF_G8
			// RG8
			|| PixelFormat == PF_R8G8
			|| PixelFormat == PF_R8G8B8A8
			|| PixelFormat == PF_R8G8B8A8_UINT
			|| PixelFormat == PF_R8G8B8A8_SNORM
				// BGRA8
			|| PixelFormat == PF_B8G8R8A8
			|| PixelFormat == PF_FloatRGBA
			// DXT1 (BC1)
			|| PixelFormat == PF_DXT1
			|| PixelFormat == PF_DXT3
			|| PixelFormat == PF_DXT5
			|| PixelFormat == PF_BC4
			|| PixelFormat == PF_BC5
			|| PixelFormat == PF_BC6H
			|| PixelFormat == PF_BC7
				)
			{
				bSupportsObject = true;
			}
		}
	}
	return bSupportsObject;
}

bool UTextureExporterDDS::ExportBinary( UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn, int32 FileIndex, uint32 PortFlags )
{
	UTexture* Texture = CastChecked<UTexture>( Object );

	return FDDSUtils::ExportToDDS(Texture, Ar);
}
