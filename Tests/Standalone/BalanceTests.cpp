#include "AVContent.h"
#include "AVEquipment.h"
#include "AVSkills.h"
#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>

int main()
{
    // A level already contains dungeon depth; no second depth multiplier.
    const int Floors[] = {1, 5, 10, 15, 16, 20, 21, 40, 100, 250};
    for (int32 Type = 0; Type < FAVContent::EnemyCount; ++Type)
    {
        const FAVEnemySpec Base = FAVContent::Enemy(Type, 1);
        assert(Base.Health > 0.f && Base.Damage > 0.f && Base.Speed > 0.f);
        for (int Floor : Floors)
        {
            const FAVEnemySpec Current = FAVContent::Enemy(Type, Floor);
            assert(Current.Health == Base.Health);
            assert(Current.Damage == Base.Damage);
            for (int Roll : {0, 33, 34, 68, 99})
            {
                const int32 Level = FAVContent::EnemyLevel(Floor, Roll);
                const float HP = FAVContent::ScaledEnemyHealth(Type, Floor, Level);
                const float Damage = Current.Damage * FAVContent::LevelDamageMultiplier(Level);
                assert(std::isfinite(HP) && std::isfinite(Damage));
                assert(HP >= Current.Health && Damage >= Current.Damage);
                assert(std::fabs(HP / Current.Health -
                    FAVContent::LevelHealthMultiplier(Level)) < 0.0005f);
            }
        }
    }
    for (int Floor = 1; Floor <= 250; ++Floor)
    {
        const int32 Camp = FAVContent::BanditGroupSize(Floor);
        assert(Camp >= 3 && Camp <= 8);
        if (Floor > 1)
            assert(Camp >= FAVContent::BanditGroupSize(Floor-1));
    }
    assert(FAVContent::BanditGroupSize(1) == 3);
    assert(FAVContent::BanditGroupSize(5) == 4);
    assert(FAVContent::BanditGroupSize(16) == 6);
    assert(FAVContent::BanditGroupSize(21) == 8);
    assert(FAVContent::BanditGroupSize(100) == 8);

    // Character starting strength remains unchanged: this patch rebalances
    // enemy scaling without quietly rewriting class identities.
    const float ExpectedHP[4] = {115.f, 82.f, 75.f, 90.f};
    const float ExpectedStamina[4] = {100.f, 120.f, 135.f, 115.f};
    const float ExpectedAttack[4] = {18.f, 22.f, 16.f, 19.f};
    const float ExpectedSpeed[4] = {4.8f, 5.7f, 4.6f, 5.1f};
    for (int32 Hero = 0; Hero < FAVContent::HeroCount; ++Hero)
    {
        assert(FAVContent::HeroHealth(Hero) == ExpectedHP[Hero]);
        assert(FAVContent::HeroStamina(Hero) == ExpectedStamina[Hero]);
        assert(FAVContent::HeroAttack(Hero) == ExpectedAttack[Hero]);
        assert(FAVContent::HeroSpeed(Hero) == ExpectedSpeed[Hero]);
    }

    // Three general branches and a class-only branch. The final pairs
    // cannot be equipped at the same time.
    for (int32 Hero = 0; Hero < 4; ++Hero)
    {
        for (int32 Id = 0; Id < FAVSkills::Count; ++Id)
        {
            const FAVSkillSpec* Node = FAVSkills::Find(Id);
            assert(Node);
            if (Node->RequiredHero >= 0 && Node->RequiredHero != Hero)
            {
                uint8 Ranks[FAVSkills::Count] = {};
                assert(!FAVSkills::CanLearn(Id, Hero, 30, Ranks,
                                          FAVSkills::Count, 29));
            }
        }
    }
    for (int32 Id = 0; Id < FAVEquipment::Count; ++Id)
    {
        const FAVEquipmentSpec* Item = FAVEquipment::Find(Id);
        assert(Item);
        assert(Item->Slot >= AVWeapon && Item->Slot <= AVAccessory);
        assert(Item->ValueA >= -100.f && Item->ValueA <= 100.f);
        assert(Item->ValueB >= -100.f && Item->ValueB <= 100.f);
        for (int32 Hero = 0; Hero < 4; ++Hero)
            assert(FAVEquipment::CanEquip(Id, Hero) ==
                (Item->RequiredHero == -1 || Item->RequiredHero == Hero));
    }

    std::cout << "FLOOR  LEVEL  HP_MULT  DMG_MULT  BANDITS\n";
    for (int Floor : Floors)
    {
        const int Level = FAVContent::EnemyLevel(Floor, 50);
        std::cout << std::setw(5) << Floor << "  " << std::setw(5) << Level
                  << "  " << std::setw(7) << std::fixed << std::setprecision(2)
                  << FAVContent::LevelHealthMultiplier(Level) << "  "
                  << std::setw(8) << FAVContent::LevelDamageMultiplier(Level)
                  << "  " << std::setw(7) << FAVContent::BanditGroupSize(Floor)
                  << "\n";
    }
    std::cout << "PASS: 57 enemy stats over 10 depths, 250 camp sizes, "
                 "four heroes, gear and skill restrictions\n";
    return 0;
}
