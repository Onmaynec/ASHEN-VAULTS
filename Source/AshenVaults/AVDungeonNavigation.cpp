#include "AVDungeonNavigation.h"

bool FAVDungeonNavigation::IsWalkable(const TArray<uint8>& Tiles, int32 Width,
                                      int32 Height, int32 X, int32 Y)
{
    return X >= 0 && Y >= 0 && X < Width && Y < Height &&
           Tiles.IsValidIndex(Y * Width + X) && Tiles[Y * Width + X] != 0;
}

bool FAVDungeonNavigation::LineOfSight(const TArray<uint8>& Tiles, int32 Width,
                                       int32 Height, const FVector2D& From,
                                       const FVector2D& To)
{
    const FVector2D Delta = To - From;
    const int32 Steps = FMath::Max(1, FMath::CeilToInt(Delta.Size() * 4.f));
    for (int32 I = 0; I <= Steps; ++I)
    {
        const FVector2D P = From + Delta * (static_cast<float>(I) / Steps);
        if (!IsWalkable(Tiles, Width, Height, FMath::FloorToInt(P.X),
                         FMath::FloorToInt(P.Y)))
            return false;
    }
    return true;
}

void FAVDungeonNavigation::Reveal(const TArray<uint8>& Tiles, int32 Width, int32 Height,
                                   const FVector2D& Hero, float Radius,
                                   TArray<uint8>& Visible, TArray<uint8>& Explored)
{
    if (Visible.Num() != Width * Height) Visible.Init(0, Width * Height);
    else for (uint8& Value : Visible) Value = 0;
    if (Explored.Num() != Width * Height) Explored.Init(0, Width * Height);
    const int32 CX = FMath::FloorToInt(Hero.X);
    const int32 CY = FMath::FloorToInt(Hero.Y);
    const int32 R = FMath::CeilToInt(Radius);
    for (int32 Y = FMath::Max(0, CY - R); Y <= FMath::Min(Height - 1, CY + R); ++Y)
        for (int32 X = FMath::Max(0, CX - R); X <= FMath::Min(Width - 1, CX + R); ++X)
        {
            if (!IsWalkable(Tiles, Width, Height, X, Y)) continue;
            const FVector2D Target(X + 0.5f, Y + 0.5f);
            if (FVector2D::Distance(Hero, Target) > Radius) continue;
            if (!LineOfSight(Tiles, Width, Height, Hero, Target)) continue;
            Visible[Y * Width + X] = 1;
            Explored[Y * Width + X] = 1;
        }
}

void FAVDungeonNavigation::BuildFlow(const TArray<uint8>& Tiles, int32 Width, int32 Height,
                                      const FVector2D& Hero, TArray<int32>& Distances)
{
    Distances.Init(-1, Width * Height);
    const int32 X = FMath::FloorToInt(Hero.X);
    const int32 Y = FMath::FloorToInt(Hero.Y);
    if (!IsWalkable(Tiles, Width, Height, X, Y)) return;
    TArray<FIntPoint> Queue;
    Queue.Reserve(Width * Height);
    Queue.Add(FIntPoint(X, Y));
    Distances[Y * Width + X] = 0;
    const int32 DX[4] = { 0, 1, 0, -1 };
    const int32 DY[4] = { -1, 0, 1, 0 };
    for (int32 Head = 0; Head < Queue.Num(); ++Head)
    {
        const FIntPoint Cell = Queue[Head];
        const int32 Base = Distances[Cell.Y * Width + Cell.X];
        for (int32 Dir = 0; Dir < 4; ++Dir)
        {
            const int32 NX = Cell.X + DX[Dir];
            const int32 NY = Cell.Y + DY[Dir];
            if (!IsWalkable(Tiles, Width, Height, NX, NY)) continue;
            const int32 Index = NY * Width + NX;
            if (Distances[Index] >= 0) continue;
            Distances[Index] = Base + 1;
            Queue.Add(FIntPoint(NX, NY));
        }
    }
}

FVector2D FAVDungeonNavigation::FlowDirection(const TArray<uint8>& Tiles,
                                               const TArray<int32>& Distances,
                                               int32 Width, int32 Height,
                                               const FVector2D& From,
                                               const FVector2D& Hero)
{
    const FVector2D Direct = (Hero - From).GetSafeNormal();
    if (LineOfSight(Tiles, Width, Height, From, Hero)) return Direct;

    const int32 X = FMath::FloorToInt(From.X);
    const int32 Y = FMath::FloorToInt(From.Y);
    if (!IsWalkable(Tiles, Width, Height, X, Y) ||
        Distances.Num() != Width * Height) return Direct;

    int32 Best = Distances[Y * Width + X];
    if (Best < 0) return Direct;
    FVector2D Target(X + 0.5f, Y + 0.5f);
    const int32 DX[4] = { 0, 1, 0, -1 };
    const int32 DY[4] = { -1, 0, 1, 0 };
    for (int32 I = 0; I < 4; ++I)
    {
        const int32 NX = X + DX[I], NY = Y + DY[I];
        if (!IsWalkable(Tiles, Width, Height, NX, NY)) continue;
        const int32 Distance = Distances[NY * Width + NX];
        if (Distance < 0 || Distance >= Best) continue;
        Best = Distance;
        Target = FVector2D(NX + 0.5f, NY + 0.5f);
    }
    return (Target - From).GetSafeNormal();
}
