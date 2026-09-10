#include "Recovery/Interface/RecoveryFlightDeck.h"
#include "Recovery/Interface/RecoveryPlayerController.h"
#include "Recovery/Interface/RecoveryIcon.h"
#include "Recovery/Interface/RecoveryEngineDiagram.h"
#include "Recovery/Shared/RecoveryUIStyle.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "DrawDebugHelpers.h"

TSharedRef<SWidget> SRecoveryFlightDeck::Text(TAttribute<FText> Value,int32 Size)
{return SNew(STextBlock).Text(Value).Font(FCoreStyle::GetDefaultFontStyle("Regular",Size)).ColorAndOpacity(RecoveryUI::Accent).AutoWrapText(true);}
TSharedRef<SWidget> SRecoveryFlightDeck::Button(const FString& Label,TFunction<void()> Action)
{
    return SNew(SButton).IsFocusable(false).ButtonStyle(&RecoveryUI::ButtonStyle()).ContentPadding(FMargin(12,10))
        .OnClicked_Lambda([Action](){Action();return FReply::Handled();})
        [SNew(SHorizontalBox)
        +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,8,0)[SNew(SRecoveryIcon).Label(Label).Color(RecoveryUI::Accent)]
        +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Text(FText::FromString(Label))]];
}
void SRecoveryFlightDeck::Construct(const FArguments& Args)
{
    Controller=Args._Controller;
    SetVisibility(TAttribute<EVisibility>::CreateLambda([this](){return Controller.IsValid() && !Controller->IsAtHome() && !Controller->IsMenuOpen()?EVisibility::SelfHitTestInvisible:EVisibility::Collapsed;}));
    ChildSlot[SNew(SOverlay)
        +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0,24)
        [SNew(SHorizontalBox)
            +SHorizontalBox::Slot().AutoWidth()[Button(TEXT("Cameras"),[this](){Controller->ToggleCameraPicker();})]
            +SHorizontalBox::Slot().AutoWidth()[Button(TEXT("Computer"),[this](){Controller->ToggleFlightComputer();})]
            +SHorizontalBox::Slot().AutoWidth()[Button(TEXT("Weather"),[this](){Controller->OpenWeather();})]
            +SHorizontalBox::Slot().AutoWidth()[Button(TEXT("Slower"),[this](){Controller->SlowerPlayback();})]
            +SHorizontalBox::Slot().AutoWidth()[Button(TEXT("Faster"),[this](){Controller->FasterPlayback();})]]
        +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(24,112,24,230)
        [SNew(SBox).WidthOverride(340)
            .Visibility_Lambda([this](){return Controller.IsValid() && (Controller->Selection.IsValid() || Controller->bFlightComputer)?EVisibility::Visible:EVisibility::Collapsed;})
            [SNew(SBorder).OnMouseButtonDown_Lambda([](const FGeometry&,const FPointerEvent&){return FReply::Handled();})
                .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.005f,.008f,.012f,.94f)).Padding(18)
                [SNew(SScrollBox)+SScrollBox::Slot()[SAssignNew(Details,SVerticalBox)]]]]];
    Refresh();
}
void SRecoveryFlightDeck::Refresh()
{
    if(!Details || !Controller.IsValid())return;Details->ClearChildren();
    const auto Add=[this](TSharedRef<SWidget> W){Details->AddSlot().AutoHeight().Padding(0,0,0,10)[W];};
    const auto Live=[this](TFunction<FString(const ASuperHeavyRecoveryDirector&)> Read)
    {
        return Text(TAttribute<FText>::CreateLambda([this,Read](){auto* D=Controller.IsValid()?Controller->GetDirector():nullptr;return FText::FromString(D?Read(*D):TEXT("Waiting for telemetry"));}));
    };
    if(Controller->Selection.IsValid())
    {
        Add(Text(FText::FromString(Controller->SelectionLabel()),20));
        Add(Text(TAttribute<FText>::CreateLambda([this](){return FText::FromString(Controller->SelectionStatus());})));
        Add(Button(TEXT("Disable"),[this](){Controller->SetSelectedPartFault(true);}));
        Add(Button(TEXT("Restore"),[this](){Controller->SetSelectedPartFault(false);}));
        Add(Button(TEXT("Outage / 1 simulation second"),[this](){Controller->SetSelectedPartFault(true,1.);}));
        Add(Button(TEXT("Close"),[this](){Controller->SelectPart({});}));return;
    }
    Add(Text(FText::FromString(TEXT("FLIGHT COMPUTER")),20));
    auto Tabs=SNew(SHorizontalBox);
    const TCHAR* Labels[]={TEXT("Flight"),TEXT("Systems"),TEXT("Events")};
    for(int32 I=0;I<3;++I)Tabs->AddSlot().FillWidth(1).Padding(0,0,3,0)
        [SNew(SButton).IsFocusable(false).ButtonStyle(&RecoveryUI::ButtonStyle()).HAlign(HAlign_Center).ContentPadding(FMargin(4,8))
        .OnClicked_Lambda([this,I](){Tab=I;Refresh();return FReply::Handled();})[Text(FText::FromString(Labels[I]),12)]];
    Add(Tabs);
    if(Tab==0)
    {
        Add(Text(FText::FromString(TEXT("CYAN ballistic estimate\nWHITE accepted plan / GREY flown")),12));
        Add(Live([](const auto& D){const auto& G=D.GetGuidanceState();
            if(G.bAlternateRecovery)return FString::Printf(TEXT("%s\nDivert DV %.0f / %.0f m/s"),G.bSafeAlternateAvailable?TEXT("OFFSHORE / ESTIMATED REACHABLE"):TEXT("IMPACT MITIGATION / NO SAFE CANDIDATE"),G.RequiredDivertDeltaVMps,G.AvailableDivertDeltaVMps);
            return FString::Printf(TEXT("%s\nCorrection fuel budget %.1f t"),G.bCorrectiveBurn?TEXT("ENTRY FOOTPRINT CORRECTION"):TEXT("TOWER RECOVERY"),G.CorrectionFuelBudgetKg/1000.);}));
        Add(Live([](const auto& D){const auto& G=D.GetGuidanceState();return FString::Printf(TEXT("%s\nT+%.2f s\nAltitude %.2f km\nVertical speed %+.1f m/s\nAttitude %.2f deg\nAngular rate %.2f deg/s"),*D.GetPhaseLabel(),G.SampleTimeS,D.AltitudeM/1000.,D.VerticalSpeedMps,D.TiltDeg,FMath::RadiansToDegrees(D.GetDynamicsState().Body.AngularVelocityWorldRadS.Size()));}));
        Add(Live([](const auto& D){const auto& G=D.GetGuidanceState();const auto& B=D.GetDynamicsState();
            const FVector Centre=D.BasePositionM+B.Body.Rotation.GetUpVector()*B.Mass.CentreFromBaseM;
            const FString Tracking=G.TerminalPlan.bFeasible?FString::Printf(TEXT("Tracking error %.1f m"),(G.TerminalReferenceM-Centre).Size()):TEXT("Tracking error — no accepted plan");
            return FString::Printf(TEXT("%s\nBallistic miss %.0f m\nBraking distance %.0f m\nFuel %.1f t / predicted burn %.1f t"),*Tracking,G.PredictedMissM,G.LandingPrediction.DistanceM,D.PropellantKg/1000.,G.LandingPrediction.FuelKg/1000.);}));
        Add(Live([](const auto& D){const auto& P=D.GetGuidanceState().TerminalCandidate;return FString::Printf(TEXT("Candidate constraints\nThrust %d   Attitude %d\nClearance %d   Fuel %d"),P.ThrustRejected,P.AttitudeRejected,P.ClearanceRejected,P.FuelRejected);}));
        Add(Live([](const auto& D){const auto& T=D.GetDynamicsState().Tower;return FString::Printf(TEXT("Rail loads   %.2f / %.2f MN\nContacts %d / 2"),T.RailLoadN[0]/1.e6,T.RailLoadN[1]/1.e6,D.SupportContactCount);}));
        Add(Button(TEXT("Force vectors"),[this](){Controller->ToggleForceOverlay();}));
    }
    else if(Tab==1)
    {
        Add(Text(FText::FromString(TEXT("ENGINES / UNDERSIDE")),12));
        Add(SNew(SRecoveryEngineDiagram).Controller(Controller.Get()));
        for(int32 I=0;I<3;++I)Add(Button(FString::Printf(TEXT("Grid fin %d"),I+1),[this,I](){Controller->SelectPart({ERecoveryPart::Fin,I});}));
        Add(Button(TEXT("RCS manifold"),[this](){Controller->SelectPart({ERecoveryPart::Rcs,0});}));
        Add(Button(TEXT("Reset experiments"),[this](){if(auto* D=Controller->GetDirector())D->ResetExperiments();}));
        Add(Live([](const auto& D){return FString::Printf(TEXT("Attitude response  %.2f x"),D.GetExperiment().AttitudeResponse);}));
        Add(SNew(SSlider).Style(&RecoveryUI::ControlSliderStyle())
            .Value_Lambda([this](){const auto* D=Controller->GetDirector();return D?float(D->GetExperiment().AttitudeResponse-.5):.5f;})
            .OnValueChanged_Lambda([this](float V){if(auto* D=Controller->GetDirector())D->SetAttitudeResponse(.5+V);}));
        Add(Live([](const auto& D){return FString::Printf(TEXT("Wind multiplier  %.2f x"),D.GetExperiment().WindScale);}));
        Add(SNew(SSlider).Style(&RecoveryUI::ControlSliderStyle())
            .Value_Lambda([this](){const auto* D=Controller->GetDirector();return D?float(D->GetExperiment().WindScale/3):1.f/3;})
            .OnValueChanged_Lambda([this](float V){if(auto* D=Controller->GetDirector())D->SetWindScale(V*3);}));
    }
    else Add(Live([](const auto& D){FString Text;const auto& Events=D.GetMissionEvents();for(int32 I=FMath::Max(0,Events.Num()-12);I<Events.Num();++I)Text+=Events[I]+TEXT("\n\n");return Text;}));
}

void SRecoveryFlightDeck::Tick(const FGeometry& Geometry,double Time,float DeltaTime)
{
    SCompoundWidget::Tick(Geometry,Time,DeltaTime);
    const auto* PC=Controller.Get();const auto* D=PC?PC->GetDirector():nullptr;if(!D)return;
    if(Generation!=D->GetMissionGeneration()){Generation=D->GetMissionGeneration();FlownPath.Reset();LastSample=-1;}
    if(D->MissionTime>LastSample+.5){FlownPath.Add(D->BasePositionM);LastSample=D->MissionTime;if(FlownPath.Num()>1500)FlownPath.RemoveAt(0);}
    if(!PC->bFlightComputer || PC->IsMenuOpen())return;
    auto* World=D->GetWorld();const auto& G=D->GetGuidanceState();
    const auto Path=[World](const TArray<FVector>& Points,FColor Color){for(int32 I=1;I<Points.Num();++I)DrawDebugLine(World,Points[I-1]*100,Points[I]*100,Color,false,0,0,1.5f);};
    Path(FlownPath,FColor(120,135,150));Path(G.BallisticPathM,FColor(65,205,235));
    for(int32 I=0;I<G.AlternateSitesM.Num();++I)
    {
        const FVector Site=G.AlternateSitesM[I]*100;
        const FColor Color=G.AlternateReachable.IsValidIndex(I) && G.AlternateReachable[I]?FColor(235,155,65):FColor(100,105,110);
        DrawDebugCircle(World,Site,50000,32,Color,false,0,0,2,FVector::ForwardVector,FVector::RightVector,false);
        DrawDebugString(World,Site,FString::Printf(TEXT("ALT %d / estimated lateral DV %.0f m/s"),I+1,G.AlternateDeltaVMps[I]),nullptr,Color,0,false,1);
    }
    if(G.TerminalPlan.bFeasible)
    {
        TArray<FVector> Points;for(int32 I=0;I<=80;++I)Points.Add(G.TerminalPlan.PositionAt(FMath::Lerp(G.TerminalPlan.ElapsedS,G.TerminalPlan.HorizonS,I/80.)));
        Path(Points,FColor::White);
    }
}
