#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AVFrontendWidget.generated.h"

class UCanvasPanel;
class UImage;
class UTexture2D;
class AAVGameMode;

/**
 * Native, asset-independent, fully interactive v1.0 frontend.
 * No editor-authored Blueprint is necessary; UMG components are built at runtime.
 */
UCLASS()
class ASHENVAULTS_API UAVFrontendWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    virtual void NativeConstruct() override;
private:
    UPROPERTY(Transient) UTexture2D* BackgroundTexture = nullptr;
    UPROPERTY(Transient) UCanvasPanel* MainCanvas = nullptr;
    int32 Page = 0; // 0 home, 1 class selection, 2 options, 3 credits
    int32 ChosenHero = 0;

    AAVGameMode* Game() const;
    void Rebuild();
    void AddLabel(const FString& Content,const FVector2D& Position,int32 FontSize,
                  const FLinearColor& Tint,bool bBold=false);
    class UButton* AddButton(const FString& Text,int32 Row,bool bActive=true);
    void AddHeroText();

    UFUNCTION() void OnContinue();
    UFUNCTION() void OnNewRun();
    UFUNCTION() void OnPlaySelected();
    UFUNCTION() void OnHero0();
    UFUNCTION() void OnHero1();
    UFUNCTION() void OnHero2();
    UFUNCTION() void OnHero3();
    UFUNCTION() void OnOptions();
    UFUNCTION() void OnCredits();
    UFUNCTION() void OnBack();
    UFUNCTION() void OnFX();
    UFUNCTION() void OnFullscreen();
    UFUNCTION() void OnQuit();
};
