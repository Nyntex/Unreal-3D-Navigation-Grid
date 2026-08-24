// Fill out your copyright notice in the Description page of Project Settings.

#include "NavigationGridSubsystem.h"

#include "PathRequester.h"

UNavigationGridSubsystem* UNavigationGridSubsystem::Get(const UObject* WorldContextObject)
{
	UWorld* World = IsValid(WorldContextObject) ? WorldContextObject->GetWorld() : nullptr;
	if (!IsValid(World))
	{
		return nullptr;
	}
	
	return World->GetSubsystem<UNavigationGridSubsystem>();
}

void UNavigationGridSubsystem::RequestPath(UObject* PathRequester, const FNavGridMovingData& NaveGridMovingData)
{
	if (!IsValid(PathRequester) ||
		!PathRequester->Implements<UPathRequester>())
	{
		return;
	}
	
	
	
	//todo
}

void UNavigationGridSubsystem::RegisterNavVolume(const FNavGridData& NavGridData)
{
	AvailableVolumes.Emplace(NavGridData);
}

void UNavigationGridSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    TArray<TPair<UObject*, std::future<FNavGridPathResult>>> PairArray = PathResults.Array();
    for (int i = PathResults.Num()-1; i >= 0; --i)
    {
        TPair<UObject*, std::future<FNavGridPathResult>>& Pair = PairArray[i];
        if (Pair.Value.valid())
        {
            IPathRequester::Execute_ReceivePathRequestResult(Pair.Key, Pair.Value.get());
            PathResults.Remove(Pair.Key);
        }
    }
}

FNavGridPathResult UNavigationGridSubsystem::FindPath(FVector startPos, AActor* startActor,
                                                      const FNavGridMovingData NavGridMovingData, TArray<FNavGridData> NavGridVolumes)
{
    FNavGridPathResult Path;
    if (NavGridVolumes.IsEmpty())
    {
        return FNavGridPathResult{};
    }

    FNavNode StartNode = FNavNode();
    FNavNode GoalNode = FNavNode();

    TArray<F_YLayer> Grid = navNodeGrid;

    //Setup Start Node
    {
        if (startActor != nullptr)
        {
            StartNode = GetNodeFromPosition(startActor->GetActorLocation(), Grid);
        }
        else if (!startPos.IsZero())
        {
            StartNode = GetNodeFromPosition(startPos, Grid);
        }
        else
        {
            return;
        }
    }
    if (!IsValid(StartNode)) return;
    if (!IsUnblocked(StartNode)) return;

    //Setup Goal Node
    {
        if (goalActor != nullptr)
        {
            GoalNode = GetNodeFromPosition(goalActor->GetActorLocation(), Grid);
        }
        else if (!goalPos.IsZero())
        {
            GoalNode = GetNodeFromPosition(goalPos, Grid);
        }
        else
        {
            return;
        }
    }
    if (!IsValid(GoalNode)) return;
    if (!IsUnblocked(GoalNode)) return;
    if (IsDestination(StartNode, GoalNode))
    {
        path.Add(goalPos);
        ReturnValue = Get_Success::Success;
        return;
    }

    //Setting up three-dimensional vectors with non constant size
    std::vector<std::vector<std::vector<bool>>> closedListBool = std::vector<std::vector<std::vector<bool>>>();

    closedListBool.reserve(xNodes);
    for (int x = 0; x < xNodes; x++)
    {
        closedListBool.push_back(std::vector<std::vector<bool>>());
        closedListBool[x].reserve(yNodes);
        for (int y = 0; y < yNodes; y++)
        {
            closedListBool[x].push_back(std::vector<bool>());
            closedListBool[x][y].reserve(zNodes);
            for (int z = 0; z < zNodes; z++)
            {
                closedListBool[x][y].push_back(bool());
                closedListBool[x][y][z] = false;
            }
        }
    }

    std::priority_queue<FNavNode> openQueue = std::priority_queue<FNavNode>();

    int x = StartNode.X;
    int y = StartNode.Y;
    int z = StartNode.Z;

    Grid[x][y][z].fCost = 0;
    Grid[x][y][z].gCost = 0;
    Grid[x][y][z].hCost = 0;
    Grid[x][y][z].parentX = x;
    Grid[x][y][z].parentY = y;
    Grid[x][y][z].parentZ = z;

    openQueue.push(Grid[x][y][z]);


    //steps = 0;
    while (!openQueue.empty())
    {
        //steps++;
        FNavNode currentNode = openQueue.top();
        openQueue.pop();

        x = currentNode.X;
        y = currentNode.Y;
        z = currentNode.Z;

        closedListBool[currentNode.X][currentNode.Y][currentNode.Z] = true;

        float gNew, hNew, fNew;

        for(auto neighbor : currentNode.neighbors)
        {
            if(IsValid(neighbor.X, neighbor.Y, neighbor.Z))
            {
                if (IsDestination(neighbor.X, neighbor.Y, neighbor.Z, GoalNode))
                {
                    Grid[neighbor.X][neighbor.Y][neighbor.Z].parentX = currentNode.X;
                    Grid[neighbor.X][neighbor.Y][neighbor.Z].parentY = currentNode.Y;
                    Grid[neighbor.X][neighbor.Y][neighbor.Z].parentZ = currentNode.Z;
                    path = TracePath(Grid, GoalNode);
                    if (goalActor != nullptr)
                    {
                        path.Emplace(goalActor->GetActorLocation());
                    }
                    else if (&goalPos != nullptr)
                    {
                        path.Emplace(goalPos);
                    }
                    ReturnValue = Get_Success::Success;
                    return;
                }
                if (closedListBool[neighbor.X][neighbor.Y][neighbor.Z] == false)
                {
                    gNew = Grid[currentNode.X][currentNode.Y][currentNode.Z].gCost + 1.0f;
                    hNew = CalculateH(neighbor.X, neighbor.Y, neighbor.Z, GoalNode);
                    fNew = gNew + hNew;
                    if (Grid[neighbor.X][neighbor.Y][neighbor.Z].fCost == FLT_MAX || Grid[neighbor.X][neighbor.Y][neighbor.Z].fCost > fNew)
                    {
                        Grid[neighbor.X][neighbor.Y][neighbor.Z].fCost = fNew;
                        Grid[neighbor.X][neighbor.Y][neighbor.Z].gCost = gNew;
                        Grid[neighbor.X][neighbor.Y][neighbor.Z].hCost = hNew;
                        Grid[neighbor.X][neighbor.Y][neighbor.Z].parentX = currentNode.X;
                        Grid[neighbor.X][neighbor.Y][neighbor.Z].parentY = currentNode.Y;
                        Grid[neighbor.X][neighbor.Y][neighbor.Z].parentZ = currentNode.Z;

                        openQueue.push(Grid[neighbor.X][neighbor.Y][neighbor.Z]);
                    }
                }
            }
        }
    }
    return;
}
