#include "AVSkills.h"
// Effect IDs: 1 HP %, 2 status duration %, 3 potion heal, 4 damage taken %,
// 5 last-stand, 6 basic damage %, 7 crit chance %, 8 every-fourth-hit damage %,
// 9 boss damage %, 10 kill cooldown reduction, 11 move speed %, 12 stamina flat,
// 13 stamina regen %, 14 dash cost %, 15 dash invulnerability seconds,
// 16 armor %, 17 stun %, 18 spin mitigation %, 19 revenge %, 20 guardian shield,
// 21 shockwave, 22 after-dash crit, 23 thrown damage, 24 spin damage,
// 25 on-kill haste, 26 third strike, 27 execute, 28 ability damage %,
// 29 magic sphere, 30 rune wave, 31 chain critical, 32 magic barrier,
// 33 projectile speed %, 34 marked damage %, 35 extra traps, 36 piercing damage %,
// 37 triple power shot, 38 precision final.
namespace {
const FAVSkillSpec Nodes[FAVSkills::Count] = {
 {0,"sturdy_bones",-1,0,2,2,-1,0,1,6},
 {1,"enduring_skin",-1,0,2,4,0,0,2,10},
 {2,"last_drop",-1,0,2,6,1,0,3,8},
 {3,"firm_stance",-1,0,1,12,2,0,4,8},
 {4,"last_spark",-1,0,1,18,3,0,5,1},
 {5,"sharpness",-1,1,2,2,-1,0,6,4},
 {6,"precision",-1,1,2,4,5,0,7,3},
 {7,"battle_rhythm",-1,1,2,6,6,0,8,7},
 {8,"titan_hunter",-1,1,1,12,7,0,9,8},
 {9,"bloodthirst",-1,1,1,18,8,0,10,0.5f},
 {10,"light_step",-1,2,2,2,-1,0,11,3},
 {11,"deep_breath",-1,2,2,4,10,0,12,10},
 {12,"second_wind",-1,2,2,6,11,0,13,6},
 {13,"efficient_dash",-1,2,1,12,12,0,14,10},
 {14,"evasion",-1,2,1,18,13,0,15,0.07f},
 {15,"steel_skin",0,3,2,3,-1,0,16,2},
 {16,"heavy_hit",0,3,2,5,15,0,17,8},
 {17,"guarded_whirl",0,3,2,8,16,0,18,8},
 {18,"counterstrike",0,3,2,12,17,0,19,8},
 {19,"protector",0,3,1,20,18,1,20,25},
 {20,"retribution",0,3,1,20,18,1,21,50},
 {21,"blade_dance",1,3,2,3,-1,0,22,3},
 {22,"perfect_throw",1,3,2,5,21,0,23,12},
 {23,"steel_whirl",1,3,2,8,22,0,24,10},
 {24,"blood_tempo",1,3,2,12,23,0,25,6},
 {25,"phantom_combo",1,3,1,20,24,2,26,50},
 {26,"executioner",1,3,1,20,24,2,27,28},
 {27,"elemental_script",2,3,2,3,-1,0,28,6},
 {28,"spark_seal",2,3,2,5,27,0,29,1},
 {29,"rune_ring",2,3,2,8,28,0,30,1},
 {30,"deep_reserve",2,3,2,12,29,0,12,10},
 {31,"chain_discharge",2,3,1,20,30,3,31,1},
 {32,"salvation_seal",2,3,1,20,30,3,32,1},
 {33,"spring_bow",3,3,2,3,-1,0,33,7},
 {34,"prey_trail",3,3,2,5,33,0,34,5},
 {35,"volley_mastery",3,3,2,8,34,0,35,1},
 {36,"piercing_force",3,3,2,12,35,0,36,10},
 {37,"triple_arrow",3,3,1,20,36,4,37,1},
 {38,"unerring_hunt",3,3,1,20,36,4,38,1}
};
}
const char* FAVSkills::Description(int32 Id)
{
    static const char* Text[Count] = {
        "+6% maximum health per rank",
        "-10% status duration per rank",
        "+8 health per potion per rank",
        "-8% damage taken under 30% health",
        "Survive one fatal strike per floor",
        "+4% basic attack damage per rank",
        "+3% critical chance per rank",
        "Every fourth hit: +7% damage per rank",
        "+8% damage to bosses",
        "Kills reduce Q/R cooldown by 0.5s",
        "+3% movement speed per rank",
        "+10 maximum stamina per rank",
        "+6% stamina recovery per rank",
        "-10% dash stamina cost",
        "+0.07s dash invulnerability",
        "+2% armor per rank",
        "More stagger and knockback per rank",
        "-8% damage after Q per rank",
        "Retaliate after taking damage",
        "Dash grants shield worth 25% max HP",
        "Every third strike creates shockwave",
        "+3% critical chance after dash",
        "+12% throw damage per rank",
        "+10% spin damage per rank",
        "Kills grant +6% speed per rank",
        "Every third hit: +50% bonus strike",
        "+28% damage below 25% enemy HP",
        "+6% ability damage per rank",
        "R changes into a magical sphere",
        "Q emits a radial rune wave",
        "Critical magic chains to 2 enemies",
        "Low HP grants one magical shield",
        "+7% arrow speed per rank",
        "+5% damage to marked foes per rank",
        "Volley strengthens; more arrows at rank 2",
        "+10% piercing shot damage per rank",
        "Piercing shot fires three arrows",
        "Fourth precise shot is critical"
    };
    return Id>=0 && Id<Count ? Text[Id] : "UNKNOWN SKILL";
}
const FAVSkillSpec* FAVSkills::Find(int32 Id) {
    return Id>=0 && Id<Count ? &Nodes[Id] : nullptr;
}
int32 FAVSkills::AwardedPointsAtLevel(int32 Level) {
    return Level<=1?0:Level>=30?29:Level-1;
}
bool FAVSkills::CanLearn(int32 Id,int32 Hero,int32 Level,
                         const uint8* Ranks,int32 RankCount,int32 Points) {
    const FAVSkillSpec* S=Find(Id);
    if(!S || !Ranks || Points<1 || Level<S->MinLevel ||
       !(S->RequiredHero<0 || S->RequiredHero==Hero) ||
       RankCount<Count || Ranks[Id]>=S->MaxRank) return false;
    if(S->PrerequisiteId>=0 && Ranks[S->PrerequisiteId]==0) return false;
    if(S->ExclusiveGroup>0)
        for(int32 I=0;I<Count;++I)
            if(I!=Id && Nodes[I].ExclusiveGroup==S->ExclusiveGroup && Ranks[I]>0)
                return false;
    return true;
}
