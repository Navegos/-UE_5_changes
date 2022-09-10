// Copyright(c) 2020, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#include "MDLMaterialExporter.h"
#include "MDLExporterUtility.h"
#include "Materials/Material.h"
#include "MDLOutputLogger.h"

UMDLMaterialExporter::UMDLMaterialExporter()
{
	bText = true;
	SupportedClass = UMaterialInterface::StaticClass();
	FormatExtension.Add(TEXT("mdl"));
	FormatDescription.Add(TEXT("Material Definition Language"));
}

bool UMDLMaterialExporter::ExportText(const class FExportObjectInnerContext* Context, UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn, uint32 PortFlags)
{
	auto Material = CastChecked<UMaterialInterface>(Object);

	if (Material)
	{
		FString OutputMDL;
		FString NewName = FPaths::GetBaseFilename(UMDLMaterialExporter::CurrentFilename);
		if (FMDLExporterUtility::IsLegalIdentifier(NewName))
		{
			FMDLExporterSetting Setting = {NewName, true, true};
			FMDLExporterUtility::ExportMDL(Material, Setting, OutputMDL);
		}
		else
		{
			UE_LOG(LogMDLOutput, Error, TEXT("MDL name %s was not legal"), *NewName);
		}

		Ar.Log(*OutputMDL);
	}

	return true;
}
