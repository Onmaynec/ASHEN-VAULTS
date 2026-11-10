#pragma once
#include "CoreMinimal.h"

// An optional choice at each floor shrine. Choice and streak are stored in SaveGame.
enum EAVShrineChoice : int32
{
    AVShrineUnchosen=0, AVShrineRest=1, AVShrineTrial=2, AVShrinePact=3
};
struct FAVShrineState
{
    int32 Choice=AVShrineUnchosen;
    int32 Remaining=0;
    int32 Streak=0; // consecutive successful trials, capped at three
    bool bRewarded=false;
};
class FAVShrineTrials
{
public:
    static constexpr int32 MaxStreak=3;
    static constexpr int32 MaxUnits=4;
    static int32 UnitsForFloor(int32 Floor);
    static int32 PactCost(float MaxHP);
    static bool CanPact(float HP,float MaxHP);
    static bool Valid(const FAVShrineState& State);
    static bool Choose(FAVShrineState& State,int32 Choice,int32 Spawned,float HP,float MaxHP);
    static bool IsTrialEnemy(int32 Id);
    static int32 TrialEnemyId(int32 Index);
    static int32 RewardGold(int32 Floor,int32 Streak);
    // Returns payout exactly once after the final identified trial enemy dies.
    static int32 OnEnemyDefeated(FAVShrineState& State,int32 EnemyId,int32 Floor);
    static void NextFloor(FAVShrineState& State);
    static int32 DamageBonusPercent(const FAVShrineState& State);
};
