#pragma once
#include "CoreMinimal.h"

// Optional objectives bound to the current generated floor; IDs are stable in SaveGame.
enum EAVContractKind : int32
{
    AVContractNone=0, AVContractCull=1, AVContractElite=2,
    AVContractChest=3, AVContractEcho=4
};

struct FAVContractState
{
    int32 Kind=AVContractNone;
    int32 Target=0;
    int32 Progress=0;
    int32 RewardGold=0;
    bool bPaid=false;
};

class FAVContracts
{
public:
    // Chooses only from objectives with enough targets generated on this floor.
    // Does not consume the dungeon or loot random stream.
    static FAVContractState Offer(int32 Seed,int32 Floor,int32 Hostiles,
                                  int32 Elites,int32 Chests,int32 Echoes);
    // Returns gold to pay exactly once; callers own their currency state.
    static int32 Record(FAVContractState& State,int32 EventKind);
    static bool Valid(const FAVContractState& State);
    static const char* Name(int32 Kind);
};
