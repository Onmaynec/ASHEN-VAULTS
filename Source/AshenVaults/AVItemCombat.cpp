#include "AVItemCombat.h"

FAVMeleeStyle FAVItemCombat::Melee(int32 Pattern,bool bSpin)
{
    FAVMeleeStyle S;
    if(bSpin)S.Range=2.25f;
    switch(Pattern)
    {
    case 1: // cleaver: broad directional sweep
        if(!bSpin){S.Range=2.15f;S.AimThreshold=-0.25f;}
        break;
    case 2: // pike: long but narrow lunge
        if(!bSpin){S.Range=2.65f;S.AimThreshold=0.50f;}
        break;
    case 3: // maul: stronger swing and stagger
        if(!bSpin)S.DamageScale=1.28f;
        S.KnockbackBonus=3.f;
        break;
    case 4:S.Status=1;break; // venom edge
    case 5:S.Status=2;break; // ember edge
    case 10:S.HealOnHit=2.f;break; // lifesteal
    default:break;
    }
    return S;
}
FAVProjectileStyle FAVItemCombat::Projectile(int32 Pattern)
{
    FAVProjectileStyle S;
    switch(Pattern)
    {
    case 4:S.Status=1;break;
    case 5:S.Status=2;break;
    case 6:S.ExtraShots=1;break;
    case 7:S.PierceCount=2;break;
    case 8:S.SpeedScale=1.25f;break;
    case 9:S.SpeedScale=0.82f;S.DamageScale=1.35f;break;
    default:break;
    }
    return S;
}
float FAVItemCombat::IncomingMeleeMultiplier(float ResistPercent,bool bGuardBuff)
{
    return FMath::Clamp(1.f-ResistPercent*0.01f-
        (bGuardBuff?0.12f:0.f),0.45f,1.f);
}
