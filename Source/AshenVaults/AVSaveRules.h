#pragma once

#include <cstdint>
#include <cstddef>

namespace AVSaveRules
{
    static const std::int32_t CurrentSchemaVersion = 6;
    static const std::int32_t MaxFloor = 1000000;
    static const std::int32_t MaxEnemies = 512;
    static const std::int32_t MaxPickups = 2048;
    static const std::int32_t MaxChests = 512;
    static const std::int32_t MaxTraps = 512;
    static const std::int32_t MaxHazards = 512;
    static const std::int32_t MaxProps = 2048;
    static const std::int32_t MaxDecorations = 2048;
    static const std::int32_t MaxRoomEvents = 256;
    static const std::int32_t MaxCampDoors = 256;
    static const std::int32_t MaxCampWalls = 512;

    std::int32_t EnemyStride(bool bV09, bool bV13, bool bV15);
    bool IsValidMapForFloor(std::int32_t Floor, std::int32_t Width,
                            std::int32_t Height);
    bool IsValidCheckpointHeader(std::int32_t Floor, std::int32_t HeroClass,
                                 std::int32_t Level, std::int32_t XP,
                                 std::int32_t Gold, std::int32_t Kills,
                                 std::int32_t Potions, std::int32_t Width,
                                 std::int32_t Height);
    bool IsPackedArrayValid(std::int32_t Num, std::int32_t Stride,
                            std::int32_t MaxRecords);
    bool AreFinite(const float* Values, std::int32_t Num);
}
