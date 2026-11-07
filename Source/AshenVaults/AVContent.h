#pragma once

#include "CoreMinimal.h"

/** Data-only combat balancing for the 0.4 content expansion.
 * Keep this independent from the HUD and UObjects so balance can be tested
 * with a small stand-alone C++ harness as well as inside UE4.27.
 *
 * IDs: 0 Ashling; 1 Sentinel; 2 Seer; 3 Vault Warden (final boss);
 * 4 Mimic; 5 Grave Hound; 6 Sporecaller; 7 Cinder Golem;
 * 8 Bone Regent (boss); 9 Forge Tyrant (boss); 10 Void Wisp.
 */
struct FAVEnemySpec
{
    float Health;
    float Speed;
    float Damage;
    bool Ranged;
    bool Boss;
};

class FAVContent
{
public:
    static constexpr int32 FinalFloor = 6;
    static constexpr int32 RelicCount = 6;
    static constexpr int32 BiomeCount = 3;
    static constexpr int32 HeroCount = 3;
    static constexpr int32 EnemyCount = 15;
    static constexpr int32 PickupCount = 8;
    // 0 Warden, 1 Duelist, 2 Arcanist. Pure data: safe in standalone tests.
    static float HeroHealth(int32 HeroId);
    static float HeroStamina(int32 HeroId);
    static float HeroAttack(int32 HeroId);
    static float HeroSpeed(int32 HeroId);
    static int32 Biome(int32 Floor);
    static int32 Boss(int32 Floor);
    static int32 EnemyRoll(int32 Floor, int32 Percentile);
    static FAVEnemySpec Enemy(int32 Type, int32 Floor);
    static int32 ShopPrice(int32 Slot, int32 Floor);
};
