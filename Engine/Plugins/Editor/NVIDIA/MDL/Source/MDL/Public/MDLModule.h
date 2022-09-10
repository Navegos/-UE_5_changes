// Copyright(c) 2020, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#pragma once

#if WITH_MDL_SDK
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "MDLDependencies.h"
#endif

class MDL_API IMDLModule : public IModuleInterface
{
public:
	virtual void CommitAndCreateTransaction() = 0;
	virtual void AddModulePath(const FString& Path, bool IgnoreCheck = false) = 0;
	virtual void RemoveModulePath(const FString& Path) = 0;
	virtual void AddResourcePath(const FString& Path) = 0;
	virtual void RemoveResourcePath(const FString& Path) = 0;
	virtual void SetExternalFileReader(TSharedPtr<class IMDLExternalReader> FileReader) = 0;
	virtual void RegisterDynamicMipsLoader(class IDynamicMipsLoader* Loader) = 0;
	virtual TSharedPtr<class IDynamicMipsLoader> GetDynamicMipsLoader() = 0;
	virtual TSharedPtr<class IMDLMaterialImporter> CreateDefaultImporter() = 0;
	virtual int32 LoadModule(const FString& ModuleName) = 0;
	virtual mi::base::Handle<const mi::neuraylib::IModule> GetLoadedModule(const FString& ModuleName) = 0;
	virtual void RemoveModule(const FString& ModuleName) = 0;

	virtual mi::base::Handle<mi::neuraylib::ITransaction> GetTransaction() = 0;
	virtual mi::base::Handle<mi::neuraylib::IMdl_distiller_api> GetDistiller() = 0;
	virtual mi::base::Handle<mi::neuraylib::IMdl_factory> GetFactory() = 0;
};

