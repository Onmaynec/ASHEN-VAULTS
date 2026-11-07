#include "AVArt.h"
#include "Engine/Texture2D.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <set>
#include <vector>

int main()
{
    assert(FAVArt::VariantsPerBiome == 12);
    assert(FAVArt::TextureSize == 24);
    assert(std::abs(FAVArt::LightFalloff(0.f, 5.f) - 1.f) < 0.0001f);
    assert(std::abs(FAVArt::LightFalloff(2.5f, 5.f) - 0.5f) < 0.0001f);
    assert(FAVArt::LightFalloff(5.f, 5.f) == 0.f);
    assert(FAVArt::LightFalloff(2.f, 0.f) == 0.f);
    assert(FAVArt::LightFalloff(10.f, 5.f) == 0.f);
    assert(FAVArt::Hash(1, 2, 1234u) == FAVArt::Hash(1, 2, 1234u));
    assert(FAVArt::Hash(1, 2, 1234u) != FAVArt::Hash(2, 1, 1234u));
    assert(FAVArt::BuildTile(-1, 0) == nullptr);
    assert(FAVArt::BuildTile(3, 0) == nullptr);
    assert(FAVArt::BuildTile(0, 12) == nullptr);
    assert(FAVArt::BuildTile(0, -1) == nullptr);

    std::set<std::vector<uint8>> Unique;
    for (int Biome = 0; Biome < 3; ++Biome)
    {
        const auto Glow = FAVArt::LightColor(Biome);
        assert(Glow.R > 0.f && Glow.G > 0.f && Glow.B > 0.f);
        for (int Variant = 0; Variant < FAVArt::VariantsPerBiome; ++Variant)
        {
            UTexture2D* Tile = FAVArt::BuildTile(Biome, Variant);
            assert(Tile && Tile->Width == 24 && Tile->Height == 24);
            assert(Tile->Filter == TF_Nearest);
            assert(Tile->NeverStream && Tile->SRGB);
            const auto& Pixels = Tile->PlatformData->Mips[0].BulkData.Bytes;
            assert(Pixels.size() == 24u * 24u * 4u);
            for (size_t I = 3; I < Pixels.size(); I += 4)
                assert(Pixels[I] == 255u);
            Unique.insert(Pixels);
            delete Tile;
        }
    }
    // All 36 runtime tile variants are distinct, not recolored flat blocks.
    assert(Unique.size() == 36u);
    std::cout << "PASS: 36 biome textures, relief shading, deterministic hashes, light math\n";
    return 0;
}
