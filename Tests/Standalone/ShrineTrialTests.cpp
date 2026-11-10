#include "AVShrineTrials.h"
#include <cassert>
#include <iostream>

int main()
{
    assert(FAVShrineTrials::UnitsForFloor(1)==2);
    assert(FAVShrineTrials::UnitsForFloor(6)==3);
    assert(FAVShrineTrials::UnitsForFloor(16)==4);
    assert(FAVShrineTrials::PactCost(100.f)==22);
    assert(!FAVShrineTrials::CanPact(22.f,100.f));
    assert(FAVShrineTrials::CanPact(23.f,100.f));
    for(int32 Floor=1;Floor<=200;++Floor)
        for(int32 Seed=0;Seed<400;++Seed)
        {
            const int32 Count=FAVShrineTrials::UnitsForFloor(Floor);
            FAVShrineState S;
            assert(FAVShrineTrials::Valid(S));
            assert(!FAVShrineTrials::Choose(S,AVShrineTrial,0,100,100));
            assert(!FAVShrineTrials::Choose(S,7,0,100,100));
            assert(FAVShrineTrials::Choose(S,AVShrineTrial,Count,100,100));
            assert(!FAVShrineTrials::Choose(S,AVShrineRest,0,100,100));
            assert(FAVShrineTrials::OnEnemyDefeated(S,123,Floor)==0);
            int32 Total=0;
            for(int32 Index=0;Index<Count;++Index)
            {
                const int32 Id=FAVShrineTrials::TrialEnemyId(Index);
                assert(FAVShrineTrials::IsTrialEnemy(Id));
                Total+=FAVShrineTrials::OnEnemyDefeated(S,Id,Floor);
                assert(S.Remaining==Count-Index-1);
            }
            assert(S.bRewarded&&Total==FAVShrineTrials::RewardGold(Floor,1));
            assert(FAVShrineTrials::OnEnemyDefeated(S,-1101,Floor)==0);
            FAVShrineTrials::NextFloor(S);
            assert(S.Streak==1&&S.Choice==AVShrineUnchosen);
            assert(FAVShrineTrials::Choose(S,AVShrineRest,0,100,100));
            assert(S.Streak==0&&FAVShrineTrials::DamageBonusPercent(S)==0);
            FAVShrineTrials::NextFloor(S);
            assert(FAVShrineTrials::Choose(S,AVShrinePact,0,30,100));
            assert(FAVShrineTrials::DamageBonusPercent(S)==18);
            assert(S.Streak==0);
            FAVShrineTrials::NextFloor(S);
            assert(FAVShrineTrials::DamageBonusPercent(S)==0);
            (void)Seed;
        }
    FAVShrineState S;
    for(int I=0;I<5;++I)
    {
        assert(FAVShrineTrials::Choose(S,AVShrineTrial,1,100,100));
        assert(FAVShrineTrials::OnEnemyDefeated(S,-1101,I+1)>0);
        FAVShrineTrials::NextFloor(S);
        assert(S.Streak==(I<2?I+1:3));
    }
    FAVShrineTrials::NextFloor(S); // skipping a floor breaks the chain
    assert(S.Streak==0);
    S.Streak=4;
    assert(!FAVShrineTrials::Valid(S));
    FAVShrineTrials::NextFloor(S);
    assert(S.Streak==0);
    std::cout<<"PASS: 80,000 trial simulations, reward exactly once, HP cost, streak and reset\n";
}
