#pragma once
#include "CoreMinimal.h"

// Room encounter rules are pure data: the world generator owns RNG and
// passes all rolls explicitly so repeated run seeds remain reproducible.
enum class EAVEncounterKind : uint8
{
    Standard = 0,
    HunterPatrol = 1,
    Swarm = 2,
    EliteGuard = 3
};

struct FAVEncounterPlan
{
    EAVEncounterKind Kind = EAVEncounterKind::Standard;
    int32 EnemyCount = 2;
    int32 GuaranteedElites = 0;
    float EliteChance = 0.10f;
    float EquipmentChance = 0.085f;
};

class FAVEncounters
{
public:
    // RoomArea measures walkable room capacity approximately in cells.
    // Rolls are clamped internally; same arguments always give same result.
    static FAVEncounterPlan Plan(int32 Floor, int32 RoomArea,
                                 int32 KindRoll, int32 CountRoll);

    // Selects one of three biome-valid enemy candidates. Hunter patrols favor
    // ranged types, swarms favor melee types, and elite guards favor high HP.
    static int32 SelectType(int32 Floor, EAVEncounterKind Kind,
                            int32 FirstRoll, int32 SecondRoll, int32 ThirdRoll);

    static const char* KindName(EAVEncounterKind Kind);
};
