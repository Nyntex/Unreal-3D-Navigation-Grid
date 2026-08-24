// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AIController.h"
#include "NavGridMovingData.generated.h"

USTRUCT(BlueprintType)
struct NAVIGATIONGRID_API FNavGridMovingData
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AAIController* Controller; 
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector GoalLocation;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* GoalActor = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AcceptanceRadius = -1.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAIOptionFlag::Type StopOnOverlap = EAIOptionFlag::Default;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAIOptionFlag::Type AcceptPartialPath = EAIOptionFlag::Default;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUsePathfinding = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bLockAILogic = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUseContinuousGoalTracking = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAIOptionFlag::Type ProjectGoalOnNavigation = EAIOptionFlag::Default;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAIOptionFlag::Type RequireNavigableEndLocation = EAIOptionFlag::Default;
};
