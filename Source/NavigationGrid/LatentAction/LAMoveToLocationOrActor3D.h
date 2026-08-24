// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UObject/Object.h"
#include "LAMoveToLocationOrActor3D.generated.h"

UENUM()
enum class EMoveInputPins : uint8
{
	Start,		//Starts movement
	Cancel,		//Cancels movement
};

UENUM()
enum class EMoveOutputPins : uint8
{
	OnStarted		UMETA(DisplayName = "On Started"),		//Called only once on start
	OnMove			UMETA(DisplayName = "On Move"),			//Every Frame
	OnCanceled		UMETA(DisplayName = "On Canceled"),		//When node is canceled
	OnCompleted		UMETA(DisplayName = "On Completed"),	//when the movement is completed
	OnFailed		UMETA(DisplayName = "On Failed"),		//When movement failed
};

UCLASS()
class NAVIGATIONGRID_API ULAMoveToLocationOrActor3D : public UObject
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
	UFUNCTION(BlueprintCallable, DisplayName="Latent Move to Location or Actor 3D",meta=(WorldContext = "WorldContext", Latent, LatentInfo = "LatentInfo", 
		ExpandEnumAsExecs = "InputPins,OutputPins"), Category = "Navigation Grid | Latent")
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

	bool HasDirectAccessToLocation(const FVector& Location, bool ShowLines = true) const;

#if WITH_EDITOR
	virtual FString GetDescription() const override
	{
		FString Text = MovementTarget.GetName() + TEXT(" is moving from ") + MovementTarget->GetActorLocation().ToString() + TEXT(" to ") + MoveLocation.ToString() + TEXT("\n The current direction is ") + CurrentMoveDirection.ToString();
		return Text;
	}
#endif

protected:
	TArray<FVector> Path{};

	int PathIndex = 0;

	float ClosenessThreshold = 50.f;
	
	float Interval = 0.3f;
	float CurrentInterval = 0.3f;
	int IndexToCheck = 0;
};
