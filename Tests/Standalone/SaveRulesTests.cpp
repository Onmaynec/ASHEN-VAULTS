#include "AVSaveRules.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

int main()
{
    using namespace AVSaveRules;
    assert(CurrentSchemaVersion==6);
    assert(EnemyStride(false,false,false)==30);
    assert(EnemyStride(true,false,false)==31);
    assert(EnemyStride(true,true,false)==33);
    assert(EnemyStride(true,true,true)==36);

    for(int Floor=1;Floor<=1000;++Floor)
    {
        const bool Bastion=(((Floor-1)/5)%4)==3;
        assert(IsValidMapForFloor(Floor,Bastion?60:52,Bastion?44:38));
        assert(!IsValidMapForFloor(Floor,Bastion?52:60,Bastion?38:44));
    }
    assert(!IsValidMapForFloor(0,52,38));
    assert(!IsValidMapForFloor(MaxFloor+1,52,38));
    assert(IsValidCheckpointHeader(1,0,1,0,0,0,2,52,38));
    assert(IsValidCheckpointHeader(20,3,30,400,100,500,0,60,44));
    assert(IsValidCheckpointHeader(21,2,1,0,0,0,2,52,38));
    assert(!IsValidCheckpointHeader(1,4,1,0,0,0,2,52,38));
    assert(!IsValidCheckpointHeader(1,0,0,0,0,0,2,52,38));
    assert(!IsValidCheckpointHeader(1,0,1,-1,0,0,2,52,38));
    assert(!IsValidCheckpointHeader(1,0,1,0,-1,0,2,52,38));
    assert(!IsValidCheckpointHeader(1,0,1,0,0,0,1000,52,38));

    assert(IsPackedArrayValid(0,36,MaxEnemies));
    assert(IsPackedArrayValid(36*512,36,MaxEnemies));
    assert(!IsPackedArrayValid(35,36,MaxEnemies));
    assert(!IsPackedArrayValid(36*513,36,MaxEnemies));
    assert(!IsPackedArrayValid(10,0,MaxEnemies));
    assert(!IsPackedArrayValid(-1,5,MaxPickups));
    assert(IsPackedArrayValid(15*256,15,MaxRoomEvents));
    assert(!IsPackedArrayValid(15*257,15,MaxRoomEvents));

    const float Good[]={0.f,1.f,-3.5f,100000.f};
    assert(AreFinite(Good,4));
    assert(AreFinite(nullptr,0));
    assert(!AreFinite(nullptr,1));
    const float BadNaN[]={0.f,std::numeric_limits<float>::quiet_NaN()};
    const float BadInf[]={std::numeric_limits<float>::infinity()};
    assert(!AreFinite(BadNaN,2));
    assert(!AreFinite(BadInf,1));
    assert(!AreFinite(Good,-1));

    std::cout<<"PASS: save schema v6, legacy enemy strides, 1,000 floor/map mappings, "
             <<"checkpoint bounds, packed-array limits and finite-value validation\n";
    return 0;
}
