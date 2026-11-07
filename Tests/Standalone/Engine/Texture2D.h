#pragma once
// Standalone-only texture mock. The actual runtime uses Unreal UTexture2D.
// This header is NEVER part of Unreal Build.cs includes.
#include "CoreMinimal.h"
#include <memory>
#include <vector>
constexpr int PF_B8G8R8A8 = 0;
constexpr int TF_Nearest = 1;
constexpr int TC_VectorDisplacementmap = 2;
constexpr int LOCK_READ_WRITE = 0;
struct FMockBulkData
{
    std::vector<uint8> Bytes;
    void* Lock(int) { return Bytes.data(); }
    void Unlock() {}
};
struct FMockMip
{
    FMockBulkData BulkData;
};
struct FMockPlatformData
{
    std::vector<FMockMip> Mips;
};
class UTexture2D
{
public:
    int Width = 0, Height = 0;
    int Filter = 0, CompressionSettings = 0;
    bool SRGB = false, NeverStream = false;
    std::unique_ptr<FMockPlatformData> PlatformData;
    static UTexture2D* CreateTransient(int W, int H, int)
    {
        auto* Texture = new UTexture2D;
        Texture->Width = W; Texture->Height = H;
        Texture->PlatformData.reset(new FMockPlatformData);
        Texture->PlatformData->Mips.resize(1);
        Texture->PlatformData->Mips[0].BulkData.Bytes.resize(W * H * 4);
        return Texture;
    }
    void UpdateResource() {}
};
