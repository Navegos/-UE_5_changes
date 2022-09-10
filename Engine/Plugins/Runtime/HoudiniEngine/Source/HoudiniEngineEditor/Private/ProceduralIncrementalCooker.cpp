// Copyright Epic Games, Inc. All Rights Reserved.

//FROSTY_BEGIN_CHANGE

#include "ProceduralIncrementalCooker.h"

#include "HoudiniEngineEditorPrivatePCH.h"
#include "HoudiniEngine.h"
#include "HoudiniEngineUtils.h"
#include "HoudiniEngineBakeUtils.h"
#include "HoudiniEngineManager.h"
#include "HoudiniPDGManager.h"
#include "HoudiniParameterInt.h"
#include "HoudiniAssetActor.h"
#include "HoudiniAssetComponent.h"

#include "FileHelpers.h"
#include "AssetSelection.h"
#include "Misc/ScopedSlowTask.h"
#include "HAL/PlatformFileManager.h"
#include "WorldPartition/WorldPartition.h"
#include "UObject/SavePackage.h"

#define LOCTEXT_NAMESPACE HOUDINI_LOCTEXT_NAMESPACE 

UProceduralIncrementalCooker::UProceduralIncrementalCooker(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#if WITH_EDITOR
bool UProceduralIncrementalCooker::Cook(UWorld* InWorld, FPackageSourceControlHelper* InSourceControlHelper)
{
	if (!InWorld)
	{
		HOUDINI_LOG_ERROR(TEXT("Cooking Tiles Failed : Invalid world."));
		return false;
	}

	World = InWorld;
	SourceControlHelper = InSourceControlHelper ? InSourceControlHelper : &DefaultPackageHelper;

	FString WorldConfigFilename = InWorld->GetPackage()->GetLoadedPath().GetLocalFullPath();
	WorldConfigFilename = FPaths::ChangeExtension(WorldConfigFilename, TEXT("ini"));
	if (FPlatformFileManager::Get().GetPlatformFile().FileExists(*WorldConfigFilename))
	{
		LoadConfig(GetClass(), *WorldConfigFilename);
	}
	else
	{
		SaveConfig(CPF_Config, *WorldConfigFilename);
		HOUDINI_LOG_ERROR(TEXT("Cooking Tiles Failed : Created missing config file %s. Please update it and re-run the cooker."), *WorldConfigFilename);
		return false;
	}

	if (TilesPerIncrement < 1)
	{
		HOUDINI_LOG_ERROR(TEXT("Cooking Tiles Failed : Invalid TilesPerIncrement."));
		return false;
	}

	const int32 TileTotalCount = TilesPerAxis * TilesPerAxis;
	const int32 TileEffectiveStartIndex = FMath::Clamp(TileStartIndex, 0, TileTotalCount);
	const int32 TilesToCook = TileTotalCount - TileEffectiveStartIndex;

	FScopedSlowTask SlowTask(TilesToCook, FText::Format(LOCTEXT("CookingAndBakingTiles", "Cooking & Baking {0} Tiles for {1}"), FText::AsNumber(TilesToCook), FText::FromString(HoudiniAssetShortName)));
	SlowTask.MakeDialog();

	if (!OnPreCook())
	{
		HOUDINI_LOG_ERROR(TEXT("Cooking Tiles Failed : OnPreCook failed."));
		return false;
	}

	UHoudiniAsset* HoudiniAsset = nullptr;
	AHoudiniAssetActor* HoudiniAssetActor = nullptr;

	auto DestroyHoudiniAsset = [&]()
	{
		FHoudiniEngine::Get().SetSingleComponentToProcess(nullptr);

		if (HoudiniAssetActor)
		{
			HoudiniAssetActor->RemoveFromRoot();
			World->DestroyActor(HoudiniAssetActor);
			HoudiniAssetActor = nullptr;
			// Need to wait for Houdini to finish executing all pending tasks
			WaitForHoudini([] { return FHoudiniEngine::Get().HasPendingTasks(); }, 10);
		}

		if (HoudiniAsset)
		{
			HoudiniAsset->RemoveFromRoot();
			HoudiniAsset = nullptr;
		}
	};

	ON_SCOPE_EXIT
	{
		DestroyHoudiniAsset();
		OnPostCook();
	};

	if (!FHoudiniEngine::IsInitialized())
	{
		const UHoudiniRuntimeSettings* HoudiniRuntimeSettings = GetDefault< UHoudiniRuntimeSettings >();
		// Restart the current Houdini Engine Session
		if (!FHoudiniEngine::Get().CreateSession(HoudiniRuntimeSettings->SessionType))
		{
			HOUDINI_LOG_ERROR(TEXT("Cooking Tiles Failed : Failed to create a Houdini session."));
			return false;
		}
		WaitForHoudini([] { return FHoudiniEngine::Get().HasPendingTasks(); }, 10);
	}

	// Split cooking into incremental steps to avoid using too much memory
	for (int32 TileIndexStart = TileEffectiveStartIndex; TileIndexStart < TileTotalCount;)
	{
		int32 TileIndexEnd = FMath::Min<int32>(TileIndexStart + TilesPerIncrement, TileTotalCount) - 1;
		SlowTask.EnterProgressFrame(TileIndexEnd - TileIndexStart);
		HOUDINI_LOG_MESSAGE(TEXT("Cooking Tiles %d to %d"), TileIndexStart, TileIndexEnd);

		FHoudiniEngine::Get().StopSession();
		WaitForHoudini([] { return FHoudiniEngine::Get().HasPendingTasks(); }, 10);

		// Restart Houdini session
		if (!FHoudiniEngine::Get().RestartSession() || !FHoudiniEngine::IsInitialized() || !FHoudiniEngine::Get().GetHoudiniEngineManager())
		{
			HOUDINI_LOG_ERROR(TEXT("Cooking Tiles Failed : Failed to restart a Houdini session."));
			return false;
		}
		WaitForHoudini([] { return FHoudiniEngine::Get().HasPendingTasks(); }, 10);

		// Load Houdini asset
		HoudiniAsset = LoadObject<UHoudiniAsset>(nullptr, *HoudiniAssetFile);
		if (!HoudiniAsset)
		{
			HOUDINI_LOG_ERROR(TEXT("Cooking Tiles Failed : Can't load Houdini Asset."));
			return false;
		}
		HoudiniAsset->AddToRoot();

		// Create Houdini asset actor
		HoudiniAssetActor = Cast<AHoudiniAssetActor>(FActorFactoryAssetProxy::AddActorForAsset(HoudiniAsset, /*bSelectActors*/false, RF_NoFlags/*RF_Transient*/));
		if (!HoudiniAssetActor)
		{
			HOUDINI_LOG_ERROR(TEXT("Cooking Tiles Failed : Invalid Houdini asset actor."));
			return false;
		}
		HoudiniAssetActor->AddToRoot();
		UHoudiniAssetComponent* HoudiniAssetComponent = HoudiniAssetActor->GetHoudiniAssetComponent();
		check(HoudiniAssetComponent);
		FHoudiniEngine::Get().SetSingleComponentToProcess(HoudiniAssetComponent);

		if (!OnPreCookTiles(TileIndexStart, TileIndexEnd))
		{
			HOUDINI_LOG_ERROR(TEXT("Cooking Tiles Failed : OnPreCookTiles(%d, %d) failed."), TileIndexStart, TileIndexEnd);
			return false;
		}

		// Update Houdini asset parameters
		auto UpdateParameters = [this, TileIndexStart, TileIndexEnd](UHoudiniAssetComponent* HoudiniAssetComponent)
		{
			OnUpdateHoudiniParameters(HoudiniAssetComponent);

			UHoudiniParameter* Parameter = HoudiniAssetComponent->FindParameterByName(TilesPerAxisParameterName);
			if (Parameter)
			{
				if (UHoudiniParameterInt* ParameterInt = Cast<UHoudiniParameterInt>(Parameter))
				{
					if (ParameterInt->GetNumberOfValues() == 1)
					{
						ParameterInt->SetValueAt(TilesPerAxis, 0);
						ParameterInt->MarkChanged(true);
					}
				}
			}

			Parameter = HoudiniAssetComponent->FindParameterByName(TilesRangeParameterName);
			if (Parameter)
			{
				if (UHoudiniParameterInt* ParameterInt = Cast<UHoudiniParameterInt>(Parameter))
				{
					if (ParameterInt->GetNumberOfValues() == 2)
					{
						ParameterInt->SetValueAt(TileIndexStart, 0);
						ParameterInt->SetValueAt(TileIndexEnd, 1);
						ParameterInt->MarkChanged(true);
					}
				}
			}
		};

		// Cook and bake asset for range of tiles
		TSet<AActor*> BakedActors;
		bool bCookSuccess = CookAndBake(HoudiniAssetComponent, UpdateParameters, BakedActors);
		if (bCookSuccess)
		{
			// Save them
			if (!OnPreSave(BakedActors))
			{
				HOUDINI_LOG_ERROR(TEXT("Cooking Tiles Failed : OnPreSave failed."));
				return false;
			}
			SaveActors(BakedActors);
		}

		// Clean-up & Unload
		if (!OnPostCookTiles(TileIndexStart, TileIndexEnd))
		{
			HOUDINI_LOG_ERROR(TEXT("Cooking Tiles Failed : OnPostCookTiles(%d, %d) failed."), TileIndexStart, TileIndexEnd);
			return false;
		}

		DestroyHoudiniAsset();
		UnloadAll();

		// Stop Houdini Session
		FHoudiniEngine::Get().StopSession();
		WaitForHoudini([] { return FHoudiniEngine::Get().HasPendingTasks(); }, 10);

		// Force GC
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);

		if (!bCookSuccess)
		{
			HOUDINI_LOG_ERROR(TEXT("Cooking Tiles %d to %d Failed"), TileIndexStart, TileIndexEnd);
			return false;
		}

		// Prepare next starting tlle index
		TileIndexStart = TileIndexEnd + 1;
	}

	return true;
}

void UProceduralIncrementalCooker::DirtyAll(UHoudiniPDGAssetLink* InPDGAssetLink)
{
	if (IsValid(InPDGAssetLink))
	{
		UTOPNetwork* const TOPNetwork = InPDGAssetLink->GetSelectedTOPNetwork();
		if (IsValid(TOPNetwork))
		{
			if (InPDGAssetLink->LinkState == EPDGLinkState::Linked)
			{
				FHoudiniPDGManager::DirtyAll(TOPNetwork);
			}
			else
			{
				UHoudiniPDGAssetLink::ClearTOPNetworkWorkItemResults(TOPNetwork);
			}
		}
	}
};

bool UProceduralIncrementalCooker::HasNodeFailed(UTOPNode* TOPNode)
{
	return (!IsValid(TOPNode) || (TOPNode->NodeState == EPDGNodeState::Cook_Failed) || TOPNode->AnyWorkItemsFailed());
};

bool UProceduralIncrementalCooker::IsCooking(UTOPNetwork* InTOPNet)
{
	bool bAlreadyCooking = InTOPNet->AnyWorkItemsPending();

	if (!bAlreadyCooking)
	{
		HAPI_PDG_GraphContextId GraphContextId = -1;
		if (HAPI_RESULT_SUCCESS != FHoudiniApi::GetPDGGraphContextId(
			FHoudiniEngine::Get().GetSession(), InTOPNet->NodeId, &GraphContextId))
		{
			HOUDINI_LOG_ERROR(TEXT("PDG Cook Output - Failed to get %s's graph context ID!"), *(InTOPNet->NodeName));
			return false;
		}

		int32 PDGState = -1;
		if (HAPI_RESULT_SUCCESS != FHoudiniApi::GetPDGState(
			FHoudiniEngine::Get().GetSession(), GraphContextId, &PDGState))
		{
			HOUDINI_LOG_ERROR(TEXT("PDG Cook Output - Failed to get %s's PDG state."), *(InTOPNet->NodeName));
			return false;
		}
		bAlreadyCooking = ((HAPI_PDG_State)PDGState == HAPI_PDG_STATE_COOKING);
	}

	return bAlreadyCooking;
}

bool UProceduralIncrementalCooker::WaitForHoudini(TFunction<bool()> WaitCondition, double TimeOutInSeconds)
{
	double StartTimestamp = FPlatformTime::Seconds();
	do
	{
		FHoudiniEngine::Get().ManualTick();
		FPlatformProcess::Sleep(0.f);
		if (FPlatformTime::Seconds() - StartTimestamp > TimeOutInSeconds)
		{
			return false;
		}
	} while (WaitCondition());
	return true;
}

bool UProceduralIncrementalCooker::CookAndBake(UHoudiniAssetComponent* HoudiniAssetComponent, TFunctionRef<void(UHoudiniAssetComponent*)> UpdateParametersFunction, TSet<AActor*>& OutBakedActors)
{
	check(HoudiniAssetComponent);

	// Make sure Houdini asset is instanciated
	HoudiniAssetComponent->OnHoudiniAssetChanged();
	if (!WaitForHoudini([&HoudiniAssetComponent] { return !HoudiniAssetComponent->IsFullyLoaded() || HoudiniAssetComponent->GetAssetState() != EHoudiniAssetState::None || !HoudiniAssetComponent->GetNumParameters(); }, 15))
	{
		HOUDINI_LOG_ERROR(TEXT("CookAndBake Error : Asset instanciation timeout..."));
		return false;
	}

	// Update asset parameters 
	UpdateParametersFunction(HoudiniAssetComponent);
	if (!WaitForHoudini([&HoudiniAssetComponent] { return !HoudiniAssetComponent->IsFullyLoaded() || HoudiniAssetComponent->NeedUpdateParameters(); }, 15))
	{
		HOUDINI_LOG_ERROR(TEXT("CookAndBake Error : Update asset parameters timeout..."));
		return false;
	}

	// Prepare PDG cook & baked
	UHoudiniPDGAssetLink* PDGAssetLink = HoudiniAssetComponent->GetPDGAssetLink();
	if (!PDGAssetLink)
		return false;

	PDGAssetLink->SelectTOPNetwork(0);
	UTOPNetwork* SelectedTOPNet = PDGAssetLink->GetSelectedTOPNetwork();
	if (!SelectedTOPNet || (PDGAssetLink->AllTOPNetworks.Num() <= 0))
		return false;

	// Repopulate the network and nodes for the assetlink
	if (!FHoudiniPDGManager::UpdatePDGAssetLink(PDGAssetLink))
		return false;

	// Should be Linked at this point
	if (PDGAssetLink->LinkState != EPDGLinkState::Linked)
		return false;

	// Force a call to DirtyAll before cooking output
	DirtyAll(PDGAssetLink);

	// Force a cleanup of previously baked actors (because Work results don't match from one cook/bake to the other, one consequence of that is that some old-components are not removed)
	FHoudiniEngineBakeUtils::CleanupPreviouslyBakedActors(PDGAssetLink);

	// Force post-cook baking for PDG nodes
	if (PDGAssetLink->AutoBakeDelegateHandle.IsValid())
	{
		PDGAssetLink->OnWorkResultObjectLoaded.Remove(PDGAssetLink->AutoBakeDelegateHandle);
	}
	PDGAssetLink->bBakeAfterAllWorkResultObjectsLoaded = true;
	PDGAssetLink->AutoBakeDelegateHandle = PDGAssetLink->OnWorkResultObjectLoaded.AddLambda(
		[&OutBakedActors](UHoudiniPDGAssetLink* InPDGAssetLink, UTOPNode* InNode, int32 InWorkItemHAPIIndex, int32 InWorkItemResultInfoIndex)
		{
			if (!IsValid(InPDGAssetLink))
				return;

			if (!InPDGAssetLink->bBakeAfterAllWorkResultObjectsLoaded)
				return;

			TArray<FHoudiniEngineBakedActor> BakedActors;
			FHoudiniEngineBakeUtils::PDGAutoBakeAfterResultObjectLoaded(InPDGAssetLink, InNode, InWorkItemHAPIIndex, InWorkItemResultInfoIndex, BakedActors);
			for (FHoudiniEngineBakedActor& BakedActor : BakedActors)
			{
				if (BakedActor.Actor)
				{
					OutBakedActors.Add(BakedActor.Actor);
				}
			}
		}
	);

	// Trigger cooking
	FHoudiniPDGManager::CookOutput(SelectedTOPNet);

	// Wait for completion
	if (!WaitForHoudini([this, &SelectedTOPNet, &PDGAssetLink] { return IsCooking(SelectedTOPNet) && !HasNodeFailed(PDGAssetLink->GetSelectedTOPNode()); }, CookAndBakeTimeLimit))
	{
		HOUDINI_LOG_ERROR(TEXT("CookAndBake Error : Cook output timeout..."));
		return false;
	}

	// Return failed or success
	return !HasNodeFailed(PDGAssetLink->GetSelectedTOPNode());
}

bool UProceduralIncrementalCooker::SaveActors(const TSet<AActor*>& Actors)
{
	bool bResult = true;
	for (AActor* Actor : Actors)
	{
		if (!CheckoutAndSavePackage(Actor->GetPackage()))
		{
			bResult = false;
		}
	}
	return bResult;
}

void UProceduralIncrementalCooker::UnloadAll()
{
	if (UWorldPartition* WorldPartition = World->GetWorldPartition())
	{
		const FBox All(FVector(-WORLD_MAX), FVector(WORLD_MAX));
		WorldPartition->UnloadEditorCells(All, false);
	}
}

bool UProceduralIncrementalCooker::CheckoutAndSavePackage(UPackage* Package)
{
	// Checkout package
	Package->MarkAsFullyLoaded();

	if (!SourceControlHelper->Checkout(Package))
	{
		HOUDINI_LOG_ERROR(TEXT("Error checking out package %s."), *Package->GetName());
		return false;
	}

	// Save package
	FPackagePath PackagePath = FPackagePath::FromPackageNameChecked(Package->GetName());
	const FString PackageFileName = PackagePath.GetLocalFullPath();
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Standalone;
	if (!UPackage::SavePackage(Package, nullptr, *PackageFileName, SaveArgs))
	{
		HOUDINI_LOG_ERROR(TEXT("Error saving package %s."), *Package->GetName());
		return false;
	}

	// Add new package to source control
	if (!SourceControlHelper->AddToSourceControl(Package))
	{
		HOUDINI_LOG_ERROR(TEXT("Error adding package %s to source control."), *Package->GetName());
		return false;
	}

	return true;
}

void UProceduralIncrementalCooker::DeletePackage(const FString& PackageName)
{
	FPackagePath PackagePath = FPackagePath::FromPackageNameChecked(PackageName);
	const FString PackageFileName = PackagePath.GetLocalFullPath();

	SourceControlHelper->Delete(PackageFileName);
}

void UProceduralIncrementalCooker::DeletePackage(UPackage* Package)
{
	SourceControlHelper->Delete(Package);
}

#endif

#undef LOCTEXT_NAMESPACE

//FROSTY_END_CHANGE