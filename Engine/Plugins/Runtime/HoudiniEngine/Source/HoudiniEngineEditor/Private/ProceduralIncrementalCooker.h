// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Set.h"
#include "PackageSourceControlHelper.h"
#include "ProceduralIncrementalCooker.generated.h"

//FROSTY_BEGIN_CHANGE

class UHoudiniAssetComponent;
class UTOPNode;
class UTOPNetwork;
class UHoudiniPDGAssetLink;
class AActor;
class UPackage;

UCLASS(Abstract, Config=Editor)
class UProceduralIncrementalCooker : public UObject
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
public:
	virtual bool Cook(UWorld* InWorld, FPackageSourceControlHelper* InSourceControlHelper);

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "CookSettings", config)
	FString HoudiniAssetShortName;

	UPROPERTY(EditAnywhere, Category = "CookSettings", config)
	FString HoudiniAssetFile;

	UPROPERTY(EditAnywhere, Category = "CookSettings", config)
	FString TilesPerAxisParameterName;

	UPROPERTY(EditAnywhere, Category = "CookSettings", config)
	FString TilesRangeParameterName;

	UPROPERTY(EditAnywhere, Category = "CookSettings", config)
	int32 TilesPerAxis = 1;

	UPROPERTY(EditAnywhere, Category = "CookSettings", config)
	int32 TilesPerIncrement = 1;

	UPROPERTY(EditAnywhere, Category = "CookSettings", config)
	int32 TileStartIndex = 0;

	UPROPERTY(EditAnywhere, Category = "CookSettings", config)
	float CookAndBakeTimeLimit = 0.f;
#endif

protected:
	virtual void OnUpdateHoudiniParameters(UHoudiniAssetComponent* HoudiniAssetComponent) {}
	virtual bool OnPreCook() { return true; }
	virtual bool OnPreCookTiles(int32 TileIndexStart, int32 TileIndexEnd) { return true; }
	virtual bool OnPreSave(TSet<AActor*>& BakedActors) { return true; }
	virtual bool OnPostCookTiles(int32 TileIndexStart, int32 TileIndexEnd) { return true; }
	virtual bool OnPostCook() { return true; }

	virtual bool CheckoutAndSavePackage(UPackage* Package);
	void DeletePackage(const FString& PackageName);
	void DeletePackage(UPackage* Package);

private:
	void DirtyAll(UHoudiniPDGAssetLink* InPDGAssetLink);
	bool HasNodeFailed(UTOPNode* TOPNode);
	bool IsCooking(UTOPNetwork* InTOPNet);
	bool WaitForHoudini(TFunction<bool()> WaitCondition, double TimeOutInSeconds = 0);
	bool CookAndBake(UHoudiniAssetComponent* HoudiniAssetComponent, TFunctionRef<void(UHoudiniAssetComponent*)> UpdateParametersFunction, TSet<AActor*>& OutBakedActors);
	bool SaveActors(const TSet<AActor*>& Actors);
	void UnloadAll();
#endif

protected:
#if WITH_EDITORONLY_DATA
	UWorld* World = nullptr;
	FPackageSourceControlHelper* SourceControlHelper = nullptr;
	FPackageSourceControlHelper DefaultPackageHelper;
#endif
};

//FROSTY_END_CHANGE