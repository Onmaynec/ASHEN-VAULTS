#include "AVHUD.h"
#include "AVGameMode.h"
#include "AVEquipment.h"
#include "AVSkills.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "Engine/Engine.h"

namespace
{
const FLinearColor Shadow(0.013f,0.017f,0.028f,0.96f);
const FLinearColor Panel(0.064f,0.077f,0.104f,0.97f);
const FLinearColor Edge(0.28f,0.24f,0.24f,1.f);
const FLinearColor Gold(0.92f,0.72f,0.36f,1.f);
const FLinearColor Pale(0.88f,0.85f,0.80f,1.f);
const FLinearColor Cyan(0.37f,0.77f,0.77f,1.f);
const FLinearColor Muted(0.48f,0.55f,0.61f,1.f);
const FLinearColor Rarity[]={
    FLinearColor(0.52f,0.55f,0.57f,1.f),
    FLinearColor(0.40f,0.74f,0.52f,1.f),
    FLinearColor(0.36f,0.65f,0.95f,1.f),
    FLinearColor(0.69f,0.40f,0.92f,1.f),
    FLinearColor(1.f,0.73f,0.28f,1.f)
};
const TCHAR* SlotNames[]={
    TEXT("WEAPON"),TEXT("HEAD"),TEXT("CHEST"),TEXT("HANDS"),
    TEXT("LEGS"),TEXT("BOOTS"),TEXT("RELIC I"),TEXT("RELIC II")
};
const TCHAR* BranchNames[]={
    TEXT("SURVIVAL"),TEXT("COMBAT"),TEXT("MOBILITY"),TEXT("CLASS PATH")
};
}

void AAVHUD::DrawGothicPanel(float X,float Y,float W,float H)
{
    const float S=Scale;
    Rect(X+5.f*S,Y+6.f*S,W,H,FLinearColor(0.f,0.f,0.f,0.55f));
    Rect(X,Y,W,H,Shadow);
    Rect(X+4.f*S,Y+4.f*S,W-8.f*S,H-8.f*S,Panel);
    Rect(X,Y,W,3.f*S,Gold);
    Rect(X,Y,3.f*S,H,Edge);
    Rect(X+W-3.f*S,Y,3.f*S,H,Edge);
    Rect(X,Y+H-3.f*S,W,3.f*S,Edge);
    for(int32 Corner=0;Corner<4;++Corner)
    {
        const float CX=X+(Corner%2?W-16.f*S:3.f*S);
        const float CY=Y+(Corner/2?H-15.f*S:3.f*S);
        Rect(CX,CY,13.f*S,3.f*S,Gold);
        Rect(CX,CY,3.f*S,12.f*S,Gold);
    }
}

void AAVHUD::DrawEquipmentIcon(int32 Id,float X,float Y,float Size)
{
    const FAVEquipmentSpec* Spec=FAVEquipment::Find(Id);
    if(!Spec)return;
    if(EquipmentAtlas)
    {
        const float U=(Id%7)/7.f,V=(Id/7)/6.f;
        DrawTexture(EquipmentAtlas,X,Y,Size,Size,U,V,
                    1.f/7.f,1.f/6.f,FLinearColor::White,
                    BLEND_Translucent,1.f,false,0.f,FVector2D::ZeroVector);
    }
    else
    {
        Rect(X+3.f*Scale,Y+3.f*Scale,Size-6.f*Scale,Size-6.f*Scale,
             Rarity[Spec->Rarity]);
        Rect(X+9.f*Scale,Y+9.f*Scale,Size-18.f*Scale,Size-18.f*Scale,Gold);
    }
}

void AAVHUD::DrawInventoryWindow(const AAVGameMode& Game)
{
    const float S=Scale;
    Rect(0.f,0.f,Canvas->SizeX,Canvas->SizeY,
         FLinearColor(0.008f,0.012f,0.02f,0.94f));
    const float X=Center.X-460.f*S,Y=Center.Y-245.f*S;
    DrawGothicPanel(X,Y,920.f*S,490.f*S);
    Rect(X+19.f*S,Y+53.f*S,882.f*S,S,
         FLinearColor(0.36f,0.28f,0.22f,0.82f));
    Text(TEXT("EQUIPMENT"),X+35.f*S,Y+16.f*S,Gold,1.5f);
    Text(FString::Printf(TEXT("INVENTORY   %d / 48"),Game.InventoryItems.Num()),
         X+590.f*S,Y+19.f*S,Cyan,1.06f);

    Rect(X+27.f*S,Y+66.f*S,311.f*S,362.f*S,
         FLinearColor(0.035f,0.046f,0.066f,0.93f));
    Rect(X+354.f*S,Y+66.f*S,539.f*S,360.f*S,
         FLinearColor(0.035f,0.046f,0.066f,0.93f));

    // The center figure uses the actual animated character sheet and palette.
    const float HX=X+166.f*S, HY=Y+238.f*S;
    for(int32 I=4;I>=0;--I)
    {
        const float R=(64.f+I*15.f)*S;
        Rect(HX-R*0.5f,HY+37.f*S-I*3.f*S,R,4.f*S,
             FLinearColor(0.56f,0.33f,0.17f,0.03f+I*0.017f));
    }
    if(HeroAtlas)
        DrawTexture(HeroAtlas,HX-68.f*S,HY-90.f*S,136.f*S,136.f*S,
                    0.f,Game.HeroClass/4.f,1.f/8.f,1.f/4.f,
                    FLinearColor::White,BLEND_Translucent,
                    1.f,false,0.f,FVector2D::ZeroVector);
    Text(FString::Printf(TEXT("HP %d   ATT %.0f   ARMOR %.0f%%"),
        FMath::CeilToInt(Game.MaxHP),Game.AttackPower,Game.ArmorRating*100.f),
        X+52.f*S,Y+380.f*S,Pale,0.84f);

    const float SlotX[8]={36,235,36,235,36,235,36,235};
    const float SlotY[8]={81,81,137,137,194,194,307,307};
    for(int32 I=0;I<8;++I)
    {
        const int32 Id=Game.EquipmentSlots.IsValidIndex(I)?Game.EquipmentSlots[I]:-1;
        const FAVEquipmentSpec* Spec=FAVEquipment::Find(Id);
        const float SX=X+SlotX[I]*S,SY=Y+SlotY[I]*S;
        Rect(SX-1.f*S,SY-1.f*S,70.f*S,49.f*S,
             Spec?Rarity[Spec->Rarity]:Edge);
        Rect(SX+1.f*S,SY+1.f*S,66.f*S,45.f*S,Shadow);
        if(Spec)DrawEquipmentIcon(Id,SX+9.f*S,SY+2.f*S,36.f*S);
        Text(SlotNames[I],SX+3.f*S,SY+33.f*S,Spec?Gold:Muted,0.62f);
    }

    // Six rows of eight clickable-looking slots; keyboard selection remains authoritative.
    for(int32 I=0;I<48;++I)
    {
        const int32 Col=I%8,Row=I/8;
        const float IX=X+(375.f+Col*63.f)*S;
        const float IY=Y+(84.f+Row*54.f)*S;
        const bool Filled=Game.InventoryItems.IsValidIndex(I);
        const int32 Id=Filled?Game.InventoryItems[I]:-1;
        const FAVEquipmentSpec* Spec=FAVEquipment::Find(Id);
        const bool Active=Filled && Game.GearSelection==I;
        Rect(IX-2.f*S,IY-2.f*S,54.f*S,49.f*S,
             Active?Gold:Spec?Rarity[Spec->Rarity]:Edge);
        Rect(IX,IY,50.f*S,45.f*S,
             Active?FLinearColor(0.15f,0.19f,0.23f,0.95f):Shadow);
        if(Spec)
            DrawEquipmentIcon(Id,IX+5.f*S,IY+3.f*S,39.f*S);
    }
    if(Game.InventoryItems.IsValidIndex(Game.GearSelection))
    {
        const int32 Id=Game.InventoryItems[Game.GearSelection];
        const FAVEquipmentSpec* Spec=FAVEquipment::Find(Id);
        if(Spec)
        {
            const FString Name=Game.GearName(Id).Left(32);
            Text(Name,X+361.f*S,Y+407.f*S,Rarity[Spec->Rarity],0.86f);
            Text(FString::Printf(TEXT("%s %.0f   %s %.0f"),
                UTF8_TO_TCHAR(FAVEquipment::StatName(Spec->StatA)),Spec->ValueA,
                UTF8_TO_TCHAR(FAVEquipment::StatName(Spec->StatB)),Spec->ValueB),
                X+361.f*S,Y+430.f*S,Pale,0.78f);
            if(Spec->PerkId>0)
                Text(FString(UTF8_TO_TCHAR(FAVEquipment::PerkName(Spec->PerkId))),
                     X+361.f*S,Y+452.f*S,Gold,0.72f);
        }
    }
    Text(TEXT("UP/DOWN  SELECT    ENTER  EQUIP    1-8  UNEQUIP    I/TAB  CLOSE"),
         X+33.f*S,Y+466.f*S,Muted,0.75f);
}

void AAVHUD::DrawTalentWindow(const AAVGameMode& Game)
{
    const float S=Scale;
    Rect(0.f,0.f,Canvas->SizeX,Canvas->SizeY,
         FLinearColor(0.008f,0.012f,0.02f,0.94f));
    const float X=Center.X-460.f*S,Y=Center.Y-245.f*S;
    DrawGothicPanel(X,Y,920.f*S,490.f*S);
    Text(TEXT("PATHS OF ASCENSION"),X+30.f*S,Y+13.f*S,Gold,1.46f);
    Text(FString::Printf(TEXT("LVL %d   SKILL POINTS %d"),Game.Level,Game.SkillPoints),
         X+642.f*S,Y+19.f*S,Cyan,0.94f);
    Rect(X+16.f*S,Y+52.f*S,886.f*S,S,Edge);
    const FLinearColor BranchColors[4]={
        FLinearColor(0.46f,0.72f,0.63f,1.f),
        FLinearColor(0.92f,0.48f,0.42f,1.f),
        FLinearColor(0.43f,0.65f,0.92f,1.f),
        FLinearColor(0.74f,0.49f,0.91f,1.f)
    };
    for(int32 Branch=0;Branch<4;++Branch)
    {
        const float BX=X+(22.f+Branch*165.f)*S;
        const int32 Count=Branch==3?6:5;
        Text(BranchNames[Branch],BX+12.f*S,Y+70.f*S,
             BranchColors[Branch],0.85f);
        for(int32 Row=0;Row<Count;++Row)
        {
            const int32 Selection=Branch*5+Row;
            const int32 Id=Game.SkillIdAt(Selection);
            const FAVSkillSpec* Spec=FAVSkills::Find(Id);
            if(!Spec)continue;
            const float NX=BX,NY=Y+(105.f+Row*56.f)*S;
            if(Row>0)
            {
                Rect(NX+69.f*S,NY-19.f*S,3.f*S,19.f*S,
                     Game.SkillRank(Id)>0?BranchColors[Branch]:Edge);
                Rect(NX+67.f*S,NY-17.f*S,7.f*S,2.f*S,Edge);
            }
            const bool Selected=Selection==Game.SkillSelection;
            const int32 Rank=Game.SkillRank(Id);
            const bool Learnable=FAVSkills::CanLearn(
                Id,Game.HeroClass,Game.Level,Game.LearnedSkills.GetData(),
                Game.LearnedSkills.Num(),Game.SkillPoints);
            const FLinearColor Border=Selected?Gold:
                Rank>0?BranchColors[Branch]:
                Learnable?Cyan:Edge;
            Rect(NX-2.f*S,NY-2.f*S,145.f*S,42.f*S,Border);
            Rect(NX,NY,141.f*S,38.f*S,Shadow);
            // Hand-drawn rune-like node emblem.
            Rect(NX+6.f*S,NY+6.f*S,22.f*S,22.f*S,Border);
            Rect(NX+9.f*S,NY+9.f*S,16.f*S,16.f*S,Shadow);
            Rect(NX+14.f*S,NY+14.f*S,6.f*S,6.f*S,Border);
            FString Name=FString(UTF8_TO_TCHAR(Spec->CodeName));
            Name.ReplaceInline(TEXT("_"),TEXT(" "));
            Text(Name.ToUpper().Left(16),NX+34.f*S,NY+7.f*S,
                 Rank>0?Pale:Muted,0.75f);
            Text(FString::Printf(TEXT("%d / %d"),Rank,Spec->MaxRank),
                 NX+88.f*S,NY+24.f*S,Rank>0?Gold:Muted,0.68f);
        }
    }
    const float RX=X+686.f*S;
    Rect(RX,Y+65.f*S,213.f*S,365.f*S,Shadow);
    const int32 Id=Game.SkillIdAt(Game.SkillSelection);
    const FAVSkillSpec* Selected=FAVSkills::Find(Id);
    if(Selected)
    {
        FString Name=FString(UTF8_TO_TCHAR(Selected->CodeName));
        Name.ReplaceInline(TEXT("_"),TEXT(" "));
        Text(Name.ToUpper().Left(23),RX+12.f*S,Y+86.f*S,Gold,0.98f);
        Text(FString::Printf(TEXT("RANK %d / %d"),
             Game.SkillRank(Id),Selected->MaxRank),
             RX+12.f*S,Y+134.f*S,Pale,0.86f);
        Text(FString::Printf(TEXT("REQUIRES LEVEL %d"),Selected->MinLevel),
             RX+12.f*S,Y+160.f*S,Muted,0.76f);
        FString Description(UTF8_TO_TCHAR(FAVSkills::Description(Id)));
        // Word-wrap source description in deterministic 27-character lines.
        int32 Offset=0,Line=0;
        while(Offset<Description.Len() && Line<5)
        {
            const FString Part=Description.Mid(Offset,26);
            Text(Part,RX+12.f*S,Y+(206.f+Line*19.f)*S,Pale,0.79f);
            Offset+=Part.Len();++Line;
        }
        const bool CanLearn=FAVSkills::CanLearn(
            Id,Game.HeroClass,Game.Level,Game.LearnedSkills.GetData(),
            Game.LearnedSkills.Num(),Game.SkillPoints);
        Rect(RX+12.f*S,Y+328.f*S,186.f*S,29.f*S,
             CanLearn?FLinearColor(0.11f,0.39f,0.32f,1.f):Edge);
        Text(CanLearn?TEXT("ENTER  LEARN TALENT"):
                      TEXT("LOCKED / MAX RANK"),
             RX+21.f*S,Y+333.f*S,CanLearn?Gold:Muted,0.77f);
    }
    Text(TEXT("UP/DOWN  NAVIGATE   ENTER  LEARN   BACKSPACE  RESET IN CAMP   K  CLOSE"),
         X+28.f*S,Y+460.f*S,Muted,0.79f);
}
