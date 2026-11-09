#include "AVSaveRules.h"
#include <cmath>

namespace AVSaveRules
{
    std::int32_t EnemyStride(bool bV09, bool bV13, bool bV15)
    {
        if (bV15) return 36;
        if (bV13) return 33;
        if (bV09) return 31;
        return 30;
    }

    bool IsValidMapForFloor(std::int32_t Floor, std::int32_t Width,
                            std::int32_t Height)
    {
        if (Floor < 1 || Floor > MaxFloor) return false;
        const bool bBastion = (((Floor - 1) / 5) % 4) == 3;
        return bBastion ? (Width == 60 && Height == 44) :
                          (Width == 52 && Height == 38);
    }

    bool IsValidCheckpointHeader(std::int32_t Floor, std::int32_t HeroClass,
                                 std::int32_t Level, std::int32_t XP,
                                 std::int32_t Gold, std::int32_t Kills,
                                 std::int32_t Potions, std::int32_t Width,
                                 std::int32_t Height)
    {
        return IsValidMapForFloor(Floor, Width, Height) &&
            HeroClass >= 0 && HeroClass < 4 &&
            Level >= 1 && Level <= MaxFloor &&
            XP >= 0 && Gold >= 0 && Kills >= 0 &&
            Potions >= 0 && Potions <= 999;
    }

    bool IsPackedArrayValid(std::int32_t Num, std::int32_t Stride,
                            std::int32_t MaxRecords)
    {
        if (Num < 0 || Stride <= 0 || MaxRecords < 0) return false;
        if (Num % Stride != 0) return false;
        return Num / Stride <= MaxRecords;
    }

    bool AreFinite(const float* Values, std::int32_t Num)
    {
        if (Num < 0 || (Num > 0 && Values == nullptr)) return false;
        for (std::int32_t I = 0; I < Num; ++I)
            if (!std::isfinite(Values[I])) return false;
        return true;
    }
}
