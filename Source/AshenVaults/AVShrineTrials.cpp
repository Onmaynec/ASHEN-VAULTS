#include "AVShrineTrials.h"
#include <cmath>

int32 FAVShrineTrials::UnitsForFloor(int32 Floor)
{
    return 2+(Floor>=6?1:0)+(Floor>=16?1:0);
}
int32 FAVShrineTrials::PactCost(float MaxHP)
{
    if(!(MaxHP>0.f))return 12;
    const float Cap=MaxHP>100000.f?100000.f:MaxHP;
    const int32 Cost=static_cast<int32>(std::ceil(Cap*0.22f));
    return Cost>12?Cost:12;
}
bool FAVShrineTrials::CanPact(float HP,float MaxHP)
{
    return HP>=static_cast<float>(PactCost(MaxHP))+1.f;
}
bool FAVShrineTrials::Valid(const FAVShrineState& State)
{
    if(State.Streak<0||State.Streak>MaxStreak)return false;
    switch(State.Choice)
    {
    case AVShrineUnchosen:
    case AVShrineRest:
    case AVShrinePact:
        return State.Remaining==0&&!State.bRewarded;
    case AVShrineTrial:
        return State.Remaining>=0&&State.Remaining<=MaxUnits &&
               (State.bRewarded==(State.Remaining==0));
    default:return false;
    }
}
bool FAVShrineTrials::Choose(FAVShrineState& State,int32 Choice,int32 Spawned,float HP,float MaxHP)
{
    if(!Valid(State)||State.Choice!=AVShrineUnchosen)return false;
    if(Choice==AVShrineTrial && (Spawned<1||Spawned>MaxUnits))return false;
    if(Choice==AVShrinePact && !CanPact(HP,MaxHP))return false;
    if(Choice!=AVShrineRest&&Choice!=AVShrineTrial&&Choice!=AVShrinePact)return false;
    State.Choice=Choice;
    State.Remaining=Choice==AVShrineTrial?Spawned:0;
    State.bRewarded=false;
    if(Choice!=AVShrineTrial)State.Streak=0;
    return true;
}
bool FAVShrineTrials::IsTrialEnemy(int32 Id)
{
    return Id<=-1101&&Id>=-1104;
}
int32 FAVShrineTrials::TrialEnemyId(int32 Index)
{
    return Index>=0&&Index<MaxUnits?-1101-Index:0;
}
int32 FAVShrineTrials::RewardGold(int32 Floor,int32 Streak)
{
    const int32 Depth=Floor<1?1:Floor>1000?1000:Floor;
    const int32 Chain=Streak<0?0:Streak>MaxStreak?MaxStreak:Streak;
    return 50+Depth*9+Chain*20;
}
int32 FAVShrineTrials::OnEnemyDefeated(FAVShrineState& State,int32 EnemyId,int32 Floor)
{
    if(!Valid(State)||State.Choice!=AVShrineTrial||State.bRewarded||
       !IsTrialEnemy(EnemyId))return 0;
    if(--State.Remaining==0)
    {
        State.bRewarded=true;
        if(State.Streak<MaxStreak)++State.Streak;
        return RewardGold(Floor,State.Streak);
    }
    return 0;
}
void FAVShrineTrials::NextFloor(FAVShrineState& State)
{
    // The chain continues only through consecutive floors cleared by a trial.
    const int32 Chain=Valid(State)&&State.Choice==AVShrineTrial&&
        State.bRewarded?State.Streak:0;
    State=FAVShrineState{};
    State.Streak=Chain;
}
int32 FAVShrineTrials::DamageBonusPercent(const FAVShrineState& State)
{
    return Valid(State)&&State.Choice==AVShrinePact?18:0;
}
