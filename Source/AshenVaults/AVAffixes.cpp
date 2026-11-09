#include "AVAffixes.h"

namespace
{
    uint32 Scramble(uint32 V)
    {
        V ^= V>>16;V*=0x7feb352du;
        V ^= V>>15;V*=0x846ca68bu;
        return V^(V>>16);
    }
}

int32 FAVAffixes::Roll(uint32 Seed,int32 Floor,int32 Type,
                       int32 EnemyId,bool bElite,bool bBoss)
{
    if(bBoss)return 0;
    const uint32 V=Scramble(Seed ^
        static_cast<uint32>(Floor)*0x9e3779b9u ^
        static_cast<uint32>(Type+1)*0x85ebca6bu ^
        static_cast<uint32>(EnemyId+1)*0xc2b2ae35u);
    // An uncommon complication, not a permanent buff to every enemy:
    // 14% on the first floor, slowly reaching 38%, elite +10 percentage points.
    const int32 Chance=FMath::Clamp(14+FMath::Max(0,Floor-1)/5+
                                   (bElite?10:0),14,48);
    if(static_cast<int32>(V%100u)>=Chance)return 0;
    return 1+static_cast<int32>((V>>9)%12u);
}

FAVAffixTraits FAVAffixes::Traits(int32 Affix,float HealthFraction,
                                  float AlertTime)
{
    FAVAffixTraits T;
    switch(static_cast<EAVEnemyAffix>(Affix))
    {
    case EAVEnemyAffix::Frenzied:
        if(HealthFraction<0.45f)
        {T.MovementScale=1.36f;T.CooldownScale=0.76f;}
        break;
    case EAVEnemyAffix::Bulwark:
        T.IncomingScale=0.78f;break;
    case EAVEnemyAffix::GlassCannon:
        T.MaxHealthScale=0.70f;T.OutgoingScale=1.32f;break;
    case EAVEnemyAffix::Blooddrinker:
        T.LeechFraction=0.20f;break;
    case EAVEnemyAffix::Venom:
        T.InflictedStatus=1;break;
    case EAVEnemyAffix::Ember:
        T.InflictedStatus=2;break;
    case EAVEnemyAffix::Commander:
        T.AllySpeedAura=1.18f;break;
    case EAVEnemyAffix::FieldMedic:
        T.AllyHealingPerSecond=0.020f;break;
    case EAVEnemyAffix::Stalker:
        T.MemorySeconds=7.5f;T.MovementScale=AlertTime>3.f?1.10f:1.f;
        break;
    case EAVEnemyAffix::Sharpshooter:
        T.BoltSpeedScale=1.30f;break;
    case EAVEnemyAffix::Titanic:
        T.MaxHealthScale=1.42f;T.MovementScale=0.76f;break;
    case EAVEnemyAffix::RuneWard:
        T.bConsumesRuneWard=true;break;
    default: break;
    }
    return T;
}

float FAVAffixes::TakeDamage(int32 Affix,float Incoming,bool& bWardConsumed)
{
    const FAVAffixTraits T=Traits(Affix,1.f,0.f);
    float Damage=FMath::Max(0.f,Incoming)*T.IncomingScale;
    if(T.bConsumesRuneWard&&!bWardConsumed)
    {
        bWardConsumed=true;
        Damage*=0.5f;
    }
    return Damage;
}

int32 FAVAffixes::BonusExperience(int32 Floor,int32 Affix)
{
    return Affix<=0||Affix>=Count?0:2+FMath::Min(200,FMath::Max(0,Floor-1)/4);
}

int32 FAVAffixes::BonusGold(int32 Floor,int32 Affix)
{
    return Affix<=0||Affix>=Count?0:3+FMath::Min(150,FMath::Max(0,Floor-1)/6);
}

const char* FAVAffixes::Name(int32 Affix)
{
    switch(static_cast<EAVEnemyAffix>(Affix))
    {
    case EAVEnemyAffix::Frenzied:return "FRENZIED";
    case EAVEnemyAffix::Bulwark:return "BULWARK";
    case EAVEnemyAffix::GlassCannon:return "GLASS CANNON";
    case EAVEnemyAffix::Blooddrinker:return "BLOODDRINKER";
    case EAVEnemyAffix::Venom:return "VENOM";
    case EAVEnemyAffix::Ember:return "EMBER";
    case EAVEnemyAffix::Commander:return "COMMANDER";
    case EAVEnemyAffix::FieldMedic:return "FIELD MEDIC";
    case EAVEnemyAffix::Stalker:return "STALKER";
    case EAVEnemyAffix::Sharpshooter:return "SHARPSHOOTER";
    case EAVEnemyAffix::Titanic:return "TITANIC";
    case EAVEnemyAffix::RuneWard:return "RUNE WARD";
    default:return "";
    }
}
