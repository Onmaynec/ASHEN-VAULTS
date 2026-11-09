#include "AVDepthDirector.h"
#include "AVDungeonNavigation.h"

uint32 FAVDepthDirector::Hash(uint32 V)
{
    V ^= V >> 16; V *= 0x7feb352du;
    V ^= V >> 15; V *= 0x846ca68bu;
    return V ^ (V >> 16);
}

EAVRoomGeometry FAVDepthDirector::Geometry(int32 Seed,int32 Floor,
                                            int32 RoomIndex,int32 Biome)
{
    const uint32 V=Hash(static_cast<uint32>(Seed) ^
        (static_cast<uint32>(Floor)*0x9e3779b9u) ^
        (static_cast<uint32>(RoomIndex)*0x85ebca6bu) ^
        (static_cast<uint32>(Biome)*0xc2b2ae35u));
    return static_cast<EAVRoomGeometry>(V%8u);
}

EAVFormation FAVDepthDirector::Formation(int32 Seed,int32 Floor,
                                         int32 RoomIndex,int32 Kind)
{
    const uint32 V=Hash(static_cast<uint32>(Seed)^
        (static_cast<uint32>(Floor)*0x51ed270bu)^
        (static_cast<uint32>(RoomIndex)*0x27d4eb2du)^
        (static_cast<uint32>(Kind)*0x165667b1u));
    // Ranged patrols keep a rear or cross-fire position, while swarms group
    // or surround; other profiles can use the full range of eight tactics.
    const int32 Base=static_cast<int32>(V%4u);
    if(Kind==1) return static_cast<EAVFormation>(
        Base==0?2:Base==1?3:Base==2?6:7);
    if(Kind==2) return static_cast<EAVFormation>(
        Base==0?4:Base==1?5:Base==2?0:3);
    if(Kind==3) return static_cast<EAVFormation>(
        Base==0?1:Base==1?2:Base==2?5:7);
    return static_cast<EAVFormation>(V%8u);
}

FAVWaveDirective FAVDepthDirector::Wave(int32 Seed,int32 Floor,
                                        int32 RoomIndex,int32 RoomArea,int32 Kind)
{
    const uint32 V=Hash(static_cast<uint32>(Seed)^
        (static_cast<uint32>(Floor)*0x68e31da4u)^
        (static_cast<uint32>(RoomIndex)*0xb5297a4du));
    FAVWaveDirective Result;
    const int32 Chance=static_cast<int32>(V%100u);
    if(RoomArea<36||Chance>=56) return Result;
    // Four triggers: proximity, victory response, treasure interception and
    // retaliation for the first casualty in this chamber.
    Result.Trigger=static_cast<EAVWaveTrigger>(1+Chance/14);
    Result.Formation=Formation(Seed^0x4c65b183,Floor,RoomIndex,Kind);
    Result.UnitCount=FMath::Clamp(2+static_cast<int32>((V>>13)%3u)
        +((Floor-1)%5>=3?1:0),2,5);
    Result.UnitCount=FMath::Min(Result.UnitCount,
        FMath::Clamp(RoomArea/14,2,5));
    Result.Kind=Kind;
    Result.WarningSeconds=0.9f+0.2f*static_cast<float>((V>>19)%4u);
    return Result;
}

bool FAVDepthDirector::ClearCell(const TArray<uint8>& Tiles,int32 Width,int32 Height,
                                  const FVector2D& P,float Radius)
{
    const float X=P.X,Y=P.Y;
    const float DX[4]={-Radius,Radius,-Radius,Radius};
    const float DY[4]={-Radius,-Radius,Radius,Radius};
    for(int32 K=0;K<4;++K)
    {
        if(!FAVDungeonNavigation::IsWalkable(Tiles,Width,Height,
           FMath::FloorToInt(X+DX[K]),FMath::FloorToInt(Y+DY[K])))
            return false;
    }
    return true;
}

void FAVDepthDirector::ApplyGeometry(TArray<uint8>& Tiles,int32 Width,int32 Height,
                                     const FAVRoomBounds& R,EAVRoomGeometry Type)
{
    if(R.W<7||R.H<6||Tiles.Num()!=Width*Height) return;
    auto Set=[&](int32 X,int32 Y,uint8 Tile)
    {
        if(X>=2&&Y>=2&&X<Width-2&&Y<Height-2)
            Tiles[Y*Width+X]=Tile;
    };
    const int32 CX=R.X+R.W/2,CY=R.Y+R.H/2;
    const int32 X1=R.X+R.W/3,X2=R.X+R.W*2/3;
    const int32 Y1=R.Y+R.H/3,Y2=R.Y+R.H*2/3;
    switch(Type)
    {
    case EAVRoomGeometry::OpenCourt:
        // Broad combat square with no artificial center blockage.
        for(int32 Y=CY-2;Y<=CY+2;++Y)
            for(int32 X=CX-2;X<=CX+2;++X) Set(X,Y,1);
        break;
    case EAVRoomGeometry::TwinPillars:
        Set(X1,Y1,0);Set(X2,Y2,0);break;
    case EAVRoomGeometry::FallenColumns:
        Set(X1,Y1,0);Set(X1+1,Y1,0);Set(X2,Y2,0);break;
    case EAVRoomGeometry::SplitHall:
        for(int32 X=R.X+2;X<R.X+R.W-2;++X)
            if(FMath::Abs(X-CX)>1)Set(X,CY,0);
        break;
    case EAVRoomGeometry::GuardPosts:
        Set(R.X+2,R.Y+2,0);Set(R.X+R.W-3,R.Y+2,0);
        Set(R.X+2,R.Y+R.H-3,0);Set(R.X+R.W-3,R.Y+R.H-3,0);
        break;
    case EAVRoomGeometry::ZigzagBaffles:
        for(int32 X=R.X+2;X<CX-1;++X)Set(X,CY-1,0);
        for(int32 X=CX+2;X<R.X+R.W-2;++X)Set(X,CY+1,0);
        break;
    case EAVRoomGeometry::SideAlcove:
        // Carve a genuine side pocket; never carve beyond map border.
        for(int32 X=R.X+R.W;X<R.X+R.W+3;++X)
            for(int32 Y=CY-1;Y<=CY+1;++Y) Set(X,Y,1);
        Set(R.X+R.W-1,CY,1);break;
    case EAVRoomGeometry::CentralIsland:
        Set(CX-1,CY-1,0);Set(CX-1,CY,0);
        Set(CX+1,CY-1,0);Set(CX+1,CY,0);
        break;
    }
    // Main interaction points and spawn center always have clear footing.
    Set(CX,CY,1);
}

void FAVDepthDirector::CarveAlternativeRoute(TArray<uint8>& Tiles,int32 Width,int32 Height,
                                               const FAVRoomBounds& A,const FAVRoomBounds& B,
                                               bool bHorizontalFirst)
{
    if(Tiles.Num()!=Width*Height)return;
    int32 X=FMath::FloorToInt(A.Center().X),Y=FMath::FloorToInt(A.Center().Y);
    const int32 TX=FMath::FloorToInt(B.Center().X);
    const int32 TY=FMath::FloorToInt(B.Center().Y);
    auto Carve=[&](int32 PX,int32 PY)
    {
        if(PX>=2&&PY>=2&&PX<Width-2&&PY<Height-2)Tiles[PY*Width+PX]=1;
    };
    // L-shaped shortcut adds a real alternative route between distant rooms.
    if(bHorizontalFirst)
    {
        while(X!=TX){Carve(X,Y);X+=X<TX?1:-1;}
        while(Y!=TY){Carve(X,Y);Y+=Y<TY?1:-1;}
    }
    else
    {
        while(Y!=TY){Carve(X,Y);Y+=Y<TY?1:-1;}
        while(X!=TX){Carve(X,Y);X+=X<TX?1:-1;}
    }
    Carve(TX,TY);
}

int32 FAVDepthDirector::RepairConnectivity(TArray<uint8>& Tiles,int32 Width,int32 Height,
                                            const FVector2D& Start,int32 MaxRepairs)
{
    if(Width<5||Height<5||Tiles.Num()!=Width*Height)return 0;
    const int32 SX=FMath::FloorToInt(Start.X),SY=FMath::FloorToInt(Start.Y);
    if(SX<2||SX>=Width-2||SY<2||SY>=Height-2)return 0;
    Tiles[SY*Width+SX]=1;
    int32 Repairs=0;
    for(;Repairs<MaxRepairs;)
    {
        TArray<int32> Distances;
        FAVDungeonNavigation::BuildFlow(Tiles,Width,Height,Start,Distances);
        int32 Best=Width+Height+1;
        int32 FromX=-1,FromY=-1,ToX=-1,ToY=-1;
        // Select the *nearest* disconnected floor component to the explored
        // area. Deterministic tie order; no random holes through the border.
        for(int32 Y=2;Y<Height-2;++Y)
            for(int32 X=2;X<Width-2;++X)
            {
                const int32 I=Y*Width+X;
                if(Tiles[I]==0||Distances[I]>=0)continue;
                for(int32 RY=2;RY<Height-2;++RY)
                    for(int32 RX=2;RX<Width-2;++RX)
                    {
                        if(Distances[RY*Width+RX]<0)continue;
                        const int32 D=FMath::Abs(X-RX)+FMath::Abs(Y-RY);
                        if(D<Best)
                        {Best=D;FromX=RX;FromY=RY;ToX=X;ToY=Y;}
                    }
            }
        if(ToX<0)break;
        // Carve a new 4-connected passage without opening map-edge cells.
        while(FromX!=ToX)
        {FromX+=FromX<ToX?1:-1;Tiles[FromY*Width+FromX]=1;}
        while(FromY!=ToY)
        {FromY+=FromY<ToY?1:-1;Tiles[FromY*Width+FromX]=1;}
        ++Repairs;
    }
    return Repairs;
}

FVector2D FAVDepthDirector::CandidateSpawn(const FAVRoomBounds& R,
                                            EAVFormation Form,int32 Index,int32 Count)
{
    const float CX=R.X+R.W*0.5f,CY=R.Y+R.H*0.5f;
    const float HalfX=FMath::Max(1.f,(R.W-3)*0.38f);
    const float HalfY=FMath::Max(1.f,(R.H-3)*0.38f);
    const float T=Count>1?static_cast<float>(Index)/(Count-1):0.5f;
    // All formations preserve a tactical shape; the collision sweep below
    // snaps invalid tiles to a safe floor square.
    switch(Form)
    {
    case EAVFormation::Perimeter:
    {const float A=6.2831853f*static_cast<float>(Index)/FMath::Max(1,Count);
     return FVector2D(CX+FMath::Cos(A)*HalfX,CY+FMath::Sin(A)*HalfY);}
    case EAVFormation::FrontLine:
        return FVector2D(CX+(T-0.5f)*2.f*HalfX,CY-HalfY);
    case EAVFormation::RearGuard:
        return FVector2D(CX+(T-0.5f)*2.f*HalfX,CY+HalfY);
    case EAVFormation::Pincer:
        return FVector2D(CX+(Index%2==0?-HalfX:HalfX),
                         CY+(T-0.5f)*HalfY);
    case EAVFormation::Pack:
        return FVector2D(CX+(Index%3-1)*0.9f,CY+(Index/3-1)*0.9f);
    case EAVFormation::Wedge:
        return FVector2D(CX+(Index%2==0?-1.f:1.f)*
                         (0.65f+Index/2*0.85f),CY-HalfY+Index*0.7f);
    case EAVFormation::Staggered:
        return FVector2D(CX+(Index%3-1)*1.55f,
                         CY+(Index/3)*1.3f-HalfY);
    case EAVFormation::CornerSentries:
        return FVector2D(CX+(Index%2==0?-HalfX:HalfX),
                         CY+(Index%4<2?-HalfY:HalfY));
    }
    return FVector2D(CX,CY);
}

bool FAVDepthDirector::InRoom(const FAVRoomBounds& R,const FVector2D& P)
{
    return P.X>=R.X && P.X<R.X+R.W && P.Y>=R.Y && P.Y<R.Y+R.H;
}

bool FAVDepthDirector::FindSpawn(const TArray<uint8>& Tiles,int32 Width,int32 Height,
                                  const FAVRoomBounds& R,EAVFormation Form,
                                  int32 Index,int32 Count,const FVector2D& Hero,
                                  const FVector2D& Exit,const FVector2D& Shrine,
                                  const TArray<FVector2D>& Occupied,FVector2D& Out,
                                  float HeroBuffer)
{
    const FVector2D Preferred=CandidateSpawn(R,Form,Index,Count);
    auto IsSafe=[&](const FVector2D& P)
    {
        if(!InRoom(R,P)||!ClearCell(Tiles,Width,Height,P))return false;
        if((P-Hero).SizeSquared()<HeroBuffer*HeroBuffer ||
           (P-Exit).SizeSquared()<6.25f ||
           (P-Shrine).SizeSquared()<4.f)return false;
        for(const FVector2D& Other:Occupied)
            if((P-Other).SizeSquared()<0.81f)return false;
        return true;
    };
    if(IsSafe(Preferred)){Out=Preferred;return true;}
    // Sweep room center-out from a fixed per-index offset. Never silently
    // fall back to a wall or pile multiple enemies on a single spawn.
    const int32 X0=R.X+1,Y0=R.Y+1;
    const int32 WW=FMath::Max(0,R.W-2),HH=FMath::Max(0,R.H-2);
    const int32 Cells=WW*HH;
    if(Cells==0)return false;
    const int32 Shift=(Index*7+Count*3)%Cells;
    for(int32 K=0;K<Cells;++K)
    {
        const int32 Cursor=(Shift+K)%Cells;
        const FVector2D P(X0+Cursor%WW+0.5f,Y0+Cursor/WW+0.5f);
        if(IsSafe(P)){Out=P;return true;}
    }
    return false;
}
