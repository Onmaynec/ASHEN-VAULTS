#pragma once
#include "CoreMinimal.h"

// Combat mutations are gameplay rules, not new monster species. Old saves
// default to None; only ordinary enemies can roll an affix.
enum class EAVEnemyAffix : uint8
{
    None=0, Frenzied=1, Bulwark=2, GlassCannon=3,
    Blooddrinker=4, Venom=5, Ember=6, Commander=7,
    FieldMedic=8, Stalker=9, Sharpshooter=10, Titanic=11, RuneWard=12
};

struct FAVAffixTraits
{
    float MaxHealthScale=1.f;
    float MovementScale=1.f;
    float OutgoingScale=1.f;
    float IncomingScale=1.f;
    float CooldownScale=1.f;
    float BoltSpeedScale=1.f;
    float MemorySeconds=3.5f;
    float AllySpeedAura=1.f;
    float AllyHealingPerSecond=0.f;
    float LeechFraction=0.f;
    int32 InflictedStatus=0; // 1 poison, 2 burn
    bool bConsumesRuneWard=false;
};

class FAVAffixes
{
public:
    static constexpr int32 Count=13; // None + twelve modifications
    static int32 Roll(uint32 Seed,int32 Floor,int32 Type,
                      int32 EnemyId,bool bElite,bool bBoss);
    static FAVAffixTraits Traits(int32 Affix,float HealthFraction,
                                 float AlertTime);
    static float TakeDamage(int32 Affix,float Incoming,bool& bWardConsumed);
    static int32 BonusExperience(int32 Floor,int32 Affix);
    static int32 BonusGold(int32 Floor,int32 Affix);
    static const char* Name(int32 Affix);
};
