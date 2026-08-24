// Fill out your copyright notice in the Description page of Project Settings.


#include "AAMoveToLocationOrActor3D.h"

#include "NavigationGrid/MoveToLocationOrActor3DStatics.h"

UAAMoveToLocationOrActor3D* UAAMoveToLocationOrActor3D::MoveToLocationOrActor3D(APawn* WorldContext, FVector Location)
{
	UAAMoveToLocationOrActor3D* Action = NewObject<UAAMoveToLocationOrActor3D>();
	Action->MovingTarget = WorldContext;
	Action->LocationToMoveTo = Location;

	UAAMoveToLocationOrActor3D** FoundObject = MoveToLocationOrActor3DStatics::CurrentMovingObjects.Find(WorldContext);

	if(FoundObject && *FoundObject)
	{
		(*FoundObject)->CancelMovement();
		MoveToLocationOrActor3DStatics::CurrentMovingObjects.Remove(WorldContext);
	}

	MoveToLocationOrActor3DStatics::CurrentMovingObjects.Emplace(WorldContext, Action);

	return Action;
}

void UAAMoveToLocationOrActor3D::Activate()
{
	if (!IsValid(GEngine) ||
		!IsValid(MovingTarget))
	{
		OnFailed.Broadcast();
		FinishMovement();
		return;
	}
	
	Super::Activate();
	
	World = GEngine->GetWorldFromContextObject(MovingTarget, EGetWorldErrorMode::ReturnNull);

#if WITH_EDITOR
	if(World == nullptr && GEditor != nullptr)
	{
		World = GEditor->GetEditorWorldContext().World();
	}
#endif

	if(!IsValid(World))
	{
		OnFailed.Broadcast();
		FinishMovement();
		return;
	}
}

void UAAMoveToLocationOrActor3D::CancelMovement()
{
	OnCanceled.Broadcast();
	FinishMovement();
}

void UAAMoveToLocationOrActor3D::Tick(float DeltaTime)
{
	if (!!IsValid(MovingTarget) || !IsValid(World))
	{
		return;
	}
	
	UpdateMove(DeltaTime);
	OnMove.Broadcast();
}

void UAAMoveToLocationOrActor3D::UpdateMove(float DeltaTime)
{

}

void UAAMoveToLocationOrActor3D::FinishMovement()
{
	MoveToLocationOrActor3DStatics::CurrentMovingObjects.Remove(MovingTarget);
	MovingTarget = nullptr;
	World = nullptr;
	SetReadyToDestroy();
}
