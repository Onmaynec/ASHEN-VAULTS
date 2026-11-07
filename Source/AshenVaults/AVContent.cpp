#include "AVContent.h"

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
    case 0: return P < 38 ? 0 : P < 66 ? 1 : P < 84 ? 2 : 5;
    case 1: return P < 30 ? 5 : P < 55 ? 6 : P < 77 ? 2 : 1;
    default: return P < 36 ? 7 : P < 61 ? 10 : P < 84 ? 1 : 6;
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
