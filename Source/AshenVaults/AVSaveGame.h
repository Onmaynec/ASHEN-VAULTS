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
    UPROPERTY(SaveGame) bool bHasCheckpoint = false;
    UPROPERTY(SaveGame) int32 CheckpointFloor = 1;
    UPROPERTY(SaveGame) int32 CheckpointSeed = 0;
    UPROPERTY(SaveGame) int32 CheckpointHeroClass = 0;
    UPROPERTY(SaveGame) int32 CheckpointLevel = 1;
    UPROPERTY(SaveGame) int32 CheckpointXP = 0;
    UPROPERTY(SaveGame) int32 CheckpointGold = 0;
    UPROPERTY(SaveGame) int32 CheckpointKills = 0;
    UPROPERTY(SaveGame) int32 CheckpointPotions = 2;
    UPROPERTY(SaveGame) int32 CheckpointGear = 0;
    UPROPERTY(SaveGame) int32 CheckpointWeaponUpgrade = 0;
    UPROPERTY(SaveGame) int32 CheckpointArmorUpgrade = 0;
    UPROPERTY(SaveGame) uint32 CheckpointRelics = 0;
    UPROPERTY(SaveGame) float CheckpointHP = 100.f;
    UPROPERTY(SaveGame) float CheckpointMaxHP = 100.f;
    UPROPERTY(SaveGame) float CheckpointStamina = 100.f;
    UPROPERTY(SaveGame) float CheckpointMaxStamina = 100.f;
    UPROPERTY(SaveGame) float CheckpointAttack = 18.f;
    UPROPERTY(SaveGame) float CheckpointSpeed = 4.8f;
    UPROPERTY(SaveGame) float CheckpointArmor = 0.f;
    UPROPERTY(SaveGame) float CheckpointRanged = 0.f;
};
