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
    {41,"commandant_seal",AVAccessory,-1,AVLegendary,0,AVNoStat,0,AVNoStat,0,9},
    // v1.5.0: new weapon patterns, armor, rings and consumable items.
    {42,"crypt_cleaver",AVWeapon,0,AVCommon,0,AVAttackFlat,10,AVMeleeDamagePercent,5,0,1,"Секач склепов"},
    {43,"wardens_pike",AVWeapon,0,AVRare,0,AVAttackFlat,14,AVAttackSpeedPercent,-7,0,2,"Пика дозорного"},
    {44,"tomb_hammer",AVWeapon,0,AVRare,0,AVAttackFlat,17,AVAttackSpeedPercent,-12,1,3,"Могильный молот"},
    {45,"echo_halberd",AVWeapon,0,AVEpic,0,AVAttackFlat,20,AVAbilityDamagePercent,8,0,2,"Алебарда эха"},
    {46,"bonebreaker_mace",AVWeapon,0,AVEpic,0,AVAttackFlat,21,AVMeleeDamagePercent,12,1,3,"Костолом"},
    {47,"sunken_flail",AVWeapon,0,AVEpic,0,AVAttackFlat,19,AVCriticalPercent,6,0,1,"Затонувший кистень"},
    {48,"bastion_greatsword",AVWeapon,0,AVLegendary,0,AVAttackFlat,29,AVAttackSpeedPercent,-19,2,3,"Меч чёрного бастиона"},
    {49,"ashward_blade",AVWeapon,0,AVLegendary,0,AVAttackFlat,25,AVArmorPercent,4,0,5,"Пепельный страж"},
    {50,"dusk_rapier",AVWeapon,1,AVCommon,0,AVAttackFlat,10,AVCriticalPercent,3,0,2,"Сумеречная рапира"},
    {51,"serpent_fang",AVWeapon,1,AVRare,0,AVAttackFlat,14,AVAttackSpeedPercent,6,0,4,"Змеиный клык"},
    {52,"ember_sickle",AVWeapon,1,AVRare,0,AVAttackFlat,15,AVMeleeDamagePercent,8,0,5,"Тлеющий серп"},
    {53,"ghost_daggers",AVWeapon,1,AVEpic,0,AVAttackFlat,19,AVAttackSpeedPercent,13,0,2,"Призрачные кинжалы"},
    {54,"redmoon_sabre",AVWeapon,1,AVEpic,0,AVAttackFlat,21,AVCriticalPercent,9,0,1,"Сабля красной луны"},
    {55,"blight_kukri",AVWeapon,1,AVEpic,0,AVAttackFlat,20,AVMeleeDamagePercent,10,0,4,"Чумной кукри"},
    {56,"quicksilver_blades",AVWeapon,1,AVLegendary,0,AVAttackFlat,24,AVAttackSpeedPercent,17,3,2,"Ртутные клинки"},
    {57,"oathbreaker_knife",AVWeapon,1,AVLegendary,0,AVAttackFlat,28,AVCriticalPercent,11,8,10,"Клинок клятвопреступника"},
    {58,"spore_scepter",AVWeapon,2,AVCommon,0,AVAttackFlat,10,AVAbilityDamagePercent,6,0,4,"Споровый скипетр"},
    {59,"cinder_focus",AVWeapon,2,AVRare,0,AVAttackFlat,14,AVCooldownPercent,-5,0,5,"Угольный фокус"},
    {60,"grave_rod",AVWeapon,2,AVRare,0,AVAttackFlat,16,AVStaminaRegenPercent,9,0,2,"Могильный жезл"},
    {61,"lantern_staff",AVWeapon,2,AVEpic,0,AVAttackFlat,19,AVAbilityDamagePercent,12,0,5,"Посох странника"},
    {62,"rune_catalyst",AVWeapon,2,AVEpic,0,AVAttackFlat,21,AVCooldownPercent,-9,7,2,"Рунный катализатор"},
    {63,"witchbone_wand",AVWeapon,2,AVLegendary,0,AVAttackFlat,26,AVCriticalPercent,8,0,4,"Ведьмина кость"},
    {64,"void_orb",AVWeapon,2,AVLegendary,0,AVAttackFlat,27,AVAbilityDamagePercent,18,7,5,"Сфера пустоты"},
    {65,"bone_wood_bow",AVWeapon,3,AVCommon,0,AVAttackFlat,11,AVProjectileSpeedPercent,6,0,8,"Костяной лук"},
    {66,"falcon_recurve",AVWeapon,3,AVRare,0,AVAttackFlat,15,AVAttackSpeedPercent,8,4,8,"Соколий рекурсив"},
    {67,"venom_longbow",AVWeapon,3,AVRare,0,AVAttackFlat,16,AVRangedDamagePercent,8,0,4,"Ядовитый длинный лук"},
    {68,"ember_arbalest",AVWeapon,3,AVEpic,0,AVAttackFlat,22,AVAttackSpeedPercent,-10,0,9,"Угольный арбалет"},
    {69,"bastion_repeater",AVWeapon,3,AVEpic,0,AVAttackFlat,20,AVProjectileSpeedPercent,14,5,6,"Бастионный повторитель"},
    {70,"moonstring_bow",AVWeapon,3,AVLegendary,0,AVAttackFlat,25,AVCriticalPercent,9,0,6,"Лунная тетива"},
    {71,"blackglass_crossbow",AVWeapon,3,AVLegendary,0,AVAttackFlat,31,AVAttackSpeedPercent,-16,5,7,"Арбалет чёрного стекла"},
    {72,"gravewatch_visage",AVHead,0,AVCommon,0,AVArmorPercent,2,AVHealthFlat,7,0,0,"Маска могильного дозора"},
    {73,"stoneward_plate",AVChest,0,AVRare,0,AVArmorPercent,6,AVMeleeResistPercent,8,0,0,"Плита каменного стража"},
    {74,"ironbound_grips",AVHands,0,AVRare,0,AVArmorPercent,3,AVMeleeDamagePercent,6,0,0,"Железные рукавицы"},
    {75,"warden_bulwark_greaves",AVLegs,0,AVEpic,0,AVArmorPercent,5,AVKnockbackResistPercent,14,0,0,"Поножи бастионного щита"},
    {76,"sentinel_treads",AVBoots,0,AVLegendary,0,AVArmorPercent,4,AVMoveSpeedPercent,5,0,0,"Сапоги часового"},
    {77,"nightveil_mask",AVHead,1,AVCommon,0,AVArmorPercent,1,AVCriticalPercent,3,0,0,"Ночная маска"},
    {78,"shade_runner_coat",AVChest,1,AVRare,0,AVArmorPercent,3,AVMeleeResistPercent,7,0,0,"Куртка теневого бегуна"},
    {79,"viper_grips",AVHands,1,AVEpic,0,AVArmorPercent,1,AVAttackSpeedPercent,13,0,0,"Змеиные перчатки"},
    {80,"redmoon_leggings",AVLegs,1,AVEpic,0,AVArmorPercent,2,AVStaminaFlat,17,0,0,"Поножи красной луны"},
    {81,"windstep_boots",AVBoots,1,AVLegendary,0,AVArmorPercent,2,AVDashCooldownPercent,-16,0,0,"Сапоги ветра"},
    {82,"moss_crown",AVHead,2,AVCommon,0,AVArmorPercent,1,AVPoisonResistPercent,16,0,0,"Моховой венец"},
    {83,"ashen_savant_robe",AVChest,2,AVRare,0,AVArmorPercent,2,AVAbilityDamagePercent,10,0,0,"Роба пепельного мудреца"},
    {84,"warded_sleeves",AVHands,2,AVRare,0,AVArmorPercent,2,AVBurnResistPercent,14,0,0,"Рукава печатей"},
    {85,"runeweaver_skirt",AVLegs,2,AVEpic,0,AVArmorPercent,2,AVStaminaFlat,22,0,0,"Рунные одежды"},
    {86,"starless_boots",AVBoots,2,AVLegendary,0,AVArmorPercent,2,AVCooldownPercent,-12,0,0,"Ботинки беззвёздья"},
    {87,"falcon_visor",AVHead,3,AVCommon,0,AVArmorPercent,1,AVCriticalPercent,4,0,0,"Соколиный визор"},
    {88,"scout_leather",AVChest,3,AVRare,0,AVArmorPercent,3,AVMeleeResistPercent,6,0,0,"Кожанка разведчика"},
    {89,"archer_wraps",AVHands,3,AVRare,0,AVArmorPercent,1,AVAttackSpeedPercent,10,0,0,"Повязки лучника"},
    {90,"pathfinder_breeches",AVLegs,3,AVEpic,0,AVArmorPercent,3,AVStaminaRegenPercent,11,0,0,"Штаны следопыта"},
    {91,"horizon_boots",AVBoots,3,AVLegendary,0,AVArmorPercent,2,AVMoveSpeedPercent,9,0,0,"Сапоги горизонта"},
    {92,"ring_of_venomward",AVAccessory,-1,AVRare,0,AVPoisonResistPercent,30,AVStatusResistPercent,8,0,0,"Кольцо противоядия"},
    {93,"ring_of_iron_skin",AVAccessory,-1,AVRare,0,AVMeleeResistPercent,9,AVArmorPercent,2,0,0,"Кольцо железной кожи"},
    {94,"ring_of_embers",AVAccessory,-1,AVEpic,0,AVBurnResistPercent,35,AVAbilityDamagePercent,8,0,0,"Кольцо углей"},
    {95,"ring_of_clear_mind",AVAccessory,-1,AVEpic,0,AVCooldownPercent,-8,AVStaminaRegenPercent,12,0,0,"Кольцо ясности"},
    {96,"ring_of_the_fourth_vault",AVAccessory,-1,AVLegendary,0,AVBossDamagePercent,12,AVHealthFlat,18,0,0,"Кольцо четвёртого свода"},
    {97,"healing_tonic",AVConsumable,-1,AVUncommon,0,AVNoStat,48,AVNoStat,0,10,0,"Настой живучести"},
    {98,"stamina_draught",AVConsumable,-1,AVUncommon,0,AVNoStat,55,AVNoStat,0,11,0,"Настой выносливости"},
    {99,"cleansing_candy",AVConsumable,-1,AVRare,0,AVNoStat,0,AVNoStat,0,12,0,"Очищающая конфета"},
    {100,"spore_antidote",AVConsumable,-1,AVRare,0,AVNoStat,14,AVNoStat,0,13,0,"Споровое противоядие"},
    {101,"honey_ash_pie",AVConsumable,-1,AVRare,0,AVNoStat,14,AVNoStat,0,14,0,"Медовый пепельный пирог"},
    {102,"iron_biscuit",AVConsumable,-1,AVRare,0,AVNoStat,15,AVNoStat,0,15,0,"Железное печенье"},
    {103,"swiftroot_tea",AVConsumable,-1,AVRare,0,AVNoStat,14,AVNoStat,0,16,0,"Чай быстрого корня"},
    {104,"eagle_eye_drops",AVConsumable,-1,AVEpic,0,AVNoStat,12,AVNoStat,0,17,0,"Капли орлиного глаза"},
    {105,"storm_flask",AVConsumable,-1,AVEpic,0,AVNoStat,12,AVNoStat,0,18,0,"Штормовой флакон"},
    {106,"guardian_tart",AVConsumable,-1,AVEpic,0,AVNoStat,18,AVNoStat,0,19,0,"Пирог хранителя"}
};
}
const char* FAVEquipment::StatName(int32 StatId)
{
    static const char* Names[] = {
        "-", "Attack", "Armor %", "HP", "Stamina", "Move %", "Attack speed %",
        "Crit %", "Melee damage %", "Ranged damage %", "Skill damage %",
        "Stamina regen %", "Dash cooldown %", "Projectile speed %",
        "Knockback resist %", "Status resist %", "Boss damage %",
        "Cooldown %", "Melee resist %", "Poison resist %", "Burn resist %"
    };
    return StatId>=0 && StatId<21 ? Names[StatId] : "Unknown";
}
const char* FAVEquipment::PerkName(int32 PerkId)
{
    static const char* Names[] = {
        "-", "Heavy stagger", "Execution bonus", "Heal on fifth hit",
        "Faster arrows", "Pierce every fourth bolt", "Heal 2 HP on kill",
        "Every fourth magic throw echoes", "Execute at low enemy HP",
        "+10% damage after elite kill",
        "Restore HP", "Restore stamina", "Cleanse poison and burn",
        "Poison ward", "Attack buff", "Melee guard", "Speed buff",
        "Critical focus", "Projectile haste", "Boss-hunter buff"
    };
    return PerkId>=0 && PerkId<20 ? Names[PerkId] : "Unknown";
}
int32 FAVEquipment::BaseId(int32 InstanceId)
{
    return InstanceId>=0 ? InstanceId % InstanceStride : InvalidId;
}
int32 FAVEquipment::AffixId(int32 InstanceId)
{
    return InstanceId>=0 ? InstanceId / InstanceStride : 0;
}
const FAVEquipmentSpec* FAVEquipment::Find(int32 InstanceId)
{
    const int32 Id=BaseId(InstanceId);
    const int32 Affix=AffixId(InstanceId);
    if(Id<0||Id>=Count||Affix<0||Affix>6)return nullptr;
    const FAVEquipmentSpec* Spec=&Items[Id];
    if(Affix>0 && (Spec->Slot!=AVWeapon || Spec->Rarity<AVEpic))
        return nullptr;
    return Spec;
}
int32 FAVEquipment::EncodeInstance(int32 Id,int32 Affix)
{
    const FAVEquipmentSpec* Spec=Find(Id);
    if(!Spec || Affix<0 || Affix>6 ||
       (Affix>0 && (Spec->Slot!=AVWeapon || Spec->Rarity<AVEpic)))
        return InvalidId;
    return Id+Affix*InstanceStride;
}
int32 FAVEquipment::RollAffix(int32 Id,int32 Percentile)
{
    const FAVEquipmentSpec* Spec=Find(Id);
    if(!Spec||Spec->Slot!=AVWeapon||Spec->Rarity<AVEpic)return 0;
    const int32 Roll=FMath::Clamp(Percentile,0,99);
    const int32 Threshold=Spec->Rarity==AVLegendary?16:9;
    return Roll<Threshold?1+((Id*7+Roll)%6):0;
}
int32 FAVEquipment::BonusAgainst(int32 InstanceId,int32 Biome,bool bElite,bool bBoss)
{
    const int32 Affix=Find(InstanceId)?AffixId(InstanceId):0;
    if(Affix>=1&&Affix<=4&&Biome==Affix-1)return 3;
    if(Affix==5&&bElite)return 4;
    if(Affix==6&&bBoss)return 5;
    return 0;
}
const char* FAVEquipment::AffixName(int32 Affix)
{
    static const char* Names[]={"","Охотник склепов","Охотник спор",
        "Охотник кузни","Охотник бастиона","Истребитель элиты",
        "Погибель хранителей"};
    return Affix>=1&&Affix<=6?Names[Affix]:"";
}
bool FAVEquipment::IsConsumable(int32 InstanceId)
{
    const FAVEquipmentSpec* Spec=Find(InstanceId);
    return Spec && Spec->Slot==AVConsumable;
}
const char* FAVEquipment::PatternName(int32 Pattern)
{
    static const char* Names[]={"Обычная атака","Широкий рубящий удар",
        "Длинный выпад","Тяжёлый удар","Отравляющее попадание",
        "Огненное попадание","Двойной выстрел","Пробивающий выстрел",
        "Быстрая стрела","Тяжёлый болт","Восстановление HP"};
    return Pattern>=0&&Pattern<11?Names[Pattern]:"Classic";
}
int32 FAVEquipment::RarityRoll(int32 Percentile)
{
    const int32 P=Percentile<0?0:Percentile>99?99:Percentile;
    return P<40?AVCommon:P<70?AVUncommon:P<88?AVRare:P<97?AVEpic:AVLegendary;
}
bool FAVEquipment::CanEquip(int32 Id,int32 HeroClass)
{
    const FAVEquipmentSpec* Spec=Find(Id);
    return Spec && Spec->Slot!=AVConsumable &&
        (Spec->RequiredHero<0 || Spec->RequiredHero==HeroClass);
}
float FAVEquipment::SetBonus(int32 SetId,int32 EquippedCount)
{
    if(SetId<1 || SetId>5 || EquippedCount<3) return 0.f;
    // set1 physical defense, set2 speed, set3 abilities, set4 ranged, set5 HP.
    switch(SetId) {case 1:return 5.f;case 2:return 8.f;case 3:return 8.f;case 4:return 10.f;default:return 20.f;}
}
