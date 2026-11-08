#include "AVHUD.h"
#include "AVGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

namespace
{
FLinearColor BoltColor(const FAVBolt& B)
{
    if(B.Visual==4) return FLinearColor(0.76f,0.45f,1.f,1.f);
    if(B.Visual==3) return FLinearColor(0.98f,0.79f,0.39f,1.f);
    if(B.Visual==2) return FLinearColor(0.80f,0.93f,0.59f,1.f);
    if(B.Visual==1) return FLinearColor(0.95f,0.72f,0.45f,1.f);
    if(B.Status==1) return FLinearColor(0.29f,0.83f,0.42f,1.f);
    if(B.Status==2) return FLinearColor(1.f,0.38f,0.14f,1.f);
    return B.bFriendly?FLinearColor(0.86f,0.88f,0.94f,1.f):
                       FLinearColor(0.93f,0.20f,0.22f,1.f);
}
FLinearColor EffectColor(const AAVGameMode& G,const FAVEffect& E)
{
    if(E.Kind==3) return G.CurrentBiome()==1?
        FLinearColor(0.47f,0.87f,0.52f,1.f):
        G.CurrentBiome()==2?FLinearColor(1.f,0.55f,0.20f,1.f):
        G.CurrentBiome()==3?FLinearColor(0.89f,0.28f,0.31f,1.f):
        FLinearColor(0.76f,0.80f,0.88f,1.f);
    if(E.Kind==2) return FLinearColor(1.f,0.43f,0.36f,1.f);
    if(G.HeroClass==2) return FLinearColor(0.72f,0.50f,0.96f,1.f);
    if(G.HeroClass==3) return FLinearColor(0.81f,0.95f,0.64f,1.f);
    return FLinearColor(1.f,0.81f,0.50f,1.f);
}
}

void AAVHUD::DrawCombatVFX(const AAVGameMode& Game)
{
    if(Game.VisualQuality==0 || Game.Phase==EAVPhase::MainMenu)return;
    const float S=Scale,T=AAVGameMode::TilePixels*S;
    const int32 Quality=Game.VisualQuality;
    // Sub-pixel sparks are deliberately quantized and use nearest-neighbor tiles.
    for(const FAVBolt& B:Game.Bolts)
    {
        if(!Game.IsVisibleAt(B.Pos))continue;
        const FVector2D Dir=B.Velocity.GetSafeNormal();
        const FLinearColor Color=BoltColor(B);
        const int32 Steps=Quality==2?8:4;
        for(int32 I=1;I<=Steps;++I)
        {
            const FVector2D Trail=B.Pos-Dir*(I*0.09f);
            const FVector2D Pt=Screen(Trail);
            const float Alpha=(1.f-float(I)/float(Steps+1))*
                (B.bFriendly?0.65f:0.42f);
            const float R=(I%3==0?2.f:1.f)*S;
            Rect(Pt.X-R*0.5f,Pt.Y-R*0.5f,R,R,
                 FLinearColor(Color.R,Color.G,Color.B,Alpha));
        }
        if(B.Visual==3 && Quality==2)
        {
            const FVector2D Tip=Screen(B.Pos);
            for(int32 K=0;K<4;++K)
            {
                const float A=6.2831853f*K/4.f+Game.RunTime*7.f;
                const FVector2D P=Tip+FVector2D(FMath::Cos(A),FMath::Sin(A))*5.f*S;
                Rect(P.X,P.Y,2.f*S,2.f*S,
                     FLinearColor(Color.R,Color.G,Color.B,0.63f));
            }
        }
    }
    for(const FAVEffect& E:Game.Effects)
    {
        if(!Game.IsVisibleAt(E.Pos))continue;
        const float Life=FMath::Clamp(E.Life/FMath::Max(E.Duration,0.001f),0.f,1.f);
        const float Progress=1.f-Life;
        const FLinearColor Color=EffectColor(Game,E);
        const FVector2D CenterPoint=Screen(E.Pos);
        if(E.Kind==0 || E.Kind==1)
        {
            const int32 Segments=Quality==2?32:16;
            const float Start=E.Kind==1?-3.14159265f:E.Angle-1.35f;
            const float Sweep=E.Kind==1?6.2831853f:2.7f;
            const float R=(E.Radius*0.65f+Progress*0.16f)*T;
            for(int32 I=0;I<Segments;++I)
            {
                const float A=Start+Sweep*float(I)/float(Segments-1);
                const FVector2D Pt=CenterPoint+
                    FVector2D(FMath::Cos(A),FMath::Sin(A))*R;
                const float Width=E.Kind==1?3.1f:2.4f;
                Rect(Pt.X,Pt.Y,Width*S,Width*S,
                    FLinearColor(Color.R,Color.G,Color.B,0.60f*Life));
                if(I%5==0 && Quality==2)
                    Rect(Pt.X-2.f*S,Pt.Y-2.f*S,1.6f*S,1.6f*S,
                        FLinearColor(1.f,1.f,0.88f,0.65f*Life));
            }
        }
        else
        {
            const int32 Count=Quality==2?17:9;
            const float Spread=E.Radius*T*(0.25f+Progress*0.88f);
            for(int32 I=0;I<Count;++I)
            {
                const float A=float(I)*6.2831853f/Count+
                    E.Angle+Game.RunTime*0.27f;
                const float R=Spread*(0.55f+float((I*7)%11)/20.f);
                const FVector2D P=CenterPoint+
                    FVector2D(FMath::Cos(A),FMath::Sin(A))*R;
                const float Size=(I%4==0?3.2f:1.5f)*S;
                Rect(P.X,P.Y,Size,Size,
                     FLinearColor(Color.R,Color.G,Color.B,
                                 0.75f*Life));
            }
        }
    }
    // Dash ghosts make hitbox motion visible without obscuring attacks.
    if(Game.DashTime>0.f && HeroAtlas && Quality==2)
    {
        const int32 Frame=6+FMath::FloorToInt(Game.RunTime*14.f)%2;
        for(int32 I=1;I<=3;++I)
        {
            const FVector2D P=Screen(Game.Hero-Game.DashDirection*(0.24f*I));
            const float Size=37.f*S;
            DrawTexture(HeroAtlas,P.X-Size*0.5f,P.Y-Size*0.56f,
                        Size,Size,Frame/8.f,Game.HeroClass/4.f,
                        1.f/8.f,1.f/4.f,
                        FLinearColor(0.49f,0.80f,0.95f,0.23f/float(I)),
                        BLEND_Translucent,1.f,false,0.f,FVector2D::ZeroVector);
        }
    }
    // Boss specials visibly expand before the damaging animation.
    for(const FAVEnemy& E:Game.Enemies)
    {
        if(E.Type!=3 && E.Type!=8 && E.Type!=9 && E.Type!=56)continue;
        if(!Game.IsVisibleAt(E.Pos))continue;
        if(E.AttackWindup<=0.f && E.PhaseShiftTime<=0.f)continue;
        const FVector2D CenterPoint=Screen(E.Pos);
        const int32 Sparks=Quality==2?18:10;
        const float R=(1.32f+0.10f*FMath::Sin(Game.RunTime*18.f))*T;
        for(int32 I=0;I<Sparks;++I)
        {
            const float A=6.2831853f*float(I)/Sparks+
                Game.RunTime*(E.PhaseShiftTime>0.f?1.2f:0.25f);
            const FVector2D P=CenterPoint+
                FVector2D(FMath::Cos(A),FMath::Sin(A))*R;
            Rect(P.X,P.Y,2.f*S,2.f*S,
                 FLinearColor(1.f,0.32f,0.22f,0.56f));
        }
    }
}
