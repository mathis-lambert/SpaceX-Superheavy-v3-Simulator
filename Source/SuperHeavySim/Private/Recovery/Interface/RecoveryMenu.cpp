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

void SRecoveryMenu::Construct(const FArguments& Args)
{
    Controller=Args._Controller;
    ChildSlot
    [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
      .BorderBackgroundColor_Lambda([this](){ return FLinearColor(.002f,.004f,.008f,Page==ERecoveryMenuPage::Photography || (Page>=ERecoveryMenuPage::Optics && Page<=ERecoveryMenuPage::SavedLooks)?.04f:Page==ERecoveryMenuPage::Cameras?0.12f:Page==ERecoveryMenuPage::Home && Controller.IsValid() && Controller->IsAtHome()?0.18f:0.55f); })
      .Padding(0)
      [SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
        [SNew(SBox).WidthOverride(1920).HeightOverride(1080)
          [SNew(SOverlay)
            +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(72,55,0,0)
              [SNew(SBox).WidthOverride(1100)[Text(TEXT("S T A R B A S E"),15,RecoveryUI::Accent)]]
            +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center)
              [SNew(SBox).Padding_Lambda([this](){return FMargin(72,100,0,60);})
               .MaxDesiredHeight(1020)[SAssignNew(Content,SBox)]]

          ]
        ]
      ]
    ];
    ShowPage();
}
FReply SRecoveryMenu::OnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    if(!Controller.IsValid() || Event.IsRepeat())return FReply::Unhandled();
    auto* PC=Controller.Get();const FKey Key=Event.GetKey();
    // Preview handles close keys even when a combo or slider owns keyboard focus.
    if(Key==EKeys::Escape)
    {
        if(PC->bVideoConfirmation)PC->RevertVideo();
        else if(Page==ERecoveryMenuPage::Cameras)PC->ResumeFlight();
        else if(Page!=ERecoveryMenuPage::Home)ShowPage(ParentPage(Page));
        else PC->TogglePauseMenu();
        return FReply::Handled();
    }
    if(Page==ERecoveryMenuPage::Cameras && Key==EKeys::Tab)
    {PC->ResumeFlight();return FReply::Handled();}
    return FReply::Unhandled();
}
void SRecoveryMenu::ShowPage(ERecoveryMenuPage NewPage)
{
    if(!Content || !Controller.IsValid()) return;
    if(NewPage==ERecoveryMenuPage::Display && Page!=ERecoveryMenuPage::Display && Page!=ERecoveryMenuPage::ImageQuality && Page!=ERecoveryMenuPage::RayTracing) ReadGraphics();
    Page=NewPage;auto* PC=Controller.Get();
    auto Rows=SNew(SVerticalBox);
    const bool Home=PC->IsAtHome();
    const float Width=Page==ERecoveryMenuPage::Cameras?1060:Page==ERecoveryMenuPage::Home?520:720;
    FString Title=Home?TEXT("RETURN TO\nSTARBASE"):TEXT("PAUSED");
    const TMap<ERecoveryMenuPage,FString> Titles={{ERecoveryMenuPage::Launch,TEXT("LAUNCH")},{ERecoveryMenuPage::Display,TEXT("DISPLAY")},{ERecoveryMenuPage::Controls,TEXT("CONTROLS")},
        {ERecoveryMenuPage::VideoConfirmation,TEXT("KEEP CHANGES?")},{ERecoveryMenuPage::Cameras,TEXT("CAMERAS")},{ERecoveryMenuPage::Photography,TEXT("PHOTOGRAPHY")},
        {ERecoveryMenuPage::Settings,TEXT("SETTINGS")},{ERecoveryMenuPage::About,TEXT("ABOUT")},{ERecoveryMenuPage::Mission,TEXT("MISSION")},{ERecoveryMenuPage::Audio,TEXT("AUDIO")},
        {ERecoveryMenuPage::ImageQuality,TEXT("IMAGE QUALITY")},{ERecoveryMenuPage::RayTracing,TEXT("RAY TRACING")},
        {ERecoveryMenuPage::Credits,TEXT("CREDITS")},{ERecoveryMenuPage::RestartConfirmation,TEXT("RESTART FLIGHT?")},{ERecoveryMenuPage::ReturnHomeConfirmation,TEXT("RETURN HOME?")},
        {ERecoveryMenuPage::Optics,TEXT("CAMERA OPTICS")},{ERecoveryMenuPage::Color,TEXT("COLOR & EXPOSURE")},{ERecoveryMenuPage::Environment,TEXT("ENVIRONMENT")},{ERecoveryMenuPage::SavedLooks,TEXT("SAVED LOOKS")}};
    if(const auto* Found=Titles.Find(Page))Title=*Found;
    Rows->AddSlot().AutoHeight().Padding(0,0,0,26)[Text(Title,Page==ERecoveryMenuPage::Home && Home?52:30)];
    if(Page==ERecoveryMenuPage::Home)
    {
        if(Home)
        {
            Rows->AddSlot().AutoHeight().Padding(0,0,0,10)[Button(TEXT("Launch"),[this](){ ShowPage(ERecoveryMenuPage::Launch); },true)];
        }
        else
        {
            Rows->AddSlot().AutoHeight().Padding(0,0,0,10)[Button(TEXT("Resume"),[PC](){ PC->ResumeFlight(); },true)];
            Rows->AddSlot().AutoHeight().Padding(0,0,0,10)[Button(TEXT("Cameras"),[this](){ ShowPage(ERecoveryMenuPage::Cameras); })];
        }
        if(!Home) Rows->AddSlot().AutoHeight().Padding(0,0,0,10)[Button(TEXT("Mission"),[this](){ ShowPage(ERecoveryMenuPage::Mission); })];
        Rows->AddSlot().AutoHeight().Padding(0,0,0,10)[Button(TEXT("Settings"),[this](){ ShowPage(ERecoveryMenuPage::Settings); })];

        Rows->AddSlot().AutoHeight().Padding(0,8,0,0)[Button(TEXT("Exit"),[PC](){ PC->QuitSimulation(); })];
    }
    else if(Page==ERecoveryMenuPage::Launch)
    {
        Choice(Rows,TEXT("Flight profile"),{TEXT("Nominal / 4 m/s wind"),TEXT("Crosswind / 14 m/s wind"),TEXT("Payload challenge / 5% more dry mass")},PC->SelectedScenario,
            [PC](int32 I){ PC->SelectedScenario=I;PC->SavePreferences(); });
        Choice(Rows,TEXT("Launch camera"),URecoveryCameraComponent::GetCameraNames(),PC->StartingCamera,
            [PC](int32 I){ PC->StartingCamera=I;PC->SavePreferences(); });
        Toggle(Rows,TEXT("Show flight telemetry"),PC->bTelemetry,[PC](bool B){ PC->SetTelemetry(B); });
        Rows->AddSlot().AutoHeight().Padding(0,12,0,0)[Button(TEXT("Launch"),[PC](){ PC->LaunchFlight(); },true)];
    }
    else if(Page==ERecoveryMenuPage::Display)
    {
        Rows->AddSlot().AutoHeight().Padding(0,0,0,14)[Button(TEXT("Image reconstruction   →"),[this](){ShowPage(ERecoveryMenuPage::ImageQuality);})];
        Rows->AddSlot().AutoHeight().Padding(0,0,0,14)[Button(TEXT("Hardware ray tracing   →"),[this](){ShowPage(ERecoveryMenuPage::RayTracing);})];
        auto Options=SNew(SVerticalBox);
        Rows->AddSlot().AutoHeight().Padding(0,0,0,14)[SNew(SBox).HeightOverride(330)[SNew(SScrollBox)+SScrollBox::Slot()[Options]]];
        TArray<FIntPoint> Resolutions={FIntPoint(1280,720),FIntPoint(1600,900),FIntPoint(1920,1080),FIntPoint(2560,1440),FIntPoint(3840,2160)};
        Resolutions.AddUnique(Draft.Resolution);TArray<FString> Labels;
        for(const auto R:Resolutions) Labels.Add(FString::Printf(TEXT("%d × %d"),R.X,R.Y));
        Choice(Options,TEXT("Output resolution"),Labels,Resolutions.IndexOfByKey(Draft.Resolution),[this,Resolutions](int32 I){ Draft.Resolution=Resolutions[I]; });
        Choice(Options,TEXT("Window mode"),{TEXT("Fullscreen"),TEXT("Borderless / desktop"),TEXT("Windowed")},Draft.WindowMode,[this](int32 I){ Draft.WindowMode=I; });
        const TArray<FString> Quality={TEXT("Low"),TEXT("Medium"),TEXT("High"),TEXT("Epic")};
        auto Presets=Quality;Presets.Add(TEXT("Custom"));
        Choice(Options,TEXT("Quality preset"),Presets,Draft.Quality<0 || Draft.Quality>3?4:Draft.Quality,[this](int32 I){ if(I<4) { Draft.Quality=Draft.Shadows=Draft.Textures=Draft.Effects=Draft.AA=Draft.GI=Draft.Reflections=I;ShowPage(ERecoveryMenuPage::Display); } });
        Choice(Options,TEXT("Shadows"),Quality,Draft.Shadows,[this](int32 I){ Draft.Shadows=I; });
        Choice(Options,TEXT("Textures"),Quality,Draft.Textures,[this](int32 I){ Draft.Textures=I; });
        Choice(Options,TEXT("Effects & atmosphere"),Quality,Draft.Effects,[this](int32 I){ Draft.Effects=I; });
        Choice(Options,TEXT("Anti-aliasing"),Quality,Draft.AA,[this](int32 I){ Draft.AA=I; });
        Choice(Options,TEXT("Global illumination"),Quality,Draft.GI,[this](int32 I){ Draft.GI=I; });
        Choice(Options,TEXT("Reflections"),Quality,Draft.Reflections,[this](int32 I){ Draft.Reflections=I; });
        Choice(Options,TEXT("Engine light intensity"),{TEXT("Off"),TEXT("Subtle / 50%"),TEXT("Standard / 100%"),TEXT("Bright / 150%"),TEXT("Very bright / 200%")},FMath::RoundToInt(Draft.Light*2),[this](int32 I){ Draft.Light=I*0.5f; });
        Choice(Options,TEXT("Frame rate limit"),{TEXT("30"),TEXT("60"),TEXT("120"),TEXT("144"),TEXT("Unlimited")},Draft.FrameLimit<=0?4:Draft.FrameLimit<=30?0:Draft.FrameLimit<=60?1:Draft.FrameLimit<=120?2:3,
            [this](int32 I){ const float Values[]={30,60,120,144,0};Draft.FrameLimit=Values[I]; });
        Toggle(Options,TEXT("Vertical sync"),Draft.bVSync,[this](bool B){ Draft.bVSync=B; });
        Rows->AddSlot().AutoHeight().Padding(0,0,0,10)[Button(TEXT("APPLY SETTINGS"),[this](){ ApplyGraphics(); },true)];
    }
    else if(Page==ERecoveryMenuPage::ImageQuality)
    {
        Choice(Rows,TEXT("Reconstruction"),RecoveryRenderSettings::ReconstructionNames(),PC->ReconstructionMode,
            [PC](int32 I){PC->SetReconstruction(I);});
        Rows->AddSlot().AutoHeight().Padding(0,0,0,14)[Text(RecoveryRenderSettings::SupportsDLSS()?TEXT("NVIDIA DLSS is available on this device."):TEXT("NVIDIA DLSS is unavailable on this device. Native rendering and Unreal TSR remain available."),14)];
    }
    else if(Page==ERecoveryMenuPage::Controls)
    {
        Rows->AddSlot().AutoHeight().Padding(0,0,0,12)[Text(TEXT("Mouse sensitivity"),15)];
        Rows->AddSlot().AutoHeight().Padding(0,0,0,22)[SNew(SSlider).Style(&RecoveryUI::ControlSliderStyle())
            .Value_Lambda([PC](){return (PC->MouseSensitivity-.1f)/1.9f;})
            .OnValueChanged_Lambda([PC](float V){PC->MouseSensitivity=.1f+V*1.9f;})
            .OnMouseCaptureEnd_Lambda([PC](){PC->SavePreferences();})
            .OnControllerCaptureEnd_Lambda([PC](){PC->SavePreferences();})];
        Toggle(Rows,TEXT("Invert vertical look"),PC->bInvertVerticalLook,[PC](bool B){PC->bInvertVerticalLook=B;PC->SavePreferences();});
        Toggle(Rows,TEXT("Automatic orbit"),PC->bAutomaticOrbit,[PC](bool B){PC->bAutomaticOrbit=B;PC->SavePreferences();});
        Rows->AddSlot().AutoHeight().Padding(0,0,0,18)[Text(TEXT("Hold right mouse: look · Left click: inspect\nWheel: zoom / speed · Arrows or WASD / ZQSD: move\nE / B: up / down"),14,RecoveryUI::Muted)];
        for(const auto& Binding:RecoveryInput::Bindings())
            Rows->AddSlot().AutoHeight().Padding(0,0,0,9)[SNew(SHorizontalBox)
                +SHorizontalBox::Slot().FillWidth(.4f)[Text(Binding.Key.GetDisplayName().ToString(),14,RecoveryUI::Accent)]
                +SHorizontalBox::Slot().FillWidth(.6f)[Text(Binding.Label,14)]];

    }
    else if(Page==ERecoveryMenuPage::VideoConfirmation)
    {
        Rows->AddSlot().AutoHeight().Padding(0,0,0,24)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",17)).ColorAndOpacity(RecoveryUI::Accent)
            .Text_Lambda([PC](){ return FText::FromString(FString::Printf(TEXT("Reverting automatically in %d seconds."),FMath::Max(0,FMath::CeilToInt(PC->VideoConfirmDeadline-FPlatformTime::Seconds())))); })];
        Rows->AddSlot().AutoHeight().Padding(0,0,0,10)[Button(TEXT("KEEP THESE SETTINGS"),[PC](){ PC->ConfirmVideo(); },true)];
        Rows->AddSlot().AutoHeight()[Button(TEXT("REVERT DISPLAY SETTINGS"),[PC](){ PC->RevertVideo(); })];
    }
    else if(Page==ERecoveryMenuPage::Photography || Page==ERecoveryMenuPage::Optics || Page==ERecoveryMenuPage::Color || Page==ERecoveryMenuPage::SavedLooks)PhotoPage(Rows);
    else if(Page==ERecoveryMenuPage::Environment)
    {
        Choice(Rows,TEXT("Weather / affects wind"),{TEXT("Clear"),TEXT("Coastal haze"),TEXT("Broken clouds"),TEXT("Overcast")},PC->WeatherPreset,[PC](int32 I){PC->SetWeatherPreset(I);});
        Rows->AddSlot().AutoHeight().Padding(0,0,0,12)[Text(TEXT("STARBASE / LOCAL TIME"),13,RecoveryUI::Muted)];
        Choice(Rows,TEXT("Clock"),{TEXT("CDT / UTC−5"),TEXT("CST / UTC−6")},PC->Photography.UtcOffsetHours<-5.5f?1:0,[PC](int32 I){PC->Photography.UtcOffsetHours=I?-6.f:-5.f;PC->SavePreferences();});
        PhotoSlider(Rows,TEXT("Date / 2026"),&PC->Photography.SolarDayOfYear,1,365,TEXT("calendar"));
        Rows->AddSlot().AutoHeight().Padding(0,0,0,12)
          [SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold",42)).ColorAndOpacity(RecoveryUI::Accent)
            .Text_Lambda([PC](){const int32 M=FMath::RoundToInt(PC->TimeOfDay*60)%1440;return FText::FromString(FString::Printf(TEXT("%02d:%02d"),M/60,M%60));})];
        auto Marks=SNew(SHorizontalBox);
        for(const TCHAR* Label:{TEXT("00:00"),TEXT("06:00"),TEXT("12:00"),TEXT("18:00")})
            Marks->AddSlot().FillWidth(1)[Text(Label,12,RecoveryUI::Muted)];
        Rows->AddSlot().AutoHeight().Padding(0,0,0,8)[Marks];
        auto Spectrum=SNew(SHorizontalBox);
        const FLinearColor Keys[]={FLinearColor(.015f,.025f,.08f),FLinearColor(.95f,.32f,.14f),FLinearColor(.35f,.65f,.9f),FLinearColor(.95f,.28f,.14f),FLinearColor(.015f,.025f,.08f)};
        for(int I=0;I<96;++I) {
            const float T=I/24.f;const int K=FMath::Min(3,int(T));
            Spectrum->AddSlot().FillWidth(1)[SNew(SBorder).Padding(0).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .BorderBackgroundColor(FMath::Lerp(Keys[K],Keys[K+1],T-K))];
        }
        Rows->AddSlot().AutoHeight().Padding(0,0,0,28)[SNew(SBox).HeightOverride(34)[SNew(SOverlay)
          +SOverlay::Slot().VAlign(VAlign_Center).Padding(7,0)[SNew(SBox).HeightOverride(6)[Spectrum]]
          +SOverlay::Slot()[SNew(SSlider).Style(&RecoveryUI::DaylightSliderStyle()).Value_Lambda([PC](){return PC->TimeOfDay/24.f;}).StepSize(1.f/288.f)
            .OnValueChanged_Lambda([PC](float V){PC->TimeOfDay=V*24.f;})
            .OnMouseCaptureEnd_Lambda([PC](){PC->SavePreferences();}).OnControllerCaptureEnd_Lambda([PC](){PC->SavePreferences();})]]];
        const auto Slider=[this,Rows,PC](const FString& Label,float* Value,float Maximum)
        {
            Rows->AddSlot().AutoHeight().Padding(0,0,0,10)[Text(Label,15)];
            Rows->AddSlot().AutoHeight().Padding(0,0,0,22)[SNew(SSlider).Style(&RecoveryUI::ControlSliderStyle()).Value_Lambda([Value,Maximum](){return *Value/Maximum;})
              .OnValueChanged_Lambda([Value,Maximum](float V){*Value=V*Maximum;})
              .OnMouseCaptureEnd_Lambda([PC](){PC->SavePreferences();}).OnControllerCaptureEnd_Lambda([PC](){PC->SavePreferences();})];
        };
        Slider(TEXT("Coastal haze / volumetric fog"),&PC->FogAmount,2.f);
    }
    else if(Page==ERecoveryMenuPage::Cameras)
    {
        const auto Names=URecoveryCameraComponent::GetCameraNames();
        auto Columns=SNew(SHorizontalBox);
        Rows->AddSlot().AutoHeight()[Columns];
        const int32 Order[2][7]={{0,1,2,3,4,5,6},{9,10,11,12,13,7,8}};
        for(int32 Col=0;Col<2;++Col)
        {
            auto List=SNew(SVerticalBox);
            Columns->AddSlot().FillWidth(1).Padding(Col?10:0,0,Col?0:10,0)[List];
            List->AddSlot().AutoHeight().Padding(0,0,0,14)[Text(Col?TEXT("LANDSCAPE & EXPLORATION"):TEXT("VEHICLE & TRACKING"),14,RecoveryUI::Accent)];
            for(int32 Row=0;Row<7;++Row)
            {
                const int32 Mode=Order[Col][Row];
                const bool Active=PC->GetDirector() && PC->GetDirector()->Viewer->GetCameraMode()==Mode;
                List->AddSlot().AutoHeight().Padding(0,0,0,9)[Button((Active?TEXT("●  "):TEXT(""))+Names[Mode],
                    [PC,Mode](){ PC->ChooseCamera(Mode); },Active)];
            }
        }
        Rows->AddSlot().AutoHeight().Padding(0,16,0,0)[Button(TEXT("Close"),[PC](){ PC->ResumeFlight(); })];
    }
    else if(Page==ERecoveryMenuPage::Settings)
    {
        for(const auto& Item:TArray<TPair<FString,ERecoveryMenuPage>>{{TEXT("Display"),ERecoveryMenuPage::Display},{TEXT("Photography"),ERecoveryMenuPage::Photography},{TEXT("Controls"),ERecoveryMenuPage::Controls},{TEXT("Audio"),ERecoveryMenuPage::Audio},{TEXT("Credits"),ERecoveryMenuPage::Credits}})
            Rows->AddSlot().AutoHeight().Padding(0,0,0,10)[Button(Item.Key,[this,Id=Item.Value](){ShowPage(Id);})];
    }
    else if(Page==ERecoveryMenuPage::About)
    {
        Rows->AddSlot().AutoHeight()[Button(TEXT("Credits"),[this](){ShowPage(ERecoveryMenuPage::Credits);})];
    }
    else if(Page==ERecoveryMenuPage::Mission)
    {
        Choice(Rows,TEXT("Playback speed"),{TEXT("0.25× / inspect"),TEXT("0.5× / slow motion"),TEXT("1× / real time"),TEXT("2× / fast forward"),TEXT("4× / fast forward")},PC->PlaybackRate<.4?0:PC->PlaybackRate<.75?1:PC->PlaybackRate<1.5?2:PC->PlaybackRate<3?3:4,
            [PC](int32 I){const float Rates[]={.25,.5,1,2,4};PC->SetPlaybackRate(Rates[I]);});
        Rows->AddSlot().AutoHeight().Padding(0,0,0,12)[Button(TEXT("Flight computer"),[PC](){PC->ResumeFlight();PC->ToggleFlightComputer();})];

        Toggle(Rows,TEXT("Flight telemetry"),PC->bTelemetry,[PC](bool B){PC->SetTelemetry(B);});
        Rows->AddSlot().AutoHeight().Padding(0,12,0,10)[Button(TEXT("Cameras"),[this](){ShowPage(ERecoveryMenuPage::Cameras);})];
        Rows->AddSlot().AutoHeight().Padding(0,0,0,10)[Button(TEXT("Restart flight"),[this](){ShowPage(ERecoveryMenuPage::RestartConfirmation);})];
        Rows->AddSlot().AutoHeight().Padding(0,0,0,10)[Button(TEXT("Return home"),[this](){ShowPage(ERecoveryMenuPage::ReturnHomeConfirmation);})];
        Rows->AddSlot().AutoHeight().Padding(0,0,0,10)[Button(TEXT("Abort flight"),[PC](){if(auto* D=PC->GetDirector())D->AbortMission();PC->ResumeFlight();})];
    }
    else if(Page==ERecoveryMenuPage::RestartConfirmation || Page==ERecoveryMenuPage::ReturnHomeConfirmation)
    {
        Rows->AddSlot().AutoHeight().Padding(0,0,0,22)[Text(TEXT("This ends the current flight."),14,RecoveryUI::Muted)];
        Rows->AddSlot().AutoHeight().Padding(0,0,0,10)[Button(Page==ERecoveryMenuPage::RestartConfirmation?TEXT("Restart"):TEXT("Return home"),[PC,this](){if(Page==ERecoveryMenuPage::RestartConfirmation)PC->RestartFlight();else PC->ReturnHome();},true)];
    }
    else if(Page==ERecoveryMenuPage::Audio)
    {
        Rows->AddSlot().AutoHeight().Padding(0,0,0,12)[Text(TEXT("Master volume"),16)];
        Rows->AddSlot().AutoHeight().Padding(0,0,0,25)[SNew(SSlider).Style(&RecoveryUI::ControlSliderStyle()).Value_Lambda([PC](){return PC->MasterVolume;})
            .OnValueChanged_Lambda([PC](float V){PC->MasterVolume=V;})
            .OnMouseCaptureEnd_Lambda([PC](){PC->SavePreferences();})];
    }
    else if(Page==ERecoveryMenuPage::RayTracing)
    {
        if(RecoveryRenderSettings::SupportsHardwareRayTracing())
            Toggle(Rows,TEXT("Hardware ray tracing / Lumen"),PC->bHardwareRayTracing,[PC](bool B){PC->SetHardwareRayTracing(B);});
        else Rows->AddSlot().AutoHeight().Padding(0,0,0,20)[Text(TEXT("Hardware ray tracing is unavailable on this device."),15)];
    }
    else if(Page==ERecoveryMenuPage::Credits)
    {
        Rows->AddSlot().AutoHeight().Padding(0,0,0,20)[Text(TEXT("EARTH / GEOGRAPHY"),16)];
        Rows->AddSlot().AutoHeight().Padding(0,0,0,20)[Text(TEXT("Data & Viewing Products: EOxCloudless https://cloudless.eox.at by EOX IT Services GmbH (Contains modified Copernicus Sentinel data 2016). CC BY 4.0. Geographic mosaic; current site structures are authored separately."),14,RecoveryUI::Muted)];
        Rows->AddSlot().AutoHeight().Padding(0,0,0,20)[Text(TEXT("Local imagery: USGS / USDA NAIP. Elevation: USGS 3DEP. Global imagery: NASA Blue Marble. Source URLs, coverage and hashes are retained with the project."),14,RecoveryUI::Muted)];
        Rows->AddSlot().AutoHeight().Padding(0,0,0,20)[Text(TEXT("FLIGHT / MODEL ASSUMPTIONS"),16)];
        Rows->AddSlot().AutoHeight()[Text(TEXT("Public vehicle references inform the geometry. Aerodynamic coefficients, engine transients and tank properties remain estimates. Successful simulation tests do not establish real-flight accuracy."),14,RecoveryUI::Muted)];
    }
    if(Page!=ERecoveryMenuPage::Home && Page!=ERecoveryMenuPage::VideoConfirmation && Page!=ERecoveryMenuPage::Cameras)
        Rows->AddSlot().AutoHeight().Padding(0,10,0,0)[Button(TEXT("Back"),[this](){ ShowPage(ParentPage(Page)); })];
    Content->SetContent(SNew(SBox).WidthOverride(Width)
        [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.003f,0.005f,0.009f,Page==ERecoveryMenuPage::Home && Home?.28f:.9f))
         .Padding(30)[SNew(SScrollBox)+SScrollBox::Slot()[Rows]]]);
    FSlateApplication::Get().SetKeyboardFocus(SharedThis(this));
}
