#include "RecoveryLoadingScreen.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateColorBrush.h"

void SRecoveryLoadingScreen::Construct(const FArguments& Args)
{
    BarStyle=FCoreStyle::Get().GetWidgetStyle<FProgressBarStyle>("ProgressBar");
    BarStyle.SetFillImage(FSlateColorBrush(FLinearColor::White));
    BarStyle.SetBackgroundImage(FSlateColorBrush(FLinearColor(.025,.032,.04)));
    ChildSlot
    [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FLinearColor(.004,.006,.009,1)).HAlign(HAlign_Center).VAlign(VAlign_Center)
        [SNew(SBox).WidthOverride(380)
            [SNew(SVerticalBox)
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)
                [SNew(STextBlock).Text(FText::FromString(TEXT("S T A R B A S E")))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular",30)).ColorAndOpacity(FLinearColor(.92,.95,.98))]
                +SVerticalBox::Slot().AutoHeight().Padding(2,0,0,40)
                [SNew(STextBlock).Text(FText::FromString(TEXT("FLIGHT SIMULATOR")))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular",10)).ColorAndOpacity(FLinearColor(.37,.43,.49))]
                +SVerticalBox::Slot().AutoHeight()
                [SNew(SBox).HeightOverride(2)[SNew(SProgressBar).Style(&BarStyle).Percent(Args._Progress)
                    .FillColorAndOpacity(FLinearColor(.72,.85,.94))]]
                +SVerticalBox::Slot().AutoHeight().Padding(0,16,0,0)
                [SNew(STextBlock).Text(Args._Status).Font(FCoreStyle::GetDefaultFontStyle("Regular",11))
                    .ColorAndOpacity(FLinearColor(.48,.54,.60))]
            ]
        ]
    ];
}
