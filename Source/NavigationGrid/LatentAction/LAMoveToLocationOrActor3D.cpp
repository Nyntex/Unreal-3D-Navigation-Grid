// Fill out your copyright notice in the Description page of Project Settings.


#include "LAMoveToLocationOrActor3D.h"

#include "NavigationGrid/HeightNavigation/HeightNavigationVolume.h"
#include "Kismet/KismetMathLibrary.h"
#include "NavigationGrid/MoveToLocationOrActor3DStatics.h"


#define ResponseLatentInfo LatentActionInfo.ExecutionFunction, LatentActionInfo.Linkage, LatentActionInfo.CallbackTarget

void ULAMoveToLocationOrActor3D::MoveToActorOrLocation3D(APawn* WorldContext, FLatentActionInfo LatentInfo,
	EMoveInputPins InputPins, EMoveOutputPins& OutputPins, FVector MoveLocation, FVector& CurrentMoveDirection)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull);

#if WITH_EDITOR
	if(World == nullptr && GEditor != nullptr)
	{
		World = GEditor->GetEditorWorldContext().World();
	}
#endif

	if(World == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Latent Move To Actor or Location Failed - No world available!!!"));
		return;
	}

	FLatentActionManager& LatentActionManager = World->GetLatentActionManager();

	FLatentMoveToActorOrLocation3D* ExistingAction = LatentActionManager.FindExistingAction<FLatentMoveToActorOrLocation3D>(nullptr, LatentInfo.UUID);

	Stop3DMovement(WorldContext);

	switch (InputPins) {
	case EMoveInputPins::Start:
	{
		//Cancel any existing action

		//Even though this instance is getting created with new, I do not have to worry about deleting it, Unreal does it for me
		FLatentMoveToActorOrLocation3D* Action = new FLatentMoveToActorOrLocation3D(LatentInfo, OutputPins, WorldContext, MoveLocation, CurrentMoveDirection);
		LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, Action);

		MoveToLocationOrActor3DStatics::CurrentMovingPawns.Add({ WorldContext, Action });
	}
	default:
		{}
	}
}

void ULAMoveToLocationOrActor3D::Stop3DMovement(APawn* WorldContext)
{
	if (!WorldContext->IsValidLowLevel()) return;

	for(TPair<APawn*, FLatentMoveToActorOrLocation3D*>& Pair : MoveToLocationOrActor3DStatics::CurrentMovingPawns)
	{
		if (!Pair.Key) return;

		if (Pair.Key == WorldContext)
		{
			if (!Pair.Value)
			{
				return;
			}
			
			Pair.Value->Output = EMoveOutputPins::OnCanceled;
#if WITH_EDITOR
			GEditor->AddOnScreenDebugMessage(INDEX_NONE, 5, FColor::Red,
				TEXT("Latent Action Movement Stopped for ") + WorldContext->GetName());
#endif
		}
	}
}

void FLatentMoveToActorOrLocation3D::UpdateOperation(FLatentResponse& Response)
{
	switch (Output)
	{
	case EMoveOutputPins::OnCanceled:
	case EMoveOutputPins::OnCompleted:
	case EMoveOutputPins::OnFailed:
	{
		for (int i = MoveToLocationOrActor3DStatics::CurrentMovingPawns.Num()-1; i >= 0; --i)
		{
			const TPair<APawn*, FLatentMoveToActorOrLocation3D*>& Pair = MoveToLocationOrActor3DStatics::CurrentMovingPawns[i];
			if (Pair.Key == MovementTarget && Pair.Value->LatentActionInfo.UUID == LatentActionInfo.UUID)
			{
				MoveToLocationOrActor3DStatics::CurrentMovingPawns.RemoveAt(i, EAllowShrinking::No);
				break;
			}
		}
		Response.FinishAndTriggerIf(true, ResponseLatentInfo);
		return;
	}
	default:
	{}
	}

	if(IsFirstCall)
	{
		GetNewPath();
		if(Path.IsEmpty() && Output != EMoveOutputPins::OnCompleted)
		{
			Output = EMoveOutputPins::OnFailed;
#if WITH_EDITOR
			GEditor->AddOnScreenDebugMessage(INDEX_NONE, 5, FColor::Red, 
				TEXT("Latent Move To Actor or Location Failed - No path available!"));
#endif
			UE_LOG(LogTemp, Error, TEXT("Latent Move To Actor or Location Failed - No path available!"));
			return;
		}

		IsFirstCall = false;
		Output = EMoveOutputPins::OnStarted;
		Response.TriggerLink(ResponseLatentInfo);
		return;
	}

	if(UKismetMathLibrary::Vector_Distance(MoveLocation, MovementTarget->GetActorLocation()) <= ClosenessThreshold) //Done moving?
	{
		Output = EMoveOutputPins::OnCompleted; //Will get completed during next cycle at the  start of the function
		return;
	}

	//Standard Move behavior
	if (PathIndex + 1 != Path.Num()) UpdateDirectPath(Response.ElapsedTime());

	if (!PathValidationCheck(0))
	{
		Output = EMoveOutputPins::OnFailed;
#if WITH_EDITOR
		GEditor->AddOnScreenDebugMessage(INDEX_NONE, 5, FColor::Red,
			TEXT("Latent Move To Actor or Location Failed - Path went missing during execution!"));
		DrawDebugBox(MovementTarget->GetWorld(), MoveLocation, FVector(50, 50, 50), FColor::Black, false, 10, 0, 25);
		DrawDebugBox(MovementTarget->GetWorld(), MoveLocation, FVector(150, 150, 150), FColor::Black, false, 10, 0, 25);
		DrawDebugBox(MovementTarget->GetWorld(), MoveLocation, FVector(300, 300,300), FColor::Black, false, 10, 0, 25);
#endif
		UE_LOG(LogTemp, Error, TEXT("Latent Move To Actor or Location Failed - Path went missing during execution!"));

		return;
	}

	UpdateMovement();

	Output = EMoveOutputPins::OnMove;
	Response.TriggerLink(ResponseLatentInfo);
}

void FLatentMoveToActorOrLocation3D::GetNewPath()
{
	//Check if position has a straight path
	FHitResult HitResult;
	MovementTarget->GetWorld()->LineTraceSingleByChannel(HitResult, MovementTarget->GetActorLocation(), MoveLocation, ECC_Visibility);

	if(!HitResult.bBlockingHit)
	{
		Path.Empty();
		Path.Add(MoveLocation);
		PathIndex = 0;
		return;
	}

	//Check for Nav Grid
	AHeightNavigationVolume* NavGrid = AHeightNavigationVolume::EvaluateNavGrid(MovementTarget, MovementTarget->GetActorLocation(), MoveLocation);
	if(!NavGrid)
	{
#if WITH_EDITOR
		GEditor->AddOnScreenDebugMessage(INDEX_NONE, 5, FColor::Red,
			TEXT("Positions do not fit into any one Height Navigation Volume."));
#endif
		UE_LOG(LogTemp, Error, TEXT("Positions do not fit into any one Height Navigation Volume."));
		Output = EMoveOutputPins::OnFailed;
		return;
	}

	//Generate a path through Nav Grid
	Get_Success success = Get_Success::Failed;
	NavGrid->GetPath(MovementTarget->GetActorLocation(), nullptr, MoveLocation, nullptr, success, Path);

#if WITH_EDITOR
	for(int i = 0; i < Path.Num()-2; ++i)
	{
		DrawDebugLine(MovementTarget->GetWorld(), Path[i], Path[i+1], FColor::Cyan, false, 5);
	}
#endif

	if (Path.IsEmpty()) Output = EMoveOutputPins::OnCompleted;
}

void FLatentMoveToActorOrLocation3D::UpdateMovement()
{
	CurrentMoveDirection = DirectionToLocation(Path[PathIndex]);
	MoveInDirection(CurrentMoveDirection);
	

	//is close enough to move to next point?
	if ((MovementTarget->GetActorLocation() - Path[PathIndex]).Length() >= ClosenessThreshold) return;

	//Is there a next possible point?
	if (PathIndex + 1 >= Path.Num()) return;

	//set next possible point
	PathIndex++;
}

void FLatentMoveToActorOrLocation3D::UpdateDirectPath(float DeltaTime)
{
	//Timer
	CurrentInterval += DeltaTime;
	if (CurrentInterval < Interval) return;
	CurrentInterval -= Interval;


	//Do we need to check for a direct path or are we almost at our goal
	if (PathIndex + 2 >= Path.Num()) return;

	//Evaluate the next position to check
	IndexToCheck += 2;
	if (IndexToCheck >= Path.Num())
	{
		IndexToCheck = PathIndex;
		return;
	}

	//Check if we have a direct path to the goal location and set it accordingly
	{
		if (HasDirectAccessToLocation(MoveLocation))
		{
			Path.Empty();
			PathIndex = 0;
			Path.Add(MoveLocation);
			return;
		}
	}

	//Check if we have a direct path to any previous location
	//this check should be run over multiple frames
	{
		if (HasDirectAccessToLocation(Path[IndexToCheck]))
		{
			PathIndex = IndexToCheck;
			return;
		}
	}
}

bool FLatentMoveToActorOrLocation3D::HasDirectAccessToLocation(const FVector& Location, bool ShowLines) const
{
	FHitResult HitResultTop(ForceInit);
	FHitResult HitResultBot(ForceInit);
	FHitResult HitResultLeft(ForceInit);
	FHitResult HitResultRight(ForceInit);

	UWorld* world = MovementTarget->GetWorld();
	FVector Origin;
	FVector BoxExtent;
	MovementTarget->GetActorBounds(true, Origin, BoxExtent);

	world->LineTraceSingleByChannel(HitResultTop, MovementTarget->GetActorLocation() + (MovementTarget->GetActorUpVector() * BoxExtent.Z * 1.5f), Location, ECC_Visibility);
	world->LineTraceSingleByChannel(HitResultBot, MovementTarget->GetActorLocation() - (MovementTarget->GetActorUpVector() * BoxExtent.Z * 1.5f), Location, ECC_Visibility);
	world->LineTraceSingleByChannel(HitResultLeft, MovementTarget->GetActorLocation() - (MovementTarget->GetActorRightVector() * BoxExtent.X * 1.5f), Location, ECC_Visibility);
	world->LineTraceSingleByChannel(HitResultRight, MovementTarget->GetActorLocation() + (MovementTarget->GetActorRightVector() * BoxExtent.X * 1.5f), Location, ECC_Visibility);

#if WITH_EDITOR
	if(ShowLines)
	{
		DrawDebugLine(MovementTarget->GetWorld(), MovementTarget->GetActorLocation() + (MovementTarget->GetActorUpVector() * BoxExtent.Z * 1.5f), Location, HitResultTop.bBlockingHit ? FColor::Red : FColor::Green, false, Interval);
		DrawDebugLine(MovementTarget->GetWorld(), MovementTarget->GetActorLocation() - (MovementTarget->GetActorUpVector() * BoxExtent.Z * 1.5f), Location, HitResultBot.bBlockingHit ? FColor::Red : FColor::Green, false, Interval);
		DrawDebugLine(MovementTarget->GetWorld(), MovementTarget->GetActorLocation() - (MovementTarget->GetActorRightVector() * BoxExtent.X * 1.5f), Location, HitResultLeft.bBlockingHit ? FColor::Red : FColor::Green, false, Interval);
		DrawDebugLine(MovementTarget->GetWorld(), MovementTarget->GetActorLocation() + (MovementTarget->GetActorRightVector() * BoxExtent.X * 1.5f), Location, HitResultRight.bBlockingHit ? FColor::Red : FColor::Green, false, Interval);
	}
#endif

	return !HitResultTop.bBlockingHit && !HitResultBot.bBlockingHit && !HitResultLeft.bBlockingHit && !HitResultRight.bBlockingHit;
}

void FLatentMoveToActorOrLocation3D::MoveInDirection(FVector Direction)
{
	MovementTarget->AddMovementInput(Direction);
}

bool FLatentMoveToActorOrLocation3D::PathValidationCheck(int Try)
{
	Try++;
	bool Valid = false;

	if (!Path.IsEmpty()) Valid = true;

	if(!Valid)
	{
		GetNewPath();
#if WITH_EDITOR
		GEditor->AddOnScreenDebugMessage(INDEX_NONE, 5, FColor::Red,
			TEXT("Tried to receive a new path to target location. Try ") + FString::SanitizeFloat(Try) + TEXT(" ."));
#endif
		UE_LOG(LogTemp, Error, TEXT("Tried to receive a new path to target location. Try %d."), Try);
	}

	if (Try >= 3) return Valid;
	return Valid ? true : PathValidationCheck(Try);
}

FVector FLatentMoveToActorOrLocation3D::DirectionToLocation(FVector Location)
{
	return DirectionToLocation(MovementTarget->GetActorLocation(), Location);
}

FVector FLatentMoveToActorOrLocation3D::DirectionToLocation(FVector StartLocation, FVector Location)
{
	FVector Direction = Location - StartLocation;
	Direction.Normalize();
	return Direction;
}