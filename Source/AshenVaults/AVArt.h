#pragma once

#include "CoreMinimal.h"

class UTexture2D;

/**
 * Authored procedural pixel-art tile library for the temporary Canvas renderer.
 *
 * Runtime textures are 24x24 RGBA, nearest-neighbour, non-streamed and owned by
 * the HUD UPROPERTY array. They are fully deterministic and require no binaries.
 * When a real Paper2D atlas is introduced, this module is the palette reference.
 */
class FAVArt
{
public:
    static constexpr int32 FloorVariants = 20;
    static constexpr int32 WallVariants = 8;
    static constexpr int32 VariantsPerBiome = FloorVariants + WallVariants;
    static constexpr int32 TextureSize = 24;

    static UTexture2D* BuildTile(int32 Biome, int32 TileVariant);
    static uint32 Hash(int32 X, int32 Y, uint32 Seed);
    static float LightFalloff(float Distance, float Radius);
    static FLinearColor LightColor(int32 Biome);
};
