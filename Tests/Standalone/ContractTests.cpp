#include "AVContracts.h"
#include <cassert>
#include <cstring>
#include <iostream>

int main()
{
    assert(FAVContracts::Valid(FAVContractState{}));
    assert(FAVContracts::Offer(42,1,0,0,0,0).Kind==AVContractNone);
    assert(FAVContracts::Offer(42,1,0,0,0,3).Kind==AVContractEcho);
    assert(FAVContracts::Offer(42,1,0,0,2,0).Kind==AVContractChest);
    assert(FAVContracts::Offer(42,1,0,3,0,0).Kind==AVContractElite);
    assert(FAVContracts::Offer(42,1,8,0,0,0).Kind==AVContractCull);
    int Counts[5]={0};
    for(int32 Seed=0;Seed<1000;++Seed)
        for(int32 Floor=1;Floor<=80;++Floor)
        {
            FAVContractState C=FAVContracts::Offer(Seed,Floor,15,3,4,2);
            FAVContractState D=FAVContracts::Offer(Seed,Floor,15,3,4,2);
            assert(FAVContracts::Valid(C));
            assert(C.Kind==D.Kind && C.Target==D.Target && C.RewardGold==D.RewardGold);
            assert(C.Target>0 && C.Target<= (C.Kind==AVContractCull?15:
                C.Kind==AVContractElite?3:C.Kind==AVContractChest?4:2));
            assert(std::strlen(FAVContracts::Name(C.Kind))>3);
            ++Counts[C.Kind];
            assert(FAVContracts::Record(C,0)==0);
            assert(FAVContracts::Record(C,C.Kind==AVContractCull?AVContractChest:AVContractCull)==0);
            int32 Earned=0;
            for(int32 I=0;I<C.Target;++I)
            {
                Earned+=FAVContracts::Record(C,C.Kind);
                assert(C.Progress==I+1);
            }
            assert(Earned==C.RewardGold && C.bPaid);
            for(int32 I=0;I<10;++I)assert(FAVContracts::Record(C,C.Kind)==0);
            assert(FAVContracts::Valid(C));
        }
    for(int I=1;I<=4;++I)assert(Counts[I]>1000);
    FAVContractState C=FAVContracts::Offer(5,10,2,0,0,0);
    assert(FAVContracts::Record(C,AVContractElite)==0);
    assert(C.Progress==1);
    assert(FAVContracts::Record(C,AVContractElite)==C.RewardGold);
    assert(FAVContracts::Record(C,AVContractElite)==0);
    C.Progress=100;
    assert(!FAVContracts::Valid(C));
    assert(FAVContracts::Record(C,AVContractCull)==0);
    assert(FAVContracts::Offer(3,2000000,0,0,1,0).RewardGold<=10000);
    std::cout<<"PASS: 80,000 reproducible offers, reachable targets, "
             <<"all four contracts, reward paid once, bounds checks and elite cull\n";
}
