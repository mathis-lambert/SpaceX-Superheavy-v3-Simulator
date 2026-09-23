#pragma once
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "Recovery/Interface/RecoveryPlayerController.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Styling/CoreStyle.h"

/** Clickable underside schematic, laid out from the real registered mounts. */
class SRecoveryEngineDiagram : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SRecoveryEngineDiagram) {} SLATE_ARGUMENT(ARecoveryPlayerController*,Controller) SLATE_END_ARGS()
    void Construct(const FArguments& Args){Controller=Args._Controller;}
    FVector2D ComputeDesiredSize(float) const override{return FVector2D(270,270);}
    bool ComputeVolatility() const override{return true;}
    FVector2D Point(const FGeometry& G,const FVector& P) const
    {return G.GetLocalSize()*.5+FVector2D(P.X,-P.Y)*FMath::Min(G.GetLocalSize().X,G.GetLocalSize().Y)/10.;}
    int32 Hit(const FGeometry& G,FVector2D Cursor) const
    {
        const auto* D=Controller.IsValid()?Controller->GetDirector():nullptr;if(!D)return INDEX_NONE;
        const auto Local=G.AbsoluteToLocal(Cursor);double Best=12;int32 Result=INDEX_NONE;
        for(int32 I=0;I<D->GetEngines().Num();++I)
        {double Distance=FVector2D::Distance(Local,Point(G,D->GetEngines()[I].PositionFromBaseM));if(Distance<Best){Best=Distance;Result=I;}}
        return Result;
    }
    FReply OnMouseButtonDown(const FGeometry& G,const FPointerEvent& E) override
    {
        if(E.GetEffectingButton()==EKeys::LeftMouseButton)
        {const int32 Index=Hit(G,E.GetScreenSpacePosition());if(Index!=INDEX_NONE && Controller.IsValid())Controller->SelectPart({ERecoveryPart::Engine,Index});}
        return FReply::Handled();
    }
    FReply OnMouseMove(const FGeometry& G,const FPointerEvent& E) override
    {
        Hover=Hit(G,E.GetScreenSpacePosition());
        const auto* D=Controller.IsValid()?Controller->GetDirector():nullptr;
        SetToolTipText(D && D->GetEngines().IsValidIndex(Hover)?FText::FromName(D->GetEngines()[Hover].Id):FText::GetEmpty());
        return FReply::Handled();
    }
    void OnMouseLeave(const FPointerEvent& E) override{Hover=INDEX_NONE;SLeafWidget::OnMouseLeave(E);}
    int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool) const override
    {
        const auto* D=Controller.IsValid()?Controller->GetDirector():nullptr;if(!D)return Layer;
        const auto Health=D->GetExperiment().AtTime(FMath::Max(0.,D->MissionTime));
        for(int32 I=0;I<D->GetEngines().Num();++I)
        {
            const auto& Engine=D->GetEngines()[I];const auto P=Point(G,Engine.PositionFromBaseM);
            const FLinearColor Color=Health.FailedEngine==I?FLinearColor(1.f,.22f,.16f):I==Hover?FLinearColor(.3f,.85f,1.f):Engine.ThrustN>1?FLinearColor::White:FLinearColor(.42f,.48f,.53f);
            TArray<FVector2D> Circle;for(int32 J=0;J<=24;++J){double A=J*2*PI/24.;Circle.Add(P+FVector2D(FMath::Cos(A),FMath::Sin(A))*9);}
            FSlateDrawElement::MakeLines(Out,Layer,G.ToPaintGeometry(),Circle,ESlateDrawEffect::None,Color,true,I==Hover?2.5f:1.f);
            FSlateDrawElement::MakeText(Out,Layer+1,G.ToPaintGeometry(FVector2f(18,14),FSlateLayoutTransform(FVector2f(P-FVector2D(6,6)))),
                FString::Printf(TEXT("%02d"),I+1),FCoreStyle::GetDefaultFontStyle("Regular",8),ESlateDrawEffect::None,Color);
        }
        return Layer+1;
    }
private:
    TWeakObjectPtr<ARecoveryPlayerController> Controller;
    int32 Hover=INDEX_NONE;
};
