#include "AVItemCombat.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main()
{
    const FAVMeleeStyle Default=FAVItemCombat::Melee(0,false);
    assert(Default.Range==1.92f&&Default.DamageScale==1.f&&
           Default.Status==0&&Default.HealOnHit==0.f);
    const FAVMeleeStyle Cleave=FAVItemCombat::Melee(1,false);
    const FAVMeleeStyle Lunge=FAVItemCombat::Melee(2,false);
    const FAVMeleeStyle Crush=FAVItemCombat::Melee(3,false);
    assert(Cleave.AimThreshold<0.f&&Cleave.Range>Default.Range);
    assert(Lunge.Range>Cleave.Range&&Lunge.AimThreshold>Default.AimThreshold);
    assert(Crush.DamageScale>1.f&&Crush.KnockbackBonus>0.f);
    assert(FAVItemCombat::Melee(4,false).Status==1);
    assert(FAVItemCombat::Melee(5,false).Status==2);
    assert(FAVItemCombat::Melee(10,false).HealOnHit==2.f);
    assert(FAVItemCombat::Melee(2,true).Range==2.25f);
    const FAVProjectileStyle Normal=FAVItemCombat::Projectile(0);
    assert(Normal.PierceCount==1&&Normal.ExtraShots==0);
    assert(FAVItemCombat::Projectile(4).Status==1);
    assert(FAVItemCombat::Projectile(5).Status==2);
    assert(FAVItemCombat::Projectile(6).ExtraShots==1);
    assert(FAVItemCombat::Projectile(7).PierceCount==2);
    assert(FAVItemCombat::Projectile(8).SpeedScale>1.f);
    assert(FAVItemCombat::Projectile(9).SpeedScale<1.f);
    assert(FAVItemCombat::Projectile(9).DamageScale>1.f);
    for(int P=0;P<=10;++P)
    {
        const FAVMeleeStyle M=FAVItemCombat::Melee(P,false);
        const FAVProjectileStyle R=FAVItemCombat::Projectile(P);
        assert(std::isfinite(M.Range)&&std::isfinite(M.DamageScale));
        assert(M.Range>=1.92f&&M.DamageScale>=1.f);
        assert(R.PierceCount>=1&&R.PierceCount<=2);
        assert(R.ExtraShots>=0&&R.ExtraShots<=1);
        assert(R.Status>=0&&R.Status<=2);
    }
    assert(FAVItemCombat::IncomingMeleeMultiplier(0.f,false)==1.f);
    assert(std::fabs(FAVItemCombat::IncomingMeleeMultiplier(9.f,false)-0.91f)<0.0001f);
    assert(std::fabs(FAVItemCombat::IncomingMeleeMultiplier(9.f,true)-0.79f)<0.0001f);
    assert(FAVItemCombat::IncomingMeleeMultiplier(120.f,true)==0.45f);
    std::cout<<"PASS: 11 weapon combat profiles, status effects, melee resistance and attack geometry\n";
    return 0;
}
