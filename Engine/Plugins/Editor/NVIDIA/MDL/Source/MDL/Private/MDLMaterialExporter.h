// Copyright(c) 2020, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#pragma once
#include "CoreMinimal.h"
#include "Exporters/Exporter.h"
#include "MDLMaterialExporter.generated.h"

UCLASS()
class UMDLMaterialExporter : public UExporter
{
	GENERATED_BODY()

	UMDLMaterialExporter();

	//~ Begin UExporter Interface
	virtual bool ExportText(const class FExportObjectInnerContext* Context, UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn, uint32 PortFlags = 0)override;
	//~ End UExporter Interface
};