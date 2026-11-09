#pragma once
#include "CoreMinimal.h"

// Pure dungeon rules. No UObject, RNG state, or assets: the game can test
// geometry and spawn behavior independently of the Unreal editor.
struct FAVRoomBounds
{
    int32 X=0, Y=0, W=0, H=0;
    FVector2D Center() const
    {
        return FVector2D(X+0.5f*W, Y+0.5f*H);
    }
};
enum class EAVRoomGeometry : uint8
{
    OpenCourt, TwinPillars, FallenColumns, SplitHall,
    GuardPosts, ZigzagBaffles, SideAlcove, CentralIsland
};
enum class EAVFormation : uint8
{
    Perimeter, FrontLine, RearGuard, Pincer,
    Pack, Wedge, Staggered, CornerSentries
};
enum class EAVWaveTrigger : uint8
{
    None, OnEntry, AfterClear, OnChestOpen, OnFirstKill
};
struct FAVWaveDirective
{
    EAVWaveTrigger Trigger=EAVWaveTrigger::None;
    EAVFormation Formation=EAVFormation::Perimeter;
    int32 UnitCount=0;
    int32 Kind=0; // EAVEncounterKind ordinal; avoid gameplay type dependency.
    float WarningSeconds=1.1f;
};
class FAVDepthDirector
{
public:
    static uint32 Hash(uint32 Value);
    static EAVRoomGeometry Geometry(int32 Seed,int32 Floor,int32 RoomIndex,int32 Biome);
    static EAVFormation Formation(int32 Seed,int32 Floor,int32 RoomIndex,int32 EncounterKind);
    static FAVWaveDirective Wave(int32 Seed,int32 Floor,int32 RoomIndex,
                                  int32 RoomArea,int32 EncounterKind);
    // True only when walkable with room for a standard mob footprint.
    static bool ClearCell(const TArray<uint8>& Tiles,int32 Width,int32 Height,
                          const FVector2D& P,float Radius=0.28f);
    static void ApplyGeometry(TArray<uint8>& Tiles,int32 Width,int32 Height,
                              const FAVRoomBounds& R,EAVRoomGeometry Geometry);
    static void CarveAlternativeRoute(TArray<uint8>& Tiles,int32 Width,int32 Height,
                                      const FAVRoomBounds& A,const FAVRoomBounds& B,
                                      bool bHorizontalFirst);
    // Repairs isolated islands created by room shapes / broken columns, not
    // bandit gates: must run before placing camp doors.
    static int32 RepairConnectivity(TArray<uint8>& Tiles,int32 Width,int32 Height,
                                    const FVector2D& Start,int32 MaxRepairs=32);
    static FVector2D CandidateSpawn(const FAVRoomBounds& R,EAVFormation Formation,
                                    int32 Index,int32 Count);
    static bool FindSpawn(const TArray<uint8>& Tiles,int32 Width,int32 Height,
                          const FAVRoomBounds& R,EAVFormation Formation,
                          int32 Index,int32 Count,const FVector2D& Hero,
                          const FVector2D& Exit,const FVector2D& Shrine,
                          const TArray<FVector2D>& Occupied,FVector2D& Out,
                          float HeroBuffer=4.f);
    static bool InRoom(const FAVRoomBounds& R,const FVector2D& Pos);
    static bool ShouldArmWave(EAVWaveTrigger Trigger,bool bInside,
                              bool bLiveEnemyInside,bool bOpenedChest,
                              bool bKillWitnessed);
    static bool AdvanceWarning(float& Seconds,float DeltaSeconds);
};
