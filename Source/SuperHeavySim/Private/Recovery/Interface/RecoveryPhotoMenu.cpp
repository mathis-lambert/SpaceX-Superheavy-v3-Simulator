#include "Recovery/Interface/RecoveryMenu.h"
#include "Recovery/Interface/RecoveryPlayerController.h"
#include "Recovery/Shared/RecoveryUIStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SSlider.h"
#include "Styling/CoreStyle.h"

void SRecoveryMenu::PhotoSlider(TSharedRef<SVerticalBox> Rows,const FString& Label,float* Value,float Minimum,float Maximum,const TCHAR* Unit,bool Logarithmic)
{
    const FString Suffix=Unit;
    Rows->AddSlot().AutoHeight().Padding(0,0,0,8)[SNew(SHorizontalBox)
        +SHorizontalBox::Slot().FillWidth(1)[Text(Label,15)]
        +SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).ColorAndOpacity(RecoveryUI::Accent)
            .Text_Lambda([Value,Suffix](){
                if(Suffix==TEXT("calendar")){
                    const auto Date=FDateTime(2026,1,1)+FTimespan::FromDays(FMath::RoundToInt(*Value)-1);
                    static const TCHAR* Months[]={TEXT("Jan"),TEXT("Feb"),TEXT("Mar"),TEXT("Apr"),TEXT("May"),TEXT("Jun"),TEXT("Jul"),TEXT("Aug"),TEXT("Sep"),TEXT("Oct"),TEXT("Nov"),TEXT("Dec")};
                    return FText::FromString(FString::Printf(TEXT("%02d %s"),Date.GetDay(),Months[Date.GetMonth()-1]));
                }
                return FText::FromString(FString::Printf(TEXT("%.2f %s"),*Value,*Suffix));})]];
    const auto PC=Controller;
    Rows->AddSlot().AutoHeight().Padding(0,0,0,20)[SNew(SSlider).Style(&RecoveryUI::ControlSliderStyle())
        .Value_Lambda([Value,Minimum,Maximum,Logarithmic](){return Logarithmic?FMath::Loge(*Value/Minimum)/FMath::Loge(Maximum/Minimum):(*Value-Minimum)/(Maximum-Minimum);})
        .OnValueChanged_Lambda([Value,Minimum,Maximum,Logarithmic](float V){*Value=Logarithmic?Minimum*FMath::Pow(Maximum/Minimum,V):FMath::Lerp(Minimum,Maximum,V);})
        .OnMouseCaptureEnd_Lambda([PC](){if(PC.IsValid())PC->SavePreferences();})
        .OnControllerCaptureEnd_Lambda([PC](){if(PC.IsValid())PC->SavePreferences();})];
}
void SRecoveryMenu::PhotoPage(TSharedRef<SVerticalBox> Rows)
{
    auto* PC=Controller.Get();if(!PC)return;
    auto& P=PC->Photography;
    if(Page==ERecoveryMenuPage::Photography)
    {
        TArray<FString> Names={TEXT("Choose a look")};for(const auto& Preset:RecoveryPhotography::Presets())Names.Add(Preset.Name);
        Choice(Rows,TEXT("Photographic preset"),Names,0,[PC,this](int32 I){if(I>0){PC->ApplyPhotoPreset(I-1);ShowPage(ERecoveryMenuPage::Photography);}});
        for(const auto& Item:TArray<TPair<FString,ERecoveryMenuPage>>{{TEXT("Camera optics"),ERecoveryMenuPage::Optics},{TEXT("Color & exposure"),ERecoveryMenuPage::Color},{TEXT("Environment"),ERecoveryMenuPage::Environment},{TEXT("Saved looks"),ERecoveryMenuPage::SavedLooks}})
            Rows->AddSlot().AutoHeight().Padding(0,0,0,10)[Button(Item.Key,[this,Id=Item.Value](){ShowPage(Id);})];
    }
    else if(Page==ERecoveryMenuPage::Optics)
    {
        Toggle(Rows,TEXT("Automatic framing"),P.bAutomaticFraming,[PC,this](bool V){PC->Photography.bAutomaticFraming=V;PC->SavePreferences();ShowPage(ERecoveryMenuPage::Optics);});
        if(!P.bAutomaticFraming)PhotoSlider(Rows,TEXT("Focal length / 36 mm sensor"),&P.FocalLengthMm,12,600,TEXT("mm"),true);
        Toggle(Rows,TEXT("Depth of field"),PC->bCameraDepthOfField,[PC,this](bool V){PC->bCameraDepthOfField=V;PC->SavePreferences();ShowPage(ERecoveryMenuPage::Optics);});
        if(PC->bCameraDepthOfField)
        {
            PhotoSlider(Rows,TEXT("Aperture"),&P.Aperture,1.4f,22,TEXT("f-stop"),true);
            Toggle(Rows,TEXT("Track camera subject"),P.bAutomaticFocus,[PC,this](bool V){PC->Photography.bAutomaticFocus=V;PC->SavePreferences();ShowPage(ERecoveryMenuPage::Optics);});
            if(!P.bAutomaticFocus)PhotoSlider(Rows,TEXT("Focus distance"),&P.FocusDistanceM,2,20000,TEXT("m"),true);
        }
        PhotoSlider(Rows,TEXT("Camera vibration"),&P.MotionStrength,0,1,TEXT(""));
        PhotoSlider(Rows,TEXT("Telephoto tracking delay"),&P.TrackingLagSeconds,0,.5f,TEXT("s"));
        Toggle(Rows,TEXT("Fixed ground-camera framing"),P.bFixedFraming,[PC](bool V){PC->Photography.bFixedFraming=V;PC->SavePreferences();});
        PhotoSlider(Rows,TEXT("Motion blur"),&PC->MotionBlur,0,.5f,TEXT(""));
        Toggle(Rows,TEXT("Automatic cinematic orbit"),PC->bAutomaticOrbit,[PC](bool V){PC->bAutomaticOrbit=V;PC->SavePreferences();});
        PhotoSlider(Rows,TEXT("Orbit speed"),&P.OrbitSpeed,0,3,TEXT("x"));
    }
    else if(Page==ERecoveryMenuPage::Color)
    {
        PhotoSlider(Rows,TEXT("Exposure compensation"),&P.ExposureBiasEV,-3,3,TEXT("EV"));
        PhotoSlider(Rows,TEXT("White balance"),&P.WhiteBalanceK,2500,10000,TEXT("K"));
        PhotoSlider(Rows,TEXT("Tint"),&P.Tint,-.3f,.3f,TEXT(""));
        PhotoSlider(Rows,TEXT("Saturation"),&P.Saturation,0,1.5f,TEXT(""));
        PhotoSlider(Rows,TEXT("Contrast"),&P.Contrast,.7f,1.3f,TEXT(""));
        PhotoSlider(Rows,TEXT("Film grain"),&PC->CameraGrain,0,.35f,TEXT(""));
    }
    else if(Page==ERecoveryMenuPage::SavedLooks)
    {
        Choice(Rows,TEXT("Saved look"),{TEXT("Look 1"),TEXT("Look 2"),TEXT("Look 3")},PhotoSlot,[this](int32 I){PhotoSlot=I;ShowPage(ERecoveryMenuPage::SavedLooks);});
        Rows->AddSlot().AutoHeight().Padding(0,0,0,10)[Button(TEXT("Save current look"),[PC,this](){PC->SavePhotoLook(PhotoSlot);ShowPage(ERecoveryMenuPage::SavedLooks);},true)];
        if(PC->HasPhotoLook(PhotoSlot))Rows->AddSlot().AutoHeight()[Button(TEXT("Load saved look"),[PC,this](){PC->LoadPhotoLook(PhotoSlot);ShowPage(ERecoveryMenuPage::SavedLooks);})];
        else Rows->AddSlot().AutoHeight()[Text(TEXT("Empty slot"),14,RecoveryUI::Muted)];
    }
}
