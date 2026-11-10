#include "AVContracts.h"

namespace
{
uint32 Mix(uint32 X)
{
    X ^= X >> 16;
    X *= 0x7feb352du;
    X ^= X >> 15;
    X *= 0x846ca68bu;
    return X ^ (X >> 16);
}
int32 MinInt(int32 A,int32 B) { return A<B?A:B; }
}

FAVContractState FAVContracts::Offer(int32 Seed,int32 Floor,int32 Hostiles,
                                    int32 Elites,int32 Chests,int32 Echoes)
{
    FAVContractState Result;
    const int32 Available[4]={Hostiles,Elites,Chests,Echoes};
    int32 Choices[4]={};
    int32 Count=0;
    for(int32 I=0;I<4;++I)
        if(Available[I]>0)Choices[Count++]=I;
    if(Count==0)return Result;

    // Floor and seed select the same offer across saves and replays, without RNG use.
    const uint32 Hash=Mix(static_cast<uint32>(Seed)^(
        static_cast<uint32>(Floor)*0x9e3779b9u)^0x5a7b13cdu);
    const int32 Choice=Choices[Hash%static_cast<uint32>(Count)];
    Result.Kind=Choice+1;
    const int32 Desired=Choice==0 ? 3+int32(Hash%4u) :
                        Choice==1 ? 1+int32((Hash>>8)%2u) :
                        1+int32((Hash>>12)%2u);
    Result.Target=MinInt(Available[Choice],Desired);
    const int32 SafeFloor=Floor<1?1:Floor>1000?1000:Floor;
    Result.RewardGold=20+SafeFloor*5+(Choice==1?30:Choice==2?20:Choice==3?15:0);
    return Result;
}

bool FAVContracts::Valid(const FAVContractState& State)
{
    if(State.Kind==AVContractNone)
        return State.Target==0&&State.Progress==0&&State.RewardGold==0&&!State.bPaid;
    return State.Kind>=AVContractCull&&State.Kind<=AVContractEcho&&
           State.Target>=1&&State.Target<=12&&
           State.Progress>=0&&State.Progress<=State.Target&&
           State.RewardGold>=1&&State.RewardGold<=10000&&
           (!State.bPaid||State.Progress==State.Target);
}

int32 FAVContracts::Record(FAVContractState& State,int32 EventKind)
{
    if(!Valid(State)||State.Kind==AVContractNone||State.bPaid||
       EventKind<AVContractCull||EventKind>AVContractEcho)return 0;
    const bool bMatched=State.Kind==EventKind ||
        (State.Kind==AVContractCull&&EventKind==AVContractElite);
    if(!bMatched)return 0;
    if(State.Progress<State.Target)++State.Progress;
    if(State.Progress==State.Target)
    {
        State.bPaid=true;
        return State.RewardGold;
    }
    return 0;
}

const char* FAVContracts::Name(int32 Kind)
{
    switch(Kind)
    {
    case AVContractCull: return "SLAY FOES";
    case AVContractElite: return "HUNT ELITES";
    case AVContractChest: return "OPEN CHESTS";
    case AVContractEcho: return "RECOVER ECHOES";
    default: return "NO CONTRACT";
    }
}
