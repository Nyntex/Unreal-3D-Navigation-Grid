// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NavGridPathResult.h"
#include "UObject/Interface.h"
#include "PathRequester.generated.h"

UINTERFACE()
class UPathRequester : public UInterface
{
	GENERATED_BODY()
};

class NAVIGATIONGRID_API IPathRequester
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent)
	void ReceivePathRequestResult(FNavGridPathResult PathResult);
};
