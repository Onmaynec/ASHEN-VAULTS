#pragma once
#include "CoreMinimal.h"

// v0.9.0. Stable item ids: persist these, never depend on array indices of pickups.
enum EAVGearSlot : int32 { AVWeapon=0, AVHead=1, AVChest=2, AVHands=3, AVLegs=4, AVBoots=5, AVAccessory=6 };
enum EAVGearRarity : int32 { AVCommon=0, AVUncommon=1, AVRare=2, AVEpic=3, AVLegendary=4 };
enum EAVGearStat : int32 {
    AVNoStat=0, AVAttackFlat=1, AVArmorPercent=2, AVHealthFlat=3, AVStaminaFlat=4,
    AVMoveSpeedPercent=5, AVAttackSpeedPercent=6, AVCriticalPercent=7,
    AVMeleeDamagePercent=8, AVRangedDamagePercent=9, AVAbilityDamagePercent=10,
    AVStaminaRegenPercent=11, AVDashCooldownPercent=12, AVProjectileSpeedPercent=13,
    AVKnockbackResistPercent=14, AVStatusResistPercent=15, AVBossDamagePercent=16,
    AVCooldownPercent=17
};
// RequiredHero -1 means any hero. SetId 0 means no set. PerkId 0 means none.
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
};
class FAVEquipment
{
public:
    static constexpr int32 WeaponCount=10;
    static constexpr int32 ArmorCount=25;
    static constexpr int32 AccessoryCount=7;
    static constexpr int32 Count=WeaponCount+ArmorCount+AccessoryCount;
    static constexpr int32 InvalidId=-1;
    static const FAVEquipmentSpec* Find(int32 ItemId);
    static const char* StatName(int32 StatId);
    static const char* PerkName(int32 PerkId);
    static int32 RarityRoll(int32 Percentile);
    static bool CanEquip(int32 ItemId, int32 HeroClass);
    static float SetBonus(int32 SetId, int32 EquippedCount); // percent 3-piece bonus
};
