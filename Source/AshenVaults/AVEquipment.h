#pragma once
#include "CoreMinimal.h"

// v0.9.0. Stable item ids: persist these, never depend on array indices of pickups.
enum EAVGearSlot : int32 { AVWeapon=0, AVHead=1, AVChest=2, AVHands=3, AVLegs=4, AVBoots=5, AVAccessory=6, AVConsumable=7 };
enum EAVGearRarity : int32 { AVCommon=0, AVUncommon=1, AVRare=2, AVEpic=3, AVLegendary=4 };
enum EAVGearStat : int32 {
    AVNoStat=0, AVAttackFlat=1, AVArmorPercent=2, AVHealthFlat=3, AVStaminaFlat=4,
    AVMoveSpeedPercent=5, AVAttackSpeedPercent=6, AVCriticalPercent=7,
    AVMeleeDamagePercent=8, AVRangedDamagePercent=9, AVAbilityDamagePercent=10,
    AVStaminaRegenPercent=11, AVDashCooldownPercent=12, AVProjectileSpeedPercent=13,
    AVKnockbackResistPercent=14, AVStatusResistPercent=15, AVBossDamagePercent=16,
    AVCooldownPercent=17, AVMeleeResistPercent=18, AVPoisonResistPercent=19,
    AVBurnResistPercent=20
};
// RequiredHero -1 means any hero. SetId 0 means no set. PerkId 0 means none.
// Pattern 0 is the classic attack; added patterns are gameplay variants, not art.
struct FAVEquipmentSpec {
    int32 Id;
    const char* CodeName;
    int32 Slot;
    int32 RequiredHero;
    int32 Rarity;
    int32 SetId;
    int32 StatA;
    float ValueA;
    int32 StatB;
    float ValueB;
    int32 PerkId;
    int32 Pattern=0;
    const char* DisplayName=nullptr;
};
class FAVEquipment
{
public:
    static constexpr int32 LegacyCount=42; // save-game IDs 0..41 stay unchanged.
    static constexpr int32 WeaponCount=40;
    static constexpr int32 ArmorCount=45;
    static constexpr int32 AccessoryCount=12;
    static constexpr int32 ConsumableCount=10;
    static constexpr int32 GearCount=WeaponCount+ArmorCount+AccessoryCount;
    static constexpr int32 Count=GearCount+ConsumableCount;
    static constexpr int32 InstanceStride=1024; // base ID in low 10 bits
    // Affix roll 0 means none; 1..4 = bonus vs one biome, 5 = elite, 6 = boss.
    static int32 BaseId(int32 InstanceId);
    static int32 AffixId(int32 InstanceId);
    static int32 EncodeInstance(int32 ItemId,int32 AffixId);
    static int32 RollAffix(int32 ItemId,int32 Percentile);
    static int32 BonusAgainst(int32 InstanceId,int32 Biome,bool bElite,bool bBoss);
    static const char* AffixName(int32 AffixId);
    static bool IsConsumable(int32 InstanceId);

    static constexpr int32 InvalidId=-1;
    static const FAVEquipmentSpec* Find(int32 ItemId);
    static const char* StatName(int32 StatId);
    static const char* PerkName(int32 PerkId);
    static const char* PatternName(int32 Pattern);
    static int32 RarityRoll(int32 Percentile);
    static bool CanEquip(int32 ItemId, int32 HeroClass);
    static float SetBonus(int32 SetId, int32 EquippedCount); // percent 3-piece bonus
};
