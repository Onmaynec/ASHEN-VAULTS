#pragma once
#include "CoreMinimal.h"
// Node IDs are stable SaveGame identifiers. 0..14 universal; 15..38 class-specific.
struct FAVSkillSpec {
    int32 Id;
    const char* CodeName;
    int32 RequiredHero; // -1 universal, 0 warden, 1 duelist, 2 arcanist, 3 marksman
    int32 Branch;       // 0 survival, 1 combat, 2 mobility, 3 class
    int32 MaxRank;
    int32 MinLevel;
    int32 PrerequisiteId; // -1 no prerequisite
    int32 ExclusiveGroup; // 0 none, 1..4 mutually exclusive class capstones
    int32 EffectId;     // interpreted by the hero stat/ability pipeline
    float ValuePerRank;
};
class FAVSkills
{
public:
    static constexpr int32 Count=39;
    // Skill node IDs (not EffectId): stable across saves.
    static constexpr int32 ArcanistSphereSkillId=28;
    static constexpr int32 ArcanistRuneWaveSkillId=29;
    static const FAVSkillSpec* Find(int32 Id);
    static const char* Description(int32 Id);
    static bool CanLearn(int32 Id,int32 Hero,int32 Level,const uint8* Ranks,int32 RankCount,int32 Points);
    static int32 AwardedPointsAtLevel(int32 Level); // levels 2 through 30, max 29
};
