#include "AVContent.h"
#include <cassert>
#include <iostream>

int main()
{
    assert(FAVContent::FloorsPerBiome == 5);
    assert(FAVContent::Biome(1) == 0 && FAVContent::Biome(5) == 0);
    assert(FAVContent::Biome(6) == 1 && FAVContent::Biome(10) == 1);
    assert(FAVContent::Biome(11) == 2 && FAVContent::Biome(15) == 2);
    assert(FAVContent::Biome(16) == 3 && FAVContent::Biome(20) == 3);
    assert(FAVContent::Biome(21) == 0 && FAVContent::Biome(31) == 2);
    assert(FAVContent::Biome(36) == 3 && FAVContent::Biome(41) == 0);
    assert(FAVContent::Boss(1) == -1 && FAVContent::Boss(5) == 8);
    assert(FAVContent::Boss(10) == 9 && FAVContent::Boss(15) == 3);
    assert(FAVContent::Boss(20) == 56 && FAVContent::Boss(21) == -1);
    assert(FAVContent::Boss(25) == 8 && FAVContent::Boss(40) == 56);
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
    assert(FAVContent::MiniBossCount == 11);
    assert(FAVContent::PickupCount == 11);
    assert(FAVContent::HeroCount == 4 && FAVContent::HeroHealth(3) == 90.f);
    assert(FAVContent::HeroAttack(3) == 19.f && FAVContent::HeroSpeed(3) == 5.1f);
    for (int32 Type = 34; Type <= 51; ++Type) {
        const FAVEnemySpec S = FAVContent::Enemy(Type, 16);
        assert(S.Health > 0.f && S.Damage > 0.f && !S.Boss);
    }
    for (int32 Type = 52; Type <= 55; ++Type) {
        assert(FAVContent::Enemy(Type, 18).Health > 100.f);
    }
    assert(FAVContent::Enemy(56, 20).Boss);
    for (int32 P = 0; P < 100; ++P) {
        const int32 Type = FAVContent::EnemyRoll(16, P);
        assert(Type >= 34 && Type <= 51);
    }
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
    std::cout << "PASS: 4 cyclic biomes, 4 heroes, 57 enemy IDs and shops\n";
    return 0;
}
