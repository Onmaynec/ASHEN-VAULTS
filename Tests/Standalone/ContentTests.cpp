#include "AVContent.h"
#include <cassert>
#include <iostream>

int main()
{
    assert(FAVContent::FloorsPerBiome == 5);
    assert(FAVContent::Biome(1) == 0 && FAVContent::Biome(5) == 0);
    assert(FAVContent::Biome(6) == 1 && FAVContent::Biome(10) == 1);
    assert(FAVContent::Biome(11) == 2 && FAVContent::Biome(15) == 2);
    assert(FAVContent::Biome(16) == 0 && FAVContent::Biome(31) == 0);
    assert(FAVContent::Boss(1) == -1 && FAVContent::Boss(5) == 8);
    assert(FAVContent::Boss(10) == 9 && FAVContent::Boss(15) == 3);
    assert(FAVContent::Boss(20) == 8 && FAVContent::Boss(21) == -1);
    for (int32 Floor = 1; Floor <= 35; ++Floor)
    {
        for (int32 Roll = 0; Roll < 100; ++Roll)
        {
            const int32 Type = FAVContent::EnemyRoll(Floor, Roll);
            assert(Type >= 0 && Type < FAVContent::EnemyCount);
            assert(Type != 3 && Type != 4 && Type != 8 && Type != 9);
            const FAVEnemySpec Enemy = FAVContent::Enemy(Type, Floor);
            assert(Enemy.Health > 0.f && Enemy.Speed > 0.f && Enemy.Damage > 0.f);
            assert(!Enemy.Boss);
        }
        for (int32 Slot = 0; Slot < 3; ++Slot)
            assert(FAVContent::ShopPrice(Slot, Floor) > 0);
    }
    assert(FAVContent::Enemy(3, 6).Boss);
    assert(FAVContent::Enemy(8, 2).Boss);
    assert(FAVContent::Enemy(9, 4).Boss);
    assert(FAVContent::Enemy(6, 3).Ranged);
    assert(!FAVContent::Enemy(7, 5).Ranged);
    assert(FAVContent::Enemy(7, 6).Health > FAVContent::Enemy(7, 5).Health);
    assert(FAVContent::EnemyLevel(6, 99) > FAVContent::EnemyLevel(1, 0));
    assert(FAVContent::LevelHealthMultiplier(10) > FAVContent::LevelHealthMultiplier(1));
    assert(FAVContent::LevelDamageMultiplier(10) > FAVContent::LevelDamageMultiplier(1));
    for (int32 Type = 15; Type < FAVContent::EnemyCount; ++Type)
        assert(FAVContent::Enemy(Type, 3).Health > 0.f);
    assert(FAVContent::MiniBossCount == 7);
    assert(FAVContent::PickupCount == 11);
    for (int32 Floor = 1; Floor <= 6; ++Floor)
    {
        assert(FAVContent::EnemyLevel(Floor, 0) == 1 + (Floor - 1) * 2);
        assert(FAVContent::EnemyLevel(Floor, 99) == 3 + (Floor - 1) * 2);
        for (int32 Type = 26; Type <= 32; ++Type)
        {
            const FAVEnemySpec Mini = FAVContent::Enemy(Type, Floor);
            assert(Mini.Health > 100.f && Mini.Damage > 0.f);
        }
    }
    assert(FAVContent::Enemy(15, 1).Speed > FAVContent::Enemy(16, 1).Speed);
    assert(FAVContent::Enemy(24, 5).Health > FAVContent::Enemy(23, 5).Health);
    assert(FAVContent::Enemy(25, 3).Ranged);
    assert(!FAVContent::Enemy(33, 3).Ranged);
    assert(FAVContent::ShopPrice(0, 6) > FAVContent::ShopPrice(0, 1));
    std::cout << "PASS: 6 floors, 3 biomes, 3 bosses, archetypes and shop prices\n";
    return 0;
}
