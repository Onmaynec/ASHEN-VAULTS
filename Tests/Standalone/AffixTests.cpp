#include "AVAffixes.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main()
{
    int32 Seen[FAVAffixes::Count]={};
    for(int32 Seed=1;Seed<=15000;++Seed)
    {
        const int32 Floor=1+Seed%120;
        const int32 Affix=FAVAffixes::Roll(
            static_cast<uint32>(Seed*911),Floor,Seed%57,Seed%400,Seed%7==0,false);
        assert(Affix>=0&&Affix<FAVAffixes::Count);
        assert(FAVAffixes::Roll(static_cast<uint32>(Seed*911),
            Floor,Seed%57,Seed%400,Seed%7==0,false)==Affix);
        assert(FAVAffixes::Roll(static_cast<uint32>(Seed*911),
            Floor,Seed%57,Seed%400,Seed%7==0,true)==0);
        ++Seen[Affix];
    }
    for(int32 Id=0;Id<FAVAffixes::Count;++Id)
    {
        assert(Seen[Id]>0);
        const FAVAffixTraits A=FAVAffixes::Traits(Id,0.9f,4.f);
        const FAVAffixTraits B=FAVAffixes::Traits(Id,0.35f,2.f);
        assert(A.MaxHealthScale>0.f && B.MaxHealthScale>0.f);
        assert(A.MovementScale>0.f && B.MovementScale>0.f);
        assert(A.CooldownScale>0.f && B.CooldownScale>0.f);
        assert(A.OutgoingScale>0.f && B.OutgoingScale>0.f);
        assert(A.IncomingScale>0.f && B.IncomingScale>0.f);
        if(Id>0)assert(FAVAffixes::Name(Id)[0]!='\0');
    }
    assert(FAVAffixes::Traits(1,0.35f,1.f).MovementScale>1.f);
    assert(FAVAffixes::Traits(1,0.85f,1.f).MovementScale==1.f);
    assert(FAVAffixes::Traits(1,0.35f,1.f).CooldownScale<1.f);
    assert(FAVAffixes::Traits(2,1.f,0.f).IncomingScale<1.f);
    assert(FAVAffixes::Traits(3,1.f,0.f).MaxHealthScale<1.f);
    assert(FAVAffixes::Traits(3,1.f,0.f).OutgoingScale>1.f);
    assert(FAVAffixes::Traits(4,1.f,0.f).LeechFraction==0.2f);
    assert(FAVAffixes::Traits(5,1.f,0.f).InflictedStatus==1);
    assert(FAVAffixes::Traits(6,1.f,0.f).InflictedStatus==2);
    assert(FAVAffixes::Traits(7,1.f,0.f).AllySpeedAura>1.f);
    assert(FAVAffixes::Traits(8,1.f,0.f).AllyHealingPerSecond>0.f);
    assert(FAVAffixes::Traits(9,1.f,0.f).MemorySeconds>3.5f);
    assert(FAVAffixes::Traits(10,1.f,0.f).BoltSpeedScale>1.f);
    assert(FAVAffixes::Traits(11,1.f,0.f).MaxHealthScale>1.f);
    assert(FAVAffixes::Traits(11,1.f,0.f).MovementScale<1.f);
    assert(FAVAffixes::Traits(12,1.f,0.f).bConsumesRuneWard);
    for(int32 Id=0;Id<FAVAffixes::Count;++Id)
    {
        bool bWard=false;
        const float First=FAVAffixes::TakeDamage(Id,100.f,bWard);
        const float Second=FAVAffixes::TakeDamage(Id,100.f,bWard);
        assert(First>=0.f&&First<=100.f);
        assert(Second>=0.f&&Second<=100.f);
        if(Id==12)
        {
            assert(First==50.f);
            assert(Second==100.f);
            assert(bWard);
        }
        else if(Id==2)
        {
            assert(First==78.f);
            assert(Second==78.f);
            assert(!bWard);
        }
        else
        {
            assert(First==Second);
            assert(!bWard);
        }
    }
    std::cout<<"PASS: 15000 deterministic affix rolls, all 12 roles, "
                "status, aura, health, damage, regen and one-use ward\n";
}
