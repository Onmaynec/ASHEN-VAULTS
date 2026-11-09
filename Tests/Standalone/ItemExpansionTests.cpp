#include "AVEquipment.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <set>
#include <string>

int main()
{
    static_assert(FAVEquipment::LegacyCount==42,"Legacy item IDs must not move");
    static_assert(FAVEquipment::Count==107,"Expected 42 + 65 stable catalog IDs");
    static_assert(FAVEquipment::GearCount==97,"40 weapons, 45 armor and 12 accessories");
    std::set<std::string> Names;
    int NewWeapons[4]={0,0,0,0};
    int NewArmor[4]={0,0,0,0};
    int CommonWeapons[4]={0,0,0,0};
    int LegendaryWeapons[4]={0,0,0,0};
    int CommonArmor[4]={0,0,0,0};
    int Patterns[11]={0};
    int SpecialResists=0;
    for(int Id=0;Id<FAVEquipment::Count;++Id)
    {
        const FAVEquipmentSpec* S=FAVEquipment::Find(Id);
        assert(S&&S->Id==Id&&S->CodeName&&S->CodeName[0]);
        assert(Names.insert(S->CodeName).second);
        assert(S->Rarity>=AVCommon&&S->Rarity<=AVLegendary);
        assert(S->RequiredHero>=-1&&S->RequiredHero<4);
        assert(S->StatA>=AVNoStat&&S->StatA<=AVBurnResistPercent);
        assert(S->StatB>=AVNoStat&&S->StatB<=AVBurnResistPercent);
        if(S->StatA==AVMeleeResistPercent||S->StatA==AVPoisonResistPercent||
           S->StatA==AVBurnResistPercent||S->StatB==AVMeleeResistPercent||
           S->StatB==AVPoisonResistPercent||S->StatB==AVBurnResistPercent)
            ++SpecialResists;
        if(Id<42)assert(!S->DisplayName); // exact legacy prototypes retained.
        else assert(S->DisplayName&&S->DisplayName[0]);
        if(Id>=42&&Id<=71)
        {
            assert(S->Slot==AVWeapon&&S->RequiredHero>=0);
            ++NewWeapons[S->RequiredHero];
            if(S->Rarity==AVCommon)++CommonWeapons[S->RequiredHero];
            if(S->Rarity==AVLegendary)++LegendaryWeapons[S->RequiredHero];
            for(int Hero=0;Hero<4;++Hero)
                assert(FAVEquipment::CanEquip(Id,Hero)==(Hero==S->RequiredHero));
            assert(S->Pattern>=1&&S->Pattern<=10);
            ++Patterns[S->Pattern];
        }
        if(Id>=72&&Id<=91)
        {
            assert(S->Slot>=AVHead&&S->Slot<=AVBoots);
            assert(S->RequiredHero>=0);
            ++NewArmor[S->RequiredHero];
            if(S->Rarity==AVCommon)++CommonArmor[S->RequiredHero];
            assert(S->Pattern==0);
        }
        if(Id>=92&&Id<=96)
        {
            assert(S->Slot==AVAccessory&&S->RequiredHero==-1);
            assert(S->Rarity>=AVRare);
        }
        if(Id>=97)
        {
            assert(S->Slot==AVConsumable);
            assert(S->PerkId==10+(Id-97));
            assert(!FAVEquipment::CanEquip(Id,0));
            assert(FAVEquipment::IsConsumable(Id));
        }
        else assert(!FAVEquipment::IsConsumable(Id));
        const int Affix=FAVEquipment::RollAffix(Id,0);
        if(S->Slot==AVWeapon&&S->Rarity>=AVEpic)
            assert(Affix>=1&&Affix<=6);
        else assert(Affix==0);
        if(S->Slot!=AVWeapon||S->Rarity<AVEpic)
        {
            assert(FAVEquipment::EncodeInstance(Id,1)==FAVEquipment::InvalidId);
            assert(FAVEquipment::RollAffix(Id,99)==0);
        }
    }
    assert(Names.size()==107&&SpecialResists>=5);
    assert(NewWeapons[0]==8&&NewWeapons[1]==8);
    assert(NewWeapons[2]==7&&NewWeapons[3]==7);
    for(int Hero=0;Hero<4;++Hero)
    {
        assert(NewArmor[Hero]==5);
        assert(CommonWeapons[Hero]>=1);
        assert(LegendaryWeapons[Hero]>=1);
        assert(CommonArmor[Hero]>=1);
    }
    for(int Pattern=1;Pattern<=10;++Pattern)
        assert(FAVEquipment::PatternName(Pattern)[0]);

    int EpicRolled=0,LegendaryRolled=0;
    for(int P=0;P<100;++P)
    {
        EpicRolled+=FAVEquipment::RollAffix(45,P)>0;
        LegendaryRolled+=FAVEquipment::RollAffix(48,P)>0;
    }
    assert(FAVEquipment::Find(45)->Rarity==AVEpic);
    assert(FAVEquipment::Find(48)->Rarity==AVLegendary);
    assert(EpicRolled==9&&LegendaryRolled==16);

    for(int Affix=1;Affix<=6;++Affix)
    {
        const int Encoded=FAVEquipment::EncodeInstance(48,Affix);
        assert(Encoded>FAVEquipment::Count&&Encoded<65536);
        const float SavedPickupAmount=static_cast<float>(Encoded);
        assert(static_cast<int>(SavedPickupAmount)==Encoded);
        assert(FAVEquipment::BaseId(Encoded)==48);
        assert(FAVEquipment::AffixId(Encoded)==Affix);
        assert(FAVEquipment::Find(Encoded)==FAVEquipment::Find(48));
        assert(FAVEquipment::CanEquip(Encoded,0));
        assert(FAVEquipment::AffixName(Affix)[0]);
        assert(FAVEquipment::BonusAgainst(Encoded,0,false,false)==
            (Affix==1?3:0));
        assert(FAVEquipment::BonusAgainst(Encoded,3,false,false)==
            (Affix==4?3:0));
        assert(FAVEquipment::BonusAgainst(Encoded,1,true,false)==
            ((Affix==2?3:0)+(Affix==5?4:0)));
        assert(FAVEquipment::BonusAgainst(Encoded,1,false,true)==
            ((Affix==2?3:0)+(Affix==6?5:0)));
    }
    assert(!FAVEquipment::Find(-1)&&!FAVEquipment::Find(107));
    assert(!FAVEquipment::Find(48+7*1024));
    assert(!FAVEquipment::Find(42+1024)); // uncommon cannot carry affix
    assert(!FAVEquipment::Find(97+1024)); // consumables never affixed

    std::cout<<"PASS: 65 new items, 107 unique stable IDs, 30 weapon patterns, "
             <<"20 class armor items, five rings, ten consumables, "
             <<"9%/16% special-affix rolls and 6 correct bonus conditions\n";
    return 0;
}
