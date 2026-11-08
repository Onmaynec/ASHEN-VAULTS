#include "AVHUD.h"
#include "AVGameMode.h"
#include "AVContent.h"
#include "AVEquipment.h"
#include "AVSkills.h"
#include "AVArt.h"
#include "AVImageLoader.h"
#include "AVDungeonNavigation.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Engine/Font.h"

namespace
{
    const FLinearColor Ink(0.045f, 0.055f, 0.085f, 1.f);
    const FLinearColor Pale(0.86f, 0.86f, 0.81f, 1.f);
    const FLinearColor GoldColor(0.95f, 0.72f, 0.27f, 1.f);
    const FLinearColor Blood(0.82f, 0.22f, 0.26f, 1.f);
    const FLinearColor Mint(0.29f, 0.82f, 0.68f, 1.f);
    const FLinearColor Aether(0.44f, 0.65f, 0.97f, 1.f);

    FColor PixelColor(uint8 R, uint8 G, uint8 B, uint8 A = 255)
    {
        return FColor(R, G, B, A);
    }
}

void AAVHUD::Rect(float X, float Y, float W, float H, const FLinearColor& Color)
{
    if (W > 0.f && H > 0.f) DrawRect(Color, X, Y, W, H);
}

void AAVHUD::Text(const FString& Value, float X, float Y, const FLinearColor& Color, float Size)
{
    if (GEngine)
        DrawText(Value, Color, X, Y, GEngine->GetSmallFont(), FMath::Max(0.5f, Scale * Size), false);
}

FVector2D AAVHUD::Screen(const FVector2D& World) const
{
    return Origin + World * (AAVGameMode::TilePixels * Scale);
}

UTexture2D* AAVHUD::MakeSprite(int32 Type)
{
    const int32 N = 16;
    TArray<FColor> Pixels;
    Pixels.Init(FColor::Transparent, N * N);
    auto Put = [&](int32 X, int32 Y, FColor C)
    {
        if (X >= 0 && X < N && Y >= 0 && Y < N) Pixels[Y * N + X] = C;
    };
    auto Box = [&](int32 X1, int32 Y1, int32 X2, int32 Y2, FColor C)
    {
        for (int32 Y = Y1; Y <= Y2; ++Y)
            for (int32 X = X1; X <= X2; ++X)
                Put(X, Y, C);
    };
    const FColor Shadow = PixelColor(19, 20, 37);
    const FColor Outline = PixelColor(33, 30, 46);
    const FColor DarkMetal = PixelColor(55, 67, 87);
    const FColor Metal = PixelColor(127, 149, 166);
    const FColor White = PixelColor(221, 223, 205);
    const FColor Rust = PixelColor(125, 62, 62);
    const FColor Fire = PixelColor(249, 162, 79);

    if (Type <= 4 || Type == 35)
    {
        // Shared silhouette, second walking pose for animation.
        const bool Hero = Type == 0 || Type == 35;
        const bool Knight = Type == 2;
        const bool Seer = Type == 3;
        const bool Boss = Type == 4;
        const FColor Cloak = Hero ? PixelColor(43, 118, 131) :
            Knight ? PixelColor(100, 89, 112) :
            Seer ? PixelColor(100, 58, 127) :
            Boss ? PixelColor(88, 53, 77) : PixelColor(124, 63, 67);
        const FColor Light = Hero ? PixelColor(90, 199, 187) :
            Knight ? PixelColor(176, 161, 170) :
            Seer ? PixelColor(174, 114, 220) :
            Boss ? PixelColor(232, 74, 100) : PixelColor(184, 98, 82);
        Box(4, 13, 11, 14, Shadow);
        Box(5, 11, 7, 13, Outline); Box(9, 11, 11, 13, Outline);
        Box(5, 12, 7, 13, DarkMetal); Box(9, 12, 11, 13, DarkMetal);
        Box(3, 7, 12, 11, Outline);
        Box(4, 7, 11, 10, Cloak);
        Box(6, 8, 9, 10, Light);
        Box(3, 8, 4, 11, DarkMetal); Box(11, 8, 12, 11, DarkMetal);
        Box(5, 3, 10, 7, Outline);
        Box(6, 4, 9, 6, Knight || Boss ? Metal : PixelColor(180, 148, 125));
        Box(5, 3, 10, 4, Cloak);
        Box(6, 5, 6, 5, Hero ? White : Fire);
        Box(9, 5, 9, 5, Hero ? White : Fire);
        if (Hero)
        {
            Box(4, 2, 11, 3, Outline); Box(5, 2, 10, 2, Light);
            Box(12, 7, 12, 11, Metal); Box(12, 5, 12, 6, White);
            Put(12, 4, White); Put(13, 8, GoldColor.ToFColor(true));
            if (Type == 35)
            {
                Box(5, 11, 7, 13, Outline);
                Box(8, 12, 10, 14, DarkMetal);
                Put(10, 11, Light);
                Put(4, 7, Light);
            }
        }
        else if (Seer)
        {
            Put(7, 1, Light); Box(5, 2, 10, 3, Cloak);
            Box(13, 5, 13, 12, Metal); Put(13, 4, Light);
            Box(12, 3, 14, 4, Light);
        }
        else if (Knight)
        {
            Box(5, 2, 10, 3, Metal); Put(4, 3, Metal); Put(11, 3, Metal);
            Box(12, 7, 14, 10, Metal);
        }
        else if (Boss)
        {
            Box(4, 2, 11, 3, Rust);
            Put(3, 1, Fire); Put(12, 1, Fire); Put(4, 2, Fire); Put(11, 2, Fire);
            Put(6, 6, Fire); Put(9, 6, Fire);
        }
        else
        {
            Put(4, 2, Rust); Put(11, 2, Rust);
            Box(6, 6, 9, 6, Rust);
        }
    }
    else if (Type == 5) // dropped gold
    {
        Box(4, 5, 11, 11, PixelColor(96, 57, 39));
        Box(5, 4, 10, 10, PixelColor(209, 136, 54));
        Box(6, 4, 9, 8, PixelColor(251, 205, 96));
        Put(7, 5, White); Put(8, 6, White);
    }
    else if (Type == 6 || Type == 7 || Type == 8) // recovery and potion
    {
        const FColor Fluid = Type == 6 ? PixelColor(183, 48, 82) :
            Type == 7 ? PixelColor(49, 137, 213) : PixelColor(219, 57, 83);
        Box(6, 2, 9, 4, Metal);
        Box(5, 5, 10, 12, Shadow);
        Box(6, 6, 9, 11, Fluid);
        Box(7, 7, 7, 9, White);
        Put(6, 12, Metal); Put(9, 12, Metal);
    }
    else if (Type == 9) // stone stairs
    {
        Box(2, 10, 13, 13, PixelColor(42, 53, 63));
        Box(4, 7, 11, 10, PixelColor(72, 95, 103));
        Box(6, 4, 9, 7, PixelColor(117, 155, 142));
        Box(7, 2, 8, 4, PixelColor(153, 221, 189));
        Put(7, 2, White);
    }
    else if (Type == 10) // projectile
    {
        Box(6, 6, 9, 9, PixelColor(173, 86, 216));
        Box(7, 7, 8, 8, White);
        Put(5, 8, Fire); Put(10, 7, Fire);
    }
    else if (Type == 11) // ancient shrine
    {
        Box(3, 12, 12, 14, Shadow);
        Box(5, 9, 10, 12, DarkMetal);
        Box(4, 8, 11, 9, Metal);
        Box(6, 5, 9, 8, PixelColor(53, 124, 133));
        Box(7, 3, 8, 6, PixelColor(86, 211, 193));
        Put(7, 2, White); Put(8, 2, White);
        Box(3, 5, 4, 10, DarkMetal); Box(11, 5, 12, 10, DarkMetal);
    }
    else if (Type == 12) // ashen damage shard
    {
        Put(7, 2, White); Put(8, 2, White);
        Box(6, 3, 9, 4, PixelColor(186, 132, 244));
        Box(5, 5, 10, 9, PixelColor(119, 70, 194));
        Box(6, 10, 9, 11, PixelColor(80, 46, 152));
        Put(7, 12, PixelColor(63, 39, 116));
        Box(7, 4, 8, 7, PixelColor(225, 194, 255));
    }

    else if (Type == 13 || Type == 14) // ancient loot cache
    {
        Box(2, 6, 13, 12, Shadow);
        Box(3, 7, 12, 11, PixelColor(98, 58, 51));
        Box(3, 5, 12, 7, PixelColor(148, 94, 64));
        Box(5, 4, 10, 5, PixelColor(184, 123, 77));
        Box(3, 8, 4, 11, PixelColor(194, 153, 82));
        Box(11, 8, 12, 11, PixelColor(194, 153, 82));
        if (Type == 13)
        {
            Box(7, 6, 8, 9, PixelColor(247, 202, 103));
            Put(7, 7, White);
        }
        else
        {
            Box(3, 3, 12, 5, DarkMetal);
            Box(5, 2, 10, 3, PixelColor(102, 80, 69));
            Box(6, 10, 9, 11, PixelColor(45, 32, 37));
        }
    }
    else if (Type == 15) // player throwing blade
    {
        Put(4, 11, PixelColor(75, 135, 137));
        Box(5, 9, 7, 11, PixelColor(133, 101, 67));
        Box(7, 7, 9, 9, Metal);
        Box(9, 5, 11, 7, White);
        Put(12, 4, White);
        Put(7, 10, Fire);
    }
    else if (Type == 16) // mimic creature
    {
        Box(2, 8, 13, 13, Shadow);
        Box(3, 7, 12, 10, PixelColor(152, 71, 75));
        Box(2, 5, 13, 8, PixelColor(116, 69, 56));
        Box(4, 3, 11, 5, PixelColor(179, 116, 70));
        Box(6, 6, 7, 7, Fire);
        Box(10, 6, 11, 7, Fire);
        for (int32 X = 4; X <= 11; X += 2) Put(X, 9, White);
        for (int32 X = 5; X <= 10; X += 2) Put(X, 11, White);
        Box(6, 12, 9, 13, PixelColor(195, 76, 118));
        Put(2, 4, Fire); Put(13, 4, Fire);
    }

    else if (Type >= 17 && Type <= 22) // new archetypes and three boss silhouettes
    {
        const FColor Body = Type == 17 ? PixelColor(118, 106, 107) :
            Type == 18 ? PixelColor(82, 146, 110) :
            Type == 19 ? PixelColor(149, 74, 55) :
            Type == 20 ? PixelColor(156, 146, 115) :
            Type == 21 ? PixelColor(186, 93, 49) : PixelColor(112, 95, 168);
        const FColor Glow = Type == 17 ? PixelColor(232, 88, 105) :
            Type == 18 ? PixelColor(188, 244, 117) :
            Type == 19 ? PixelColor(255, 159, 57) :
            Type == 20 ? PixelColor(244, 201, 131) :
            Type == 21 ? PixelColor(252, 171, 68) : PixelColor(189, 154, 251);
        Box(3, 12, 12, 14, Shadow);
        Box(3, 7, 12, 11, Outline);
        Box(4, 5, 11, 10, Body);
        Box(5, 4, 10, 6, Body);
        Box(6, 7, 7, 8, Glow); Box(9, 7, 10, 8, Glow);
        Box(4, 11, 6, 13, Body); Box(9, 11, 11, 13, Body);
        if (Type == 17) // grave hound: low, horned silhouette
        {
            Box(2, 8, 5, 10, Body);
            Put(3, 5, White); Put(11, 3, White); Put(12, 6, Shadow);
            Box(12, 9, 14, 10, Body);
        }
        else if (Type == 18) // sporecaller: toxic mushroom cap
        {
            Box(3, 3, 12, 5, Glow);
            Box(5, 2, 10, 3, Body);
            Put(6, 3, White); Put(10, 4, White);
            Put(2, 9, Glow); Put(13, 10, Glow);
        }
        else if (Type == 19) // cinder golem: stone arms and magma seams
        {
            Box(1, 7, 4, 12, Body); Box(11, 7, 14, 12, Body);
            Box(7, 4, 8, 13, Glow);
            Box(4, 8, 6, 8, Glow); Box(9, 10, 11, 10, Glow);
        }
        else if (Type == 20) // bone regent: crown
        {
            Box(3, 3, 12, 4, Glow);
            Put(3, 1, White); Put(7, 0, White); Put(12, 1, White);
            Box(4, 6, 11, 10, Metal);
            Box(6, 8, 9, 8, Shadow);
        }
        else if (Type == 21) // forge tyrant: blazing horns
        {
            Box(3, 2, 5, 5, Fire); Box(10, 2, 12, 5, Fire);
            Put(2, 1, Glow); Put(13, 1, Glow);
            Box(6, 9, 9, 12, Glow);
            Box(1, 9, 3, 12, Body); Box(12, 9, 14, 12, Body);
        }
        else if (Type == 22) // void wisp: floating orb
        {
            Box(4, 4, 11, 11, Body);
            Box(6, 6, 9, 9, Glow);
            Box(7, 7, 8, 8, White);
            Put(2, 5, Glow); Put(13, 10, Glow); Put(6, 2, Glow);
            Box(4, 12, 11, 14, FColor::Transparent);
        }
    }
    else if ((Type >= 47 && Type <= 69) || (Type >= 81 && Type <= 103))
    {
        // Enemy-specific 16x16 pixel art. 81..103 are alternate walking frames.
        const bool Walking = Type >= 81;
        const int32 Kind = Walking ? Type - 70 : Type - 36;
        const bool Golem = Kind == 23 || Kind == 24 || Kind == 13 || Kind == 29 || Kind == 32;
        const bool Skeleton = Kind == 17 || Kind == 27;
        const bool Ghost = Kind == 14 || Kind == 18;
        const bool Rat = Kind == 15;
        const bool Zombie = Kind == 16 || Kind == 22;
        const bool Bandit = Kind == 25 || Kind == 33;
        const bool Witch = Kind == 12 || Kind == 19 || Kind == 21 || Kind == 28 || Kind == 31;
        const bool Mini = Kind >= 26 && Kind <= 32;
        const FColor Primary = Golem ? (Kind == 24 || Kind == 32 ? PixelColor(113, 125, 143) :
            Kind == 23 || Kind == 29 ? PixelColor(103, 105, 101) : PixelColor(122, 67, 56)) :
            Skeleton ? PixelColor(211, 207, 174) :
            Ghost ? PixelColor(107, 171, 187) :
            Rat ? PixelColor(107, 85, 102) :
            Zombie ? PixelColor(91, 127, 93) :
            Bandit ? PixelColor(134, 67, 58) :
            Witch ? PixelColor(91, 76, 134) :
            Kind == 20 ? PixelColor(122, 125, 133) :
            Kind == 30 ? PixelColor(160, 51, 62) : PixelColor(127, 91, 87);
        const FColor Highlight = Golem ? (Kind == 24 || Kind == 32 ? PixelColor(196, 208, 221) :
            Kind == 23 || Kind == 29 ? PixelColor(163, 172, 157) : Fire) :
            Skeleton ? White : Ghost ? PixelColor(171, 242, 233) :
            Rat ? PixelColor(190, 128, 134) :
            Zombie ? PixelColor(154, 186, 125) :
            Bandit ? PixelColor(231, 163, 98) :
            Witch ? PixelColor(182, 124, 217) : PixelColor(220, 148, 111);
        const FColor Eyes = Ghost ? White : Witch ? PixelColor(199, 255, 118) : Fire;
        if (Ghost)
        {
            Box(4, 4, 11, 10, Primary);
            Box(5, 3, 10, 5, Highlight);
            Box(3, 8, 12, 11, Primary);
            Box(4, 12, 6, 13, Highlight); Box(9, 12, 11, 13, Highlight);
            Put(7, 14, Highlight);
        }
        else if (Rat)
        {
            Box(2, 9, 12, 12, Primary);
            Box(9, 6, 13, 10, Primary);
            Box(10, 5, 11, 6, Highlight);
            Put(12, 8, Eyes);
            Box(1, 11, 3, 12, Highlight);
            Box(4, 13, 5, 14, Shadow); Box(10, 13, 11, 14, Shadow);
        }
        else if (Golem)
        {
            Box(2, 7, 13, 12, Outline);
            Box(3, 5, 12, 11, Primary);
            Box(5, 2, 10, 7, Primary);
            Box(1, 7, 3, 12, Primary); Box(12, 7, 14, 12, Primary);
            Box(4, 12, 6, 14, Primary); Box(9, 12, 11, 14, Primary);
            Box(7, 4, 8, 10, Highlight);
            Put(6, 5, Eyes); Put(9, 5, Eyes);
            if (Kind == 13) { Put(4, 3, Fire); Put(11, 3, Fire); }
        }
        else
        {
            Box(4, 7, 11, 12, Outline);
            Box(5, 7, 10, 11, Primary);
            Box(5, 11, 7, Walking ? 12 : 14, Primary);
            Box(9, Walking ? 11 : 12, 11, Walking ? 14 : 13, Primary);
            Box(5, 3, 10, 7, Outline);
            Box(6, 4, 9, 6, Skeleton ? White : Highlight);
            Box(3, 8, 4, 11, Primary);
            Box(11, 8, 12, 11, Primary);
            Put(6, 5, Eyes); Put(9, 5, Eyes);
            if (Skeleton)
            {
                Box(7, 8, 8, 11, White);
                Put(5, 9, White); Put(10, 9, White);
                Box(6, 6, 9, 6, Shadow);
            }
            if (Zombie)
            {
                Box(5, 2, 10, 3, Primary);
                Put(5, 10, Highlight); Put(11, 12, Highlight);
            }
            if (Witch)
            {
                Box(3, 3, 12, 4, Primary);
                Box(5, 1, 10, 3, Primary);
                Box(12, 5, 13, 13, DarkMetal);
                Put(13, 4, Highlight);
                Put(13, 2, Eyes);
            }
            if (Bandit)
            {
                Box(4, 2, 11, 4, Rust);
                Box(5, 8, 10, 9, Highlight);
                Box(12, 5, 13, 12, DarkMetal);
                Put(13, 4, Fire);
            }
            if (Kind == 20) { Box(4, 2, 11, 4, Metal); Box(11, 8, 14, 11, Metal); }
            if (Kind == 11 || Kind == 30) { Box(3, 2, 12, 3, Rust); Put(12, 2, Fire); }
        }
        if (Mini)
        {
            Put(3, 2, Fire); Put(12, 2, Fire);
            Box(5, 0, 10, 1, Rust);
            Put(7, 0, Highlight); Put(9, 0, Highlight);
        }
        if (Walking)
        {
            Put(3, 12, Highlight);
            Put(12, 13, Highlight);
        }
    }
    else if ((Type >= 128 && Type <= 173) || (Type >= 182 && Type <= 227))
    {
        // Variant sprites have distinct armor tint and insignia, plus two walk poses.
        const bool bSubclass=Type>=182;
        const int32 Kind=34+(bSubclass?(Type-182):(Type-128))/2;
        const bool Walking=(Type&1)!=0;
        const bool Giant = Kind == 42 || Kind == 48 || Kind == 54 || Kind == 56;
        const bool Armored = Kind == 34 || Kind == 35 || Kind == 36 || Kind == 38 ||
                             Kind == 44 || Kind == 48 || Kind == 50 || Kind == 53 || Kind == 56;
        const bool Beast = Kind == 40 || Kind == 41 || Kind == 43;
        const bool Mage = Kind == 46 || Kind == 47 || Kind == 49 || Kind == 50 || Kind == 55;
        const bool Mini = Kind >= 52 && Kind <= 55;
        const FColor Base=bSubclass?PixelColor(97,55,65) :
            Armored ? PixelColor(68, 71, 82) :
            Beast ? PixelColor(106, 75, 70) :
            Mage ? PixelColor(94, 54, 82) : PixelColor(100, 78, 77);
        const FColor Trim=bSubclass?PixelColor(196,125,103) :
            Armored ? PixelColor(169, 156, 144) :
            Beast ? PixelColor(170, 91, 79) :
            Mage ? PixelColor(170, 88, 125) : PixelColor(174, 114, 103);
        const FColor Glow = PixelColor(232, 77, 72);
        Box(4, 12, 11, 14, Shadow);
        if (Beast)
        {
            Box(3, 7, 12, 11, Base);
            Box(9, 5, 13, 9, Trim);
            Put(11, 7, Glow); Put(12, 7, White);
            Box(3, 11, 5, Walking ? 12 : 14, Base);
            Box(10, Walking ? 12 : 11, 12, 14, Base);
            Put(5, 5, Trim); Put(10, 4, Trim);
        }
        else
        {
            Box(Giant ? 2 : 4, 7, Giant ? 13 : 11, 12, Outline);
            Box(Giant ? 3 : 5, 7, Giant ? 12 : 10, 11, Base);
            Box(5, 3, 10, 7, Outline); Box(6, 4, 9, 6, Trim);
            Box(4, 12, 6, Walking ? 13 : 14, Base);
            Box(9, Walking ? 12 : 11, 11, 14, Base);
            Box(2, 8, 4, 11, Base); Box(11, 8, 13, 11, Base);
            Put(6, 5, Glow); Put(9, 5, Glow);
            if (Armored) { Box(4, 2, 11, 3, Trim); Box(5, 8, 10, 8, Trim); }
            if (Mage) { Box(4, 2, 11, 4, Base); Put(13, 6, Glow); Put(13, 4, Trim); }
            if (Kind == 35 || Kind == 38 || Kind == 53) { Box(13, 3, 14, 13, Trim); Put(13, 2, White); }
            if (Kind == 36 || Kind == 53) { Box(1, 7, 4, 12, Trim); Box(2, 8, 3, 11, DarkMetal); }
            if (Kind == 37 || Kind == 45) { Box(11, 8, 15, 9, Trim); Put(15, 8, White); }
            if (Kind == 39 || Kind == 51) { Put(4, 2, Glow); Put(11, 2, Glow); }
            if (Giant) { Box(1, 7, 3, 13, Trim); Box(12, 7, 14, 13, Trim); }
            if (Kind == 56) { Box(3, 2, 12, 3, Glow); Put(4, 0, White); Put(8, 0, White); Put(12, 0, White); }
        }
        if(Kind==47 || Kind==49) {
            Box(13,4,14,13,DarkMetal); Put(13,3,Glow);
            Box(5,11,10,13,Base);
        }
        if(Kind==46 || Kind==50) {
            Box(12,7,14,9,Glow); Put(14,7,White);
        }
        if(Kind==48 || Kind==52) {
            Box(13,2,15,12,Trim); Box(11,2,15,5,DarkMetal);
        }
        if(Kind==51) {
            Box(2,6,3,13,Metal); Box(12,6,13,13,Metal);
        }
        if(Kind==41) {
            Box(1,6,4,8,Trim); Box(11,6,14,8,Trim);
        }
        if(Kind==43) {
            Box(1,10,14,13,Base); Put(12,10,Glow);
        }
        if(bSubclass) {
            Put(2,3,Glow);Put(13,3,Glow);
            Box(5,2,10,3,PixelColor(155,74,64));
            Put(7,8,GoldColor.ToFColor(true));
        }
        if (Mini) { Put(3, 2, Glow); Put(12, 2, Glow); }
        if (Walking) { Put(3, 13, Trim); Put(12, 13, Trim); }
    }
    else if (Type == 174 || Type == 175) // Marksman, idle/walk
    {
        const bool Walking = Type == 175;
        const FColor Leather = PixelColor(82, 79, 62);
        const FColor Cloak = PixelColor(58, 86, 68);
        Box(4, 13, 11, 14, Shadow);
        Box(4, 8, 11, 12, Cloak);
        Box(5, 4, 10, 8, Outline);
        Box(6, 5, 9, 7, PixelColor(154, 132, 102));
        Box(4, 3, 11, 5, Cloak);
        Put(6, 6, White); Put(9, 6, White);
        Box(5, 11, 7, Walking ? 12 : 14, Leather);
        Box(9, Walking ? 11 : 12, 11, Walking ? 14 : 13, Leather);
        Box(12, 3, 12, 12, Rust); // bow
        Put(13, 4, White); Put(13, 11, White);
        Box(10, 8, 14, 8, Metal);
    }
    else if (Type == 176) // Arrow
    {
        Box(2, 7, 12, 8, Rust);
        Box(11, 6, 14, 9, Metal);
        Put(1, 6, White); Put(1, 9, White);
    }
    else if (Type >= 177 && Type <= 179) // Bastion statue / torch / seal
    {
        if (Type == 177) {
            Box(3, 12, 12, 14, Shadow); Box(5, 4, 10, 12, DarkMetal);
            Box(6, 3, 9, 6, Metal); Box(4, 7, 11, 10, PixelColor(74, 77, 83));
        } else if (Type == 178) {
            Box(6, 7, 9, 14, Metal); Box(4, 10, 11, 12, DarkMetal);
            Box(6, 3, 9, 8, Fire); Put(7, 2, White); Put(8, 2, White);
        } else {
            Box(3, 12, 12, 14, Shadow); Box(5, 3, 10, 12, DarkMetal);
            Box(6, 5, 9, 10, PixelColor(160, 53, 62)); Put(7, 4, White);
        }
    }
    else if(Type>=228 && Type<=243)
    {
        const int32 Item=Type-228;
        const int32 Region=Item/4,Style=Item%4;
        const FColor Body=Region==0?PixelColor(114,105,99):
            Region==1?PixelColor(67,122,75):
            Region==2?PixelColor(133,86,69):PixelColor(81,84,94);
        const FColor Detail=Region==0?PixelColor(190,169,145):
            Region==1?PixelColor(142,203,106):
            Region==2?PixelColor(229,133,74):PixelColor(175,63,66);
        if(Style==0) // bone pile / fungi / embers / torn banner
        {
            Box(2,10,13,14,Shadow);
            Box(3,8,12,12,Body);
            Box(5,6,10,8,Detail);
            Put(5,9,White);Put(10,6,White);
            Box(7,11,9,12,DarkMetal);
        }
        else if(Style==1) // coffins / stalks / hanging chain / weapon rack
        {
            Box(3,4,12,14,Shadow);
            Box(5,3,10,13,Body);
            Box(6,4,9,5,Detail);
            Box(4,8,5,13,DarkMetal);
            Box(11,8,12,13,DarkMetal);
            Put(7,10,White);Put(10,11,Detail);
        }
        else if(Style==2) // ancient carving / roots / lava / broken statue
        {
            Box(3,12,12,14,Shadow);
            Box(6,3,10,13,Body);
            Box(5,5,11,8,Detail);
            Box(7,8,9,12,DarkMetal);
            Put(8,4,White);Put(6,7,White);
        }
        else // candles / spores / ingots / caged torch
        {
            Box(5,8,10,14,DarkMetal);
            Box(4,11,11,13,Body);
            Box(6,3,9,9,Detail);
            Put(7,2,White);Put(8,3,Fire);
            Put(10,5,Detail);
        }
        if(Region==3) {
            Box(2,12,13,14,DarkMetal);
            Put(3,4,Detail);Put(12,4,Detail);
        }
    }
    else if (Type == 181) // magic sphere
    {
        Box(5,4,10,11,PixelColor(112,66,164));
        Box(6,5,9,10,PixelColor(190,126,241));
        Box(7,6,8,9,White);
        Put(2,8,PixelColor(180,94,242)); Put(13,8,PixelColor(180,94,242));
    }
    else if (Type == 180) // equipment bag
    {
        Box(4, 7, 11, 13, Rust); Box(5, 4, 10, 9, PixelColor(170, 119, 75));
        Box(6, 3, 9, 4, GoldColor.ToFColor(true)); Put(7, 10, White);
    }
    else if (Type >= 104 && Type <= 106) // salvageable enemy equipment
    {
        if (Type == 104) // chestplate
        {
            Box(4, 4, 11, 12, Outline);
            Box(5, 4, 10, 11, Metal);
            Box(6, 5, 9, 9, DarkMetal);
            Box(7, 5, 8, 7, White);
            Box(3, 6, 4, 9, Metal); Box(11, 6, 12, 9, Metal);
        }
        else if (Type == 105) // bow
        {
            for (int32 Y = 2; Y <= 13; ++Y) Put(11, Y, White);
            Box(5, 3, 7, 5, Rust); Box(4, 6, 5, 9, Rust);
            Box(5, 10, 7, 12, Rust); Box(7, 12, 10, 13, Rust);
            Box(7, 2, 10, 3, Rust); Box(3, 7, 13, 8, Metal);
            Put(13, 7, White);
        }
        else // enchanted blade
        {
            Box(7, 2, 8, 10, PixelColor(175, 105, 227));
            Box(7, 1, 8, 3, White);
            Box(4, 10, 11, 11, Metal);
            Box(7, 12, 8, 14, Rust);
            Put(6, 5, Fire); Put(9, 7, Fire);
        }
    }
    else if (Type == 23) // wandering merchant
    {
        Box(3, 12, 12, 14, Shadow);
        Box(4, 6, 11, 12, PixelColor(86, 72, 121));
        Box(5, 3, 10, 6, PixelColor(175, 139, 109));
        Box(3, 3, 12, 4, PixelColor(81, 63, 93));
        Box(6, 5, 6, 5, GoldColor.ToFColor(true));
        Box(9, 5, 9, 5, GoldColor.ToFColor(true));
        Box(12, 7, 14, 11, PixelColor(159, 116, 58));
        Put(13, 8, Fire); Put(13, 10, Fire);
    }
    else if (Type == 24 || Type == 25) // destructible pot or crate
    {
        if (Type == 24)
        {
            Box(4, 5, 11, 12, PixelColor(87, 69, 73));
            Box(5, 3, 10, 5, PixelColor(142, 113, 100));
            Box(5, 6, 10, 11, PixelColor(149, 94, 87));
            Box(6, 6, 6, 11, PixelColor(198, 136, 105));
            Box(7, 12, 8, 13, Shadow);
        }
        else
        {
            Box(3, 4, 12, 13, PixelColor(102, 64, 48));
            Box(4, 5, 11, 12, PixelColor(165, 106, 68));
            Box(5, 5, 6, 12, PixelColor(83, 59, 56));
            Box(10, 5, 11, 12, PixelColor(83, 59, 56));
            Box(3, 8, 12, 9, PixelColor(198, 142, 83));
        }
    }

    else if (Type >= 26 && Type <= 34) // unique 3-biome layered props
    {
        const int32 Object = Type - 26;
        if (Object == 0) // crypt tombstone
        {
            Box(4, 4, 11, 13, Outline);
            Box(5, 3, 10, 12, Metal);
            Box(6, 5, 9, 11, DarkMetal);
            Put(7, 6, White); Put(8, 6, White);
            Box(2, 13, 13, 14, Shadow);
        }
        else if (Object == 1) // crypt candelabra
        {
            Box(7, 5, 8, 13, Metal); Box(5, 13, 10, 14, DarkMetal);
            Box(3, 8, 12, 9, Metal);
            for (int32 X = 4; X <= 12; X += 4)
            {
                Put(X, 6, Fire); Put(X, 5, White);
                Put(X, 7, GoldColor.ToFColor(true));
            }
        }
        else if (Object == 2) // crypt rune monolith
        {
            Box(3, 12, 12, 14, Shadow);
            Box(5, 3, 10, 12, DarkMetal);
            Box(6, 2, 9, 11, PixelColor(74, 91, 110));
            Box(7, 5, 8, 7, PixelColor(114, 225, 222));
            Put(6, 8, White); Put(9, 8, White);
        }
        else if (Object == 3) // giant mushroom cluster
        {
            Box(6, 7, 9, 13, PixelColor(192, 172, 138));
            Box(3, 4, 12, 8, PixelColor(72, 130, 95));
            Box(5, 2, 10, 5, PixelColor(128, 207, 122));
            Put(5, 5, White); Put(10, 5, White);
            Box(11, 10, 14, 13, PixelColor(77, 133, 81));
            Put(13, 9, PixelColor(173, 228, 112));
        }
        else if (Object == 4) // bioluminescent flower
        {
            Box(7, 7, 8, 14, PixelColor(73, 133, 77));
            Box(4, 4, 11, 9, PixelColor(42, 104, 83));
            Box(6, 3, 9, 8, PixelColor(111, 234, 181));
            Box(7, 5, 8, 6, White);
            Put(3, 7, PixelColor(159, 255, 202));
            Put(12, 7, PixelColor(159, 255, 202));
        }
        else if (Object == 5) // echo root stone
        {
            Box(2, 12, 13, 14, Shadow);
            Box(4, 4, 11, 12, PixelColor(58, 86, 72));
            Box(5, 2, 10, 4, PixelColor(104, 147, 104));
            Box(7, 5, 8, 10, PixelColor(184, 234, 98));
            Put(6, 7, White); Put(9, 7, White);
            Put(3, 10, PixelColor(113, 197, 84));
            Put(12, 11, PixelColor(113, 197, 84));
        }
        else if (Object == 6) // forge brazier
        {
            Box(5, 10, 10, 13, DarkMetal);
            Box(3, 8, 12, 11, Metal);
            Box(4, 7, 11, 9, PixelColor(147, 63, 43));
            Box(6, 3, 9, 8, Fire);
            Box(7, 2, 8, 6, White);
            Put(5, 5, PixelColor(233, 107, 45));
            Put(10, 5, PixelColor(233, 107, 45));
        }
        else if (Object == 7) // anvil with iron base
        {
            Box(5, 10, 10, 14, DarkMetal);
            Box(3, 6, 12, 10, Metal);
            Box(2, 5, 13, 7, PixelColor(134, 126, 128));
            Box(3, 4, 7, 5, White);
            Put(2, 8, Shadow); Put(13, 8, Shadow);
        }
        else // forge echo core
        {
            Box(3, 12, 12, 14, Shadow);
            Box(5, 4, 10, 12, PixelColor(104, 62, 71));
            Box(6, 3, 9, 11, PixelColor(174, 90, 62));
            Box(7, 5, 8, 8, Fire);
            Put(7, 4, White); Put(8, 4, White);
            Put(5, 2, PixelColor(255, 191, 87));
            Put(10, 2, PixelColor(255, 191, 87));
        }
    }

    UTexture2D* Texture = UTexture2D::CreateTransient(N, N, PF_B8G8R8A8);
    if (!Texture) return nullptr;
    Texture->Filter = TF_Nearest;
    Texture->SRGB = true;
    Texture->NeverStream = true;
    Texture->CompressionSettings = TC_VectorDisplacementmap;
    uint8* Data = static_cast<uint8*>(Texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
    if (Data)
    {
        for (int32 I = 0; I < Pixels.Num(); ++I)
        {
            Data[I * 4 + 0] = Pixels[I].B;
            Data[I * 4 + 1] = Pixels[I].G;
            Data[I * 4 + 2] = Pixels[I].R;
            Data[I * 4 + 3] = Pixels[I].A;
        }
    }
    Texture->PlatformData->Mips[0].BulkData.Unlock();
    Texture->UpdateResource();
    return Texture;
}

void AAVHUD::InitSprites()
{
    if(Sprites.Num()>0)return;
    for(int32 I=0;I<=243;++I) Sprites.Add(MakeSprite(I));
    const TCHAR* Atlases[4]={
        TEXT("AVVisuals/Tiles/CryptAtlas.png"),
        TEXT("AVVisuals/Tiles/SporeAtlas.png"),
        TEXT("AVVisuals/Tiles/FoundryAtlas.png"),
        TEXT("AVVisuals/Tiles/BastionAtlas.png")
    };
    HeroAtlas=FAVImageLoader::LoadPNG(TEXT("AVVisuals/Characters/HeroAtlas.png"));
    EquipmentAtlas=FAVImageLoader::LoadPNG(TEXT("AVVisuals/Icons/EquipmentAtlas.png"));
    BossAtlas=FAVImageLoader::LoadPNG(TEXT("AVVisuals/Bosses/BossAtlas.png"));
    EnemyAtlas=FAVImageLoader::LoadPNG(TEXT("AVVisuals/Enemies/EnemyAtlas.png"));
    for(int32 Biome=0;Biome<4;++Biome) {
        AuthoredAtlases.Add(FAVImageLoader::LoadPNG(Atlases[Biome]));
        // Procedural fallback remains deterministic when the PNG is absent.
        for(int32 Variant=0;Variant<FAVArt::VariantsPerBiome;++Variant)
            BiomeTiles.Add(FAVArt::BuildTile(Biome,Variant));
    }
}

void AAVHUD::Sprite(int32 Type, const FVector2D& Pos, float Size, float Alpha)
{
    if (!Canvas || !Sprites.IsValidIndex(Type) || !Sprites[Type]) return;
    const FVector2D At = Screen(Pos);
    const float Width = AAVGameMode::TilePixels * Scale * Size;
    // UCanvas::DrawTile does not accept an FLinearColor argument on UE4.27.
    // AHUD::DrawTexture uses the correct tint + blend-mode signature.
    DrawTexture(Sprites[Type], At.X - Width * 0.5f, At.Y - Width * 0.5f,
                Width, Width, 0.f, 0.f, 1.f, 1.f,
                FLinearColor(1.f, 1.f, 1.f, Alpha), BLEND_Translucent,
                1.f, false, 0.f, FVector2D::ZeroVector);
}

void AAVHUD::SpriteRotated(int32 Type,const FVector2D& Pos,float Size,
                           float AngleRadians,float Alpha)
{
    if(!Canvas || !Sprites.IsValidIndex(Type) || !Sprites[Type])return;
    const FVector2D At=Screen(Pos);
    const float Width=AAVGameMode::TilePixels*Scale*Size;
    DrawTexture(Sprites[Type],At.X-Width*0.5f,At.Y-Width*0.5f,
                Width,Width,0.f,0.f,1.f,1.f,
                FLinearColor(1.f,1.f,1.f,Alpha),BLEND_Translucent,
                1.f,false,FMath::RadiansToDegrees(AngleRadians),
                FVector2D(0.5f,0.5f));
}

void AAVHUD::DrawDungeon(const AAVGameMode& Game)
{
    const float T = AAVGameMode::TilePixels * Scale;
    const int32 MinX = FMath::Max(0, FMath::FloorToInt(-Origin.X / T) - 1);
    const int32 MaxX = FMath::Min(Game.MapW - 1,
        FMath::CeilToInt((Canvas->SizeX - Origin.X) / T) + 1);
    const int32 MinY = FMath::Max(0, FMath::FloorToInt(-Origin.Y / T) - 1);
    const int32 MaxY = FMath::Min(Game.MapH - 1,
        FMath::CeilToInt((Canvas->SizeY - Origin.Y) / T) + 1);
    const int32 Biome = Game.CurrentBiome();
    for (int32 Y = MinY; Y <= MaxY; ++Y)
    {
        for (int32 X = MinX; X <= MaxX; ++X)
        {
            const float SX = Origin.X + X * T, SY = Origin.Y + Y * T;
            const uint32 Hash = FAVArt::Hash(X, Y, static_cast<uint32>(Game.RunSeed));
            if (Game.IsFloor(X, Y))
            {
                if (!Game.IsExploredCell(X, Y)) continue;
                const int32 Variant = static_cast<int32>(Hash % static_cast<uint32>(FAVArt::FloorVariants));
                const int32 Index = Biome * FAVArt::VariantsPerBiome + Variant;
                if(AuthoredAtlases.IsValidIndex(Biome) && AuthoredAtlases[Biome])
                {
                    const float U=(Variant%7)/7.f;
                    const float V=(Variant/7)/4.f;
                    DrawTexture(AuthoredAtlases[Biome],SX,SY,T+0.5f,T+0.5f,
                                U,V,1.f/7.f,1.f/4.f,FLinearColor::White,
                                BLEND_Translucent,1.f,false,0.f,
                                FVector2D::ZeroVector);
                }
                else if(BiomeTiles.IsValidIndex(Index) && BiomeTiles[Index])
                    DrawTexture(BiomeTiles[Index],SX,SY,T+0.5f,T+0.5f,
                                0.f,0.f,1.f,1.f,FLinearColor::White,
                                BLEND_Translucent,1.f,false,0.f,
                                FVector2D::ZeroVector);
                if (!Game.IsVisibleCell(X, Y))
                    Rect(SX, SY, T + 0.5f, T + 0.5f,
                         FLinearColor(0.005f, 0.016f, 0.026f, 0.82f));
            }
            else
            {
                bool bExploredNeighbour = false;
                bool bVisibleNeighbour = false;
                for (int32 DY = -1; DY <= 1; ++DY)
                    for (int32 DX = -1; DX <= 1; ++DX)
                    {
                        bExploredNeighbour |= Game.IsExploredCell(X + DX, Y + DY);
                        bVisibleNeighbour |= Game.IsVisibleCell(X + DX, Y + DY);
                    }
                if (!bExploredNeighbour) continue;
                const int32 Variant = FAVArt::FloorVariants +
                    static_cast<int32>(Hash % static_cast<uint32>(FAVArt::WallVariants));
                const int32 Index = Biome * FAVArt::VariantsPerBiome + Variant;
                if(AuthoredAtlases.IsValidIndex(Biome) && AuthoredAtlases[Biome])
                {
                    const float U=(Variant%7)/7.f;
                    const float V=(Variant/7)/4.f;
                    DrawTexture(AuthoredAtlases[Biome],SX,SY,T+0.5f,T+0.5f,
                                U,V,1.f/7.f,1.f/4.f,FLinearColor::White,
                                BLEND_Translucent,1.f,false,0.f,
                                FVector2D::ZeroVector);
                }
                else if(BiomeTiles.IsValidIndex(Index) && BiomeTiles[Index])
                    DrawTexture(BiomeTiles[Index],SX,SY,T+0.5f,T+0.5f,
                                0.f,0.f,1.f,1.f,FLinearColor::White,
                                BLEND_Translucent,1.f,false,0.f,
                                FVector2D::ZeroVector);
                // Contact shadow behind masonry ledge gives 2.5D wall height.
                if (Game.IsFloor(X, Y + 1))
                    Rect(SX, SY + 19.f * Scale, T, 5.f * Scale,
                         FLinearColor(0.f, 0.006f, 0.01f, 0.40f));
                if (!bVisibleNeighbour)
                    Rect(SX, SY, T + 0.5f, T + 0.5f,
                         FLinearColor(0.f, 0.012f, 0.018f, 0.86f));
            }
        }
    }
}

// Palette-quantized, wall-occluded lighting pass on the actual tile grid.
// This is a lightweight Canvas lighting implementation (NOT a GPU HLSL
// post-process). Sources never illuminate through solid dungeon walls.
void AAVHUD::DrawLighting(const AAVGameMode& Game)
{
    if (Game.VisualQuality == 0) return;
    const float T = AAVGameMode::TilePixels * Scale;
    const FLinearColor Color = FAVArt::LightColor(Game.CurrentBiome());
    const int32 MinX = FMath::Max(0, FMath::FloorToInt(-Origin.X / T) - 1);
    const int32 MaxX = FMath::Min(Game.MapW - 1,
        FMath::CeilToInt((Canvas->SizeX - Origin.X) / T) + 1);
    const int32 MinY = FMath::Max(0, FMath::FloorToInt(-Origin.Y / T) - 1);
    const int32 MaxY = FMath::Min(Game.MapH - 1,
        FMath::CeilToInt((Canvas->SizeY - Origin.Y) / T) + 1);
    for (int32 Y = MinY; Y <= MaxY; ++Y)
    {
        for (int32 X = MinX; X <= MaxX; ++X)
        {
            if (!Game.IsFloor(X, Y) || !Game.IsVisibleCell(X, Y)) continue;
            const FVector2D Cell(X + 0.5f, Y + 0.5f);
            const float HeroDistance = FVector2D::Distance(Cell, Game.Hero);
            float Brightness = 0.12f + 0.16f * FAVArt::LightFalloff(HeroDistance, 8.f);
            float WarmLight = 0.f;
            if (Game.VisualQuality >= 1)
            {
                for (const FAVDecor& Decor : Game.Decorations)
                {
                    if ((Decor.Type != 1 && Decor.Type != 4 && Decor.Type != 6 && Decor.Type != 10 &&
                Decor.Type != 15 && Decor.Type != 19 && Decor.Type != 22 && Decor.Type != 27) ||
                        !Game.IsVisibleAt(Decor.Pos)) continue;
                    const float Distance = FVector2D::Distance(Cell, Decor.Pos);
                    if (Distance > 5.0f) continue;
                    if (Game.VisualQuality == 2 &&
                        !FAVDungeonNavigation::LineOfSight(
                             Game.Tiles, Game.MapW, Game.MapH,
                             Decor.Pos, Cell))
                        continue;
                    const float Flicker = 0.85f + 0.15f *
                        FMath::Sin(Game.RunTime * 7.2f + Decor.Phase);
                    WarmLight += 0.33f * FAVArt::LightFalloff(Distance, 5.f) * Flicker;
                }
            }
            // Combat effects briefly illuminate nearby tilework.
            if (Game.VisualQuality == 2)
            {
                for (const FAVEffect& Effect : Game.Effects)
                {
                    if (FVector2D::Distance(Effect.Pos, Cell) > 3.0f) continue;
                    if (!FAVDungeonNavigation::LineOfSight(
                          Game.Tiles, Game.MapW, Game.MapH,
                          Effect.Pos, Cell))
                        continue;
                    Brightness += 0.20f * FAVArt::LightFalloff(
                        FVector2D::Distance(Effect.Pos, Cell), 3.f) *
                        (Effect.Life / FMath::Max(0.01f, Effect.Duration));
                }
            }
            // Hazards and hostile projectiles illuminate the immediate floor
            // while remaining subject to fog and the visible-cell mask.
            if (Game.VisualQuality == 2)
            {
                for (const FAVHazard& Hazard : Game.Hazards)
                    if (FVector2D::Distance(Hazard.Pos, Cell) < 3.1f)
                        WarmLight += 0.18f * FAVArt::LightFalloff(
                            FVector2D::Distance(Hazard.Pos, Cell), 3.1f);
                for (const FAVBolt& Bolt : Game.Bolts)
                    if (FVector2D::Distance(Bolt.Pos, Cell) < 2.6f)
                        WarmLight += 0.16f * FAVArt::LightFalloff(
                            FVector2D::Distance(Bolt.Pos, Cell), 2.6f);
            }
            // Pixel-quantized darkness preserves hard edges while animated
            // light sources carve readable pools in every biome.
            const float Total=FMath::Clamp(Brightness+WarmLight,0.f,0.92f);
            Brightness=FMath::FloorToInt(Total*10.f)/10.f;
            const float SX=Origin.X+X*T,SY=Origin.Y+Y*T;
            const float Darkness=FMath::Clamp(0.64f-Brightness*1.05f,0.03f,0.64f);
            Rect(SX,SY,T+0.5f,T+0.5f,
                 FLinearColor(0.006f,0.01f,0.026f,Darkness));
            if(Brightness>0.13f)
                Rect(SX,SY,T+0.5f,T+0.5f,
                    FLinearColor(Color.R,Color.G,Color.B,
                                 FMath::Min(0.27f,Brightness*0.28f)));
            // Tiny local highlight where the brightest lamps strike the floor.
            if(WarmLight>0.18f && Game.VisualQuality==2)
                Rect(SX+3.f*Scale,SY+3.f*Scale,T-6.f*Scale,T-6.f*Scale,
                    FLinearColor(Color.R,Color.G,Color.B,
                                 FMath::Min(0.12f,WarmLight*0.17f)));
        }
    }
}

// Tiny parallax ash/spore/ember flecks use only a fixed number of draw calls.
// Deterministic positions from the run seed; no UObject particle allocations.
void AAVHUD::DrawAtmosphere(const AAVGameMode& Game)
{
    if (Game.VisualQuality == 0) return;
    const int32 Count = Game.VisualQuality == 2 ? 58 : 24;
    const int32 Biome = Game.CurrentBiome();
    const FLinearColor Palette = Biome == 0 ?
        FLinearColor(0.66f, 0.68f, 0.74f, 0.38f) :
        Biome == 1 ? FLinearColor(0.44f, 0.93f, 0.52f, 0.48f) :
        Biome == 2 ? FLinearColor(1.f, 0.58f, 0.26f, 0.55f) :
                     FLinearColor(0.74f, 0.28f, 0.26f, 0.55f);
    for (int32 I = 0; I < Count; ++I)
    {
        const uint32 H = FAVArt::Hash(I, Game.Floor, static_cast<uint32>(Game.RunSeed));
        const float W = Canvas->SizeX, Ht = Canvas->SizeY - 120.f * Scale;
        if (Ht <= 0.f || W <= 0.f) return;
        const float Speed = Biome == 2 ? -14.f : Biome == 1 ? 7.f : 3.5f;
        const float X = FMath::Fmod(static_cast<float>(H % 4096u) / 4096.f * W +
            Game.RunTime * (I % 3 == 0 ? Speed * 1.5f : Speed) + W * 8.f, W);
        const float Y = FMath::Fmod(static_cast<float>((H >> 12) % 4096u) /
            4096.f * Ht + Game.RunTime *
            (Biome == 2 ? -23.f : Biome == 1 ? -8.f : 10.f) + Ht * 8.f, Ht);
        const float Size = ((H >> 7) % 4u == 0u ? 2.f : 1.f) * Scale;
        if (Y < 38.f * Scale) continue;
        Rect(X, Y, Size, Size, Palette);
    }

    // Subtle screen-space vignette masks the harsh edges of the procedural
    // viewport without blurring texels or covering the UI panel.
    const float FrameWidth = 7.f * Scale;
    for (int32 Band = 0; Band < 7; ++Band)
    {
        const float A = 0.045f * (7.f - Band) / 7.f;
        const FLinearColor Shade(0.f, 0.01f, 0.025f, A);
        Rect(Band * FrameWidth, 38.f * Scale,
             FrameWidth, Canvas->SizeY - 110.f * Scale, Shade);
        Rect(Canvas->SizeX - (Band + 1) * FrameWidth, 38.f * Scale,
             FrameWidth, Canvas->SizeY - 110.f * Scale, Shade);
        Rect(0.f, 38.f * Scale + Band * FrameWidth,
             Canvas->SizeX, FrameWidth, Shade);
    }
}

void AAVHUD::DrawActors(const AAVGameMode& Game)
{
    const float T = AAVGameMode::TilePixels * Scale;
    const float Pulse = 0.5f + 0.5f * FMath::Sin(Game.RunTime * 4.5f);

    // Objects remain hidden behind walls until line-of-sight reaches them.
    if (Game.bInSafeCamp || Game.bInLobby)
    {
        const FVector2D Gate = Screen(Game.Exit);
        Rect(Gate.X - 16.f * Scale, Gate.Y - 20.f * Scale,
             32.f * Scale, 40.f * Scale, Mint);
        Text(Game.bInLobby ? TEXT("RESUME [E]") : TEXT("EXIT [E]"),
             Gate.X - 34.f * Scale, Gate.Y - 38.f * Scale, Pale, 0.72f);
        if (Game.bInSafeCamp)
        {
            const FVector2D Trader = Screen(Game.Merchant);
            Sprite(23, Game.Merchant, 1.15f);
            Text(TEXT("MERCHANT [E]"), Trader.X - 44.f * Scale,
                 Trader.Y - 29.f * Scale, GoldColor, 0.72f);
            const FVector2D Smith = Screen(FVector2D(32.5f, 22.5f));
            Rect(Smith.X - 11.f * Scale, Smith.Y - 16.f * Scale,
                 22.f * Scale, 29.f * Scale, GoldColor);
            Text(TEXT("SMITH [E] / SHIFT+E"), Smith.X - 75.f * Scale,
                 Smith.Y - 34.f * Scale, Pale, 0.72f);
            const FVector2D Traveler = Screen(Game.Shrine);
            Sprite(23, Game.Shrine, 1.1f);
            Text(TEXT("TRAVELER: HOME [E]"), Traveler.X - 69.f * Scale,
                 Traveler.Y - 31.f * Scale, Mint, 0.72f);
        }
    }
    if (Game.bHasSafeCampDoor && Game.IsVisibleAt(Game.SafeCampDoor + FVector2D(0.f, 1.f)))
    {
        const FVector2D Door = Screen(Game.SafeCampDoor);
        Rect(Door.X - 12.f * Scale, Door.Y - 17.f * Scale,
             24.f * Scale, 33.f * Scale, GoldColor);
        Text(TEXT("SAFE CAMP [E]"), Door.X - 40.f * Scale,
             Door.Y - 31.f * Scale, Pale, 0.75f);
    }
    if (Game.IsVisibleAt(Game.Exit))
    {
        const FVector2D Stair = Screen(Game.Exit);
        Rect(Stair.X - T * 0.47f, Stair.Y - T * 0.35f, T * 0.94f, T * 0.8f,
             Game.IsExitReady() ? FLinearColor(0.17f, 0.52f, 0.44f, 0.32f + Pulse * 0.25f) :
                                  FLinearColor(0.26f, 0.12f, 0.15f, 0.45f));
        Sprite(9, Game.Exit, 1.1f, Game.IsExitReady() ? 1.f : 0.55f);
    }
    if (Game.bMerchantSpawned && Game.IsVisibleAt(Game.Merchant))
    {
        const FVector2D Pt = Screen(Game.Merchant);
        Rect(Pt.X - 17.f * Scale, Pt.Y - 16.f * Scale,
             34.f * Scale, 3.f * Scale, GoldColor);
        Sprite(23, Game.Merchant, 1.15f);
    }
    for (const FAVHazard& Hazard : Game.Hazards)
    {
        if (!Game.IsVisibleAt(Hazard.Pos)) continue;
        const FVector2D Pt = Screen(Hazard.Pos);
        const float Glow = 0.2f + 0.1f * FMath::Sin(Game.RunTime * 6.f + Hazard.Pos.X);
        const FLinearColor Color = Hazard.Type == 0 ?
            FLinearColor(0.28f, 0.83f, 0.27f, Glow) :
            FLinearColor(1.f, 0.31f, 0.05f, Glow);
        Rect(Pt.X - 20.f * Scale, Pt.Y - 20.f * Scale,
             40.f * Scale, 40.f * Scale, Color);
        Rect(Pt.X - 11.f * Scale, Pt.Y - 8.f * Scale,
             22.f * Scale, 16.f * Scale, Color);
        for (int32 I = 0; I < 4; ++I)
            Rect(Pt.X + (I * 7.f - 12.f) * Scale,
                 Pt.Y + (FMath::Sin(Game.RunTime * 3.f + I) * 5.f - 5.f) * Scale,
                 3.f * Scale, 3.f * Scale,
                 Hazard.Type == 0 ? Mint : GoldColor);
    }
    for (const FAVProp& Prop : Game.Props)
        if (Game.IsVisibleAt(Prop.Pos))
            Sprite(Prop.Type == 0 ? 24 : 25, Prop.Pos, 0.93f);

    // Layered world silhouettes: non-colliding 2.5D props have contact
    // shadows and subtle emissive features that reflect the biome palette.
    for (const FAVDecor& Decor : Game.Decorations)
    {
        if (!Game.IsVisibleAt(Decor.Pos)) continue;
        const FVector2D Pt = Screen(Decor.Pos);
        const float S = Scale;
        const bool bLuminous=Decor.Type==1 || Decor.Type==4 ||
             Decor.Type==6 || Decor.Type==10 ||
             Decor.Type==15 || Decor.Type==19 ||
             Decor.Type==22 || Decor.Type==27;
        const bool bEcho = Decor.Type % 3 == 2;
        const float PulseIntensity = 0.5f + 0.5f *
            FMath::Sin(Game.RunTime * 5.5f + Decor.Phase);
        const FLinearColor LightColor = FAVArt::LightColor(Game.CurrentBiome());
        Rect(Pt.X - 11.f * S, Pt.Y + 8.f * S,
             22.f * S, 5.f * S,
             FLinearColor(0.f, 0.003f, 0.012f, 0.55f));
        if (bLuminous && Game.VisualQuality > 0)
        {
            Rect(Pt.X - 15.f * S, Pt.Y - 17.f * S,
                 30.f * S, 26.f * S,
                 FLinearColor(LightColor.R, LightColor.G, LightColor.B,
                              0.035f + PulseIntensity * 0.085f));
        }
        const FVector2D Bob = Decor.Pos +
            FVector2D(0.f, bLuminous ? -PulseIntensity * 0.06f : 0.f);
        Sprite(Decor.Type >= 12 ? 228 + Decor.Type - 12 :
               Decor.Type >= 9 ? 177 + Decor.Type - 9 : 26 + Decor.Type,
               Bob, bEcho ? 1.32f : 1.12f,
               Decor.bActivated ? 0.45f : 1.f);
        if (bEcho && !Decor.bActivated)
        {
            for (int32 I = 0; I < 3; ++I)
            {
                const float A = Game.RunTime * 1.6f + Decor.Phase + I * 2.094f;
                const FVector2D Spark = Pt + FVector2D(FMath::Cos(A), FMath::Sin(A)) *
                    (13.f * S);
                Rect(Spark.X, Spark.Y, 2.f * S, 2.f * S,
                     FLinearColor(LightColor.R, LightColor.G, LightColor.B,
                                  0.5f + 0.45f * PulseIntensity));
            }
        }
    }

    if (!Game.bShrineUsed && Game.IsVisibleAt(Game.Shrine))
    {
        const FVector2D Site = Screen(Game.Shrine);
        Rect(Site.X - 17.f * Scale, Site.Y - 11.f * Scale,
             34.f * Scale, 25.f * Scale,
             FLinearColor(0.12f, 0.42f, 0.42f, 0.13f + Pulse * 0.17f));
        Sprite(11, Game.Shrine, 1.4f);
    }

    for (const FVector2D& Wall : Game.CampWalls)
    {
        if (!Game.IsVisibleAt(Wall)) continue;
        const FVector2D Pt = Screen(Wall);
        Rect(Pt.X - 10.f * Scale, Pt.Y - 10.f * Scale,
             20.f * Scale, 20.f * Scale, FLinearColor(0.25f, 0.18f, 0.15f, 1.f));
        Rect(Pt.X - 7.f * Scale, Pt.Y - 9.f * Scale,
             4.f * Scale, 18.f * Scale, FLinearColor(0.48f, 0.32f, 0.2f, 1.f));
        Rect(Pt.X + 3.f * Scale, Pt.Y - 9.f * Scale,
             4.f * Scale, 18.f * Scale, FLinearColor(0.48f, 0.32f, 0.2f, 1.f));
    }
    for (const FVector2D& Door : Game.CampDoors)
    {
        const FVector2D Pt = Screen(Door);
        if (!Game.IsExploredCell(FMath::FloorToInt(Door.X), FMath::FloorToInt(Door.Y)) &&
            FVector2D::Distance(Door, Game.Hero) > 4.f) continue;
        Rect(Pt.X - 11.f * Scale, Pt.Y - 11.f * Scale,
             22.f * Scale, 22.f * Scale, FLinearColor(0.16f, 0.11f, 0.10f, 1.f));
        Rect(Pt.X - 8.f * Scale, Pt.Y - 10.f * Scale,
             16.f * Scale, 20.f * Scale, FLinearColor(0.54f, 0.31f, 0.17f, 1.f));
        Rect(Pt.X - 9.f * Scale, Pt.Y - 2.f * Scale,
             18.f * Scale, 4.f * Scale, FLinearColor(0.23f, 0.20f, 0.22f, 1.f));
        Rect(Pt.X + 3.f * Scale, Pt.Y + 1.f * Scale,
             3.f * Scale, 3.f * Scale, GoldColor);
        if (FVector2D::Distance(Door, Game.Hero) < 2.5f)
            Text(TEXT("[E] OPEN"), Pt.X - 22.f * Scale, Pt.Y - 23.f * Scale,
                 GoldColor, 0.76f);
    }

    for (const FAVChest& Chest : Game.Chests)
    {
        if (!Game.IsVisibleAt(Chest.Pos)) continue;
        Sprite(Chest.bOpened ? 14 : 13, Chest.Pos, 1.17f);
        if (!Chest.bOpened)
        {
            const FVector2D Pt = Screen(Chest.Pos);
            Rect(Pt.X - 7.f * Scale, Pt.Y - 17.f * Scale,
                 14.f * Scale, 2.f * Scale,
                 FLinearColor(0.7f, 0.52f, 0.22f, 0.5f + Pulse * 0.45f));
        }
    }

    for (const FAVTrap& Trap : Game.Traps)
    {
        if (!Game.IsVisibleAt(Trap.Pos)) continue;
        const FVector2D Pt = Screen(Trap.Pos);
        const bool Armed = Trap.Windup > 0.f;
        const FLinearColor TrapColor = Armed ?
            (FMath::Sin(Game.RunTime * 25.f) > 0.f ?
                FLinearColor(0.96f, 0.29f, 0.20f, 1.f) : GoldColor) :
            (Trap.Cooldown > 0.f ?
                FLinearColor(0.25f, 0.27f, 0.31f, 0.6f) :
                FLinearColor(0.38f, 0.14f, 0.17f, 0.85f));
        Rect(Pt.X - 8.f * Scale, Pt.Y - 8.f * Scale,
             16.f * Scale, 16.f * Scale, Ink);
        if (Trap.Type == 0) // spike teeth
        {
            for (int32 I = -1; I <= 1; ++I)
                Rect(Pt.X + (I * 5.f - 1.f) * Scale, Pt.Y - 5.f * Scale,
                     3.f * Scale, 11.f * Scale, TrapColor);
        }
        else if (Trap.Type == 1) // flaming vent
        {
            Rect(Pt.X - 6.f * Scale, Pt.Y - 6.f * Scale,
                 12.f * Scale, 12.f * Scale, FLinearColor(0.46f, 0.22f, 0.1f, 1.f));
            Rect(Pt.X - 3.f * Scale, Pt.Y - 5.f * Scale,
                 6.f * Scale, 10.f * Scale, TrapColor);
        }
        else if (Trap.Type == 2) // dart mechanism
        {
            Rect(Pt.X - 6.f * Scale, Pt.Y - 3.f * Scale,
                 12.f * Scale, 6.f * Scale, TrapColor);
            Rect(Pt.X - 2.f * Scale, Pt.Y - 7.f * Scale,
                 4.f * Scale, 14.f * Scale, FLinearColor(0.67f, 0.7f, 0.74f, 1.f));
        }
        else if (Trap.Type == 3) // collapsing stone press
        {
            Rect(Pt.X - 10.f * Scale, Pt.Y - 10.f * Scale,
                 20.f * Scale, 20.f * Scale, TrapColor);
            Rect(Pt.X - 6.f * Scale, Pt.Y - 6.f * Scale,
                 12.f * Scale, 12.f * Scale, Ink);
        }
        else if (Trap.Type == 4) // swinging axe
        {
            Rect(Pt.X - 2.f * Scale, Pt.Y - 11.f * Scale, 4.f * Scale, 22.f * Scale, Metal);
            Rect(Pt.X - 10.f * Scale, Pt.Y + 5.f * Scale, 20.f * Scale, 6.f * Scale, TrapColor);
        }
        else if (Trap.Type == 5) // collapsing flagstone
        {
            Rect(Pt.X - 10.f * Scale, Pt.Y - 9.f * Scale, 20.f * Scale, 18.f * Scale, TrapColor);
            Rect(Pt.X - 2.f * Scale, Pt.Y - 7.f * Scale, 3.f * Scale, 14.f * Scale, Ink);
            Rect(Pt.X - 7.f * Scale, Pt.Y + 1.f * Scale, 14.f * Scale, 2.f * Scale, Ink);
        }
        else if (Trap.Type == 6) // ceiling spear launcher
        {
            for (int32 K=-1; K<=1; ++K)
                Rect(Pt.X + (K*6.f-1.f)*Scale,Pt.Y-9.f*Scale,
                     3.f*Scale,18.f*Scale,TrapColor);
        }
        if (Armed)
        {
            Rect(Pt.X - 14.f * Scale, Pt.Y - 14.f * Scale,
                 28.f * Scale, 2.f * Scale, TrapColor);
            Rect(Pt.X - 14.f * Scale, Pt.Y + 12.f * Scale,
                 28.f * Scale, 2.f * Scale, TrapColor);
        }
    }

    for (const FAVPickup& P : Game.Pickups)
    {
        if (!Game.IsVisibleAt(P.Pos)) continue;
        const FVector2D Pt = P.Pos + FVector2D(0.f, -0.11f * FMath::Sin(P.Time * 5.2f));
        Sprite(P.Type == 0 ? 5 : P.Type == 1 ? 6 : P.Type == 2 ? 7 :
               P.Type == 3 ? 8 : P.Type >= 8 && P.Type <= 10 ? 96 + P.Type : P.Type == 11 ? 180 : 12,
               Pt, P.Type >= 8 ? 0.9f : P.Type == 4 ? 0.9f : 0.7f);
    }
    for(const FAVBolt& Bolt:Game.Bolts)
    {
        if(!Game.IsVisibleAt(Bolt.Pos))continue;
        if(Bolt.bFriendly && Bolt.Visual!=0)
            SpriteRotated(Bolt.Visual==4?181:176,Bolt.Pos,0.72f,
                FMath::Atan2(Bolt.Velocity.Y,Bolt.Velocity.X));
        else
            Sprite(Bolt.bFriendly?15:10,Bolt.Pos,
                   Bolt.bFriendly?0.7f:0.55f);
    }

    for (const FAVEnemy& E : Game.Enemies)
    {
        if (!Game.IsVisibleAt(E.Pos)) continue;
        const FVector2D Pt = Screen(E.Pos);
        const bool bBoss = E.Type == 3 || E.Type == 8 || E.Type == 9 || E.Type == 56 || E.bMiniBoss;
        const float Radius = bBoss ? 1.65f : (E.Type == 7 || E.Type == 23 || E.Type == 24) ? 1.3f : 1.f;
        Rect(Pt.X - 9.f * Scale * Radius, Pt.Y + 7.f * Scale * Radius,
             18.f * Scale * Radius, 3.f * Scale,
             FLinearColor(0.f, 0.f, 0.f, 0.34f));
        if (E.bElite || E.bMiniBoss)
        {
            Rect(Pt.X - 11.f * Scale, Pt.Y - 15.f * Scale, 22.f * Scale, 2.f * Scale, GoldColor);
            Rect(Pt.X - 13.f * Scale, Pt.Y - 13.f * Scale, 2.f * Scale, 22.f * Scale, GoldColor);
        }
        // Telegraphs belong to the creature silhouette, not UI progress bars.
        // A crouch, pulse and raised casting hands are drawn in the actor space.
        const bool bCasting = E.AttackWindup > 0.f || E.Telegraph > 0.f;
        const bool bTransformed = bBoss && E.HP < E.MaxHP * 0.45f;
        const float CastPulse = 0.5f + 0.5f * FMath::Sin(Game.RunTime * 24.f);
        if (bCasting)
        {
            const FLinearColor CastColor = bTransformed ?
                FLinearColor(1.f, 0.12f, 0.10f, 0.9f) :
                FLinearColor(0.95f, 0.55f, 0.18f, 0.85f);
            const float Lift = (4.f + 3.f * CastPulse) * Scale * Radius;
            Rect(Pt.X - 12.f * Scale * Radius, Pt.Y - Lift,
                 5.f * Scale * Radius, 7.f * Scale * Radius, CastColor);
            Rect(Pt.X + 7.f * Scale * Radius, Pt.Y - Lift,
                 5.f * Scale * Radius, 7.f * Scale * Radius, CastColor);
        }
        const int32 SpriteType=E.Type>=34 ?
            (E.Subclass!=0 && E.Type<=51 ? 182 : 128) +
            (E.Type-34)*2 + (FMath::FloorToInt(Game.RunTime*6.f+E.Id)%2) :
            E.Type >= 11 ?
            ((FMath::FloorToInt(Game.RunTime * (E.Type == 15 ? 10.f : 6.f) + E.Id) % 2) == 0 ?
             36 + E.Type : 70 + E.Type) : E.Type == 0 ? 1 : E.Type == 1 ? 2 :
            E.Type == 2 ? 3 : E.Type == 3 ? 4 : E.Type == 4 ? 16 :
            E.Type == 5 ? 17 : E.Type == 6 ? 18 : E.Type == 7 ? 19 :
            E.Type == 8 ? 20 : E.Type == 9 ? 21 : 22;
        const float EnemyBob = FMath::Sin(Game.RunTime *
            (E.Type == 5 ? 15.f : 7.f) + E.Id * 0.7f) *
            (E.Type == 10 ? 0.12f : 0.045f);
        const bool bGuardian=E.Type==3 || E.Type==8 || E.Type==9 || E.Type==56;
        if(bGuardian && BossAtlas)
        {
            const int32 Row=E.Type==8?0:E.Type==9?1:E.Type==3?2:3;
            const int32 Frame=E.HP<E.MaxHP*0.45f?
                4+(FMath::FloorToInt(Game.RunTime*4.f)%2):
                (bCasting?2+(FMath::FloorToInt(Game.RunTime*12.f)%2):
                            FMath::FloorToInt(Game.RunTime*2.f)%2);
            const FVector2D BP=Screen(E.Pos+FVector2D(0.f,EnemyBob));
            const float Size=75.f*Scale;
            const FLinearColor Aura=E.Type==8?
                FLinearColor(0.67f,0.32f,0.71f,0.07f):
                E.Type==9?FLinearColor(1.f,0.36f,0.13f,0.07f):
                E.Type==56?FLinearColor(0.92f,0.13f,0.23f,0.07f):
                FLinearColor(0.30f,0.81f,0.55f,0.07f);
            if(Game.VisualQuality>=1)
                for(int32 Halo=3;Halo>0;--Halo) {
                    const float H=Size*(1.f+Halo*0.24f);
                    Rect(BP.X-H*0.5f,BP.Y-H*0.55f,H,H*0.83f,
                         FLinearColor(Aura.R,Aura.G,Aura.B,Aura.A/Halo));
                }
            DrawTexture(BossAtlas,BP.X-Size*0.5f,BP.Y-Size*0.54f,
                        Size,Size,Frame/6.f,Row/4.f,1.f/6.f,1.f/4.f,
                        FLinearColor(1.f,1.f,1.f,E.Flash>0.f?0.65f:1.f),
                        BLEND_Translucent,1.f,false,0.f,
                        FVector2D::ZeroVector);
        }
        else if(EnemyAtlas && E.Type>=0 && E.Type<=56)
        {
            const bool Moving=E.MemoryTime>0.f || E.AlertTime>0.f ||
                              E.Wander>0.f;
            const int32 Frame=bCasting?1:(Moving?
                FMath::FloorToInt(Game.RunTime*7.f+E.Id)%2:0);
            const int32 Index=E.Type*2+Frame;
            const float Size=29.f*Scale*Radius;
            const FVector2D PtSprite=Screen(E.Pos+FVector2D(0.f,EnemyBob));
            DrawTexture(EnemyAtlas,PtSprite.X-Size*0.5f,
                        PtSprite.Y-Size*0.57f,Size,Size,
                        (Index%12)/12.f,(Index/12)/10.f,
                        1.f/12.f,1.f/10.f,
                        FLinearColor(1.f,1.f,1.f,E.Flash>0.f?0.65f:1.f),
                        BLEND_Translucent,1.f,false,0.f,
                        FVector2D::ZeroVector);
        }
        else
            Sprite(SpriteType,E.Pos+FVector2D(0.f,EnemyBob),
                   Radius,E.Flash>0.f?0.55f:1.f);
        if(Game.CurrentBiome()==3 && E.Type>=34 && E.Type<=56 &&
           (E.bMiniBoss || E.Type==56 ||
            FVector2D::Distance(E.Pos,Game.Hero)<2.4f)) {
            const FString Label(UTF8_TO_TCHAR(
                FAVContent::BastionVariantName(E.Type,E.Subclass)));
            Text(Label,Pt.X-40.f*Scale,Pt.Y-27.f*Scale*Radius,
                 E.bMiniBoss || E.Type==56 ? GoldColor : Pale,0.71f);
        }
        // Secondary subclass mark is visually distinct and does not affect the enemy silhouette.
        if (E.Type >= 34 && E.Type <= 51 && E.Subclass != 0)
            Rect(Pt.X - 2.f * Scale, Pt.Y - 14.f * Scale, 4.f * Scale, 3.f * Scale, GoldColor);
        if (E.PhaseShiftTime > 0.f)
        {
            const float TransformPulse = 0.4f + 0.6f * FMath::Sin(Game.RunTime * 28.f);
            const float HaloRadius = (18.f + 13.f * E.PhaseShiftTime / 1.5f) * Scale;
            Rect(Pt.X - HaloRadius, Pt.Y - HaloRadius,
                 HaloRadius * 2.f, 3.f * Scale,
                 FLinearColor(0.9f, 0.08f, 0.12f, TransformPulse));
            Rect(Pt.X - HaloRadius, Pt.Y + HaloRadius,
                 HaloRadius * 2.f, 3.f * Scale,
                 FLinearColor(0.9f, 0.08f, 0.12f, TransformPulse));
        }
        if (bTransformed)
        {
            // Visible phase mutation: glowing red eyes and additional limbs.
            const FLinearColor Eyes(1.f, 0.08f + 0.12f * CastPulse, 0.04f, 1.f);
            Rect(Pt.X - 6.f * Scale, Pt.Y - 5.f * Scale, 3.f * Scale, 3.f * Scale, Eyes);
            Rect(Pt.X + 3.f * Scale, Pt.Y - 5.f * Scale, 3.f * Scale, 3.f * Scale, Eyes);
            Rect(Pt.X - 18.f * Scale, Pt.Y + 1.f * Scale, 5.f * Scale, 12.f * Scale, Blood);
            Rect(Pt.X + 13.f * Scale, Pt.Y + 1.f * Scale, 5.f * Scale, 12.f * Scale, Blood);
        }
        // Readable placeholders until dedicated Paper2D sprites are authored.
        const FLinearColor Marker = E.Faction == 1 ? FLinearColor(0.9f, 0.25f, 0.22f, 1.f) :
            E.Type == 23 ? FLinearColor(0.48f, 0.51f, 0.56f, 1.f) :
            E.Type == 24 ? FLinearColor(0.75f, 0.72f, 0.62f, 1.f) :
            E.Type >= 26 ? FLinearColor(0.88f, 0.43f, 0.14f, 1.f) :
            FLinearColor(0.33f, 0.75f, 0.65f, 1.f);
        if (E.Type >= 15)
            Rect(Pt.X - 8.f * Scale, Pt.Y - 13.f * Scale,
                 16.f * Scale, 2.f * Scale, Marker);
        if (E.Equipment == 1)
        {
            Rect(Pt.X - 6.f * Scale, Pt.Y - 1.f * Scale,
                 12.f * Scale, 8.f * Scale, FLinearColor(0.52f, 0.57f, 0.65f, 0.85f));
            Rect(Pt.X - 2.f * Scale, Pt.Y + 1.f * Scale,
                 4.f * Scale, 3.f * Scale, FLinearColor(0.82f, 0.86f, 0.88f, 1.f));
        }
        else if (E.Equipment == 2)
        {
            Rect(Pt.X + 8.f * Scale, Pt.Y - 9.f * Scale,
                 3.f * Scale, 15.f * Scale, FLinearColor(0.69f, 0.42f, 0.21f, 1.f));
            Rect(Pt.X + 11.f * Scale, Pt.Y - 7.f * Scale,
                 1.f * Scale, 11.f * Scale, Pale);
        }
        else if (E.Equipment == 3)
            Rect(Pt.X + 8.f * Scale, Pt.Y - 10.f * Scale,
                 4.f * Scale, 14.f * Scale, GoldColor);
        Text(FString::Printf(TEXT("Lv.%d"), E.Level),
             Pt.X - 12.f * Scale, Pt.Y - (bBoss ? 37.f : 29.f) * Scale, Pale, 0.68f);
        if (E.HP < E.MaxHP || bBoss || E.bElite)
        {
            const float W = (bBoss ? 45.f : 23.f) * Scale;
            Rect(Pt.X - W * 0.5f, Pt.Y - (bBoss ? 25.f : 19.f) * Scale, W, 3.f * Scale, Ink);
            Rect(Pt.X - W * 0.5f, Pt.Y - (bBoss ? 25.f : 19.f) * Scale,
                 W * FMath::Clamp(E.HP / E.MaxHP, 0.f, 1.f), 2.f * Scale, Blood);
        }
    }
    // Soft dark shadow makes the hero readable on top of floor tiles.
    const FVector2D Hp = Screen(Game.Hero);
    Rect(Hp.X - 10.f * Scale, Hp.Y + 9.f * Scale, 20.f * Scale, 3.f * Scale,
         FLinearColor(0.f, 0.f, 0.f, 0.5f));
    if (Game.DashTime > 0.f)
    {
        const FVector2D Direction = Game.HeroVelocity.GetSafeNormal();
        for (int32 Trail = 1; Trail <= 3; ++Trail)
            Sprite(0, Game.Hero - Direction * (0.22f * Trail),
                   1.12f, 0.30f / Trail);
    }
    const bool bWalking = Game.HeroVelocity.Size() > 0.2f;
    const int32 WalkFrame = Game.HeroClass == 3 ?
        (bWalking && (FMath::FloorToInt(Game.RunTime * 8.f) % 2 != 0) ? 175 : 174) :
        (bWalking && (FMath::FloorToInt(Game.RunTime * 8.f) % 2 != 0) ? 35 : 0);
    const float WalkBob = bWalking ? FMath::Sin(Game.RunTime * 16.f) * 0.045f : 0.f;
    const float HeroAlpha=Game.Invulnerability>0.f &&
        FMath::Sin(Game.RunTime*32.f)>0.f?0.5f:1.f;
    if(HeroAtlas)
    {
        const bool Attacking=Game.AttackCooldown>0.07f ||
            Game.SpinCooldown>1.25f ||
            (Game.HeroClass==3 && Game.KnifeCooldown>1.65f);
        const int32 Frame=Game.DashTime>0.f?6+FMath::FloorToInt(Game.RunTime*14.f)%2:
            Attacking?4+FMath::FloorToInt(Game.RunTime*12.f)%2:
            bWalking?2+FMath::FloorToInt(Game.RunTime*9.f)%2:
            FMath::FloorToInt(Game.RunTime*2.f)%2;
        const FVector2D Position=Screen(Game.Hero+FVector2D(0.f,WalkBob));
        const float Size=37.f*Scale;
        DrawTexture(HeroAtlas,Position.X-Size*0.5f,Position.Y-Size*0.56f,
                    Size,Size,Frame/8.f,Game.HeroClass/4.f,
                    1.f/8.f,1.f/4.f,
                    FLinearColor(1.f,1.f,1.f,HeroAlpha),
                    BLEND_Translucent,1.f,false,0.f,FVector2D::ZeroVector);
    }
    else Sprite(WalkFrame,Game.Hero+FVector2D(0.f,WalkBob),1.17f,HeroAlpha);

    const FVector2D Arrow = Hp + Game.Aim * (18.f * Scale);
    Rect(Arrow.X - Scale, Arrow.Y - Scale, 3.f * Scale, 3.f * Scale, GoldColor);
    for (const FAVEffect& E : Game.Effects)
    {
        if (!Game.IsVisibleAt(E.Pos)) continue;
        const FVector2D Pt = Screen(E.Pos);
        const float Progress = 1.f - E.Life / FMath::Max(0.001f, E.Duration);
        if (E.Kind == 0 || E.Kind == 1)
        {
            const int32 Segments = E.Kind == 1 ? 24 : 11;
            const float Radius = (E.Radius * 0.72f + Progress * 0.35f) * T;
            const float Start = E.Kind == 1 ? 0.f : E.Angle - 1.28f;
            const float Sweep = E.Kind == 1 ? 6.2831853f : 2.56f;
            for (int32 I = 0; I < Segments; ++I)
            {
                const float A = Start + Sweep * I / (Segments - 1.f);
                const FVector2D Pixel = Pt + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Radius;
                Rect(Pixel.X, Pixel.Y, 4.f * Scale, 4.f * Scale,
                     E.Kind == 1 ? FLinearColor(0.34f, 0.86f, 0.83f, 1.f - Progress) :
                                   FLinearColor(0.92f, 0.84f, 0.61f, 1.f - Progress));
            }
        }
        else
        {
            const FLinearColor Color = E.Kind == 2 ? Blood : GoldColor;
            const float Radius = E.Radius * T * (0.3f + Progress);
            const int32 Shards = Game.VisualQuality == 2 ? 14 : 8;
            for (int32 I = 0; I < Shards; ++I)
            {
                const float A = I * 6.283185f / Shards + E.Angle;
                const FVector2D Direction(FMath::Cos(A), FMath::Sin(A));
                const FVector2D Pixel = Pt + Direction * Radius;
                const float Particle = (I % 4 == 0 ? 4.f : 2.f) * Scale;
                Rect(Pixel.X, Pixel.Y, Particle, Particle,
                     FLinearColor(Color.R, Color.G, Color.B, 1.f - Progress));
                if (Game.VisualQuality == 2)
                    Rect(Pixel.X - Direction.X * 5.f * Scale,
                         Pixel.Y - Direction.Y * 5.f * Scale,
                         2.f * Scale, 2.f * Scale,
                         FLinearColor(Color.R, Color.G, Color.B,
                                      0.45f * (1.f - Progress)));
            }
        }
    }

    if (Game.bMerchantSpawned && FVector2D::Distance(Game.Hero, Game.Merchant) < 2.0f)
        Text(TEXT("E  VISIT THE MERCHANT"),
             Hp.X - 77.f * Scale, Hp.Y - 49.f * Scale, GoldColor, 0.85f);
    else if (!Game.bShrineUsed && FVector2D::Distance(Game.Hero, Game.Shrine) < 2.1f)
        Text(TEXT("E  PRAY  +HP / +DAMAGE"),
             Hp.X - 69.f * Scale, Hp.Y - 49.f * Scale, Mint, 0.88f);
    else if ([&Game]()
    {
        for (const FAVChest& Chest : Game.Chests)
            if (!Chest.bOpened && FVector2D::Distance(Game.Hero, Chest.Pos) < 1.85f)
                return true;
        return false;
    }())
        Text(TEXT("E  OPEN TREASURE"), Hp.X - 61.f * Scale,
             Hp.Y - 49.f * Scale, GoldColor, 0.88f);
    else if ([&Game]()
    {
        for (const FAVDecor& Decor : Game.Decorations)
            if (!Decor.bActivated && Decor.Type < 12 && Decor.Type % 3 == 2 &&
                FVector2D::Distance(Game.Hero, Decor.Pos) < 2.1f)
                return true;
        return false;
    }())
        Text(TEXT("E  RECLAIM AN ECHO"),
             Hp.X - 69.f * Scale, Hp.Y - 49.f * Scale, Aether, 0.88f);
    else if (Game.IsExitReady() && FVector2D::Distance(Game.Hero, Game.Exit) < 2.1f)
        Text(TEXT("E  ENTER THE DEPTHS"),
             Hp.X - 57.f * Scale, Hp.Y - 49.f * Scale, Mint, 0.88f);
}

void AAVHUD::Bar(float X, float Y, float W, float H, float Fraction,
                 const FLinearColor& Fill, const FString& Label)
{
    Rect(X, Y, W, H, FLinearColor(0.02f, 0.03f, 0.05f, 0.95f));
    Rect(X + 2.f * Scale, Y + 2.f * Scale,
         (W - 4.f * Scale) * FMath::Clamp(Fraction, 0.f, 1.f),
         H - 4.f * Scale, Fill);
    Text(Label, X + 5.f * Scale, Y + 1.f * Scale, Pale, 0.83f);
}

void AAVHUD::DrawMiniMap(const AAVGameMode& Game)
{
    const float Unit = 2.65f * Scale;
    const float X0 = Canvas->SizeX - (Game.MapW * Unit + 21.f * Scale);
    const float Y0 = 48.f * Scale;
    const float W = Game.MapW * Unit;
    const float H = Game.MapH * Unit;
    Rect(X0 - 7.f * Scale, Y0 - 17.f * Scale, W + 14.f * Scale, H + 23.f * Scale,
         FLinearColor(0.025f, 0.034f, 0.055f, 0.9f));
    Text(TEXT("THE VAULT"), X0, Y0 - 16.f * Scale, Pale, 0.85f);
    for (int32 Y = 0; Y < Game.MapH; ++Y)
        for (int32 X = 0; X < Game.MapW; ++X)
            if (Game.IsExploredCell(X, Y))
                Rect(X0 + X * Unit, Y0 + Y * Unit, Unit * 0.87f, Unit * 0.87f,
                     Game.IsVisibleCell(X, Y) ?
                         FLinearColor(0.26f, 0.34f, 0.37f, 1.f) :
                         FLinearColor(0.13f, 0.16f, 0.19f, 1.f));
    if (Game.bMerchantSpawned && Game.IsExploredCell(
            FMath::FloorToInt(Game.Merchant.X), FMath::FloorToInt(Game.Merchant.Y)))
        Rect(X0 + Game.Merchant.X * Unit - Unit * 0.5f,
             Y0 + Game.Merchant.Y * Unit - Unit * 0.5f,
             4.f * Scale, 4.f * Scale, GoldColor);
    if (!Game.bShrineUsed && Game.IsExploredCell(
            FMath::FloorToInt(Game.Shrine.X), FMath::FloorToInt(Game.Shrine.Y)))
        Rect(X0 + Game.Shrine.X * Unit - Unit * 0.5f,
             Y0 + Game.Shrine.Y * Unit - Unit * 0.5f,
             3.f * Scale, 3.f * Scale, Mint);
    if (Game.IsExploredCell(FMath::FloorToInt(Game.Exit.X),
                            FMath::FloorToInt(Game.Exit.Y)))
        Rect(X0 + Game.Exit.X * Unit - Unit * 0.5f,
             Y0 + Game.Exit.Y * Unit - Unit * 0.5f,
             3.f * Scale, 3.f * Scale, Game.IsExitReady() ? Mint : Blood);
    for (const FAVDecor& Decor : Game.Decorations)
        if (!Decor.bActivated && Decor.Type % 3 == 2 &&
            Game.IsExploredCell(FMath::FloorToInt(Decor.Pos.X),
                                FMath::FloorToInt(Decor.Pos.Y)))
            Rect(X0 + Decor.Pos.X * Unit - Unit * 0.5f,
                 Y0 + Decor.Pos.Y * Unit - Unit * 0.5f,
                 3.f * Scale, 3.f * Scale, Aether);
    for (const FAVChest& Chest : Game.Chests)
        if (!Chest.bOpened && Game.IsExploredCell(FMath::FloorToInt(Chest.Pos.X),
                                                  FMath::FloorToInt(Chest.Pos.Y)))
            Rect(X0 + Chest.Pos.X * Unit - Unit * 0.5f,
                 Y0 + Chest.Pos.Y * Unit - Unit * 0.5f,
                 3.f * Scale, 3.f * Scale, GoldColor);
    for (const FAVEnemy& E : Game.Enemies)
        if (Game.IsVisibleAt(E.Pos))
            Rect(X0 + E.Pos.X * Unit - Unit * 0.5f,
                 Y0 + E.Pos.Y * Unit - Unit * 0.5f,
                 2.f * Scale, 2.f * Scale, Blood);
    Rect(X0 + Game.Hero.X * Unit - Unit * 0.5f, Y0 + Game.Hero.Y * Unit - Unit * 0.5f,
         4.f * Scale, 4.f * Scale, GoldColor);
    Text(FString::Printf(TEXT("SEED %d"), Game.RunSeed),
         X0, Y0 + H + 7.f * Scale, Pale, 0.72f);
}

void AAVHUD::DrawInterface(const AAVGameMode& Game)
{
    const float P = 16.f * Scale;
    Rect(0.f, 0.f, Canvas->SizeX, 37.f * Scale, FLinearColor(0.018f, 0.024f, 0.041f, 0.94f));
    Text(TEXT("ASHEN VAULTS"), P, 10.f * Scale, GoldColor, 1.2f);
    const TCHAR* BiomeName = Game.CurrentBiome() == 0 ? TEXT("ASHEN CRYPT") :
                             Game.CurrentBiome() == 1 ? TEXT("SPORE HOLLOWS") :
                             Game.CurrentBiome() == 2 ? TEXT("EMBER FOUNDRY") :
                                                        TEXT("BLACK BASTION");
    Text(FString::Printf(TEXT("DEPTH %d  |  %s %d/5  |  LVL %d  |  KILLS %d  |  GOLD %d"),
        Game.Floor, BiomeName, Game.BiomeDepth(), Game.Level, Game.Kills, Game.Gold),
        214.f * Scale, 10.f * Scale, Pale, 0.86f);

    if (Game.bInSafeCamp)
        Text(TEXT("PEACEFUL CAMP"), Canvas->SizeX * 0.70f, 10.f * Scale, Mint, 0.95f);
    else if (Game.bInLobby)
        Text(TEXT("HOME / LOBBY"), Canvas->SizeX * 0.70f, 10.f * Scale, Mint, 0.95f);
    else if (Game.bInBossArena)
        Text(TEXT("GUARDIAN ARENA"), Canvas->SizeX * 0.70f, 10.f * Scale, Blood, 0.95f);

    const float Y = Canvas->SizeY - 74.f * Scale;
    Rect(0.f, Y, Canvas->SizeX, 74.f * Scale, FLinearColor(0.018f, 0.025f, 0.041f, 0.94f));
    Bar(P, Y + 10.f * Scale, 225.f * Scale, 17.f * Scale, Game.HP / Game.MaxHP, Blood,
        FString::Printf(TEXT("HP  %d / %d"), FMath::CeilToInt(Game.HP), FMath::CeilToInt(Game.MaxHP)));
    Bar(P, Y + 32.f * Scale, 225.f * Scale, 13.f * Scale, Game.Stamina / Game.MaxStamina, Mint,
        FString::Printf(TEXT("STAMINA  %d / %d"), FMath::CeilToInt(Game.Stamina), FMath::CeilToInt(Game.MaxStamina)));
    Bar(P, Y + 51.f * Scale, 225.f * Scale, 10.f * Scale,
        (float)Game.XP / FMath::Max(1, Game.NextXP()), Aether,
        FString::Printf(TEXT("XP  %d / %d"), Game.XP, Game.NextXP()));

    Text(Game.HeroClass == 3 ? TEXT("LMB   SHOOT") : TEXT("LMB   SLASH"), 280.f * Scale, (Y / Scale + 11.f) * Scale, Pale, 0.95f);
    Text(Game.HeroClass == 3 ? TEXT("Q / RMB  VOLLEY") : TEXT("Q / RMB   SPIN"), 280.f * Scale, Y + 34.f * Scale,
         Game.Stamina >= (Game.HasRelic(3) ? 24.f : Game.HeroClass == 3 ? 30.f : 32.f) &&
         Game.SpinCooldown <= 0.f ? Mint : Pale, 0.95f);
    Text(Game.HeroClass == 3 ? TEXT("R   PIERCE") : TEXT("R   THROW BLADE"), 447.f * Scale, Y + 11.f * Scale,
         Game.KnifeCooldown <= 0.f && Game.Stamina >= (Game.HeroClass == 3 ? 20.f : 18.f) ? Mint : Pale, 0.95f);
    Text(FString::Printf(TEXT("F   POTION x%d"), Game.Potions),
         447.f * Scale, Y + 34.f * Scale, Game.Potions > 0 ? GoldColor : Pale, 0.95f);
    Text(Game.IsExitReady() ? TEXT("E   EXIT READY") :
         FString::Printf(TEXT("FOES %d  |  CACHE %d"),
             Game.EnemiesRemaining(), Game.UnopenedChests()),
         646.f * Scale, Y + 11.f * Scale, Game.IsExitReady() ? Mint : Blood, 0.95f);
    Text(Game.HeroClass == 3 ?
         FString::Printf(TEXT("RELICS %d  AIM x%d"), Game.RelicCount, Game.ShotSeries) :
         FString::Printf(TEXT("RELICS %d   ECHOES %d"), Game.RelicCount, Game.Echoes),
         646.f * Scale, Y + 34.f * Scale, Pale, 0.83f);
    Text(FString::Printf(TEXT("ARMOR %.0f%%  GEAR %d  SKILLS %d"),
         Game.ArmorRating * 100.f,Game.InventoryItems.Num(),Game.SkillPoints),
         280.f * Scale, Y + 53.f * Scale, GoldColor, 0.73f);
    Text(TEXT("I  GEAR   K  SKILLS"),645.f*Scale,Y+53.f*Scale,Mint,0.73f);
    Text(FString::Printf(TEXT("F3  FX %s"), Game.VisualQuality == 2 ?
         TEXT("HIGH") : Game.VisualQuality == 1 ? TEXT("MED") : TEXT("LOW")),
         816.f * Scale, Y + 53.f * Scale, Aether, 0.73f);

    if (Game.PoisonTime > 0.f)
        Text(FString::Printf(TEXT("POISON  %.0f"), Game.PoisonTime),
             16.f * Scale, 53.f * Scale, Mint, 0.96f);
    if (Game.BurnTime > 0.f)
        Text(FString::Printf(TEXT("BURN  %.0f"), Game.BurnTime),
             16.f * Scale, (Game.PoisonTime > 0.f ? 72.f : 53.f) * Scale, GoldColor, 0.96f);

    // Show the active floor guardian even when it moves behind fog.
    for (const FAVEnemy& Boss : Game.Enemies)
    {
        if (Boss.Type != 3 && Boss.Type != 8 && Boss.Type != 9 && Boss.Type != 56) continue;
        const TCHAR* Name = Boss.Type == 8 ? TEXT("THE BONE REGENT") :
                            Boss.Type == 9 ? TEXT("FORGE TYRANT") :
                            Boss.Type == 56 ? TEXT("THE COMMANDANT") :
                                             TEXT("THE VAULT WARDEN");
        const float X = Canvas->SizeX * 0.5f - 195.f * Scale;
        const float BossY = Canvas->SizeY - 110.f * Scale;
        Rect(X, BossY, 390.f * Scale, 21.f * Scale,
             FLinearColor(0.035f, 0.04f, 0.065f, 0.94f));
        Rect(X + 3.f * Scale, BossY + 13.f * Scale,
             384.f * Scale * FMath::Clamp(Boss.HP / Boss.MaxHP, 0.f, 1.f),
             5.f * Scale, Blood);
        Text(Name, X + 108.f * Scale, BossY + 1.f * Scale, GoldColor, 0.82f);
        break;
    }
    if (Game.Combo >= 2)
        Text(FString::Printf(TEXT("COMBO x%d"), Game.Combo),
             Canvas->SizeX * 0.5f - 46.f * Scale, 53.f * Scale, GoldColor, 1.4f);
    if (Game.ToastTime > 0.f)
        Text(Game.Toast, Canvas->SizeX * 0.5f - 123.f * Scale,
             92.f * Scale, Mint, 1.02f);
    DrawMiniMap(Game);
}

void AAVHUD::DrawOverlay(const AAVGameMode& Game)
{
    if(Game.Phase==EAVPhase::Playing) return;
    if(Game.Phase==EAVPhase::Inventory){DrawInventoryWindow(Game);return;}
    if(Game.Phase==EAVPhase::Skills){DrawTalentWindow(Game);return;}
    Rect(0.f,0.f,Canvas->SizeX,Canvas->SizeY,
         FLinearColor(0.01f,0.015f,0.025f,0.84f));
    if(Game.Phase == EAVPhase::Inventory || Game.Phase == EAVPhase::Skills)
    {
        const float Left=Canvas->SizeX*0.5f-370.f*Scale;
        const float Top=Canvas->SizeY*0.5f-207.f*Scale;
        Rect(Left,Top,740.f*Scale,414.f*Scale,
             FLinearColor(0.065f,0.078f,0.11f,0.98f));
        Rect(Left,Top,740.f*Scale,4.f*Scale,GoldColor);
        if(Game.Phase == EAVPhase::Inventory)
        {
            const TCHAR* Slots[8]={
                TEXT("1 WEAPON"),TEXT("2 HEAD"),TEXT("3 CHEST"),
                TEXT("4 HANDS"),TEXT("5 LEGS"),TEXT("6 BOOTS"),
                TEXT("7 TRINKET"),TEXT("8 TRINKET")
            };
            Text(TEXT("EQUIPMENT"),Left+28.f*Scale,Top+20.f*Scale,GoldColor,1.2f);
            Text(FString::Printf(TEXT("BAG %d / 48"),Game.InventoryItems.Num()),
                 Left+420.f*Scale,Top+20.f*Scale,Mint,1.0f);
            for(int32 Slot=0;Slot<8;++Slot)
            {
                const int32 Id=Game.EquipmentSlots.IsValidIndex(Slot)?
                    Game.EquipmentSlots[Slot]:-1;
                const float Y=Top+(61.f+Slot*34.f)*Scale;
                Rect(Left+24.f*Scale,Y-2.f*Scale,318.f*Scale,29.f*Scale,
                     FLinearColor(0.10f,0.12f,0.16f,0.90f));
                Text(Slots[Slot],Left+31.f*Scale,Y+3.f*Scale,Aether,0.80f);
                Text(Game.GearName(Id).Left(24),Left+112.f*Scale,
                     Y+3.f*Scale,Id<0?Pale:GoldColor,0.74f);
            }
            const int32 Visible=9;
            const int32 First=FMath::Max(0,Game.GearSelection-Visible/2);
            for(int32 J=0;J<Visible && J+First<Game.InventoryItems.Num();++J)
            {
                const int32 Index=J+First;
                const int32 ItemId=Game.InventoryItems[Index];
                const FAVEquipmentSpec* S=FAVEquipment::Find(ItemId);
                if(!S)continue;
                const bool Selected=Index==Game.GearSelection;
                const float Y=Top+(59.f+J*32.f)*Scale;
                Rect(Left+374.f*Scale,Y,344.f*Scale,29.f*Scale,
                     Selected?FLinearColor(0.18f,0.24f,0.28f,1.f):
                              FLinearColor(0.10f,0.12f,0.16f,0.90f));
                Text(Game.GearName(ItemId).Left(27),Left+382.f*Scale,
                     Y+6.f*Scale,Selected?Mint:Pale,0.76f);
                Text(FString::Printf(TEXT("R%d"),S->Rarity),
                     Left+673.f*Scale,Y+6.f*Scale,GoldColor,0.70f);
            }
            if(Game.InventoryItems.IsValidIndex(Game.GearSelection))
            {
                const FAVEquipmentSpec* S=FAVEquipment::Find(
                    Game.InventoryItems[Game.GearSelection]);
                if(S)
                {
                    Text(FString::Printf(TEXT("%s %.0f  /  %s %.0f"),
                         UTF8_TO_TCHAR(FAVEquipment::StatName(S->StatA)),S->ValueA,
                         UTF8_TO_TCHAR(FAVEquipment::StatName(S->StatB)),S->ValueB),
                         Left+379.f*Scale,Top+349.f*Scale,Pale,0.76f);
                    if(S->PerkId>0)
                        Text(FString(UTF8_TO_TCHAR(FAVEquipment::PerkName(S->PerkId))),
                             Left+379.f*Scale,Top+367.f*Scale,GoldColor,0.76f);
                }
            }
            Text(TEXT("UP/DOWN SELECT   ENTER EQUIP   1-8 UNEQUIP   I/TAB EXIT"),
                 Left+96.f*Scale,Top+390.f*Scale,Mint,0.85f);
        }
        else
        {
            Text(TEXT("CHARACTER TALENTS"),Left+28.f*Scale,Top+18.f*Scale,GoldColor,1.2f);
            Text(FString::Printf(TEXT("LEVEL %d   AVAILABLE POINTS %d"),
                 Game.Level,Game.SkillPoints),Left+359.f*Scale,
                 Top+21.f*Scale,Mint,0.95f);
            const int32 First=FMath::Max(0,Game.SkillSelection-5);
            for(int32 J=0;J<11 && J+First<21;++J)
            {
                const int32 Selection=J+First;
                const int32 Id=Game.SkillIdAt(Selection);
                const FAVSkillSpec* S=FAVSkills::Find(Id);
                if(!S)continue;
                const bool Selected=Selection==Game.SkillSelection;
                FString Name=FString(UTF8_TO_TCHAR(S->CodeName));
                Name.ReplaceInline(TEXT("_"),TEXT(" "));
                Name=Name.ToUpper();
                const float Y=Top+(55.f+J*28.f)*Scale;
                Rect(Left+24.f*Scale,Y,420.f*Scale,26.f*Scale,
                     Selected?FLinearColor(0.19f,0.23f,0.30f,1.f):
                              FLinearColor(0.10f,0.12f,0.16f,0.92f));
                Text(Name.Left(29),Left+32.f*Scale,Y+5.f*Scale,
                     Selected?Mint:Pale,0.87f);
                Text(FString::Printf(TEXT("%d/%d"),Game.SkillRank(Id),S->MaxRank),
                     Left+386.f*Scale,Y+5.f*Scale,
                     Game.SkillRank(Id)>0?GoldColor:Aether,0.82f);
            }
            const int32 Id=Game.SkillIdAt(Game.SkillSelection);
            const FAVSkillSpec* S=FAVSkills::Find(Id);
            if(S)
            {
                Rect(Left+461.f*Scale,Top+55.f*Scale,254.f*Scale,303.f*Scale,
                     FLinearColor(0.09f,0.11f,0.15f,1.f));
                FString Name=FString(UTF8_TO_TCHAR(S->CodeName));
                Name.ReplaceInline(TEXT("_"),TEXT(" "));
                Text(Name.ToUpper().Left(25),Left+472.f*Scale,
                     Top+70.f*Scale,GoldColor,0.94f);
                Text(FString::Printf(TEXT("RANK %d / %d"),Game.SkillRank(Id),S->MaxRank),
                     Left+472.f*Scale,Top+111.f*Scale,Pale,0.9f);
                Text(FString::Printf(TEXT("UNLOCK LEVEL %d"),S->MinLevel),
                     Left+472.f*Scale,Top+145.f*Scale,Aether,0.85f);
                FString Description=FString(UTF8_TO_TCHAR(FAVSkills::Description(Id)));
                Text(Description.Left(28),Left+472.f*Scale,
                     Top+181.f*Scale,Pale,0.79f);
                if(Description.Len()>28)
                    Text(Description.Mid(28,28),Left+472.f*Scale,
                         Top+198.f*Scale,Pale,0.79f);
                Text(S->PrerequisiteId>=0?
                     FString::Printf(TEXT("REQUIRES SKILL %d"),S->PrerequisiteId):
                     TEXT("NO PREREQUISITE"),
                     Left+472.f*Scale,Top+221.f*Scale,Pale,0.8f);
                Text(FAVSkills::CanLearn(Id,Game.HeroClass,Game.Level,
                     Game.LearnedSkills.GetData(),Game.LearnedSkills.Num(),Game.SkillPoints)?
                     TEXT("READY TO LEARN"):TEXT("LOCKED"),
                     Left+472.f*Scale,Top+275.f*Scale,Mint,0.9f);
            }
            Text(TEXT("UP/DOWN NAVIGATE   ENTER LEARN   BACKSPACE RESPEC IN CAMP   K EXIT"),
                 Left+51.f*Scale,Top+389.f*Scale,Mint,0.77f);
        }
        return;
    }
    const float Left = Canvas->SizeX * 0.5f - 260.f * Scale;
    const float Top = Canvas->SizeY * 0.5f - 112.f * Scale;
    Rect(Left, Top, 520.f * Scale, 230.f * Scale, FLinearColor(0.07f, 0.085f, 0.12f, 0.97f));
    Rect(Left, Top, 520.f * Scale, 4.f * Scale, GoldColor);
    if (Game.Phase == EAVPhase::Upgrade)
    {
        Text(TEXT("CHOOSE YOUR BLESSING"), Left + 110.f * Scale, Top + 21.f * Scale, GoldColor, 1.2f);
        Text(TEXT("1  IRON HEART      +25 MAX HP  /  HEAL +40"), Left + 43.f * Scale, Top + 71.f * Scale, Pale, 1.f);
        Text(TEXT("2  ASHEN BLADE     +7 ATTACK DAMAGE"), Left + 43.f * Scale, Top + 113.f * Scale, Pale, 1.f);
        Text(TEXT("3  WINDRUNNER      +SPEED  /  +12 STAMINA"), Left + 43.f * Scale, Top + 155.f * Scale, Pale, 1.f);
        Text(TEXT("PRESS 1, 2 OR 3"), Left + 183.f * Scale, Top + 204.f * Scale, Mint, 0.9f);
    }
    else if (Game.Phase == EAVPhase::Relic)
    {
        static const TCHAR* RelicNames[6] =
        {
            TEXT("BLOODFANG"), TEXT("IRON OATH"), TEXT("STORMGLASS"),
            TEXT("EMBER SIGIL"), TEXT("WIND CHARM"), TEXT("GILDED EYE")
        };
        static const TCHAR* Descriptions[6] =
        {
            TEXT("+3 HP PER KILL  /  +15 MAX HP"),
            TEXT("20 PERCENT LESS DAMAGE"),
            TEXT("STRONGER, FASTER THROW"),
            TEXT("WHIRLWIND BOOST  /  +4 ATTACK"),
            TEXT("CHEAPER DODGE  /  +20 STAMINA"),
            TEXT("50 PERCENT MORE GOLD")
        };
        Text(TEXT("CHOOSE AN ANCIENT RELIC"),
             Left + 105.f * Scale, Top + 20.f * Scale, GoldColor, 1.2f);
        for (int32 I = 0; I < 3; ++I)
        {
            const int32 Index = FMath::Clamp(Game.RelicChoices[I], 0, 5);
            const float Y = Top + (68.f + I * 43.f) * Scale;
            Text(FString::Printf(TEXT("%d  %s"), I + 1, RelicNames[Index]),
                 Left + 38.f * Scale, Y, Mint, 1.f);
            Text(Descriptions[Index], Left + 229.f * Scale, Y + 2.f * Scale, Pale, 0.81f);
        }
        Text(TEXT("PRESS 1 / 2 / 3 TO CLAIM"),
             Left + 154.f * Scale, Top + 205.f * Scale, GoldColor, 0.9f);
    }
    else if (Game.Phase == EAVPhase::Shop)
    {
        Text(TEXT("THE WANDERING MERCHANT"),
             Left + 113.f * Scale, Top + 19.f * Scale, GoldColor, 1.2f);
        Text(FString::Printf(TEXT("YOUR GOLD  %d"), Game.Gold),
             Left + 204.f * Scale, Top + 49.f * Scale, Pale, 0.94f);
        static const TCHAR* Items[3] =
        { TEXT("HEAL +45 HP"), TEXT("SHARPEN  +5 DAMAGE"), TEXT("HEALTH POTION +1") };
        for (int32 I = 0; I < 3; ++I)
        {
            const float Y = Top + (80.f + I * 38.f) * Scale;
            const bool Sold = Game.ShopPurchased[I];
            Text(FString::Printf(TEXT("%d  %s"), I + 1, Items[I]),
                 Left + 39.f * Scale, Y, Sold ? Aether : Pale, 1.f);
            Text(Sold ? TEXT("SOLD") :
                 FString::Printf(TEXT("%d GOLD"), Game.ShopPrice(I)),
                 Left + 345.f * Scale, Y, Sold ? Aether : GoldColor, 0.9f);
        }
        if (Game.bInSafeCamp)
        {
            Text(FString::Printf(TEXT("PACK: ARMOR %d  BOWS %d  WEAPONS %d  SALVAGE %d"),
                 Game.BackpackArmor, Game.BackpackBows, Game.BackpackWeapons,
                 Game.PackMaterials),
                 Left + 33.f * Scale, Top + 183.f * Scale, GoldColor, 0.75f);
        }
        Text(Game.bInSafeCamp ? TEXT("1/2/3 BUY   4 SELL PACK   E/ESC EXIT") : TEXT("1/2/3 BUY   E/ESC EXIT"),
             Left + 126.f * Scale, Top + 206.f * Scale, Mint, 0.89f);
        if (Game.ToastTime > 0.f)
            Text(Game.Toast, Left + 185.f * Scale, Top + 178.f * Scale, Blood, 0.84f);
    }
    else if (Game.Phase == EAVPhase::Paused)
    {
        Text(TEXT("PAUSED"), Left + 210.f * Scale, Top + 56.f * Scale, GoldColor, 1.6f);
        Text(TEXT("PRESS P OR ESC TO RESUME"), Left + 125.f * Scale, Top + 137.f * Scale, Pale, 1.f);
    }
    else
    {
        const bool Won = Game.Phase == EAVPhase::Victory;
        Text(Won ? TEXT("THE VAULT IS SILENT") : TEXT("YOU HAVE FALLEN"),
             Left + (Won ? 125.f : 153.f) * Scale, Top + 28.f * Scale,
             Won ? Mint : Blood, 1.4f);
        Text(FString::Printf(TEXT("DEPTH %d    KILLS %d    GOLD %d"),
             Game.Floor, Game.Kills, Game.Gold),
             Left + 133.f * Scale, Top + 92.f * Scale, Pale, 1.f);
        Text(FString::Printf(TEXT("BEST DEPTH %d    BEST KILLS %d"),
             Game.BestFloor, Game.BestKills),
             Left + 132.f * Scale, Top + 128.f * Scale, GoldColor, 0.95f);
        Text(TEXT("ENTER  NEW RUN     R  RETRY SAME SEED"),
             Left + 90.f * Scale, Top + 184.f * Scale, Mint, 0.94f);
    }
}

void AAVHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas) return;
    const AAVGameMode* Game = GetWorld() ? GetWorld()->GetAuthGameMode<AAVGameMode>() : nullptr;
    if (!Game) return;
    if(Game->Phase==EAVPhase::MainMenu) return;

    InitSprites();
    Scale = FMath::Max(0.1f, FMath::Min(Canvas->SizeX / AAVGameMode::ViewW,
                                        Canvas->SizeY / AAVGameMode::ViewH));
    Center = FVector2D(Canvas->SizeX * 0.5f, Canvas->SizeY * 0.5f);
    const FVector2D Shake(FMath::Sin(Game->RunTime * 113.f),
                          FMath::Cos(Game->RunTime * 91.f));
    Origin = Center - Game->Hero * (AAVGameMode::TilePixels * Scale) +
             Shake * (Game->ScreenShake * 11.f * Scale);
    Rect(0.f, 0.f, Canvas->SizeX, Canvas->SizeY, Ink);
    DrawDungeon(*Game);
    DrawLighting(*Game);
    DrawActors(*Game);
    DrawAtmosphere(*Game);
    if (Game->Invulnerability > 0.5f)
        Rect(0.f, 0.f, Canvas->SizeX, Canvas->SizeY,
             FLinearColor(0.33f, 0.025f, 0.036f, 0.10f));
    if (Game->VisualQuality > 0 && (Game->PoisonTime > 0.f || Game->BurnTime > 0.f))
    {
        const FLinearColor Tint = Game->BurnTime > 0.f ?
            FLinearColor(0.9f, 0.23f, 0.06f, 0.17f) :
            FLinearColor(0.24f, 0.8f, 0.29f, 0.15f);
        Rect(0.f, 37.f * Scale, 8.f * Scale,
             Canvas->SizeY - 112.f * Scale, Tint);
        Rect(Canvas->SizeX - 8.f * Scale, 37.f * Scale,
             8.f * Scale, Canvas->SizeY - 112.f * Scale, Tint);
    }
    DrawInterface(*Game);
    DrawOverlay(*Game);
}
