#pragma once
#include "CoreMinimal.h"

class UTexture2D;

/**
 * Loads portable, CC0 or original PNGs directly from Content/AVVisuals.
 * Keeps artwork non-proprietary and source-controlled without binary .uassets;
 * paths are case-sensitive and included in packaged builds via DefaultGame.ini.
 */
class FAVImageLoader
{
public:
    static UTexture2D* LoadPNG(const FString& RelativeContentPath, bool bPixelArt = true);
};
