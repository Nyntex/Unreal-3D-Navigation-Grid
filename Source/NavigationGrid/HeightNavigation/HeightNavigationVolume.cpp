// Fill out your copyright notice in the Description page of Project Settings.


#include "HeightNavigationVolume.h"

//#include "Builders/CubeBuilder.h"
//#include "Components/BrushComponent.h"
//#include "Engine/BrushBuilder.h"
#include "Kismet/KismetMathLibrary.h"


void AHeightNavigationVolume::DrawBox(FVector pos, FColor color) const
{
    DrawDebugBox(GetWorld(), pos, FVector(25, 25, 25), color, false, 4, 0, 15);
}

void AHeightNavigationVolume::GenerateNavNodeGrid()
{
    ClearGrid();

    SetNodeCount();

    for (int x = 0; x < xNodes; x++)
    {
        navNodeGrid.Add(F_YLayer());
        for (int y = 0; y < yNodes; y++)
        {
            navNodeGrid[x].yLayer.Add(F_ZLayer());
            for (int z = 0; z < zNodes; z++)
            {
                FNavNode node;
                node.X = x;
                node.Y = y;
                node.Z = z;
                navNodeGrid[x][y].zLayer.Add(node);
            }
        }
    }
    if (navNodeGrid.IsEmpty()) return;
    startPosition = GetWorldPositionFromNode(navNodeGrid[0][0][0]);
    endPosition = GetActorForwardVector() * GetExtents().X + GetActorRightVector() * GetExtents().Y + GetActorUpVector() * GetExtents().Z + GetActorLocation();

    SetupNeighbors();
    //ShowGrid();
}

void AHeightNavigationVolume::SetupNeighbors()
{
    if (IsGridEmpty()) return;

    TArray<FNavNode> neighbors = TArray<FNavNode>();
    

    for (int x = 0; x < xNodes; x++)
    {
        for (int y = 0; y < yNodes; y++)
        {
            for (int z = 0; z < zNodes; z++)
            {
                neighbors.Empty();
                GetNeighbors(navNodeGrid[x][y][z], navNodeGrid, neighbors);

                FVector start = GetWorldPositionFromNode(navNodeGrid[x][y][z]);
                for(FNavNode neighbor : neighbors)
                {
                    FVector end = GetWorldPositionFromNode(neighbor);

                    FCollisionQueryParams traceParams = FCollisionQueryParams(FName(TEXT("trace")), true, this);
                    traceParams.bTraceComplex = true;
                    traceParams.bReturnPhysicalMaterial = false;
                    traceParams.bFindInitialOverlaps = true;
                    FHitResult result(ForceInit);
                    FHitResult resultReversed(ForceInit);

                    GetWorld()->LineTraceSingleByChannel(result, start, end, ECC_WorldStatic, traceParams);
                    GetWorld()->LineTraceSingleByChannel(resultReversed, end, start, ECC_WorldStatic, traceParams);
                    if(!result.bBlockingHit && !resultReversed.bBlockingHit)
                    {
                        navNodeGrid[x][y][z].neighbors.Add(FVector(neighbor.X, neighbor.Y, neighbor.Z));
                    }
                }

                if (navNodeGrid[x][y][z].neighbors.IsEmpty()) navNodeGrid[x][y][z].blocked = true;

            }
        }
    }
}

void AHeightNavigationVolume::ClearGrid()
{
    if (navNodeGrid.IsEmpty()) return;
    if (navNodeGrid[0].yLayer.IsEmpty()) return;
    if (navNodeGrid[0].yLayer[0].zLayer.IsEmpty()) return;

    for(int x = navNodeGrid.Num() - 1; x >= 0; x--)
    {
        for (int y = navNodeGrid[0].yLayer.Num() - 1; y >= 0; y--)
        {
            navNodeGrid[x].yLayer[y].zLayer.Empty();
        }
        navNodeGrid[x].yLayer.Empty();
    }
    navNodeGrid.Empty();
}

void AHeightNavigationVolume::ShowGrid()
{
    for(int x = 0; x < navNodeGrid.Num(); x++)
    {
        for(int y = 0; y < navNodeGrid[0].Num(); y++)
        {
            for(int z = 0; z < navNodeGrid[0][0].Num(); z++)
            {
                FNavNode currentNode = navNodeGrid[x][y][z];
                FVector worldPosition = GetWorldPositionFromNode(currentNode);
                DrawBox(worldPosition, navNodeGrid[x][y][z].blocked == true ? FColor::Red : FColor::Green);
            }
        }
    }
}


void AHeightNavigationVolume::GetNeighbors(FNavNode node, TArray<F_YLayer>& grid, TArray<FNavNode>& neighbors)
{
    if (IsGridEmpty()) return;

    const bool forward = node.X + 1 < xNodes;
    const bool back = node.X - 1 >= 0;
    const bool right = node.Y + 1 < yNodes;
    const bool left = node.Y - 1 >= 0;
    const bool up = node.Z + 1 < zNodes;
    const bool down = node.Z - 1 >= 0;

    if (forward) neighbors.Add(grid[node.X + 1][node.Y][node.Z]);
    if (back) neighbors.Add(grid[node.X - 1][node.Y][node.Z]);

    if (right) neighbors.Add(grid[node.X][node.Y + 1][node.Z]);
    if (left) neighbors.Add(grid[node.X][node.Y - 1][node.Z]);

    if (up) neighbors.Add(grid[node.X][node.Y][node.Z + 1]);
    if (down) neighbors.Add(grid[node.X][node.Y][node.Z - 1]);
}

bool AHeightNavigationVolume::IsValid(FNavNode node) const
{
    return (node.X >= 0 && node.Y >= 0 && node.Z >= 0 &&
        node.X < xNodes && node.Y < yNodes && node.Z < zNodes);
}

bool AHeightNavigationVolume::IsValid(int x, int y, int z) const
{
    return (x >= 0 && y >= 0 && z >= 0 &&
        x < xNodes && y < yNodes && z < zNodes);
}

bool AHeightNavigationVolume::IsUnblocked(FNavNode node)
{
    if (node.X < 0 && node.Y < 0 && node.Z < 0 && node.X >= xNodes && node.Y >= yNodes && node.Z >= zNodes) return false;
    return !navNodeGrid[node.X][node.Y][node.Z].blocked;
}

bool AHeightNavigationVolume::IsUnblocked(int x, int y, int z)
{
    if (x < 0 || y < 0 || z < 0 || x >= xNodes || y >= yNodes || z >= zNodes) return false;
    return !navNodeGrid[x][y][z].blocked;
}

bool AHeightNavigationVolume::IsDestination(FNavNode node, FNavNode goal) const
{
    return (node == goal);
}

bool AHeightNavigationVolume::IsDestination(int x, int y, int z, FNavNode goal) const
{
    return (x == goal.X && y == goal.Y && z == goal.Z);
}

FNavNode AHeightNavigationVolume::GetNodeFromPosition(FVector position, TArray<F_YLayer> grid)
{
    FNavNode badNode = FNavNode();
    badNode.X = -1;
    badNode.Y = -1;
    badNode.Z = -1;
    badNode.blocked = true;

    if (!IsInsideVolume(position)) return badNode;

    int x = -1;
    for(int i = 1; i <= xNodes; i++)
    {
        x++;
        if (!IsInsideVolume(position - GetActorForwardVector() * (distanceBetweenNodes * i))) break;
    }

    int y = -1;
    for (int i = 1; i <= xNodes; i++)
    {
        y++;
        if (!IsInsideVolume(position - GetActorRightVector() * (distanceBetweenNodes * i))) break;
    }

    int z = -1;
    for (int i = 1; i <= xNodes; i++)
    {
        z++;
        if (!IsInsideVolume(position - GetActorUpVector() * (distanceBetweenNodes * i))) break;
    }

    if(x >= xNodes)
    {
        x = xNodes - 1;
    }

    if (y >= yNodes)
    {
        y = yNodes - 1;
    }

    if (z >= zNodes)
    {
        z = zNodes - 1;
    }

    FVector tempPos = position;
    FVector nodePos = GetWorldPositionFromNode(grid[x][y][z]);
    float closestDistance = FVector::Distance(nodePos, position);
    if (closestDistance < distanceBetweenNodes / 2) return grid[x][y][z];

    int tempX = x;
    int tempY = y;
    int tempZ = z;


    float temp = 0.0f;
    if (x + 1 < xNodes && y + 1 < yNodes && z + 1 < zNodes)
    {
        temp = FVector::Distance(position, GetWorldPositionFromNode(grid[x + 1][y + 1][z + 1]));
        if(closestDistance > temp)
        {
            closestDistance = temp;
            tempX = x + 1;
            tempY = y + 1;
            tempZ = z + 1;
            if (closestDistance < distanceBetweenNodes / 2) return grid[tempX][tempY][tempZ];
        }
    }

    if (x + 1 < xNodes)
    {
        temp = FVector::Distance(position, GetWorldPositionFromNode(grid[x + 1][y][z]));
        if (closestDistance > temp)
        {
            closestDistance = temp;
            tempX = x + 1;
            tempY = y;
            tempZ = z;
            if (closestDistance < distanceBetweenNodes / 2) return grid[tempX][tempY][tempZ];
        }
    }

    if (y + 1 < yNodes)
    {
        temp = FVector::Distance(position, GetWorldPositionFromNode(grid[x][y+1][z]));
        if (closestDistance > temp)
        {
            closestDistance = temp;
            tempX = x;
            tempY = y+1;
            tempZ = z;
            if (closestDistance < distanceBetweenNodes / 2) return grid[tempX][tempY][tempZ];
        }
    }

    if (z + 1 < zNodes)
    {
        temp = FVector::Distance(position, GetWorldPositionFromNode(grid[x][y][z + 1]));
        if (closestDistance > temp)
        {
            closestDistance = temp;
            tempX = x;
            tempY = y;
            tempZ = z + 1;
            if (closestDistance < distanceBetweenNodes / 2) return grid[tempX][tempY][tempZ];
        }
    }

    if (x + 1 < xNodes && y + 1 < yNodes)
    {
        temp = FVector::Distance(position, GetWorldPositionFromNode(grid[x + 1][y + 1][z]));
        if (closestDistance > temp)
        {
            closestDistance = temp;
            tempX = x + 1;
            tempY = y + 1;
            tempZ = z;
            if (closestDistance < distanceBetweenNodes / 2) return grid[tempX][tempY][tempZ];
        }
    }

    if (x + 1 < xNodes && z + 1 < zNodes)
    {
        temp = FVector::Distance(position, GetWorldPositionFromNode(grid[x+1][y][z+1]));
        if (closestDistance > temp)
        {
            closestDistance = temp;
            tempX = x+1;
            tempY = y;
            tempZ = z+1;
            if (closestDistance < distanceBetweenNodes / 2) return grid[tempX][tempY][tempZ];
        }
    }

    if (y + 1 < yNodes && z + 1 < zNodes)
    {
        temp = FVector::Distance(position, GetWorldPositionFromNode(grid[x][y+1][z+1]));
        if (closestDistance > temp)
        {
            closestDistance = temp;
            tempX = x;
            tempY = y + 1;
            tempZ = z + 1;
            if (closestDistance < distanceBetweenNodes / 2) return grid[tempX][tempY][tempZ];
        }
    }

    return grid[tempX][tempY][tempZ];
}

FVector AHeightNavigationVolume::GetWorldPositionFromNode(FNavNode node) const
{
    const FVector extents = GetExtents();
    FVector x = GetActorForwardVector() * extents.X;
    FVector y = GetActorRightVector() * extents.Y;
    FVector z = GetActorUpVector() * extents.Z;

    //const FVector startPos = GetActorLocation() - (x + y + z);
    const FVector startPos = -(x + y + z);
    //startPosition = startPos + GetActorLocation();

    x = GetActorForwardVector() * distanceBetweenNodes * node.X;
    y = GetActorRightVector() * distanceBetweenNodes * node.Y;
    z = GetActorUpVector() * distanceBetweenNodes * node.Z;

    return startPos + x + y + z + GetActorLocation();
}

FVector AHeightNavigationVolume::GetGridSize() const
{
    return FVector(xNodes, yNodes, zNodes);
}

void AHeightNavigationVolume::BeginPlay()
{
    Super::BeginPlay();
    GenerateNavNodeGrid();
}

FVector AHeightNavigationVolume::GetExtents() const
{
    //UCubeBuilder* builder = (UCubeBuilder*)GetBrushBuilder(); //This is to get Brush Settings
    //return (FVector(abs(GetActorScale().X),abs(GetActorScale().Y),abs(GetActorScale().Z)) * (FVector(builder->X, builder->Y, builder->Z) * 0.5));
    return (FVector(abs(GetActorScale().X), abs(GetActorScale().Y), abs(GetActorScale().Z)) * 100);
}

bool AHeightNavigationVolume::IsGridEmpty() const
{
    if (navNodeGrid.IsEmpty()) return true;
    if (navNodeGrid[0].yLayer.IsEmpty()) return true;
    if (navNodeGrid[0].yLayer[0].zLayer.IsEmpty()) return true;
    return false;
}

void AHeightNavigationVolume::SetNodeCount()
{
    xNodes = int((GetExtents().X * 2) / distanceBetweenNodes) + 1;
    yNodes = int((GetExtents().Y * 2) / distanceBetweenNodes) + 1;
    zNodes = int((GetExtents().Z * 2) / distanceBetweenNodes) + 1;
}

FVector AHeightNavigationVolume::GetRandomMovablePosition() const
{
    TArray<FVector> possibleSpots = TArray<FVector>();

    for(int x = 0; x < xNodes; x++)
    {
        for(int y = 0; y < yNodes; y++)
        {
            for(int z = 0; z < zNodes; z++)
            {
                if(!navNodeGrid[x][y][z].blocked)
                {
                    possibleSpots.Add(GetWorldPositionFromNode(navNodeGrid[x][y][z]));
                }
            }
        }
    }

    const int i = rand() % possibleSpots.Num();
    return possibleSpots[i];
}

float AHeightNavigationVolume::CalculateH(float x, float y, float z, FNavNode goal)
{
    float h = float(abs(x - goal.X) + abs(y - goal.Y) + abs(z - goal.Z));
    //float h = sqrt(pow(x - goal.X, 2) + pow(y - goal.Y, 2) + pow(z - goal.Z, 2));
    return h;
}

bool AHeightNavigationVolume::IsInsideVolume(FVector position) const
{
    TArray<float> angles = TArray<float>();

    FVector directionTarget = position - endPosition;
    directionTarget.Normalize();

    float angle = UKismetMathLibrary::RadiansToDegrees(acos(UE::Geometry::Dot(-GetActorForwardVector(), directionTarget)));
    angles.Add(angle);
    angle = UKismetMathLibrary::RadiansToDegrees(acos(UE::Geometry::Dot(-GetActorRightVector(), directionTarget)));
    angles.Add(angle);
    angle = UKismetMathLibrary::RadiansToDegrees(acos(UE::Geometry::Dot(-GetActorUpVector(), directionTarget)));
    angles.Add(angle);

    //Bottom most corner
    directionTarget = position - startPosition;
    directionTarget.Normalize();

    angle = UKismetMathLibrary::RadiansToDegrees(acos(UE::Geometry::Dot(GetActorForwardVector(), directionTarget)));
    angles.Add(angle);
    angle = UKismetMathLibrary::RadiansToDegrees(acos(UE::Geometry::Dot(GetActorRightVector(), directionTarget)));
    angles.Add(angle);
    angle = UKismetMathLibrary::RadiansToDegrees(acos(UE::Geometry::Dot(GetActorUpVector(), directionTarget)));
    angles.Add(angle);

    for (auto it : angles)
    {
        if (it > 90 || it < 0) return false;
    }
    return true;
}


//The Algorithm
void AHeightNavigationVolume::GetPath(FVector startPos, AActor* startActor, FVector goalPos, AActor* goalActor, Get_Success& ReturnValue, TArray<FVector>& path)
{
    ReturnValue = Get_Success::Failed;
    path.Empty();
    if (IsGridEmpty()) return;

    FNavNode startNode = FNavNode();
    FNavNode goalNode = FNavNode();

    TArray<F_YLayer> grid = navNodeGrid;

    //Setup Start Node
    {
        if (startActor != nullptr)
        {
            startNode = GetNodeFromPosition(startActor->GetActorLocation(), grid);
        }
        else if (!startPos.IsZero())
        {
            startNode = GetNodeFromPosition(startPos, grid);
        }
        else
        {
            return;
        }
    }
    if (!IsValid(startNode)) return;
    if (!IsUnblocked(startNode)) return;

    //Setup Goal Node
    {
        if (goalActor != nullptr)
        {
            goalNode = GetNodeFromPosition(goalActor->GetActorLocation(), grid);
        }
        else if (!goalPos.IsZero())
        {
            goalNode = GetNodeFromPosition(goalPos, grid);
        }
        else
        {
            return;
        }
    }
    if (!IsValid(goalNode)) return;
    if (!IsUnblocked(goalNode)) return;
    if (IsDestination(startNode, goalNode))
    {
        path.Add(goalPos);
        ReturnValue = Get_Success::Success;
        return;
    }

    //Setting up three-dimensional vectors with non constant size
    std::vector<std::vector<std::vector<bool>>> closedListBool = std::vector<std::vector<std::vector<bool>>>();

    for (int x = 0; x < xNodes; x++)
    {
        closedListBool.push_back(std::vector<std::vector<bool>>());
        for (int y = 0; y < yNodes; y++)
        {
            closedListBool[x].push_back(std::vector<bool>());
            for (int z = 0; z < zNodes; z++)
            {
                closedListBool[x][y].push_back(bool());
                closedListBool[x][y][z] = false;
            }
        }
    }

    std::priority_queue<FNavNode> openQueue = std::priority_queue<FNavNode>();

    int x = startNode.X;
    int y = startNode.Y;
    int z = startNode.Z;

    grid[x][y][z].fCost = 0;
    grid[x][y][z].gCost = 0;
    grid[x][y][z].hCost = 0;
    grid[x][y][z].parentX = x;
    grid[x][y][z].parentY = y;
    grid[x][y][z].parentZ = z;

    openQueue.push(grid[x][y][z]);


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
                if (IsDestination(neighbor.X, neighbor.Y, neighbor.Z, goalNode))
                {
                    grid[neighbor.X][neighbor.Y][neighbor.Z].parentX = currentNode.X;
                    grid[neighbor.X][neighbor.Y][neighbor.Z].parentY = currentNode.Y;
                    grid[neighbor.X][neighbor.Y][neighbor.Z].parentZ = currentNode.Z;
                    path = TracePath(grid, goalNode);
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
                    gNew = grid[currentNode.X][currentNode.Y][currentNode.Z].gCost + 1.0f;
                    hNew = CalculateH(neighbor.X, neighbor.Y, neighbor.Z, goalNode);
                    fNew = gNew + hNew;
                    if (grid[neighbor.X][neighbor.Y][neighbor.Z].fCost == FLT_MAX || grid[neighbor.X][neighbor.Y][neighbor.Z].fCost > fNew)
                    {
                        grid[neighbor.X][neighbor.Y][neighbor.Z].fCost = fNew;
                        grid[neighbor.X][neighbor.Y][neighbor.Z].gCost = gNew;
                        grid[neighbor.X][neighbor.Y][neighbor.Z].hCost = hNew;
                        grid[neighbor.X][neighbor.Y][neighbor.Z].parentX = currentNode.X;
                        grid[neighbor.X][neighbor.Y][neighbor.Z].parentY = currentNode.Y;
                        grid[neighbor.X][neighbor.Y][neighbor.Z].parentZ = currentNode.Z;

                        openQueue.push(grid[neighbor.X][neighbor.Y][neighbor.Z]);
                    }
                }
            }
        }
    }
    return;
}

TArray<FVector> AHeightNavigationVolume::TracePath(TArray<F_YLayer> grid, FNavNode goalNode)
{
    TArray<FVector> temp = TArray<FVector>();
    int x = goalNode.X;
    int y = goalNode.Y;
    int z = goalNode.Z;

    while(!(grid[x][y][z].parentX == x && grid[x][y][z].parentY == y && grid[x][y][z].parentZ == z))
    {
        //pathNodes.Add(grid[x][y][z]); //DEBUG
        temp.Add(GetWorldPositionFromNode(grid[x][y][z]));
        const FNavNode tempNode = grid[x][y][z];
        x = tempNode.parentX;
        y = tempNode.parentY;
        z = tempNode.parentZ;
    }
    temp.Add(GetWorldPositionFromNode(grid[x][y][z]));
    //pathNodes.Add(grid[x][y][z]); //DEBUG

    TArray<FVector> retVal = TArray<FVector>();
    for(int i = temp.Num() - 1; i >= 0; i--)
    {
        retVal.Add(temp[i]);
    }

    return retVal;
}






/*
// TEST FUNCTIONS
//void AHeightNavigationVolume::testAlgorithm()
//{
//    FVector start = startPosition + GetActorForwardVector() * 10 + GetActorUpVector() * 10 + GetActorRightVector() * 10;
//    FVector end = endPosition - GetActorForwardVector() * 10 - GetActorUpVector() * 10 - GetActorRightVector() * 10;
//    TArray<FVector> path = TArray<FVector>();
//    pathNodes = TArray<FNavNode>();
//    GetPath(start, nullptr, end, nullptr, navNodeGrid, path);
//    debugPath = path;
//}
//
//void AHeightNavigationVolume::showPath()
//{
//    //ShowGrid();
//    if (debugPath.IsEmpty()) return;
//
//    for(auto pos : debugPath)
//    {
//        DrawDebugBox(GetWorld(), pos, FVector(25, 25, 25),
//            FColor::Red, false, 6, 0, 15);
//    }
//
//    DrawDebugBox(GetWorld(), debugPath[0], FVector(25, 25, 25),
//        FColor::Cyan, false, 6, 0, 15);
//
//    DrawDebugBox(GetWorld(), debugPath[debugPath.Num() - 1], FVector(25, 25, 25),
//        FColor::Magenta, false, 6, 0, 15);
//}
//
//void AHeightNavigationVolume::showTestedNotes()
//{
//    if (testedNodes.IsEmpty()) return;
//
//    for(auto node : testedNodes)
//    {
//        DrawBox(GetWorldPositionFromNode(node), FColor::Green);
//    }
//
//    DrawBox(GetWorldPositionFromNode(testedNodes[0]), FColor::Red);
//
//    DrawBox(GetWorldPositionFromNode(testedNodes[testedNodes.Num()-1]), FColor::Red);
//}

//void AHeightNavigationVolume::showTestPositions()
//{
    //DrawBox(startPosition, FColor::White);
    //DrawBox(endPosition, FColor::Black);
    //DrawBox(testPosition, FColor::Green);


    //DrawBox(startPosition + GetActorForwardVector() * 100 + GetActorUpVector() * 100 + GetActorRightVector() * 100, FColor::Cyan);
    //DrawBox(endPosition - GetActorForwardVector() * 100 - GetActorUpVector() * 100 - GetActorRightVector() * 100, FColor::Orange);


    //anglesToPoint.Empty();
    //FVector directionTarget = (testPosition + GetActorForwardVector() * 100 + GetActorUpVector() * 100 + GetActorRightVector() * 100) - endPosition;
    //directionTarget.Normalize();

    //float angle = UKismetMathLibrary::RadiansToDegrees(acos(UE::Geometry::Dot(-GetActorForwardVector(), directionTarget)));
    //anglesToPoint.Add(angle);
    //angle = UKismetMathLibrary::RadiansToDegrees(acos(UE::Geometry::Dot(-GetActorRightVector(), directionTarget)));
    //anglesToPoint.Add(angle);
    //angle = UKismetMathLibrary::RadiansToDegrees(acos(UE::Geometry::Dot(-GetActorUpVector(), directionTarget)));
    //anglesToPoint.Add(angle);

    ////Bottom most corner
    //directionTarget = (testPosition + GetActorForwardVector() * 100 + GetActorUpVector() * 100 + GetActorRightVector() * 100) - startPosition;
    //directionTarget.Normalize();

    //angle = UKismetMathLibrary::RadiansToDegrees(acos(UE::Geometry::Dot(GetActorForwardVector(), directionTarget)));
    //anglesToPoint.Add(angle);
    //angle = UKismetMathLibrary::RadiansToDegrees(acos(UE::Geometry::Dot(GetActorRightVector(), directionTarget)));
    //anglesToPoint.Add(angle);
    //angle = UKismetMathLibrary::RadiansToDegrees(acos(UE::Geometry::Dot(GetActorUpVector(), directionTarget)));
    //anglesToPoint.Add(angle);

    //anglesToPoint.Empty();
    //FVector directionTarget = (startPosition + GetActorForwardVector() * 100 + GetActorUpVector() * 100 + GetActorRightVector() * 100) - endPosition;
    //directionTarget.Normalize();

    //float angle = UKismetMathLibrary::RadiansToDegrees(acos(UE::Geometry::Dot(-GetActorForwardVector(), directionTarget)));
    //anglesToPoint.Add(angle);
    //angle = UKismetMathLibrary::RadiansToDegrees(acos(UE::Geometry::Dot(-GetActorRightVector(), directionTarget)));
    //anglesToPoint.Add(angle);
    //angle = UKismetMathLibrary::RadiansToDegrees(acos(UE::Geometry::Dot(-GetActorUpVector(), directionTarget)));
    //anglesToPoint.Add(angle);

    ////Bottom most corner
    //directionTarget = (startPosition + GetActorForwardVector() * 100 + GetActorUpVector() * 100 + GetActorRightVector() * 100) - startPosition;
    //directionTarget.Normalize();

    //angle = UKismetMathLibrary::RadiansToDegrees(acos(UE::Geometry::Dot(GetActorForwardVector(), directionTarget)));
    //anglesToPoint.Add(angle);
    //angle = UKismetMathLibrary::RadiansToDegrees(acos(UE::Geometry::Dot(GetActorRightVector(), directionTarget)));
    //anglesToPoint.Add(angle);
    //angle = UKismetMathLibrary::RadiansToDegrees(acos(UE::Geometry::Dot(GetActorUpVector(), directionTarget)));
    //anglesToPoint.Add(angle);


    //directionTarget = (endPosition - GetActorForwardVector() * 100 - GetActorUpVector() * 100 - GetActorRightVector() * 100) - endPosition;
    //directionTarget.Normalize();

    //angle = UKismetMathLibrary::RadiansToDegrees(acos(UE::Geometry::Dot(-GetActorForwardVector(), directionTarget)));
    //anglesToPoint.Add(angle);
    //angle = UKismetMathLibrary::RadiansToDegrees(acos(UE::Geometry::Dot(-GetActorRightVector(), directionTarget)));
    //anglesToPoint.Add(angle);
    //angle = UKismetMathLibrary::RadiansToDegrees(acos(UE::Geometry::Dot(-GetActorUpVector(), directionTarget)));
    //anglesToPoint.Add(angle);

    ////Bottom most corner
    //directionTarget = (endPosition - GetActorForwardVector() * 100 - GetActorUpVector() * 100 - GetActorRightVector() * 100) - startPosition;
    //directionTarget.Normalize();

    //angle = UKismetMathLibrary::RadiansToDegrees(acos(UE::Geometry::Dot(GetActorForwardVector(), directionTarget)));
    //anglesToPoint.Add(angle);
    //angle = UKismetMathLibrary::RadiansToDegrees(acos(UE::Geometry::Dot(GetActorRightVector(), directionTarget)));
    //anglesToPoint.Add(angle);
    //angle = UKismetMathLibrary::RadiansToDegrees(acos(UE::Geometry::Dot(GetActorUpVector(), directionTarget)));
    //anglesToPoint.Add(angle);
//}
*/