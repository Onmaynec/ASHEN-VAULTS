#pragma once
#include "CoreMinimal.h"

// Pure deterministic layout module: used by Unreal and standalone connectivity QA.
struct FAVFortressRoom
{
    int32 X=0,Y=0,W=0,H=0;
};
class FAVFortressLayout
{
public:
    static constexpr int32 Width=60;
    static constexpr int32 Height=44;
    static constexpr int32 RoomCount=9;
    static void Build(int32 Seed,TArray<uint8>& Tiles,TArray<FAVFortressRoom>& Rooms);
};
