#pragma once
#include "CoreMinimal.h"
#include "AVDepthDirector.h"
#include "AVEncounters.h"

// 24 independent, seed-stable room rules. One geometry, one garrison and
// one response rule are chosen per room (8 x 8 x 8 combinations).
enum class EAVRoomShapeRule : uint8
{
    ClearLane, CrossCover, BrokenRing, TwinIslands,
    FlankDoors, BarricadeLine, NarrowThroat, OpenPocket
};
enum class EAVGarrisonRule : uint8
{
    Balanced, Vanguard, RearGuard, Crossfire,
    PackedSwarm, VeteranPair, ArmoredLine, Skirmishers
};
enum class EAVResponseRule : uint8
{
    Quiet, EntryAlarm, ClearRetaliation, TreasureAmbush,
    FirstBlood, DelayedAlarm, EliteCounterattack, DoubleFlank
};
struct FAVRoomTactic
{
    EAVRoomShapeRule Shape=EAVRoomShapeRule::ClearLane;
    EAVGarrisonRule Garrison=EAVGarrisonRule::Balanced;
    EAVResponseRule Response=EAVResponseRule::Quiet;
};

class FAVRoomTactics
{
public:
    static constexpr int32 ShapeCount=8;
    static constexpr int32 GarrisonCount=8;
    static constexpr int32 ResponseCount=8;
    static FAVRoomTactic Select(int32 Seed,int32 Floor,int32 Room,int32 Biome);
    static void ApplyShape(TArray<uint8>& Tiles,int32 Width,int32 Height,
                           const FAVRoomBounds& Room,EAVRoomShapeRule Shape);
    static FAVEncounterPlan Garrison(FAVEncounterPlan Plan,EAVGarrisonRule Rule,
                                     int32 Area,int32 Floor);
    static EAVFormation Formation(EAVGarrisonRule Rule,EAVFormation Original);
    static EAVEncounterKind EnemyRole(EAVGarrisonRule Rule,EAVEncounterKind Original);
    static FAVWaveDirective Response(FAVWaveDirective Wave,EAVResponseRule Rule,
                                     int32 Area,int32 Floor);
    static const char* ShapeName(EAVRoomShapeRule Rule);
    static const char* GarrisonName(EAVGarrisonRule Rule);
    static const char* ResponseName(EAVResponseRule Rule);
};
