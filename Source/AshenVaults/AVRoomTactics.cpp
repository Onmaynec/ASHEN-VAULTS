#include "AVRoomTactics.h"

namespace
{
    uint32 Mix(int32 Seed,int32 Floor,int32 Room,int32 Biome,uint32 Salt)
    {
        return FAVDepthDirector::Hash(static_cast<uint32>(Seed)^Salt^
            static_cast<uint32>(Floor)*0x9e3779b9u^
            static_cast<uint32>(Room)*0x85ebca6bu^
            static_cast<uint32>(Biome)*0xc2b2ae35u);
    }
}
FAVRoomTactic FAVRoomTactics::Select(int32 Seed,int32 Floor,int32 Room,int32 Biome)
{
    FAVRoomTactic Result;
    Result.Shape=static_cast<EAVRoomShapeRule>(
        Mix(Seed,Floor,Room,Biome,0x1129a91bu)%ShapeCount);
    Result.Garrison=static_cast<EAVGarrisonRule>(
        Mix(Seed,Floor,Room,Biome,0x9dbe6713u)%GarrisonCount);
    Result.Response=static_cast<EAVResponseRule>(
        Mix(Seed,Floor,Room,Biome,0xf4a51781u)%ResponseCount);
    return Result;
}
void FAVRoomTactics::ApplyShape(TArray<uint8>& Tiles,int32 Width,int32 Height,
                                const FAVRoomBounds& R,EAVRoomShapeRule Rule)
{
    if(Tiles.Num()!=Width*Height||R.W<8||R.H<7)return;
    const int32 CX=R.X+R.W/2,CY=R.Y+R.H/2;
    auto Set=[&](int32 X,int32 Y,uint8 V)
    {
        // Never overwrite room rim/corridor entrances, map border or the
        // three-cell-wide center crossing used by interactions and traversal.
        if(X<=R.X+1||X>=R.X+R.W-2||
           Y<=R.Y+1||Y>=R.Y+R.H-2||
           X<2||Y<2||X>=Width-2||Y>=Height-2)return;
        if(FMath::Abs(X-CX)<=1||FMath::Abs(Y-CY)<=1)return;
        Tiles[Y*Width+X]=V;
    };
    switch(Rule)
    {
    case EAVRoomShapeRule::ClearLane:
        for(int32 X=R.X+2;X<R.X+R.W-2;++X)
            for(int32 Y=CY-1;Y<=CY+1;++Y)Tiles[Y*Width+X]=1;
        break;
    case EAVRoomShapeRule::CrossCover:
        for(int32 SX : {-2,2})for(int32 SY : {-2,2})Set(CX+SX,CY+SY,0);
        break;
    case EAVRoomShapeRule::BrokenRing:
        for(int32 X=CX-3;X<=CX+3;++X)
            {Set(X,CY-3,0);Set(X,CY+3,0);}
        break;
    case EAVRoomShapeRule::TwinIslands:
        for(int32 X=CX-3;X<=CX-2;++X)
            {Set(X,CY-2,0);Set(X,CY+2,0);}
        for(int32 X=CX+2;X<=CX+3;++X)
            {Set(X,CY-2,0);Set(X,CY+2,0);}
        break;
    case EAVRoomShapeRule::FlankDoors:
        for(int32 Y=R.Y+2;Y<R.Y+R.H-2;++Y)
            {Set(CX-3,Y,0);Set(CX+3,Y,0);}
        break;
    case EAVRoomShapeRule::BarricadeLine:
        for(int32 X=R.X+2;X<R.X+R.W-2;++X)
            if((X-CX)%3!=0)Set(X,CY-3,0);
        break;
    case EAVRoomShapeRule::NarrowThroat:
        for(int32 X=R.X+2;X<R.X+R.W-2;++X)
            {Set(X,CY-2,0);Set(X,CY+2,0);}
        break;
    case EAVRoomShapeRule::OpenPocket:
        for(int32 Y=R.Y+2;Y<R.Y+R.H-2;++Y)
            for(int32 X=R.X+2;X<R.X+R.W-2;++X)
                if((X<CX-1&&Y<CY-1)||(X>CX+1&&Y>CY+1))
                    Set(X,Y,1);
        break;
    }
}
FAVEncounterPlan FAVRoomTactics::Garrison(FAVEncounterPlan Plan,
                                          EAVGarrisonRule Rule,int32 Area,int32 Floor)
{
    switch(Rule)
    {
    case EAVGarrisonRule::Balanced:break;
    case EAVGarrisonRule::Vanguard:
        Plan.EnemyCount+=1;Plan.EliteChance+=0.04f;break;
    case EAVGarrisonRule::RearGuard:
        Plan.EnemyCount-=1;Plan.EquipmentChance+=0.08f;break;
    case EAVGarrisonRule::Crossfire:
        Plan.EnemyCount+=1;Plan.Kind=EAVEncounterKind::HunterPatrol;break;
    case EAVGarrisonRule::PackedSwarm:
        Plan.EnemyCount+=2;Plan.Kind=EAVEncounterKind::Swarm;
        Plan.EliteChance*=0.5f;break;
    case EAVGarrisonRule::VeteranPair:
        Plan.EnemyCount=2+(Floor>=10?1:0);
        Plan.GuaranteedElites=1;Plan.EliteChance+=0.10f;break;
    case EAVGarrisonRule::ArmoredLine:
        Plan.EquipmentChance+=0.22f;Plan.EnemyCount-=1;break;
    case EAVGarrisonRule::Skirmishers:
        Plan.Kind=EAVEncounterKind::HunterPatrol;
        Plan.EnemyCount+=1;Plan.EliteChance*=0.7f;break;
    }
    Plan.EnemyCount=FMath::Clamp(Plan.EnemyCount,1,
        FMath::Clamp(FMath::Max(0,Area)/5,3,8));
    Plan.GuaranteedElites=FMath::Min(Plan.GuaranteedElites,Plan.EnemyCount);
    Plan.EliteChance=FMath::Clamp(Plan.EliteChance,0.f,0.65f);
    Plan.EquipmentChance=FMath::Clamp(Plan.EquipmentChance,0.f,0.65f);
    return Plan;
}
EAVFormation FAVRoomTactics::Formation(EAVGarrisonRule Rule,EAVFormation Original)
{
    switch(Rule)
    {
    case EAVGarrisonRule::Vanguard:return EAVFormation::FrontLine;
    case EAVGarrisonRule::RearGuard:return EAVFormation::RearGuard;
    case EAVGarrisonRule::Crossfire:return EAVFormation::Pincer;
    case EAVGarrisonRule::PackedSwarm:return EAVFormation::Pack;
    case EAVGarrisonRule::VeteranPair:return EAVFormation::Wedge;
    case EAVGarrisonRule::ArmoredLine:return EAVFormation::FrontLine;
    case EAVGarrisonRule::Skirmishers:return EAVFormation::Staggered;
    default:return Original;
    }
}
EAVEncounterKind FAVRoomTactics::EnemyRole(EAVGarrisonRule Rule,EAVEncounterKind Original)
{
    switch(Rule)
    {
    case EAVGarrisonRule::Crossfire:
    case EAVGarrisonRule::Skirmishers:
    case EAVGarrisonRule::RearGuard:return EAVEncounterKind::HunterPatrol;
    case EAVGarrisonRule::PackedSwarm:
    case EAVGarrisonRule::Vanguard:return EAVEncounterKind::Swarm;
    case EAVGarrisonRule::VeteranPair:
    case EAVGarrisonRule::ArmoredLine:return EAVEncounterKind::EliteGuard;
    default:return Original;
    }
}
FAVWaveDirective FAVRoomTactics::Response(FAVWaveDirective Wave,
                                           EAVResponseRule Rule,int32 Area,int32 Floor)
{
    // No events in cramped rooms. Every active response remains bounded and
    // uses the same warning/trigger serialization as older saves.
    if(Area<36)return FAVWaveDirective();
    switch(Rule)
    {
    case EAVResponseRule::Quiet:return FAVWaveDirective();
    case EAVResponseRule::EntryAlarm:
        Wave.Trigger=EAVWaveTrigger::OnEntry;break;
    case EAVResponseRule::ClearRetaliation:
        Wave.Trigger=EAVWaveTrigger::AfterClear;break;
    case EAVResponseRule::TreasureAmbush:
        Wave.Trigger=EAVWaveTrigger::OnChestOpen;break;
    case EAVResponseRule::FirstBlood:
        Wave.Trigger=EAVWaveTrigger::OnFirstKill;break;
    case EAVResponseRule::DelayedAlarm:
        Wave.Trigger=EAVWaveTrigger::OnEntry;
        Wave.WarningSeconds=2.2f;Wave.UnitCount+=1;break;
    case EAVResponseRule::EliteCounterattack:
        Wave.Trigger=EAVWaveTrigger::AfterClear;Wave.Kind=3;
        Wave.UnitCount=2;Wave.WarningSeconds=1.5f;break;
    case EAVResponseRule::DoubleFlank:
        Wave.Trigger=EAVWaveTrigger::OnFirstKill;
        Wave.Formation=EAVFormation::Pincer;
        Wave.UnitCount+=2;Wave.WarningSeconds=1.8f;break;
    }
    Wave.UnitCount=FMath::Clamp(Wave.UnitCount>0?Wave.UnitCount:2,
        2,FMath::Clamp(Area/14,2,5));
    Wave.WarningSeconds=FMath::Clamp(Wave.WarningSeconds,0.9f,3.f);
    (void)Floor;
    return Wave;
}
const char* FAVRoomTactics::ShapeName(EAVRoomShapeRule R)
{
    static const char* Names[]={"CLEAR LANE","CROSS COVER","BROKEN RING",
        "TWIN ISLANDS","FLANK DOORS","BARRICADE LINE","NARROW THROAT","OPEN POCKET"};
    const int32 I=static_cast<int32>(R);
    return I>=0&&I<ShapeCount?Names[I]:"INVALID";
}
const char* FAVRoomTactics::GarrisonName(EAVGarrisonRule R)
{
    static const char* Names[]={"BALANCED","VANGUARD","REAR GUARD",
        "CROSSFIRE","PACKED SWARM","VETERAN PAIR","ARMORED LINE","SKIRMISHERS"};
    const int32 I=static_cast<int32>(R);
    return I>=0&&I<GarrisonCount?Names[I]:"INVALID";
}
const char* FAVRoomTactics::ResponseName(EAVResponseRule R)
{
    static const char* Names[]={"QUIET","ENTRY ALARM","CLEAR RETALIATION",
        "TREASURE AMBUSH","FIRST BLOOD","DELAYED ALARM",
        "ELITE COUNTERATTACK","DOUBLE FLANK"};
    const int32 I=static_cast<int32>(R);
    return I>=0&&I<ResponseCount?Names[I]:"INVALID";
}
