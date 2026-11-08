#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AVGameMode.generated.h"

UENUM()
enum class EAVPhase : uint8 { Playing, Upgrade, Relic, Shop, Paused, Dead, Victory, Inventory, Skills };

USTRUCT()
struct FAVEnemy
{
    GENERATED_BODY()
    FVector2D Pos = FVector2D::ZeroVector;
    FVector2D Knockback = FVector2D::ZeroVector;
    int32 Type = 0; // enemy archetype ID
    int32 Level = 1;
    int32 Equipment = 0; // 0 none, 1 armor, 2 bow, 3 enchanted weapon
    int32 Faction = 0; // 0 dungeon, 1 bandits
    FVector2D LastSeenHero = FVector2D::ZeroVector;
    float MemoryTime = 0.f;
    float DodgeCooldown = 0.f;
    float AbilityCooldown = 0.f;
    float AlertTime = 0.f;
    float PathRefresh = 0.f;
    FVector2D CachedPathDirection = FVector2D::ZeroVector;
    float AttackWindup = 0.f;
    float PhaseShiftTime = 0.f;
    bool bPhaseShifted = false;
    FVector2D LockedTarget = FVector2D::ZeroVector;
    int32 AttackPattern = 0;
    bool bMiniBoss = false;
    float HP = 10.f;
    float MaxHP = 10.f;
    float Cooldown = 0.f;
    float Flash = 0.f;
    float Wander = 0.f;
    float Telegraph = 0.f; // Melee attack wind-up, rendered by the HUD.
    bool bElite = false;
    int32 Id = 0;
    int32 LastVolleyId = -1; // transient overlap protection
    int32 VolleyHitCount = 0;
};

USTRUCT()
struct FAVPickup
{
    GENERATED_BODY()
    FVector2D Pos = FVector2D::ZeroVector;
    int32 Type = 0; // 0 gold, 1 health, 2 stamina, 3 potion, 4 damage shard
    int32 Amount = 1;
    float Time = 0.f;
};

USTRUCT()
struct FAVBolt
{
    GENERATED_BODY()
    FVector2D Pos = FVector2D::ZeroVector;
    FVector2D Velocity = FVector2D::ZeroVector;
    float Time = 2.f;
    int32 Damage = 10;
    int32 PierceRemaining = 1; // how many unique enemies a friendly shot can hit
    int32 Visual = 0; // 0 existing bolt; 1 normal arrow; 2 volley; 3 piercing shot
    int32 ShotId = 0; // normal-shot sequence for the Marksman passive
    int32 VolleyId = 0;
    TArray<int32> HitEnemyIds; // transient, projectiles are not in SaveGame
    bool bFriendly = false;
    int32 Status = 0; // poison=1, burn=2
    int32 Faction = 0; // 0 dungeon, 1 bandits; hero projectiles use bFriendly
};

USTRUCT()
struct FAVEffect
{
    GENERATED_BODY()
    FVector2D Pos = FVector2D::ZeroVector;
    float Life = 0.3f;
    float Duration = 0.3f;
    float Radius = 1.f;
    int32 Kind = 0; // 0 melee, 1 spin, 2 hit, 3 death
    float Angle = 0.f;
};

USTRUCT()
struct FAVTrap
{
    GENERATED_BODY()
    FVector2D Pos = FVector2D::ZeroVector;
    int32 Type = 0; // 0 spikes, 1 flame vent, 2 dart launcher
    float Windup = 0.f;
    float Cooldown = 0.f;
};

USTRUCT()
struct FAVChest
{
    GENERATED_BODY()
    FVector2D Pos = FVector2D::ZeroVector;
    bool bOpened = false;
    bool bMimic = false;
};

USTRUCT()
struct FAVHazard
{
    GENERATED_BODY()
    FVector2D Pos = FVector2D::ZeroVector;
    int32 Type = 0;
    float Cooldown = 0.f;
};

USTRUCT()
struct FAVProp
{
    GENERATED_BODY()
    FVector2D Pos = FVector2D::ZeroVector;
    float HP = 25.f;
    int32 Type = 0;
};

/** Layered non-colliding world prop. Rune nodes can be activated once. */
USTRUCT()
struct FAVDecor
{
    GENERATED_BODY()
    FVector2D Pos = FVector2D::ZeroVector;
    int32 Type = 0; // 0..2 crypt, 3..5 spores, 6..8 foundry
    bool bActivated = false;
    float Phase = 0.f;
};

struct FAVWorldSnapshot
{
    TArray<uint8> Tiles, Visible, Explored;
    TArray<int32> FlowDistances;
    TArray<FAVEnemy> Enemies;
    TArray<FAVPickup> Pickups;
    TArray<FAVBolt> Bolts;
    TArray<FAVTrap> Traps;
    TArray<FAVChest> Chests;
    TArray<FAVHazard> Hazards;
    TArray<FAVProp> Props;
    TArray<FAVDecor> Decorations;
    TArray<FAVEffect> Effects;
    TArray<FVector2D> CampDoors, CampWalls;
    FVector2D Hero, Exit, Shrine, Merchant, BanditCampCenter;
    bool bMerchantSpawned = false, bBanditCamp = false, bShrineUsed = false;
};

UCLASS()
class ASHENVAULTS_API AAVGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AAVGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    static constexpr int32 FloorsPerBiome = 5;
    // Unbounded descent; every fifth floor is a biome guardian floor.
    int32 MapW = 52; // fourth biome is larger; original maps keep 52x38
    int32 MapH = 38;
    static constexpr float TilePixels = 24.f;
    static constexpr float ViewW = 960.f;
    static constexpr float ViewH = 540.f;

    UPROPERTY(BlueprintReadOnly) int32 Floor = 1;
    UPROPERTY(BlueprintReadOnly) int32 HeroClass = 0; // 3 = Marksman
    UPROPERTY(BlueprintReadOnly) int32 Level = 1;
    UPROPERTY(BlueprintReadOnly) int32 XP = 0;
    UPROPERTY(BlueprintReadOnly) int32 Gold = 0;
    UPROPERTY(BlueprintReadOnly) int32 Kills = 0;
    UPROPERTY(BlueprintReadOnly) int32 RunSeed = 0;
    UPROPERTY(BlueprintReadOnly) int32 Combo = 0;
    UPROPERTY(BlueprintReadOnly) int32 Potions = 2;
    UPROPERTY(BlueprintReadOnly) int32 RelicCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 Echoes = 0;
    UPROPERTY(BlueprintReadOnly) int32 VisualQuality = 2; // 0 low, 1 medium, 2 high
    UPROPERTY(BlueprintReadOnly) float HP = 100.f;
    UPROPERTY(BlueprintReadOnly) float MaxHP = 100.f;
    UPROPERTY(BlueprintReadOnly) float Stamina = 100.f;
    UPROPERTY(BlueprintReadOnly) float MaxStamina = 100.f;
    UPROPERTY(BlueprintReadOnly) float AttackPower = 18.f;
    UPROPERTY(BlueprintReadOnly) float MoveSpeed = 4.8f;
    UPROPERTY(BlueprintReadOnly) float ArmorRating = 0.f;
    UPROPERTY(BlueprintReadOnly) float RangedBonus = 0.f;
    UPROPERTY(BlueprintReadOnly) int32 ShotSeries = 0;
    UPROPERTY(BlueprintReadOnly) int32 EquippedGear = 0;
    // v0.9.0 item instances: 0 weapon, 1 head, 2 chest, 3 hands, 4 legs,
    // 5 boots, 6 and 7 accessories. -1 means empty.
    TArray<int32> EquipmentSlots;
    TArray<int32> InventoryItems;
    TArray<uint8> LearnedSkills;
    int32 SkillPoints = 0;
    int32 GearSelection = 0;
    int32 SkillSelection = 0;
    bool bFreeRespecUsed = false;
    bool bLastStandUsed = false;
    float EquipmentAppliedHP = 0.f;
    float EquipmentAppliedStamina = 0.f;
    float EquipmentAppliedAttack = 0.f;
    float EquipmentAppliedSpeed = 0.f;
    float EquipmentAppliedArmor = 0.f;
    float RewardDamageTime = 0.f;
    float DashBonusTime = 0.f;
    float RevengeTime = 0.f;
    int32 BasicAttackSequence = 0;

    EAVPhase Phase = EAVPhase::Playing;
    FVector2D Hero = FVector2D(4.f, 4.f);
    FVector2D HeroVelocity = FVector2D::ZeroVector;
    FVector2D Aim = FVector2D(1.f, 0.f);
    FVector2D Exit = FVector2D(8.f, 8.f);
    FVector2D Shrine = FVector2D(8.f, 8.f);
    bool bShrineUsed = false;
    FVector2D Merchant = FVector2D::ZeroVector;
    bool bMerchantSpawned = false;
    bool ShopPurchased[3] = {false, false, false};
    int32 RelicChoices[3] = {0, 1, 2};
    uint32 RelicMask = 0;
    bool bRelicPending = false;
    float PoisonTime = 0.f;
    float BurnTime = 0.f;
    float StatusTick = 0.f;
    float DashCooldown = 0.f;
    float ComboTime = 0.f;
    FString Toast;
    float ToastTime = 0.f;
    TArray<uint8> Tiles;
    // Every floor is initially unknown. Visible is the current line-of-sight
    // mask, Explored is persistent for that floor.
    TArray<uint8> Visible;
    TArray<uint8> Explored;
    TArray<int32> FlowDistances;
    TArray<FAVEnemy> Enemies;
    bool bBanditCamp = false;
    bool bInSafeCamp = false, bInLobby = false, bInBossArena = false;
    bool bGuardianDefeated = false, bHasSafeCampDoor = false;
    FVector2D SafeCampDoor = FVector2D::ZeroVector;
    int32 PendingGuardian = -1;
    int32 PackGold = 0, PackMaterials = 0;
    int32 BackpackArmor = 0, BackpackBows = 0, BackpackWeapons = 0;
    int32 EquippedSlotsMask = 0;
    int32 WeaponUpgrade = 0, ArmorUpgrade = 0;
    FAVWorldSnapshot StoredWorld;
    bool bWorldStored = false;
    TArray<FVector2D> CampDoors;
    TArray<FVector2D> CampWalls;
    FVector2D BanditCampCenter = FVector2D::ZeroVector;
    TArray<FAVPickup> Pickups;
    TArray<FAVBolt> Bolts;
    TArray<FAVTrap> Traps;
    TArray<FAVChest> Chests;
    TArray<FAVHazard> Hazards;
    TArray<FAVProp> Props;
    TArray<FAVDecor> Decorations;
    TArray<FAVEffect> Effects;
    float AttackCooldown = 0.f;
    float KnifeCooldown = 0.f;
    float SpinCooldown = 0.f;
    float DashTime = 0.f;
    float Invulnerability = 0.f;
    float ScreenShake = 0.f;
    float RunTime = 0.f;
    int32 BestFloor = 1;
    int32 BestKills = 0;
    int32 LifetimeGold = 0;

    bool IsFloor(int32 X, int32 Y) const;
    bool IsVisibleCell(int32 X, int32 Y) const;
    bool IsExploredCell(int32 X, int32 Y) const;
    bool IsVisibleAt(const FVector2D& Pos) const;
    int32 UnopenedChests() const;
    bool CanMove(const FVector2D& Pos, float Radius) const;
    int32 EnemiesRemaining() const { return Enemies.Num(); }
    int32 NextXP() const { return 8 + Level * 7; }
    bool IsExitReady() const { return Enemies.Num() == 0; }
    bool HasRelic(int32 Index) const { return Index >= 0 && Index < 6 && (RelicMask & (1u << Index)) != 0; }
    int32 CurrentBiome() const { return ((Floor - 1) / FloorsPerBiome) % 4; }
    int32 BiomeDepth() const { return (Floor - 1) % FloorsPerBiome + 1; }
    int32 BiomeCycle() const { return (Floor - 1) / (FloorsPerBiome * 4); }
    bool IsGuardianFloor() const { return BiomeDepth() == FloorsPerBiome; }
    int32 ShopPrice(int32 Slot) const;
    void NewRun(bool bSameSeed = false);
    int32 SkillIdAt(int32 Selection) const;
    int32 SkillRank(int32 Id) const;
    float GearStat(int32 Stat) const;
    bool HasGearPerk(int32 PerkId) const;
    FString GearName(int32 ItemId) const;

private:
    FRandomStream RNG;
    bool PrevAttack = false;
    bool PrevSpin = false;
    bool PrevDash = false;
    bool PrevHeal = false;
    bool PrevUse = false;
    bool PrevPause = false;
    bool PrevEnter = false;
    bool PrevReplay = false;
    bool PrevQuality = false;
    bool PrevHero = false;
    bool PrevChoices[3] = { false, false, false };
    FVector2D DashDirection = FVector2D(1.f, 0.f);
    int32 ShotTargetId = -1;
    int32 ShotCounter = 0;
    int32 LastConnectedShotId = 0;
    int32 VolleyCounter = 0;
    float LastShotHitTime = -100.f;
    int32 LastVisionX = -1;
    int32 LastVisionY = -1;
    float FlowRefreshTime = 0.f;

    void GenerateFloor();
    void RebuildCharacterBonuses();
    void EquipInventoryItem(int32 Index);
    void UnequipSlot(int32 Slot);
    void LearnSkill(int32 Selection);
    void ResetSkills();
    void SpawnGearDrop(const FVector2D& Pos, int32 MinimumRarity = 0);
    float ModifiedAttackDamage(float Damage, const FAVEnemy& Target, bool bRanged, bool bAbility);
    float BasicAttackInterval() const;
    float ProjectileSpeedMultiplier() const;
    void StoreDungeon();
    void RestoreDungeon();
    void EnterSafeCamp();
    void EnterBossArena();
    void EnterLobby();
    void ResumeFromLobby();
    void SaveCheckpoint();
    void LoadCheckpointWorld();
    void MakeSafeRoom(bool bLobby);
    void ForgeUpgrade(int32 Slot);
    void SellPack();
    void PollInput(float DeltaSeconds);
    void UpdateCombat(float DeltaSeconds);
    void RefreshAwareness(float DeltaSeconds, bool bForce = false);
    void ThrowKnife();
    void ShootArrow();
    void FireVolley();
    void PiercingShot();
    void OfferRelic();
    void ChooseRelic(int32 Choice);
    void BuyFromMerchant(int32 Slot);
    void StrikeProps(bool bSpin, float Damage);
    bool OpenNearbyChest();
    bool OpenNearbyCampDoor();
    bool ActivateNearbyEcho();
    void DamageHero(float Damage, const FVector2D& Source);
    void Strike(bool bSpin);
    void KillEnemy(int32 Index);
    void GrantExperience(int32 Amount);
    void SelectUpgrade(int32 Choice);
    void AddEffect(const FVector2D& Pos, int32 Kind, float Radius = 1.f, float Angle = 0.f);
    void DropLoot(const FVector2D& Pos, bool bBoss);
    void CastEnemyAbility(int32 Index, const FVector2D& Target);
    void SpawnEnemyBolt(const FAVEnemy& Enemy, const FVector2D& Direction,
                        float Speed, float DamageScale, int32 Status = 0);
    void RecordRun();
    FVector2D MoveWithCollision(const FVector2D& From, const FVector2D& Velocity, float DeltaSeconds, float Radius) const;
};
