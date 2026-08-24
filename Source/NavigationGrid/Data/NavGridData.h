// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NavigationGrid/HeightNavigation/HeightNavigationVolume.h"
#include "NavGridData.generated.h"


USTRUCT()
struct NAVIGATIONGRID_API FNavGridData
{
	GENERATED_BODY()
public:
	
	FTransform GridTransform{};
	
	TArray<F_YLayer> GridLayers{};
	
	int32 XNodes = 0;
	int32 YNodes = 0;
	int32 ZNodes = 0;
};
