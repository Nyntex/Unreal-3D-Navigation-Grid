// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <map>
#include "Delegates/DelegateCombinations.h"
#include "LatentActions.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "MoveToLocationOrActor3D.generated.h"


#pragma region AsyncAction
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMoveToLocationOrActor3DOutputPin);

UCLASS(BlueprintType, meta=(ExposedAsyncProxy = AsyncAction))
class UMoveToLocationOrActor3D : public UBlueprintAsyncActionBase, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContext", BlueprintInternalUseOnly = "true"), Category = "Navigation Grid")
	static UMoveToLocationOrActor3D* MoveToLocationOrActor3D(APawn* WorldContext, FVector Location);

	//Should fire every tick right after the move command has been given
	UPROPERTY(BlueprintReadOnly, Category= "Move to Location or Actor 3D", BlueprintAssignable)
	FMoveToLocationOrActor3DOutputPin OnMove;

	UPROPERTY(BlueprintReadOnly, Category = "Move to Location or Actor 3D", BlueprintAssignable)
	FMoveToLocationOrActor3DOutputPin OnFinish;

	UPROPERTY(BlueprintReadOnly, Category = "Move to Location or Actor 3D", BlueprintAssignable)
	FMoveToLocationOrActor3DOutputPin OnCanceled;

	UPROPERTY(BlueprintReadOnly, Category = "Move to Location or Actor 3D", BlueprintAssignable)
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
#pragma endregion


#pragma region LatentAction

UENUM()
enum class EMoveInputPins : uint8
{
	Start,		//Starts movement
	Cancel,		//Cancels movement
};

UENUM()
enum class EMoveOutputPins : uint8
{
	OnStarted,		//Called only once on start
	OnMove,			//Every Frame
	OnCanceled,		//When node is canceled
	OnCompleted,	//when the movement is completed
	OnFailed,		//When movement failed
};

UCLASS()
class UMoveToActorOrLocation3D : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 *Start the movement and move context object to target location.
	 *
	 *Fails if location is outside of 3D Navigation Grid or location is invalid
	 *
	 *Only one instance can be created per object. When creating a new latent action the old one will be canceled
	 *
	 *@param WorldContext			Variable to hold the object calling this function. Pin will be hidden
	 *@param LatentInfo				Variable to hold the information required to process a latent function. Will be hidden on the node
	 *@param InputPins				Variable to hold the different input pins
	 *@param OutputPins				Variable to hold the different output pins
	 *@param MoveLocation			Variable to hold the location the context object is supposed to be moved to
	 *@param CurrentMoveDirection	Gives the current direction the context is moving towards, this is a local value and does not show the final location
	 */
	UFUNCTION(BlueprintCallable, meta=(WorldContext = "WorldContext", Latent, LatentInfo = "LatentInfo", 
		ExpandEnumAsExecs = "InputPins,OutputPins"), Category = "Move to Actor or Location 3D")
	static void MoveToActorOrLocation3D(APawn* WorldContext, FLatentActionInfo LatentInfo, EMoveInputPins InputPins, 
		EMoveOutputPins& OutputPins, FVector MoveLocation, FVector& CurrentMoveDirection);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContext"))
	static void Stop3DMovement(APawn* WorldContext);
};


class FLatentMoveToActorOrLocation3D : public FPendingLatentAction
{
public:
	TObjectPtr<APawn> MovementTarget;

	FVector MoveLocation = FVector::Zero();

public:
	FVector& CurrentMoveDirection;

	bool IsFirstCall = true;

public:
	FLatentActionInfo LatentActionInfo;

	EMoveOutputPins& Output;

	FLatentMoveToActorOrLocation3D(FLatentActionInfo& LatentInfo, EMoveOutputPins& OutputPins, APawn* WorldContext,
		FVector MoveLocation, FVector& CurrentMoveDirection)
			: MovementTarget(WorldContext), MoveLocation(MoveLocation), CurrentMoveDirection(CurrentMoveDirection), LatentActionInfo(LatentInfo), Output(OutputPins)
	{
		Output = EMoveOutputPins::OnStarted;
		IsFirstCall = true;
		CurrentMoveDirection = FVector::Zero();
	}

	virtual void UpdateOperation(FLatentResponse& Response) override;

	void GetNewPath();

	/*
	 *Uses the PathIndex to move to locations. The MoveLocation has no relevance for this
	 *function because this one uses the path, the GetNewPath() method retrieves the path using
	 *the move location. Moves the target using control inputs, I bet there are better ways but this works for me
	 *
	 **/
	void UpdateMovement();

	void MoveInDirection(FVector Direction);

	bool PathValidationCheck(int Try);

	//Receive the normalised direction to the given target location (Base is MovementTarget->ActorLocation)
	FVector DirectionToLocation(FVector Location);
	static FVector DirectionToLocation(FVector StartLocation, FVector Location);

	/**
	 * Checks 2 positions, the goal position and one position before it.
	 * With each call the position gets put further and further away until it is the end where it gets reset
	 * to a couple positions in front of the current location.
	 *
	 * Does nothing when to close to the goal.
	 *
	 * Runs every couple milliseconds.
	 *
	 * @param DeltaTime		Required to increase the timer
	 */
	void UpdateDirectPath(float DeltaTime);

#if WITH_EDITOR
	virtual FString GetDescription() const override
	{
		FString text = MovementTarget.GetName() + TEXT(" is moving from ") + MovementTarget->GetActorLocation().ToString() + TEXT(" to ") + MoveLocation.ToString() + TEXT("\n The current direction is ") + CurrentMoveDirection.ToString();
		return text;
	}
#endif

protected:
	TArray<FVector> Path = TArray<FVector>();

	int PathIndex = 0;

	float ClosenessThreshold = 50.f;


#pragma region DirectPathLoop

	float Interval = 0.0f;
	float CurrentInterval = 0.3f;
	int IndexToCheck = 0;

#pragma endregion

};
#pragma endregion


namespace MoveToLocationOrActor3DStatics
{
	//This Array will limit the amount of actions to one per object so that the same
	//Action will not get called on the same target twice.
	static std::map<APawn*, UMoveToLocationOrActor3D*> CurrentMovingObjects = std::map<APawn*, UMoveToLocationOrActor3D*>();

	static TArray<std::pair<APawn*, FLatentMoveToActorOrLocation3D*>> CurrentMovingPawns;
}