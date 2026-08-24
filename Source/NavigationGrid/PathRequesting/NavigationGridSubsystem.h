// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include <thread>
#include <future>
#include "NavGridPathResult.h"
#include "NavigationGrid/Data/NavGridData.h"
#include "NavigationGrid/Data/NavGridMovingData.h"
#include "NavigationGrid/HeightNavigation/HeightNavigationVolume.h"
#include "NavigationGridSubsystem.generated.h"


UCLASS()
class NAVIGATIONGRID_API UNavigationGridSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
	
public:
	static UNavigationGridSubsystem* Get(const UObject* WorldContextObject);
	
	void RequestPath(UObject* PathRequester, const FNavGridMovingData& NaveGridMovingData);
	
	void RegisterNavVolume(const FNavGridData& NavGridData);
	
	virtual void Tick(float DeltaTime) override;
protected:
	UPROPERTY()
	TArray<FNavGridData> AvailableVolumes;
	
	UPROPERTY()
	TMap<UObject*, std::future<FNavGridPathResult>> PathResults;
	
private:
	FNavGridPathResult FindPath(FVector startPos, AActor* startActor, const FNavGridMovingData& NavGridMovingData);
};
