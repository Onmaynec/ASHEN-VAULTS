#include "AVFortressLayout.h"

namespace {
uint32 Next(uint32& State)
{
    State ^= State << 13;
    State ^= State >> 17;
    State ^= State << 5;
    return State;
}
int32 Roll(uint32& State,int32 Lo,int32 Hi)
{
    return Lo+static_cast<int32>(Next(State) % static_cast<uint32>(Hi-Lo+1));
}
void Carve(TArray<uint8>& Tiles,int32 X,int32 Y)
{
    if(X>=2 && X<FAVFortressLayout::Width-2 &&
       Y>=2 && Y<FAVFortressLayout::Height-2)
        Tiles[Y*FAVFortressLayout::Width+X]=1;
}
void Gallery(TArray<uint8>& Tiles,
             const FAVFortressRoom& A,const FAVFortressRoom& B)
{
    int32 X=A.X+A.W/2,Y=A.Y+A.H/2;
    const int32 TX=B.X+B.W/2,TY=B.Y+B.H/2;
    auto Dig=[&](int32 CX,int32 CY){
        for(int32 O=-1;O<=1;++O) {
            Carve(Tiles,CX+O,CY);
            Carve(Tiles,CX,CY+O);
        }
    };
    while(X!=TX){Dig(X,Y);X+=TX>X?1:-1;}
    while(Y!=TY){Dig(X,Y);Y+=TY>Y?1:-1;}
    Dig(TX,TY);
}
}
void FAVFortressLayout::Build(int32 Seed,TArray<uint8>& Tiles,
                            TArray<FAVFortressRoom>& Rooms)
{
    Tiles.Init(0,Width*Height);
    Rooms.Init(FAVFortressRoom{},0);
    uint32 State=static_cast<uint32>(Seed)^0xA92C574Du;
    if(State==0) State=0x9E3779B9u;
    const int32 AnchorX[3]={3,22,43};
    const int32 AnchorY[3]={3,16,32};
    for(int32 Row=0;Row<3;++Row)
        for(int32 Col=0;Col<3;++Col) {
            const bool Center=Row==1 && Col==1;
            FAVFortressRoom R;
            R.X=AnchorX[Col]+(Col==1?0:Roll(State,0,1));
            R.Y=AnchorY[Row]+(Row==1?0:Roll(State,0,1));
            R.W=Center?16:Roll(State,10,14);
            R.H=Center?12:Roll(State,7,9);
            Rooms.Add(R);
            for(int32 Y=R.Y;Y<R.Y+R.H;++Y)
                for(int32 X=R.X;X<R.X+R.W;++X)
                    Carve(Tiles,X,Y);
        }
    // A spanning tree keeps entry-to-boss traversal possible at every seed.
    for(int32 Row=0;Row<3;++Row)
        for(int32 Col=0;Col<2;++Col)
            Gallery(Tiles,Rooms[Row*3+Col],Rooms[Row*3+Col+1]);
    Gallery(Tiles,Rooms[1],Rooms[4]);
    Gallery(Tiles,Rooms[4],Rooms[7]);
    // Unpredictable side gates create different circular fighting routes.
    for(int32 Col=0;Col<3;Col+=2)
        for(int32 Row=0;Row<2;++Row)
            if(Roll(State,0,99)<72)
                Gallery(Tiles,Rooms[Row*3+Col],Rooms[(Row+1)*3+Col]);
}
