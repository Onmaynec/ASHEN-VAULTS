#include "AVGameMode.h"
#include "AVHUD.h"
#include "AVSaveGame.h"
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
    HP = 100.f; MaxHP = 100.f; Stamina = 100.f; MaxStamina = 100.f;
    AttackPower = 18.f; MoveSpeed = 4.8f; RunTime = 0.f;
    AttackCooldown = 0.f; SpinCooldown = 0.f; DashTime = 0.f;
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
    Tiles.Init(0, MapW * MapH);
    Enemies.Reset(); Pickups.Reset(); Bolts.Reset(); Effects.Reset(); Traps.Reset();
    bShrineUsed = false;
    struct FRoom { int32 X, Y, W, H; FVector2D Center() const { return FVector2D(X + W / 2.f, Y + H / 2.f); } };
    TArray<FRoom> Rooms;
    const int32 TargetCount = 9 + Floor * 2;
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
        return;
    }
    Hero = Rooms[0].Center();
    Exit = Rooms.Last().Center();
    Shrine = Rooms[Rooms.Num() / 2].Center();
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
                FVector2D::Distance(Candidate, Shrine) < 2.2f)
                continue;
            FAVTrap Trap;
            Trap.Pos = Candidate;
            Traps.Add(Trap);
            break;
        }
    }

    int32 NextId = 1;
    for (int32 I = 1; I < Rooms.Num(); ++I)
    {
        const FRoom& R = Rooms[I];
        const int32 Count = RNG.RandRange(1, FMath::Min(4, 2 + Floor));
        for (int32 E = 0; E < Count; ++E)
        {
            FAVEnemy Enemy;
            Enemy.Pos = FVector2D(RNG.FRandRange(R.X + 1.f, R.X + R.W - 1.f),
                                  RNG.FRandRange(R.Y + 1.f, R.Y + R.H - 1.f));
            Enemy.Type = RNG.RandRange(0, 2);
            Enemy.MaxHP = (Enemy.Type == 1 ? 46.f : (Enemy.Type == 2 ? 22.f : 29.f)) * (1.f + 0.28f * (Floor - 1));
            Enemy.bElite = RNG.FRand() < (0.12f + 0.03f * Floor);
            if (Enemy.bElite) Enemy.MaxHP *= 1.65f;
            Enemy.HP = Enemy.MaxHP;
            Enemy.Cooldown = RNG.FRandRange(0.3f, 1.6f);
            Enemy.Id = NextId++;
            Enemies.Add(Enemy);
        }
    }
    if (Floor == 3)
    {
        FAVEnemy Boss;
        Boss.Pos = Exit; Boss.Type = 3; Boss.HP = 360.f; Boss.MaxHP = 360.f;
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
}

void AAVGameMode::AddEffect(const FVector2D& Pos, int32 Kind, float Radius, float Angle)
{
    FAVEffect Effect;
    Effect.Pos = Pos; Effect.Kind = Kind; Effect.Radius = Radius; Effect.Angle = Angle;
    Effect.Duration = Kind == 3 ? 0.65f : 0.24f;
    Effect.Life = Effect.Duration;
    Effects.Add(Effect);
}

void AAVGameMode::Strike(bool bSpin)
{
    const float Cost = bSpin ? 32.f : 0.f;
    if (Stamina < Cost || (bSpin ? SpinCooldown > 0.f : AttackCooldown > 0.f)) return;
    Stamina -= Cost;
    if (bSpin) SpinCooldown = 1.65f;
    else AttackCooldown = 0.33f;
    AddEffect(Hero, bSpin ? 1 : 0, bSpin ? 2.2f : 1.75f, FMath::Atan2(Aim.Y, Aim.X));
    const float Range = bSpin ? 2.25f : 1.92f;
    const float Damage = AttackPower * (bSpin ? 1.65f : 1.f);
    for (int32 I = Enemies.Num() - 1; I >= 0; --I)
    {
        FAVEnemy& Enemy = Enemies[I];
        FVector2D Delta = Enemy.Pos - Hero;
        const float Distance = Delta.Size();
        if (Distance > Range + (Enemy.Type == 3 ? 0.3f : 0.f)) continue;
        if (!bSpin && FVector2D::DotProduct(SafeDirection(Delta), Aim) < 0.10f) continue;
        Enemy.HP -= Damage;
        Enemy.Flash = 0.16f;
        Enemy.Knockback += SafeDirection(Delta) * (Enemy.Type == 3 ? 1.f : 5.f);
        AddEffect(Enemy.Pos, 2, 0.55f);
        if (Enemy.HP <= 0.f) KillEnemy(I);
    }
}

void AAVGameMode::DamageHero(float Damage, const FVector2D& Source)
{
    if (Invulnerability > 0.f || Phase != EAVPhase::Playing) return;
    HP = FMath::Max(0.f, HP - Damage);
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
        Pickup.Amount = bBoss ? RNG.RandRange(8, 15) : RNG.RandRange(1, 5);
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
    const bool bBoss = Dead.Type == 3;
    Enemies.RemoveAtSwap(Index);
    ++Kills;
    Combo = ComboTime > 0.f ? Combo + 1 : 1;
    ComboTime = 4.5f;
    if (Combo % 5 == 0)
    {
        Gold += 5;
        Toast = TEXT("COMBO BONUS  +5 GOLD");
        ToastTime = 1.5f;
    }
    GrantExperience(bBoss ? 35 : (3 + Floor) * (Dead.bElite ? 2 : 1));
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
    const bool bReplay = Pressed(PC, EKeys::R, PrevReplay);
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
    if (bDash && Stamina >= 23.f && DashTime <= 0.f)
    {
        Stamina -= 23.f;
        DashDirection = Direction.IsNearlyZero() ? Aim : Direction;
        DashTime = 0.17f;
        Invulnerability = FMath::Max(Invulnerability, 0.25f);
        AddEffect(Hero, 1, 0.7f);
    }
    if (DashTime > 0.f)
        Hero = MoveWithCollision(Hero, DashDirection * 16.f, DeltaSeconds, 0.27f);
    else
        Hero = MoveWithCollision(Hero, Direction * MoveSpeed, DeltaSeconds, 0.27f);

    if (bHit) Strike(false);
    if (bSpin) Strike(true);
    if (bHeal && Potions > 0 && HP < MaxHP)
    {
        --Potions; HP = FMath::Min(MaxHP, HP + 42.f);
        AddEffect(Hero, 2, 1.f);
    }
    if (bUse && !bShrineUsed && FVector2D::Distance(Hero, Shrine) < 1.55f)
    {
        bShrineUsed = true;
        HP = FMath::Min(MaxHP, HP + 35.f);
        AttackPower += 2.f;
        Toast = TEXT("SHRINE BLESSING  +35 HP  +2 DAMAGE");
        ToastTime = 2.6f;
        AddEffect(Shrine, 1, 1.6f);
    }
    else if (bUse && IsExitReady() && FVector2D::Distance(Hero, Exit) < 1.7f)
    {
        if (Floor >= 3) { Phase = EAVPhase::Victory; RecordRun(); }
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
    // Wind-up attacks are deliberately avoidable: a dash or a step out of range
    // during the warning prevents the damage.
    for (int32 I = Enemies.Num() - 1; I >= 0; --I)
    {
        FAVEnemy& E = Enemies[I];
        E.Cooldown -= DeltaSeconds;
        E.Flash = FMath::Max(0.f, E.Flash - DeltaSeconds);
        const FVector2D ToHero = Hero - E.Pos;
        const float Distance = ToHero.Size();
        const FVector2D Direction = SafeDirection(ToHero);
        const float Speed = (E.Type == 0 ? 2.45f : E.Type == 1 ? 1.65f :
                             E.Type == 2 ? 2.05f : 1.8f) * (E.bElite ? 1.18f : 1.f);
        FVector2D DesiredVelocity = FVector2D::ZeroVector;
        if (Distance < 13.f && E.Telegraph <= 0.f)
        {
            if (E.Type == 2 && Distance < 4.f)
                DesiredVelocity = -Direction * Speed;
            else if (Distance > (E.Type == 2 ? 6.f : E.Type == 3 ? 1.3f : 0.9f))
                DesiredVelocity = Direction * Speed;
        }
        E.Pos = MoveWithCollision(E.Pos, DesiredVelocity + E.Knockback,
                                  DeltaSeconds, E.Type == 3 ? 0.48f : 0.26f);
        E.Knockback = FMath::Lerp(E.Knockback, FVector2D::ZeroVector,
                                  FMath::Clamp(DeltaSeconds * 11.f, 0.f, 1.f));

        if (E.Telegraph > 0.f)
        {
            E.Telegraph -= DeltaSeconds;
            if (E.Telegraph <= 0.f)
            {
                E.Telegraph = 0.f;
                if (FVector2D::Distance(E.Pos, Hero) < (E.Type == 3 ? 1.5f : 1.20f))
                    DamageHero((E.Type == 1 ? 18.f : E.Type == 3 ? 21.f :
                                10.f + Floor * 2) * (E.bElite ? 1.35f : 1.f), E.Pos);
                E.Cooldown = E.Type == 3 ? 1.2f : E.Type == 1 ? 1.4f : 1.1f;
            }
            continue;
        }
        if (E.Cooldown > 0.f || Distance > 11.f) continue;

        if (E.Type == 2 || (E.Type == 3 && Distance >= 1.5f))
        {
            if (Distance > 1.1f)
            {
                FAVBolt Bolt;
                Bolt.Pos = E.Pos;
                Bolt.Velocity = Direction * (E.Type == 3 ? 7.8f : 6.8f);
                Bolt.Damage = FMath::RoundToInt((E.Type == 3 ? 19.f :
                    12.f + Floor * 2.f) * (E.bElite ? 1.35f : 1.f));
                Bolts.Add(Bolt);
                if (E.Type == 3)
                {
                    const bool bEnraged = E.HP < E.MaxHP * 0.5f;
                    const FVector2D Orthogonal(-Direction.Y, Direction.X);
                    for (int32 Sign = -1; Sign <= 1; Sign += 2)
                    {
                        FAVBolt Side = Bolt;
                        Side.Velocity = SafeDirection(Direction + Orthogonal * (0.30f * Sign)) * 7.3f;
                        Bolts.Add(Side);
                        if (bEnraged)
                        {
                            FAVBolt Outer = Bolt;
                            Outer.Velocity = SafeDirection(Direction + Orthogonal * (0.59f * Sign)) * 7.6f;
                            Bolts.Add(Outer);
                        }
                    }
                }
            }
            E.Cooldown = E.Type == 3 ? (E.HP < E.MaxHP * 0.5f ? 0.96f : 1.25f) : 2.1f;
        }
        else if (Distance < (E.Type == 3 ? 1.5f : 1.05f))
        {
            E.Telegraph = E.Type == 3 ? 0.47f : E.Type == 1 ? 0.44f : 0.31f;
        }
    }

    for (int32 I = Bolts.Num() - 1; I >= 0; --I)
    {
        FAVBolt& Bolt = Bolts[I];
        Bolt.Pos += Bolt.Velocity * DeltaSeconds;
        Bolt.Time -= DeltaSeconds;
        if (Bolt.Time <= 0.f || !CanMove(Bolt.Pos, 0.06f))
        {
            Bolts.RemoveAtSwap(I);
            continue;
        }
        if (FVector2D::Distance(Bolt.Pos, Hero) < 0.43f)
        {
            DamageHero(Bolt.Damage, Bolt.Pos);
            Bolts.RemoveAtSwap(I);
        }
    }

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
        {
            Trap.Windup = 0.62f;
        }
    }

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
            default: break;
            }
            Pickups.RemoveAtSwap(I);
        }
    }
}

void AAVGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const float DT = FMath::Min(DeltaSeconds, 0.05f);
    PollInput(DT);
    if (Phase != EAVPhase::Playing) return;
    RunTime += DT;
    AttackCooldown = FMath::Max(0.f, AttackCooldown - DT);
    SpinCooldown = FMath::Max(0.f, SpinCooldown - DT);
    DashTime = FMath::Max(0.f, DashTime - DT);
    Invulnerability = FMath::Max(0.f, Invulnerability - DT);
    ScreenShake = FMath::Max(0.f, ScreenShake - DT);
    Stamina = FMath::Min(MaxStamina, Stamina + 14.5f * DT);
    ComboTime = FMath::Max(0.f, ComboTime - DT);
    if (ComboTime == 0.f) Combo = 0;
    ToastTime = FMath::Max(0.f, ToastTime - DT);
    UpdateCombat(DT);
    for (int32 I = Effects.Num() - 1; I >= 0; --I)
    {
        Effects[I].Life -= DT;
        if (Effects[I].Life <= 0.f) Effects.RemoveAtSwap(I);
    }
}
