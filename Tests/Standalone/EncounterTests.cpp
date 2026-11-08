#include "AVEncounters.h"
#include "AVContent.h"
#include "AVSkills.h"
#include <cassert>
#include <iostream>

int main()
{
    const int32 Floors[] = {1, 2, 5, 6, 10, 11, 15, 16, 20, 21, 40, 80, 120};
    int32 TotalPlans = 0;
    int32 TotalSelections = 0;
    for (const int32 Floor : Floors)
    {
        int32 Weights[4] = {0, 0, 0, 0};
        int32 StandardRanged = 0, PatrolRanged = 0;
        int32 StandardMelee = 0, SwarmMelee = 0;
        for (int32 Roll = 0; Roll < 100; ++Roll)
        {
            for (const int32 Area : {0, 25, 36, 80, 200})
            {
                const FAVEncounterPlan Plan = FAVEncounters::Plan(
                    Floor, Area, Roll, (Roll * 31 + 7) % 100);
                const FAVEncounterPlan Repeat = FAVEncounters::Plan(
                    Floor, Area, Roll, (Roll * 31 + 7) % 100);
                const int32 Index = static_cast<int32>(Plan.Kind);
                assert(Index >= 0 && Index <= 3);
                ++Weights[Index];
                assert(Plan.EnemyCount == Repeat.EnemyCount);
                assert(Plan.Kind == Repeat.Kind);
                assert(Plan.EliteChance == Repeat.EliteChance);
                assert(Plan.EquipmentChance == Repeat.EquipmentChance);
                assert(Plan.GuaranteedElites >= 0);
                assert(Plan.GuaranteedElites <= Plan.EnemyCount);
                assert(Plan.EnemyCount >= 1 && Plan.EnemyCount <= 8);
                const int32 Capacity = FMath::Clamp(FMath::Max(0, Area) / 5, 3, 8);
                assert(Plan.EnemyCount <= Capacity);
                assert(Plan.EliteChance >= 0.f && Plan.EliteChance <= 1.f);
                assert(Plan.EquipmentChance >= 0.f && Plan.EquipmentChance <= 1.f);
                if (Plan.Kind == EAVEncounterKind::EliteGuard)
                {
                    assert(Plan.GuaranteedElites == 1);
                    assert(Plan.EquipmentChance > 0.085f);
                }
                else assert(Plan.GuaranteedElites == 0);
                ++TotalPlans;
            }

            // The three candidates are independent deterministic percentile
            // samples; the source roster remains tied to the current biome.
            const int32 A = Roll;
            const int32 B = (Roll * 37 + 13) % 100;
            const int32 C = (Roll * 71 + 29) % 100;
            const int32 Ordinary = FAVEncounters::SelectType(
                Floor, EAVEncounterKind::Standard, A, B, C);
            const int32 Hunter = FAVEncounters::SelectType(
                Floor, EAVEncounterKind::HunterPatrol, A, B, C);
            const int32 Swarm = FAVEncounters::SelectType(
                Floor, EAVEncounterKind::Swarm, A, B, C);
            const int32 Veteran = FAVEncounters::SelectType(
                Floor, EAVEncounterKind::EliteGuard, A, B, C);

            const int32 Options[3] = {
                FAVContent::EnemyRoll(Floor, A),
                FAVContent::EnemyRoll(Floor, B),
                FAVContent::EnemyRoll(Floor, C)
            };
            assert(Ordinary == Options[0]);
            for (const int32 Type : {Ordinary, Hunter, Swarm, Veteran})
            {
                assert(Type >= 0 && Type < FAVContent::EnemyCount);
                assert(!FAVContent::Enemy(Type, Floor).Boss);
                ++TotalSelections;
            }
            assert(FAVContent::Enemy(Veteran, Floor).Health >=
                   FAVContent::Enemy(Options[0], Floor).Health);
            assert(FAVContent::Enemy(Veteran, Floor).Health >=
                   FAVContent::Enemy(Options[1], Floor).Health);
            assert(FAVContent::Enemy(Veteran, Floor).Health >=
                   FAVContent::Enemy(Options[2], Floor).Health);
            StandardRanged += FAVContent::Enemy(Ordinary, Floor).Ranged ? 1 : 0;
            PatrolRanged += FAVContent::Enemy(Hunter, Floor).Ranged ? 1 : 0;
            StandardMelee += !FAVContent::Enemy(Ordinary, Floor).Ranged ? 1 : 0;
            SwarmMelee += !FAVContent::Enemy(Swarm, Floor).Ranged ? 1 : 0;
        }
        // Counts are for five different room areas, hence 5x weighting.
        assert(Weights[0] == 45 * 5);
        assert(Weights[1] == 20 * 5);
        assert(Weights[2] == 22 * 5);
        assert(Weights[3] == 13 * 5);
        assert(PatrolRanged >= StandardRanged);
        assert(SwarmMelee >= StandardMelee);
    }

    // Regression: Arcanist ability IDs are the *node* IDs, not EffectIds.
    const FAVSkillSpec* Sphere = FAVSkills::Find(
        FAVSkills::ArcanistSphereSkillId);
    const FAVSkillSpec* Wave = FAVSkills::Find(
        FAVSkills::ArcanistRuneWaveSkillId);
    assert(Sphere && Sphere->RequiredHero == 2 &&
           Sphere->EffectId == 29 && Sphere->Id == 28);
    assert(Wave && Wave->RequiredHero == 2 &&
           Wave->EffectId == 30 && Wave->Id == 29);
    assert(Sphere->PrerequisiteId == 27);
    assert(Wave->PrerequisiteId == 28);

    assert(FAVEncounters::Plan(1, 100, -10, 1000).Kind ==
           EAVEncounterKind::Standard);
    assert(FAVEncounters::SelectType(1, EAVEncounterKind::Standard,
                                    -500, 1000, -1) ==
           FAVContent::EnemyRoll(1, 0));
    std::cout << "PASS: " << TotalPlans << " room plans, "
              << TotalSelections << " enemy selections across "
              << (sizeof(Floors) / sizeof(Floors[0]))
              << " depths; Arcanist spell mappings\n";
    return 0;
}
