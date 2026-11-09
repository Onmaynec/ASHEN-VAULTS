#include "AVGameMode.h"
#include "AVEquipment.h"
#include "AVItemCombat.h"
#include "AVSkills.h"
#include "AVFortressLayout.h"
#include "AVHUD.h"
#include "AVFrontendWidget.h"
#include "Blueprint/UserWidget.h"
#include "AVSaveGame.h"
#include "AVDungeonNavigation.h"
#include "AVContent.h"
#include "AVEncounters.h"
#include "AVDepthDirector.h"
#include "AVRoomTactics.h"
#include "AVAffixes.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    constexpr float TwoPi = 6.283185307f;
    bool Pressed(APlayerController* PC, const FKey& Key, bool& Previous)
    {
        const bool Down = PC && PC->IsInputKeyDown(Key);
        const bool Result = Down && !Previous;
        Previous = Down;
        return Result;
    }
    FVector2D SafeDirection(const FVector2D& Value)
    {
        const float Length = Value.Size();
        return Length > KINDA_SMALL_NUMBER ? Value / Length : FVector2D::ZeroVector;
    }
}

AAVGameMode::AAVGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    HUDClass = AAVHUD::StaticClass();
    RNG.Initialize(73921);
}

void AAVGameMode::BeginPlay()
{
    Super::BeginPlay();
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        PC->bShowMouseCursor = true;
        PC->bEnableClickEvents = true;
        PC->SetInputMode(FInputModeGameAndUI());
    }
    if (USaveGame* Existing = UGameplayStatics::LoadGameFromSlot(TEXT("AshenVaultsRecord"), 0))
    {
        if (UAVSaveGame* Save = Cast<UAVSaveGame>(Existing))
        {
            BestFloor = Save->BestFloor;
            BestKills = Save->BestKills;
            LifetimeGold = Save->TotalGold;
        }
    }
    // Construct the initial state without deleting a saved run during boot.
    NewRun(false, true);
    if (UAVSaveGame* Save = Cast<UAVSaveGame>(
        UGameplayStatics::LoadGameFromSlot(TEXT("AshenVaultsRecord"), 0)))
    {
        if (Save->bHasCheckpoint)
        {
            bCanContinue = true;
            Floor = FMath::Max(1, Save->CheckpointFloor);
            MapW = CurrentBiome() == 3 ? 60 : 52;
            MapH = CurrentBiome() == 3 ? 44 : 38;
            RunSeed = Save->CheckpointSeed;
            HeroClass = Save->CheckpointHeroClass;
            Level = Save->CheckpointLevel; XP = Save->CheckpointXP;
            Gold = Save->CheckpointGold; Kills = Save->CheckpointKills;
            Potions = Save->CheckpointPotions; EquippedGear = Save->CheckpointGear;
            EquippedSlotsMask = Save->CheckpointEquippedMask;
            WeaponUpgrade = Save->CheckpointWeaponUpgrade;
            ArmorUpgrade = Save->CheckpointArmorUpgrade;
            RelicMask = Save->CheckpointRelics;
            RelicCount = 0; for (int32 I = 0; I < 6; ++I) if (HasRelic(I)) ++RelicCount;
            HP = Save->CheckpointHP; MaxHP = Save->CheckpointMaxHP;
            Stamina = Save->CheckpointStamina; MaxStamina = Save->CheckpointMaxStamina;
            AttackPower = Save->CheckpointAttack; MoveSpeed = Save->CheckpointSpeed;
            ArmorRating = Save->CheckpointArmor; RangedBonus = Save->CheckpointRanged;
            EquipmentSlots.Init(-1,8); InventoryItems.Reset();
            LearnedSkills.Init(0,FAVSkills::Count);
            SkillPoints=FAVSkills::AwardedPointsAtLevel(Level);
            if (Save->bCheckpointV09Data) {
                if(Save->CheckpointEquipmentSlots.Num()==8)
                    EquipmentSlots=Save->CheckpointEquipmentSlots;
                for(int32 Id : Save->CheckpointInventoryItems)
                    if(FAVEquipment::Find(Id) && InventoryItems.Num()<48)
                        InventoryItems.Add(Id);
                if(Save->CheckpointSkillRanks.Num()==FAVSkills::Count)
                    LearnedSkills=Save->CheckpointSkillRanks;
                SkillPoints=FMath::Clamp(Save->CheckpointSkillPoints,0,29);
                bFreeRespecUsed=Save->bCheckpointFreeRespecUsed;
                bLastStandUsed=Save->bCheckpointLastStandUsed;
                bMagicBarrierUsed=Save->bCheckpointMagicBarrierUsed;
                if(Save->bCheckpointV15Data)
                {
                    FoodAttackTime=Save->CheckpointFoodAttackTime;
                    FoodGuardTime=Save->CheckpointFoodGuardTime;
                    FoodSpeedTime=Save->CheckpointFoodSpeedTime;
                    FoodCritTime=Save->CheckpointFoodCritTime;
                    FoodProjectileTime=Save->CheckpointFoodProjectileTime;
                    FoodBossTime=Save->CheckpointFoodBossTime;
                    FoodPoisonWardTime=Save->CheckpointFoodPoisonWardTime;
                }
            }
            for(int32& Id : EquipmentSlots)
                if(Id>=0 && !FAVEquipment::CanEquip(Id,HeroClass)) Id=-1;
            RebuildCharacterBonuses();
            // Do not overwrite the existing checkpoint while booting the lobby.
            bInLobby = true;
            bInSafeCamp = false;
            bWorldStored = false;
            MakeSafeRoom(true);
            Toast = TEXT("HOME: PRESS E AT THE PORTAL TO RESUME");
            ToastTime = 3.f;
        }
    }
    Phase = EAVPhase::MainMenu;
    if(APlayerController* PC=UGameplayStatics::GetPlayerController(this,0))
    {
        FrontendWidget=CreateWidget<UAVFrontendWidget>(PC,UAVFrontendWidget::StaticClass());
        if(FrontendWidget)
        {
            FrontendWidget->AddToViewport(100);
            FInputModeUIOnly Mode;
            PC->SetInputMode(Mode);
            PC->bShowMouseCursor=true;
        }
    }
}

void AAVGameMode::StartFromMenu(bool bContinue,int32 SelectedHero)
{
    if(bContinue && bCanContinue)
    {
        Phase=EAVPhase::Playing;
        Toast=TEXT("DESCEND FROM THE UNDERGROUND CITY");
        ToastTime=3.f;
    }
    else
    {
        HeroClass=FMath::Clamp(SelectedHero,0,3);
        NewRun();
        Toast=TEXT("THE DESCENT BEGINS");
        ToastTime=2.f;
    }
    if(APlayerController* PC=UGameplayStatics::GetPlayerController(this,0))
    {
        PC->SetInputMode(FInputModeGameAndUI());
        PC->bShowMouseCursor=true;
    }
}

bool AAVGameMode::IsFloor(int32 X, int32 Y) const
{
    return X >= 0 && Y >= 0 && X < MapW && Y < MapH &&
        Tiles.IsValidIndex(Y * MapW + X) && Tiles[Y * MapW + X] != 0;
}

bool AAVGameMode::IsVisibleCell(int32 X, int32 Y) const
{
    return X >= 0 && Y >= 0 && X < MapW && Y < MapH &&
        Visible.IsValidIndex(Y * MapW + X) && Visible[Y * MapW + X] != 0;
}

bool AAVGameMode::IsExploredCell(int32 X, int32 Y) const
{
    return X >= 0 && Y >= 0 && X < MapW && Y < MapH &&
        Explored.IsValidIndex(Y * MapW + X) && Explored[Y * MapW + X] != 0;
}

bool AAVGameMode::IsVisibleAt(const FVector2D& Pos) const
{
    return IsVisibleCell(FMath::FloorToInt(Pos.X), FMath::FloorToInt(Pos.Y));
}

int32 AAVGameMode::UnopenedChests() const
{
    int32 Count = 0;
    for (const FAVChest& Chest : Chests)
        if (!Chest.bOpened) ++Count;
    return Count;
}

void AAVGameMode::RefreshAwareness(float DeltaSeconds, bool bForce)
{
    if (Tiles.Num() != MapW * MapH) return;
    const int32 X = FMath::FloorToInt(Hero.X);
    const int32 Y = FMath::FloorToInt(Hero.Y);
    if (bForce || X != LastVisionX || Y != LastVisionY)
    {
        FAVDungeonNavigation::Reveal(Tiles, MapW, MapH, Hero, 8.5f, Visible, Explored);
        LastVisionX = X;
        LastVisionY = Y;
    }
    FlowRefreshTime -= DeltaSeconds;
    if (bForce || FlowRefreshTime <= 0.f)
    {
        FAVDungeonNavigation::BuildFlow(Tiles, MapW, MapH, Hero, FlowDistances);
        FlowRefreshTime = 0.22f;
    }
}

bool AAVGameMode::CanMove(const FVector2D& Pos, float Radius) const
{
    return IsFloor(FMath::FloorToInt(Pos.X - Radius), FMath::FloorToInt(Pos.Y - Radius)) &&
           IsFloor(FMath::FloorToInt(Pos.X + Radius), FMath::FloorToInt(Pos.Y - Radius)) &&
           IsFloor(FMath::FloorToInt(Pos.X - Radius), FMath::FloorToInt(Pos.Y + Radius)) &&
           IsFloor(FMath::FloorToInt(Pos.X + Radius), FMath::FloorToInt(Pos.Y + Radius));
}

FVector2D AAVGameMode::MoveWithCollision(const FVector2D& From, const FVector2D& Velocity, float DeltaSeconds, float Radius) const
{
    FVector2D Next = From;
    const float StepX = Velocity.X * DeltaSeconds;
    const float StepY = Velocity.Y * DeltaSeconds;
    const int32 Steps = FMath::Max(1, FMath::CeilToInt(FMath::Max(FMath::Abs(StepX), FMath::Abs(StepY)) / 0.18f));
    for (int32 I = 0; I < Steps; ++I)
    {
        FVector2D Candidate(Next.X + StepX / Steps, Next.Y);
        if (CanMove(Candidate, Radius)) Next.X = Candidate.X;
        Candidate = FVector2D(Next.X, Next.Y + StepY / Steps);
        if (CanMove(Candidate, Radius)) Next.Y = Candidate.Y;
    }
    return Next;
}


int32 AAVGameMode::SkillIdAt(int32 Selection) const
{
    return Selection >= 0 && Selection < 15 ? Selection :
           Selection >= 15 && Selection < 21 ? 15 + FMath::Clamp(HeroClass, 0, 3) * 6 + Selection - 15 : -1;
}

int32 AAVGameMode::SkillRank(int32 Id) const
{
    return LearnedSkills.IsValidIndex(Id) ? int32(LearnedSkills[Id]) : 0;
}

float AAVGameMode::GearStat(int32 Stat) const
{
    float Result = 0.f;
    int32 Sets[6] = {0,0,0,0,0,0};
    for (int32 ItemId : EquipmentSlots)
    {
        const FAVEquipmentSpec* I = FAVEquipment::Find(ItemId);
        if (!I) continue;
        if (I->StatA == Stat) Result += I->ValueA;
        if (I->StatB == Stat) Result += I->ValueB;
        if (I->SetId > 0 && I->SetId <= 5) ++Sets[I->SetId];
    }
    // Each thematic set has one 3-piece bonus, regardless of the other sets.
    if (Stat == AVArmorPercent) Result += Sets[1] >= 3 ? 5.f : 0.f;
    if (Stat == AVAttackSpeedPercent) Result += Sets[2] >= 3 ? 8.f : 0.f;
    if (Stat == AVAbilityDamagePercent) Result += Sets[3] >= 3 ? 8.f : 0.f;
    if (Stat == AVRangedDamagePercent) Result += Sets[4] >= 3 ? 10.f : 0.f;
    if (Stat == AVHealthFlat) Result += Sets[5] >= 3 ? 20.f : 0.f;
    return Result;
}

bool AAVGameMode::HasGearPerk(int32 Perk) const
{
    for (int32 Id : EquipmentSlots)
    {
        const FAVEquipmentSpec* I = FAVEquipment::Find(Id);
        if (I && I->PerkId == Perk) return true;
    }
    return false;
}

FString AAVGameMode::GearName(int32 ItemId) const
{
    const FAVEquipmentSpec* S = FAVEquipment::Find(ItemId);
    if (!S) return TEXT("EMPTY");
    FString Name = FString(UTF8_TO_TCHAR(S->DisplayName?S->DisplayName:S->CodeName));
    if(!S->DisplayName)Name.ReplaceInline(TEXT("_"),TEXT(" "));
    const int32 Affix=FAVEquipment::AffixId(ItemId);
    if(Affix>0)
        Name+=TEXT(" [")+FString(UTF8_TO_TCHAR(FAVEquipment::AffixName(Affix)))+TEXT("]");
    return Name.ToUpper();
}

void AAVGameMode::RebuildCharacterBonuses()
{
    // Base attributes continue to contain the old level, forge, and relic bonuses.
    // Remove the previous equipment/skill delta first; never stack on re-equip.
    const float BaseHP = MaxHP - EquipmentAppliedHP;
    const float BaseStamina = MaxStamina - EquipmentAppliedStamina;
    const float BaseAttack = AttackPower - EquipmentAppliedAttack;
    const float BaseSpeed = MoveSpeed - EquipmentAppliedSpeed;
    const float BaseArmor = ArmorRating - EquipmentAppliedArmor;
    EquipmentAppliedHP = GearStat(AVHealthFlat) + BaseHP * (0.06f * SkillRank(0));
    EquipmentAppliedStamina = GearStat(AVStaminaFlat) + 10.f * SkillRank(11);
    // Hero-specific skill 30 adds another stamina tier.
    if (HeroClass == 2) EquipmentAppliedStamina += 10.f * SkillRank(30);
    EquipmentAppliedAttack = GearStat(AVAttackFlat);
    EquipmentAppliedSpeed = BaseSpeed * (GearStat(AVMoveSpeedPercent) +
        3.f * SkillRank(10)) * 0.01f;
    EquipmentAppliedArmor = FMath::Min(0.45f, BaseArmor +
        (GearStat(AVArmorPercent) + 2.f * SkillRank(15)) * 0.01f) - BaseArmor;
    MaxHP = BaseHP + EquipmentAppliedHP;
    MaxStamina = BaseStamina + EquipmentAppliedStamina;
    AttackPower = BaseAttack + EquipmentAppliedAttack;
    MoveSpeed = BaseSpeed + EquipmentAppliedSpeed;
    ArmorRating = FMath::Clamp(BaseArmor + EquipmentAppliedArmor, 0.f, 0.45f);
    HP = FMath::Min(HP, MaxHP);
    Stamina = FMath::Min(Stamina, MaxStamina);
}

int32 AAVGameMode::EquippedWeaponPattern() const
{
    if(!EquipmentSlots.IsValidIndex(0))return 0;
    const FAVEquipmentSpec* W=FAVEquipment::Find(EquipmentSlots[0]);
    return W&&W->Slot==AVWeapon?W->Pattern:0;
}
float AAVGameMode::StatusResistanceFactor(bool bPoison) const
{
    const float Special=GearStat(bPoison?AVPoisonResistPercent:AVBurnResistPercent);
    const float Ward=bPoison&&FoodPoisonWardTime>0.f?35.f:0.f;
    return FMath::Clamp(1.f-
        (GearStat(AVStatusResistPercent)+Special+Ward+10.f*SkillRank(1))*0.01f,
        0.20f,1.f);
}
float AAVGameMode::BasicAttackInterval() const
{
    return FAVContent::BasicAttackInterval(
        HeroClass, GearStat(AVAttackSpeedPercent));
}

float AAVGameMode::ProjectileSpeedMultiplier() const
{
    const float Skill = HeroClass == 3 ? 7.f * SkillRank(33) : 0.f;
    return FMath::Clamp(1.f + (GearStat(AVProjectileSpeedPercent) + Skill +
        (HasGearPerk(4) ? 10.f : 0.f) +
        (FoodProjectileTime>0.f?20.f:0.f)) * 0.01f, 0.8f, 1.8f);
}

float AAVGameMode::ModifiedAttackDamage(float Damage, const FAVEnemy& Target,
                                       bool bRanged, bool bAbility)
{
    float Percent = bRanged ? GearStat(AVRangedDamagePercent) : GearStat(AVMeleeDamagePercent);
    Percent += bAbility ? GearStat(AVAbilityDamagePercent) + 6.f * SkillRank(27) :
                          4.f * SkillRank(5);
    if(FoodAttackTime>0.f)Percent+=12.f;
    if(FoodBossTime>0.f&&(Target.bMiniBoss||FAVContent::Enemy(Target.Type,Floor).Boss))
        Percent+=14.f;
    if (bAbility && HeroClass == 1 && !bRanged) Percent += 10.f * SkillRank(23);
    if (bAbility && HeroClass == 1 && bRanged) Percent += 12.f * SkillRank(22);
    if (Target.bMiniBoss || FAVContent::Enemy(Target.Type, Floor).Boss)
        Percent += GearStat(AVBossDamagePercent) + 8.f * SkillRank(8);
    if (Target.HP < Target.MaxHP * 0.20f && HasGearPerk(8)) Percent += 12.f;
    if (Target.HP < Target.MaxHP * 0.30f && HasGearPerk(2)) Percent += 20.f;
    if (HeroClass == 1 && Target.HP < Target.MaxHP * 0.25f) Percent += 28.f * SkillRank(26);
    if (HeroClass == 3 && Target.Id == ShotTargetId && bRanged)
        Percent += 5.f * SkillRank(34);
    if (RewardDamageTime > 0.f) Percent += 10.f;
    if (RevengeTime > 0.f) Percent += 8.f * SkillRank(18);
    if (!bAbility && (++BasicAttackSequence % 4) == 0) Percent += 7.f * SkillRank(7);
    if(HeroClass==1 && DashBonusTime>0.f) Percent+=3.f*SkillRank(21);
    if(HeroClass==1 && LastKillBoostTime>0.f) Percent+=6.f*SkillRank(24);
    const bool GuaranteedCrit=HeroClass==3 && SkillRank(38)>0 &&
        ShotSeries>=4 && bRanged && !bAbility;
    if(GuaranteedCrit) ShotSeries=0;
    const float CritChance=FMath::Clamp(0.05f+
        GearStat(AVCriticalPercent)*0.01f+0.03f*SkillRank(6)+
        (FoodCritTime>0.f?0.12f:0.f)+
        (HeroClass==1 && DashBonusTime>0.f ? 0.03f*SkillRank(21):0.f),
        0.f,0.40f);
    const bool bCritical=GuaranteedCrit || RNG.FRand()<CritChance;
    if(bCritical) Percent+=50.f;
    if(bCritical && HeroClass==2 && SkillRank(31)>0 && ChainCooldown<=0.f) {
        int32 JumpCount=0;
        for(FAVEnemy& Other:Enemies)
            if(Other.Id!=Target.Id && Other.HP>0.f &&
               FVector2D::Distance(Other.Pos,Target.Pos)<3.f) {
                Other.HP-=FMath::Max(1.f,Damage*0.28f);
                Other.Flash=0.15f;
                if(++JumpCount>=2)break;
            }
        ChainCooldown=1.5f;
    }
    const float Effective=FMath::Max(1.f,Damage*(1.f+Percent*0.01f));
    const int32 Weapon=EquipmentSlots.IsValidIndex(0)?EquipmentSlots[0]:-1;
    return Effective+FAVEquipment::BonusAgainst(Weapon,CurrentBiome(),
        Target.bElite,Target.bMiniBoss||FAVContent::Enemy(Target.Type,Floor).Boss);
}

void AAVGameMode::UseConsumable(int32 Index)
{
    if(!InventoryItems.IsValidIndex(Index))return;
    const int32 Id=InventoryItems[Index];
    const FAVEquipmentSpec* S=FAVEquipment::Find(Id);
    if(!S||S->Slot!=AVConsumable)return;
    switch(S->PerkId)
    {
    case 10:if(HP>=MaxHP)return;HP=FMath::Min(MaxHP,HP+S->ValueA);break;
    case 11:if(Stamina>=MaxStamina)return;Stamina=FMath::Min(MaxStamina,Stamina+S->ValueA);break;
    case 12:if(PoisonTime<=0.f&&BurnTime<=0.f)return;
        PoisonTime=0.f;BurnTime=0.f;StatusTick=0.f;break;
    case 13:PoisonTime=0.f;FoodPoisonWardTime=FMath::Max(FoodPoisonWardTime,S->ValueA);break;
    case 14:FoodAttackTime=FMath::Max(FoodAttackTime,S->ValueA);break;
    case 15:FoodGuardTime=FMath::Max(FoodGuardTime,S->ValueA);break;
    case 16:FoodSpeedTime=FMath::Max(FoodSpeedTime,S->ValueA);break;
    case 17:FoodCritTime=FMath::Max(FoodCritTime,S->ValueA);break;
    case 18:FoodProjectileTime=FMath::Max(FoodProjectileTime,S->ValueA);break;
    case 19:FoodBossTime=FMath::Max(FoodBossTime,S->ValueA);break;
    default:return;
    }
    InventoryItems.RemoveAt(Index);
    GearSelection=FMath::Clamp(GearSelection,0,FMath::Max(0,InventoryItems.Num()-1));
    Toast=TEXT("USED: ")+GearName(Id);ToastTime=2.f;
    AddEffect(Hero,2,0.8f);
}

void AAVGameMode::EquipInventoryItem(int32 Index)
{
    if (!InventoryItems.IsValidIndex(Index)) return;
    const int32 ItemId = InventoryItems[Index];
    if(FAVEquipment::IsConsumable(ItemId)){UseConsumable(Index);return;}
    if (!FAVEquipment::CanEquip(ItemId, HeroClass)) {
        Toast = TEXT("NOT FOR THIS CLASS"); ToastTime=2.f; return;
    }
    const FAVEquipmentSpec* Item = FAVEquipment::Find(ItemId);
    if (!Item) return;
    int32 Slot = Item->Slot;
    if (Slot == AVAccessory && EquipmentSlots.IsValidIndex(7) &&
        EquipmentSlots[6] >= 0 && EquipmentSlots[7] < 0) Slot = 7;
    if (!EquipmentSlots.IsValidIndex(Slot)) return;
    if (EquipmentSlots[Slot] >= 0) InventoryItems.Add(EquipmentSlots[Slot]);
    EquipmentSlots[Slot] = ItemId;
    InventoryItems.RemoveAt(Index);
    GearSelection = FMath::Clamp(GearSelection, 0, FMath::Max(0, InventoryItems.Num()-1));
    RebuildCharacterBonuses();
    Toast = TEXT("EQUIPPED: ") + GearName(ItemId);
    ToastTime = 2.3f;
}

void AAVGameMode::UnequipSlot(int32 Slot)
{
    if (!EquipmentSlots.IsValidIndex(Slot) || EquipmentSlots[Slot] < 0 ||
        InventoryItems.Num() >= 48) return;
    const int32 Id = EquipmentSlots[Slot];
    EquipmentSlots[Slot] = -1;
    InventoryItems.Add(Id);
    RebuildCharacterBonuses();
    Toast = TEXT("UNEQUIPPED: ") + GearName(Id);
    ToastTime = 2.f;
}

void AAVGameMode::LearnSkill(int32 Selection)
{
    const int32 Id = SkillIdAt(Selection);
    if (Id < 0 || !FAVSkills::CanLearn(Id, HeroClass, Level,
        LearnedSkills.GetData(), LearnedSkills.Num(), SkillPoints)) {
        Toast = TEXT("SKILL LOCKED OR NO POINTS"); ToastTime=2.f; return;
    }
    ++LearnedSkills[Id];
    --SkillPoints;
    RebuildCharacterBonuses();
    Toast = TEXT("SKILL UNLOCKED"); ToastTime = 2.f;
}

void AAVGameMode::ResetSkills()
{
    if (!bInLobby && !bInSafeCamp) {
        Toast=TEXT("RESPEC IN TOWN OR CAMP"); ToastTime=2.f; return;
    }
    const int32 Cost = bFreeRespecUsed ? 20 + Level * 5 : 0;
    if (Gold < Cost) { Toast=TEXT("NOT ENOUGH GOLD"); ToastTime=2.f; return; }
    Gold -= Cost;
    bFreeRespecUsed = true;
    LearnedSkills.Init(0, FAVSkills::Count);
    SkillPoints = FAVSkills::AwardedPointsAtLevel(Level);
    RebuildCharacterBonuses();
    Toast=TEXT("SKILLS RESET"); ToastTime=2.f;
}

void AAVGameMode::SpawnGearDrop(const FVector2D& Pos, int32 MinimumRarity)
{
    // Consumables share the backpack but cannot be equipped.
    const bool bConsumable=RNG.FRand()<0.18f;
    const int32 Rarity = FMath::Max(MinimumRarity, FAVEquipment::RarityRoll(RNG.RandRange(0,99)));
    const int32 MaxTier = Floor < 6 ? AVRare : Floor < 11 ? AVEpic : AVLegendary;
    TArray<int32> Candidates;
    for (int32 I=0; I<FAVEquipment::Count; ++I) {
        const FAVEquipmentSpec* Item = FAVEquipment::Find(I);
        if (!Item || Item->Rarity > MaxTier ||
            Item->Rarity < FMath::Min(Rarity,MaxTier) ||
            (Item->Slot==AVConsumable)!=bConsumable)continue;
        if (Item->RequiredHero == HeroClass || Item->RequiredHero == -1)
            Candidates.Add(I);
    }
    if (Candidates.Num() == 0)
        for(int32 I=0;I<FAVEquipment::Count;++I) {
            const FAVEquipmentSpec* Item=FAVEquipment::Find(I);
            if(Item && Item->Rarity<=MaxTier &&
               (Item->Slot==AVConsumable)==bConsumable &&
               (Item->RequiredHero==HeroClass || Item->RequiredHero==-1))
                Candidates.Add(I);
        }
    if(Candidates.Num()==0) return;
    FAVPickup Gear;
    Gear.Pos=Pos;
    Gear.Type=11;
    const int32 BaseId=Candidates[RNG.RandRange(0,Candidates.Num()-1)];
    Gear.Amount=FAVEquipment::EncodeInstance(BaseId,
        FAVEquipment::RollAffix(BaseId,RNG.RandRange(0,99)));
    Pickups.Add(Gear);
}

void AAVGameMode::InvalidateCheckpoint()
{
    bCanContinue = false;
    UAVSaveGame* Save = Cast<UAVSaveGame>(
        UGameplayStatics::LoadGameFromSlot(TEXT("AshenVaultsRecord"), 0));
    if (!Save || !Save->bHasCheckpoint) return;
    // Preserve lifetime statistics but never allow an abandoned/dead run
    // to reappear as a valid "Continue" choice after the next launch.
    Save->bHasCheckpoint = false;
    UGameplayStatics::SaveGameToSlot(Save, TEXT("AshenVaultsRecord"), 0);
}

void AAVGameMode::NewRun(bool bSameSeed, bool bKeepExistingCheckpoint)
{
    if (!bKeepExistingCheckpoint) InvalidateCheckpoint();
    Floor = 1; Level = 1; XP = 0; Gold = 0; Kills = 0; Potions = 2;
    EquipmentSlots.Init(-1, 8); InventoryItems.Reset();
    LearnedSkills.Init(0, FAVSkills::Count);
    SkillPoints=0; GearSelection=0; SkillSelection=0;
    bFreeRespecUsed=false; bLastStandUsed=false;
    EquipmentAppliedHP=0.f; EquipmentAppliedStamina=0.f;
    EquipmentAppliedAttack=0.f; EquipmentAppliedSpeed=0.f; EquipmentAppliedArmor=0.f;
    RewardDamageTime=0.f; DashBonusTime=0.f; RevengeTime=0.f;
    FoodAttackTime=0.f;FoodGuardTime=0.f;FoodSpeedTime=0.f;
    FoodCritTime=0.f;FoodProjectileTime=0.f;FoodBossTime=0.f;
    FoodPoisonWardTime=0.f;
    BasicAttackSequence=0; MeleeSwingCount=0; BlooddrinkerHitCount=0;
    ChainCooldown=0.f; GuardWhirlTime=0.f; TemporaryShield=0.f;
    ShieldCooldown=0.f; bMagicBarrierUsed=false;
    LastKillBoostTime=0.f; MarkedHitTime=0.f;
    MapW = 52; MapH = 38; ShotSeries = 0; ShotTargetId = -1;
    ShotCounter = 0; LastConnectedShotId = 0; VolleyCounter = 0;
    LastShotHitTime = -100.f;
    MaxHP = FAVContent::HeroHealth(HeroClass); HP = MaxHP;
    MaxStamina = FAVContent::HeroStamina(HeroClass); Stamina = MaxStamina;
    AttackPower = FAVContent::HeroAttack(HeroClass);
    MoveSpeed = FAVContent::HeroSpeed(HeroClass); RunTime = 0.f;
    ArmorRating = 0.f; RangedBonus = 0.f; EquippedGear = 0;
    bInSafeCamp = false; bInLobby = false; bInBossArena = false;
    bWorldStored = false; PackGold = 0; PackMaterials = 0;
    BackpackArmor = 0; BackpackBows = 0; BackpackWeapons = 0;
    EquippedSlotsMask = 0;
    WeaponUpgrade = 0; ArmorUpgrade = 0;
    RelicMask = 0u; RelicCount = 0; Echoes = 0; bRelicPending = false;
    HeroVelocity = FVector2D::ZeroVector;
    PoisonTime = 0.f; BurnTime = 0.f; StatusTick = 0.f; DashCooldown = 0.f;
    AttackCooldown = 0.f; KnifeCooldown = 0.f; SpinCooldown = 0.f; DashTime = 0.f;
    Invulnerability = 0.f; ScreenShake = 0.f;
    Combo = 0; ComboTime = 0.f; Toast.Empty(); ToastTime = 0.f;
    Phase = EAVPhase::Playing;
    if (!bSameSeed || RunSeed == 0)
        RunSeed = static_cast<int32>(FDateTime::UtcNow().GetTicks() & 0x7fffffff);
    RNG.Initialize(RunSeed);
    GenerateFloor();
}

void AAVGameMode::GenerateFloor()
{
    // Dimensions belong to the biome and never reshape previously generated floors.
    ShotSeries=0; ShotTargetId=-1; LastShotHitTime=-100.f;
    bLastStandUsed=false; RewardDamageTime=0.f;
    TemporaryShield=0.f; LastKillBoostTime=0.f; ChainCooldown=0.f;
    GuardWhirlTime=0.f; ShieldCooldown=0.f; bMagicBarrierUsed=false;
    MapW = CurrentBiome() == 3 ? 60 : 52;
    MapH = CurrentBiome() == 3 ? 44 : 38;
    // Floor layouts are independent of combat loot RNG consumption.
    // This makes the same run seed reproducible on every depth.
    RNG.Initialize(static_cast<int32>(static_cast<uint32>(RunSeed) ^
        (0x9e3779b9u * static_cast<uint32>(Floor))));
    Tiles.Init(0, MapW * MapH);
    Visible.Init(0, MapW * MapH);
    Explored.Init(0, MapW * MapH);
    FlowDistances.Init(-1, MapW * MapH);
    LastVisionX = -1; LastVisionY = -1; FlowRefreshTime = 0.f;
    Enemies.Reset(); Pickups.Reset(); Bolts.Reset(); Effects.Reset(); Traps.Reset(); Chests.Reset();
    Hazards.Reset(); Props.Reset(); Decorations.Reset(); RoomEvents.Reset();
    HeroVelocity = FVector2D::ZeroVector;
    bMerchantSpawned = false;
    bBanditCamp = false;
    bGuardianDefeated = false; bHasSafeCampDoor = false;
    PendingGuardian = -1;
    CampDoors.Reset(); CampWalls.Reset();
    for (int32 I = 0; I < 3; ++I) ShopPurchased[I] = false;
    PoisonTime = 0.f; BurnTime = 0.f; StatusTick = 0.f;
    AttackCooldown = 0.f; KnifeCooldown = 0.f; SpinCooldown = 0.f;
    DashTime = 0.f; DashCooldown = 0.f;
    bShrineUsed = false;
    struct FRoom { int32 X, Y, W, H; FVector2D Center() const { return FVector2D(X + W / 2.f, Y + H / 2.f); } };
    TArray<FRoom> Rooms;
    const int32 TargetCount = CurrentBiome() == 3 ?
        FMath::Min(14, 10 + BiomeDepth()) : FMath::Min(20, 9 + BiomeDepth() * 2);
    if(CurrentBiome()==3)
    {
        TArray<FAVFortressRoom> Layout;
        const int32 LayoutSeed=static_cast<int32>(static_cast<uint32>(RunSeed)^
            (static_cast<uint32>(Floor)*0x9E3779B9u));
        FAVFortressLayout::Build(LayoutSeed,Tiles,Layout);
        for(const FAVFortressRoom& R:Layout)
            Rooms.Add({R.X,R.Y,R.W,R.H});
    }
    else
    {
    for (int32 Attempt = 0; Attempt < 220 && Rooms.Num() < TargetCount; ++Attempt)
    {
        const bool bBastion = CurrentBiome() == 3;
        // The Bastion has a guaranteed main battle hall, wings and broad galleries.
        // Every fourth chamber can become a larger arena with room for
        // crossfire, flanks and elite squads; other rooms remain compact.
        const bool bArena=Rooms.Num()>0 && Rooms.Num()%4==0;
        const int32 W=bBastion ? (Rooms.Num()==0?18:RNG.RandRange(8,14)) :
            RNG.RandRange(bArena?10:5,bArena?15:11);
        const int32 H=bBastion ? (Rooms.Num()==0?14:RNG.RandRange(8,12)) :
            RNG.RandRange(bArena?8:5,bArena?11:9);
        FRoom R = { RNG.RandRange(2, MapW - W - 3), RNG.RandRange(2, MapH - H - 3), W, H };
        bool bOverlaps = false;
        for (const FRoom& Existing : Rooms)
        {
            if (R.X - 2 < Existing.X + Existing.W && R.X + R.W + 2 > Existing.X &&
                R.Y - 2 < Existing.Y + Existing.H && R.Y + R.H + 2 > Existing.Y)
            {
                bOverlaps = true;
                break;
            }
        }
        if (bOverlaps) continue;
        for (int32 Y = R.Y; Y < R.Y + R.H; ++Y)
        {
            for (int32 X = R.X; X < R.X + R.W; ++X)
            {
                bool bCarve = true;
                if (CurrentBiome() == 1)
                {
                    // Spore Hollows: irregular rounded organic chambers.
                    const float DX = (X + 0.5f - (R.X + R.W * 0.5f)) /
                        FMath::Max(1.f, R.W * 0.5f);
                    const float DY = (Y + 0.5f - (R.Y + R.H * 0.5f)) /
                        FMath::Max(1.f, R.H * 0.5f);
                    bCarve = DX * DX + DY * DY < 1.13f ||
                        (FMath::Abs(DX) < 0.22f && FMath::Abs(DY) < 0.85f);
                }
                else if (CurrentBiome() == 2)
                {
                    // Ember Foundry: straight-sided halls with clipped corners.
                    const int32 CornerX = FMath::Min(X - R.X, R.X + R.W - 1 - X);
                    const int32 CornerY = FMath::Min(Y - R.Y, R.Y + R.H - 1 - Y);
                    bCarve = CornerX + CornerY >= 2 ||
                        (CornerX >= 1 && CornerY >= 1);
                }
                // Bastion keeps square fortified rooms rather than caves or clipped corners.
                if (bCarve) Tiles[Y * MapW + X] = 1;
            }
        }

        if (Rooms.Num() > 0)
        {
            FVector2D A = Rooms.Last().Center(), B = R.Center();
            int32 AX = FMath::FloorToInt(A.X), AY = FMath::FloorToInt(A.Y);
            const int32 BX = FMath::FloorToInt(B.X), BY = FMath::FloorToInt(B.Y);
            const bool bHorizontalFirst = RNG.RandRange(0, 1) == 0;
            auto DigX = [&]() {
                while (AX != BX) {
                    Tiles[AY * MapW + AX] = 1;
                    if (CurrentBiome() == 3 && AY + 1 < MapH - 1) Tiles[(AY + 1) * MapW + AX] = 1;
                    AX += BX > AX ? 1 : -1;
                }
                Tiles[AY * MapW + AX] = 1;
            };
            auto DigY = [&]() {
                while (AY != BY) {
                    Tiles[AY * MapW + AX] = 1;
                    if (CurrentBiome() == 3 && AX + 1 < MapW - 1) Tiles[AY * MapW + AX + 1] = 1;
                    AY += BY > AY ? 1 : -1;
                }
                Tiles[AY * MapW + AX] = 1;
            };
            if (bHorizontalFirst) { DigX(); DigY(); }
            else { DigY(); DigX(); }
        }
        Rooms.Add(R);
    }
    // Extra loops create alternate approaches rather than a single linear chain.
    for (int32 I = 0; I + 3 < Rooms.Num(); I += 3)
    {
        FVector2D A = Rooms[I].Center(), B = Rooms[I + 3].Center();
        int32 X = FMath::FloorToInt(A.X), Y = FMath::FloorToInt(A.Y);
        const int32 BX = FMath::FloorToInt(B.X), BY = FMath::FloorToInt(B.Y);
        while (X != BX)
        {
            Tiles[Y * MapW + X] = 1;
            X += BX > X ? 1 : -1;
        }
        while (Y != BY)
        {
            Tiles[Y * MapW + X] = 1;
            Y += BY > Y ? 1 : -1;
        }
        Tiles[Y * MapW + X] = 1;
    }
    } // end classic room-and-corridor generator

    // Eight distinct combat-room topologies, applied before doors, traps
    // and hostile camps. The entry room stays clear.
    for(int32 RoomIndex=1;RoomIndex<Rooms.Num();++RoomIndex)
    {
        const FRoom& R=Rooms[RoomIndex];
        FAVRoomBounds Bounds;
        Bounds.X=R.X;Bounds.Y=R.Y;Bounds.W=R.W;Bounds.H=R.H;
        FAVDepthDirector::ApplyGeometry(Tiles,MapW,MapH,Bounds,
            FAVDepthDirector::Geometry(RunSeed,Floor,RoomIndex,CurrentBiome()));
        const FAVRoomTactic Tactic=FAVRoomTactics::Select(
            RunSeed,Floor,RoomIndex,CurrentBiome());
        FAVRoomTactics::ApplyShape(Tiles,MapW,MapH,Bounds,Tactic.Shape);
    }
    // Additional shortcuts connect distant wings instead of only consecutive
    // rooms. The connectivity pass repairs dead pockets after cover placement.
    if(Rooms.Num()>=5 && (Floor%3)!=0)
    {
        const FRoom& A=Rooms[1],& B=Rooms[Rooms.Num()-2];
        FAVRoomBounds Left,Right;
        Left.X=A.X;Left.Y=A.Y;Left.W=A.W;Left.H=A.H;
        Right.X=B.X;Right.Y=B.Y;Right.W=B.W;Right.H=B.H;
        FAVDepthDirector::CarveAlternativeRoute(
            Tiles,MapW,MapH,Left,Right,(Floor%2)==0);
    }
    if(Rooms.Num()>0)
        FAVDepthDirector::RepairConnectivity(Tiles,MapW,MapH,Rooms[0].Center());
    if (Rooms.Num() == 0)
    {
        for (int32 Y = 4; Y <= 13; ++Y)
            for (int32 X = 4; X <= 17; ++X) Tiles[Y * MapW + X] = 1;
        Hero = FVector2D(9.5f, 8.5f);
        Exit = FVector2D(13.5f, 8.5f);
        Shrine = FVector2D(6.5f, 8.5f);
        Merchant = FVector2D(15.5f, 8.5f);
        bMerchantSpawned = true;
        RefreshAwareness(0.f, true);
        return;
    }
    Hero = Rooms[0].Center();
    Exit = Rooms.Last().Center();
    Shrine = Rooms[Rooms.Num() / 2].Center();
    if (Rooms.Num() >= 3)
    {
        Merchant = Rooms[1].Center();
        bMerchantSpawned = false; // Merchant relocated to peaceful camp.
    }
    // Sparse interior stone pillars create cover and alternate routes.
    // Keep spawn, merchant, exit and shrine chambers unobstructed.
    for (int32 I = 2; I < Rooms.Num() - 1; ++I)
    {
        if (I == Rooms.Num() / 2 || RNG.FRand() > 0.45f) continue;
        const FRoom& R = Rooms[I];
        if (R.W < 8 || R.H < 7) continue;
        const int32 PX = R.X + 2 + RNG.RandRange(0, R.W - 6);
        const int32 PY = R.Y + 2 + RNG.RandRange(0, R.H - 6);
        if (FVector2D::Distance(FVector2D(PX + 0.5f, PY + 0.5f), R.Center()) < 2.f)
            continue;
        Tiles[PY * MapW + PX] = 0;
        if (R.W >= 10 && RNG.RandRange(0, 1) == 1)
            Tiles[PY * MapW + PX + 1] = 0;
    }
    // Rooms remain connected by the carved corridors. Keep traps away from
    // spawn, stairs and the shrine, so none of the interactions become blocked.
    for (int32 I = 1; I < Rooms.Num(); ++I)
    {
        if (I % 2 != 0) continue;
        const FRoom& R = Rooms[I];
        for (int32 Attempt = 0; Attempt < 4; ++Attempt)
        {
            const FVector2D Candidate(
                RNG.RandRange(R.X + 1, R.X + R.W - 2) + 0.5f,
                RNG.RandRange(R.Y + 1, R.Y + R.H - 2) + 0.5f);
            if (!CanMove(Candidate, 0.3f) ||
                FVector2D::Distance(Candidate, Hero) < 4.f ||
                FVector2D::Distance(Candidate, Exit) < 2.2f ||
                FVector2D::Distance(Candidate, Shrine) < 2.2f ||
                (bMerchantSpawned && FVector2D::Distance(Candidate, Merchant) < 2.2f))
                continue;
            FAVTrap Trap;
            Trap.Pos = Candidate;
            Trap.Type = CurrentBiome() == 3 ? RNG.RandRange(3, 6) : RNG.RandRange(0, 2);
            Traps.Add(Trap);
            break;
        }
    }

    // Treasure chambers: optional detours; one chance in five hides a mimic.
    for (int32 I = 2; I < Rooms.Num() - 1; I += 3)
    {
        const FRoom& R = Rooms[I];
        FAVChest Chest;
        Chest.Pos = FVector2D(R.X + 1.5f, R.Y + 1.5f);
        if (FVector2D::Distance(Chest.Pos, Shrine) < 2.5f ||
            FVector2D::Distance(Chest.Pos, Exit) < 2.5f ||
            (bMerchantSpawned && FVector2D::Distance(Chest.Pos, Merchant) < 2.5f))
            Chest.Pos = FVector2D(R.X + R.W - 1.5f, R.Y + R.H - 1.5f);
        TArray<FVector2D> Reserved;
        for (const FAVTrap& Trap : Traps) Reserved.Add(Trap.Pos);
        for (const FAVChest& Existing : Chests) Reserved.Add(Existing.Pos);
        if (!CanMove(Chest.Pos,0.3f))
        {
            FAVRoomBounds Bounds;
            Bounds.X=R.X;Bounds.Y=R.Y;Bounds.W=R.W;Bounds.H=R.H;
            if(!FAVDepthDirector::FindSpawn(Tiles,MapW,MapH,Bounds,
                EAVFormation::CornerSentries,I,Rooms.Num(),Hero,Exit,Shrine,
                Reserved,Chest.Pos,4.f))continue;
        }
        bool bBlocked = false;
        for (const FVector2D& Spot : Reserved)
            bBlocked |= (Chest.Pos-Spot).SizeSquared()<1.69f;
        if (bBlocked) continue;
        Chest.bMimic = RNG.FRand() < 0.18f;
        Chests.Add(Chest);
    }

    int32 NextId = 1;
    for (int32 I = 1; I < Rooms.Num(); ++I)
    {
        // The merchant's chamber is a safe retreat, not an accidental arena.
        if (bMerchantSpawned && I == 1) continue;
        const FRoom& R = Rooms[I];
        const FAVRoomTactic Tactic=FAVRoomTactics::Select(
            RunSeed,Floor,I,CurrentBiome());
        const FAVEncounterPlan Encounter = FAVRoomTactics::Garrison(
            FAVEncounters::Plan(Floor,R.W*R.H,
                RNG.RandRange(0,99),RNG.RandRange(0,99)),
            Tactic.Garrison,R.W*R.H,Floor);
        FAVRoomBounds Bounds;
        Bounds.X=R.X;Bounds.Y=R.Y;Bounds.W=R.W;Bounds.H=R.H;
        const EAVFormation Formation=FAVRoomTactics::Formation(
            Tactic.Garrison,FAVDepthDirector::Formation(
                RunSeed,Floor,I,static_cast<int32>(Encounter.Kind)));
        // Traps and opened treasure positions are reserved before spawning.
        TArray<FVector2D> Occupied;
        for(const FAVEnemy& Existing:Enemies)Occupied.Add(Existing.Pos);
        for(const FAVTrap& Trap:Traps)Occupied.Add(Trap.Pos);
        for(const FAVHazard& Hazard:Hazards)Occupied.Add(Hazard.Pos);
        for(const FAVChest& Chest:Chests)Occupied.Add(Chest.Pos);
        for (int32 E = 0; E < Encounter.EnemyCount; ++E)
        {
            FAVEnemy Enemy;
            if(!FAVDepthDirector::FindSpawn(Tiles,MapW,MapH,Bounds,Formation,
                E,Encounter.EnemyCount,Hero,Exit,Shrine,Occupied,Enemy.Pos,4.5f))
                continue;
            Enemy.Type = FAVEncounters::SelectType(
                Floor, FAVRoomTactics::EnemyRole(Tactic.Garrison,Encounter.Kind),
                RNG.RandRange(0, 99), RNG.RandRange(0, 99),
                RNG.RandRange(0, 99));
            if (Enemy.Type >= 34 && Enemy.Type <= 51) Enemy.Subclass = RNG.RandRange(0, 1);
            Enemy.Level = FAVContent::EnemyLevel(Floor, RNG.RandRange(0, 99));
            Enemy.Equipment = RNG.FRand() < Encounter.EquipmentChance ?
                RNG.RandRange(1, 3) : 0;
            Enemy.bElite = E < Encounter.GuaranteedElites ||
                RNG.FRand() < Encounter.EliteChance;
            Enemy.Affix=FAVAffixes::Roll(static_cast<uint32>(RunSeed),
                Floor,Enemy.Type,NextId,Enemy.bElite,false);
            // Affix health modifies the same single-pass floor-scaled base.
            Enemy.MaxHP = FAVContent::ScaledEnemyHealth(Enemy.Type,Floor,Enemy.Level)*
                (Enemy.Equipment==1?1.35f:1.f)*
                (Enemy.bElite?1.65f:1.f)*
                FAVAffixes::Traits(Enemy.Affix,1.f,0.f).MaxHealthScale;
            Enemy.HP = Enemy.MaxHP;
            Enemy.Cooldown = RNG.FRandRange(0.3f, 1.6f);
            // Do not teleport a validated spawn onto an unvalidated corner.
            // Merchant chambers are skipped above; all other positions have
            // already passed the walkability and occupancy checks.
            Enemy.Id = NextId++;
            Occupied.Add(Enemy.Pos);
            Enemies.Add(Enemy);
        }

        // Optional second-stage encounters. The four triggers have distinct
        // conditions and a visible warning, never surprise instant damage.
        const int32 MaxRoomEvents=FMath::Min(5,2+BiomeDepth()/2);
        if(I>1 && I<Rooms.Num()-1 && RoomEvents.Num()<MaxRoomEvents)
        {
            const FAVWaveDirective Directive=FAVRoomTactics::Response(
                FAVDepthDirector::Wave(RunSeed,Floor,I,R.W*R.H,
                    static_cast<int32>(Encounter.Kind)),
                Tactic.Response,R.W*R.H,Floor);
            if(Directive.Trigger!=EAVWaveTrigger::None)
            {
                FAVRoomEvent Event;
                Event.Center=R.Center();
                Event.X=R.X;Event.Y=R.Y;Event.W=R.W;Event.H=R.H;
                Event.Trigger=static_cast<int32>(Directive.Trigger);
                Event.Formation=static_cast<int32>(Directive.Formation);
                Event.Kind=Directive.Kind;
                Event.Count=Directive.UnitCount;
                Event.Countdown=Directive.WarningSeconds;
                Event.UniqueId=I;
                RoomEvents.Add(Event);
                if(Directive.Trigger==EAVWaveTrigger::OnChestOpen)
                {
                    bool bHasTreasure=false;
                    for(const FAVChest& Chest:Chests)
                        if(Chest.Pos.X>=R.X && Chest.Pos.X<R.X+R.W &&
                           Chest.Pos.Y>=R.Y && Chest.Pos.Y<R.Y+R.H)
                        {bHasTreasure=true;break;}
                    if(!bHasTreasure)
                    {
                        const FVector2D Cache(R.X+1.5f,R.Y+R.H-1.5f);
                        if(CanMove(Cache,0.3f) &&
                           (Cache-Hero).SizeSquared()>16.f &&
                           (Cache-Exit).SizeSquared()>6.25f)
                        {FAVChest Chest;Chest.Pos=Cache;Chests.Add(Chest);}
                        else RoomEvents.Pop(); // No reachable reward to guard.
                    }
                }
            }
        }
    }
    // Optional bandit hideout: occupied chamber with treasure, ambush and rival faction.
    if (Rooms.Num() >= 5 && RNG.FRand() < 0.38f)
    {
        const int32 RoomIndex = RNG.RandRange(2, Rooms.Num() - 2);
        const FRoom& Camp = Rooms[RoomIndex];
        BanditCampCenter = Camp.Center();
        bBanditCamp = true;
        // Rebuild this chamber as an enclosed hideout. Every former corridor
        // entrance becomes an interactable wooden gate; other rim tiles are walls.
        for (int32 Y = Camp.Y; Y < Camp.Y + Camp.H; ++Y)
        {
            for (int32 X = Camp.X; X < Camp.X + Camp.W; ++X)
            {
                const bool bRim = X == Camp.X || X == Camp.X + Camp.W - 1 ||
                                  Y == Camp.Y || Y == Camp.Y + Camp.H - 1;
                if (!bRim) continue;
                const bool bCorridor =
                    (X == Camp.X && IsFloor(X - 1, Y)) ||
                    (X == Camp.X + Camp.W - 1 && IsFloor(X + 1, Y)) ||
                    (Y == Camp.Y && IsFloor(X, Y - 1)) ||
                    (Y == Camp.Y + Camp.H - 1 && IsFloor(X, Y + 1));
                const FVector2D Point(X + 0.5f, Y + 0.5f);
                Tiles[Y * MapW + X] = 0;
                if (bCorridor) CampDoors.Add(Point);
                else CampWalls.Add(Point);
            }
        }
        // Keep at least one usable gate if the generated corridor touches a corner.
        if (CampDoors.Num() == 0)
        {
            const int32 X = Camp.X + Camp.W / 2, Y = Camp.Y;
            CampDoors.Add(FVector2D(X + 0.5f, Y + 0.5f));
        }
        FAVRoomBounds CampRegion;
        CampRegion.X=Camp.X;CampRegion.Y=Camp.Y;
        CampRegion.W=Camp.W;CampRegion.H=Camp.H;
        TArray<FVector2D> CampOccupied;
        for(const FAVEnemy& Other:Enemies)CampOccupied.Add(Other.Pos);
        for (int32 I = 0; I < FAVContent::BanditGroupSize(Floor); ++I)
        {
            FAVEnemy Bandit;
            if(!FAVDepthDirector::FindSpawn(Tiles,MapW,MapH,CampRegion,
                EAVFormation::CornerSentries,I,FAVContent::BanditGroupSize(Floor),
                Hero,Exit,Shrine,CampOccupied,Bandit.Pos,4.f))continue;
            Bandit.Type = I % 3 == 0 ? 25 : 33;
            Bandit.Faction = 1;
            Bandit.Level = FAVContent::EnemyLevel(Floor, RNG.RandRange(0, 99));
            Bandit.Affix=FAVAffixes::Roll(static_cast<uint32>(RunSeed),
                Floor,Bandit.Type,NextId,false,false);
            Bandit.MaxHP = FAVContent::ScaledEnemyHealth(Bandit.Type, Floor, Bandit.Level)*
                FAVAffixes::Traits(Bandit.Affix,1.f,0.f).MaxHealthScale;
            Bandit.HP = Bandit.MaxHP;
            Bandit.Cooldown = RNG.FRandRange(0.3f, 1.1f);
            Bandit.Id = NextId++;
            CampOccupied.Add(Bandit.Pos);
            Enemies.Add(Bandit);
        }
        for (int32 I = 0; I < 2; ++I)
        {
            FAVChest Loot;
            Loot.Pos = FVector2D(Camp.X + 1.5f + I, Camp.Y + 1.5f);
            Loot.bMimic = false;
            Chests.Add(Loot);
        }
    }
    // Seven possible miniboss archetypes, one optional encounter per floor.
    if (Rooms.Num() >= 4 && RNG.FRand() < 0.55f)
    {
        const FRoom& Arena = Rooms[RNG.RandRange(2, Rooms.Num() - 2)];
        FAVEnemy Mini;
        Mini.Type = CurrentBiome() == 3 ? 52 + RNG.RandRange(0, 3) : 26 + RNG.RandRange(0, 6);
        Mini.bMiniBoss = true;
        Mini.Pos = Arena.Center();
        Mini.Level = FAVContent::EnemyLevel(Floor, 99);
        Mini.MaxHP = FAVContent::ScaledEnemyHealth(Mini.Type, Floor, Mini.Level);
        Mini.HP = Mini.MaxHP;
        Mini.Cooldown = 1.f;
        Mini.Id = NextId++;
        Enemies.Add(Mini);
        // The mini-boss is protected by a small biome-native vanguard.
        // Escort units are standard enemies, not new archetypes.
        FAVRoomBounds MinRegion;
        MinRegion.X=Arena.X;MinRegion.Y=Arena.Y;
        MinRegion.W=Arena.W;MinRegion.H=Arena.H;
        TArray<FVector2D> GuardOccupied;
        for(const FAVEnemy& Existing:Enemies)GuardOccupied.Add(Existing.Pos);
        for(int32 G=0;G<2;++G)
        {
            FAVEnemy Guard;
            if(!FAVDepthDirector::FindSpawn(Tiles,MapW,MapH,MinRegion,
                EAVFormation::Wedge,G,2,Hero,Exit,Shrine,
                GuardOccupied,Guard.Pos,4.5f))continue;
            Guard.Type=FAVContent::EnemyRoll(Floor,RNG.RandRange(0,99));
            Guard.Level=FAVContent::EnemyLevel(Floor,RNG.RandRange(0,99));
            Guard.MaxHP=FAVContent::ScaledEnemyHealth(Guard.Type,Floor,Guard.Level);
            Guard.HP=Guard.MaxHP; Guard.Cooldown=1.f+0.3f*G;
            Guard.Id=NextId++;GuardOccupied.Add(Guard.Pos);
            Enemies.Add(Guard);
        }
    }
    PendingGuardian = IsGuardianFloor() ? FAVContent::Boss(Floor) : -1;
    // The guardian is spawned in a separate arena when its portal is used.
    if (Rooms.Num() > 2 && RNG.FRand() < 0.36f)
    {
        const FRoom& Approach = Rooms[1];
        // Portal is anchored in the room's northern wall, not in its floor.
        const int32 DoorX = Approach.X + Approach.W / 2;
        const int32 DoorY = Approach.Y - 1;
        SafeCampDoor = FVector2D(DoorX + 0.5f, DoorY + 0.5f);
        bHasSafeCampDoor = DoorY > 0 && DoorX > 0 &&
            DoorX < MapW - 1 && Tiles[DoorY * MapW + DoorX] == 0 &&
            Tiles[(DoorY + 1) * MapW + DoorX] == 1;
    }
    // Add a couple of guaranteed recovery pickups away from the spawn.
    for (int32 I = 1; I < Rooms.Num(); I += 3)
    {
        FAVPickup Pickup;
        Pickup.Pos = Rooms[I].Center();
        Pickup.Type = (I % 2 == 0) ? 1 : 2;
        Pickup.Amount = 22;
        Pickups.Add(Pickup);
    }
    // A small, deterministic pre-guardian recovery station in the entrance
    // chamber. Uses existing pickups, with no new artwork or item IDs.
    if(IsGuardianFloor() && Rooms.Num()>0)
    {
        const FVector2D Supply=Hero+FVector2D(1.6f,0.f);
        if(CanMove(Supply,0.25f))
        {
            FAVPickup P;
            P.Pos=Supply;P.Type=Floor%2==0?2:1;
            P.Amount=FMath::Min(45,20+BiomeCycle()*4);
            Pickups.Add(P);
        }
    }
    if (Floor >= 3 && CurrentBiome() != 3)
    {
        for (int32 I = 2; I < Rooms.Num() - 1; I += 2)
        {
            const FRoom& R = Rooms[I];
            FAVHazard Hazard;
            Hazard.Type = CurrentBiome() == 2 ? 1 : 0;
            Hazard.Pos = FVector2D(R.X + 1.5f, R.Y + R.H - 1.5f);
            bool bNearEnemy=false;
            for(const FAVEnemy& Enemy:Enemies)
                if((Enemy.Pos-Hazard.Pos).SizeSquared()<2.25f)
                {bNearEnemy=true;break;}
            if (!bNearEnemy && CanMove(Hazard.Pos,0.3f) &&
                (Hazard.Pos-Hero).SizeSquared()>16.f &&
                (Hazard.Pos-Shrine).SizeSquared()>6.25f &&
                (Hazard.Pos-Exit).SizeSquared()>6.25f &&
                (!bMerchantSpawned ||
                 (Hazard.Pos-Merchant).SizeSquared()>6.25f))
                Hazards.Add(Hazard);
        }
    }
    for (int32 I = 1; I < Rooms.Num(); ++I)
    {
        const FRoom& R = Rooms[I];
        FAVProp Prop;
        Prop.Type = CurrentBiome() >= 2 ? 1 : 0;
        Prop.Pos = FVector2D(R.X + R.W - 1.5f, R.Y + 1.5f);
        if (FVector2D::Distance(Prop.Pos, Shrine) < 2.f ||
            FVector2D::Distance(Prop.Pos, Exit) < 2.f ||
            (bMerchantSpawned && FVector2D::Distance(Prop.Pos, Merchant) < 2.f))
            continue;
        bool bNearChest = false;
        for (const FAVChest& Chest : Chests)
            bNearChest |= FVector2D::Distance(Prop.Pos, Chest.Pos) < 1.1f;
        if (!bNearChest) Props.Add(Prop);
    }
    // Deterministic environmental silhouettes. They are decorative and do
    // not block collision, stairs or pickup positions. Each biome has its own
    // stonework, light fixture and one-time Echo artifact.
    const int32 BiomeBase = CurrentBiome() * 3;
    for (int32 I = 0; I < Rooms.Num(); ++I)
    {
        const FRoom& R = Rooms[I];
        const int32 Count = I == 0 ? 2 : RNG.RandRange(2, 3);
        for (int32 D = 0; D < Count; ++D)
        {
            FAVDecor Decor;
            Decor.Type = BiomeBase + (D + I) % 3;
            Decor.Pos = FVector2D(
                D == 0 ? R.X + 1.6f : D == 1 ? R.X + R.W - 1.6f : R.X + R.W * 0.5f,
                D == 0 ? R.Y + 1.6f : D == 1 ? R.Y + 1.6f : R.Y + R.H - 1.6f);
            Decor.Phase = RNG.FRandRange(0.f, 6.28318f);
            if (FVector2D::Distance(Decor.Pos, Hero) < 2.3f ||
                FVector2D::Distance(Decor.Pos, Shrine) < 1.8f ||
                FVector2D::Distance(Decor.Pos, Exit) < 1.8f ||
                (bMerchantSpawned && FVector2D::Distance(Decor.Pos, Merchant) < 1.8f))
                continue;
            bool bNearTrap = false;
            for (const FAVTrap& Trap : Traps)
                bNearTrap |= FVector2D::Distance(Trap.Pos, Decor.Pos) < 0.9f;
            if (!bNearTrap) Decorations.Add(Decor);
        }
    }
    // v1.0 scenic dressing: four additional non-blocking props per biome.
    // No puzzle interactions or collision changes; these simply give each
    // combat room more visual character while keeping the exit traversable.
    for(int32 RoomIndex=1;RoomIndex<Rooms.Num();++RoomIndex)
    {
        const FRoom& R=Rooms[RoomIndex];
        const int32 Amount=RNG.RandRange(1,3);
        for(int32 I=0;I<Amount;++I)
        {
            FAVDecor D;
            D.Type=12+CurrentBiome()*4+RNG.RandRange(0,3);
            D.Pos=FVector2D(R.X+RNG.FRandRange(2.f,FMath::Max(2.1f,float(R.W)-2.f)),
                            R.Y+RNG.FRandRange(2.f,FMath::Max(2.1f,float(R.H)-2.f)));
            D.Phase=RNG.FRandRange(0.f,6.2831853f);
            if(!CanMove(D.Pos,0.34f))continue;
            if(FVector2D::Distance(D.Pos,Exit)<2.2f ||
               FVector2D::Distance(D.Pos,Hero)<1.8f ||
               FVector2D::Distance(D.Pos,Shrine)<1.8f)continue;
            Decorations.Add(D);
        }
    }
    RefreshAwareness(0.f, true);
}

void AAVGameMode::SpawnRoomWave(FAVRoomEvent& Event)
{
    FAVRoomBounds Region;
    Region.X=Event.X;Region.Y=Event.Y;
    Region.W=Event.W;Region.H=Event.H;
    TArray<FVector2D> Occupied;
    for(const FAVEnemy& Mob:Enemies)
        if(Mob.HP>0.f)Occupied.Add(Mob.Pos);
    for(const FAVTrap& Trap:Traps)Occupied.Add(Trap.Pos);
    for(const FAVHazard& Hazard:Hazards)Occupied.Add(Hazard.Pos);
    for(const FAVChest& Chest:Chests)Occupied.Add(Chest.Pos);

    int32 Spawned=0;
    for(int32 I=0;I<Event.Count && Enemies.Num()<96;++I)
    {
        FAVEnemy E;
        if(!FAVDepthDirector::FindSpawn(
            Tiles,MapW,MapH,Region,
            static_cast<EAVFormation>(FMath::Clamp(Event.Formation,0,7)),
            I,Event.Count,Hero,Exit,Shrine,Occupied,E.Pos,3.5f))
            continue; // No stacking inside walls or on the player.
        const uint32 Salt=FAVDepthDirector::Hash(
            static_cast<uint32>(RunSeed) ^
            static_cast<uint32>(Floor)*0x7f4a7c15u ^
            static_cast<uint32>(Event.UniqueId)*0x85ebca6bu ^
            static_cast<uint32>(I)*0x165667b1u);
        E.Type=FAVEncounters::SelectType(Floor,
            static_cast<EAVEncounterKind>(FMath::Clamp(Event.Kind,0,3)),
            static_cast<int32>(Salt%100u),
            static_cast<int32>((Salt>>8)%100u),
            static_cast<int32>((Salt>>16)%100u));
        E.Level=FAVContent::EnemyLevel(Floor,static_cast<int32>((Salt>>20)%100u));
        if(E.Type>=34&&E.Type<=51)E.Subclass=static_cast<int32>((Salt>>27)&1u);
        E.Equipment=(Event.Kind==3 && I==0) ? 1 : 0;
        E.bElite=(Event.Kind==3 && I==0);
        E.Affix=FAVAffixes::Roll(Salt,Floor,E.Type,
            400000+((Floor%500)*1000)+Event.UniqueId*12+I,
            E.bElite,false);
        E.MaxHP=FAVContent::ScaledEnemyHealth(E.Type,Floor,E.Level)*
            (E.Equipment==1?1.35f:1.f)*(E.bElite?1.65f:1.f)*
            FAVAffixes::Traits(E.Affix,1.f,0.f).MaxHealthScale;
        E.HP=E.MaxHP;E.Cooldown=0.65f+0.2f*(I%3);
        // Reserved, stable across cold loads. Do not collide with ordinary
        // mob IDs (1..N), minibosses or boss summons.
        E.Id=400000+((Floor%500)*1000)+Event.UniqueId*12+I;
        Occupied.Add(E.Pos);
        Enemies.Add(E);
        ++Spawned;
        AddEffect(E.Pos,1,0.65f);
    }
    Event.bTriggered=true;
    Event.bArmed=false;
    Toast=Spawned>0?TEXT("REINFORCEMENTS ARRIVE") :
        TEXT("REINFORCEMENTS DISPERSED");
    ToastTime=2.2f;
}

void AAVGameMode::UpdateRoomEvents(float DeltaSeconds)
{
    if(bInSafeCamp||bInLobby||bInBossArena||Phase!=EAVPhase::Playing)return;
    for(FAVRoomEvent& Event:RoomEvents)
    {
        if(Event.bTriggered)continue;
        FAVRoomBounds Region;
        Region.X=Event.X;Region.Y=Event.Y;
        Region.W=Event.W;Region.H=Event.H;
        const bool bInside=FAVDepthDirector::InRoom(Region,Hero);
        if(!Event.bArmed)
        {
            bool bLiveEnemyInside=false;
            for(const FAVEnemy& Mob:Enemies)
                if(Mob.HP>0.f&&FAVDepthDirector::InRoom(Region,Mob.Pos))
                {bLiveEnemyInside=true;break;}
            bool bOpenedChest=false;
            if(Event.Trigger==static_cast<int32>(EAVWaveTrigger::OnChestOpen))
                for(const FAVChest& Chest:Chests)
                    if(Chest.bOpened &&
                       FAVDepthDirector::InRoom(Region,Chest.Pos))
                    {bOpenedChest=true;break;}
            if(FAVDepthDirector::ShouldArmWave(
                static_cast<EAVWaveTrigger>(Event.Trigger),
                bInside,bLiveEnemyInside,bOpenedChest,Event.bKillWitnessed))
            {
                Event.bArmed=true;
                Toast=TEXT("WARNING: HOSTILE REINFORCEMENTS");
                ToastTime=2.3f;
                AddEffect(Event.Center,1,1.4f);
            }
        }
        else if(FAVDepthDirector::AdvanceWarning(
            Event.Countdown,DeltaSeconds))
            SpawnRoomWave(Event);
    }
}

void AAVGameMode::AddEffect(const FVector2D& Pos, int32 Kind, float Radius, float Angle)
{
    FAVEffect Effect;
    Effect.Pos = Pos; Effect.Kind = Kind; Effect.Radius = Radius; Effect.Angle = Angle;
    Effect.Duration = Kind == 3 ? 0.65f : 0.24f;
    Effect.Life = Effect.Duration;
    Effects.Add(Effect);
}

int32 AAVGameMode::ShopPrice(int32 Slot) const
{
    return FAVContent::ShopPrice(Slot, Floor);
}

void AAVGameMode::OfferRelic()
{
    // At most three bosses, so there are always new relics to choose from.
    TArray<int32> Available;
    for (int32 Relic = 0; Relic < FAVContent::RelicCount; ++Relic)
        if (!HasRelic(Relic)) Available.Add(Relic);
    for (int32 I = Available.Num() - 1; I > 0; --I)
        Available.Swap(I, RNG.RandRange(0, I));
    if (Available.Num() == 0)
    {
        bRelicPending = false;
        Phase = XP >= NextXP() ? EAVPhase::Upgrade : EAVPhase::Playing;
        return;
    }
    for (int32 I = 0; I < 3; ++I)
        RelicChoices[I] = Available[I % Available.Num()];
    bRelicPending = true;
    Phase = EAVPhase::Relic;
}

void AAVGameMode::ChooseRelic(int32 Choice)
{
    if (Phase != EAVPhase::Relic || Choice < 0 || Choice > 2) return;
    const int32 Relic = RelicChoices[Choice];
    if (!HasRelic(Relic))
    {
        RelicMask |= 1u << Relic;
        ++RelicCount;
        if (Relic == 4) MaxStamina += 20.f;
        if (Relic == 0) { MaxHP += 15.f; HP = FMath::Min(MaxHP, HP + 35.f); }
        if (Relic == 3) AttackPower += 4.f;
        Stamina = MaxStamina;
    }
    bRelicPending = false;
    Toast = TEXT("ANCIENT RELIC ACQUIRED");
    ToastTime = 2.7f;
    Phase = XP >= NextXP() ? EAVPhase::Upgrade : EAVPhase::Playing;
}

void AAVGameMode::BuyFromMerchant(int32 Slot)
{
    if (Phase != EAVPhase::Shop || Slot < 0 || Slot >= 3) return;
    if (ShopPurchased[Slot])
    {
        Toast = TEXT("SOLD OUT"); ToastTime = 1.4f;
        return;
    }
    const int32 Price = ShopPrice(Slot);
    if (Gold < Price)
    {
        Toast = TEXT("NOT ENOUGH GOLD"); ToastTime = 1.4f;
        return;
    }
    if (Slot == 0 && HP >= MaxHP)
    {
        Toast = TEXT("HEALTH ALREADY FULL"); ToastTime = 1.4f;
        return;
    }
    Gold -= Price;
    ShopPurchased[Slot] = true;
    switch (Slot)
    {
    case 0: HP = FMath::Min(MaxHP, HP + 45.f); break;
    case 1: AttackPower += 5.f; break;
    case 2: ++Potions; break;
    default: break;
    }
    Toast = TEXT("PURCHASE COMPLETE"); ToastTime = 1.5f;
}

void AAVGameMode::StrikeProps(bool bSpin, float Damage)
{
    const float Range = bSpin ? 2.2f : 1.9f;
    for (int32 I = Props.Num() - 1; I >= 0; --I)
    {
        FAVProp& Prop = Props[I];
        const FVector2D Delta = Prop.Pos - Hero;
        if (Delta.Size() > Range ||
            (!bSpin && FVector2D::DotProduct(SafeDirection(Delta), Aim) < 0.1f) ||
            !FAVDungeonNavigation::LineOfSight(Tiles, MapW, MapH, Hero, Prop.Pos))
            continue;
        Prop.HP -= Damage;
        AddEffect(Prop.Pos, 2, 0.5f);
        if (Prop.HP > 0.f) continue;
        const FVector2D Site = Prop.Pos;
        Props.RemoveAtSwap(I);
        AddEffect(Site, 3, 0.7f);
        FAVPickup Drop;
        Drop.Pos = Site;
        Drop.Type = RNG.FRand() < 0.20f ? 1 : 0;
        Drop.Amount = Drop.Type == 1 ? 20 : RNG.RandRange(3, 10);
        Pickups.Add(Drop);
    }
}

void AAVGameMode::Strike(bool bSpin)
{
    const float Cost = bSpin ? (HasRelic(3) ? 24.f : 32.f) : 0.f;
    if (Stamina < Cost || (bSpin ? SpinCooldown > 0.f : AttackCooldown > 0.f)) return;
    Stamina -= Cost;
    if (bSpin) {
        SpinCooldown=1.65f*FMath::Max(0.5f,
            1.f+GearStat(AVCooldownPercent)*0.01f);
        if(HeroClass==0 && SkillRank(17)>0) GuardWhirlTime=1.6f;
    }
    else {
        AttackCooldown=BasicAttackInterval();
        ++MeleeSwingCount;
    }
    AddEffect(Hero, bSpin ? 1 : 0, bSpin ? 2.2f : 1.75f, FMath::Atan2(Aim.Y, Aim.X));
    const int32 Pattern=EquippedWeaponPattern();
    const FAVMeleeStyle Style=FAVItemCombat::Melee(Pattern,bSpin);
    const float Range=Style.Range;
    const float Arc=Style.AimThreshold;
    const float Damage=AttackPower*
        (bSpin?(HasRelic(3)?2.f:1.65f):Style.DamageScale);
    for (int32 I = Enemies.Num() - 1; I >= 0; --I)
    {
        FAVEnemy& Enemy = Enemies[I];
        FVector2D Delta = Enemy.Pos - Hero;
        const float Distance = Delta.Size();
        if (Distance > Range + (FAVContent::Enemy(Enemy.Type, Floor).Boss ? 0.45f : 0.f)) continue;
        if (!bSpin && FVector2D::DotProduct(SafeDirection(Delta), Aim) < Arc) continue;
        // Fix: sword swings can no longer hit actors through solid stone walls.
        if (!FAVDungeonNavigation::LineOfSight(Tiles, MapW, MapH, Hero, Enemy.Pos)) continue;
        // Crypt guards and armored enemies block frontal strikes.
        const bool bShield = Enemy.Type == 20 || Enemy.Type == 36 ||
                             Enemy.Type == 53 || Enemy.Equipment == 1;
        const bool bFacing = FVector2D::DotProduct(SafeDirection(Hero - Enemy.Pos),
                                                   SafeDirection(Enemy.LastSeenHero - Enemy.Pos)) > 0.25f;
        float HitDamage=ModifiedAttackDamage(Damage,Enemy,false,bSpin);
        if(HeroClass==1 && !bSpin && SkillRank(25)>0 &&
            (MeleeSwingCount%3)==0) HitDamage*=1.5f;
        if(bShield && bFacing) HitDamage*=0.58f;
        ApplyDamageToEnemy(Enemy,HitDamage);
        if(Style.Status==1)Enemy.PoisonTime=FMath::Max(Enemy.PoisonTime,3.5f);
        if(Style.Status==2)Enemy.BurnTime=FMath::Max(Enemy.BurnTime,3.f);
        if(Style.HealOnHit>0.f)HP=FMath::Min(MaxHP,HP+Style.HealOnHit);
        Enemy.Flash = 0.16f;
        Enemy.Knockback += SafeDirection(Delta) *
            (Enemy.Type == 3 ? 1.f : 5.f + Style.KnockbackBonus +
             (HasGearPerk(1)?2.f:0.f) +
             (HeroClass==0 ? 0.5f*SkillRank(16) : 0.f));
        AddEffect(Enemy.Pos,2,0.55f);
        if(HasGearPerk(3) && (++BlooddrinkerHitCount%5)==0)
            HP=FMath::Min(MaxHP,HP+3.f);
        if(Enemy.HP<=0.f) KillEnemy(I);
    }
    if(HeroClass==0 && SkillRank(20)>0 && !bSpin &&
       (MeleeSwingCount%3)==0)
        for(int32 I=0;I<8;++I) {
            const float A=6.2831853f*I/8.f;
            FAVBolt Wave;
            Wave.bFriendly=true; Wave.Pos=Hero;
            Wave.Velocity=FVector2D(FMath::Cos(A),FMath::Sin(A))*7.f;
            Wave.Time=0.35f;
            Wave.Damage=FMath::RoundToInt(AttackPower*0.5f);
            Bolts.Add(Wave);
        }
    if(HeroClass==2 && bSpin &&
       SkillRank(FAVSkills::ArcanistRuneWaveSkillId)>0) {
        const int32 Count=
            SkillRank(FAVSkills::ArcanistRuneWaveSkillId)>=2?12:8;
        for(int32 I=0;I<Count;++I) {
            const float A=6.2831853f*I/Count;
            FAVBolt Wave; Wave.bFriendly=true; Wave.Pos=Hero;
            Wave.Velocity=FVector2D(FMath::Cos(A),FMath::Sin(A))*6.f;
            Wave.Time=0.55f; Wave.Visual=4;
            Wave.Damage=FMath::RoundToInt(AttackPower*0.35f);
            Bolts.Add(Wave);
        }
    }
    if(Phase==EAVPhase::Playing) StrikeProps(bSpin,Damage);
}

void AAVGameMode::ShootArrow()
{
    if (HeroClass != 3 || AttackCooldown > 0.f || Aim.IsNearlyZero()) return;
    AttackCooldown=BasicAttackInterval();
    FAVBolt Arrow;
    Arrow.bFriendly = true;
    Arrow.Pos = Hero + Aim * 0.52f;
    Arrow.Velocity=Aim*(13.f*ProjectileSpeedMultiplier());
    Arrow.Time=8.5f/(13.f*ProjectileSpeedMultiplier());
    Arrow.Damage = FMath::RoundToInt(AttackPower + RangedBonus);
    const FAVProjectileStyle Style=FAVItemCombat::Projectile(
        EquippedWeaponPattern());
    Arrow.PierceRemaining=Style.PierceCount;
    Arrow.Damage=FMath::RoundToInt(Arrow.Damage*Style.DamageScale);
    Arrow.Velocity*=Style.SpeedScale;
    Arrow.Status=Style.Status;
    Arrow.Time=8.5f/FMath::Max(1.f,Arrow.Velocity.Size());
    Arrow.Visual = 1;
    Arrow.ShotId=++ShotCounter;
    if(HasGearPerk(5) && ShotCounter%4==0) Arrow.PierceRemaining=2;
    Bolts.Add(Arrow);
    if(Style.ExtraShots>0)
    {
        FAVBolt Twin=Arrow;
        const float A=0.10f;
        Twin.Velocity=FVector2D(Arrow.Velocity.X*FMath::Cos(A)-
            Arrow.Velocity.Y*FMath::Sin(A),
            Arrow.Velocity.X*FMath::Sin(A)+Arrow.Velocity.Y*FMath::Cos(A));
        Twin.Damage=FMath::Max(1,FMath::RoundToInt(Arrow.Damage*0.60f));
        Twin.ShotId=++ShotCounter;
        Bolts.Add(Twin);
    }
    AddEffect(Hero, 0, 0.5f, FMath::Atan2(Aim.Y, Aim.X));
}

void AAVGameMode::FireVolley()
{
    const float Cost = HasRelic(3) ? 24.f : 30.f;
    if (HeroClass != 3 || Stamina < Cost || SpinCooldown > 0.f || Aim.IsNearlyZero()) return;
    Stamina -= Cost;
    SpinCooldown=FMath::Max(1.4f,3.4f*(1.f+GearStat(AVCooldownPercent)*0.01f));
    const float Angle=FMath::Atan2(Aim.Y,Aim.X);
    const int32 VolleyId = ++VolleyCounter;
    const int32 Spread=SkillRank(35)>=2?2:1;
    for(int32 I=-Spread;I<=Spread;++I)
    {
        const float A=Angle+I*(12.f*3.14159265f/180.f);
        const FVector2D Direction(FMath::Cos(A), FMath::Sin(A));
        FAVBolt Arrow;
        Arrow.bFriendly = true;
        Arrow.Pos = Hero + Direction * 0.52f;
        Arrow.Velocity=Direction*(13.f*ProjectileSpeedMultiplier());
        Arrow.Time=8.5f/(13.f*ProjectileSpeedMultiplier());
        Arrow.Damage = FMath::RoundToInt((AttackPower + RangedBonus) *
                           (HasRelic(3)?0.90f:0.75f)*(1.f+0.1f*SkillRank(35)));
        Arrow.Visual = 2;
        Arrow.VolleyId = VolleyId;
        Bolts.Add(Arrow);
    }
    AddEffect(Hero, 1, 0.75f, Angle);
}

void AAVGameMode::PiercingShot()
{
    if (HeroClass != 3 || KnifeCooldown > 0.f || Stamina < 20.f || Aim.IsNearlyZero()) return;
    Stamina -= 20.f;
    KnifeCooldown = 2.f;
    FAVBolt Arrow;
    Arrow.bFriendly = true;
    Arrow.Pos = Hero + Aim * 0.52f;
    Arrow.Velocity=Aim*(17.f*ProjectileSpeedMultiplier());
    Arrow.Time=10.f/(17.f*ProjectileSpeedMultiplier());
    Arrow.Damage=FMath::RoundToInt((AttackPower+RangedBonus)*
        (HasRelic(2)?1.736f:1.55f)*(1.f+0.1f*SkillRank(36)));
    Arrow.Visual = 3;
    Arrow.PierceRemaining = 2;
    Bolts.Add(Arrow);
    if(SkillRank(37)>0) {
        const float A=FMath::Atan2(Aim.Y,Aim.X);
        for(int32 S=-1;S<=1;S+=2) {
            const float B=A+S*0.21f;
            FAVBolt Extra=Arrow;
            Extra.Velocity=FVector2D(FMath::Cos(B),FMath::Sin(B))*
                (17.f*ProjectileSpeedMultiplier());
            Extra.Damage=FMath::RoundToInt(Extra.Damage*0.62f);
            Bolts.Add(Extra);
        }
    }
    AddEffect(Hero,0,0.8f,FMath::Atan2(Aim.Y,Aim.X));
}

void AAVGameMode::ThrowKnife()
{
    if (KnifeCooldown > 0.f || Stamina < 18.f || Aim.IsNearlyZero()) return;
    Stamina -= 18.f;
    KnifeCooldown=(HasRelic(2)?0.43f:0.65f)*
        FMath::Max(0.5f,1.f+GearStat(AVCooldownPercent)*0.01f);
    FAVBolt Blade;
    Blade.bFriendly = true;
    Blade.Pos = Hero + Aim * 0.52f;
    Blade.Velocity=Aim*(11.f*ProjectileSpeedMultiplier());
    Blade.Time=1.1f/ProjectileSpeedMultiplier();
    Blade.Damage=FMath::RoundToInt(AttackPower*
        (HasRelic(2)?1.12f:0.83f)+RangedBonus);
    const int32 Pattern=EquippedWeaponPattern();
    Blade.Status=FAVItemCombat::Projectile(Pattern).Status;
    if(Pattern==2){Blade.Velocity*=1.20f;Blade.Time*=0.84f;}
    if(HeroClass==2 &&
       SkillRank(FAVSkills::ArcanistSphereSkillId)>0) {
        Blade.Visual=4; Blade.Damage=FMath::RoundToInt(
            Blade.Damage*(1.f+0.12f*
                SkillRank(FAVSkills::ArcanistSphereSkillId)));
    }
    Bolts.Add(Blade);
    if(HeroClass==2 && HasGearPerk(7) && (++BasicAttackSequence%4)==0) {
        FAVBolt Bonus=Blade;
        Bonus.Velocity=SafeDirection(Aim+FVector2D(-Aim.Y,Aim.X)*0.22f)*
                       (11.f*ProjectileSpeedMultiplier());
        Bonus.Damage=FMath::RoundToInt(Bonus.Damage*0.5f);
        Bolts.Add(Bonus);
    }
    AddEffect(Hero,0,0.5f,FMath::Atan2(Aim.Y,Aim.X));
}

bool AAVGameMode::OpenNearbyCampDoor()
{
    for (int32 I = CampDoors.Num() - 1; I >= 0; --I)
    {
        if (FVector2D::Distance(Hero, CampDoors[I]) > 1.85f) continue;
        const int32 X = FMath::FloorToInt(CampDoors[I].X);
        const int32 Y = FMath::FloorToInt(CampDoors[I].Y);
        Tiles[Y * MapW + X] = 1;
        AddEffect(CampDoors[I], 3, 0.9f);
        CampDoors.RemoveAtSwap(I);
        FlowRefreshTime = 0.f;
        RefreshAwareness(0.f, true);
        Toast = TEXT("BANDIT GATE OPENED");
        ToastTime = 2.f;
        return true;
    }
    return false;
}

bool AAVGameMode::OpenNearbyChest()
{
    for (FAVChest& Chest : Chests)
    {
        if (Chest.bOpened || FVector2D::Distance(Hero, Chest.Pos) >= 1.45f) continue;
        Chest.bOpened = true;
        AddEffect(Chest.Pos, 3, 0.85f);
        if (Chest.bMimic)
        {
            FAVEnemy Mimic;
            Mimic.Type = 4;
            Mimic.Pos = Chest.Pos;
            Mimic.Level = FAVContent::EnemyLevel(Floor, RNG.RandRange(0,99));
            Mimic.MaxHP = FAVContent::ScaledEnemyHealth(Mimic.Type, Floor, Mimic.Level);
            Mimic.HP = Mimic.MaxHP;
            Mimic.Cooldown = 0.35f;
            Mimic.Id = 100000 + Kills + Chests.Num();
            Enemies.Add(Mimic);
            Toast = TEXT("A MIMIC! DODGE!");
        }
        else
        {
            const int32 Treasure = RNG.RandRange(12, 28) * Floor;
            Gold += HasRelic(5) ? FMath::CeilToInt(Treasure * 1.5f) : Treasure;
            FAVPickup Reward;
            Reward.Pos = Chest.Pos;
            Reward.Type = RNG.FRand() < 0.22f ? 4 : RNG.FRand() < 0.55f ? 3 : 1;
            Reward.Amount = Reward.Type == 4 ? 2 : Reward.Type == 3 ? 1 : 30;
            Pickups.Add(Reward);
            if(RNG.FRand()<0.38f) SpawnGearDrop(Chest.Pos);
            Toast = TEXT("TREASURE FOUND!");
        }
        ToastTime = 2.3f;
        return true;
    }
    return false;
}

bool AAVGameMode::ActivateNearbyEcho()
{
    for (FAVDecor& Decor : Decorations)
    {
        if (Decor.bActivated || (Decor.Type >= 12 || Decor.Type % 3 != 2) ||
            FVector2D::Distance(Decor.Pos, Hero) >= 1.7f)
            continue;
        Decor.bActivated = true;
        ++Echoes;
        Gold += 5 * Floor;
        MaxStamina += 4.f;
        Stamina = FMath::Min(MaxStamina, Stamina + 18.f);
        // Echoes map a nearby chamber without revealing live enemies.
        TArray<uint8> TemporaryVisibility;
        FAVDungeonNavigation::Reveal(Tiles, MapW, MapH, Decor.Pos,
                                     9.f, TemporaryVisibility, Explored);
        Toast = TEXT("ECHO RECLAIMED: +GOLD / +STAMINA / MAP");
        ToastTime = 2.9f;
        AddEffect(Decor.Pos, 1, 2.7f);
        return true;
    }
    return false;
}

void AAVGameMode::DamageHero(float Damage,const FVector2D& Source,
                             bool bMeleeAttack)
{
    if (Invulnerability > 0.f || Phase != EAVPhase::Playing || bInSafeCamp || bInLobby) return;
    float AdjustedDamage=Damage*(HasRelic(1)?0.80f:1.f)*
        (1.f-FMath::Clamp(ArmorRating,0.f,0.45f));
    if(bMeleeAttack)
        AdjustedDamage*=FAVItemCombat::IncomingMeleeMultiplier(
            GearStat(AVMeleeResistPercent),FoodGuardTime>0.f);
    if(HP<MaxHP*0.3f) AdjustedDamage*=1.f-0.08f*SkillRank(3);
    if(HeroClass==0 && GuardWhirlTime>0.f)
        AdjustedDamage*=1.f-0.08f*SkillRank(17);
    if(TemporaryShield>0.f) {
        const float Absorbed=FMath::Min(TemporaryShield,AdjustedDamage);
        TemporaryShield-=Absorbed;
        AdjustedDamage-=Absorbed;
    }
    if(AdjustedDamage>0.f) RevengeTime=3.f;
    HP=FMath::Max(0.f,HP-AdjustedDamage);
    if(HP<=0.f && SkillRank(4)>0 && !bLastStandUsed) {
        bLastStandUsed=true; HP=1.f;
        Invulnerability=FMath::Max(Invulnerability,0.7f);
    }
    if(HP<MaxHP*0.25f && HeroClass==2 && SkillRank(32)>0 &&
       TemporaryShield<=0.f && !bMagicBarrierUsed) {
        TemporaryShield=MaxHP*0.18f; bMagicBarrierUsed=true;
    }
    Invulnerability = 0.72f;
    ScreenShake = 0.26f;
    const float KnockbackScale=FMath::Clamp(1.f-
        GearStat(AVKnockbackResistPercent)*0.01f,0.2f,1.f);
    Hero=MoveWithCollision(Hero,SafeDirection(Hero-Source)*
         2.8f*KnockbackScale,0.10f,0.27f);
    AddEffect(Hero, 2, 0.8f);
    if (HP <= 0.f)
    {
        Phase = EAVPhase::Dead;
        RecordRun();
    }
}

void AAVGameMode::DropLoot(const FVector2D& Pos, bool bBoss)
{
    const int32 Count = bBoss ? 13 : RNG.RandRange(1, 3);
    for (int32 I = 0; I < Count; ++I)
    {
        FAVPickup Pickup;
        Pickup.Pos = Pos + FVector2D(RNG.FRandRange(-0.8f, 0.8f), RNG.FRandRange(-0.8f, 0.8f));
        if (!CanMove(Pickup.Pos, 0.1f)) Pickup.Pos = Pos;
        Pickup.Type = 0;
        Pickup.Amount = bBoss ? RNG.RandRange(12, 20) : RNG.RandRange(2, 7);
        if (HasRelic(5)) Pickup.Amount = FMath::CeilToInt(Pickup.Amount * 1.5f);
        Pickups.Add(Pickup);
    }
    if (RNG.FRand() < (bBoss ? 1.f : 0.10f))
    {
        FAVPickup Pickup; Pickup.Pos = Pos;
        Pickup.Type = 3; Pickup.Amount = 1;
        Pickups.Add(Pickup);
    }
}

float AAVGameMode::ApplyDamageToEnemy(FAVEnemy& Enemy,float RawDamage)
{
    const float Actual=FAVAffixes::TakeDamage(
        Enemy.Affix,RawDamage,Enemy.bWardConsumed);
    Enemy.HP-=Actual;
    return Actual;
}

void AAVGameMode::OnAffixedEnemyHitHero(FAVEnemy& Enemy,float ActualDamage)
{
    if(Enemy.Affix==static_cast<int32>(EAVEnemyAffix::Blooddrinker))
        Enemy.HP=FMath::Min(Enemy.MaxHP,Enemy.HP+ActualDamage*0.20f);
    const int32 Status=FAVAffixes::Traits(
        Enemy.Affix,Enemy.HP/FMath::Max(1.f,Enemy.MaxHP),
        Enemy.AlertTime).InflictedStatus;
    if(Status==1)PoisonTime=FMath::Max(PoisonTime,
        2.4f*StatusResistanceFactor(true));
    else if(Status==2)BurnTime=FMath::Max(BurnTime,
        2.f*StatusResistanceFactor(false));
}

void AAVGameMode::KillEnemy(int32 Index)
{
    if (!Enemies.IsValidIndex(Index)) return;
    const FAVEnemy Dead = Enemies[Index];
    for(FAVRoomEvent& Event:RoomEvents)
    {
        if(Event.bTriggered || Event.Trigger!=
            static_cast<int32>(EAVWaveTrigger::OnFirstKill))continue;
        FAVRoomBounds Bounds;
        Bounds.X=Event.X;Bounds.Y=Event.Y;Bounds.W=Event.W;Bounds.H=Event.H;
        if(FAVDepthDirector::InRoom(Bounds,Dead.Pos))
            Event.bKillWitnessed=true;
    }
    const bool bBoss = FAVContent::Enemy(Dead.Type, Floor).Boss;
    Enemies.RemoveAtSwap(Index);
    ++Kills;
    if (HasRelic(0)) HP = FMath::Min(MaxHP, HP + 3.f);
    Combo = ComboTime > 0.f ? Combo + 1 : 1;
    ComboTime = 4.5f;
    if (Combo % 5 == 0)
    {
        Gold += 5;
        Toast = TEXT("COMBO BONUS  +5 GOLD");
        ToastTime = 1.5f;
    }
    const int32 XPReward=(bBoss ? 22+Floor*5 :
        (3+Floor)*(Dead.bElite?2:1))+
        FAVAffixes::BonusExperience(Floor,Dead.Affix);
    GrantExperience(XPReward);
    Gold+=FAVAffixes::BonusGold(Floor,Dead.Affix);
    if (Dead.Equipment != 0) ++PackMaterials;
    if (Dead.Equipment != 0 && RNG.FRand() < 0.14f)
    {
        FAVPickup Gear;
        Gear.Pos = Dead.Pos;
        Gear.Type = Dead.Equipment == 1 ? 8 : Dead.Equipment == 2 ? 9 : 10;
        Gear.Amount = 1;
        Pickups.Add(Gear);
    }
    AddEffect(Dead.Pos, 3, bBoss ? 2.2f : Dead.bMiniBoss ? 1.7f : 0.9f);
    DropLoot(Dead.Pos, bBoss || Dead.bMiniBoss);
    if (bBoss || (Dead.bMiniBoss && RNG.FRand()<0.8f) ||
        (Dead.bElite && RNG.FRand()<0.28f) ||
        (!Dead.bMiniBoss && !Dead.bElite && RNG.FRand()<0.075f))
        SpawnGearDrop(Dead.Pos,bBoss || Dead.bMiniBoss ? AVRare : AVUncommon);
    if(HasGearPerk(6)) HP=FMath::Min(MaxHP,HP+2.f);
    if(SkillRank(24)>0 && HeroClass==1) LastKillBoostTime=3.f;
    if(Dead.bElite && HasGearPerk(9)) RewardDamageTime=8.f;
    if(SkillRank(9)>0) {
        SpinCooldown=FMath::Max(0.f,SpinCooldown-0.5f);
        KnifeCooldown=FMath::Max(0.f,KnifeCooldown-0.5f);
    }
    if (Dead.bElite)
    {
        FAVPickup Shard;
        Shard.Pos = Dead.Pos;
        Shard.Type = 4;
        Shard.Amount = 2;
        Pickups.Add(Shard);
    }
    // Rare run-long upgrade drops: vitality, endurance and haste shards.
    if (RNG.FRand() < (bBoss ? 0.85f : 0.075f))
    {
        FAVPickup Upgrade;
        Upgrade.Pos = Dead.Pos;
        Upgrade.Type = RNG.RandRange(5, 7);
        Upgrade.Amount = 1;
        Pickups.Add(Upgrade);
    }
    if (bBoss)
    {
        if (bInBossArena) bGuardianDefeated = true;
        OfferRelic();
    }
}

void AAVGameMode::GrantExperience(int32 Amount)
{
    XP += Amount;
    if (Phase == EAVPhase::Playing && XP >= NextXP()) Phase = EAVPhase::Upgrade;
}

void AAVGameMode::SelectUpgrade(int32 Choice)
{
    if (Phase != EAVPhase::Upgrade) return;
    XP -= NextXP();
    ++Level;
    if(Level<=30) ++SkillPoints;
    if (Choice == 0) { MaxHP += 25.f; HP = FMath::Min(MaxHP, HP + 40.f); }
    if (Choice == 1) AttackPower += 7.f;
    if (Choice == 2) { MoveSpeed += 0.55f; MaxStamina += 12.f; Stamina = MaxStamina; }
    RebuildCharacterBonuses();
    Phase = XP >= NextXP() ? EAVPhase::Upgrade : EAVPhase::Playing;
}


void AAVGameMode::StoreDungeon()
{
    StoredWorld.Tiles = Tiles; StoredWorld.Visible = Visible; StoredWorld.Explored = Explored;
    StoredWorld.FlowDistances = FlowDistances; StoredWorld.Enemies = Enemies;
    StoredWorld.Pickups = Pickups; StoredWorld.Bolts = Bolts; StoredWorld.Traps = Traps;
    StoredWorld.Chests = Chests; StoredWorld.Hazards = Hazards;
    StoredWorld.Props = Props; StoredWorld.Decorations = Decorations;
    StoredWorld.Effects = Effects; StoredWorld.RoomEvents=RoomEvents;
    StoredWorld.CampDoors = CampDoors;
    StoredWorld.CampWalls = CampWalls;
    StoredWorld.Hero = Hero; StoredWorld.Exit = Exit;
    StoredWorld.Shrine = Shrine; StoredWorld.Merchant = Merchant;
    StoredWorld.BanditCampCenter = BanditCampCenter;
    StoredWorld.bMerchantSpawned = bMerchantSpawned;
    StoredWorld.bBanditCamp = bBanditCamp;
    StoredWorld.bShrineUsed = bShrineUsed;
    bWorldStored = true;
}

void AAVGameMode::RestoreDungeon()
{
    if (!bWorldStored) return;
    Tiles = StoredWorld.Tiles; Visible = StoredWorld.Visible;
    Explored = StoredWorld.Explored; FlowDistances = StoredWorld.FlowDistances;
    Enemies = StoredWorld.Enemies; Pickups = StoredWorld.Pickups;
    Bolts = StoredWorld.Bolts; Traps = StoredWorld.Traps;
    Chests = StoredWorld.Chests; Hazards = StoredWorld.Hazards;
    Props = StoredWorld.Props; Decorations = StoredWorld.Decorations;
    Effects = StoredWorld.Effects; RoomEvents=StoredWorld.RoomEvents;
    CampDoors = StoredWorld.CampDoors;
    CampWalls = StoredWorld.CampWalls;
    Hero = StoredWorld.Hero; Exit = StoredWorld.Exit;
    Shrine = StoredWorld.Shrine; Merchant = StoredWorld.Merchant;
    BanditCampCenter = StoredWorld.BanditCampCenter;
    bMerchantSpawned = StoredWorld.bMerchantSpawned;
    bBanditCamp = StoredWorld.bBanditCamp;
    bShrineUsed = StoredWorld.bShrineUsed;
    bWorldStored = false;
    bInSafeCamp = false; bInBossArena = false;
    LastVisionX = -1; LastVisionY = -1;
    RefreshAwareness(0.f, true);
}

void AAVGameMode::MakeSafeRoom(bool bLobby)
{
    Tiles.Init(0, MapW * MapH);
    for (int32 Y = 12; Y <= 26; ++Y)
        for (int32 X = 14; X <= 38; ++X)
            Tiles[Y * MapW + X] = 1;
    Visible.Init(0, MapW * MapH); Explored.Init(0, MapW * MapH);
    FlowDistances.Init(-1, MapW * MapH);
    Enemies.Reset(); Bolts.Reset(); Traps.Reset(); Hazards.Reset();
    Chests.Reset(); Pickups.Reset(); Props.Reset(); Decorations.Reset();
    CampDoors.Reset(); CampWalls.Reset(); Effects.Reset();
    RoomEvents.Reset(); // safe spaces and guardian arenas have no scripted waves
    bMerchantSpawned = !bLobby;
    Merchant = FVector2D(20.5f, 17.5f);
    Hero = FVector2D(26.5f, 22.5f);
    Exit = FVector2D(26.5f, 25.5f);
    Shrine = FVector2D(32.5f, 17.5f);
    HeroVelocity = FVector2D::ZeroVector;
    LastVisionX = -1; LastVisionY = -1;
    RefreshAwareness(0.f, true);
}

void AAVGameMode::EnterSafeCamp()
{
    if (bInSafeCamp || bInBossArena || bInLobby) return;
    StoreDungeon();
    bInSafeCamp = true;
    MakeSafeRoom(false);
    Toast = TEXT("SAFE CAMP: MERCHANT / SMITH / TRAVELER");
    ToastTime = 3.f;
}

void AAVGameMode::EnterBossArena()
{
    if (bInSafeCamp || bInBossArena || bInLobby || PendingGuardian < 0) return;
    StoreDungeon();
    bInBossArena = true;
    MakeSafeRoom(true);
    // Isolated guardian chamber: a wide octagonal fighting floor.
    Tiles.Init(0, MapW * MapH);
    for (int32 Y = 8; Y <= 30; ++Y)
        for (int32 X = 11; X <= 41; ++X)
            if (FMath::Abs(X - 26) + FMath::Abs(Y - 19) <= 25)
                Tiles[Y * MapW + X] = 1;
    Visible.Init(0, MapW * MapH);
    Explored.Init(0, MapW * MapH);
    Hero = FVector2D(26.5f, 27.5f);
    Exit = FVector2D(26.5f, 29.5f);
    for (int32 I = 0; I < 8; ++I)
    {
        const float Angle = I * 6.2831853f / 8.f;
        FAVDecor Pillar;
        Pillar.Type = CurrentBiome() * 3 + (I % 3);
        Pillar.Pos = FVector2D(26.5f + FMath::Cos(Angle) * 11.f,
                               19.5f + FMath::Sin(Angle) * 8.f);
        Decorations.Add(Pillar);
    }
    FAVEnemy Boss;
    Boss.Type = PendingGuardian;
    Boss.Level = FAVContent::EnemyLevel(Floor, 99);
    Boss.Pos = FVector2D(26.5f, 16.5f);
    Boss.MaxHP = FAVContent::ScaledEnemyHealth(Boss.Type, Floor, Boss.Level);
    Boss.HP = Boss.MaxHP;
    Boss.Cooldown = 1.f;
    Boss.Id = 900000 + Floor;
    Enemies.Add(Boss);
    LastVisionX = -1; LastVisionY = -1;
    RefreshAwareness(0.f, true);
}

void AAVGameMode::SellPack()
{
    Gold += PackMaterials * FMath::Max(1, 3 + Floor / 5) +
        BackpackArmor * 18 + BackpackBows * 15 + BackpackWeapons * 20;
    for(int32 Id : InventoryItems) {
        const FAVEquipmentSpec* Item=FAVEquipment::Find(Id);
        if(Item) Gold += (8 + 12 * Item->Rarity) * FMath::Max(1,1+Floor/5);
    }
    InventoryItems.Reset();
    PackGold=0; PackMaterials=0;
    BackpackArmor=0; BackpackBows=0; BackpackWeapons=0;
}

void AAVGameMode::ForgeUpgrade(int32 Slot)
{
    const int32 Price = 25 + 15 * (Slot == 0 ? WeaponUpgrade : ArmorUpgrade);
    if (Gold < Price)
    {
        Toast = TEXT("NOT ENOUGH GOLD"); ToastTime = 2.f;
        return;
    }
    Gold -= Price;
    if (Slot == 0) { ++WeaponUpgrade; AttackPower += 3.f; }
    else { ++ArmorUpgrade; ArmorRating = FMath::Min(0.45f, ArmorRating + 0.04f); }
    Toast = TEXT("FORGE UPGRADE COMPLETE"); ToastTime = 2.f;
}

void AAVGameMode::SaveCheckpoint()
{
    UAVSaveGame* Save = Cast<UAVSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UAVSaveGame::StaticClass()));
    if (!Save) return;
    Save->BestFloor = FMath::Max(BestFloor, Floor);
    Save->BestKills = FMath::Max(BestKills, Kills);
    Save->TotalGold = LifetimeGold;
    Save->bHasCheckpoint = true;
    Save->CheckpointFloor = Floor; Save->CheckpointSeed = RunSeed;
    Save->CheckpointHeroClass = HeroClass;
    Save->CheckpointLevel = Level; Save->CheckpointXP = XP;
    Save->CheckpointGold = Gold; Save->CheckpointKills = Kills;
    Save->CheckpointPotions = Potions; Save->CheckpointGear = EquippedGear;
    Save->CheckpointEquippedMask = EquippedSlotsMask;
    Save->CheckpointWeaponUpgrade = WeaponUpgrade;
    Save->CheckpointArmorUpgrade = ArmorUpgrade;
    Save->CheckpointRelics = RelicMask;
    Save->CheckpointHP = HP; Save->CheckpointMaxHP = MaxHP;
    Save->CheckpointStamina = Stamina; Save->CheckpointMaxStamina = MaxStamina;
    Save->CheckpointAttack = AttackPower; Save->CheckpointSpeed = MoveSpeed;
    // v0.9.0 stores the base values, so gear and skills never double apply.
    Save->CheckpointMaxHP=MaxHP-EquipmentAppliedHP;
    Save->CheckpointMaxStamina=MaxStamina-EquipmentAppliedStamina;
    Save->CheckpointAttack=AttackPower-EquipmentAppliedAttack;
    Save->CheckpointSpeed=MoveSpeed-EquipmentAppliedSpeed;
    Save->CheckpointArmor=ArmorRating-EquipmentAppliedArmor;
    Save->CheckpointRanged=RangedBonus;
    Save->bCheckpointV09Data=true;
    Save->bCheckpointV13Data=true;
    Save->bCheckpointV15Data=true;
    Save->CheckpointFoodAttackTime=FoodAttackTime;
    Save->CheckpointFoodGuardTime=FoodGuardTime;
    Save->CheckpointFoodSpeedTime=FoodSpeedTime;
    Save->CheckpointFoodCritTime=FoodCritTime;
    Save->CheckpointFoodProjectileTime=FoodProjectileTime;
    Save->CheckpointFoodBossTime=FoodBossTime;
    Save->CheckpointFoodPoisonWardTime=FoodPoisonWardTime;
    Save->CheckpointMapWidth=MapW; Save->CheckpointMapHeight=MapH;
    Save->CheckpointInventoryItems=InventoryItems;
    Save->CheckpointEquipmentSlots=EquipmentSlots;
    Save->CheckpointSkillRanks=LearnedSkills;
    Save->CheckpointSkillPoints=SkillPoints;
    Save->bCheckpointFreeRespecUsed=bFreeRespecUsed;
    Save->bCheckpointLastStandUsed=bLastStandUsed;
    Save->bCheckpointMagicBarrierUsed=bMagicBarrierUsed;
    // Store the dungeon, not the peaceful camp or the lobby.
    const FAVWorldSnapshot* World = bWorldStored ? &StoredWorld : nullptr;
    const TArray<FAVRoomEvent>& SavedRoomEvents =
        World ? World->RoomEvents : RoomEvents;
    const TArray<FAVEnemy>& SavedEnemies = World ? World->Enemies : Enemies;
    const TArray<FAVPickup>& SavedPickups = World ? World->Pickups : Pickups;
    const TArray<FAVChest>& SavedChests = World ? World->Chests : Chests;
    const TArray<FAVTrap>& SavedTraps = World ? World->Traps : Traps;
    const TArray<FAVHazard>& SavedHazards = World ? World->Hazards : Hazards;
    const TArray<FAVProp>& SavedProps = World ? World->Props : Props;
    const TArray<FAVDecor>& SavedDecor = World ? World->Decorations : Decorations;
    Save->CheckpointTiles = World ? World->Tiles : Tiles;
    Save->CheckpointExplored = World ? World->Explored : Explored;
    Save->CheckpointHero = World ? World->Hero : Hero;
    Save->CheckpointExit = World ? World->Exit : Exit;
    Save->CheckpointShrine = World ? World->Shrine : Shrine;
    Save->CheckpointMerchant = World ? World->Merchant : Merchant;
    Save->CheckpointSafeCampDoor = SafeCampDoor;
    Save->bCheckpointHasSafeCamp = bHasSafeCampDoor;
    Save->bCheckpointShrineUsed = World ? World->bShrineUsed : bShrineUsed;
    Save->bCheckpointBanditCamp = World ? World->bBanditCamp : bBanditCamp;
    Save->bCheckpointGuardianDefeated = bGuardianDefeated;
    Save->CheckpointGuardian = PendingGuardian;
    Save->CheckpointPackMaterials = PackMaterials;
    Save->CheckpointPackArmor = BackpackArmor;
    Save->CheckpointPackBows = BackpackBows;
    Save->CheckpointPackWeapons = BackpackWeapons;
    Save->bCheckpointShopPurchased0 = ShopPurchased[0];
    Save->bCheckpointShopPurchased1 = ShopPurchased[1];
    Save->bCheckpointShopPurchased2 = ShopPurchased[2];
    Save->CheckpointRoomEventData.Reset();
    for(const FAVRoomEvent& Event:SavedRoomEvents)
    {
        const float Fields[]={
            Event.Center.X,Event.Center.Y,
            float(Event.X),float(Event.Y),float(Event.W),float(Event.H),
            float(Event.Trigger),float(Event.Kind),float(Event.Formation),
            float(Event.Count),float(Event.UniqueId),Event.Countdown,
            Event.bArmed?1.f:0.f,Event.bTriggered?1.f:0.f,
            Event.bKillWitnessed?1.f:0.f
        };
        for(float Value:Fields)Save->CheckpointRoomEventData.Add(Value);
    }
    Save->CheckpointEnemyData.Reset();
    for (const FAVEnemy& E : SavedEnemies)
    {
        const float V[] = {E.Pos.X, E.Pos.Y, float(E.Type), float(E.Level),
            float(E.Equipment), float(E.Faction), E.LastSeenHero.X, E.LastSeenHero.Y,
            E.MemoryTime, E.DodgeCooldown, E.AbilityCooldown, E.AlertTime,
            E.PathRefresh, E.CachedPathDirection.X, E.CachedPathDirection.Y,
            E.AttackWindup, E.LockedTarget.X, E.LockedTarget.Y, float(E.AttackPattern),
            E.bMiniBoss ? 1.f : 0.f, E.HP, E.MaxHP, E.Cooldown, E.Flash,
            E.Wander, E.Telegraph, E.bElite ? 1.f : 0.f, float(E.Id),
            E.PhaseShiftTime, E.bPhaseShifted ? 1.f : 0.f,
            float(E.Subclass),float(E.Affix),E.bWardConsumed?1.f:0.f,
            E.PoisonTime,E.BurnTime,E.StatusTick};
        for (float F : V) Save->CheckpointEnemyData.Add(F);
    }
    Save->CheckpointPickupData.Reset();
    for (const FAVPickup& P : SavedPickups)
    {
        Save->CheckpointPickupData.Add(P.Pos.X); Save->CheckpointPickupData.Add(P.Pos.Y);
        Save->CheckpointPickupData.Add(float(P.Type));
        Save->CheckpointPickupData.Add(float(P.Amount));
        Save->CheckpointPickupData.Add(P.Time);
    }
    Save->CheckpointChestData.Reset();
    for (const FAVChest& C : SavedChests)
    {
        Save->CheckpointChestData.Add(C.Pos.X); Save->CheckpointChestData.Add(C.Pos.Y);
        Save->CheckpointChestData.Add(C.bOpened ? 1.f : 0.f);
        Save->CheckpointChestData.Add(C.bMimic ? 1.f : 0.f);
    }
    Save->CheckpointTrapData.Reset();
    for (const FAVTrap& T : SavedTraps)
    {
        Save->CheckpointTrapData.Add(T.Pos.X); Save->CheckpointTrapData.Add(T.Pos.Y);
        Save->CheckpointTrapData.Add(float(T.Type)); Save->CheckpointTrapData.Add(T.Windup);
        Save->CheckpointTrapData.Add(T.Cooldown);
    }
    Save->CheckpointHazardData.Reset();
    for (const FAVHazard& H : SavedHazards)
    {
        Save->CheckpointHazardData.Add(H.Pos.X); Save->CheckpointHazardData.Add(H.Pos.Y);
        Save->CheckpointHazardData.Add(float(H.Type)); Save->CheckpointHazardData.Add(H.Cooldown);
    }
    Save->CheckpointPropData.Reset();
    for (const FAVProp& P : SavedProps)
    {
        Save->CheckpointPropData.Add(P.Pos.X); Save->CheckpointPropData.Add(P.Pos.Y);
        Save->CheckpointPropData.Add(P.HP); Save->CheckpointPropData.Add(float(P.Type));
    }
    Save->CheckpointDecorData.Reset();
    for (const FAVDecor& D : SavedDecor)
    {
        Save->CheckpointDecorData.Add(D.Pos.X); Save->CheckpointDecorData.Add(D.Pos.Y);
        Save->CheckpointDecorData.Add(float(D.Type));
        Save->CheckpointDecorData.Add(D.bActivated ? 1.f : 0.f);
        Save->CheckpointDecorData.Add(D.Phase);
    }
    Save->CheckpointCampDoorData.Reset();
    for (const FVector2D& P : (World ? World->CampDoors : CampDoors))
    {
        Save->CheckpointCampDoorData.Add(P.X); Save->CheckpointCampDoorData.Add(P.Y);
    }
    Save->CheckpointCampWallData.Reset();
    for (const FVector2D& P : (World ? World->CampWalls : CampWalls))
    {
        Save->CheckpointCampWallData.Add(P.X); Save->CheckpointCampWallData.Add(P.Y);
    }
    UGameplayStatics::SaveGameToSlot(Save, TEXT("AshenVaultsRecord"), 0);
}

void AAVGameMode::LoadCheckpointWorld()
{
    UAVSaveGame* Save = Cast<UAVSaveGame>(
        UGameplayStatics::LoadGameFromSlot(TEXT("AshenVaultsRecord"), 0));
    if (!Save || !Save->bHasCheckpoint) { GenerateFloor(); return; }
    GenerateFloor();
    if (Save->CheckpointTiles.Num() != MapW * MapH) return;
    Tiles = Save->CheckpointTiles;
    Explored = Save->CheckpointExplored.Num() == MapW * MapH ?
        Save->CheckpointExplored : Explored;
    Visible.Init(0, MapW * MapH);
    FlowDistances.Init(-1, MapW * MapH);
    Hero = Save->CheckpointHero; Exit = Save->CheckpointExit;
    Shrine = Save->CheckpointShrine; Merchant = Save->CheckpointMerchant;
    SafeCampDoor = Save->CheckpointSafeCampDoor;
    bHasSafeCampDoor = Save->bCheckpointHasSafeCamp;
    bShrineUsed = Save->bCheckpointShrineUsed;
    bBanditCamp = Save->bCheckpointBanditCamp;
    bGuardianDefeated = Save->bCheckpointGuardianDefeated;
    PendingGuardian = Save->CheckpointGuardian;
    PackMaterials = Save->CheckpointPackMaterials;
    BackpackArmor = Save->CheckpointPackArmor;
    BackpackBows = Save->CheckpointPackBows;
    BackpackWeapons = Save->CheckpointPackWeapons;
    ShopPurchased[0] = Save->bCheckpointShopPurchased0;
    ShopPurchased[1] = Save->bCheckpointShopPurchased1;
    ShopPurchased[2] = Save->bCheckpointShopPurchased2;
    RoomEvents.Reset();
    const TArray<float>& RD=Save->CheckpointRoomEventData;
    // Old saves have no scripted room events: continue them unchanged.
    if(RD.Num()%15==0)
        for(int32 I=0;I+14<RD.Num();I+=15)
        {
            FAVRoomEvent Event;
            Event.Center=FVector2D(RD[I],RD[I+1]);
            Event.X=FMath::Clamp(int32(RD[I+2]),0,MapW-1);
            Event.Y=FMath::Clamp(int32(RD[I+3]),0,MapH-1);
            Event.W=FMath::Clamp(int32(RD[I+4]),0,MapW-Event.X);
            Event.H=FMath::Clamp(int32(RD[I+5]),0,MapH-Event.Y);
            Event.Trigger=FMath::Clamp(int32(RD[I+6]),0,4);
            Event.Kind=FMath::Clamp(int32(RD[I+7]),0,3);
            Event.Formation=FMath::Clamp(int32(RD[I+8]),0,7);
            Event.Count=FMath::Clamp(int32(RD[I+9]),0,5);
            Event.UniqueId=FMath::Clamp(int32(RD[I+10]),0,128);
            Event.Countdown=FMath::Clamp(RD[I+11],0.f,5.f);
            Event.bArmed=RD[I+12]!=0.f;
            Event.bTriggered=RD[I+13]!=0.f;
            Event.bKillWitnessed=RD[I+14]!=0.f;
            if(Event.W>=3&&Event.H>=3&&Event.Trigger>=1)
                RoomEvents.Add(Event);
        }
    Enemies.Reset();
    const TArray<float>& ED = Save->CheckpointEnemyData;
    const int32 EnemyStride=Save->bCheckpointV15Data?36:
        Save->bCheckpointV13Data?33:
        Save->bCheckpointV09Data?31:30;
    for (int32 I = 0; I + EnemyStride - 1 < ED.Num(); I += EnemyStride)
    {
        FAVEnemy E;
        E.Pos = FVector2D(ED[I], ED[I + 1]); E.Type = int32(ED[I + 2]);
        E.Level = int32(ED[I + 3]); E.Equipment = int32(ED[I + 4]);
        E.Faction = int32(ED[I + 5]);
        E.LastSeenHero = FVector2D(ED[I + 6], ED[I + 7]);
        E.MemoryTime = ED[I + 8]; E.DodgeCooldown = ED[I + 9];
        E.AbilityCooldown = ED[I + 10]; E.AlertTime = ED[I + 11];
        E.PathRefresh = ED[I + 12];
        E.CachedPathDirection = FVector2D(ED[I + 13], ED[I + 14]);
        E.AttackWindup = ED[I + 15];
        E.LockedTarget = FVector2D(ED[I + 16], ED[I + 17]);
        E.AttackPattern = int32(ED[I + 18]); E.bMiniBoss = ED[I + 19] != 0.f;
        E.HP = ED[I + 20]; E.MaxHP = ED[I + 21]; E.Cooldown = ED[I + 22];
        E.Flash = ED[I + 23]; E.Wander = ED[I + 24];
        E.Telegraph = ED[I + 25]; E.bElite = ED[I + 26] != 0.f;
        E.Id = int32(ED[I + 27]); E.PhaseShiftTime = ED[I + 28];
        E.bPhaseShifted = ED[I + 29] != 0.f;
        if (EnemyStride >= 31) E.Subclass = FMath::Clamp(int32(ED[I+30]),0,1);
        if (EnemyStride >= 33)
        {
            E.Affix=FMath::Clamp(int32(ED[I+31]),0,12);
            E.bWardConsumed=ED[I+32]!=0.f;
        }
        if(EnemyStride>=36)
        {
            E.PoisonTime=FMath::Clamp(ED[I+33],0.f,30.f);
            E.BurnTime=FMath::Clamp(ED[I+34],0.f,30.f);
            E.StatusTick=FMath::Clamp(ED[I+35],0.f,1.f);
        }
        Enemies.Add(E);
    }
    Pickups.Reset();
    const TArray<float>& PD = Save->CheckpointPickupData;
    for (int32 I = 0; I + 4 < PD.Num(); I += 5)
    {
        FAVPickup P; P.Pos = FVector2D(PD[I], PD[I + 1]);
        P.Type = int32(PD[I + 2]); P.Amount = int32(PD[I + 3]);
        P.Time = PD[I + 4]; Pickups.Add(P);
    }
    Chests.Reset();
    const TArray<float>& CD = Save->CheckpointChestData;
    for (int32 I = 0; I + 3 < CD.Num(); I += 4)
    {
        FAVChest C; C.Pos = FVector2D(CD[I], CD[I + 1]);
        C.bOpened = CD[I + 2] != 0.f; C.bMimic = CD[I + 3] != 0.f;
        Chests.Add(C);
    }
    Traps.Reset();
    const TArray<float>& TD = Save->CheckpointTrapData;
    for (int32 I = 0; I + 4 < TD.Num(); I += 5)
    {
        FAVTrap T; T.Pos = FVector2D(TD[I], TD[I + 1]);
        T.Type = int32(TD[I + 2]); T.Windup = TD[I + 3];
        T.Cooldown = TD[I + 4]; Traps.Add(T);
    }
    Hazards.Reset();
    const TArray<float>& HD = Save->CheckpointHazardData;
    for (int32 I = 0; I + 3 < HD.Num(); I += 4)
    {
        FAVHazard H; H.Pos = FVector2D(HD[I], HD[I + 1]);
        H.Type = int32(HD[I + 2]); H.Cooldown = HD[I + 3]; Hazards.Add(H);
    }
    Props.Reset();
    const TArray<float>& PR = Save->CheckpointPropData;
    for (int32 I = 0; I + 3 < PR.Num(); I += 4)
    {
        FAVProp P; P.Pos = FVector2D(PR[I], PR[I + 1]);
        P.HP = PR[I + 2]; P.Type = int32(PR[I + 3]); Props.Add(P);
    }
    Decorations.Reset();
    const TArray<float>& DD = Save->CheckpointDecorData;
    for (int32 I = 0; I + 4 < DD.Num(); I += 5)
    {
        FAVDecor D; D.Pos = FVector2D(DD[I], DD[I + 1]);
        D.Type = int32(DD[I + 2]); D.bActivated = DD[I + 3] != 0.f;
        D.Phase = DD[I + 4]; Decorations.Add(D);
    }
    CampDoors.Reset();
    for (int32 I = 0; I + 1 < Save->CheckpointCampDoorData.Num(); I += 2)
        CampDoors.Add(FVector2D(Save->CheckpointCampDoorData[I],
                                Save->CheckpointCampDoorData[I + 1]));
    CampWalls.Reset();
    for (int32 I = 0; I + 1 < Save->CheckpointCampWallData.Num(); I += 2)
        CampWalls.Add(FVector2D(Save->CheckpointCampWallData[I],
                                Save->CheckpointCampWallData[I + 1]));
    if(Save->bCheckpointV09Data) {
        bLastStandUsed=Save->bCheckpointLastStandUsed;
        bMagicBarrierUsed=Save->bCheckpointMagicBarrierUsed;
    }
    LastVisionX = -1; LastVisionY = -1;
    RefreshAwareness(0.f, true);
}

void AAVGameMode::EnterLobby()
{
    if (bInBossArena) return;
    if (bInSafeCamp) SellPack();
    bInLobby = true; bInSafeCamp = false;
    // Keep the exact dungeon snapshot for same-session continuation.
    SaveCheckpoint();
    MakeSafeRoom(true);
    Toast = TEXT("HOME: PRESS E AT THE PORTAL TO RESUME");
    ToastTime = 3.f;
}

void AAVGameMode::ResumeFromLobby()
{
    if (!bInLobby) return;
    bInLobby = false;
    if (bWorldStored) RestoreDungeon();
    else LoadCheckpointWorld(); // Cold restart: restore exact saved dungeon state.
    Toast = TEXT("DESCENT RESUMED");
    ToastTime = 2.5f;
}

void AAVGameMode::RecordRun()
{
    BestFloor = FMath::Max(BestFloor, Floor);
    BestKills = FMath::Max(BestKills, Kills);
    LifetimeGold += Gold;
    UAVSaveGame* Save = Cast<UAVSaveGame>(
        UGameplayStatics::LoadGameFromSlot(TEXT("AshenVaultsRecord"), 0));
    if (!Save) Save = Cast<UAVSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UAVSaveGame::StaticClass()));
    if (Save)
    {
        // A completed/dead run cannot be resumed from its old checkpoint.
        Save->bHasCheckpoint = false;
        bCanContinue = false;
        Save->BestFloor = BestFloor; Save->BestKills = BestKills; Save->TotalGold = LifetimeGold;
        UGameplayStatics::SaveGameToSlot(Save, TEXT("AshenVaultsRecord"), 0);
    }
}

void AAVGameMode::PollInput(float DeltaSeconds)
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) return;

    if(Phase==EAVPhase::MainMenu)
    {
        // UMG owns input while the title screen is visible.
        if(!FrontendWidget && PC->WasInputKeyJustPressed(EKeys::Enter))
            StartFromMenu(bCanContinue,HeroClass);
        return;
    }
    const bool bPause = Pressed(PC, EKeys::Escape, PrevPause) || PC->WasInputKeyJustPressed(EKeys::P);
    const bool bRestart = Pressed(PC, EKeys::Enter, PrevEnter);
    const bool bReplay = Pressed(PC, EKeys::R, PrevReplay); // Retry after death; knife during play.
    if (!bInLobby && !bInSafeCamp && Pressed(PC, EKeys::H, PrevHero))
    {
        HeroClass = (HeroClass + 1) % FAVContent::HeroCount;
        NewRun(); // Class changes restart the run; never mix stats mid-run.
        Toast = HeroClass == 0 ? TEXT("WARDEN SELECTED") :
                HeroClass == 1 ? TEXT("DUELIST SELECTED") :
                HeroClass == 2 ? TEXT("ARCANIST SELECTED") : TEXT("MARKSMAN SELECTED");
        ToastTime = 3.f;
        return;
    }
    if (Pressed(PC, EKeys::F3, PrevQuality))
    {
        VisualQuality = (VisualQuality + 1) % 3;
        Toast = VisualQuality == 2 ? TEXT("VISUALS: HIGH") :
                VisualQuality == 1 ? TEXT("VISUALS: MEDIUM") : TEXT("VISUALS: LOW");
        ToastTime = 2.f;
    }
    const bool bChoice0 = Pressed(PC, EKeys::One, PrevChoices[0]);
    const bool bChoice1 = Pressed(PC, EKeys::Two, PrevChoices[1]);
    const bool bChoice2 = Pressed(PC, EKeys::Three, PrevChoices[2]);
    const bool bHit = Pressed(PC, EKeys::LeftMouseButton, PrevAttack) ||
        (PC->IsInputKeyDown(EKeys::LeftMouseButton) && AttackCooldown <= 0.f);
    const bool bSpin = Pressed(PC, EKeys::Q, PrevSpin) || PC->WasInputKeyJustPressed(EKeys::RightMouseButton);
    const bool bDash = Pressed(PC, EKeys::SpaceBar, PrevDash);
    const bool bHeal = Pressed(PC, EKeys::F, PrevHeal);
    const bool bUse = Pressed(PC, EKeys::E, PrevUse);

    if (Phase == EAVPhase::Dead || Phase == EAVPhase::Victory)
    {
        if (bRestart) NewRun();
        else if (bReplay) NewRun(true);
        return;
    }
    // Inventory and skill tree intentionally pause the game, including enemy AI.
    const bool bInventoryKey = PC->WasInputKeyJustPressed(EKeys::I) ||
                               PC->WasInputKeyJustPressed(EKeys::Tab);
    const bool bSkillsKey = PC->WasInputKeyJustPressed(EKeys::K);
    if (Phase == EAVPhase::Inventory)
    {
        if (bInventoryKey || bPause) { Phase=EAVPhase::Playing; return; }
        if (PC->WasInputKeyJustPressed(EKeys::Up) && InventoryItems.Num()>0)
            GearSelection=(GearSelection+InventoryItems.Num()-1)%InventoryItems.Num();
        if (PC->WasInputKeyJustPressed(EKeys::Down) && InventoryItems.Num()>0)
            GearSelection=(GearSelection+1)%InventoryItems.Num();
        if (bRestart || bUse) EquipInventoryItem(GearSelection);
        const FKey Slots[8]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four,
                             EKeys::Five,EKeys::Six,EKeys::Seven,EKeys::Eight};
        for(int32 I=0;I<8;++I)
            if(PC->WasInputKeyJustPressed(Slots[I])) { UnequipSlot(I); break; }
        return;
    }
    if (Phase == EAVPhase::Skills)
    {
        if (bSkillsKey || bPause) { Phase=EAVPhase::Playing; return; }
        if (PC->WasInputKeyJustPressed(EKeys::Up))
            SkillSelection=(SkillSelection+20)%21;
        if (PC->WasInputKeyJustPressed(EKeys::Down))
            SkillSelection=(SkillSelection+1)%21;
        if (bRestart || bUse) LearnSkill(SkillSelection);
        if (PC->WasInputKeyJustPressed(EKeys::BackSpace)) ResetSkills();
        return;
    }
    if (Phase == EAVPhase::Playing && bInventoryKey)
    { Phase=EAVPhase::Inventory; GearSelection=0; return; }
    if (Phase == EAVPhase::Playing && bSkillsKey)
    { Phase=EAVPhase::Skills; SkillSelection=0; return; }
    if (Phase == EAVPhase::Shop)
    {
        if (bPause || bUse) { Phase = EAVPhase::Playing; return; }
        if (bChoice0) BuyFromMerchant(0);
        else if (bChoice1) BuyFromMerchant(1);
        else if (bChoice2) BuyFromMerchant(2);
        else if (PC->WasInputKeyJustPressed(EKeys::Four) && bInSafeCamp) SellPack();
        return;
    }
    if (Phase == EAVPhase::Relic)
    {
        if (bChoice0) ChooseRelic(0);
        else if (bChoice1) ChooseRelic(1);
        else if (bChoice2) ChooseRelic(2);
        return;
    }
    if (bPause)
    {
        Phase = Phase == EAVPhase::Paused ? EAVPhase::Playing : EAVPhase::Paused;
        return;
    }
    if (Phase == EAVPhase::Paused) return;
    if (Phase == EAVPhase::Upgrade)
    {
        if (bChoice0) SelectUpgrade(0);
        else if (bChoice1) SelectUpgrade(1);
        else if (bChoice2) SelectUpgrade(2);
        return;
    }

    int32 ViewportX = 960, ViewportY = 540;
    PC->GetViewportSize(ViewportX, ViewportY);
    float MouseX = ViewportX * 0.5f, MouseY = ViewportY * 0.5f;
    if (PC->GetMousePosition(MouseX, MouseY))
        Aim = SafeDirection(FVector2D(MouseX - ViewportX * 0.5f, MouseY - ViewportY * 0.5f));
    if (Aim.IsNearlyZero()) Aim = FVector2D(1.f, 0.f);

    FVector2D Direction(0.f, 0.f);
    if (PC->IsInputKeyDown(EKeys::W)) Direction.Y -= 1.f;
    if (PC->IsInputKeyDown(EKeys::S)) Direction.Y += 1.f;
    if (PC->IsInputKeyDown(EKeys::A)) Direction.X -= 1.f;
    if (PC->IsInputKeyDown(EKeys::D)) Direction.X += 1.f;
    Direction = SafeDirection(Direction);
    const float DashCost=(HasRelic(4)?16.f:23.f)*(1.f-0.1f*SkillRank(13));
    if (bDash && Stamina >= DashCost && DashCooldown <= 0.f && DashTime <= 0.f)
    {
        Stamina -= DashCost;
        DashCooldown = HasRelic(4) ? 0.47f : 0.7f;
        DashDirection = Direction.IsNearlyZero() ? Aim : Direction;
        DashTime = 0.17f;
        Invulnerability=FMath::Max(Invulnerability,0.25f+0.07f*SkillRank(14));
        DashBonusTime=3.f;
        if(HeroClass==0 && SkillRank(19)>0 && ShieldCooldown<=0.f) {
            TemporaryShield=FMath::Max(TemporaryShield,MaxHP*0.25f);
            ShieldCooldown=12.f;
        }
        AddEffect(Hero,1,0.7f);
    }
    const float EffectiveMove=MoveSpeed*(FoodSpeedTime>0.f?1.15f:1.f)*
        (HeroClass==1 && LastKillBoostTime>0.f ?
            (1.f+0.06f*SkillRank(24)):1.f);
    const FVector2D BeforeMovement = Hero;
    if (DashTime > 0.f)
    {
        // The dash never gains extra distance on slow frames.
        const float DashDt = FMath::Min(DashTime, DeltaSeconds);
        Hero = MoveWithCollision(Hero, DashDirection * 16.f, DashDt, 0.27f);
        if (DeltaSeconds > DashDt)
            Hero = MoveWithCollision(Hero, Direction * EffectiveMove,
                                     DeltaSeconds - DashDt, 0.27f);
    }
    else
        Hero=MoveWithCollision(Hero,Direction*EffectiveMove,DeltaSeconds,0.27f);
    HeroVelocity = (Hero - BeforeMovement) / FMath::Max(0.001f, DeltaSeconds);

    if (bHit) { if (HeroClass == 3) ShootArrow(); else Strike(false); }
    // A sword can defeat a guardian and open a relic selection instantly:
    // don't process another action in the same frame after phase changes.
    if (Phase != EAVPhase::Playing) return;
    if (bSpin) { if (HeroClass == 3) FireVolley(); else Strike(true); }
    if (Phase != EAVPhase::Playing) return;
    if (bReplay) { if (HeroClass == 3) PiercingShot(); else ThrowKnife(); }
    if (Phase != EAVPhase::Playing) return;
    if (bHeal && Potions > 0 && HP < MaxHP)
    {
        --Potions; HP = FMath::Min(MaxHP, HP + 42.f + 8.f*SkillRank(2));
        PoisonTime = 0.f; BurnTime = 0.f;
        AddEffect(Hero, 2, 1.f);
    }
    if (bInLobby)
    {
        if (bUse && FVector2D::Distance(Hero, Exit) < 1.8f) ResumeFromLobby();
        return;
    }
    if (bInSafeCamp)
    {
        if (bUse && FVector2D::Distance(Hero, Exit) < 1.8f) RestoreDungeon();
        else if (bUse && FVector2D::Distance(Hero, Shrine) < 1.8f) EnterLobby();
        else if (bUse && FVector2D::Distance(Hero, Merchant) < 1.8f) Phase = EAVPhase::Shop;
        else if (bUse && FVector2D::Distance(Hero, FVector2D(32.5f, 22.5f)) < 1.8f)
        {
            ForgeUpgrade(PC->IsInputKeyDown(EKeys::LeftShift) ? 1 : 0);
        }
        return;
    }
    if (bUse && bHasSafeCampDoor && FVector2D::Distance(Hero, SafeCampDoor) < 1.7f)
    {
        EnterSafeCamp();
        return;
    }
    if (bUse && bInBossArena && IsExitReady() && FVector2D::Distance(Hero, Exit) < 1.8f)
    {
        RestoreDungeon();
        bGuardianDefeated = true;
        PendingGuardian = -1;
        Toast = TEXT("GUARDIAN DEFEATED: THE DESCENT OPENS");
        ToastTime = 2.5f;
        return;
    }
    if (bUse && !bInBossArena && PendingGuardian >= 0 &&
        FVector2D::Distance(Hero, Exit) < 1.8f)
    {
        EnterBossArena();
        return;
    }
    if (bUse && bMerchantSpawned && FVector2D::Distance(Hero, Merchant) < 1.65f)
    {
        Phase = EAVPhase::Shop;
    }
    else if (bUse && !bShrineUsed && FVector2D::Distance(Hero, Shrine) < 1.55f)
    {
        bShrineUsed = true;
        HP = FMath::Min(MaxHP, HP + 35.f);
        AttackPower += 2.f;
        Toast = TEXT("SHRINE BLESSING  +35 HP  +2 DAMAGE");
        ToastTime = 2.6f;
        AddEffect(Shrine, 1, 1.6f);
    }
    else if (bUse && OpenNearbyCampDoor())
    {
        // Gates unlock the rival hideout and allow combat through the entrance.
    }
    else if (bUse && OpenNearbyChest())
    {
        // A chest may release a mimic, which re-locks the stairs.
    }
    else if (bUse && ActivateNearbyEcho())
    {
        // One-time lore artifact, reward and nearby map discovery.
    }
    else if (bUse && !bInBossArena && IsExitReady() && FVector2D::Distance(Hero, Exit) < 1.7f)
    {
        {
            ++Floor;
            HP = FMath::Min(MaxHP, HP + 22.f);
            Stamina = MaxStamina;
            GenerateFloor();
        }
    }
}

void AAVGameMode::SpawnEnemyBolt(const FAVEnemy& Enemy, const FVector2D& Direction,
                                  float Speed, float DamageScale, int32 Status)
{
    FAVBolt Bolt;
    Bolt.Pos = Enemy.Pos + SafeDirection(Direction) * 0.35f;
    Bolt.Velocity = SafeDirection(Direction) * Speed;
    Bolt.Damage = FMath::Max(1, FMath::RoundToInt(
        FAVContent::Enemy(Enemy.Type, Floor).Damage *
        FAVContent::LevelDamageMultiplier(Enemy.Level) *
        (Enemy.bElite ? 1.35f : 1.f) * DamageScale));
    const FAVAffixTraits A=FAVAffixes::Traits(Enemy.Affix,
        Enemy.HP/FMath::Max(1.f,Enemy.MaxHP),Enemy.AlertTime);
    Bolt.Damage=FMath::RoundToInt(Bolt.Damage*A.OutgoingScale);
    Bolt.Velocity*=A.BoltSpeedScale;
    Bolt.Status = Status!=0?Status:A.InflictedStatus;
    Bolt.Affix=Enemy.Affix;
    Bolt.SourceEnemyId=Enemy.Id;
    Bolt.Faction = Enemy.Faction;
    Bolt.Time = 2.1f;
    Bolts.Add(Bolt);
}

void AAVGameMode::CastEnemyAbility(int32 Index, const FVector2D& Target)
{
    if (!Enemies.IsValidIndex(Index)) return;
    FAVEnemy& E = Enemies[Index];
    const FVector2D Dir = SafeDirection(Target - E.Pos);
    const FVector2D Side(-Dir.Y, Dir.X);
    const int32 Pattern = E.Type;
    const bool bFrenzy = E.HP < E.MaxHP * 0.45f;
    const float D = FVector2D::Distance(E.Pos, Target);
    // Seven minibosses have separate, telegraphed spell patterns.
    if (Pattern == 26) // Grave Butcher: short-range cleaving cone
    {
        if (D < 3.5f)
            for (int32 K = -2; K <= 2; ++K)
                SpawnEnemyBolt(E, Dir + Side * (0.25f * K), 6.5f, 0.62f);
        E.AbilityCooldown = bFrenzy ? 3.1f : 4.5f;
    }
    else if (Pattern == 27) // Bone Archer: staggered triple arrow volley
    {
        for (int32 K = -1; K <= 1; ++K)
            SpawnEnemyBolt(E, Dir + Side * (0.22f * K), 11.0f, 0.80f);
        E.AbilityCooldown = bFrenzy ? 2.1f : 3.5f;
    }
    else if (Pattern == 28) // Rot Witch: poison fan
    {
        for (int32 K = -2; K <= 2; ++K)
            SpawnEnemyBolt(E, Dir + Side * (0.29f * K), 5.7f, 0.54f, 1);
        E.AbilityCooldown = bFrenzy ? 2.7f : 4.1f;
    }
    else if (Pattern == 29) // Stone Sentinel: radial shockwave
    {
        for (int32 K = 0; K < 12; ++K)
        {
            const float A = 6.2831853f * K / 12.f;
            SpawnEnemyBolt(E, FVector2D(FMath::Cos(A), FMath::Sin(A)),
                           5.1f, 0.58f);
        }
        E.AbilityCooldown = bFrenzy ? 3.2f : 5.f;
    }
    else if (Pattern == 30) // Blood Reaver: dash lunge plus bleeding blade fan
    {
        E.Pos = MoveWithCollision(E.Pos, Dir * 8.0f, 0.23f, 0.4f);
        for (int32 K = -1; K <= 1; ++K)
            SpawnEnemyBolt(E, Dir + Side * (K * 0.35f), 7.7f, 0.68f);
        E.AbilityCooldown = bFrenzy ? 2.6f : 4.2f;
    }
    else if (Pattern == 31) // Cinder Shaman: burning cross burst
    {
        for (int32 K = 0; K < 8; ++K)
        {
            const float A = 6.2831853f * K / 8.f;
            SpawnEnemyBolt(E, FVector2D(FMath::Cos(A), FMath::Sin(A)),
                           6.8f, 0.72f, 2);
        }
        E.AbilityCooldown = bFrenzy ? 2.5f : 4.0f;
    }
    else if (Pattern == 32) // Iron Tyrant: narrow penetrating salvo
    {
        for (int32 K = -2; K <= 2; ++K)
            SpawnEnemyBolt(E, Dir + Side * (K * 0.12f), 9.1f, 0.95f, 2);
        E.AbilityCooldown = bFrenzy ? 3.0f : 4.8f;
    }
    else if (Pattern == 56) // Commandant: uncompromising melee guardian, two stages
    {
        // First stage cleaves a frontal arc. After the armor breaks at 45% HP
        // the Commandant lunges and chains heavier fan strikes.
        const bool Enraged = E.bPhaseShifted && E.PhaseShiftTime <= 0.f;
        if (Enraged)
            E.Pos = MoveWithCollision(E.Pos, Dir * 8.5f, 0.24f, 0.47f);
        const int32 Count = Enraged ? 7 : 5;
        for (int32 K = -(Count / 2); K <= Count / 2; ++K)
            SpawnEnemyBolt(E, Dir + Side * (K * (Enraged ? 0.23f : 0.18f)),
                           Enraged ? 8.4f : 6.8f, Enraged ? 0.79f : 0.66f);
        if (Enraged && E.AttackPattern % 3 == 2)
            for (int32 K = 0; K < 10; ++K)
            {
                const float A = 6.2831853f * K / 10.f;
                SpawnEnemyBolt(E, FVector2D(FMath::Cos(A), FMath::Sin(A)),
                               5.6f, 0.42f);
            }
        E.AbilityCooldown = Enraged ? 2.9f : 4.1f;
    }
    else if (Pattern >= 52 && Pattern <= 55) // Bastion's four miniboss archetypes
    {
        if (Pattern == 52) // Executioner: heavy axe cleave
            for (int32 K = -2; K <= 2; ++K)
                SpawnEnemyBolt(E, Dir + Side * K * 0.30f, 6.3f, 0.78f);
        else if (Pattern == 53) // Captain: shield wall and counter-thrust
            for (int32 K = -1; K <= 1; ++K)
                SpawnEnemyBolt(E, Dir + Side * K * 0.18f, 9.2f, 0.73f);
        else if (Pattern == 54) // Siege Golem: heavy ground shockwave
            for (int32 K = 0; K < 12; ++K)
            {
                const float A = 6.2831853f * K / 12.f;
                SpawnEnemyBolt(E, FVector2D(FMath::Cos(A), FMath::Sin(A)), 5.0f, 0.72f);
            }
        else // Inquisitor: ranged magic with a sweeping fan
            for (int32 K = -3; K <= 3; ++K)
                SpawnEnemyBolt(E, Dir + Side * K * 0.22f, 8.7f, 0.63f, 2);
        E.AbilityCooldown = bFrenzy ? 2.8f : 4.6f;
    }
    else if (Pattern >= 34 && Pattern <= 51) // Bastion normal enemies: 18 types, two variants by stable enemy id
    {
        const bool Variant = E.Subclass != 0;
        // Every variant changes the ability and cadence, not just HP or tint.
        switch (Pattern)
        {
        case 34: // infantry: wide slash / hard straight thrust
            for (int32 K = -1; K <= 1; ++K)
                SpawnEnemyBolt(E, Dir + Side * K * (Variant ? 0.12f : 0.34f),
                               7.f, Variant ? 0.77f : 0.50f);
            break;
        case 35: // pikeman: precision thrust / spear sweep
            for (int32 K = Variant ? -2 : 0; K <= (Variant ? 2 : 0); ++K)
                SpawnEnemyBolt(E, Dir + Side * K * 0.24f, 9.f, Variant ? 0.48f : 0.98f);
            break;
        case 36: // shieldbearer: a shield pulse / guarded counter
            for (int32 K = 0; K < (Variant ? 3 : 6); ++K)
            {
                const float A = Variant ? FMath::Atan2(Dir.Y, Dir.X) + (K-1) * 0.35f :
                    6.2831853f * K / 6.f;
                SpawnEnemyBolt(E, FVector2D(FMath::Cos(A), FMath::Sin(A)), 5.7f, 0.55f);
            }
            break;
        case 37: // crossbowman: sniper / spread bolts
            for (int32 K = Variant ? -1 : 0; K <= (Variant ? 1 : 0); ++K)
                SpawnEnemyBolt(E, Dir + Side * K * 0.26f, Variant ? 9.5f : 12.0f,
                               Variant ? 0.48f : 0.92f);
            break;
        case 38: // halberdier: sweeping cone / spinning swing
            for (int32 K = 0; K < (Variant ? 8 : 5); ++K)
            {
                const float A = Variant ? 6.2831853f * K / 8.f :
                    FMath::Atan2(Dir.Y,Dir.X) + (K-2) * 0.30f;
                SpawnEnemyBolt(E,FVector2D(FMath::Cos(A),FMath::Sin(A)),5.9f,0.53f);
            }
            break;
        case 39: // berserker: close assault / crossing blows
            E.Pos = MoveWithCollision(E.Pos, Dir * (Variant ? 10.f : 6.f), 0.17f, 0.26f);
            for (int32 K=-1;K<=1;++K)
                SpawnEnemyBolt(E,Dir+Side*K*(Variant?0.5f:0.2f),6.3f,0.59f);
            break;
        case 40: // hound: fast pounce / short bloody fan
            E.Pos = MoveWithCollision(E.Pos, Dir * (Variant ? 11.f : 7.f), 0.19f, 0.26f);
            if (Variant)
                for (int32 K=-1;K<=1;++K)
                    SpawnEnemyBolt(E,Dir+Side*K*0.24f,6.1f,0.39f);
            break;
        case 41: // gargoyle: flank blink / diving dash
        {
            const FVector2D Desired=E.Pos+(Variant?Side*2.2f:Dir*2.1f);
            if (CanMove(Desired,0.26f)) E.Pos=Desired;
            if (Variant) SpawnEnemyBolt(E,Dir,8.f,0.65f);
            break;
        }
        case 42: // bonebreaker: shockwave / focused smash
            for (int32 K=0;K<(Variant?3:8);++K)
            {
                const float A=Variant?FMath::Atan2(Dir.Y,Dir.X)+(K-1)*0.2f:
                    6.2831853f*K/8.f;
                SpawnEnemyBolt(E,FVector2D(FMath::Cos(A),FMath::Sin(A)),4.8f,0.58f);
            }
            break;
        case 43: // crawler: erratic dash / venom spit
            if (!Variant) E.Pos=MoveWithCollision(E.Pos,Dir*10.f,0.18f,0.24f);
            else for(int32 K=-1;K<=1;++K)
                SpawnEnemyBolt(E,Dir+Side*K*0.25f,7.2f,0.38f,1);
            break;
        case 44: // possessed knight: blade crescent / armor-piercing salvo
            for(int32 K=-2;K<=2;++K)
                SpawnEnemyBolt(E,Dir+Side*K*(Variant?0.1f:0.31f),6.9f,Variant?0.71f:0.52f);
            break;
        case 45: // dark hunter: evasive sidestep / quick volley
            if(!Variant) {
                const FVector2D Move=E.Pos+Side*2.1f;
                if(CanMove(Move,0.26f)) E.Pos=Move;
            }
            for(int32 K=Variant?-1:0;K<=(Variant?1:0);++K)
                SpawnEnemyBolt(E,Dir+Side*K*0.16f,10.2f,Variant?0.47f:0.75f);
            break;
        case 46: // battlemage: fan of magic / radial burst
            for(int32 K=0;K<(Variant?8:5);++K) {
                const float A=Variant?6.2831853f*K/8.f:
                    FMath::Atan2(Dir.Y,Dir.X)+(K-2)*0.25f;
                SpawnEnemyBolt(E,FVector2D(FMath::Cos(A),FMath::Sin(A)),7.8f,0.51f);
            }
            break;
        case 47: // summoner: summons after casting / defensive spell fan
            if (!Variant)
                for(int32 K=-1;K<=1;++K)
                    SpawnEnemyBolt(E,Dir+Side*K*0.27f,6.2f,0.40f);
            break;
        case 48: // punisher: brutal cone / crushing circular slam
            for(int32 K=0;K<(Variant?10:5);++K) {
                const float A=Variant?6.2831853f*K/10.f:
                    FMath::Atan2(Dir.Y,Dir.X)+(K-2)*0.23f;
                SpawnEnemyBolt(E,FVector2D(FMath::Cos(A),FMath::Sin(A)),5.5f,0.63f);
            }
            break;
        case 49: // blood priest: healing prayer / fire fan
            if (!Variant)
                for (FAVEnemy& Ally : Enemies)
                    if (Ally.HP>0.f && Ally.Faction==E.Faction &&
                        FVector2D::DistSquared(Ally.Pos,E.Pos)<16.f)
                        Ally.HP=FMath::Min(Ally.MaxHP,Ally.HP+Ally.MaxHP*0.12f);
            else for(int32 K=-1;K<=1;++K)
                SpawnEnemyBolt(E,Dir+Side*K*0.3f,6.6f,0.50f,2);
            break;
        case 50: // seal sentinel: guarding radial pulse / targeted runes
            for(int32 K=0;K<(Variant?3:8);++K) {
                const float A=Variant?FMath::Atan2(Dir.Y,Dir.X)+(K-1)*0.31f:
                    6.2831853f*K/8.f;
                SpawnEnemyBolt(E,FVector2D(FMath::Cos(A),FMath::Sin(A)),7.f,0.51f);
            }
            break;
        case 51: // ravager: dash dual slash / rapid horizontal flurry
            if (!Variant) E.Pos=MoveWithCollision(E.Pos,Dir*11.f,0.16f,0.26f);
            for(int32 K=-(Variant?2:1);K<=(Variant?2:1);++K)
                SpawnEnemyBolt(E,Dir+Side*K*0.27f,8.2f,0.42f);
            break;
        default: break;
        }
        E.AbilityCooldown = Variant ? 4.7f : 3.8f;
    }
    else if (Pattern == 3 || Pattern == 8 || Pattern == 9) // existing bosses: new signature phase attack
    {
        const int32 Count = Pattern == 3 ? (bFrenzy ? 18 : 12) :
            Pattern == 8 ? (bFrenzy ? 12 : 8) : (bFrenzy ? 16 : 10);
        const int32 Status = Pattern == 9 ? 2 : 0;
        for (int32 K = 0; K < Count; ++K)
        {
            const float A = 6.2831853f * K / Count + RunTime * 0.28f;
            SpawnEnemyBolt(E, FVector2D(FMath::Cos(A), FMath::Sin(A)),
                           Pattern == 8 ? 5.8f : 7.0f, 0.55f, Status);
        }
        E.AbilityCooldown = bFrenzy ? 2.8f : 5.6f;
        if (Pattern == 9) // Forge Tyrant: persistent molten ground
        {
            for (int32 K = -1; K <= 1; ++K)
            {
                const FVector2D Pos = E.Pos + Dir * 2.2f + Side * (K * 1.25f);
                if (!CanMove(Pos, 0.2f) || Hazards.Num() >= 40) continue;
                FAVHazard FirePatch;
                FirePatch.Pos = Pos;
                FirePatch.Type = 1;
                Hazards.Add(FirePatch);
            }
        }
        if (Pattern == 3) // Vault Warden: phase blink to reposition
        {
            const FVector2D Blink = E.Pos + Side * (E.Id % 2 ? 2.4f : -2.4f);
            if (CanMove(Blink, 0.47f)) E.Pos = Blink;
        }
    }
    else if (Pattern == 0 && E.bElite) // reinforced ashling: close-range triple slash
    {
        if (D < 3.5f)
            for (int32 K = -1; K <= 1; ++K)
                SpawnEnemyBolt(E, Dir + Side * (K * 0.4f), 6.6f, 0.6f);
        E.AbilityCooldown = 4.4f;
    }
    else if (Pattern == 23 || Pattern == 24 || Pattern == 7) // golem ground slam
    {
        if (D < 3.5f)
        {
            const int32 Shards = Pattern == 24 ? 12 : Pattern == 23 ? 6 : 8;
            for (int32 K = 0; K < Shards; ++K)
            {
                const float A = 6.2831853f * K / Shards;
                SpawnEnemyBolt(E, FVector2D(FMath::Cos(A), FMath::Sin(A)),
                               Pattern == 24 ? 6.1f : 4.2f,
                               Pattern == 24 ? 0.55f : 0.45f, Pattern == 7 ? 2 : 0);
            }
        }
        E.AbilityCooldown = 5.2f;
    }
    else if (Pattern == 19 || Pattern == 12 || Pattern == 21 || Pattern == 6)
    {
        for (int32 K = -1; K <= 1; ++K)
            SpawnEnemyBolt(E, Dir + Side * (K * 0.38f), 6.1f, 0.6f,
                           Pattern == 6 || Pattern == 21 ? 1 : 0);
        E.AbilityCooldown = 4.8f;
    }
    else if (Pattern == 18 || Pattern == 14) // ghost blink
    {
        const FVector2D Destination = E.Pos + Side * (E.Id % 2 ? 2.2f : -2.2f);
        if (CanMove(Destination, 0.28f)) E.Pos = Destination;
        E.AbilityCooldown = 3.4f;
    }
    else E.AbilityCooldown = 6.f;
    // Increment exactly once before any summon can grow Enemies/TArray.
    // Otherwise an old FAVEnemy& may dangle after reallocation.
    ++E.AttackPattern;
    E.AbilityCooldown*=FAVAffixes::Traits(E.Affix,
        E.HP/FMath::Max(1.f,E.MaxHP),E.AlertTime).CooldownScale;
    AddEffect(E.Pos, 1, E.bMiniBoss || FAVContent::Enemy(E.Type, Floor).Boss ? 2.f : 1.f);
    if (Pattern == 8 && Enemies.Num() <= 108) // Leave room for both summons
    {
        const FVector2D SummonCenter = E.Pos;
        const int32 SummonLevel = E.Level;
        const int32 SummonCast = E.AttackPattern;
        for (int32 K = -1; K <= 1; K += 2)
        {
            const FVector2D Spawn = SummonCenter + Side * (K * 1.5f);
            if (!CanMove(Spawn, 0.28f)) continue;
            FAVEnemy Skeleton;
            Skeleton.Type = 17;
            Skeleton.Pos = Spawn;
            Skeleton.Level = SummonLevel;
            Skeleton.MaxHP = FAVContent::ScaledEnemyHealth(17, Floor, Skeleton.Level);
            Skeleton.HP = Skeleton.MaxHP;
            Skeleton.Cooldown = 0.8f;
            Skeleton.Id = 200000 + Kills + SummonCast * 10 + K;
            Enemies.Add(Skeleton);
        }
    }
    if (Pattern == 47 && E.Subclass != 0 && Enemies.Num() < 100)
    {
        // Summon only after the caster's effects are complete (TArray may reallocate).
        const FVector2D Spawn = E.Pos + Side * (E.AttackPattern % 2 == 0 ? 1.2f : -1.2f);
        if (CanMove(Spawn,0.26f))
        {
            FAVEnemy Soldier;
            Soldier.Type=34; Soldier.Pos=Spawn; Soldier.Faction=E.Faction;
            Soldier.Level=E.Level;
            Soldier.MaxHP=FAVContent::ScaledEnemyHealth(34,Floor,Soldier.Level) * 0.55f;
            Soldier.HP=Soldier.MaxHP; Soldier.Id=300000+E.Id*1000+E.AttackPattern;
            Soldier.Cooldown=0.8f;
            Enemies.Add(Soldier);
        }
    }

}

void AAVGameMode::UpdateCombat(float DeltaSeconds)
{
    for (int32 I = Enemies.Num() - 1; I >= 0; --I)
    {
        if (Phase != EAVPhase::Playing) return;
        if (Enemies[I].HP <= 0.f) { KillEnemy(I); continue; }
        FAVEnemy& E = Enemies[I];
        E.PoisonTime=FMath::Max(0.f,E.PoisonTime-DeltaSeconds);
        E.BurnTime=FMath::Max(0.f,E.BurnTime-DeltaSeconds);
        if(E.PoisonTime>0.f||E.BurnTime>0.f)
        {
            E.StatusTick+=DeltaSeconds;
            if(E.StatusTick>=1.f)
            {
                E.StatusTick-=1.f;
                ApplyDamageToEnemy(E,(E.PoisonTime>0.f?3.f:0.f)+
                    (E.BurnTime>0.f?5.f:0.f));
                if(E.HP<=0.f){KillEnemy(I);continue;}
            }
        }
        else E.StatusTick=0.f;
        const FAVEnemySpec Spec = FAVContent::Enemy(E.Type, Floor);
        if (Spec.Boss && !E.bPhaseShifted && E.HP <= E.MaxHP * 0.45f)
        {
            E.bPhaseShifted = true;
            E.PhaseShiftTime = 1.5f;
            E.AttackWindup = 0.f;
            E.Telegraph = 0.f;
            E.AbilityCooldown = FMath::Max(E.AbilityCooldown, 1.5f);
        }
        if (E.PhaseShiftTime > 0.f)
        {
            const float Previous = E.PhaseShiftTime;
            E.PhaseShiftTime = FMath::Max(0.f, E.PhaseShiftTime - DeltaSeconds);
            if (Previous > 0.f && E.PhaseShiftTime == 0.f)
            {
                // Transformation finishes with a biome-specific visible pulse.
                const int32 Shards = E.Type == 8 ? 8 : E.Type == 9 ? 12 : 16;
                for (int32 K = 0; K < Shards; ++K)
                {
                    const float A = 6.2831853f * K / Shards;
                    SpawnEnemyBolt(E, FVector2D(FMath::Cos(A), FMath::Sin(A)),
                                   E.Type == 8 ? 5.0f : 6.5f, 0.38f,
                                   E.Type == 9 ? 2 : 0);
                }
                AddEffect(E.Pos, 1, 2.5f);
                E.Cooldown = 1.0f;
            }
            continue; // Transformation is a distinct non-attacking animation phase.
        }
        E.Cooldown -= DeltaSeconds;
        E.AbilityCooldown -= DeltaSeconds;
        E.PathRefresh -= DeltaSeconds;
        E.AlertTime = FMath::Max(0.f, E.AlertTime - DeltaSeconds);
        E.Flash = FMath::Max(0.f, E.Flash - DeltaSeconds);
        FVector2D Target = Hero;
        bool bTargetEnemy = false;
        float TargetDistanceSq = FVector2D::DistSquared(Hero, E.Pos);
        for (int32 J = 0; J < Enemies.Num(); ++J)
        {
            if (J == I || Enemies[J].HP <= 0.f ||
                Enemies[J].Faction == E.Faction) continue;
            const float DistSq = FVector2D::DistSquared(E.Pos, Enemies[J].Pos);
            if (DistSq < TargetDistanceSq && DistSq < 49.f)
            { Target = Enemies[J].Pos; TargetDistanceSq = DistSq; bTargetEnemy = true; }
        }
        const bool bSeesTarget = TargetDistanceSq < 121.f &&
            FAVDungeonNavigation::LineOfSight(
            Tiles, MapW, MapH, E.Pos, Target);
        if (bSeesTarget)
        {
            E.LastSeenHero = Target;
            E.MemoryTime = FAVAffixes::Traits(E.Affix,
                E.HP/FMath::Max(1.f,E.MaxHP),E.AlertTime).MemorySeconds;
            E.AlertTime = 5.f;
            // Nearby allies receive the target position as a group alert.
            for (int32 J = 0; J < Enemies.Num(); ++J)
            {
                if (J == I || Enemies[J].Faction != E.Faction ||
                    FVector2D::DistSquared(E.Pos, Enemies[J].Pos) > 30.25f) continue;
                if (Enemies[J].MemoryTime < 1.5f)
                {
                    Enemies[J].LastSeenHero = Target;
                    Enemies[J].MemoryTime = 1.5f;
                }
            }
        }
        else E.MemoryTime = FMath::Max(0.f, E.MemoryTime - DeltaSeconds);
        const FVector2D Pursuit = bSeesTarget ? Target :
            E.MemoryTime > 0.f ? E.LastSeenHero : E.Pos;
        const FVector2D ToHero = Target - E.Pos;
        const float Distance = ToHero.Size();
        const FVector2D Direction = SafeDirection(ToHero);
        const FAVAffixTraits Affix=FAVAffixes::Traits(
            E.Affix,E.HP/FMath::Max(1.f,E.MaxHP),E.AlertTime);
        float CommandAura=1.f;
        for(const FAVEnemy& Ally:Enemies)
        {
            if(Ally.Id==E.Id||Ally.HP<=0.f||Ally.Faction!=E.Faction||
               Ally.Affix!=static_cast<int32>(EAVEnemyAffix::Commander))
                continue;
            if((Ally.Pos-E.Pos).SizeSquared()<25.f)
            {CommandAura=1.18f;break;}
        }
        const float Speed=Spec.Speed*(E.bElite?1.18f:1.f)*
            ((E.Type==56||E.Type==39)&&E.HP<E.MaxHP*0.45f?1.27f:1.f)*
            Affix.MovementScale*CommandAura;
        // Medics heal their living nearby allies continuously while fighting.
        if(Affix.AllyHealingPerSecond>0.f)
            for(FAVEnemy& Ally:Enemies)
                if(Ally.Id!=E.Id&&Ally.HP>0.f&&Ally.Faction==E.Faction &&
                   Ally.HP<Ally.MaxHP&&
                   (Ally.Pos-E.Pos).SizeSquared()<16.f)
                    Ally.HP=FMath::Min(Ally.MaxHP,
                        Ally.HP+Ally.MaxHP*Affix.AllyHealingPerSecond*DeltaSeconds);
        const bool bCaster = (Spec.Ranged || E.Equipment == 2) && !Spec.Boss;
        const bool bMeleeBoss = E.Type == 8 || E.Type == 56;
        const float Reach = Spec.Boss ? (bMeleeBoss ? 2.0f : 1.6f) :
                            (E.Type == 7 || E.Type == 13 || E.Type == 35 || E.Type == 38 || E.Type == 42) ? 1.30f : 1.08f;
        FVector2D DesiredVelocity = FVector2D::ZeroVector;
        if ((bSeesTarget || E.MemoryTime > 0.f) && E.Telegraph <= 0.f)
        {
            if (bCaster && Distance < 3.8f)
                DesiredVelocity = -Direction * Speed;
            else if (Distance > (bCaster ? 5.3f :
                                 Spec.Ranged && !bMeleeBoss ? 4.9f : Reach * 0.8f))
            {
                if (!bSeesTarget && E.MemoryTime > 0.f)
                {
                    if (E.PathRefresh <= 0.f)
                    {
                        TArray<int32> MemoryFlow;
                        FAVDungeonNavigation::BuildFlow(Tiles, MapW, MapH,
                                                         Pursuit, MemoryFlow);
                        E.CachedPathDirection = FAVDungeonNavigation::FlowDirection(
                            Tiles, MemoryFlow, MapW, MapH, E.Pos, Pursuit);
                        E.PathRefresh = 0.65f + (E.Id % 5) * 0.09f;
                    }
                    DesiredVelocity = E.CachedPathDirection * Speed;
                }
                else DesiredVelocity = bTargetEnemy ?
                    SafeDirection(Pursuit - E.Pos) * Speed :
                    FAVDungeonNavigation::FlowDirection(
                        Tiles, FlowDistances, MapW, MapH, E.Pos, Hero) * Speed;
            }
        }
        E.DodgeCooldown = FMath::Max(0.f, E.DodgeCooldown - DeltaSeconds);
        if (bSeesTarget && Distance < 4.f && E.DodgeCooldown <= 0.f &&
            (E.Type == 15 || E.Type == 17 || E.Type == 22 || E.Type == 30))
        {
            const FVector2D Side(-Direction.Y, Direction.X);
            DesiredVelocity += Side * (E.Id % 2 ? 1.f : -1.f) * Speed * 0.7f;
            E.DodgeCooldown = 1.3f;
        }
        for (int32 J = 0; J < Enemies.Num(); ++J)
        {
            if (J == I || Enemies[J].Faction != E.Faction) continue;
            const FVector2D Away = E.Pos - Enemies[J].Pos;
            const float DistanceSq = Away.SizeSquared();
            if (DistanceSq > 0.01f && DistanceSq < 0.6f)
                DesiredVelocity += SafeDirection(Away) * Speed * 0.35f;
        }
        // Pack hunters converge quickly, while crypt guards advance as a shield wall.
        if ((E.Type == 15 || E.Type == 22 || E.Type == 5) && bSeesTarget && Distance < 6.f)
            DesiredVelocity += Direction * Speed * 0.22f;
        if ((E.Type == 20 || E.Type == 1) && Distance < 3.2f)
            DesiredVelocity *= 0.78f;
        if ((E.Type == 17 || E.Type == 11 || E.Type == 30) &&
            bSeesTarget && Distance > 2.f && Distance < 6.f)
        {
            const FVector2D Side(-Direction.Y, Direction.X);
            DesiredVelocity += Side * (E.Id % 2 ? 1.f : -1.f) * Speed * 0.36f;
        }
        E.Pos = MoveWithCollision(E.Pos, DesiredVelocity + E.Knockback,
                                  DeltaSeconds, Spec.Boss ? 0.47f : 0.26f);
        E.Knockback = FMath::Lerp(E.Knockback, FVector2D::ZeroVector,
                                  FMath::Clamp(DeltaSeconds * 10.f, 0.f, 1.f));
        if (E.AttackWindup > 0.f)
        {
            E.AttackWindup -= DeltaSeconds;
            if (E.AttackWindup <= 0.f)
            {
                E.AttackWindup = 0.f;
                if (FAVDungeonNavigation::LineOfSight(Tiles, MapW, MapH, E.Pos, E.LockedTarget))
                    CastEnemyAbility(I, E.LockedTarget);
                else E.AbilityCooldown = 1.f;
            }
            continue;
        }
        if (E.AbilityCooldown <= 0.f && bSeesTarget && Distance < 8.0f &&
            (E.bMiniBoss || Spec.Boss || (E.bElite && E.Type == 0) || E.Type == 7 || E.Type == 23 ||
             E.Type == 24 || E.Type == 19 || E.Type == 12 || E.Type == 21 ||
             E.Type == 6 || E.Type == 18 || E.Type == 14 ||
             (E.Type >= 34 && E.Type <= 51)))
        {
            E.LockedTarget = Target;
            E.AttackWindup = E.bMiniBoss || Spec.Boss ? 0.85f : 0.55f;
            continue;
        }
        if (E.Telegraph > 0.f)
        {
            E.Telegraph -= DeltaSeconds;
            if (E.Telegraph <= 0.f)
            {
                E.Telegraph = 0.f;
                // Enemies cannot swing through walls, either.
                if (Distance < Reach + 0.16f && bSeesTarget)
                {
                    const float Damage = Spec.Damage * FAVContent::LevelDamageMultiplier(E.Level) *
                        (E.bElite ? 1.35f : 1.f) * (E.Equipment == 3 ? 1.25f : 1.f)*
                        Affix.OutgoingScale;
                    if (bTargetEnemy)
                    {
                        for (int32 J = 0; J < Enemies.Num(); ++J)
                        {
                            if (J != I && Enemies[J].Faction != E.Faction &&
                                FVector2D::Distance(Enemies[J].Pos, Target) < 0.6f)
                            { ApplyDamageToEnemy(Enemies[J],Damage); break; }
                        }
                    }
                    else
                    {
                        const float BeforeHP=HP;
                        DamageHero(Damage,E.Pos,true);
                        if(BeforeHP>HP)
                            OnAffixedEnemyHitHero(E,BeforeHP-HP);
                    }
                }
                E.Cooldown = (Spec.Boss ? 1.15f : E.Type == 7 ? 1.6f :
                             E.Type == 1 ? 1.4f : 1.08f)*Affix.CooldownScale;
            }
            continue;
        }
        if (E.Cooldown > 0.f || Distance > 8.3f || !bSeesTarget) continue;

        const bool bRangedAttack = ((Spec.Ranged || E.Equipment == 2) && Distance >= Reach) ||
                                   (bMeleeBoss && E.Type == 8 && Distance >= 2.2f) ||
                                   (E.bMiniBoss && Distance >= 2.3f);
        if (bRangedAttack)
        {
            // Fix: ranged enemies require actual line of sight, not distance alone.
            if (!FAVDungeonNavigation::LineOfSight(Tiles, MapW, MapH, E.Pos, Target))
            {
                E.Cooldown = 0.20f;
                continue;
            }
            FAVBolt Bolt;
            Bolt.Pos = E.Pos;
            Bolt.Velocity = Direction * (Spec.Boss ? 7.8f : E.Type == 10 ? 9.f : 6.8f)*
                Affix.BoltSpeedScale;
            Bolt.Damage = FMath::RoundToInt(Spec.Damage * FAVContent::LevelDamageMultiplier(E.Level) *
                (E.bElite ? 1.35f : 1.f) * (E.Equipment == 3 ? 1.25f : 1.f)*
                Affix.OutgoingScale);
            Bolt.Status = Affix.InflictedStatus!=0 ? Affix.InflictedStatus :
                (E.Type == 6 ? 1 : E.Type == 9 ? 2 : 0);
            Bolt.Affix=E.Affix;
            Bolt.SourceEnemyId=E.Id;
            Bolt.Faction = E.Faction;
            Bolts.Add(Bolt);

            if (Spec.Boss)
            {
                const FVector2D Side(-Direction.Y, Direction.X);
                const bool bFurious = E.HP <= E.MaxHP * 0.5f;
                for (int32 Sign = -1; Sign <= 1; Sign += 2)
                {
                    FAVBolt Spread = Bolt;
                    Spread.Velocity = SafeDirection(Direction + Side * ((E.bMiniBoss ? 0.18f + (E.Type - 26) * 0.075f : 0.33f) * Sign)) * 7.4f;
                    Bolts.Add(Spread);
                    if (bFurious)
                    {
                        FAVBolt Far = Bolt;
                        Far.Velocity = SafeDirection(Direction + Side * (0.65f * Sign)) * 7.1f;
                        Bolts.Add(Far);
                    }
                }
                // Minibosses use seven distinct spread widths and cadence profiles.
                E.Cooldown = (E.bMiniBoss ? (0.85f + (E.Type - 26) * 0.13f) :
                    bFurious ? (E.Type == 9 ? 0.85f : 0.96f) : 1.35f)*
                    Affix.CooldownScale;
            }
            else E.Cooldown = (E.Type == 10 ? 1.40f : E.Type == 6 ? 1.75f : 2.1f)*
                Affix.CooldownScale;
        }
        else if (Distance < Reach + 0.2f)
        {
            E.Telegraph = Spec.Boss ? 0.53f : E.Type == 7 ? 0.54f :
                          E.Type == 1 ? 0.44f : E.Type == 4 ? 0.26f : 0.31f;
        }
    }

    // Missiles are swept against both walls and actors, preventing a fast
    // projectile crossing a wall when its end point happens to be on a floor.
    for (int32 I = Bolts.Num() - 1; I >= 0; --I)
    {
        if (Phase != EAVPhase::Playing) return;
        FAVBolt& Bolt = Bolts[I];
        const FVector2D Old = Bolt.Pos;
        Bolt.Pos += Bolt.Velocity * DeltaSeconds;
        Bolt.Time -= DeltaSeconds;
        if (Bolt.Time <= 0.f || !CanMove(Bolt.Pos, 0.06f) ||
            !FAVDungeonNavigation::LineOfSight(Tiles, MapW, MapH, Old, Bolt.Pos))
        {
            // A missed normal shot breaks the precision chain only if no newer shot connected.
            if (Bolt.Visual == 1 && Bolt.ShotId > LastConnectedShotId)
            { ShotSeries = 0; ShotTargetId = -1; }
            Bolts.RemoveAtSwap(I);
            continue;
        }
        const FVector2D Segment = Bolt.Pos - Old;
        const float LenSq = Segment.SizeSquared();
        if (Bolt.bFriendly)
        {
            bool bHit = false;
            for (int32 J = Enemies.Num() - 1; J >= 0; --J)
            {
                FAVEnemy& E = Enemies[J];
                if (Bolt.HitEnemyIds.Contains(E.Id)) continue;
                const float T = LenSq > KINDA_SMALL_NUMBER ?
                    FMath::Clamp(FVector2D::DotProduct(E.Pos - Old, Segment) / LenSq, 0.f, 1.f) : 0.f;
                const float Radius = FAVContent::Enemy(E.Type, Floor).Boss ? 0.76f : 0.47f;
                if ((Old + Segment * T - E.Pos).SizeSquared() > Radius * Radius) continue;
                // The three arrows from one volley may land at most twice on one target.
                if (Bolt.Visual == 2 && E.LastVolleyId == Bolt.VolleyId &&
                    E.VolleyHitCount >= 2) continue;
                int32 Damage = Bolt.Damage;
                if (Bolt.Visual == 1)
                {
                    const bool bSameTarget = ShotTargetId == E.Id &&
                        RunTime - LastShotHitTime <= 4.f;
                    ShotSeries = bSameTarget ? FMath::Min(4, ShotSeries + 1) : 1;
                    ShotTargetId = E.Id;
                    LastShotHitTime = RunTime;
                    LastConnectedShotId = Bolt.ShotId;
                    Damage = FMath::RoundToInt(Damage *
                        (1.f + 0.08f * (ShotSeries - 1)));
                }
                if (Bolt.Visual == 2)
                {
                    if (E.LastVolleyId != Bolt.VolleyId)
                    { E.LastVolleyId = Bolt.VolleyId; E.VolleyHitCount = 0; }
                    ++E.VolleyHitCount;
                }
                if(Bolt.Visual==3 && Bolt.HitEnemyIds.Num()>0)
                    Damage=FMath::RoundToInt(Damage*0.45f);
                Damage=FMath::RoundToInt(ModifiedAttackDamage(Damage,E,true,Bolt.Visual!=1));
                Bolt.HitEnemyIds.Add(E.Id);
                --Bolt.PierceRemaining;
                ApplyDamageToEnemy(E,Damage);
                if(Bolt.Status==1)E.PoisonTime=FMath::Max(E.PoisonTime,3.f);
                if(Bolt.Status==2)E.BurnTime=FMath::Max(E.BurnTime,3.f);
                E.Flash = 0.16f;
                E.Knockback += SafeDirection(E.Pos - Old) * 3.1f;
                AddEffect(E.Pos, 2, 0.4f);
                if (E.HP <= 0.f) KillEnemy(J);
                bHit=true;
                if(Bolt.PierceRemaining<=0) break;
            }
            if(bHit && Bolt.PierceRemaining<=0) Bolts.RemoveAtSwap(I);
        }
        else
        {
            bool bHit = false;
            // Enemy bolts collide with members of the opposing faction.
            for (int32 J = Enemies.Num() - 1; J >= 0; --J)
            {
                FAVEnemy& Victim = Enemies[J];
                if (Victim.Faction == Bolt.Faction || Victim.HP <= 0.f) continue;
                const float T = LenSq > KINDA_SMALL_NUMBER ?
                    FMath::Clamp(FVector2D::DotProduct(Victim.Pos - Old, Segment) / LenSq, 0.f, 1.f) : 0.f;
                if ((Old + Segment * T - Victim.Pos).SizeSquared() > 0.1764f) continue;
                ApplyDamageToEnemy(Victim,Bolt.Damage);
                Victim.Flash = 0.15f;
                AddEffect(Victim.Pos, 2, 0.5f);
                bHit = true;
                break;
            }
            if (!bHit)
            {
                const float T = LenSq > KINDA_SMALL_NUMBER ?
                    FMath::Clamp(FVector2D::DotProduct(Hero - Old, Segment) / LenSq, 0.f, 1.f) : 0.f;
                if ((Old + Segment * T - Hero).SizeSquared() < 0.1849f)
                {
                    const bool bVulnerable = Invulnerability <= 0.f;
                    const int32 Status = Bolt.Status;
                    const float BeforeHP=HP;
                    DamageHero(Bolt.Damage, Bolt.Pos);
                    if(BeforeHP>HP && Bolt.Affix==
                       static_cast<int32>(EAVEnemyAffix::Blooddrinker))
                        for(FAVEnemy& Source:Enemies)
                            if(Source.Id==Bolt.SourceEnemyId && Source.HP>0.f)
                            {
                                Source.HP=FMath::Min(Source.MaxHP,
                                    Source.HP+(BeforeHP-HP)*0.20f);
                                break;
                            }
                    if (bVulnerable && Phase != EAVPhase::Dead)
                    {
                        if(Status==1)PoisonTime=FMath::Max(PoisonTime,
                            4.2f*StatusResistanceFactor(true));
                        if(Status==2)BurnTime=FMath::Max(BurnTime,
                            3.3f*StatusResistanceFactor(false));
                    }
                    bHit = true;
                }
            }
            if (bHit) Bolts.RemoveAtSwap(I);
        }
    }

    if (Phase != EAVPhase::Playing) return;
    for (FAVTrap& Trap : Traps)
    {
        Trap.Cooldown = FMath::Max(0.f, Trap.Cooldown - DeltaSeconds);
        if (Trap.Windup > 0.f)
        {
            Trap.Windup -= DeltaSeconds;
            if (Trap.Windup <= 0.f)
            {
                Trap.Windup = 0.f;
                Trap.Cooldown = Trap.Type == 0 ? 2.8f : Trap.Type >= 3 ? 4.4f : 3.7f;
                AddEffect(Trap.Pos, 2, Trap.Type == 1 ? 1.4f : 0.8f);
                if (Trap.Type == 0 && FVector2D::Distance(Trap.Pos, Hero) < 0.88f)
                    DamageHero(14.f + Floor * 3.f, Trap.Pos);
                else if (Trap.Type == 1)
                {
                    if (FVector2D::Distance(Trap.Pos, Hero) < 1.5f)
                    {
                        BurnTime = FMath::Max(BurnTime, 3.5f);
                        DamageHero(10.f + Floor * 2.f, Trap.Pos);
                    }
                }
                else if (Trap.Type >= 3 && Trap.Type <= 5)
                {
                    const float Radius = Trap.Type == 3 ? 1.4f :
                                         Trap.Type == 4 ? 1.65f : 1.1f;
                    if (FVector2D::Distance(Trap.Pos, Hero) < Radius)
                        DamageHero((Trap.Type == 3 ? 26.f : 17.f) + Floor * 2.f, Trap.Pos);
                }
                else if (Trap.Type == 6)
                {
                    const FVector2D Dir = SafeDirection(Hero - Trap.Pos);
                    const FVector2D Side(-Dir.Y, Dir.X);
                    for (int32 K = -1; K <= 1; ++K)
                    {
                        FAVBolt Spear;
                        Spear.Pos = Trap.Pos;
                        Spear.Velocity = SafeDirection(Dir + Side * (K * 0.19f)) * 11.f;
                        Spear.Damage = 10 + Floor * 2;
                        Spear.Time = 1.f;
                        Bolts.Add(Spear);
                    }
                }
                else if (Trap.Type == 2)
                {
                    FAVBolt Dart;
                    Dart.Pos = Trap.Pos;
                    Dart.Velocity = SafeDirection(Hero - Trap.Pos) * 10.f;
                    Dart.Damage = 8 + Floor * 2;
                    Dart.Time = 1.1f;
                    Bolts.Add(Dart);
                }
            }
        }
        else if (Trap.Cooldown <= 0.f &&
                 FVector2D::Distance(Trap.Pos, Hero) < 0.78f)
            Trap.Windup = Trap.Type == 3 ? 1.f : Trap.Type == 4 ? 0.78f :
                          Trap.Type == 5 ? 1.15f : Trap.Type == 6 ? 0.8f : 0.62f;
    }

    // Contact areas apply a temporary status, but not during a dodge iframe.
    for (FAVHazard& Hazard : Hazards)
    {
        Hazard.Cooldown = FMath::Max(0.f, Hazard.Cooldown - DeltaSeconds);
        if (Hazard.Cooldown > 0.f ||
            FVector2D::Distance(Hero, Hazard.Pos) > 1.f ||
            Invulnerability > 0.f) continue;
        Hazard.Cooldown = 1.25f;
        if(Hazard.Type==0)PoisonTime=FMath::Max(PoisonTime,
            4.f*StatusResistanceFactor(true));
        else BurnTime=FMath::Max(BurnTime,
            3.f*StatusResistanceFactor(false));
        AddEffect(Hazard.Pos, 2, 0.6f);
    }

    if (Phase != EAVPhase::Playing) return;
    for (int32 I = Pickups.Num() - 1; I >= 0; --I)
    {
        FAVPickup& P = Pickups[I];
        P.Time += DeltaSeconds;
        if (FVector2D::Distance(P.Pos, Hero) < 0.82f)
        {
            switch (P.Type)
            {
            case 0: Gold += P.Amount; break;
            case 1: HP = FMath::Min(MaxHP, HP + P.Amount); break;
            case 2: Stamina = FMath::Min(MaxStamina, Stamina + P.Amount); break;
            case 3: ++Potions; break;
            case 4:
                AttackPower += P.Amount;
                Toast = TEXT("ASHEN SHARD  +2 DAMAGE");
                ToastTime = 1.8f;
                break;
            case 5: MaxHP += 12.f; HP = FMath::Min(MaxHP, HP + 12.f);
                Toast = TEXT("VITALITY SHARD: +12 MAX HP"); ToastTime = 2.f; break;
            case 6: MaxStamina += 10.f; Stamina = FMath::Min(MaxStamina, Stamina + 10.f);
                Toast = TEXT("ENDURANCE SHARD: +10 MAX STAMINA"); ToastTime = 2.f; break;
            case 7: MoveSpeed += 0.18f;
                Toast = TEXT("HASTE SHARD: +0.18 SPEED"); ToastTime = 2.f; break;
            case 8:
                if ((EquippedSlotsMask & 1) == 0)
                {
                    EquippedSlotsMask |= 1; ++EquippedGear;
                    ArmorRating = FMath::Min(0.45f, ArmorRating + 0.08f);
                    Toast = TEXT("CHESTPLATE EQUIPPED"); 
                }
                else { ++BackpackArmor; Toast = TEXT("CHESTPLATE STORED IN PACK"); }
                ToastTime = 2.4f; break;
            case 9:
                if ((EquippedSlotsMask & 2) == 0)
                {
                    EquippedSlotsMask |= 2; ++EquippedGear; RangedBonus += 2.f;
                    Toast = TEXT("BOW EQUIPPED");
                }
                else { ++BackpackBows; Toast = TEXT("BOW STORED IN PACK"); }
                ToastTime = 2.4f; break;
            case 11:
                if(InventoryItems.Num()>=48) {
                    Toast=TEXT("PACK FULL: SELL GEAR AT CAMP"); ToastTime=2.f;
                    continue;
                }
                if(FAVEquipment::Find(P.Amount)) {
                    InventoryItems.Add(P.Amount);
                    Toast=TEXT("GEAR FOUND: ")+GearName(P.Amount);
                    ToastTime=3.f;
                }
                break;
            case 10:
                if ((EquippedSlotsMask & 4) == 0)
                {
                    EquippedSlotsMask |= 4; ++EquippedGear; AttackPower += 3.f;
                    Toast = TEXT("WEAPON EQUIPPED");
                }
                else { ++BackpackWeapons; Toast = TEXT("WEAPON STORED IN PACK"); }
                ToastTime = 2.4f; break;
            default: break;
            }
            Pickups.RemoveAtSwap(I);
        }
    }
}

void AAVGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    // The old 50ms clamp silently slowed the whole world below 20 FPS.
    // Subdivide up to half a second of real elapsed time into stable steps.
    // Very long OS stalls are capped to avoid simulating unsafe seconds of
    // unseen attacks in a single frame; normal 10/15/30 FPS is unaffected.
    const float FrameDelta = FMath::Clamp(DeltaSeconds, 0.f, 0.5f);
    PollInput(FrameDelta);
    if (Phase != EAVPhase::Playing) return;
    const int32 Steps = FMath::Clamp(FMath::CeilToInt(FrameDelta / (1.f / 45.f)), 1, 24);
    const float DT = FrameDelta / Steps;
    for (int32 Step = 0; Step < Steps; ++Step)
    {
        if (Phase != EAVPhase::Playing) break;
        RunTime += DT;
        AttackCooldown = FMath::Max(0.f, AttackCooldown - DT);
        KnifeCooldown = FMath::Max(0.f, KnifeCooldown - DT);
        SpinCooldown = FMath::Max(0.f, SpinCooldown - DT);
        DashTime = FMath::Max(0.f, DashTime - DT);
        DashCooldown = FMath::Max(0.f, DashCooldown - DT);
        Invulnerability = FMath::Max(0.f, Invulnerability - DT);
        ScreenShake = FMath::Max(0.f, ScreenShake - DT);
        Stamina=FMath::Min(MaxStamina,Stamina+
            14.5f*(1.f+(GearStat(AVStaminaRegenPercent)+
            6.f*SkillRank(12))*0.01f)*DT);
        RewardDamageTime=FMath::Max(0.f,RewardDamageTime-DT);
        DashBonusTime=FMath::Max(0.f,DashBonusTime-DT);
        RevengeTime=FMath::Max(0.f,RevengeTime-DT);
        LastKillBoostTime=FMath::Max(0.f,LastKillBoostTime-DT);
        FoodAttackTime=FMath::Max(0.f,FoodAttackTime-DT);
        FoodGuardTime=FMath::Max(0.f,FoodGuardTime-DT);
        FoodSpeedTime=FMath::Max(0.f,FoodSpeedTime-DT);
        FoodCritTime=FMath::Max(0.f,FoodCritTime-DT);
        FoodProjectileTime=FMath::Max(0.f,FoodProjectileTime-DT);
        FoodBossTime=FMath::Max(0.f,FoodBossTime-DT);
        FoodPoisonWardTime=FMath::Max(0.f,FoodPoisonWardTime-DT);
        GuardWhirlTime=FMath::Max(0.f,GuardWhirlTime-DT);
        ChainCooldown=FMath::Max(0.f,ChainCooldown-DT);
        ShieldCooldown=FMath::Max(0.f,ShieldCooldown-DT);
        ComboTime = FMath::Max(0.f, ComboTime - DT);
        if (ComboTime <= 0.f) Combo = 0;
        ToastTime = FMath::Max(0.f, ToastTime - DT);
        RefreshAwareness(DT);
        if(!bInSafeCamp&&!bInLobby&&!bInBossArena)UpdateRoomEvents(DT);
        if (!bInSafeCamp && !bInLobby) UpdateCombat(DT);
        if (Phase != EAVPhase::Playing) break;

        PoisonTime = FMath::Max(0.f, PoisonTime - DT);
        BurnTime = FMath::Max(0.f, BurnTime - DT);
        if (PoisonTime > 0.f || BurnTime > 0.f)
        {
            StatusTick += DT;
            if (StatusTick >= 1.f)
            {
                StatusTick -= 1.f;
                const float DotDamage = (PoisonTime > 0.f ? 4.f : 0.f) +
                                        (BurnTime > 0.f ? 6.f : 0.f);
                DamageHero(DotDamage, Hero);
            }
        }
        else StatusTick = 0.f;

        for (int32 I = Effects.Num() - 1; I >= 0; --I)
        {
            Effects[I].Life -= DT;
            if (Effects[I].Life <= 0.f) Effects.RemoveAtSwap(I);
        }
    }
}
