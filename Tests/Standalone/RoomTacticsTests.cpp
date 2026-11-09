#include "AVRoomTactics.h"
#include "AVContent.h"
#include "AVDungeonNavigation.h"
#include <cassert>
#include <iostream>

int main()
{
    int32 Shapes[8]={},Garrisons[8]={},Responses[8]={};
    int32 Trials=0,ChangedShapes=0,ResponseEvents=0;
    for(int32 Seed=1;Seed<=40;++Seed)
        for(int32 Floor : {1,5,9,14,19,40})
            for(int32 Room=1;Room<=32;++Room)
            {
                const FAVRoomTactic T=FAVRoomTactics::Select(
                    Seed,Floor,Room,(Floor-1)/5%4);
                const FAVRoomTactic Again=FAVRoomTactics::Select(
                    Seed,Floor,Room,(Floor-1)/5%4);
                assert(T.Shape==Again.Shape&&T.Garrison==Again.Garrison&&
                       T.Response==Again.Response);
                const int32 A=static_cast<int32>(T.Shape);
                const int32 B=static_cast<int32>(T.Garrison);
                const int32 C=static_cast<int32>(T.Response);
                assert(A>=0&&A<8&&B>=0&&B<8&&C>=0&&C<8);
                ++Shapes[A];++Garrisons[B];++Responses[C];
                assert(FAVRoomTactics::ShapeName(T.Shape)[0]!='I');
                assert(FAVRoomTactics::GarrisonName(T.Garrison)[0]!='I');
                assert(FAVRoomTactics::ResponseName(T.Response)[0]!='I');

                const FAVEncounterPlan Base=FAVEncounters::Plan(Floor,90,30,70);
                const FAVEncounterPlan G=FAVRoomTactics::Garrison(
                    Base,T.Garrison,90,Floor);
                assert(G.EnemyCount>=1&&G.EnemyCount<=8);
                assert(G.GuaranteedElites<=G.EnemyCount);
                assert(G.EliteChance>=0&&G.EliteChance<=1);
                assert(G.EquipmentChance>=0&&G.EquipmentChance<=1);
                const auto Form=FAVRoomTactics::Formation(
                    T.Garrison,EAVFormation::Perimeter);
                assert(static_cast<int32>(Form)<8);
                const auto Role=FAVRoomTactics::EnemyRole(T.Garrison,G.Kind);
                assert(static_cast<int32>(Role)<4);

                FAVWaveDirective Wave=FAVDepthDirector::Wave(
                    Seed,Floor,Room,90,static_cast<int32>(G.Kind));
                Wave=FAVRoomTactics::Response(Wave,T.Response,90,Floor);
                if(T.Response==EAVResponseRule::Quiet)
                    assert(Wave.Trigger==EAVWaveTrigger::None);
                else
                {
                    assert(Wave.Trigger!=EAVWaveTrigger::None);
                    assert(Wave.UnitCount>=2&&Wave.UnitCount<=5);
                    assert(Wave.WarningSeconds>=0.9f);
                    ++ResponseEvents;
                }

                TArray<uint8> Tiles;Tiles.Init(1,32*26);
                const FAVRoomBounds Bounds={7,5,16,14};
                FAVRoomTactics::ApplyShape(Tiles,32,26,Bounds,T.Shape);
                // Protected rim, center cross and all map edges remain open.
                for(int32 Y=0;Y<26;++Y)
                    for(int32 X=0;X<32;++X)
                    {
                        if(X==0||Y==0||X==31||Y==25||
                           X==Bounds.X||X==Bounds.X+Bounds.W-1||
                           Y==Bounds.Y||Y==Bounds.Y+Bounds.H-1||
                           (X>=Bounds.X&&X<Bounds.X+Bounds.W&&
                            Y>=Bounds.Y&&Y<Bounds.Y+Bounds.H&&
                            (FMath::Abs(X-15)<=1||FMath::Abs(Y-12)<=1)))
                            assert(Tiles[Y*32+X]==1);
                        if(Tiles[Y*32+X]==0)++ChangedShapes;
                    }
                // Walkable access to room center must survive every rule.
                TArray<int32> Distances;
                FAVDungeonNavigation::BuildFlow(Tiles,32,26,
                    FVector2D(8.5f,12.5f),Distances);
                assert(Distances[12*32+15]>=0);
                ++Trials;
            }
    for(int32 I=0;I<8;++I)
    {
        assert(Shapes[I]>0&&Garrisons[I]>0&&Responses[I]>0);
    }
    assert(ChangedShapes>0&&ResponseEvents>0);
    // Small rooms must not spawn reinforcements, regardless of response.
    for(int32 I=0;I<8;++I)
        assert(FAVRoomTactics::Response(FAVWaveDirective(),
            static_cast<EAVResponseRule>(I),20,1).Trigger==
            EAVWaveTrigger::None);
    std::cout<<"PASS: "<<Trials<<" room combinations; 24 rule types; "
             <<ResponseEvents<<" delayed responses\n";
    return 0;
}
