#include "Recovery/Shared/RecoveryUIStyle.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
namespace RecoveryUI
{
    const FLinearColor Accent(0.95f,0.96f,0.98f),Muted(0.55f,0.58f,0.62f),Ink(0.004f,0.005f,0.007f);
    const FButtonStyle& ButtonStyle()
    {
        static const FButtonStyle Style=FButtonStyle()
            .SetNormal(FSlateRoundedBoxBrush(FLinearColor(.015f,.019f,.026f,.5f),0.f,FLinearColor(.28f,.31f,.36f,.45f),1.f))
            .SetHovered(FSlateRoundedBoxBrush(FLinearColor(.13f,.15f,.18f,.85f),0.f,FLinearColor(.8f,.85f,.9f,.8f),1.f))
            .SetPressed(FSlateColorBrush(FLinearColor(0.22f,0.23f,0.25f)))
            .SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));
        return Style;
    }
    const FButtonStyle& PrimaryStyle()
    {
        static const FButtonStyle Style=FButtonStyle(ButtonStyle())
            .SetNormal(FSlateColorBrush(Accent)).SetHovered(FSlateColorBrush(FLinearColor::White))
            .SetPressed(FSlateColorBrush(FLinearColor(0.7f,0.72f,0.75f)));
        return Style;
    }
    const FSliderStyle& DaylightSliderStyle()
    {
        static const FSliderStyle Style=FSliderStyle()
            .SetNormalBarImage(FSlateColorBrush(FLinearColor::Transparent))
            .SetHoveredBarImage(FSlateColorBrush(FLinearColor::Transparent))
            .SetDisabledBarImage(FSlateColorBrush(FLinearColor::Transparent))
            .SetNormalThumbImage(FSlateRoundedBoxBrush(FLinearColor::White,7.f,FVector2D(14,26)))
            .SetHoveredThumbImage(FSlateRoundedBoxBrush(FLinearColor(1,.82f,.55f),7.f,FVector2D(14,30)))
            .SetDisabledThumbImage(FSlateColorBrush(FLinearColor::Gray)).SetBarThickness(6.f);
        return Style;
    }
    const FSliderStyle& ControlSliderStyle()
    {
        static const FSliderStyle Style=FSliderStyle(DaylightSliderStyle())
            .SetNormalBarImage(FSlateRoundedBoxBrush(FLinearColor(.24f,.27f,.31f),2.f))
            .SetHoveredBarImage(FSlateRoundedBoxBrush(FLinearColor(.42f,.45f,.49f),2.f))
            .SetBarThickness(3.f);
        return Style;
    }
    const FCheckBoxStyle& ToggleStyle()
    {
        static const FSlateRoundedBoxBrush Off(FLinearColor(.035f,.043f,.053f),4.f,FLinearColor(.48f,.51f,.55f),1.f,FVector2D(18,18));
        static const FSlateRoundedBoxBrush On(Accent,4.f,FVector2D(18,18));
        static const FCheckBoxStyle Style=FCheckBoxStyle().SetUncheckedImage(Off).SetUncheckedHoveredImage(Off)
            .SetUncheckedPressedImage(On).SetCheckedImage(On).SetCheckedHoveredImage(On).SetCheckedPressedImage(Off);
        return Style;
    }
}
