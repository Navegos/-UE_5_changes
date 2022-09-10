// Copyright(c) 2020, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#pragma once
#include "CoreMinimal.h"
#include "MDLParameter.h"
#include "MDLDependencies.h"
#include "MDLImporter.h"

typedef TFunction<void(const FString&, const FString&, float Gamma)> FLoadInstanceTextureCallback;

class MDL_API FMDLImporterUtility
{
public:
	static const FString GetProjectFunctionPath()
	{
		static FString FunctionPath(TEXT("/Game/MDL/Functions"));
		return FunctionPath;
	}

	static const FString GetProjectMaterialPath()
	{
		static FString FunctionPath(TEXT("/Game/MDL"));
		return FunctionPath;
	}

	static const FString GetProjectBaseModulePath()
	{
		static FString FunctionPath(TEXT("/Game/MDL/Base"));
		return FunctionPath;
	}

	static bool IsBaseModule(const FString& Name);
	static class UMaterialInterface* GetBaseMaterial(class UMaterialInterface* MaterialInterface);
	static class UMaterialInterface* FindBaseModule(const FString& FileName, const FString& MaterialName);
	static class UMaterialInterface* LoadBaseModule(const FString& FileName, const FString& MaterialName = TEXT(""));
	static void UpdateBaseModuleParameters(const FString& Name);
	static bool GetDisplayNameFromBaseModule(const FString& MaterialName, const FString& ParameterName, FString& DisplayName);
	static bool GetMdlParameterTypeAndNameFromBaseModule(const FString& InMaterialName, const FString& InDisplayName, EMdlValueType& ValueType, FString& ParameterName);
	static bool GetBaseModuleByMaterialName(const FString& MaterialName, FString& ModuleName);
	static bool GetBaseModuleByMaterial(const class UMaterialInterface* MaterialInterface, FString& ModuleName);
	static void CreateInstanceFromBaseMDL(class UMaterialInstanceConstant* MaterialInstance, const mi::base::Handle<const mi::neuraylib::IMaterial_definition>& MaterialDefinition, FLoadInstanceTextureCallback InCallback = nullptr);

	static bool FindDisplayNameByParameterName(const mi::base::Handle<const mi::neuraylib::IMaterial_definition>& MaterialDefinition, const char* ParameterName, FString& DisplayName);
	static void UpdateParametersSheet(const mi::base::Handle<const mi::neuraylib::IMaterial_definition>& MaterialDefinition, const mi::base::Handle<const mi::neuraylib::ICompiled_material>& CompiledMaterial, FMDLParametersList& List);

	static TMap<FString, struct FMDLParametersList> BaseParametersSheet;
	static TMap<FString, FString> BaseMaterialToModuleList;

	static class UMaterialFunction* LoadMDLFunction(const FString& AssetPath, const FString& AssetName, int32 ArraySize = 0);
	static void LoadMaterialGraphDefinitions();
	static void UnloadMaterialGraphDefinitions();
	static bool CreateMdlInstance(const FString& ModuleName, const FString& IdentifierName, const FString& InstanceName);
	static bool SetCall(const FString& InstanceTarget, const FString& ParameterName, const FString& InstanceCall);
    static bool DistillMaterialInstance(class UMaterial* Material, const FString& MaterialInstanceName, bool bUseDisplayName = true);
    static bool DistillCompiledMaterial(UMaterial* Material, const mi::base::Handle<const mi::neuraylib::IMaterial_definition>& MaterialDefinition, const mi::base::Handle<const mi::neuraylib::ICompiled_material>& CompiledMaterial, bool bUseDisplayName = true, TArray<FString>* ErrorFunctionCalls = nullptr, FLoadTextureCallback InCallback = nullptr);
	static void ClearMaterial(UMaterial* Material);
	static FString GetDistillerTargetName();
	static FString GetPrototype(const FString& MaterialName);
};