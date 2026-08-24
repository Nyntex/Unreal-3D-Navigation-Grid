#pragma once
#include "Navigation/PathFollowingComponent.h"

#include "NavGridPathResult.generated.h"



USTRUCT()
struct NAVIGATIONGRID_API FNavGridPathResult
{
	GENERATED_BODY()
public:
	TArray<FVector> PathPoints;
	
	EPathFollowingRequestResult::Type Type;	
};
