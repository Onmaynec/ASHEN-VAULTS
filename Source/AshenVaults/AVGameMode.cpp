#include "AVGameMode.h"
#include "AVHUD.h"
#include "AVSaveGame.h"
#include "AVDungeonNavigation.h"
#include "AVContent.h"
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
    NewRun();
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

void AAVGameMode::NewRun(bool bSameSeed)
{
    Floor = 1; Level = 1; XP = 0; Gold = 0; Kills = 0; Potions = 2;
    MaxHP = FAVContent::HeroHealth(HeroClass); HP = MaxHP;
    MaxStamina = FAVContent::HeroStamina(HeroClass); Stamina = MaxStamina;
    AttackPower = FAVContent::HeroAttack(HeroClass);
    MoveSpeed = FAVContent::HeroSpeed(HeroClass); RunTime = 0.f;
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
    Hazards.Reset(); Props.Reset(); Decorations.Reset();
    HeroVelocity = FVector2D::ZeroVector;
    bMerchantSpawned = false;
    bBanditCamp = false;
    for (int32 I = 0; I < 3; ++I) ShopPurchased[I] = false;
    PoisonTime = 0.f; BurnTime = 0.f; StatusTick = 0.f;
    AttackCooldown = 0.f; KnifeCooldown = 0.f; SpinCooldown = 0.f;
    DashTime = 0.f; DashCooldown = 0.f;
    bShrineUsed = false;
    struct FRoom { int32 X, Y, W, H; FVector2D Center() const { return FVector2D(X + W / 2.f, Y + H / 2.f); } };
    TArray<FRoom> Rooms;
    const int32 TargetCount = FMath::Min(20, 9 + Floor * 2);
    for (int32 Attempt = 0; Attempt < 220 && Rooms.Num() < TargetCount; ++Attempt)
    {
        const int32 W = RNG.RandRange(5, 11);
        const int32 H = RNG.RandRange(5, 9);
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
            for (int32 X = R.X; X < R.X + R.W; ++X)
                Tiles[Y * MapW + X] = 1;

        if (Rooms.Num() > 0)
        {
            FVector2D A = Rooms.Last().Center(), B = R.Center();
            int32 AX = FMath::FloorToInt(A.X), AY = FMath::FloorToInt(A.Y);
            const int32 BX = FMath::FloorToInt(B.X), BY = FMath::FloorToInt(B.Y);
            const bool bHorizontalFirst = RNG.RandRange(0, 1) == 0;
            auto DigX = [&]() {
                while (AX != BX) { Tiles[AY * MapW + AX] = 1; AX += BX > AX ? 1 : -1; }
                Tiles[AY * MapW + AX] = 1;
            };
            auto DigY = [&]() {
                while (AY != BY) { Tiles[AY * MapW + AX] = 1; AY += BY > AY ? 1 : -1; }
                Tiles[AY * MapW + AX] = 1;
            };
            if (bHorizontalFirst) { DigX(); DigY(); }
            else { DigY(); DigX(); }
        }
        Rooms.Add(R);
    }
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
        bMerchantSpawned = true;
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
            if (FVector2D::Distance(Candidate, Hero) < 4.f ||
                FVector2D::Distance(Candidate, Exit) < 2.2f ||
                FVector2D::Distance(Candidate, Shrine) < 2.2f ||
                (bMerchantSpawned && FVector2D::Distance(Candidate, Merchant) < 2.2f))
                continue;
            FAVTrap Trap;
            Trap.Pos = Candidate;
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
        bool bBlocked = false;
        for (const FAVTrap& Trap : Traps)
            bBlocked |= FVector2D::Distance(Chest.Pos, Trap.Pos) < 1.3f;
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
        const int32 Count = RNG.RandRange(2 + Floor / 3, FMath::Min(8, 3 + Floor));
        for (int32 E = 0; E < Count; ++E)
        {
            FAVEnemy Enemy;
            Enemy.Pos = FVector2D(RNG.FRandRange(R.X + 1.f, R.X + R.W - 1.f),
                                  RNG.FRandRange(R.Y + 1.f, R.Y + R.H - 1.f));
            Enemy.Type = FAVContent::EnemyRoll(Floor, RNG.RandRange(0, 99));
            Enemy.Level = FAVContent::EnemyLevel(Floor, RNG.RandRange(0, 99));
            Enemy.Equipment = RNG.FRand() < 0.085f ? RNG.RandRange(1, 3) : 0;
            if (Enemy.Type == 16 && Enemy.Equipment == 2) Enemy.Type = 25; // armed zombie uses archer AI
            Enemy.MaxHP = FAVContent::Enemy(Enemy.Type, Floor).Health *
                FAVContent::LevelHealthMultiplier(Enemy.Level) *
                (Enemy.Equipment == 1 ? 1.35f : 1.f);
            Enemy.bElite = RNG.FRand() < FMath::Min(0.26f, 0.10f + 0.025f * Floor);
            if (Enemy.bElite) Enemy.MaxHP *= 1.65f;
            Enemy.HP = Enemy.MaxHP;
            Enemy.Cooldown = RNG.FRandRange(0.3f, 1.6f);
            if (bMerchantSpawned && FVector2D::Distance(Enemy.Pos, Merchant) < 2.f)
                Enemy.Pos = FVector2D(R.X + 1.2f, R.Y + 1.2f);
            Enemy.Id = NextId++;
            Enemies.Add(Enemy);
        }
    }
    // Optional bandit hideout: occupied chamber with treasure, ambush and rival faction.
    if (Rooms.Num() >= 5 && RNG.FRand() < 0.38f)
    {
        const int32 RoomIndex = RNG.RandRange(2, Rooms.Num() - 2);
        const FRoom& Camp = Rooms[RoomIndex];
        BanditCampCenter = Camp.Center();
        bBanditCamp = true;
        for (int32 I = 0; I < 3 + Floor / 2; ++I)
        {
            FAVEnemy Bandit;
            Bandit.Pos = FVector2D(RNG.FRandRange(Camp.X + 1.f, Camp.X + Camp.W - 1.f),
                                   RNG.FRandRange(Camp.Y + 1.f, Camp.Y + Camp.H - 1.f));
            Bandit.Type = I % 3 == 0 ? 25 : 33;
            Bandit.Faction = 1;
            Bandit.Level = FAVContent::EnemyLevel(Floor, RNG.RandRange(0, 99));
            Bandit.MaxHP = FAVContent::Enemy(Bandit.Type, Floor).Health *
                           FAVContent::LevelHealthMultiplier(Bandit.Level);
            Bandit.HP = Bandit.MaxHP;
            Bandit.Cooldown = RNG.FRandRange(0.3f, 1.1f);
            Bandit.Id = NextId++;
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
        Mini.Type = 26 + RNG.RandRange(0, 6);
        Mini.bMiniBoss = true;
        Mini.Pos = Arena.Center();
        Mini.Level = FAVContent::EnemyLevel(Floor, 99);
        Mini.MaxHP = FAVContent::Enemy(Mini.Type, Floor).Health *
                     FAVContent::LevelHealthMultiplier(Mini.Level);
        Mini.HP = Mini.MaxHP;
        Mini.Cooldown = 1.f;
        Mini.Id = NextId++;
        Enemies.Add(Mini);
    }
    const int32 BossType = FAVContent::Boss(Floor);
    if (BossType >= 0)
    {
        FAVEnemy Boss;
        Boss.Pos = Exit; Boss.Type = BossType;
        Boss.Level = FAVContent::EnemyLevel(Floor, 99);
        Boss.MaxHP = FAVContent::Enemy(BossType, Floor).Health *
            FAVContent::LevelHealthMultiplier(Boss.Level);
        Boss.HP = Boss.MaxHP;
        Boss.Cooldown = 1.f; Boss.Id = NextId++;
        Enemies.Add(Boss);
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
    if (Floor >= 3)
    {
        for (int32 I = 2; I < Rooms.Num() - 1; I += 2)
        {
            const FRoom& R = Rooms[I];
            FAVHazard Hazard;
            Hazard.Type = Floor >= 5 ? 1 : 0;
            Hazard.Pos = FVector2D(R.X + 1.5f, R.Y + R.H - 1.5f);
            if (FVector2D::Distance(Hazard.Pos, Shrine) > 2.f &&
                FVector2D::Distance(Hazard.Pos, Exit) > 2.f &&
                (!bMerchantSpawned || FVector2D::Distance(Hazard.Pos, Merchant) > 2.f))
                Hazards.Add(Hazard);
        }
    }
    for (int32 I = 1; I < Rooms.Num(); ++I)
    {
        const FRoom& R = Rooms[I];
        FAVProp Prop;
        Prop.Type = Floor >= 5 ? 1 : 0;
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
    RefreshAwareness(0.f, true);
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
    if (bSpin) SpinCooldown = 1.65f;
    else AttackCooldown = 0.33f;
    AddEffect(Hero, bSpin ? 1 : 0, bSpin ? 2.2f : 1.75f, FMath::Atan2(Aim.Y, Aim.X));
    const float Range = bSpin ? 2.25f : 1.92f;
    const float Damage = AttackPower * (bSpin ? (HasRelic(3) ? 2.f : 1.65f) : 1.f);
    for (int32 I = Enemies.Num() - 1; I >= 0; --I)
    {
        FAVEnemy& Enemy = Enemies[I];
        FVector2D Delta = Enemy.Pos - Hero;
        const float Distance = Delta.Size();
        if (Distance > Range + (FAVContent::Enemy(Enemy.Type, Floor).Boss ? 0.45f : 0.f)) continue;
        if (!bSpin && FVector2D::DotProduct(SafeDirection(Delta), Aim) < 0.10f) continue;
        // Fix: sword swings can no longer hit actors through solid stone walls.
        if (!FAVDungeonNavigation::LineOfSight(Tiles, MapW, MapH, Hero, Enemy.Pos)) continue;
        Enemy.HP -= Damage;
        Enemy.Flash = 0.16f;
        Enemy.Knockback += SafeDirection(Delta) * (Enemy.Type == 3 ? 1.f : 5.f);
        AddEffect(Enemy.Pos, 2, 0.55f);
        if (Enemy.HP <= 0.f) KillEnemy(I);
    }
    if (Phase == EAVPhase::Playing) StrikeProps(bSpin, Damage);
}

void AAVGameMode::ThrowKnife()
{
    if (KnifeCooldown > 0.f || Stamina < 18.f || Aim.IsNearlyZero()) return;
    Stamina -= 18.f;
    KnifeCooldown = HasRelic(2) ? 0.43f : 0.65f;
    FAVBolt Blade;
    Blade.bFriendly = true;
    Blade.Pos = Hero + Aim * 0.52f;
    Blade.Velocity = Aim * 11.f;
    Blade.Time = 1.1f;
    Blade.Damage = FMath::RoundToInt(AttackPower * (HasRelic(2) ? 1.12f : 0.83f));
    Bolts.Add(Blade);
    AddEffect(Hero, 0, 0.5f, FMath::Atan2(Aim.Y, Aim.X));
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
            Mimic.MaxHP = 64.f + Floor * 18.f;
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
        if (Decor.bActivated || Decor.Type % 3 != 2 ||
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

void AAVGameMode::DamageHero(float Damage, const FVector2D& Source)
{
    if (Invulnerability > 0.f || Phase != EAVPhase::Playing) return;
    const float AdjustedDamage = HasRelic(1) ? Damage * 0.80f : Damage;
    HP = FMath::Max(0.f, HP - AdjustedDamage);
    Invulnerability = 0.72f;
    ScreenShake = 0.26f;
    Hero = MoveWithCollision(Hero, SafeDirection(Hero - Source) * 2.8f, 0.10f, 0.27f);
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

void AAVGameMode::KillEnemy(int32 Index)
{
    if (!Enemies.IsValidIndex(Index)) return;
    const FAVEnemy Dead = Enemies[Index];
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
    GrantExperience(bBoss ? 22 + Floor * 5 : (3 + Floor) * (Dead.bElite ? 2 : 1));
    if (Dead.Equipment != 0 && RNG.FRand() < 0.14f)
    {
        FAVPickup Gear;
        Gear.Pos = Dead.Pos;
        Gear.Type = Dead.Equipment == 1 ? 5 : Dead.Equipment == 2 ? 7 : 4;
        Gear.Amount = Dead.Equipment == 3 ? 3 : 1;
        Pickups.Add(Gear);
    }
    AddEffect(Dead.Pos, 3, bBoss ? 2.2f : 0.9f);
    DropLoot(Dead.Pos, bBoss);
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
        Upgrade.Pos = Pos;
        Upgrade.Type = RNG.RandRange(5, 7);
        Upgrade.Amount = 1;
        Pickups.Add(Upgrade);
    }
    if (bBoss)
        OfferRelic();
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
    if (Choice == 0) { MaxHP += 25.f; HP = FMath::Min(MaxHP, HP + 40.f); }
    if (Choice == 1) AttackPower += 7.f;
    if (Choice == 2) { MoveSpeed += 0.55f; MaxStamina += 12.f; Stamina = MaxStamina; }
    Phase = XP >= NextXP() ? EAVPhase::Upgrade : EAVPhase::Playing;
}

void AAVGameMode::RecordRun()
{
    BestFloor = FMath::Max(BestFloor, Floor);
    BestKills = FMath::Max(BestKills, Kills);
    LifetimeGold += Gold;
    UAVSaveGame* Save = Cast<UAVSaveGame>(UGameplayStatics::CreateSaveGameObject(UAVSaveGame::StaticClass()));
    if (Save)
    {
        Save->BestFloor = BestFloor; Save->BestKills = BestKills; Save->TotalGold = LifetimeGold;
        UGameplayStatics::SaveGameToSlot(Save, TEXT("AshenVaultsRecord"), 0);
    }
}

void AAVGameMode::PollInput(float DeltaSeconds)
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) return;

    const bool bPause = Pressed(PC, EKeys::Escape, PrevPause) || PC->WasInputKeyJustPressed(EKeys::P);
    const bool bRestart = Pressed(PC, EKeys::Enter, PrevEnter);
    const bool bReplay = Pressed(PC, EKeys::R, PrevReplay); // Retry after death; knife during play.
    if (Pressed(PC, EKeys::H, PrevHero))
    {
        HeroClass = (HeroClass + 1) % FAVContent::HeroCount;
        NewRun(); // Class changes restart the run; never mix stats mid-run.
        Toast = HeroClass == 0 ? TEXT("WARDEN SELECTED") :
                HeroClass == 1 ? TEXT("DUELIST SELECTED") : TEXT("ARCANIST SELECTED");
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
    const bool bHeal = Pressed(PC, EKeys::E, PrevHeal);
    const bool bUse = Pressed(PC, EKeys::F, PrevUse);

    if (Phase == EAVPhase::Dead || Phase == EAVPhase::Victory)
    {
        if (bRestart) NewRun();
        else if (bReplay) NewRun(true);
        return;
    }
    if (Phase == EAVPhase::Shop)
    {
        if (bPause || bUse) { Phase = EAVPhase::Playing; return; }
        if (bChoice0) BuyFromMerchant(0);
        else if (bChoice1) BuyFromMerchant(1);
        else if (bChoice2) BuyFromMerchant(2);
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
    const float DashCost = HasRelic(4) ? 16.f : 23.f;
    if (bDash && Stamina >= DashCost && DashCooldown <= 0.f && DashTime <= 0.f)
    {
        Stamina -= DashCost;
        DashCooldown = HasRelic(4) ? 0.47f : 0.7f;
        DashDirection = Direction.IsNearlyZero() ? Aim : Direction;
        DashTime = 0.17f;
        Invulnerability = FMath::Max(Invulnerability, 0.25f);
        AddEffect(Hero, 1, 0.7f);
    }
    const FVector2D BeforeMovement = Hero;
    if (DashTime > 0.f)
    {
        // The dash never gains extra distance on slow frames.
        const float DashDt = FMath::Min(DashTime, DeltaSeconds);
        Hero = MoveWithCollision(Hero, DashDirection * 16.f, DashDt, 0.27f);
        if (DeltaSeconds > DashDt)
            Hero = MoveWithCollision(Hero, Direction * MoveSpeed,
                                     DeltaSeconds - DashDt, 0.27f);
    }
    else
        Hero = MoveWithCollision(Hero, Direction * MoveSpeed, DeltaSeconds, 0.27f);
    HeroVelocity = (Hero - BeforeMovement) / FMath::Max(0.001f, DeltaSeconds);

    if (bHit) Strike(false);
    // A sword can defeat a guardian and open a relic selection instantly:
    // don't process another action in the same frame after phase changes.
    if (Phase != EAVPhase::Playing) return;
    if (bSpin) Strike(true);
    if (Phase != EAVPhase::Playing) return;
    if (bReplay) ThrowKnife();
    if (Phase != EAVPhase::Playing) return;
    if (bHeal && Potions > 0 && HP < MaxHP)
    {
        --Potions; HP = FMath::Min(MaxHP, HP + 42.f);
        PoisonTime = 0.f; BurnTime = 0.f;
        AddEffect(Hero, 2, 1.f);
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
    else if (bUse && OpenNearbyChest())
    {
        // A chest may release a mimic, which re-locks the stairs.
    }
    else if (bUse && ActivateNearbyEcho())
    {
        // One-time lore artifact, reward and nearby map discovery.
    }
    else if (bUse && IsExitReady() && FVector2D::Distance(Hero, Exit) < 1.7f)
    {
        if (Floor >= FinalFloor) { Phase = EAVPhase::Victory; RecordRun(); }
        else
        {
            ++Floor;
            HP = FMath::Min(MaxHP, HP + 22.f);
            Stamina = MaxStamina;
            GenerateFloor();
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
        const FAVEnemySpec Spec = FAVContent::Enemy(E.Type, Floor);
        E.Cooldown -= DeltaSeconds;
        E.Flash = FMath::Max(0.f, E.Flash - DeltaSeconds);
        FVector2D Target = Hero;
        bool bTargetEnemy = false;
        float TargetDistance = FVector2D::Distance(Hero, E.Pos);
        for (int32 J = 0; J < Enemies.Num(); ++J)
        {
            if (J == I || Enemies[J].Faction == E.Faction) continue;
            const float D = FVector2D::Distance(E.Pos, Enemies[J].Pos);
            if (D < TargetDistance && D < 7.f)
            { Target = Enemies[J].Pos; TargetDistance = D; bTargetEnemy = true; }
        }
        const bool bSeesTarget = FAVDungeonNavigation::LineOfSight(
            Tiles, MapW, MapH, E.Pos, Target);
        if (bSeesTarget) { E.LastSeenHero = Target; E.MemoryTime = 3.5f; }
        else E.MemoryTime = FMath::Max(0.f, E.MemoryTime - DeltaSeconds);
        const FVector2D Pursuit = bSeesTarget ? Target :
            E.MemoryTime > 0.f ? E.LastSeenHero : E.Pos;
        const FVector2D ToHero = Target - E.Pos;
        const float Distance = ToHero.Size();
        const FVector2D Direction = SafeDirection(ToHero);
        const float Speed = Spec.Speed * (E.bElite ? 1.18f : 1.f);
        const bool bCaster = Spec.Ranged && !Spec.Boss;
        const bool bMeleeBoss = E.Type == 8;
        const float Reach = Spec.Boss ? (bMeleeBoss ? 2.0f : 1.6f) :
                            (E.Type == 7 || E.Type == 13) ? 1.30f : 1.08f;
        FVector2D DesiredVelocity = FVector2D::ZeroVector;
        if ((bSeesTarget || E.MemoryTime > 0.f) && E.Telegraph <= 0.f)
        {
            if (bCaster && Distance < 3.8f)
                DesiredVelocity = -Direction * Speed;
            else if (Distance > (bCaster ? 5.3f :
                                 Spec.Ranged && !bMeleeBoss ? 4.9f : Reach * 0.8f))
                DesiredVelocity = bTargetEnemy || !bSeesTarget ?
                    SafeDirection(Pursuit - E.Pos) * Speed :
                    FAVDungeonNavigation::FlowDirection(
                        Tiles, FlowDistances, MapW, MapH, E.Pos, Hero) * Speed;
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
            if (Away.SizeSquared() > 0.01f && Away.SizeSquared() < 0.6f)
                DesiredVelocity += SafeDirection(Away) * Speed * 0.35f;
        }
        E.Pos = MoveWithCollision(E.Pos, DesiredVelocity + E.Knockback,
                                  DeltaSeconds, Spec.Boss ? 0.47f : 0.26f);
        E.Knockback = FMath::Lerp(E.Knockback, FVector2D::ZeroVector,
                                  FMath::Clamp(DeltaSeconds * 10.f, 0.f, 1.f));
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
                        (E.bElite ? 1.35f : 1.f) * (E.Equipment == 3 ? 1.25f : 1.f);
                    if (bTargetEnemy)
                    {
                        for (int32 J = 0; J < Enemies.Num(); ++J)
                        {
                            if (J != I && Enemies[J].Faction != E.Faction &&
                                FVector2D::Distance(Enemies[J].Pos, Target) < 0.6f)
                            { Enemies[J].HP -= Damage; break; }
                        }
                    }
                    else DamageHero(Damage, E.Pos);
                }
                E.Cooldown = Spec.Boss ? 1.15f : E.Type == 7 ? 1.6f :
                             E.Type == 1 ? 1.4f : 1.08f;
            }
            continue;
        }
        if (E.Cooldown > 0.f || Distance > 8.3f || !bSeesTarget) continue;

        const bool bRangedAttack = (Spec.Ranged && Distance >= Reach) ||
                                   (bMeleeBoss && Distance >= 2.2f) ||
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
            Bolt.Velocity = Direction * (Spec.Boss ? 7.8f : E.Type == 10 ? 9.f : 6.8f);
            Bolt.Damage = FMath::RoundToInt(Spec.Damage * FAVContent::LevelDamageMultiplier(E.Level) *
                (E.bElite ? 1.35f : 1.f) * (E.Equipment == 3 ? 1.25f : 1.f));
            Bolt.Status = (E.Type == 6 ? 1 : E.Type == 9 ? 2 : 0);
            if (bTargetEnemy)
            {
                for (int32 J = Enemies.Num() - 1; J >= 0; --J)
                {
                    if (J == I || Enemies[J].Faction == E.Faction ||
                        FVector2D::Distance(Enemies[J].Pos, Target) > 0.6f) continue;
                    Enemies[J].HP -= Bolt.Damage;
                    break;
                }
                E.Cooldown = 1.5f;
                continue;
            }
            Bolts.Add(Bolt);

            if (Spec.Boss || E.bMiniBoss)
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
                E.Cooldown = E.bMiniBoss ? (0.85f + (E.Type - 26) * 0.13f) :
                    bFurious ? (E.Type == 9 ? 0.85f : 0.96f) : 1.35f;
            }
            else E.Cooldown = E.Type == 10 ? 1.40f : E.Type == 6 ? 1.75f : 2.1f;
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
                const float T = LenSq > KINDA_SMALL_NUMBER ?
                    FMath::Clamp(FVector2D::DotProduct(E.Pos - Old, Segment) / LenSq, 0.f, 1.f) : 0.f;
                const float Radius = FAVContent::Enemy(E.Type, Floor).Boss ? 0.76f : 0.47f;
                if (FVector2D::Distance(Old + Segment * T, E.Pos) > Radius) continue;
                E.HP -= Bolt.Damage;
                E.Flash = 0.16f;
                E.Knockback += SafeDirection(E.Pos - Old) * 3.1f;
                AddEffect(E.Pos, 2, 0.4f);
                if (E.HP <= 0.f) KillEnemy(J);
                bHit = true;
                break;
            }
            if (bHit) Bolts.RemoveAtSwap(I);
        }
        else
        {
            const float T = LenSq > KINDA_SMALL_NUMBER ?
                FMath::Clamp(FVector2D::DotProduct(Hero - Old, Segment) / LenSq, 0.f, 1.f) : 0.f;
            if (FVector2D::Distance(Old + Segment * T, Hero) < 0.43f)
            {
                const bool bVulnerable = Invulnerability <= 0.f;
                const int32 Status = Bolt.Status;
                DamageHero(Bolt.Damage, Bolt.Pos);
                if (bVulnerable && Phase != EAVPhase::Dead)
                {
                    if (Status == 1) PoisonTime = FMath::Max(PoisonTime, 4.2f);
                    if (Status == 2) BurnTime = FMath::Max(BurnTime, 3.3f);
                }
                Bolts.RemoveAtSwap(I);
            }
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
                Trap.Cooldown = 2.8f;
                AddEffect(Trap.Pos, 2, 0.8f);
                if (FVector2D::Distance(Trap.Pos, Hero) < 0.88f)
                    DamageHero(14.f + Floor * 3.f, Trap.Pos);
            }
        }
        else if (Trap.Cooldown <= 0.f &&
                 FVector2D::Distance(Trap.Pos, Hero) < 0.78f)
            Trap.Windup = 0.62f;
    }

    // Contact areas apply a temporary status, but not during a dodge iframe.
    for (FAVHazard& Hazard : Hazards)
    {
        Hazard.Cooldown = FMath::Max(0.f, Hazard.Cooldown - DeltaSeconds);
        if (Hazard.Cooldown > 0.f ||
            FVector2D::Distance(Hero, Hazard.Pos) > 1.f ||
            Invulnerability > 0.f) continue;
        Hazard.Cooldown = 1.25f;
        if (Hazard.Type == 0) PoisonTime = FMath::Max(PoisonTime, 4.0f);
        else BurnTime = FMath::Max(BurnTime, 3.f);
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
        Stamina = FMath::Min(MaxStamina, Stamina + 14.5f * DT);
        ComboTime = FMath::Max(0.f, ComboTime - DT);
        if (ComboTime <= 0.f) Combo = 0;
        ToastTime = FMath::Max(0.f, ToastTime - DT);
        RefreshAwareness(DT);
        UpdateCombat(DT);
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
