#pragma once
#include "CoreMinimal.h"

// Pure gameplay parameters for the ten new weapon attack modes.
// Visual clips are intentionally deferred; these values drive real combat.
struct FAVMeleeStyle
{
    float Range=1.92f;
    float AimThreshold=0.10f;
    float DamageScale=1.f;
    float KnockbackBonus=0.f;
    int32 Status=0; // 0 none, 1 poison, 2 burn
    float HealOnHit=0.f;
};
struct FAVProjectileStyle
{
    float SpeedScale=1.f;
    float DamageScale=1.f;
    int32 PierceCount=1;
    int32 ExtraShots=0;
    int32 Status=0;
};
class FAVItemCombat
{
public:
    static FAVMeleeStyle Melee(int32 Pattern,bool bSpin);
    static FAVProjectileStyle Projectile(int32 Pattern);
    static float IncomingMeleeMultiplier(float ResistPercent,bool bGuardBuff);
};
