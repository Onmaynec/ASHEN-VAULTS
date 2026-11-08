#include "AVEncounters.h"
#include "AVContent.h"

namespace
{
    int32 Percentile(int32 Value)
    {
        return FMath::Clamp(Value, 0, 99);
    }

    bool PreferredRole(int32 Type, int32 Floor, EAVEncounterKind Kind)
    {
        const bool bRanged = FAVContent::Enemy(Type, Floor).Ranged;
        return Kind == EAVEncounterKind::HunterPatrol ? bRanged :
               Kind == EAVEncounterKind::Swarm ? !bRanged : true;
    }
}

FAVEncounterPlan FAVEncounters::Plan(int32 Floor, int32 RoomArea,
                                    int32 KindRoll, int32 CountRoll)
{
    const int32 Depth = (FMath::Max(1, Floor) - 1) % FAVContent::FloorsPerBiome;
    const int32 Base = 2 + (Depth >= 2 ? 1 : 0) + (Depth >= 4 ? 1 : 0);
    const int32 KindValue = Percentile(KindRoll);
    const int32 SizeValue = Percentile(CountRoll);
    const int32 Capacity = FMath::Clamp(FMath::Max(0, RoomArea) / 5, 3, 8);

    FAVEncounterPlan Result;
    if (KindValue < 45)
    {
        Result.Kind = EAVEncounterKind::Standard;
        Result.EnemyCount = Base + SizeValue % 3;
        Result.EliteChance = FMath::Min(0.26f, 0.10f + 0.025f * FMath::Max(1, Floor));
    }
    else if (KindValue < 65)
    {
        Result.Kind = EAVEncounterKind::HunterPatrol;
        Result.EnemyCount = Base + 1 + SizeValue % 2;
        Result.EliteChance = 0.12f;
    }
    else if (KindValue < 87)
    {
        Result.Kind = EAVEncounterKind::Swarm;
        Result.EnemyCount = Base + 2 + SizeValue % 3;
        Result.EliteChance = 0.06f;
        Result.EquipmentChance = 0.045f;
    }
    else
    {
        Result.Kind = EAVEncounterKind::EliteGuard;
        Result.EnemyCount = FMath::Max(2, Base - 1 + SizeValue % 2);
        Result.EliteChance = 0.35f;
        Result.EquipmentChance = 0.22f;
        Result.GuaranteedElites = 1;
    }
    Result.EnemyCount = FMath::Clamp(Result.EnemyCount, 1, Capacity);
    return Result;
}

int32 FAVEncounters::SelectType(int32 Floor, EAVEncounterKind Kind,
                               int32 FirstRoll, int32 SecondRoll, int32 ThirdRoll)
{
    const int32 Candidates[3] =
    {
        FAVContent::EnemyRoll(Floor, Percentile(FirstRoll)),
        FAVContent::EnemyRoll(Floor, Percentile(SecondRoll)),
        FAVContent::EnemyRoll(Floor, Percentile(ThirdRoll))
    };
    if (Kind == EAVEncounterKind::EliteGuard)
    {
        int32 Chosen = Candidates[0];
        float BestHP = FAVContent::Enemy(Chosen, Floor).Health;
        for (int32 Index = 1; Index < 3; ++Index)
        {
            const float HP = FAVContent::Enemy(Candidates[Index], Floor).Health;
            if (HP > BestHP)
            {
                Chosen = Candidates[Index];
                BestHP = HP;
            }
        }
        return Chosen;
    }
    if (Kind == EAVEncounterKind::HunterPatrol || Kind == EAVEncounterKind::Swarm)
    {
        for (int32 Type : Candidates)
            if (PreferredRole(Type, Floor, Kind)) return Type;
    }
    return Candidates[0];
}

const char* FAVEncounters::KindName(EAVEncounterKind Kind)
{
    switch (Kind)
    {
    case EAVEncounterKind::Standard: return "STANDARD";
    case EAVEncounterKind::HunterPatrol: return "HUNTER PATROL";
    case EAVEncounterKind::Swarm: return "SWARM";
    case EAVEncounterKind::EliteGuard: return "ELITE GUARD";
    default: return "STANDARD";
    }
}
