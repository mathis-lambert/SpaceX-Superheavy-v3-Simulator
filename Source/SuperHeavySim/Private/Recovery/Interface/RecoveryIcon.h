#pragma once
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"

// Original vector line icons: resolution independent, no bitmap/font dependency.
class SRecoveryIcon : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SRecoveryIcon) : _Label(), _Color(FLinearColor::White) {}
        SLATE_ARGUMENT(FString,Label)
        SLATE_ARGUMENT(FLinearColor,Color)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args){Label=Args._Label.ToLower();Color=Args._Color;}
    FVector2D ComputeDesiredSize(float) const override{return FVector2D(22,22);}
    int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool) const override
    {
        const FVector2D Size=G.GetLocalSize();const float Scale=FMath::Min(Size.X,Size.Y)/24.f;
        const auto Path=[&](std::initializer_list<FVector2D> Points){TArray<FVector2D>P;for(const auto& V:Points)P.Add(V*Scale);FSlateDrawElement::MakeLines(Out,Layer,G.ToPaintGeometry(),P,ESlateDrawEffect::None,Color*Style.GetColorAndOpacityTint(),true,1.65f*Scale);};
        const auto Circle=[&](float X,float Y,float R){TArray<FVector2D>P;for(int I=0;I<=32;++I){const float A=I*2*PI/32;P.Add(FVector2D(X+R*FMath::Cos(A),Y+R*FMath::Sin(A))*Scale);}FSlateDrawElement::MakeLines(Out,Layer,G.ToPaintGeometry(),P,ESlateDrawEffect::None,Color*Style.GetColorAndOpacityTint(),true,1.65f*Scale);};
        if(Label.Contains(TEXT("computer")) || Label.Contains(TEXT("systems"))){Path({{3,4},{21,4},{21,17},{3,17},{3,4}});Path({{8,21},{16,21},{12,21},{12,17}});Path({{6,9},{9,12},{6,15},{11,14},{16,14}});}
        else if(Label.Contains(TEXT("weather"))){Circle(9,8,3);Path({{9,1},{9,3}});Path({{2,8},{4,8}});Path({{3,2},{5,4}});Path({{5,18},{5,14},{9,12},{13,14},{17,12},{21,15},{21,18},{5,18}});}
        else if(Label.Contains(TEXT("faster"))){Path({{4,5},{11,12},{4,19}});Path({{13,5},{20,12},{13,19}});}
        else if(Label.Contains(TEXT("slower"))){Path({{11,5},{4,12},{11,19}});Path({{20,5},{13,12},{20,19}});}
        else if(Label.Contains(TEXT("events"))){Path({{4,5},{20,5},{20,20},{4,20},{4,5}});Path({{8,9},{16,9}});Path({{8,13},{16,13}});Path({{8,17},{13,17}});}
        else if(Label.Contains(TEXT("back")) || Label.Contains(TEXT("cancel")))Path({{19,12},{5,12},{11,6},{5,12},{11,18}});
        else if(Label.Contains(TEXT("camera")) || Label.Contains(TEXT("view")) || Label.Contains(TEXT("orbit")) || Label.Contains(TEXT("observer"))){Path({{3,7},{7,7},{9,4},{15,4},{17,7},{21,7},{21,20},{3,20},{3,7}});Circle(12,13,4);}
        else if(Label.Contains(TEXT("display")) || Label.Contains(TEXT("graphics")) || Label.Contains(TEXT("image"))){Path({{3,4},{21,4},{21,17},{3,17},{3,4}});Path({{12,17},{12,21},{7,21},{17,21}});}
        else if(Label.StartsWith(TEXT("light")) || Label.Contains(TEXT("environment")) || Label.Contains(TEXT("ray"))){Circle(12,12,4);for(int I=0;I<8;++I){float A=I*PI/4;Path({{12+7*FMath::Cos(A),12+7*FMath::Sin(A)},{12+10*FMath::Cos(A),12+10*FMath::Sin(A)}});}}
        else if(Label.Contains(TEXT("audio"))){Path({{3,9},{7,9},{12,5},{12,19},{7,15},{3,15},{3,9}});Path({{16,8},{18,10},{18,14},{16,16}});Path({{19,5},{22,9},{22,15},{19,19}});}
        else if(Label.Contains(TEXT("lab")) || Label.Contains(TEXT("experiment"))){Path({{8,3},{16,3},{14,3},{14,10},{21,20},{3,20},{10,10},{10,3}});Path({{7,15},{17,15}});}
        else if(Label.Contains(TEXT("settings")) || Label.Contains(TEXT("controls"))){for(int I=0;I<3;++I){float Y=5+7*I;Path({{3,Y},{21,Y}});Circle(I==1?9:15,Y,2);}}
        else if(Label.Contains(TEXT("restart")) || Label.Contains(TEXT("restore"))){Path({{4,10},{6,5},{12,3},{18,6},{21,12},{18,18},{12,21},{6,18}});Path({{3,4},{4,10},{10,9}});}
        else if(Label.Contains(TEXT("home")) || Label.Contains(TEXT("starbase"))){Path({{3,11},{12,3},{21,11},{19,11},{19,21},{5,21},{5,11}});Path({{10,21},{10,15},{14,15},{14,21}});}
        else if(Label.Contains(TEXT("exit")) || Label.Contains(TEXT("abort"))){Path({{10,3},{3,3},{3,21},{10,21}});Path({{8,12},{21,12},{16,7},{21,12},{16,17}});}
        else if(Label.Contains(TEXT("source")) || Label.Contains(TEXT("credit"))){Path({{6,3},{16,3},{20,7},{20,21},{6,21},{6,3}});Path({{10,10},{16,10}});Path({{10,14},{16,14}});Path({{10,18},{15,18}});}
        else if(Label.Contains(TEXT("launch")) || Label.Contains(TEXT("resume")) || Label.Contains(TEXT("flight"))){Path({{7,3},{21,12},{7,21},{7,3}});}
        else if(Label.Contains(TEXT("keep")) || Label.Contains(TEXT("apply"))){Path({{4,12},{10,18},{21,6}});}
        else {Circle(12,12,8);Path({{8,12},{16,12}});}
        return Layer;
    }
private:FString Label;FLinearColor Color;
};
