// Copyright(c) 2020, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#pragma once

#if WITH_MDL_SDK
#include "MDLImporter.h"
#include "MDLDependencies.h"
#include "MaterialExpressions.h"


template <typename ReturnType, typename MDLType>
static ReturnType GetExpressionConstant(mi::base::Handle<mi::neuraylib::IExpression const> const& Expression)
{
	check(Expression->get_kind() == mi::neuraylib::IExpression::EK_CONSTANT);

	mi::base::Handle<mi::neuraylib::IValue const> const& Value = mi::base::make_handle(Expression.get_interface<mi::neuraylib::IExpression_constant const>()->get_value());
	check(Value.get_interface<MDLType const>());

	return Value.get_interface<MDLType const>()->get_value();
}

template <typename ReturnType, typename MDLType>
static TArray<ReturnType> GetExpressionConstant(mi::base::Handle<mi::neuraylib::IExpression_list const> ExpressionList)
{
	TArray<ReturnType> ReturnValues;
	for (mi::Size ExpressionListIndex = 0, ExpressionListSize = ExpressionList->get_size(); ExpressionListIndex < ExpressionListSize; ExpressionListIndex++)
	{
		ReturnValues.Add(GetExpressionConstant<ReturnType, MDLType>(mi::base::make_handle(ExpressionList->get_expression(ExpressionListIndex))));
	}
	return ReturnValues;
}

class UMDLMaterialFactory;

class FMDLMaterialImporter : public IMDLMaterialImporter
{
public:
	FMDLMaterialImporter(UMDLMaterialFactory* Factory);
	virtual ~FMDLMaterialImporter() {}

	UMaterial* CreateMaterial(UObject* InParent, FName InName, EObjectFlags Flags, FFeedbackContext* Warn) const;
	virtual bool ImportMaterial(UMaterial* Material, const mi::base::Handle<const mi::neuraylib::IMaterial_definition>& InMaterialDefinition, const mi::base::Handle<const mi::neuraylib::ICompiled_material>& InCompiledMaterial, UMaterialExpressionClearCoatNormalCustomOutput*& OutClearCoatNormalCustomOutput, FLoadTextureCallback InCallback = nullptr) override;
	virtual bool ImportDistilledMaterial(UMaterial* Material, const mi::base::Handle<const mi::neuraylib::IMaterial_definition>& InMaterialDefinition, const mi::base::Handle<const mi::neuraylib::ICompiled_material>& InCompiledMaterial, UMaterialExpressionClearCoatNormalCustomOutput*& OutClearCoatNormalCustomOutput, FLoadTextureCallback InCallback = nullptr) override;

	virtual bool IsDistillOff(const mi::base::Handle<const mi::neuraylib::IMaterial_definition>& InMaterialDefinition) override;
	virtual const TArray<FString>& GetLastInvalidFunctionCalls() override { return InvalidFunctionCalls; }
private:
	UTexture* LoadTexture(const FString& RelativePath, const mi::base::Handle<const mi::neuraylib::ITexture>& Texture, TextureCompressionSettings InCompression = TC_Default);
	UTexture* LoadTexture(const FString& Filename, const FString& AssetDir, const FString& AssetName, bool srgb = true, TextureCompressionSettings InCompression = TC_Default);
	TArray<FMaterialExpressionConnection> MakeFunctionCall(const FString& CallPath, const mi::base::Handle<const mi::neuraylib::IFunction_definition>& FunctionDefinition, int ArraySize, const FString& AssetNamePostfix, TArray<FMaterialExpressionConnection>& Inputs);

	TArray<FMaterialExpressionConnection> CreateExpression(const mi::base::Handle<const mi::neuraylib::ICompiled_material>& CompiledMaterial, const mi::base::Handle<const mi::neuraylib::IExpression>& MDLExpression, const FString& CallPath);
	TArray<FMaterialExpressionConnection> CreateExpressionConstant(const mi::base::Handle<const mi::neuraylib::IValue>& MDLConstant);
	TArray<FMaterialExpressionConnection> CreateExpressionFunctionCall(const mi::base::Handle<const mi::neuraylib::ICompiled_material>& CompiledMaterial, const mi::base::Handle<const mi::neuraylib::IExpression_direct_call>& MDLFunctionCall, const FString& CallPath);
	TArray<FMaterialExpressionConnection> CreateExpressionConstructorCall(const mi::base::Handle<const mi::neuraylib::IType>& MDLType, const TArray<FMaterialExpressionConnection>& Arguments);
	TArray<FMaterialExpressionConnection> CreateExpressionTemporary(const mi::base::Handle<const mi::neuraylib::ICompiled_material>& CompiledMaterial, const mi::base::Handle<const mi::neuraylib::IExpression_temporary>& MDLExpression, const FString& CallPath);

	TArray<FMaterialExpressionConnection> GetExpressionParameter(const mi::base::Handle<const mi::neuraylib::IExpression_parameter>& MDLExpression);

	mi::neuraylib::IValue::Kind ImportParameter(TArray<FMaterialExpressionConnection>& Parameter, FString Name, const mi::base::Handle<mi::neuraylib::IValue const>& Value, TextureCompressionSettings InCompression = TC_Default);
	void ImportParameters(const mi::base::Handle<const mi::neuraylib::IMaterial_definition>& MaterialDefinition, const mi::base::Handle<const mi::neuraylib::ICompiled_material>& CompiledMaterial);

	void SetClearCoatNormal(const FMaterialExpressionConnection& Base, const UMaterialExpression* Normal);
	void SetPropertiesFromAnnotation(const mi::base::Handle<const mi::neuraylib::IMaterial_definition>& InMaterialDefinition);

private:
	class FMDLModule* MDLModule;
	UObject* ParentPackage;
	const UMDLMaterialFactory* ParentFactory;
	UMaterial* CurrentUE4Material;
	UMaterialExpressionClearCoatNormalCustomOutput* CurrentClearCoatNormal;
	TArray<TArray<FMaterialExpressionConnection>> Parameters, Temporaries;
	FMaterialExpressionConnection CurrentNormalExpression;
	FMaterialExpressionConnection TranslucentOpacity;
	FMaterialExpressionConnection EmissiveOpacity;
	FMaterialExpressionConnection SubsurfaceColor;
	FMaterialExpressionConnection SubsurfaceOpacity;
	FMaterialExpressionConnection OpacityEnabled;

	UMaterialFunction* MakeFloat2;
	UMaterialFunction* MakeFloat3;
	UMaterialFunction* MakeFloat4;
	bool InGeometryExpression;
	FString PackagePath;
	FLoadTextureCallback LoadTextureCallback;
	TArray<FString> InvalidFunctionCalls;

#if defined(USE_WORLD_ALIGNED_TEXTURES)
	FMaterialExpressionConnection UseWorldAlignedTextureParameter;
#endif
};

extern UMaterialFunction* LoadFunction(const FString& AssetPath, const FString& AssetName, int32 ArraySize = 0);

#endif
