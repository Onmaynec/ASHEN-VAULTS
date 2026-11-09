#include "AVDepthDirector.h"
#include "AVDungeonNavigation.h"
#include <cassert>
#include <cmath>
#include <iostream>

namespace
{
    const int32 W=52,H=38;
    void MakeRoom(TArray<uint8>& Tiles,const FAVRoomBounds& R)
    {
        for(int32 Y=R.Y;Y<R.Y+R.H;++Y)
            for(int32 X=R.X;X<R.X+R.W;++X)
                Tiles[Y*W+X]=1;
    }
    FAVRoomBounds Rect(int32 X,int32 Y,int32 WW,int32 HH)
    {
        FAVRoomBounds R;R.X=X;R.Y=Y;R.W=WW;R.H=HH;return R;
    }
    bool AllConnected(const TArray<uint8>& Tiles,const FVector2D& Start)
    {
        TArray<int32> Dist;
        FAVDungeonNavigation::BuildFlow(Tiles,W,H,Start,Dist);
        for(int32 Index=0;Index<Tiles.Num();++Index)
            if(Tiles[Index]!=0&&Dist[Index]<0)return false;
        return true;
    }
}
int main()
{
    const FAVRoomBounds Entry=Rect(4,4,11,10);
    const FAVRoomBounds Fight=Rect(22,8,13,12);
    const FAVRoomBounds Rear=Rect(34,23,12,10);
    const FVector2D Hero=Entry.Center();
    int32 Geometries[8]={},Formations[8]={},Waves[5]={};
    for(int32 Seed=1;Seed<=400;++Seed)
    {
        const int32 Floor=1+(Seed%120);
        const int32 Biome=(Floor-1)/5%4;
        const auto G=FAVDepthDirector::Geometry(Seed,Floor,2,Biome);
        const auto F=FAVDepthDirector::Formation(Seed,Floor,2,Seed%4);
        ++Geometries[static_cast<int32>(G)];
        ++Formations[static_cast<int32>(F)];
        const FAVWaveDirective Wave=FAVDepthDirector::Wave(
            Seed,Floor,3,Fight.W*Fight.H,Seed%4);
        ++Waves[static_cast<int32>(Wave.Trigger)];
        assert(Wave.UnitCount>=0&&Wave.UnitCount<=5);
        assert(Wave.Kind==0||Wave.Kind==Seed%4);
        if(Wave.Trigger!=EAVWaveTrigger::None)
        {
            assert(Wave.UnitCount>=2);
            assert(Wave.WarningSeconds>=0.9f);
            assert(Wave.WarningSeconds<=1.51f);
        }
        // Verify identical seeds yield identical room prescriptions.
        assert(G==FAVDepthDirector::Geometry(Seed,Floor,2,Biome));
        assert(F==FAVDepthDirector::Formation(Seed,Floor,2,Seed%4));
        assert(Wave.Trigger==FAVDepthDirector::Wave(
            Seed,Floor,3,Fight.W*Fight.H,Seed%4).Trigger);

        TArray<uint8> Tiles;Tiles.Init(0,W*H);
        MakeRoom(Tiles,Entry);MakeRoom(Tiles,Fight);MakeRoom(Tiles,Rear);
        FAVDepthDirector::ApplyGeometry(Tiles,W,H,Fight,G);
        FAVDepthDirector::ApplyGeometry(Tiles,W,H,Rear,
            static_cast<EAVRoomGeometry>((Seed+3)%8));
        FAVDepthDirector::CarveAlternativeRoute(Tiles,W,H,Entry,Fight,true);
        FAVDepthDirector::CarveAlternativeRoute(Tiles,W,H,Fight,Rear,false);
        const int32 Repairs=FAVDepthDirector::RepairConnectivity(Tiles,W,H,Hero);
        assert(Repairs>=0&&Repairs<=32);
        assert(AllConnected(Tiles,Hero));
        assert(FAVDepthDirector::ClearCell(Tiles,W,H,Hero));
        assert(FAVDepthDirector::ClearCell(Tiles,W,H,Fight.Center()));

        // Eight formations must spawn without overlapping monsters, blocking
        // walls, invalid footprints or colliding with hero/shrine/exit.
        TArray<FVector2D> Occupied;
        const FVector2D Exit(9.5f,9.5f),Shrine(7.5f,9.5f);
        for(int32 I=0;I<8;++I)
        {
            FVector2D Pos;
            const bool Placed=FAVDepthDirector::FindSpawn(
                Tiles,W,H,Fight,static_cast<EAVFormation>((Seed+I)%8),
                I,8,Hero,Exit,Shrine,Occupied,Pos,4.5f);
            assert(Placed);
            assert(FAVDepthDirector::InRoom(Fight,Pos));
            assert(FAVDepthDirector::ClearCell(Tiles,W,H,Pos));
            for(const FVector2D& Previous:Occupied)
                assert((Pos-Previous).SizeSquared()>=0.809f);
            Occupied.Add(Pos);
        }
        assert(Occupied.Num()==8);
    }
    for(int32 I=0;I<8;++I)
    {
        assert(Geometries[I]>0);
        assert(Formations[I]>0);
    }
    for(int32 I=0;I<5;++I)assert(Waves[I]>0);

    // Four events are armed only by their own trigger and in-room context.
    for(int32 Trigger=1;Trigger<=4;++Trigger)
    {
        const EAVWaveTrigger Type=static_cast<EAVWaveTrigger>(Trigger);
        for(int32 Mask=0;Mask<16;++Mask)
        {
            const bool Inside=(Mask&1)!=0;
            const bool Living=(Mask&2)!=0;
            const bool Opened=(Mask&4)!=0;
            const bool Killed=(Mask&8)!=0;
            const bool Should=Inside && (
                (Trigger==1) || (Trigger==2&&!Living) ||
                (Trigger==3&&Opened) || (Trigger==4&&Killed));
            assert(FAVDepthDirector::ShouldArmWave(
                Type,Inside,Living,Opened,Killed)==Should);
        }
    }
    assert(!FAVDepthDirector::ShouldArmWave(
        EAVWaveTrigger::None,true,false,true,true));
    float Warning=1.2f;
    for(int32 I=0;I<11;++I)
        assert(!FAVDepthDirector::AdvanceWarning(Warning,0.1f));
    assert(FAVDepthDirector::AdvanceWarning(Warning,0.1f));
    assert(Warning==0.f);
    assert(FAVDepthDirector::AdvanceWarning(Warning,0.f));
    float Unmodified=0.9f;
    assert(!FAVDepthDirector::AdvanceWarning(Unmodified,-4.f));
    assert(Unmodified==0.9f);

    // Disconnected islands are repaired without opening exterior bounds.
    TArray<uint8> Disconnected;Disconnected.Init(0,W*H);
    MakeRoom(Disconnected,Entry);MakeRoom(Disconnected,Rear);
    assert(!AllConnected(Disconnected,Hero));
    assert(FAVDepthDirector::RepairConnectivity(
        Disconnected,W,H,Hero,32)>0);
    assert(AllConnected(Disconnected,Hero));
    for(int32 X=0;X<W;++X)
    {
        assert(Disconnected[X]==0);
        assert(Disconnected[(H-1)*W+X]==0);
    }
    for(int32 Y=0;Y<H;++Y)
    {
        assert(Disconnected[Y*W]==0);
        assert(Disconnected[Y*W+W-1]==0);
    }
    // A room too small for an event cannot schedule reinforcements.
    assert(FAVDepthDirector::Wave(1,10,2,20,3).Trigger==
           EAVWaveTrigger::None);
    std::cout<<"PASS: 400 procedural layouts, 8 wall patterns, "
                "8 formations, four event triggers, 3200 safe spawns "
                "and connectivity repair\n";
}
