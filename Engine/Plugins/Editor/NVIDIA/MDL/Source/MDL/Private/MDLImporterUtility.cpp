// Copyright(c) 2020, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#include "MDLImporterUtility.h"
#include "MDLPrivateModule.h"
#include "MDLKeywords.h"
#include "NodeArrangement.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/SavePackage.h"
#include "MaterialExpressions.h"
#include "MDLMaterialImporter.h"
#include "MDLPathUtility.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceConstant.h"

bool FMDLImporterUtility::IsBaseModule(const FString& InFileName)
{
	FString BaseTemplateFile = FPaths::GetPath(InFileName) / FPaths::GetBaseFilename(InFileName);

	for (auto Name : BaseTemplates)
	{
		if (BaseTemplateFile == Name.ToString())
		{
			return true;
		}
	}

	for (auto Name : MaterialGraphTemplates)
	{
		if (BaseTemplateFile == Name.ToString())
		{
			return true;
		}
	}

	return false;
}

UMaterialInterface* FMDLImporterUtility::GetBaseMaterial(UMaterialInterface* MaterialInterface)
{
	if (MaterialInterface == nullptr)
	{
		return nullptr;
	}

	TArray<FString> BaseMaterials;
	BaseParametersSheet.GenerateKeyArray(BaseMaterials);

	auto CheckBaseMaterial = [&BaseMaterials](const UMaterialInterface* Material)
	{
		for (auto Name : BaseMaterials)
		{
			auto PackageName = GetProjectBaseModulePath() / Name;
			if (Name.StartsWith(TEXT("gltf")))
			{
				PackageName = GetProjectBaseModulePath() / TEXT("gltf") / Name;
			}
			if (FPackageName::DoesPackageExist(PackageName))
			{
				if (Material == LoadObject<UMaterialInterface>(nullptr, *PackageName))
				{
					return true;
				}
			}
		}

		return false;
	};

	auto CheckMaterial = MaterialInterface;
	// Check self at first
	if (CheckBaseMaterial(CheckMaterial))
	{
		return CheckMaterial;
	}

	if (MaterialInterface->IsA<UMaterialInstance>())
	{
		// Check parent
		CheckMaterial = Cast<UMaterialInstance>(MaterialInterface)->Parent;
		if (CheckBaseMaterial(CheckMaterial))
		{
			return CheckMaterial;
		}

		// Check root material
		CheckMaterial = Cast<UMaterialInstance>(MaterialInterface)->GetMaterial();
		if (CheckBaseMaterial(CheckMaterial))
		{
			return CheckMaterial;
		}
	}

	return nullptr;
}

UMaterialInterface* FMDLImporterUtility::FindBaseModule(const FString& FileName, const FString& MaterialName)
{
	FString LocalPackagePath = GetProjectBaseModulePath() / FPaths::GetPath(FileName) / MaterialName;
	return LoadObject<UMaterialInterface>(nullptr, *LocalPackagePath);
}

void FMDLImporterUtility::LoadMaterialGraphDefinitions()
{
	auto MDLPluginModule = FModuleManager::GetModulePtr<FMDLModule>("MDL");

	for (auto Name : MaterialGraphTemplates)
	{
		FString ModuleName = Name.ToString();
		ModuleName.ReplaceInline(TEXT("/"), TEXT("::"));
		if (!ModuleName.StartsWith(TEXT("::")))
		{
			ModuleName = TEXT("::") + ModuleName;
		}

		MDLPluginModule->LoadModule(ModuleName);
	}
}

void FMDLImporterUtility::UnloadMaterialGraphDefinitions()
{
	auto MDLPluginModule = FModuleManager::GetModulePtr<FMDLModule>("MDL");

	for (int32 Index = MaterialGraphTemplates.Num() - 1; Index >= 0; --Index)
	{
		FString ModuleName = MaterialGraphTemplates[Index].ToString();
		ModuleName.ReplaceInline(TEXT("/"), TEXT("::"));
		if (!ModuleName.StartsWith(TEXT("::")))
		{
			ModuleName = TEXT("::") + ModuleName;
		}
		MDLPluginModule->RemoveModule(ModuleName);		
	}

	MDLPluginModule->CommitAndCreateTransaction();
}

bool FMDLImporterUtility::SetCall(const FString& InstanceTarget, const FString& ParameterName, const FString& InstanceCall)
{
	auto MDLPluginModule = FModuleManager::GetModulePtr<FMDLModule>("MDL");
	mi::neuraylib::Argument_editor ArgumentEditor(MDLPluginModule->Transaction.get(), TCHAR_TO_UTF8(*InstanceTarget), MDLPluginModule->MDLFactory.get());
	if (!ArgumentEditor.is_valid())
	{
		return false;
	}

	mi::Sint32 Result = ArgumentEditor.set_call(TCHAR_TO_UTF8(*ParameterName), TCHAR_TO_UTF8(*InstanceCall));
	return Result == 0;
}

bool FMDLImporterUtility::CreateMdlInstance(const FString& ModuleName, const FString& IdentifierName, const FString& InstanceName)
{
	auto MDLPluginModule = FModuleManager::GetModulePtr<FMDLModule>("MDL");
	mi::base::Handle<const mi::neuraylib::IModule> MDLModule = MDLPluginModule->GetLoadedModule(ModuleName);
	if (!MDLModule.is_valid_interface())
	{
		return false;
	}
	
	FString MangledModuleName = MangleMdlPath(ModuleName);
	// get mdl definition
	mi::neuraylib::Definition_wrapper Definition = mi::neuraylib::Definition_wrapper(MDLPluginModule->Transaction.get(), TCHAR_TO_UTF8(*(TEXT("mdl") + MangledModuleName + TEXT("::") + IdentifierName)), MDLPluginModule->MDLFactory.get());

	if (!Definition.is_valid())
	{
		// MDL 1.7 need full DB name including parameters list.
		// Some SubIdentifier only defined the name without parameters, we should look for the full name
		FString DBName = TEXT("mdl") + MangledModuleName + TEXT("::") + IdentifierName;
		FString FullDBName;
		for(int32 FunctionIndex = 0; FunctionIndex < MDLModule->get_function_count(); ++FunctionIndex)
		{
			FString FunctionNameInModule = MDLModule->get_function(FunctionIndex);
			if (FunctionNameInModule.StartsWith(DBName))
			{
				FullDBName = FunctionNameInModule;
				break;
			}
		}

		if (FullDBName.IsEmpty())
		{
			for(int32 MaterialIndex = 0; MaterialIndex < MDLModule->get_material_count(); ++MaterialIndex)
			{
				FString MaterialNameInModule = MDLModule->get_material(MaterialIndex);
				if (MaterialNameInModule.StartsWith(DBName))
				{
					FullDBName = MaterialNameInModule;
					break;
				}
			}
		}

		if (!FullDBName.IsEmpty())
		{
			Definition = mi::neuraylib::Definition_wrapper(MDLPluginModule->Transaction.get(), TCHAR_TO_UTF8(*FullDBName), MDLPluginModule->MDLFactory.get());
		}

		if (!Definition.is_valid())
		{
			return false;
		}
	}

	// create default arguments for parameters without default
	mi::base::Handle<mi::neuraylib::IExpression_factory> ExpressionFactory(
		MDLPluginModule->MDLFactory->create_expression_factory(MDLPluginModule->Transaction.get()));
	mi::base::Handle<mi::neuraylib::IValue_factory> ValueFactory(
		MDLPluginModule->MDLFactory->create_value_factory(MDLPluginModule->Transaction.get()));
	mi::base::Handle<mi::neuraylib::IExpression_list> ExpressionList(
		ExpressionFactory->create_expression_list());
	mi::base::Handle<const mi::neuraylib::IExpression_list> Defaults(
		Definition.get_defaults());
	mi::base::Handle<const mi::neuraylib::IType_list> types(Definition.get_parameter_types());
	mi::Size Count = Definition.get_parameter_count();
	for (mi::Size Index = 0; Index < Count; ++Index)
	{
		const char *param_name = Definition.get_parameter_name(Index);

		mi::base::Handle<const mi::neuraylib::IExpression> DefaultExpression(
			Defaults->get_expression(param_name));
		if (DefaultExpression)
		{
			mi::base::Handle<const mi::neuraylib::IExpression> ClonedExpression(ExpressionFactory->clone<mi::neuraylib::IExpression>(DefaultExpression.get()));
			ExpressionList->add_expression(param_name, ClonedExpression.get());
		}
		else
		{
			mi::base::Handle<const mi::neuraylib::IType> Type(types->get_type(param_name));
			mi::base::Handle<mi::neuraylib::IValue> Value(ValueFactory->create(Type.get()));
			mi::base::Handle<const mi::neuraylib::IExpression> ConstantExpression(ExpressionFactory->create_constant(Value.get()));
			ExpressionList->add_expression(param_name, ConstantExpression.get());
		}
	}
	// instance definition
	mi::base::Handle<mi::neuraylib::IScene_element> Instance(Definition.create_instance(ExpressionList.get()));
	if (!Instance.is_valid_interface())
	{
		return false;
	}
	// store for later use
	mi::Sint32 Result = MDLPluginModule->Transaction->store(Instance.get(), TCHAR_TO_UTF8(*InstanceName));
	return Result == 0;
}

void FMDLImporterUtility::ClearMaterial(UMaterial* Material)
{
	Material->BaseColor.Expression = nullptr;
	Material->EmissiveColor.Expression = nullptr;
	Material->SubsurfaceColor.Expression = nullptr;
	Material->Roughness.Expression = nullptr;
	Material->Metallic.Expression = nullptr;
	Material->Specular.Expression = nullptr;
	Material->Opacity.Expression = nullptr;
	Material->Refraction.Expression = nullptr;
	Material->OpacityMask.Expression = nullptr;
	Material->ClearCoat.Expression = nullptr;
	Material->ClearCoatRoughness.Expression = nullptr;
	Material->Normal.Expression = nullptr;

	Material->Expressions.Empty();
}

FString FMDLImporterUtility::GetDistillerTargetName()
{
	UMDLSettings* Settings = GetMutableDefault<UMDLSettings>();
	switch (Settings->DistillationTarget)
	{
	case EDistillationTarget::Diffuse:
		return "diffuse";
		break;
	case EDistillationTarget::DiffuseGlossy:
		return "diffuse_glossy";
		break;
	case EDistillationTarget::UE4:
		return "ue4";
		break;
	default:
		check(false);
	}

	return "";
}

bool FMDLImporterUtility::DistillCompiledMaterial(UMaterial* Material, const mi::base::Handle<const mi::neuraylib::IMaterial_definition>& MaterialDefinition, const mi::base::Handle<const mi::neuraylib::ICompiled_material>& CompiledMaterial, bool bUseDisplayName, TArray<FString>* ErrorFunctionCalls, FLoadTextureCallback InCallback)
{
	if (Material == nullptr)
	{
		return false;
	}

	auto MDLPluginModule = FModuleManager::GetModulePtr<FMDLModule>("MDL");
	UMDLSettings* Settings = GetMutableDefault<UMDLSettings>();

	Material->bTangentSpaceNormal = true;
	Material->bUseMaterialAttributes = true;

	Settings->bUseDisplayNameForParameter = bUseDisplayName;

	UMaterialExpressionClearCoatNormalCustomOutput* ClearcoatNormal = nullptr;

	FMDLMaterialImporter MDLImporter = FMDLMaterialImporter(nullptr);
	bool bImportSuccess = false;
	bool bDistillOff = MDLImporter.IsDistillOff(MaterialDefinition) || (DistillOffTemplates.Find(*Material->GetName()) != INDEX_NONE);
	if (Settings->bDistillation && !bDistillOff )
	{
		// get the distilling target from the settings
		FString Target = GetDistillerTargetName();

		// Distilling
		mi::base::Handle<const mi::neuraylib::ICompiled_material> DistilledMaterial(MDLPluginModule->MDLDistillerAPI->distill_material(CompiledMaterial.get(), TCHAR_TO_UTF8(*Target)));
		if (!DistilledMaterial.is_valid_interface())
		{
			return false;
		}

		bImportSuccess = MDLImporter.ImportDistilledMaterial(Material, MaterialDefinition, DistilledMaterial, ClearcoatNormal, InCallback);
	}
	else
	{
		bImportSuccess = MDLImporter.ImportMaterial(Material, MaterialDefinition, CompiledMaterial, ClearcoatNormal, InCallback);
	}
	
	if (ErrorFunctionCalls)
	{
		*ErrorFunctionCalls = MDLImporter.GetLastInvalidFunctionCalls();
	}

	if (!bImportSuccess)
	{
		return false;
	}

	// Arrange expression nodes in material editor
	TArray<UMaterialExpression*> OutputExpressions;
	OutputExpressions.Add(Material->MaterialAttributes.Expression);
	if (ClearcoatNormal)
	{
		OutputExpressions.Add(ClearcoatNormal);
	}

	ArrangeNodes(Material, OutputExpressions);

	Material->PostEditChange();
	Material->MarkPackageDirty();

	return true;
}

bool FMDLImporterUtility::DistillMaterialInstance(UMaterial* Material, const FString& MaterialInstanceName, bool bUseDisplayName)
{
	if (Material == nullptr)
	{
		return false;
	}

	ClearMaterial(Material);
	auto MDLPluginModule = FModuleManager::GetModulePtr<FMDLModule>("MDL");
	UMDLSettings* Settings = GetMutableDefault<UMDLSettings>();
	mi::Uint32 flags = Settings->bInstanceCompilation ? mi::neuraylib::IMaterial_instance::DEFAULT_OPTIONS : mi::neuraylib::IMaterial_instance::CLASS_COMPILATION;
	mi::base::Handle<mi::neuraylib::IMdl_execution_context> Context(MDLPluginModule->MDLFactory->create_execution_context());
	Context->set_option(TCHAR_TO_ANSI(TEXT("meters_per_scene_unit")), Settings->MetersPerSceneUnit);
	
	const mi::base::Handle<const mi::neuraylib::IMaterial_instance> MaterialInstance(
		MDLPluginModule->Transaction->access<mi::neuraylib::IMaterial_instance>(TCHAR_TO_UTF8(*MaterialInstanceName)));
	
	if (!MaterialInstance.is_valid_interface())
	{
		return false;
	}

	mi::base::Handle<const mi::neuraylib::IMaterial_definition> MaterialDefinition = mi::base::make_handle(MDLPluginModule->Transaction->access<mi::neuraylib::IMaterial_definition>(MaterialInstance->get_material_definition()));
	if (!MaterialDefinition.is_valid_interface())
	{
		return false;
	}
	
	mi::base::Handle<const mi::neuraylib::ICompiled_material> CompiledMaterial(MaterialInstance->create_compiled_material(flags, Context.get()));
	if (!CompiledMaterial.is_valid_interface())
	{
		return false;
	}

	return DistillCompiledMaterial(Material, MaterialDefinition, CompiledMaterial, bUseDisplayName);
}

void FMDLImporterUtility::CreateInstanceFromBaseMDL(UMaterialInstanceConstant* MaterialInstance, const mi::base::Handle<const mi::neuraylib::IMaterial_definition>& MaterialDefinition, FLoadInstanceTextureCallback InCallback)
{
	auto MDLModule = FModuleManager::GetModulePtr<IMDLModule>("MDL");
	const mi::base::Handle<const mi::neuraylib::IExpression_list> Defaults(MaterialDefinition->get_defaults());

	for (mi::Size i = 0; i < Defaults->get_size(); ++i)
	{
		FString DisplayName;
		if (FindDisplayNameByParameterName(MaterialDefinition, Defaults->get_name(i), DisplayName))
		{
			mi::base::Handle<const mi::neuraylib::IExpression> DefaultExpression(Defaults->get_expression(i));

			mi::neuraylib::IExpression::Kind kind = DefaultExpression->get_kind();
			if (kind == mi::neuraylib::IExpression::EK_CONSTANT)
			{
				mi::base::Handle<const mi::neuraylib::IValue> ExpressionValue(DefaultExpression.get_interface<const mi::neuraylib::IExpression_constant>()->get_value());
				switch (ExpressionValue->get_kind())
				{
				case mi::neuraylib::IValue::Kind::VK_BOOL:
				{
					bool Ret = GetExpressionConstant<bool, mi::neuraylib::IValue_bool>(DefaultExpression);
					MaterialInstance->SetScalarParameterValueEditorOnly(*DisplayName, Ret ? 1.0f : 0.0f);
				}
				break;
				case mi::neuraylib::IValue::Kind::VK_INT:
				{
					int32 Ret = GetExpressionConstant<int32, mi::neuraylib::IValue_int>(DefaultExpression);
					MaterialInstance->SetScalarParameterValueEditorOnly(*DisplayName, (float)Ret);
				}
				break;
				case mi::neuraylib::IValue::Kind::VK_FLOAT:
				{
					float Ret = GetExpressionConstant<float, mi::neuraylib::IValue_float>(DefaultExpression);
					MaterialInstance->SetScalarParameterValueEditorOnly(*DisplayName, Ret);
				}
				break;
				case mi::neuraylib::IValue::Kind::VK_STRING:
				{
					FString Ret = GetExpressionConstant<FString, mi::neuraylib::IValue_string>(DefaultExpression);
				}
				break;
				case mi::neuraylib::IValue::Kind::VK_VECTOR:
				{
					const mi::base::Handle<const mi::neuraylib::IValue_vector> Value(ExpressionValue.get_interface<const mi::neuraylib::IValue_vector>());

					FLinearColor Color(
						mi::base::make_handle(Value->get_value(0)).get_interface<const mi::neuraylib::IValue_float>()->get_value(),
						mi::base::make_handle(Value->get_value(1)).get_interface<const mi::neuraylib::IValue_float>()->get_value(),
						1.0f);

					if (Value->get_size() > 2)
					{
						Color.B = mi::base::make_handle(Value->get_value(2)).get_interface<const mi::neuraylib::IValue_float>()->get_value();
					}
					if (Value->get_size() > 3)
					{
						Color.A = mi::base::make_handle(Value->get_value(3)).get_interface<const mi::neuraylib::IValue_float>()->get_value();
					}
					MaterialInstance->SetVectorParameterValueEditorOnly(*DisplayName, Color);
				}
				break;
				case mi::neuraylib::IValue::Kind::VK_MATRIX:
				{
					const mi::base::Handle<const mi::neuraylib::IValue_matrix> Value(ExpressionValue.get_interface<const mi::neuraylib::IValue_matrix>());

				}
				break;
				case mi::neuraylib::IValue::Kind::VK_COLOR:
				{
					const mi::base::Handle<const mi::neuraylib::IValue_color> Value(ExpressionValue.get_interface<const mi::neuraylib::IValue_color>());

					FLinearColor Color(
						mi::base::make_handle<const mi::neuraylib::IValue_float>(Value->get_value(0))->get_value(),
						mi::base::make_handle<const mi::neuraylib::IValue_float>(Value->get_value(1))->get_value(),
						mi::base::make_handle<const mi::neuraylib::IValue_float>(Value->get_value(2))->get_value());
					MaterialInstance->SetVectorParameterValueEditorOnly(*DisplayName, Color);
				}
				break;
				case mi::neuraylib::IValue::Kind::VK_TEXTURE:
				{
					const mi::base::Handle<const mi::neuraylib::IValue_texture> Value(ExpressionValue.get_interface<const mi::neuraylib::IValue_texture>());
					const mi::base::Handle<const mi::neuraylib::ITexture> MDLTexture(MDLModule->GetTransaction()->access<mi::neuraylib::ITexture>(Value->get_value()));

					if (InCallback && MDLTexture)
					{
						FString TextureName(MDLTexture->get_image());
						TextureName.RemoveFromStart("MI_default_image_");

						float Gamma = MDLTexture->get_gamma();
						InCallback(TextureName, DisplayName, Gamma);
					}
				}
				break;
				}
			}
		}
	}
}

UMaterialInterface* FMDLImporterUtility::LoadBaseModule(const FString& FileName, const FString& MaterialName)
{
	UMaterialInterface* Material = nullptr;
	if (!IsBaseModule(FileName))
	{
		return Material;
	}

	if (!MaterialName.IsEmpty())
	{
		Material = FindBaseModule(FileName, MaterialName);
		if (Material)
		{
			return Material;
		}
	}

	FString ModuleName = FPaths::GetPath(FileName) / FPaths::GetBaseFilename(FileName);
	ModuleName.ReplaceInline(TEXT("/"), TEXT("::"));
	if (!ModuleName.StartsWith(TEXT("::")))
	{
		ModuleName = TEXT("::") + ModuleName;
	}
	auto MDLPluginModule = FModuleManager::GetModulePtr<FMDLModule>("MDL");
	{
		if (MDLPluginModule->LoadModule(ModuleName) >= 0)
		{
			mi::base::Handle<const mi::neuraylib::IModule> MDLModule = MDLPluginModule->GetLoadedModule(ModuleName);

			if (MDLModule.is_valid_interface())
			{
				for (auto MaterialIdx = 0; MaterialIdx < MDLModule->get_material_count(); ++MaterialIdx)
				{
					mi::base::Handle<const mi::neuraylib::IMaterial_definition> MaterialDefinition = mi::base::make_handle(MDLPluginModule->Transaction->access<mi::neuraylib::IMaterial_definition>(MDLModule->get_material(MaterialIdx)));

					UMDLSettings* Settings = GetMutableDefault<UMDLSettings>();
					const mi::base::Handle<const mi::neuraylib::IMaterial_instance> MaterialInstance(MaterialDefinition->create_material_instance(nullptr));
					if (!MaterialInstance.is_valid_interface())
					{
						continue;
					}

					FString ModuleMaterialName = MDLModule->get_material(MaterialIdx);
					ModuleMaterialName = ModuleMaterialName.Left(ModuleMaterialName.Find(TEXT("(")));
					FString(MoveTemp(ModuleMaterialName)).Split("::", nullptr, &ModuleMaterialName, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
					if (!MaterialName.IsEmpty())
					{
						if (ModuleMaterialName != MaterialName)
						{
							continue;
						}
					}
					else
					{
						if (FindBaseModule(FileName, ModuleMaterialName))
						{
							continue;
						}
					}

					FString LocalPackagePath = GetProjectBaseModulePath() / FPaths::GetPath(FileName) / ModuleMaterialName;
					UPackage* Package = nullptr;

					// Check preset
					FString Prototype = GetPrototype(MDLModule->get_material(MaterialIdx));
					if (!Prototype.IsEmpty())
					{
						FString BaseMaterialFile, BaseMaterialName;
						FString(Prototype).Split("::", &BaseMaterialFile, &BaseMaterialName, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
						BaseMaterialFile.RemoveFromStart(TEXT("mdl"));
						BaseMaterialFile.ReplaceInline(TEXT("::"), TEXT("/"));

						Package = CreatePackage(*LocalPackagePath);
						Material = NewObject<UMaterialInstanceConstant>(Package, *ModuleMaterialName, EObjectFlags::RF_Standalone | EObjectFlags::RF_Public | EObjectFlags::RF_Transactional);
						UMaterialInterface* ParentMaterial = FindBaseModule(BaseMaterialFile, BaseMaterialName);
						auto MaterialInstanceConstant = Cast<UMaterialInstanceConstant>(Material);
						MaterialInstanceConstant->ClearParameterValuesEditorOnly();
						MaterialInstanceConstant->SetParentEditorOnly(ParentMaterial);
						MaterialInstanceConstant->SetScalarParameterValueEditorOnly(TEXT("World-Aligned Textures"), 0);
						CreateInstanceFromBaseMDL(MaterialInstanceConstant, MaterialDefinition);
					}
					else
					{
						mi::Uint32 flags = Settings->bInstanceCompilation ? mi::neuraylib::IMaterial_instance::DEFAULT_OPTIONS : mi::neuraylib::IMaterial_instance::CLASS_COMPILATION;
						mi::base::Handle<mi::neuraylib::IMdl_execution_context> Context(MDLPluginModule->MDLFactory->create_execution_context());
						Context->set_option(TCHAR_TO_ANSI(TEXT("meters_per_scene_unit")), Settings->MetersPerSceneUnit);
						mi::base::Handle<const mi::neuraylib::ICompiled_material> CompiledMaterial(MaterialInstance->create_compiled_material(flags, Context.get()));
						if (!CompiledMaterial.is_valid_interface())
						{
							continue;
						}

						Package = CreatePackage(*LocalPackagePath);
						Material = NewObject<UMaterial>(Package, *ModuleMaterialName, EObjectFlags::RF_Standalone | EObjectFlags::RF_Public | EObjectFlags::RF_Transactional);
						if (!DistillCompiledMaterial(Cast<UMaterial>(Material), MaterialDefinition, CompiledMaterial))
						{
							continue;
						}
					}

					if (Package)
					{
						FString FilePath;// Save to disk
						if (FPackageName::TryConvertLongPackageNameToFilename(LocalPackagePath, FilePath, FPackageName::GetAssetPackageExtension()))
						{
							FSavePackageArgs SaveArgs;
							SaveArgs.TopLevelFlags = RF_Standalone;
							SaveArgs.SaveFlags = SAVE_NoError;
							UPackage::SavePackage(Package, Material, *FilePath, SaveArgs);
						}
					}
				}
			}
		}
	}
	MDLPluginModule->RemoveModule(ModuleName);
	MDLPluginModule->CommitAndCreateTransaction();

	return Material;
}

// The sheet stores the parameters info for base mdl materials
TMap<FString, FMDLParametersList> FMDLImporterUtility::BaseParametersSheet;
// The list stores UE4 material name to mdl module name
TMap<FString, FString> FMDLImporterUtility::BaseMaterialToModuleList;

bool FMDLImporterUtility::FindDisplayNameByParameterName(const mi::base::Handle<const mi::neuraylib::IMaterial_definition>& MaterialDefinition, const char* ParameterName, FString& DisplayName)
{
	// Get material
	if (!MaterialDefinition)
	{
		return false;
	}

	auto ParamAnno = mi::base::make_handle(MaterialDefinition->get_parameter_annotations());
	if (!ParamAnno)
	{
		return false;
	}

	{
		for (auto BlockIdx = 0; BlockIdx < ParamAnno->get_size(); ++BlockIdx)
		{
			auto ParamName = ParamAnno->get_name(BlockIdx);

			if (strcmp(ParamName, ParameterName) == 0)
			{
				auto AnnoBlock = mi::base::make_handle(ParamAnno->get_annotation_block(BlockIdx));
				for (auto AnnoIdx = 0; AnnoIdx < AnnoBlock->get_size(); ++AnnoIdx)
				{
					auto Anno = mi::base::make_handle(AnnoBlock->get_annotation(AnnoIdx));
					auto Name = Anno->get_name();
					if (strcmp(Name, "::anno::display_name(string)") != 0 || mi::base::make_handle(Anno->get_arguments())->get_size() <= 0)
					{
						continue;
					}

					TArray<FString> DisplayNames = GetExpressionConstant<FString, mi::neuraylib::IValue_string>(mi::base::make_handle(Anno->get_arguments()));
					check(DisplayNames.Num() == 1);

					DisplayName = DisplayNames[0];
					return true;
				}
			}
		}
	}

	// Nothing was found, used the ParameterName;
	DisplayName = ParameterName;
	return true;
}

void FMDLImporterUtility::UpdateParametersSheet(const mi::base::Handle<const mi::neuraylib::IMaterial_definition>& MaterialDefinition, const mi::base::Handle<const mi::neuraylib::ICompiled_material>& CompiledMaterial, FMDLParametersList& List)
{
	TArray<FMDLParameterInfo> ParametersList;
	TArray<FString> UniqueDisplayNames;
	TArray<FString> DisplayNamesToLabel;

	for (mi::Size i = 0; i < CompiledMaterial->get_parameter_count(); ++i)
	{
		FString DisplayName;
		if (FMDLImporterUtility::FindDisplayNameByParameterName(MaterialDefinition, CompiledMaterial->get_parameter_name(i), DisplayName))
		{
			FString ParameterName = CompiledMaterial->get_parameter_name(i);
			EMdlValueType ValueType = EMdlValueType::MDL_UNKNOWN;

			auto Argument = CompiledMaterial->get_argument(i);
			if (Argument)
			{
				switch (Argument->get_kind())
				{
				case mi::neuraylib::IValue::Kind::VK_BOOL:	ValueType = EMdlValueType::MDL_BOOL; break;
				case mi::neuraylib::IValue::Kind::VK_INT:	ValueType = EMdlValueType::MDL_INT; break;
				case mi::neuraylib::IValue::Kind::VK_FLOAT:	ValueType = EMdlValueType::MDL_FLOAT; break;
				case mi::neuraylib::IValue::Kind::VK_VECTOR:
				{
					mi::base::Handle<mi::neuraylib::IValue_vector const> NewValue(Argument->get_interface<mi::neuraylib::IValue_vector>());
					switch (NewValue->get_size())
					{
					case 2:	ValueType = EMdlValueType::MDL_FLOAT2; break;
					case 3:	ValueType = EMdlValueType::MDL_FLOAT3; break;
					case 4:	ValueType = EMdlValueType::MDL_FLOAT4; break;
					}
				}
				break;
				case mi::neuraylib::IValue::Kind::VK_COLOR:	ValueType = EMdlValueType::MDL_COLOR; break;
				case mi::neuraylib::IValue::Kind::VK_TEXTURE:	ValueType = EMdlValueType::MDL_TEXTURE; break;
				}

				FMDLParameterInfo ParameterInfo;
				ParameterInfo.ParameterType = ValueType;
				ParameterInfo.ParameterName = ParameterName;
				ParameterInfo.DisplayName = DisplayName;
				ParametersList.Add(ParameterInfo);

				if (UniqueDisplayNames.Find(DisplayName) == INDEX_NONE)
				{
					UniqueDisplayNames.Add(DisplayName);
				}
				else
				{
					DisplayNamesToLabel.AddUnique(DisplayName);
				}
			}
		}
	}

	// NOTE: Display name in MDL annotation is not unique, we should label it in UE4
	// see ImportParameters() in FMDLMaterialImporter
	for (auto DispName : DisplayNamesToLabel)
	{
		int Number = 0;
		for (auto& Info : ParametersList)
		{
			if (Info.DisplayName == DispName)
			{
				Info.DisplayName = DispName + TEXT(" ") + FString::FromInt(Number + 1);
				++Number;
			}
		}
	}

	List.ParametersList = ParametersList;
}

void FMDLImporterUtility::UpdateBaseModuleParameters(const FString& Name)
{
	UMDLSettings* Settings = GetMutableDefault<UMDLSettings>();
	FString ModuleName = TEXT("::") + Name;
	ModuleName.ReplaceInline(TEXT("/"), TEXT("::"));
	auto MDLPluginModule = FModuleManager::GetModulePtr<FMDLModule>("MDL");
	{
		if (MDLPluginModule->LoadModule(ModuleName) >= 0)
		{
			mi::base::Handle<const mi::neuraylib::IModule> MDLModule = MDLPluginModule->GetLoadedModule(ModuleName);

			if (MDLModule.is_valid_interface())
			{
				for (auto MaterialIdx = 0; MaterialIdx < MDLModule->get_material_count(); ++MaterialIdx)
				{
					mi::base::Handle<const mi::neuraylib::IMaterial_definition> MaterialDefinition = mi::base::make_handle(MDLPluginModule->Transaction->access<mi::neuraylib::IMaterial_definition>(MDLModule->get_material(MaterialIdx)));

					FString MaterialName = MDLModule->get_material(MaterialIdx);
					MaterialName = MaterialName.Left(MaterialName.Find(TEXT("(")));
					FString(MoveTemp(MaterialName)).Split("::", nullptr, &MaterialName, ESearchCase::CaseSensitive, ESearchDir::FromEnd);

					const mi::base::Handle<const mi::neuraylib::IMaterial_instance> MaterialInstance(MaterialDefinition->create_material_instance(nullptr));
					if (!MaterialInstance.is_valid_interface())
					{
						continue;
					}

					mi::Uint32 flags = Settings->bInstanceCompilation ? mi::neuraylib::IMaterial_instance::DEFAULT_OPTIONS : mi::neuraylib::IMaterial_instance::CLASS_COMPILATION;
					mi::base::Handle<mi::neuraylib::IMdl_execution_context> Context(MDLPluginModule->GetFactory()->create_execution_context());
					Context->set_option(TCHAR_TO_ANSI(TEXT("meters_per_scene_unit")), Settings->MetersPerSceneUnit);
					mi::base::Handle<const mi::neuraylib::ICompiled_material> CompiledMaterial(MaterialInstance->create_compiled_material(flags, Context.get()));
					if (!CompiledMaterial.is_valid_interface())
					{
						continue;
					}

					FMDLParametersList List;
					UpdateParametersSheet(MaterialDefinition, CompiledMaterial, List);
					BaseParametersSheet.Add(MaterialName, List);
					BaseMaterialToModuleList.Add(MaterialName, Name);
				}
			}
		}
	}
	MDLPluginModule->RemoveModule(ModuleName);
	MDLPluginModule->CommitAndCreateTransaction();
}

bool FMDLImporterUtility::GetBaseModuleByMaterialName(const FString& MaterialName, FString& OutModuleName)
{
	auto ModuleName = BaseMaterialToModuleList.Find(MaterialName);
	if (ModuleName)
	{
		OutModuleName = *ModuleName;
		return true;
	}

	return false;
}

bool FMDLImporterUtility::GetBaseModuleByMaterial(const class UMaterialInterface* MaterialInterface, FString& ModuleName)
{
	if (MaterialInterface == nullptr)
	{
		return false;
	}

	if (GetBaseModuleByMaterialName(MaterialInterface->GetName(), ModuleName))
	{
		return true;
	}

	if (MaterialInterface->IsA<UMaterialInstance>())
	{
		if (GetBaseModuleByMaterialName(Cast<UMaterialInstance>(MaterialInterface)->Parent->GetName(), ModuleName))
		{
			return true;
	}

		if (GetBaseModuleByMaterialName(Cast<UMaterialInstance>(MaterialInterface)->GetMaterial()->GetName(), ModuleName))
		{
			return true;
		}
	}

	return false;
}

bool FMDLImporterUtility::GetDisplayNameFromBaseModule(const FString& InMaterialName, const FString& InParameterName, FString& OutDisplayName)
{
	auto ParametersList = BaseParametersSheet.Find(InMaterialName);
	if (ParametersList)
	{
		for (auto Parameter : ParametersList->ParametersList)
		{
			if (Parameter.ParameterName == InParameterName)
			{
				OutDisplayName = Parameter.DisplayName;
				return true;
			}
		}
	}

	return false;
}

bool FMDLImporterUtility::GetMdlParameterTypeAndNameFromBaseModule(const FString& InMaterialName, const FString& InDisplayName, EMdlValueType& ValueType, FString& ParameterName)
{
	auto ParametersList = BaseParametersSheet.Find(InMaterialName);
	if (ParametersList)
	{
		for (auto Parameter : ParametersList->ParametersList)
		{
			if (Parameter.DisplayName == InDisplayName)
			{
				ValueType = Parameter.ParameterType;
				ParameterName = Parameter.ParameterName;
				return true;
			}
		}
	}

	return false;
}

UMaterialFunction* FMDLImporterUtility::LoadMDLFunction(const FString& AssetPath, const FString& AssetName, int32 ArraySize)
{
	return LoadFunction(AssetPath, AssetName, ArraySize);
}

FString FMDLImporterUtility::GetPrototype(const FString& MaterialName)
{
	auto MDLPluginModule = FModuleManager::GetModulePtr<FMDLModule>("MDL");
	mi::base::Handle<const mi::neuraylib::IMaterial_definition> MaterialDefinition = mi::base::make_handle(MDLPluginModule->GetTransaction()->access<mi::neuraylib::IMaterial_definition>(TCHAR_TO_UTF8(*MaterialName)));

	FString Prototype = MaterialDefinition->get_prototype();
	Prototype.RemoveFromStart(TEXT("mdl"));
	Prototype = UnmangleMdlPath(Prototype);
	Prototype = Prototype.Left(Prototype.Find(TEXT("(")));
	Prototype.RemoveFromStart(TEXT("::"));
	return Prototype;
}