#include "AVFrontendWidget.h"
#include "AVGameMode.h"
#include "AVImageLoader.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Styling/CoreStyle.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
const FLinearColor Gold(0.91f,0.70f,0.39f,1.f);
const FLinearColor Pale(0.84f,0.85f,0.84f,1.f);
const FLinearColor Dim(0.56f,0.62f,0.66f,1.f);
const FLinearColor Red(0.66f,0.16f,0.20f,1.f);
FSlateFontInfo Font(int32 Size,bool Bold)
{
    return FCoreStyle::GetDefaultFontStyle(Bold?TEXT("Bold"):TEXT("Regular"),Size);
}
}

AAVGameMode* UAVFrontendWidget::Game() const
{
    UWorld* World=GetWorld();
    return World?Cast<AAVGameMode>(World->GetAuthGameMode()):nullptr;
}

void UAVFrontendWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if(!BackgroundTexture)
        BackgroundTexture=FAVImageLoader::LoadPNG(TEXT("AVVisuals/Backgrounds/MenuBackdrop.png"));
    Rebuild();
}

void UAVFrontendWidget::AddLabel(const FString& Content,const FVector2D& Position,
                                  int32 FontSize,const FLinearColor& Tint,bool bBold)
{
    UTextBlock* Label=WidgetTree->ConstructWidget<UTextBlock>();
    Label->SetText(FText::FromString(Content));
    Label->SetColorAndOpacity(FSlateColor(Tint));
    Label->SetFont(Font(FontSize,bBold));
    Label->SetShadowOffset(FVector2D(2.f,3.f));
    Label->SetShadowColorAndOpacity(FLinearColor(0.f,0.f,0.f,0.85f));
    UCanvasPanelSlot* Slot=MainCanvas->AddChildToCanvas(Label);
    Slot->SetPosition(Position);
    Slot->SetAutoSize(true);
}

UButton* UAVFrontendWidget::AddButton(const FString& Content,int32 Row,bool bActive)
{
    UButton* Button=WidgetTree->ConstructWidget<UButton>();
    Button->SetIsEnabled(bActive);
    Button->SetBackgroundColor(bActive?FLinearColor(0.30f,0.14f,0.16f,0.97f):
                                     FLinearColor(0.085f,0.09f,0.10f,0.83f));
    UTextBlock* Label=WidgetTree->ConstructWidget<UTextBlock>();
    Label->SetText(FText::FromString(Content));
    Label->SetFont(Font(19,true));
    Label->SetColorAndOpacity(FSlateColor(bActive?Pale:Dim));
    Label->SetJustification(ETextJustify::Center);
    Button->AddChild(Label);
    UCanvasPanelSlot* Slot=MainCanvas->AddChildToCanvas(Button);
    Slot->SetPosition(FVector2D(76.f,196.f+Row*52.f));
    Slot->SetSize(FVector2D(320.f,47.f));
    return Button;
}

void UAVFrontendWidget::AddHeroText()
{
    const TCHAR* Names[4]={TEXT("WARDEN"),TEXT("DUELIST"),
                           TEXT("ARCANIST"),TEXT("MARKSMAN")};
    const TCHAR* Roles[4]={
        TEXT("Heavy armor / control / endurance"),
        TEXT("Rapid strikes / critical hits / mobility"),
        TEXT("Forbidden magic / spells / ranged burst"),
        TEXT("Arrows / piercing shots / precision")
    };
    AddLabel(FString::Printf(TEXT("%s  —  %s"),Names[ChosenHero],Roles[ChosenHero]),
             FVector2D(420.f,260.f),17,Gold,true);
}

void UAVFrontendWidget::Rebuild()
{
    if(!WidgetTree)return;
    MainCanvas=WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget=MainCanvas;
    UImage* Backdrop=WidgetTree->ConstructWidget<UImage>();
    if(BackgroundTexture) Backdrop->SetBrushFromTexture(BackgroundTexture,true);
    else Backdrop->SetColorAndOpacity(FLinearColor(0.10f,0.09f,0.13f,1.f));
    UCanvasPanelSlot* BG=MainCanvas->AddChildToCanvas(Backdrop);
    BG->SetAnchors(FAnchors(0.f,0.f,1.f,1.f));
    BG->SetOffsets(FMargin(0.f));

    UBorder* Shade=WidgetTree->ConstructWidget<UBorder>();
    Shade->SetBrushColor(FLinearColor(0.016f,0.021f,0.035f,0.90f));
    UCanvasPanelSlot* ShadeSlot=MainCanvas->AddChildToCanvas(Shade);
    ShadeSlot->SetAnchors(FAnchors(0.f,0.f,0.52f,1.f));
    ShadeSlot->SetOffsets(FMargin(0.f));
    AddLabel(TEXT("ASHEN"),FVector2D(69.f,43.f),59,Pale,true);
    AddLabel(TEXT("VAULTS"),FVector2D(69.f,99.f),59,Gold,true);
    AddLabel(TEXT("THE DEPTHS ARE NEVER SILENT"),
             FVector2D(73.f,164.f),13,Dim,true);

    if(Page==0)
    {
        UButton* Continue=AddButton(TEXT("CONTINUE DESCENT"),0,Game()&&Game()->bCanContinue);
        Continue->OnClicked.AddDynamic(this,&UAVFrontendWidget::OnContinue);
        AddButton(TEXT("NEW EXPEDITION"),1)->OnClicked.AddDynamic(this,&UAVFrontendWidget::OnNewRun);
        AddButton(TEXT("SETTINGS"),2)->OnClicked.AddDynamic(this,&UAVFrontendWidget::OnOptions);
        AddButton(TEXT("CREDITS / LICENSES"),3)->OnClicked.AddDynamic(this,&UAVFrontendWidget::OnCredits);
        AddButton(TEXT("EXIT GAME"),4)->OnClicked.AddDynamic(this,&UAVFrontendWidget::OnQuit);
    }
    else if(Page==1)
    {
        AddLabel(TEXT("CHOOSE YOUR HERO"),FVector2D(73.f,180.f),17,Gold,true);
        const TCHAR* Labels[4]={TEXT("WARDEN"),TEXT("DUELIST"),TEXT("ARCANIST"),TEXT("MARKSMAN")};
        for(int32 I=0;I<4;++I)
        {
            UButton* B=AddButton(FString::Printf(TEXT("%s %s"),
                             ChosenHero==I?TEXT("◆"):TEXT(" "),Labels[I]),I);
            switch(I) {
            case 0:B->OnClicked.AddDynamic(this,&UAVFrontendWidget::OnHero0);break;
            case 1:B->OnClicked.AddDynamic(this,&UAVFrontendWidget::OnHero1);break;
            case 2:B->OnClicked.AddDynamic(this,&UAVFrontendWidget::OnHero2);break;
            default:B->OnClicked.AddDynamic(this,&UAVFrontendWidget::OnHero3);break;
            }
        }
        AddButton(TEXT("BEGIN THE DESCENT"),4)->OnClicked.AddDynamic(this,&UAVFrontendWidget::OnPlaySelected);
        AddButton(TEXT("BACK"),5)->OnClicked.AddDynamic(this,&UAVFrontendWidget::OnBack);
        AddHeroText();
    }
    else if(Page==2)
    {
        const AAVGameMode* GM=Game();
        const TCHAR* Quality=GM?(GM->VisualQuality==2?TEXT("HIGH"):
                            GM->VisualQuality==1?TEXT("MEDIUM"):TEXT("LOW")):TEXT("HIGH");
        AddButton(FString::Printf(TEXT("PIXEL EFFECTS: %s"),Quality),0)->
            OnClicked.AddDynamic(this,&UAVFrontendWidget::OnFX);
        AddButton(TEXT("TOGGLE FULLSCREEN"),1)->OnClicked.AddDynamic(this,&UAVFrontendWidget::OnFullscreen);
        AddButton(TEXT("BACK"),3)->OnClicked.AddDynamic(this,&UAVFrontendWidget::OnBack);
        AddLabel(TEXT("WASD MOVE   MOUSE AIM   SPACE DASH"),
                 FVector2D(76.f,474.f),13,Pale);
        AddLabel(TEXT("I INVENTORY   K TALENTS   F3 EFFECTS"),
                 FVector2D(76.f,493.f),13,Pale);
    }
    else
    {
        AddLabel(TEXT("ASHEN VAULTS — ORIGINAL GAME ART"),
                 FVector2D(76.f,220.f),18,Gold,true);
        AddLabel(TEXT("Asset licenses are recorded under Docs/ASSET_CREDITS.md"),
                 FVector2D(76.f,259.f),14,Pale);
        AddLabel(TEXT("Unreal Engine 4.27, Paper2D, UMG, Niagara"),
                 FVector2D(76.f,289.f),14,Dim);
        AddButton(TEXT("BACK"),3)->OnClicked.AddDynamic(this,&UAVFrontendWidget::OnBack);
    }
    AddLabel(TEXT("v1.7.0  /  SHRINE OF CHOICE"),
             FVector2D(76.f,514.f),13,Gold);
}

void UAVFrontendWidget::OnContinue()
{
    if(AAVGameMode* GM=Game()) {
        GM->StartFromMenu(true,GM->HeroClass);
        RemoveFromParent();
    }
}
void UAVFrontendWidget::OnNewRun(){Page=1;Rebuild();}
void UAVFrontendWidget::OnPlaySelected()
{
    if(AAVGameMode* GM=Game()) {
        GM->StartFromMenu(false,ChosenHero);
        RemoveFromParent();
    }
}
void UAVFrontendWidget::OnHero0(){ChosenHero=0;Rebuild();}
void UAVFrontendWidget::OnHero1(){ChosenHero=1;Rebuild();}
void UAVFrontendWidget::OnHero2(){ChosenHero=2;Rebuild();}
void UAVFrontendWidget::OnHero3(){ChosenHero=3;Rebuild();}
void UAVFrontendWidget::OnOptions(){Page=2;Rebuild();}
void UAVFrontendWidget::OnCredits(){Page=3;Rebuild();}
void UAVFrontendWidget::OnBack(){Page=0;Rebuild();}
void UAVFrontendWidget::OnFX(){
    if(AAVGameMode* GM=Game()) GM->VisualQuality=(GM->VisualQuality+1)%3;
    Rebuild();
}
void UAVFrontendWidget::OnFullscreen()
{
    if(GEngine && GEngine->GetGameUserSettings()) {
        UGameUserSettings* S=GEngine->GetGameUserSettings();
        S->SetFullscreenMode(S->GetFullscreenMode()==EWindowMode::Windowed?
                             EWindowMode::Fullscreen:EWindowMode::Windowed);
        S->ApplySettings(false);
        S->SaveSettings();
    }
    Rebuild();
}
void UAVFrontendWidget::OnQuit()
{
    UKismetSystemLibrary::QuitGame(this,GetOwningPlayer(),EQuitPreference::Quit,false);
}
