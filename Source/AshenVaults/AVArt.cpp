#include "AVArt.h"
#include "Engine/Texture2D.h"

namespace
{
    FColor RGB(int32 R, int32 G, int32 B)
    {
        return FColor(static_cast<uint8>(FMath::Clamp(R, 0, 255)),
                      static_cast<uint8>(FMath::Clamp(G, 0, 255)),
                      static_cast<uint8>(FMath::Clamp(B, 0, 255)), 255);
    }

    FColor Shift(const FColor& In, int32 Amount)
    {
        return RGB(static_cast<int32>(In.R) + Amount,
                   static_cast<int32>(In.G) + Amount,
                   static_cast<int32>(In.B) + Amount);
    }

    bool Rune(int32 X, int32 Y)
    {
        const int32 DX = FMath::Abs(X - 12);
        const int32 DY = FMath::Abs(Y - 12);
        return (DX + DY == 7 && X > 4 && X < 20 && Y > 4 && Y < 20) ||
               (DX == 0 && DY <= 4) || (DY == 0 && DX <= 4);
    }
}

uint32 FAVArt::Hash(int32 X, int32 Y, uint32 Seed)
{
    uint32 H = Seed ^ (static_cast<uint32>(X) * 0x9e3779b1u) ^
                      (static_cast<uint32>(Y) * 0x85ebca77u);
    H ^= H >> 16; H *= 0x7feb352du;
    H ^= H >> 15; H *= 0x846ca68bu;
    H ^= H >> 16;
    return H;
}

float FAVArt::LightFalloff(float Distance, float Radius)
{
    if (Radius <= 0.f) return 0.f;
    const float T = FMath::Clamp(1.f - Distance / Radius, 0.f, 1.f);
    // Smoothstep avoids banding from linear brightness; quantized by renderer.
    return T * T * (3.f - 2.f * T);
}

FLinearColor FAVArt::LightColor(int32 Biome)
{
    switch (Biome)
    {
    case 1: return FLinearColor(0.26f, 0.89f, 0.62f, 1.f);
    case 2: return FLinearColor(1.f, 0.49f, 0.22f, 1.f);
    case 3: return FLinearColor(0.81f, 0.37f, 0.30f, 1.f); // Bastion blood-red torches
    default: return FLinearColor(0.54f, 0.71f, 1.f, 1.f);
    }
}

UTexture2D* FAVArt::BuildTile(int32 Biome, int32 TileVariant)
{
    if (Biome < 0 || Biome > 3 || TileVariant < 0 ||
        TileVariant >= VariantsPerBiome)
        return nullptr;

    const bool bWall = TileVariant >= 8;
    const int32 Variant = bWall ? TileVariant - 8 : TileVariant;
    const int32 N = TextureSize;
    const FColor Base = Biome == 0 ? RGB(49, 61, 76) :
                        Biome == 1 ? RGB(42, 75, 62) : Biome == 2 ? RGB(83, 56, 53) : RGB(49, 50, 54);
    const FColor Edge = Biome == 0 ? RGB(24, 34, 49) :
                        Biome == 1 ? RGB(25, 44, 40) : Biome == 2 ? RGB(47, 28, 35) : RGB(23, 24, 29);
    const FColor Light = Biome == 0 ? RGB(115, 147, 164) :
                         Biome == 1 ? RGB(93, 159, 112) : Biome == 2 ? RGB(202, 109, 74) : RGB(123, 117, 113);
    const FColor Accent = Biome == 0 ? RGB(89, 152, 162) :
                          Biome == 1 ? RGB(153, 223, 111) : Biome == 2 ? RGB(255, 149, 63) : RGB(186, 77, 70);
    const uint32 Seed = 9719u + static_cast<uint32>(Biome) * 31337u +
                        static_cast<uint32>(TileVariant) * 487u;

    TArray<FColor> Pixels;
    Pixels.Init(FColor(), N * N);
    for (int32 Y = 0; Y < N; ++Y)
    {
        for (int32 X = 0; X < N; ++X)
        {
            const uint32 H = Hash(X, Y, Seed);
            const int32 Grain = static_cast<int32>(H % 17u) - 8;
            FColor C = Shift(Base, Grain);
            if (bWall)
            {
                // Top lip and vertical brick face; a baked directional relief
                // acts as a low-cost normal/height shading approximation.
                if (Y < 5)
                    C = Shift(Light, -12 + Grain);
                else if (Y == 5 || Y == 21)
                    C = Edge;
                else if (Y > 21)
                    C = Shift(Edge, -7);
                else
                {
                    const int32 Row = (Y - 6) / 7;
                    const int32 Offset = (Row % 2) * 9;
                    const int32 Joint = (X + Offset) % 12;
                    if (Joint == 0 || (Y - 6) % 7 == 0)
                        C = Edge;
                    else if (Joint == 1 || (Y - 6) % 7 == 1)
                        C = Shift(Light, -34 + Grain);
                    else if (X >= 20)
                        C = Shift(Edge, 10 + Grain);
                }
                if (Variant == 1 && Y > 8 && Y < 21 && ((X + Y / 3) % 11 == 0))
                    C = Edge; // fractured masonry
                if (Variant == 2 && Y > 10 && (H % 27u == 0))
                    C = Accent; // glowing mineral dust
                if (Variant == 3 && Y > 10 && ((X / 3 + Y / 2) % 19 == 0))
                    C = Shift(Edge, -5); // missing stone chips
            }
            else
            {
                // Chiselled bevel, mortar lines, variable stone surface.
                if (X == 0 || Y == 0) C = Shift(Light, -42 + Grain);
                if (X == 23 || Y == 23) C = Edge;
                if (X == 1 && Y > 3) C = Shift(Light, -58 + Grain);
                if (Variant == 0 && X > 4 && X < 19 && Y > 4 && Y < 19)
                {
                    if (X == 5 || Y == 5) C = Shift(Light, -33 + Grain);
                    if (X == 18 || Y == 18) C = Shift(Edge, 15);
                }
                else if (Variant == 1)
                {
                    if (FMath::Abs(X - (7 + Y / 3)) <= 1 && Y > 4 && Y < 20)
                        C = Edge; // stepped crack
                    if ((X == 13 || X == 14) && Y > 14) C = Shift(Light, -20);
                }
                else if (Variant == 2)
                {
                    if (Rune(X, Y)) C = Accent;
                    if (Rune(X + 1, Y + 1)) C = Shift(Edge, 24);
                }
                else if (Variant == 3)
                {
                    if ((X / 5 + Y / 5) % 2 == 0)
                        C = Shift(Base, Grain + 9); // larger stones
                    if (X % 5 == 0 || Y % 5 == 0) C = Edge;
                }
                else if (Variant == 4)
                {
                    if ((H % 23u) < 3u) C = Shift(Accent, -56);
                    if (Biome == 1 && (X - 11) * (X - 11) +
                        (Y - 10) * (Y - 10) < 13) C = Shift(Accent, -26);
                }
                else if (Variant == 5)
                {
                    if (Y > 6 && Y < 19 && X > 6 && X < 17 &&
                        ((X + Y * 2) % 13) == 0) C = Edge;
                    if (Biome == 2 && ((X * 3 + Y) % 17 < 2))
                        C = Shift(Accent, -12); // hot seams
                }
                else if (Variant == 6)
                {
                    if ((X + 2 * Y) % 13 == 0) C = Shift(Edge, -2);
                    if (Y >= 9 && Y <= 14 && X >= 8 && X <= 15)
                        C = Shift(Base, Grain + 16);
                }
                else if (Variant == 7)
                {
                    if (X > 7 && X < 17 && Y > 7 && Y < 17 &&
                        ((X + Y) % 3 == 0)) C = Shift(Light, -42);
                    if (Biome == 0 && X >= 11 && X <= 12 && Y >= 9 && Y <= 15)
                        C = Accent;
                }
            }
            // Small one-pixel highlights reinforce handmade pixel detail.
            if (H % 137u == 0u) C = Shift(C, 21);
            Pixels[Y * N + X] = C;
        }
    }
    UTexture2D* Texture = UTexture2D::CreateTransient(N, N, PF_B8G8R8A8);
    if (!Texture) return nullptr;
    Texture->Filter = TF_Nearest;
    Texture->SRGB = true;
    Texture->NeverStream = true;
    Texture->CompressionSettings = TC_VectorDisplacementmap;
    uint8* Data = static_cast<uint8*>(
        Texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
    if (!Data)
    {
        Texture->PlatformData->Mips[0].BulkData.Unlock();
        return nullptr;
    }
    for (int32 I = 0; I < Pixels.Num(); ++I)
    {
        Data[I * 4 + 0] = Pixels[I].B;
        Data[I * 4 + 1] = Pixels[I].G;
        Data[I * 4 + 2] = Pixels[I].R;
        Data[I * 4 + 3] = 255;
    }
    Texture->PlatformData->Mips[0].BulkData.Unlock();
    Texture->UpdateResource();
    return Texture;
}
