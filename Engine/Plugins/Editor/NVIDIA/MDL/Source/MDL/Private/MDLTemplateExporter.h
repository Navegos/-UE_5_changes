// Copyright(c) 2020, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#pragma once
#include "CoreMinimal.h"
#include "MDLExporterUtility.h"

class FMDLTemplateExporter
{
public:
	FMDLTemplateExporter(class UMaterialInterface* InMaterial, const FMDLExporterSetting& InSetting, FExportTextureCallback InCallback);
	bool ExportModule(FString& Output, struct FMDLTranslatorResult* OutResult);

private:
	void ExportHeader();
	void ExportImports(bool Extension17);
	void ExportAnnotationsDefine();
	void ExportMaterial();
	void ExportParamters();
	void ExportLetBlock();
	void ExportModuleBlock();
	void ExportAnnotation();
	void ExportCustomExpressions();
	bool Translate(struct FMDLTranslatorResult& OutResult);
	FMDLTranslatorResult* IsCached(const FStaticParameterSet& InStaticParamSet);
	void UpdateExportedMaterial(const FMDLTranslatorResult& InResult);
	void AssembleLetExpressions();
	void RemoveUnusedVariables(struct FMDLTranslatorResult& TranslatorResult);

private:
	class UMaterialInterface* OriginalMaterial;
	FMDLExporterSetting ExporterSetting;

	TArray<FString> InputCodes;
	TArray<FString> LocalVariables;
	TArray<FString> InputVariables;
	FString NormalCode;
	FString NormalAssignment;
	FString VertexMembersSetupAndAssignments;
	FString PixelMembersSetupAndAssignments;
	FString CustomExpressions;
	FString CustomVSOutputCode;
	FString CustomPSOutputCode;
	FString LetExpressions;

	FString UsedTemplate;
	
	FString ExportedMDLString; // Main export mdl
	FExportTextureCallback ExportTextureCallback;
};

