

#pragma once

#include "NavigationGrid/LatentAction/LAMoveToLocationOrActor3D.h"
#include "NavigationGrid/AsyncAction/AAMoveToLocationOrActor3D.h"


namespace MoveToLocationOrActor3DStatics
{
	//This Array will limit the amount of actions to one per object so that the same
	//Action will not get called on the same target twice.
	static TMap<APawn*, UAAMoveToLocationOrActor3D*> CurrentMovingObjects{};

	static TArray<TPair<APawn*, FLatentMoveToActorOrLocation3D*>> CurrentMovingPawns;
	
	static TMap<uint32, FLatentMoveToActorOrLocation3D*> CurrentMovingIds;
};
