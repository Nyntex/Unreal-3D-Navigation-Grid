// Fill out your copyright notice in the Description page of Project Settings.


#include "MoveToLocationOrActor3D.h"

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
	UpdateMove(DeltaTime);
	OnMove.Broadcast();
}

void UMoveToLocationOrActor3D::UpdateMove(float DeltaTime)
{

}

void UMoveToLocationOrActor3D::FinishMovement()
{
	//MoveTicker.UnRegisterTickFunction();
	MoveToLocationOrActor3DStatics::CurrentMovingObjects.erase(MovingTarget);

	SetReadyToDestroy();
}