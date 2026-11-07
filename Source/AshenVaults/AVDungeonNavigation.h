#pragma once

#include "CoreMinimal.h"

/**
 * Engine-only, deterministic visibility + grid navigation helpers.
 * Tile value 0 is blocking; nonzero is walkable. No UObject / allocations
 * beyond caller-owned TArrays. Four-way BFS follows 1-tile corridors.
 */
class FAVDungeonNavigation
{
public:
    static bool IsWalkable(const TArray<uint8>& Tiles, int32 Width, int32 Height,
                           int32 X, int32 Y);
    static bool LineOfSight(const TArray<uint8>& Tiles, int32 Width, int32 Height,
                            const FVector2D& From, const FVector2D& To);
    static void Reveal(const TArray<uint8>& Tiles, int32 Width, int32 Height,
                       const FVector2D& Hero, float Radius,
                       TArray<uint8>& Visible, TArray<uint8>& Explored);
    static void BuildFlow(const TArray<uint8>& Tiles, int32 Width, int32 Height,
                          const FVector2D& Hero, TArray<int32>& Distances);
    static FVector2D FlowDirection(const TArray<uint8>& Tiles,
                                   const TArray<int32>& Distances,
                                   int32 Width, int32 Height,
                                   const FVector2D& From, const FVector2D& Hero);
};
