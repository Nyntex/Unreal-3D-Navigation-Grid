// Fill out your copyright notice in the Description page of Project Settings.


#include "MoveToLocationOrActor3D.h"

#include "VectorTypes.h"
#include "HeightNavigation/HeightNavigationVolume.h"

void UMoveToActorOrLocation3D::Stop3DMovement(APawn* WorldContext)
{
	if (!WorldContext->IsValidLowLevel()) return;

	//auto it = MoveToLocationOrActor3DStatics::CurrentMovingPawns.find(WorldContext);
	for(auto it : MoveToLocationOrActor3DStatics::CurrentMovingPawns)
	{
		if (!it.first) return;

		if (it.first == WorldContext)
		{
			if (!it.second) return;
			it.second->Output = EMoveOutputPins::OnCanceled;
#if WITH_EDITOR
			GEditor->AddOnScreenDebugMessage(INDEX_NONE, 5, FColor::Red,
				TEXT("Latent Action Movement Stopped for ") + WorldContext->GetName());
#endif
		}
	}
}

#pragma region AsyncAction
UMoveToLocationOrActor3D* UMoveToLocationOrActor3D::MoveToLocationOrActor3D(APawn* WorldContext, FVector Location)
{
	UMoveToLocationOrActor3D* Action = NewObject<UMoveToLocationOrActor3D>();
	Action->MovingTarget = WorldContext;
	Action->LocationToMoveTo = Location;

	auto iterator = MoveToLocationOrActor3DStatics::CurrentMovingObjects.find(WorldContext);

	if(iterator != MoveToLocationOrActor3DStatics::CurrentMovingObjects.end())
	{
		iterator->second->CancelMovement();
		MoveToLocationOrActor3DStatics::CurrentMovingObjects.erase(iterator);
	}

	MoveToLocationOrActor3DStatics::CurrentMovingObjects.try_emplace(WorldContext, Action);

	return Action;
}

void UMoveToLocationOrActor3D::Activate()
{
	Super::Activate();

	World = GEngine->GetWorldFromContextObject(MovingTarget, EGetWorldErrorMode::ReturnNull);

#if WITH_EDITOR
	if(World == nullptr && GEditor != nullptr)
	{
		World = GEditor->GetEditorWorldContext().World();
	}
#endif

	if(!World)
	{
		OnFailed.Broadcast();
		FinishMovement();
		return;
	}
}

void UMoveToLocationOrActor3D::CancelMovement()
{
	OnCanceled.Broadcast();
	FinishMovement();
}

void UMoveToLocationOrActor3D::Tick(float DeltaTime)
{
	if (!MovingTarget || !World) return;
	UpdateMove(DeltaTime);
	OnMove.Broadcast();
}

void UMoveToLocationOrActor3D::UpdateMove(float DeltaTime)
{

}

void UMoveToLocationOrActor3D::FinishMovement()
{
	MoveToLocationOrActor3DStatics::CurrentMovingObjects.erase(MovingTarget);
	MovingTarget = nullptr;
	World = nullptr;
	SetReadyToDestroy();
}
#pragma endregion

#pragma region LatentAction
void UMoveToActorOrLocation3D::MoveToActorOrLocation3D(APawn* WorldContext, FLatentActionInfo LatentInfo,
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

#define ResponseLatentInfo LatentActionInfo.ExecutionFunction, LatentActionInfo.Linkage, LatentActionInfo.CallbackTarget

void FLatentMoveToActorOrLocation3D::UpdateOperation(FLatentResponse& Response)
{
	//FPendingLatentAction::UpdateOperation(Response);

	switch (Output)
	{
	case EMoveOutputPins::OnCanceled:
	case EMoveOutputPins::OnCompleted:
	case EMoveOutputPins::OnFailed:
	{
		std::pair<APawn*, FLatentMoveToActorOrLocation3D*>* PairToRemove = nullptr;
			for(std::pair<APawn*, FLatentMoveToActorOrLocation3D*> pair : MoveToLocationOrActor3DStatics::CurrentMovingPawns)
			{
				if (pair.first == MovementTarget && pair.second->LatentActionInfo.UUID == LatentActionInfo.UUID)
				{
					PairToRemove = &pair;
					break;
				}
			}
			if (PairToRemove)
			{
				MoveToLocationOrActor3DStatics::CurrentMovingPawns.Remove(*PairToRemove);
			}
			else
			{
#if WITH_EDITOR
				GEditor->AddOnScreenDebugMessage(INDEX_NONE, 5, FColor::Red,
					TEXT("Latent Action Movement 3D - Finishing up action but it was never added to existing actions?"));
#endif
				UE_LOG(LogTemp, Warning, TEXT("Latent Action Movement 3D - Finishing up action but it was never added to existing actions?"));
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

	if(UE::Geometry::Distance(MoveLocation, MovementTarget->GetActorLocation()) <= ClosenessThreshold) //Done moving?
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

	FHitResult HitResultTop;
	FHitResult HitResultBot;
	FHitResult HitResultLeft;
	FHitResult HitResultRight;

	UWorld* world = MovementTarget->GetWorld();
	FVector Origin;
	FVector BoxExtent;
	MovementTarget->GetActorBounds(true, Origin, BoxExtent);

	//Check if we have a direct path to the goal location and set it accordingly
	{
		world->LineTraceSingleByChannel(HitResultTop, MovementTarget->GetActorLocation() + (MovementTarget->GetActorUpVector() * BoxExtent.Z * 1.5f), MoveLocation, ECC_Visibility);
		world->LineTraceSingleByChannel(HitResultBot, MovementTarget->GetActorLocation() - (MovementTarget->GetActorUpVector() * BoxExtent.Z * 1.5f), MoveLocation, ECC_Visibility);
		world->LineTraceSingleByChannel(HitResultLeft, MovementTarget->GetActorLocation() - (MovementTarget->GetActorRightVector() * BoxExtent.X * 1.5f), MoveLocation, ECC_Visibility);
		world->LineTraceSingleByChannel(HitResultRight, MovementTarget->GetActorLocation() + (MovementTarget->GetActorRightVector() * BoxExtent.X * 1.5f), MoveLocation, ECC_Visibility);

		if (!HitResultTop.bBlockingHit && !HitResultBot.bBlockingHit && !HitResultLeft.bBlockingHit && !HitResultRight.bBlockingHit)
		{
			Path.Empty();
			PathIndex = 0;
			Path.Add(MoveLocation);
			return;
		}

#if WITH_EDITOR
		DrawDebugLine(MovementTarget->GetWorld(), MovementTarget->GetActorLocation() + (MovementTarget->GetActorUpVector() * BoxExtent.Z * 1.5f), MoveLocation, HitResultTop.bBlockingHit ? FColor::Red : FColor::Green, false, Interval);
		DrawDebugLine(MovementTarget->GetWorld(), MovementTarget->GetActorLocation() - (MovementTarget->GetActorUpVector() * BoxExtent.Z * 1.5f), MoveLocation, HitResultBot.bBlockingHit ? FColor::Red : FColor::Green, false, Interval);
		DrawDebugLine(MovementTarget->GetWorld(), MovementTarget->GetActorLocation() - (MovementTarget->GetActorRightVector() * BoxExtent.X * 1.5f), MoveLocation, HitResultLeft.bBlockingHit ? FColor::Red : FColor::Green, false, Interval);
		DrawDebugLine(MovementTarget->GetWorld(), MovementTarget->GetActorLocation() + (MovementTarget->GetActorRightVector() * BoxExtent.X * 1.5f), MoveLocation, HitResultRight.bBlockingHit ? FColor::Red : FColor::Green, false, Interval);
#endif
	}

	//Check if we have a direct path to any previous location
	//this check should be run over multiple frames
	{
		world->LineTraceSingleByChannel(HitResultTop, MovementTarget->GetActorLocation() + (MovementTarget->GetActorUpVector() * BoxExtent.Z * 1.5f), Path[IndexToCheck], ECC_Visibility);
		world->LineTraceSingleByChannel(HitResultBot, MovementTarget->GetActorLocation() - (MovementTarget->GetActorUpVector() * BoxExtent.Z * 1.5f), Path[IndexToCheck], ECC_Visibility);
		world->LineTraceSingleByChannel(HitResultLeft, MovementTarget->GetActorLocation() - (MovementTarget->GetActorRightVector() * BoxExtent.X * 1.5f), Path[IndexToCheck], ECC_Visibility);
		world->LineTraceSingleByChannel(HitResultRight, MovementTarget->GetActorLocation() + (MovementTarget->GetActorRightVector() * BoxExtent.X * 1.5f), Path[IndexToCheck], ECC_Visibility);

		if (!HitResultTop.bBlockingHit && !HitResultBot.bBlockingHit && !HitResultLeft.bBlockingHit && !HitResultRight.bBlockingHit)
		{
			PathIndex = IndexToCheck;
			return;
		}

#if WITH_EDITOR
		DrawDebugLine(MovementTarget->GetWorld(), MovementTarget->GetActorLocation() + (MovementTarget->GetActorUpVector() * BoxExtent.Z * 1.5f), Path[IndexToCheck], HitResultTop.bBlockingHit ? FColor::Red : FColor::Green, false, Interval);
		DrawDebugLine(MovementTarget->GetWorld(), MovementTarget->GetActorLocation() - (MovementTarget->GetActorUpVector() * BoxExtent.Z * 1.5f), Path[IndexToCheck], HitResultBot.bBlockingHit ? FColor::Red : FColor::Green, false, Interval);
		DrawDebugLine(MovementTarget->GetWorld(), MovementTarget->GetActorLocation() - (MovementTarget->GetActorRightVector() * BoxExtent.X * 1.5f), Path[IndexToCheck], HitResultLeft.bBlockingHit ? FColor::Red : FColor::Green, false, Interval);
		DrawDebugLine(MovementTarget->GetWorld(), MovementTarget->GetActorLocation() + (MovementTarget->GetActorRightVector() * BoxExtent.X * 1.5f), Path[IndexToCheck], HitResultRight.bBlockingHit ? FColor::Red : FColor::Green, false, Interval);
#endif
	}



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
#pragma endregion
