#include "AVEquipment.h"
#include "AVSkills.h"
#include <cassert>
#include <iostream>
int main() {
    int weapons=0,armor=0,accessories=0,sets[6]={0,0,0,0,0,0};
    for(int id=0;id<FAVEquipment::Count;++id) {
        const FAVEquipmentSpec* p=FAVEquipment::Find(id);
        assert(p && p->Id==id && p->CodeName[0]!='\\0');
        assert(p->Rarity>=AVCommon && p->Rarity<=AVLegendary);
        assert(p->RequiredHero>=-1 && p->RequiredHero<4);
        assert(FAVEquipment::CanEquip(id,p->RequiredHero<0?0:p->RequiredHero));
        if(p->Slot==AVWeapon)++weapons;
        else if(p->Slot==AVAccessory)++accessories;
        else ++armor;
        if(p->SetId>0) {assert(p->SetId<=5);++sets[p->SetId];}
    }
    assert(weapons==10 && armor==25 && accessories==7);
    for(int id=1;id<=5;++id){assert(sets[id]==5);assert(FAVEquipment::SetBonus(id,3)>0);}
    assert(FAVEquipment::RarityRoll(0)==AVCommon);
    assert(FAVEquipment::RarityRoll(39)==AVCommon);
    assert(FAVEquipment::RarityRoll(40)==AVUncommon);
    assert(FAVEquipment::RarityRoll(70)==AVRare);
    assert(FAVEquipment::RarityRoll(88)==AVEpic);
    assert(FAVEquipment::RarityRoll(99)==AVLegendary);
    assert(!FAVEquipment::Find(-1) && !FAVEquipment::Find(42));
    uint8 ranks[FAVSkills::Count]={0};
    for(int id=0;id<FAVSkills::Count;++id) {
        const FAVSkillSpec* p=FAVSkills::Find(id);
        assert(p && p->Id==id && p->CodeName[0]!='\\0');
        assert(p->MaxRank==1 || p->MaxRank==2);
        assert(p->RequiredHero>=-1 && p->RequiredHero<=3);
        assert(p->PrerequisiteId<id);
    }
    assert(FAVSkills::Count==39 && FAVSkills::AwardedPointsAtLevel(1)==0);
    assert(FAVSkills::AwardedPointsAtLevel(30)==29);
    assert(FAVSkills::CanLearn(0,3,2,ranks,FAVSkills::Count,1));
    assert(!FAVSkills::CanLearn(1,3,10,ranks,FAVSkills::Count,1)); // prerequisite
    ranks[0]=1;
    assert(FAVSkills::CanLearn(1,3,10,ranks,FAVSkills::Count,1));
    assert(!FAVSkills::CanLearn(15,3,20,ranks,FAVSkills::Count,1)); // wrong class
    assert(!FAVSkills::CanLearn(37,3,19,ranks,FAVSkills::Count,1)); // below level
    assert(!FAVSkills::CanLearn(0,3,20,ranks,FAVSkills::Count,0)); // no points
    std::cout<<"PASS: 42 equipment specs, five sets, 39 skills and restrictions\\n";
    return 0;
}
