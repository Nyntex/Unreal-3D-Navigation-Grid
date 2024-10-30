// Fill out your copyright notice in the Description page of Project Settings.


#include "MoveToLocationOrActor3D.h"

#include "VectorTypes.h"

void UMoveToActorOrLocation3D::Stop3DMovement(APawn* WorldContext)
{
	if (!WorldContext) return;

	auto it = MoveToLocationOrActor3DStatics::CurrentMovingPawns.find(WorldContext);
	if (it != MoveToLocationOrActor3DStatics::CurrentMovingPawns.end())
	{
		it->second->WantToCancel = true;
		MoveToLocationOrActor3DStatics::CurrentMovingPawns.erase(it);
		Stop3DMovement(WorldContext);
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

	if(World == nullptr && GEditor != nullptr)
	{
		World = GEditor->GetEditorWorldContext().World();
	}

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
		UE_LOG(LogTemp, Error, TEXT("Latent Move To Actor or Location Failed - No world available!"));
		return;
	}
	FLatentActionManager& LatentActionManager = World->GetLatentActionManager();

	FLatentMoveToActorOrLocation3D* ExistingAction = LatentActionManager.FindExistingAction<FLatentMoveToActorOrLocation3D>(LatentInfo.CallbackTarget, LatentInfo.UUID);
	
	switch (InputPins) {
	case EMoveInputPins::Start:
	{
		//Cancel any existing action
		Stop3DMovement(WorldContext);

		FLatentMoveToActorOrLocation3D* Action;

		if (!ExistingAction)
		{
			//Even though this instance is getting created with new, I do not have to worry about deleting it, Unreal does it for me
			Action = new FLatentMoveToActorOrLocation3D(LatentInfo, OutputPins, WorldContext, MoveLocation, CurrentMoveDirection);
			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, Action);
		}
		else
		{
			ExistingAction->WantToCancel = true;
			Action = new FLatentMoveToActorOrLocation3D(LatentInfo, OutputPins, WorldContext, MoveLocation, CurrentMoveDirection);
			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, Action);
		}

			//MoveToLocationOrActor3DStatics::CurrentMovingPawns.try_emplace(WorldContext, Action);
		MoveToLocationOrActor3DStatics::CurrentMovingPawns.insert({ WorldContext, Action });
		
	}
	case EMoveInputPins::Cancel:
	{
		if(ExistingAction)
		{
			ExistingAction->WantToCancel = true;

			auto it = MoveToLocationOrActor3DStatics::CurrentMovingPawns.find(WorldContext);
			if (it != MoveToLocationOrActor3DStatics::CurrentMovingPawns.end())
			{
				MoveToLocationOrActor3DStatics::CurrentMovingPawns.erase(it);
			}
		}
	}
	}
}

#define ResponseLatentInfo LatentActionInfo.ExecutionFunction, LatentActionInfo.Linkage, LatentActionInfo.CallbackTarget

void FLatentMoveToActorOrLocation3D::UpdateOperation(FLatentResponse& Response)
{
	//FPendingLatentAction::UpdateOperation(Response);

	if(WantToCancel)
	{
		//MoveToLocationOrActor3DStatics::CurrentMovingPawns.erase(MovementTarget);
		Output = EMoveOutputPins::OnCanceled;
		Response.FinishAndTriggerIf(true, ResponseLatentInfo);
		return;
	}

	float DeltaTime = Response.ElapsedTime();

	if(IsFirstCall)
	{
		GetNewPath();
		IsFirstCall = false;
		Output = EMoveOutputPins::OnStarted;
		Response.TriggerLink(ResponseLatentInfo);
		return;
	}

	if(UE::Geometry::Distance(MoveLocation, MovementTarget->GetActorLocation()) <= 50) //Done moving?
	{
		MoveToLocationOrActor3DStatics::CurrentMovingPawns.erase(MovementTarget);
		Output = EMoveOutputPins::OnCompleted;
		Response.FinishAndTriggerIf(true, ResponseLatentInfo);
		return;
	}

	//Standard Move behavior
	UpdateMovement(DeltaTime);
	Output = EMoveOutputPins::OnMove;
	Response.TriggerLink(ResponseLatentInfo);
}

void FLatentMoveToActorOrLocation3D::GetNewPath()
{

}

void FLatentMoveToActorOrLocation3D::UpdateMovement(float DeltaTime)
{

}

#pragma endregion
