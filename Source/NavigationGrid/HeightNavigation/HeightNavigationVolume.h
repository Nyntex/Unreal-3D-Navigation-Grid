// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Volume.h"
#include "NavNode.h"
#include "HeightNavigationVolume.generated.h"

UENUM(BlueprintType)
enum class Get_Success : uint8
{
    Success,
	Failed
};

USTRUCT(BlueprintType, Blueprintable)
struct F_ZLayer
{
    GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly)
	TArray<FNavNode> zLayer = TArray<FNavNode>();

	FNavNode& operator[](int i)
	{
		return zLayer[i];
	}

	const FNavNode& operator[](int i) const
	{
		return zLayer[i];
	}

	int Num() const
	{
		return zLayer.Num();
	}
};

USTRUCT(BlueprintType, Blueprintable)
struct F_YLayer
{
    GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly)
	TArray<F_ZLayer> yLayer = TArray<F_ZLayer>();

	F_ZLayer& operator[](int i)
	{
		return yLayer[i];
	}

	const F_ZLayer& operator[](int i) const
	{
		return yLayer[i];
	}

	int Num() const
	{
		return yLayer.Num();
	}
};

/**
 * 
 */
UCLASS()
class NAVIGATIONGRID_API AHeightNavigationVolume : public AVolume
{
	GENERATED_BODY()

	//Functions
public:
	void DrawBox(FVector pos, FColor color) const;

	//Grid Functions
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Height Navigation Volume")
	void GenerateNavNodeGrid();
	void SetupNeighbors();
	void GetNeighbors(FNavNode node, TArray<F_YLayer>& grid, TArray<FNavNode>& neighbors);
	UFUNCTION(BlueprintCallable, Category = "Height Navigation Volume")
    FVector GetExtents() const;
	UFUNCTION(BlueprintCallable, Category = "Height Navigation Volume")
	bool IsGridEmpty() const;
	void InitializeNodeCount();
	UFUNCTION(BlueprintCallable, Category = "Height Navigation Volume")
	FVector GetRandomMovablePosition() const;

	//Input either a world position or an actor
	//When putting in an Actor it is converted to the Actor Location
	//Actor is getting prioritized over the position so set one of them, not both
	//Returns: An Array of Vector3 where the first position is the first Node to move to
	//and the last position is the position you put in
	UFUNCTION(BlueprintCallable, Category = "Height Navigation Volume", meta=(ExpandEnumAsExecs="ReturnValue"))
	void GetPath(FVector startPos, AActor* startActor, FVector goalPos, AActor* goalActor, Get_Success& ReturnValue, TArray<FVector>& path);
	TArray<FVector> TracePath(TArray<F_YLayer> grid, FNavNode goalNode);
	float CalculateH(float x, float y, float z, FNavNode goal);
	UFUNCTION(BlueprintCallable, Category = "Height Navigation Volume")
	bool IsInsideVolume(FVector position) const;

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Height Navigation Volume")
	void ClearGrid();
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Height Navigation Volume")
	void ShowGrid();

	//Is the position of the node inside the the boundaries or not
	bool IsValid(FNavNode node) const;
	bool IsValid(int x, int y, int z) const ;

	//Is the position inside the grid unblocked and therefore able to be moved to
	bool IsUnblocked(FNavNode node);
	bool IsUnblocked(int x, int y, int z);

	//Is the given node at the same position as the goal Node
	bool IsDestination(FNavNode node, FNavNode goal) const;
	bool IsDestination(int x, int y, int z, FNavNode goal) const;

	//Takes the world position and sets it into context of the grid and
	//returns a Node that as closest to the given point
	FNavNode GetNodeFromPosition(FVector position, TArray<F_YLayer> grid);

	//Converts the position of a Node to world position
	FVector GetWorldPositionFromNode(FNavNode node) const;

	FVector GetGridSize() const;

	void BeginPlay() override;

	/*
	//TEST FUNCTIONS!!!!
	//UFUNCTION(CallInEditor, Category = "Height Navigation Volume", meta = (EditCondition = "showDebugSettings==true", EditConditionHides))
	//void testAlgorithm();
	//UFUNCTION(CallInEditor, Category = "Height Navigation Volume", meta = (EditCondition = "showDebugSettings==true", EditConditionHides))
	//void showPath();
	//UFUNCTION(CallInEditor, Category = "Height Navigation Volume", meta = (EditCondition = "showDebugSettings==true", EditConditionHides))
	//void showTestedNotes();
	//UFUNCTION(CallInEditor, Category = "Height Navigation Volume", meta = (EditCondition = "showDebugSettings==true", EditConditionHides))
	//void showTestPositions();
	*/

	//Member
public:
	UPROPERTY(EditAnywhere, Category="Height Navigation Volume", meta=(Units="cm"), BlueprintReadOnly)
	float distanceBetweenNodes = 800;

protected:
	UPROPERTY(EditInstanceOnly, Category = "Height Navigation Volume")
	bool showDebugSettings = false;

	//UPROPERTY(VisibleInstanceOnly, Category = "Height Navigation Volume", meta = (EditCondition = "showDebugSettings==true", EditConditionHides))
	TArray<F_YLayer> navNodeGrid = TArray<F_YLayer>();

	UPROPERTY(VisibleInstanceOnly, Category = "Height Navigation Volume", meta = (EditCondition = "showDebugSettings==true", EditConditionHides))
	FVector startPosition = FVector();
	UPROPERTY(VisibleInstanceOnly, Category = "Height Navigation Volume", meta = (EditCondition = "showDebugSettings==true", EditConditionHides))
	FVector endPosition = FVector();
	UPROPERTY(VisibleInstanceOnly, Category = "Height Navigation Volume", meta = (EditCondition = "showDebugSettings==true", EditConditionHides))
	FVector endNode = FVector();

	UPROPERTY(VisibleInstanceOnly, Category = "Height Navigation Volume", meta = (EditCondition = "showDebugSettings==true", EditConditionHides))
	int xNodes{ 0 };
	UPROPERTY(VisibleInstanceOnly, Category = "Height Navigation Volume", meta = (EditCondition = "showDebugSettings==true", EditConditionHides))
	int yNodes{ 0 };
	UPROPERTY(VisibleInstanceOnly, Category = "Height Navigation Volume", meta = (EditCondition = "showDebugSettings==true", EditConditionHides))
    int zNodes{ 0 };

	UPROPERTY(VisibleInstanceOnly, Category = "Height Navigation Volume", meta = (EditCondition = "showDebugSettings==true", EditConditionHides))
	TArray<AHeightNavigationVolume*> overlappingVolumes = TArray<AHeightNavigationVolume*>();

	//UPROPERTY(VisibleInstanceOnly, Category = "Height Navigation Volume", meta = (EditCondition = "showDebugSettings==true", EditConditionHides))
	//int steps = 0;

	/*
	//UPROPERTY(EditInstanceOnly, Category = "Height Navigation Volume", meta = (EditCondition = "showDebugSettings==true", EditConditionHides))
	//TArray<float> anglesToPoint = TArray<float>();
	//UPROPERTY(EditInstanceOnly, Category = "Height Navigation Volume", meta = (EditCondition = "showDebugSettings==true", EditConditionHides))
	//FVector testPosition = FVector();

	//UPROPERTY(VisibleInstanceOnly, Category = "Height Navigation Volume", meta = (EditCondition = "showDebugSettings==true", EditConditionHides))
	//TArray<FVector> debugPath = TArray<FVector>();
	//UPROPERTY(VisibleInstanceOnly, Category = "Height Navigation Volume", meta = (EditCondition = "showDebugSettings==true", EditConditionHides))
	//TArray<FNavNode> pathNodes = TArray<FNavNode>();
	//UPROPERTY(VisibleInstanceOnly, Category = "Height Navigation Volume", meta = (EditCondition = "showDebugSettings==true", EditConditionHides))
	//TArray<FNavNode> testedNodes = TArray<FNavNode>();
	*/
};
