#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AVHUD.generated.h"

class AAVGameMode;
class UTexture2D;

/**
 * Resolution-independent pixel art renderer.
 * Logical 960x540 viewport; all gameplay remains in C++ without imported assets.
 */
UCLASS()
class ASHENVAULTS_API AAVHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

private:
    UPROPERTY(Transient)
    TArray<UTexture2D*> Sprites;

    UPROPERTY(Transient)
    TArray<UTexture2D*> BiomeTiles; // fallback/generated variants
    UPROPERTY(Transient)
    TArray<UTexture2D*> AuthoredAtlases; // 4 authored PNG atlases, 7x4 at 24px
    UPROPERTY(Transient) UTexture2D* HeroAtlas = nullptr;
    UPROPERTY(Transient) UTexture2D* EquipmentAtlas = nullptr;

    float Scale = 1.f;
    FVector2D Center = FVector2D::ZeroVector;
    FVector2D Origin = FVector2D::ZeroVector;

    void InitSprites();
    UTexture2D* MakeSprite(int32 Type);
    void Rect(float X, float Y, float W, float H, const FLinearColor& Color);
    void Text(const FString& Value, float X, float Y, const FLinearColor& Color, float Size = 1.f);
    void Sprite(int32 Type, const FVector2D& Pos, float Size = 1.f, float Alpha = 1.f);
    void SpriteRotated(int32 Type, const FVector2D& Pos, float Size,
                       float AngleRadians, float Alpha = 1.f);
    FVector2D Screen(const FVector2D& World) const;
    void DrawDungeon(const AAVGameMode& Game);
    void DrawLighting(const AAVGameMode& Game);
    void DrawAtmosphere(const AAVGameMode& Game);
    void DrawActors(const AAVGameMode& Game);
    void DrawInterface(const AAVGameMode& Game);
    void Bar(float X, float Y, float W, float H, float Fraction,
             const FLinearColor& Fill, const FString& Label);
    void DrawMiniMap(const AAVGameMode& Game);
    void DrawOverlay(const AAVGameMode& Game);
    void DrawInventoryWindow(const AAVGameMode& Game);
    void DrawTalentWindow(const AAVGameMode& Game);
    void DrawEquipmentIcon(int32 ItemId, float X, float Y, float Size);
    void DrawGothicPanel(float X,float Y,float Width,float Height);
};
