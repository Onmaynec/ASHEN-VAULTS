#include "AVContent.h"

float FAVContent::HeroHealth(int32 Id) { return Id == 1 ? 82.f : Id == 2 ? 75.f : 115.f; }
float FAVContent::HeroStamina(int32 Id) { return Id == 1 ? 120.f : Id == 2 ? 135.f : 100.f; }
float FAVContent::HeroAttack(int32 Id) { return Id == 1 ? 22.f : Id == 2 ? 16.f : 18.f; }
float FAVContent::HeroSpeed(int32 Id) { return Id == 1 ? 5.7f : Id == 2 ? 4.6f : 4.8f; }

int32 FAVContent::EnemyLevel(int32 Floor, int32 Roll)
{
    const int32 Depth = Floor > 1 ? Floor - 1 : 0;
    const int32 Variation = Roll < 0 ? 0 : Roll > 99 ? 99 : Roll;
    return 1 + Depth * 2 + Variation / 34;
}
float FAVContent::LevelHealthMultiplier(int32 Level)
{
    return 1.f + 0.13f * (Level > 1 ? Level - 1 : 0);
}
float FAVContent::LevelDamageMultiplier(int32 Level)
{
    return 1.f + 0.085f * (Level > 1 ? Level - 1 : 0);
}

int32 FAVContent::Biome(int32 Floor)
{
    if (Floor <= 2) return 0; // Ashen Crypt
    if (Floor <= 4) return 1; // Spore Hollows
    return 2;                 // Ember Foundry
}

int32 FAVContent::Boss(int32 Floor)
{
    if (Floor == 2) return 8; // Bone Regent
    if (Floor == 4) return 9; // Forge Tyrant
    if (Floor == 6) return 3; // Vault Warden
    return -1;
}

int32 FAVContent::EnemyRoll(int32 Floor, int32 Percentile)
{
    const int32 P = Percentile < 0 ? 0 : (Percentile > 99 ? 99 : Percentile);
    switch (Biome(Floor))
    {
    case 0: return P < 17 ? 0 : P < 30 ? 15 : P < 43 ? 16 : P < 55 ? 17 : P < 65 ? 18 : P < 74 ? 19 : P < 82 ? 20 : P < 89 ? 1 : P < 95 ? 11 : 5;
    case 1: return P < 17 ? 5 : P < 31 ? 6 : P < 44 ? 21 : P < 57 ? 22 : P < 69 ? 23 : P < 79 ? 12 : P < 88 ? 2 : P < 94 ? 16 : 1;
    default: return P < 18 ? 7 : P < 33 ? 10 : P < 47 ? 24 : P < 59 ? 22 : P < 72 ? 13 : P < 83 ? 14 : P < 91 ? 23 : 1;
    }
}

FAVEnemySpec FAVContent::Enemy(int32 Type, int32 Floor)
{
    FAVEnemySpec S = { 30.f, 2.2f, 12.f, false, false };
    switch (Type)
    {
    case 0: S = { 29.f, 2.45f, 12.f, false, false }; break;
    case 1: S = { 46.f, 1.65f, 18.f, false, false }; break;
    case 2: S = { 22.f, 2.05f, 15.f, true, false }; break;
    case 3: S = { 520.f, 1.8f, 26.f, true, true }; break;
    case 4: S = { 62.f, 3.05f, 19.f, false, false }; break;
    case 5: S = { 25.f, 3.4f, 14.f, false, false }; break;
    case 6: S = { 31.f, 1.75f, 12.f, true, false }; break;
    case 7: S = { 80.f, 1.28f, 23.f, false, false }; break;
    case 8: S = { 240.f, 1.65f, 20.f, false, true }; break;
    case 9: S = { 360.f, 1.52f, 23.f, true, true }; break;
    case 10: S = { 21.f, 2.85f, 13.f, true, false }; break;
    case 11: S = { 34.f, 3.10f, 16.f, false, false }; break; // Ash Reaver
    case 12: S = { 40.f, 1.85f, 17.f, true, false }; break; // Spore Witch
    case 13: S = { 64.f, 2.25f, 21.f, false, false }; break; // Ember Knight
    case 14: S = { 27.f, 3.35f, 16.f, true, false }; break; // Cinder Specter
    case 15: S = { 19.f, 3.7f, 7.f, false, false }; break; // Rat
    case 16: S = { 38.f, 1.8f, 12.f, false, false }; break; // Zombie
    case 17: S = { 29.f, 2.4f, 13.f, false, false }; break; // Skeleton
    case 18: S = { 23.f, 2.8f, 14.f, true, false }; break; // Ghost
    case 19: S = { 32.f, 1.9f, 16.f, true, false }; break; // Witch
    case 20: S = { 50.f, 1.55f, 18.f, false, false }; break; // Crypt Guard
    case 21: S = { 35.f, 2.1f, 14.f, true, false }; break; // Druid
    case 22: S = { 24.f, 3.1f, 12.f, false, false }; break; // Wretch
    case 23: S = { 95.f, 1.15f, 24.f, false, false }; break; // Stone Golem
    case 24: S = { 120.f, 1.05f, 27.f, false, false }; break; // Iron Golem
    case 25: S = { 42.f, 2.7f, 17.f, true, false }; break; // Bandit Archer
    case 26: S = { 175.f, 2.1f, 23.f, false, false }; break; // Grave Butcher
    case 27: S = { 145.f, 2.5f, 20.f, true, false }; break; // Bone Archer
    case 28: S = { 190.f, 1.6f, 22.f, true, false }; break; // Rot Witch
    case 29: S = { 230.f, 1.3f, 25.f, false, false }; break; // Stone Sentinel
    case 30: S = { 165.f, 2.6f, 22.f, false, false }; break; // Blood Reaver
    case 31: S = { 205.f, 1.9f, 24.f, true, false }; break; // Cinder Shaman
    case 32: S = { 250.f, 1.5f, 28.f, true, false }; break; // Iron Tyrant
    case 33: S = { 45.f, 2.65f, 17.f, false, false }; break; // Bandit Raider
    default: break;
    }
    if (!S.Boss)
        S.Health *= 1.f + 0.22f * (Floor > 1 ? Floor - 1 : 0);
    return S;
}

int32 FAVContent::ShopPrice(int32 Slot, int32 Floor)
{
    const int32 Tier = Floor > 1 ? Floor - 1 : 0;
    switch (Slot)
    {
    case 0: return 16 + 4 * Tier; // 45 HP
    case 1: return 30 + 6 * Tier; // +5 damage
    case 2: return 22 + 4 * Tier; // 1 potion
    default: return 0;
    }
}
