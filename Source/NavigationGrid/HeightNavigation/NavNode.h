// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NavNode.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType, Blueprintable)
struct FNavNode
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	int X = 0;
	UPROPERTY(VisibleAnywhere)
	int Y = 0;
	UPROPERTY(VisibleAnywhere)
	int Z = 0;

	UPROPERTY(VisibleAnywhere)
	int parentX = -1;
	UPROPERTY(VisibleAnywhere)
	int parentY = -1;
	UPROPERTY(VisibleAnywhere)
	int parentZ = -1;

	UPROPERTY(VisibleAnywhere)
	bool blocked = false;

	UPROPERTY(VisibleAnywhere)
	float gCost = FLT_MAX;
	UPROPERTY(VisibleAnywhere)
	float hCost = FLT_MAX;
	UPROPERTY(VisibleAnywhere)
	float fCost = FLT_MAX;

	UPROPERTY(VisibleAnywhere)
	TArray<FVector> neighbors = TArray<FVector>();
};

//Same Node when they share one position
inline bool operator==(const FNavNode& first, const FNavNode& second)
{
	if (first.X == second.X && first.Y == second.Y && first.Z == second.Z) return true;
	return false;
}

//inverted < operator to see what has lower costs, one is lower when it has greater cost
inline bool operator<(const FNavNode& first, const FNavNode& second)
{
	return first.fCost > second.fCost;
}

//inverted > operator to see what has lower costs, one is greater when it has lower cost
inline bool operator>(const FNavNode& first, const FNavNode& second)
{
	return first.fCost < second.fCost;
}