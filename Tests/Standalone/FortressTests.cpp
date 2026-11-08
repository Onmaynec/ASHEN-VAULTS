#include "AVFortressLayout.h"
#include <cassert>
#include <iostream>
#include <queue>
#include <set>
#include <vector>
int main()
{
    std::set<unsigned long long> Fingerprints;
    for(int Seed=0;Seed<300;++Seed)
    {
        TArray<uint8> Tiles;
        TArray<FAVFortressRoom> Rooms;
        FAVFortressLayout::Build(Seed*7919,Tiles,Rooms);
        assert(Tiles.Num()==FAVFortressLayout::Width*FAVFortressLayout::Height);
        assert(Rooms.Num()==FAVFortressLayout::RoomCount);
        unsigned long long Signature=1469598103934665603ull;
        std::vector<int> Seen(static_cast<size_t>(Tiles.Num()),0);
        std::queue<int> Q;
        const int W=FAVFortressLayout::Width,H=FAVFortressLayout::Height;
        const int SX=Rooms[0].X+Rooms[0].W/2;
        const int SY=Rooms[0].Y+Rooms[0].H/2;
        const int Start=SY*W+SX;
        assert(Tiles[Start]==1);
        Q.push(Start);Seen[Start]=1;
        while(!Q.empty())
        {
            const int N=Q.front();Q.pop();
            const int X=N%W,Y=N/W;
            const int DX[4]={-1,1,0,0},DY[4]={0,0,-1,1};
            for(int I=0;I<4;++I) {
                const int XX=X+DX[I],YY=Y+DY[I];
                if(XX<0||XX>=W||YY<0||YY>=H)continue;
                const int Id=YY*W+XX;
                if(Tiles[Id]==1&&!Seen[Id]){Seen[Id]=1;Q.push(Id);}
            }
        }
        for(int I=0;I<Tiles.Num();++I) {
            Signature^=static_cast<unsigned long long>(Tiles[I]+I%7);
            Signature*=1099511628211ull;
            if(Tiles[I]==1) assert(Seen[I]==1);
        }
        for(const FAVFortressRoom& R:Rooms)
            assert(Seen[(R.Y+R.H/2)*W+R.X+R.W/2]);
        Fingerprints.insert(Signature);
        TArray<uint8> Copy;
        TArray<FAVFortressRoom> CopyRooms;
        FAVFortressLayout::Build(Seed*7919,Copy,CopyRooms);
        for(int I=0;I<Tiles.Num();++I) assert(Tiles[I]==Copy[I]);
    }
    assert(Fingerprints.size()>250u);
    std::cout<<"PASS: 300 Bastion seeds, all open cells connected, deterministic layouts\n";
}
