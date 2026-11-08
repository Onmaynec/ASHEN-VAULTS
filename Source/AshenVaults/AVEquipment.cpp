#include "AVEquipment.h"

// Item ID 0..9 weapon, 10..34 armor, 35..41 accessory.
// Stat values in percent are percentage points, not fractions.
namespace {
const FAVEquipmentSpec Items[FAVEquipment::Count] = {
    // Weapons: Warden 0, Duelist 1, Arcanist 2, Marksman 3.
    {0,"black_watch_sword",AVWeapon,0,AVUncommon,0,AVAttackFlat,12,AVMeleeDamagePercent,8,0},
    {1,"bastion_war_maul",AVWeapon,0,AVRare,0,AVAttackFlat,19,AVAttackSpeedPercent,-15,1},
    {2,"executioner_cleaver",AVWeapon,0,AVLegendary,0,AVAttackFlat,27,AVNoStat,0,2},
    {3,"raider_sabre",AVWeapon,1,AVUncommon,0,AVAttackFlat,11,AVAttackSpeedPercent,12,0},
    {4,"renegade_twinblades",AVWeapon,1,AVEpic,0,AVAttackFlat,17,AVCriticalPercent,7,0},
    {5,"blooddrinker_dagger",AVWeapon,1,AVLegendary,0,AVAttackFlat,20,AVNoStat,0,3},
    {6,"battlemage_wand",AVWeapon,2,AVRare,0,AVAttackFlat,15,AVCooldownPercent,-8,0},
    {7,"shattered_seal_staff",AVWeapon,2,AVEpic,0,AVAttackFlat,21,AVAbilityDamagePercent,15,0},
    {8,"watchman_longbow",AVWeapon,3,AVRare,0,AVAttackFlat,16,AVAttackSpeedPercent,12,4},
    {9,"commandant_crossbow",AVWeapon,3,AVLegendary,0,AVAttackFlat,29,AVAttackSpeedPercent,-20,5},
    // Set 1: garrison (Warden).
    {10,"sergeant_helm",AVHead,0,AVUncommon,1,AVArmorPercent,3,AVNoStat,0,0},
    {11,"bastion_cuirass",AVChest,0,AVEpic,1,AVArmorPercent,7,AVHealthFlat,20,0},
    {12,"watch_gauntlets",AVHands,0,AVRare,1,AVArmorPercent,2,AVAbilityDamagePercent,8,0},
    {13,"ward_legplates",AVLegs,0,AVRare,1,AVArmorPercent,4,AVStaminaFlat,10,0},
    {14,"steel_watch_boots",AVBoots,0,AVEpic,1,AVArmorPercent,2,AVKnockbackResistPercent,15,0},
    // Set 2: infiltrator (Duelist).
    {15,"raider_hood",AVHead,1,AVUncommon,2,AVArmorPercent,1,AVCriticalPercent,3,0},
    {16,"renegade_jacket",AVChest,1,AVRare,2,AVArmorPercent,3,AVMoveSpeedPercent,5,0},
    {17,"killer_gloves",AVHands,1,AVEpic,2,AVArmorPercent,1,AVAttackSpeedPercent,9,0},
    {18,"scout_leggings",AVLegs,1,AVRare,2,AVArmorPercent,2,AVStaminaFlat,10,0},
    {19,"silent_step_boots",AVBoots,1,AVLegendary,2,AVArmorPercent,1,AVDashCooldownPercent,-12,0},
    // Set 3: battlemage (Arcanist).
    {20,"caster_mask",AVHead,2,AVUncommon,3,AVArmorPercent,1,AVAbilityDamagePercent,6,0},
    {21,"shattered_seal_robe",AVChest,2,AVEpic,3,AVArmorPercent,2,AVStaminaFlat,20,0},
    {22,"rune_bracers",AVHands,2,AVRare,3,AVArmorPercent,1,AVCooldownPercent,-7,0},
    {23,"black_magister_belt",AVLegs,2,AVRare,3,AVArmorPercent,2,AVStaminaRegenPercent,8,0},
    {24,"sorcerer_boots",AVBoots,2,AVLegendary,3,AVArmorPercent,1,AVCooldownPercent,-10,0},
    // Set 4: marksman.
    {25,"watchman_hood",AVHead,3,AVUncommon,4,AVArmorPercent,1,AVProjectileSpeedPercent,5,0},
    {26,"ranger_jacket",AVChest,3,AVRare,4,AVArmorPercent,4,AVRangedDamagePercent,8,0},
    {27,"eagle_eye_gloves",AVHands,3,AVEpic,4,AVArmorPercent,1,AVCriticalPercent,5,0},
    {28,"hunter_leggings",AVLegs,3,AVRare,4,AVArmorPercent,2,AVStaminaFlat,10,0},
    {29,"hunting_boots",AVBoots,3,AVLegendary,4,AVArmorPercent,1,AVMoveSpeedPercent,6,0},
    // Set 5: relic armor (any hero).
    {30,"iron_mask",AVHead,-1,AVRare,5,AVArmorPercent,2,AVHealthFlat,10,0},
    {31,"last_watch_mail",AVChest,-1,AVLegendary,5,AVArmorPercent,5,AVHealthFlat,15,0},
    {32,"prisoner_bracers",AVHands,-1,AVUncommon,5,AVArmorPercent,2,AVMeleeDamagePercent,5,0},
    {33,"jailer_legplates",AVLegs,-1,AVEpic,5,AVArmorPercent,3,AVStaminaFlat,12,0},
    {34,"commandant_boots",AVBoots,-1,AVLegendary,5,AVArmorPercent,2,AVBossDamagePercent,8,0},
    // Accessories. Perks 6..8 must be processed in gameplay, never as permanent stat bonuses.
    {35,"iron_will_amulet",AVAccessory,-1,AVRare,0,AVStatusResistPercent,20,AVNoStat,0,0},
    {36,"butcher_ring",AVAccessory,0,AVEpic,0,AVNoStat,0,AVNoStat,0,6},
    {37,"renegade_bracelet",AVAccessory,1,AVRare,0,AVDashCooldownPercent,-10,AVNoStat,0,0},
    {38,"black_orders_tome",AVAccessory,2,AVLegendary,0,AVNoStat,0,AVNoStat,0,7},
    {39,"marksman_medallion",AVAccessory,3,AVRare,0,AVAttackSpeedPercent,8,AVProjectileSpeedPercent,5,0},
    {40,"last_strike_signet",AVAccessory,-1,AVEpic,0,AVNoStat,0,AVNoStat,0,8},
    {41,"commandant_seal",AVAccessory,-1,AVLegendary,0,AVNoStat,0,AVNoStat,0,9}
};
}
const FAVEquipmentSpec* FAVEquipment::Find(int32 Id)
{
    return Id>=0 && Id<Count ? &Items[Id] : nullptr;
}
int32 FAVEquipment::RarityRoll(int32 Percentile)
{
    const int32 P=Percentile<0?0:Percentile>99?99:Percentile;
    return P<40?AVCommon:P<70?AVUncommon:P<88?AVRare:P<97?AVEpic:AVLegendary;
}
bool FAVEquipment::CanEquip(int32 Id,int32 HeroClass)
{
    const FAVEquipmentSpec* Spec=Find(Id);
    return Spec && (Spec->RequiredHero<0 || Spec->RequiredHero==HeroClass);
}
float FAVEquipment::SetBonus(int32 SetId,int32 EquippedCount)
{
    if(SetId<1 || SetId>5 || EquippedCount<3) return 0.f;
    // set1 physical defense, set2 speed, set3 abilities, set4 ranged, set5 HP.
    switch(SetId) {case 1:return 5.f;case 2:return 8.f;case 3:return 8.f;case 4:return 10.f;default:return 20.f;}
}
