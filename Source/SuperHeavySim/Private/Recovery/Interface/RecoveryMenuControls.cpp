#include "Recovery/Interface/RecoveryMenu.h"
#include "Recovery/Interface/RecoveryIcon.h"
#include "Recovery/Shared/RecoveryUIStyle.h"
#include "Recovery/Interface/RecoveryPlayerController.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Flight/SuperHeavyLaunchTower.h"
#include "Recovery/Presentation/RecoveryRenderSettings.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SSlider.h"

TSharedRef<SWidget> SRecoveryMenu::Text(const FString& Value, int32 Size, FLinearColor Color) const
{
    return SNew(STextBlock)
        .Text(FText::FromString(Value))
        .Font(FCoreStyle::GetDefaultFontStyle(Size >= 24 ? "Bold" : "Regular", Size))
        .ColorAndOpacity(Color)
        .AutoWrapText(true);
}
TSharedRef<SWidget> SRecoveryMenu::Button(const FString& Label, TFunction<void()> Action, bool bPrimary) const
{
    return SNew(SBox).HeightOverride(
        50)[SNew(SButton)
                .ButtonStyle(bPrimary ? &RecoveryUI::PrimaryStyle() : &RecoveryUI::ButtonStyle())
                .HAlign(HAlign_Left)
                .VAlign(VAlign_Center)
                .ContentPadding(FMargin(22, 0))
                .OnClicked_Lambda([Action = MoveTemp(Action)]() {
                    Action();
                    return FReply::Handled();
                })[SNew(SHorizontalBox) +
                   SHorizontalBox::Slot()
                       .AutoWidth()
                       .VAlign(VAlign_Center)
                       .Padding(0, 0, 14, 0)
                           [SNew(SRecoveryIcon).Label(Label).Color(bPrimary ? RecoveryUI::Ink : RecoveryUI::Accent)] +
                   SHorizontalBox::Slot().FillWidth(1).VAlign(
                       VAlign_Center)[Text(Label, 16, bPrimary ? RecoveryUI::Ink : RecoveryUI::Accent)]]];
}
void SRecoveryMenu::Choice(TSharedRef<SVerticalBox> Rows, const FString& Label, TArray<FString> Values, int32 Selected,
                           TFunction<void(int32)> Changed)
{
    auto Options = MakeShared<TArray<TSharedPtr<FString>>>();
    for (const auto& Value : Values)
        Options->Add(MakeShared<FString>(Value));
    auto SelectedText = MakeShared<FString>(Values[FMath::Clamp(Selected, 0, Values.Num() - 1)]);
    Rows->AddSlot().AutoHeight().Padding(0, 0, 0, 12)
        [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(0.49f).VAlign(VAlign_Center)[Text(Label, 15)] +
         SHorizontalBox::Slot().FillWidth(0.51f)[SNew(SBox).HeightOverride(
             42)[SNew(SComboBox<TSharedPtr<FString>>)
                     .OptionsSource(&Options.Get())
                     .InitiallySelectedItem((*Options)[FMath::Clamp(Selected, 0, Values.Num() - 1)])
                     .ButtonStyle(&RecoveryUI::ButtonStyle())
                     .ContentPadding(FMargin(14, 4))
                     .OnGenerateWidget_Lambda([Options, this](TSharedPtr<FString> Value) { return Text(*Value, 14); })
                     .OnSelectionChanged_Lambda(
                         [Options, SelectedText, Changed](TSharedPtr<FString> Value, ESelectInfo::Type Type) {
                             if (Value.IsValid() && Type != ESelectInfo::Direct)
                             {
                                 *SelectedText = *Value;
                                 Changed(Options->IndexOfByKey(Value));
                             }
                         })[SNew(STextBlock)
                                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
                                .ColorAndOpacity(RecoveryUI::Accent)
                                .Text_Lambda([SelectedText]() { return FText::FromString(*SelectedText); })]]]];
}
void SRecoveryMenu::Toggle(TSharedRef<SVerticalBox> Rows, const FString& Label, bool Selected,
                           TFunction<void(bool)> Changed)
{
    Rows->AddSlot().AutoHeight().Padding(
        0, 0, 0, 16)[SNew(SCheckBox)
                         .Style(&RecoveryUI::ToggleStyle())
                         .IsChecked(Selected ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                         .OnCheckStateChanged_Lambda([Changed](ECheckBoxState State) {
                             Changed(State == ECheckBoxState::Checked);
                         })[SNew(SBox).Padding(FMargin(12, 4))[Text(Label, 15)]]];
}
void SRecoveryMenu::ReadGraphics()
{
    if (auto* S = GEngine->GetGameUserSettings())
    {
        Draft.Resolution = S->GetScreenResolution();
        Draft.WindowMode = int32(S->GetFullscreenMode());
        Draft.Quality = S->GetOverallScalabilityLevel();
        Draft.Shadows = S->GetShadowQuality();
        Draft.Textures = S->GetTextureQuality();
        Draft.Effects = S->GetVisualEffectQuality();
        Draft.AA = S->GetAntiAliasingQuality();
        Draft.GI = S->GetGlobalIlluminationQuality();
        Draft.Reflections = S->GetReflectionQuality();
        Draft.FrameLimit = S->GetFrameRateLimit();
        Draft.bVSync = S->IsVSyncEnabled();
    }
    if (Controller.IsValid())
        Draft.Light = Controller->EngineLightScale;
}
void SRecoveryMenu::ApplyGraphics()
{
    auto* S = GEngine->GetGameUserSettings();
    if (!S || !Controller.IsValid())
        return;
    const FIntPoint PreviousResolution = S->GetScreenResolution();
    const int32 PreviousMode = int32(S->GetFullscreenMode());
    const bool Changed = PreviousResolution != Draft.Resolution || PreviousMode != Draft.WindowMode;
    if (Changed)
        S->ConfirmVideoMode();
    if (Draft.Quality >= 0 && Draft.Quality <= 3)
        S->SetOverallScalabilityLevel(Draft.Quality);
    S->SetShadowQuality(Draft.Shadows);
    S->SetTextureQuality(Draft.Textures);
    S->SetVisualEffectQuality(Draft.Effects);
    S->SetAntiAliasingQuality(Draft.AA);
    S->SetGlobalIlluminationQuality(Draft.GI);
    S->SetReflectionQuality(Draft.Reflections);
    S->SetFrameRateLimit(Draft.FrameLimit);
    S->SetVSyncEnabled(Draft.bVSync);
    S->SetScreenResolution(Draft.Resolution);
    S->SetFullscreenMode(EWindowMode::Type(Draft.WindowMode));
    S->ApplySettings(false);
    Controller->SetReconstruction(Controller->ReconstructionMode);
    Controller->EngineLightScale = Draft.Light;
    Controller->SavePreferences();
    if (Changed)
        Controller->BeginVideoConfirmation(PreviousResolution, PreviousMode);
    else
        ShowPage(ERecoveryMenuPage::Display);
}
