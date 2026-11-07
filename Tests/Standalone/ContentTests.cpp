#include "AVContent.h"
#include <cassert>
#include <iostream>

int main()
{
    assert(FAVContent::FinalFloor == 6);
    assert(FAVContent::Biome(1) == 0 && FAVContent::Biome(2) == 0);
    assert(FAVContent::Biome(3) == 1 && FAVContent::Biome(4) == 1);
    assert(FAVContent::Biome(5) == 2 && FAVContent::Biome(6) == 2);
    assert(FAVContent::Boss(1) == -1 && FAVContent::Boss(2) == 8);
    assert(FAVContent::Boss(4) == 9 && FAVContent::Boss(6) == 3);
    for (int32 Floor = 1; Floor <= FAVContent::FinalFloor; ++Floor)
    {
        for (int32 Roll = 0; Roll < 100; ++Roll)
        {
            const int32 Type = FAVContent::EnemyRoll(Floor, Roll);
            assert(Type >= 0 && Type <= 10);
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
    assert(FAVContent::ShopPrice(0, 6) > FAVContent::ShopPrice(0, 1));
    std::cout << "PASS: 6 floors, 3 biomes, 3 bosses, archetypes and shop prices\n";
    return 0;
}
