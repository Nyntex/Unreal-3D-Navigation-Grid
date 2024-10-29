// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <map>
#include "Delegates/DelegateCombinations.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "MoveToLocationOrActor3D.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMoveToLocationOrActor3DOutputPin);

UCLASS(BlueprintType, meta=(ExposedAsyncProxy = AsyncAction))
class UMoveToLocationOrActor3D : public UBlueprintAsyncActionBase, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContext", BlueprintInternalUseOnly = "true"), Category = "Navigation Grid")
	static UMoveToLocationOrActor3D* MoveToLocationOrActor3D(APawn* WorldContext, FVector Location);

	//Should fire every tick right after the move command has been given
	UPROPERTY(BlueprintReadOnly, Category= "Move to Location or Actor 3D")
	FMoveToLocationOrActor3DOutputPin OnMove;

	UPROPERTY(BlueprintReadOnly, Category = "Move to Location or Actor 3D")
	FMoveToLocationOrActor3DOutputPin OnFinish;

	UPROPERTY(BlueprintReadOnly, Category = "Move to Location or Actor 3D")
	FMoveToLocationOrActor3DOutputPin OnCanceled;

	UPROPERTY(BlueprintReadOnly, Category = "Move to Location or Actor 3D")
	FMoveToLocationOrActor3DOutputPin OnFailed;

private:
	//Starts the action, automatically called when blueprint node is called
	virtual void Activate() override;

public:
	UPROPERTY(BlueprintReadOnly, Category = "Move to Location or Actor 3D")
	FVector LocationToMoveTo = FVector::Zero();


public:
	UFUNCTION(BlueprintCallable, Category = "Move to Location or Actor 3D")
	void CancelMovement();

	virtual void Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override
	{
		return ETickableTickType::Always;
	}
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UMoveToLocationOrActor3D, STATGROUP_Tickables);
	}
	virtual bool IsTickableWhenPaused() const
	{
		return false;
	}
	virtual bool IsTickableInEditor() const
	{
		return false;
	}

private:
	void UpdateMove(float DeltaTime);

	//Clean up the action object
	void FinishMovement();

	UPROPERTY()
	TObjectPtr<APawn> MovingTarget;
	UPROPERTY()
	TObjectPtr<UWorld> World;
};

namespace MoveToLocationOrActor3DStatics
{
	//This Array will limit the amount of actions to one per object so that the same
	//Action will not get called on the same target twice.
	static std::map<APawn*, UMoveToLocationOrActor3D*> CurrentMovingObjects = std::map<APawn*, UMoveToLocationOrActor3D*>();
}