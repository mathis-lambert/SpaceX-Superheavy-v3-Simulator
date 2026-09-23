#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

enum class ERecoveryMenuPage : uint8
{
    Home,
    Launch,
    Display,
    Controls,
    VideoConfirmation,
    Cameras,
    Photography,
    Settings,
    About,
    Mission,
    Audio,
    ImageQuality,
    RayTracing,
    Credits,
    RestartConfirmation,
    ReturnHomeConfirmation,
    Optics,
    Color,
    Environment,
    SavedLooks
};

class ARecoveryPlayerController;
class SVerticalBox;
class SBox;
class SRecoveryMenu : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SRecoveryMenu) {}
    SLATE_ARGUMENT(ARecoveryPlayerController*, Controller) SLATE_END_ARGS() void Construct(const FArguments& Args);
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FReply OnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    void ShowPage(ERecoveryMenuPage NewPage = ERecoveryMenuPage::Home);

private:
    TWeakObjectPtr<ARecoveryPlayerController> Controller;
    TSharedPtr<SBox> Content;
    ERecoveryMenuPage Page = ERecoveryMenuPage::Home;
    int32 PhotoSlot = 0;
    struct FDisplayDraft
    {
        FIntPoint Resolution;
        int32 WindowMode = 2, Quality = 3, Shadows = 3, Textures = 3, Effects = 3, AA = 3, GI = 3, Reflections = 3;
        float FrameLimit = 60, Light = 1;
        bool bVSync = false;
    } Draft;
    static ERecoveryMenuPage ParentPage(ERecoveryMenuPage Page)
    {
        switch (Page)
        {
        case ERecoveryMenuPage::Display:
        case ERecoveryMenuPage::Controls:
        case ERecoveryMenuPage::Photography:
        case ERecoveryMenuPage::Audio:
        case ERecoveryMenuPage::Credits:
            return ERecoveryMenuPage::Settings;
        case ERecoveryMenuPage::ImageQuality:
        case ERecoveryMenuPage::RayTracing:
            return ERecoveryMenuPage::Display;
        case ERecoveryMenuPage::RestartConfirmation:
        case ERecoveryMenuPage::ReturnHomeConfirmation:
            return ERecoveryMenuPage::Mission;
        case ERecoveryMenuPage::Optics:
        case ERecoveryMenuPage::Color:
        case ERecoveryMenuPage::Environment:
        case ERecoveryMenuPage::SavedLooks:
            return ERecoveryMenuPage::Photography;
        default:
            return ERecoveryMenuPage::Home;
        }
    }
    void ReadGraphics();
    void ApplyGraphics();
    TSharedRef<SWidget> Text(const FString& Value, int32 Size = 14,
                             FLinearColor Color = FLinearColor(0.87f, 0.91f, 0.94f)) const;
    TSharedRef<SWidget> Button(const FString& Label, TFunction<void()> Action, bool bPrimary = false) const;
    void Choice(TSharedRef<SVerticalBox> Rows, const FString& Label, TArray<FString> Values, int32 Selected,
                TFunction<void(int32)> Changed);
    void Toggle(TSharedRef<SVerticalBox> Rows, const FString& Label, bool Selected, TFunction<void(bool)> Changed);
    void PhotoSlider(TSharedRef<SVerticalBox> Rows, const FString& Label, float* Value, float Minimum, float Maximum,
                     const TCHAR* Unit, bool Logarithmic = false);
    void PhotoPage(TSharedRef<SVerticalBox> Rows);
};
