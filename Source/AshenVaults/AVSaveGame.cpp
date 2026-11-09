#include "AVSaveGame.h"

#include "AVEquipment.h"
#include "AVSaveRules.h"
#include "AVSkills.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    const TCHAR* PrimarySaveSlot = TEXT("AshenVaultsRecord");
    const TCHAR* BackupSaveSlot = TEXT("AshenVaultsRecord_Backup");

    bool Finite(float Value)
    {
        return AVSaveRules::AreFinite(&Value,1);
    }

    bool FiniteArray(const TArray<float>& Values)
    {
        return AVSaveRules::AreFinite(Values.GetData(),Values.Num());
    }

    bool Packed(const TArray<float>& Values,int32 Stride,int32 MaxRecords)
    {
        return AVSaveRules::IsPackedArrayValid(Values.Num(),Stride,MaxRecords) &&
            FiniteArray(Values);
    }
}

bool UAVSaveGame::IsStructurallyValid() const
{
    if(SaveSchemaVersion<0 ||
       SaveSchemaVersion>AVSaveRules::CurrentSchemaVersion ||
       CheckpointSchemaVersion<0 ||
       CheckpointSchemaVersion>AVSaveRules::CurrentSchemaVersion)
        return false;
    if(BestFloor<1 || BestFloor>AVSaveRules::MaxFloor ||
       BestKills<0 || TotalGold<0)
        return false;
    if(!bHasCheckpoint) return true;

    if(!AVSaveRules::IsValidCheckpointHeader(
        CheckpointFloor,CheckpointHeroClass,CheckpointLevel,CheckpointXP,
        CheckpointGold,CheckpointKills,CheckpointPotions,
        CheckpointMapWidth,CheckpointMapHeight))
    {
        // Older saves did not always store the map dimensions that the
        // current four-biome cycle expects. Keep known legacy map sizes
        // readable; the world loader regenerates a mismatched old floor.
        const bool bLegacyMap =
            (CheckpointMapWidth==52 && CheckpointMapHeight==38) ||
            (CheckpointMapWidth==60 && CheckpointMapHeight==44);
        if(CheckpointSchemaVersion>=AVSaveRules::CurrentSchemaVersion ||
           !bLegacyMap || CheckpointFloor<1 ||
           CheckpointFloor>AVSaveRules::MaxFloor ||
           CheckpointHeroClass<0 || CheckpointHeroClass>=4 ||
           CheckpointLevel<1 || CheckpointLevel>AVSaveRules::MaxFloor ||
           CheckpointXP<0 || CheckpointGold<0 || CheckpointKills<0 ||
           CheckpointPotions<0 || CheckpointPotions>999)
            return false;
    }

    if(CheckpointSchemaVersion>=AVSaveRules::CurrentSchemaVersion &&
       (!bCheckpointV09Data || !bCheckpointV13Data || !bCheckpointV15Data))
        return false;
    if((bCheckpointV15Data && (!bCheckpointV13Data || !bCheckpointV09Data)) ||
       (bCheckpointV13Data && !bCheckpointV09Data))
        return false;

    const int32 ExpectedMapCells=CheckpointMapWidth*CheckpointMapHeight;
    if((CheckpointTiles.Num()!=0 && CheckpointTiles.Num()!=ExpectedMapCells) ||
       (CheckpointExplored.Num()!=0 && CheckpointExplored.Num()!=ExpectedMapCells))
        return false;
    if(CheckpointTiles.Num()>60*44 || CheckpointExplored.Num()>60*44)
        return false;

    if(CheckpointInventoryItems.Num()>48 ||
       (CheckpointEquipmentSlots.Num()!=0 && CheckpointEquipmentSlots.Num()!=8) ||
       (CheckpointSkillRanks.Num()!=0 && CheckpointSkillRanks.Num()!=FAVSkills::Count) ||
       (bCheckpointV09Data &&
        (CheckpointEquipmentSlots.Num()!=8 ||
         CheckpointSkillRanks.Num()!=FAVSkills::Count)))
        return false;
    if(CheckpointSkillPoints<0 || CheckpointSkillPoints>29 ||
       (CheckpointEquippedMask&~255)!=0 || (CheckpointRelics&~63u)!=0)
        return false;

    for(int32 Id:CheckpointInventoryItems)
        if(!FAVEquipment::Find(Id)) return false;
    for(int32 Id:CheckpointEquipmentSlots)
        if(Id!=-1 && (!FAVEquipment::Find(Id) ||
                      !FAVEquipment::CanEquip(Id,CheckpointHeroClass)))
            return false;
    for(int32 I=0;I<CheckpointSkillRanks.Num();++I)
    {
        const FAVSkillSpec* Skill=FAVSkills::Find(I);
        if(!Skill || CheckpointSkillRanks[I]>Skill->MaxRank)
            return false;
    }

    const int32 EnemyStride=AVSaveRules::EnemyStride(
        bCheckpointV09Data,bCheckpointV13Data,bCheckpointV15Data);
    if(!Packed(CheckpointEnemyData,EnemyStride,AVSaveRules::MaxEnemies) ||
       !Packed(CheckpointRoomEventData,15,AVSaveRules::MaxRoomEvents) ||
       !Packed(CheckpointPickupData,5,AVSaveRules::MaxPickups) ||
       !Packed(CheckpointChestData,4,AVSaveRules::MaxChests) ||
       !Packed(CheckpointTrapData,5,AVSaveRules::MaxTraps) ||
       !Packed(CheckpointHazardData,4,AVSaveRules::MaxHazards) ||
       !Packed(CheckpointPropData,4,AVSaveRules::MaxProps) ||
       !Packed(CheckpointDecorData,5,AVSaveRules::MaxDecorations) ||
       !Packed(CheckpointCampDoorData,2,AVSaveRules::MaxCampDoors) ||
       !Packed(CheckpointCampWallData,2,AVSaveRules::MaxCampWalls))
        return false;

    const float Scalars[]={
        CheckpointHP,CheckpointMaxHP,CheckpointStamina,CheckpointMaxStamina,
        CheckpointAttack,CheckpointSpeed,CheckpointArmor,CheckpointRanged,
        CheckpointFoodAttackTime,CheckpointFoodGuardTime,CheckpointFoodSpeedTime,
        CheckpointFoodCritTime,CheckpointFoodProjectileTime,CheckpointFoodBossTime,
        CheckpointFoodPoisonWardTime,CheckpointHero.X,CheckpointHero.Y,
        CheckpointExit.X,CheckpointExit.Y,CheckpointShrine.X,CheckpointShrine.Y,
        CheckpointMerchant.X,CheckpointMerchant.Y,CheckpointSafeCampDoor.X,
        CheckpointSafeCampDoor.Y
    };
    if(!AVSaveRules::AreFinite(Scalars,static_cast<int32>(sizeof(Scalars)/sizeof(Scalars[0]))))
        return false;
    if(CheckpointMaxHP<=0.f || CheckpointMaxStamina<=0.f ||
       CheckpointHP<0.f || CheckpointHP>CheckpointMaxHP+0.01f ||
       CheckpointStamina<0.f || CheckpointStamina>CheckpointMaxStamina+0.01f ||
       CheckpointAttack<=0.f || CheckpointSpeed<=0.f ||
       CheckpointFoodAttackTime<0.f || CheckpointFoodGuardTime<0.f ||
       CheckpointFoodSpeedTime<0.f || CheckpointFoodCritTime<0.f ||
       CheckpointFoodProjectileTime<0.f || CheckpointFoodBossTime<0.f ||
       CheckpointFoodPoisonWardTime<0.f)
        return false;
    return true;
}

UAVSaveGame* UAVSaveGame::LoadBestAvailable()
{
    UAVSaveGame* Primary=Cast<UAVSaveGame>(
        UGameplayStatics::LoadGameFromSlot(PrimarySaveSlot,0));
    if(Primary && Primary->IsStructurallyValid())
        return Primary;

    UAVSaveGame* Backup=Cast<UAVSaveGame>(
        UGameplayStatics::LoadGameFromSlot(BackupSaveSlot,0));
    if(Backup && Backup->IsStructurallyValid())
    {
        const bool bRestored=UGameplayStatics::SaveGameToSlot(
            Backup,PrimarySaveSlot,0);
        UE_LOG(LogTemp,Warning,
            TEXT("ASHEN VAULTS: primary save %s; valid backup recovered (restore=%s)."),
            Primary?TEXT("failed validation"):TEXT("unavailable"),
            bRestored?TEXT("success"):TEXT("memory-only"));
        return Backup;
    }
    if(Primary)
        UE_LOG(LogTemp,Error,TEXT("ASHEN VAULTS: primary save failed structural validation and no valid backup exists."));
    else if(UGameplayStatics::DoesSaveGameExist(BackupSaveSlot,0))
        UE_LOG(LogTemp,Error,TEXT("ASHEN VAULTS: backup save also failed structural validation."));
    return nullptr;
}

bool UAVSaveGame::SaveWithBackup(UAVSaveGame* Save)
{
    if(!Save) return false;
    Save->SaveSchemaVersion=AVSaveRules::CurrentSchemaVersion;
    if(Save->bHasCheckpoint)
        Save->CheckpointSchemaVersion=AVSaveRules::CurrentSchemaVersion;
    else
        Save->CheckpointSchemaVersion=0;
    if(!Save->IsStructurallyValid())
    {
        UE_LOG(LogTemp,Error,TEXT("ASHEN VAULTS: refused to write structurally invalid save data."));
        return false;
    }

    UAVSaveGame* Previous=Cast<UAVSaveGame>(
        UGameplayStatics::LoadGameFromSlot(PrimarySaveSlot,0));
    UAVSaveGame* ExistingBackup=Cast<UAVSaveGame>(
        UGameplayStatics::LoadGameFromSlot(BackupSaveSlot,0));
    if(Previous && Previous->IsStructurallyValid())
    {
        if(!UGameplayStatics::SaveGameToSlot(Previous,BackupSaveSlot,0))
        {
            UE_LOG(LogTemp,Error,TEXT("ASHEN VAULTS: could not write backup; primary save was left untouched."));
            return false;
        }
    }
    else if(ExistingBackup && ExistingBackup->IsStructurallyValid())
    {
        // Preserve the last known-good backup while replacing a corrupt primary.
    }

    if(UGameplayStatics::SaveGameToSlot(Save,PrimarySaveSlot,0))
        return true;

    if(ExistingBackup && ExistingBackup->IsStructurallyValid())
        UGameplayStatics::SaveGameToSlot(ExistingBackup,PrimarySaveSlot,0);
    else if(Previous && Previous->IsStructurallyValid())
        UGameplayStatics::SaveGameToSlot(Previous,PrimarySaveSlot,0);
    UE_LOG(LogTemp,Error,TEXT("ASHEN VAULTS: primary save write failed; attempted recovery from the previous copy."));
    return false;
}
