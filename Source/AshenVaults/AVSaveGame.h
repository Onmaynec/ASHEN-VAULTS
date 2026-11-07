#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "AVSaveGame.generated.h"

/** Local single-player record for the first vertical slice. */
UCLASS()
class ASHENVAULTS_API UAVSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(SaveGame)
    int32 BestFloor = 1;

    UPROPERTY(SaveGame)
    int32 BestKills = 0;

    UPROPERTY(SaveGame)
    int32 TotalGold = 0;
};
