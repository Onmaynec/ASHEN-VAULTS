#pragma once

#include "CoreMinimal.h"

/** Pure-data combat balancing for v0.7.1, standalone-testable without UE4.
 * Enemy IDs 0..14: original creatures and three story bosses.
 * 15..24: rats, undead, spellcasters, guards and stone/iron golems.
 * 25 and 33: bandit archer and raider; 26..32: seven minibosses.
 * Levels scale by dungeon depth and randomized spawn tier.
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
    static constexpr int32 FloorsPerBiome = 5;
    static constexpr int32 RelicCount = 6;
    static constexpr int32 BiomeCount = 4;
    static constexpr int32 HeroCount = 4;
    static constexpr int32 EnemyCount = 57;
    static constexpr int32 MiniBossCount = 11;
    static int32 EnemyLevel(int32 Floor, int32 Roll);
    static float LevelHealthMultiplier(int32 Level);
    static float LevelDamageMultiplier(int32 Level);
    // One health scaling pass. Level already increases with floor.
    static float ScaledEnemyHealth(int32 Type, int32 Floor, int32 Level);
    // Bound camp occupancy independently of the unbounded floor number.
    static int32 BanditGroupSize(int32 Floor);
    static constexpr int32 PickupCount = 11;
    // 0 Warden, 1 Duelist, 2 Arcanist, 3 Marksman. Pure data.
    static float HeroHealth(int32 HeroId);
    static float HeroStamina(int32 HeroId);
    static float HeroAttack(int32 HeroId);
    static float HeroSpeed(int32 HeroId);
    static float BasicAttackInterval(int32 HeroId, float AttackSpeedPercent);
    static int32 Biome(int32 Floor);
    static int32 Boss(int32 Floor);
    static int32 EnemyRoll(int32 Floor, int32 Percentile);
    static FAVEnemySpec Enemy(int32 Type, int32 Floor);
    static const char* BastionVariantName(int32 Type, int32 Subclass);
    static int32 ShopPrice(int32 Slot, int32 Floor);
};
