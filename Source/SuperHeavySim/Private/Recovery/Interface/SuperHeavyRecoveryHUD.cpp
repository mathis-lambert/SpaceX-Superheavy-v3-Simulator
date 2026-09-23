#include "Recovery/Interface/SuperHeavyRecoveryHUD.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Interface/RecoveryPlayerController.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

/** Slate glyphs are rasterized at viewport resolution, after scene postprocessing. */
class SRecoveryTelemetry : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SRecoveryTelemetry) {} SLATE_ARGUMENT(ASuperHeavyRecoveryDirector*,Director) SLATE_END_ARGS()
    void Construct(const FArguments& Args){Director=Args._Director;SetVisibility(EVisibility::HitTestInvisible);}
    virtual FVector2D ComputeDesiredSize(float) const override{return FVector2D(1920,1080);}
    virtual bool ComputeVolatility() const override{return true;}
    virtual int32 OnPaint(const FPaintArgs& Args,const FGeometry& G,const FSlateRect& Clip,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool Enabled) const override
    {
        const auto* D=Director.Get();if(!D || !D->bShowTelemetry)return Layer;
        const auto* PC=Cast<ARecoveryPlayerController>(D->GetWorld()->GetFirstPlayerController());
        if(PC && PC->IsMenuOpen())return Layer;
        const FVector2D View=G.GetLocalSize();const float W=View.X,H=View.Y,S=FMath::Min(W/1920.f,H/1080.f);
        const FLinearColor White(.95f,.96f,.98f),Grey(.5f,.54f,.59f),Dim(.12f,.14f,.17f,.8f),Black(.002f,.003f,.005f,.76f);
        const FLinearColor Status=D->Phase==ERecoveryPhase::Aborted?FLinearColor(1.f,.2f,.12f):White;
        const auto Rect=[&](float X,float Y,float Width,float Height,FLinearColor Color)
        { FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(FVector2f(Width*S,Height*S),FSlateLayoutTransform(FVector2f(X*S,Y*S))),FCoreStyle::Get().GetBrush("WhiteBrush"),ESlateDrawEffect::None,Color); };
        const auto Line=[&](float X,float Y,float X2,float Y2,FLinearColor Color,float Thickness=1.f)
        { TArray<FVector2D> P={FVector2D(X*S,Y*S),FVector2D(X2*S,Y2*S)};FSlateDrawElement::MakeLines(Out,Layer+1,G.ToPaintGeometry(),P,ESlateDrawEffect::None,Color,true,Thickness*S); };
        const auto Text=[&](const FString& T,float X,float Y,int32 Size,FLinearColor Color,bool Bold=false)
        { const FSlateFontInfo Font=FCoreStyle::GetDefaultFontStyle(Bold?"Bold":"Regular",FMath::Max(9,FMath::RoundToInt(Size*S)));FSlateDrawElement::MakeText(Out,Layer+2,G.ToPaintGeometry(FVector2f(1,1),FSlateLayoutTransform(FVector2f(X*S,Y*S))),T,Font,ESlateDrawEffect::None,Color); };
        const auto Circle=[&](float X,float Y,float Radius,FLinearColor Color,float Thickness)
        { TArray<FVector2D> P;for(int I=0;I<=24;++I){const float A=I*2*PI/24;P.Add(FVector2D((X+FMath::Cos(A)*Radius)*S,(Y+FMath::Sin(A)*Radius)*S));}FSlateDrawElement::MakeLines(Out,Layer+1,G.ToPaintGeometry(),P,ESlateDrawEffect::None,Color,true,Thickness*S); };
        const float VW=W/S,VH=H/S,BY=VH-188;
        double TotalThrustN=0,MaxGimbalDeg=0;int32 BurningEngines=0;
        for(const auto& Engine:D->GetEngines())
        {
            TotalThrustN+=Engine.ThrustN;
            if(Engine.ThrustN>1000)++BurningEngines;
            MaxGimbalDeg=FMath::Max(MaxGimbalDeg,FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Engine.DirectionBody.Z,-1.,1.))));
        }
        Rect(0,BY,VW,188,Black);Line(32,BY,VW-32,BY,FLinearColor(.6f,.62f,.66f,.35f));
        for(int Ring=0;Ring<3;++Ring)
        {
            const int N=Ring==0?20:Ring==1?10:3;const float R=Ring==0?59:Ring==1?38:15;
            int I=0;
            for(const auto& Engine:D->GetEngines())
            {
                const int Group=Engine.bCentral?2:Engine.bGimballed?1:0;if(Group!=Ring)continue;
                const bool On=Engine.ThrustN>1000;const float A=I++*2*PI/N-PI/2;
                Circle(100+FMath::Cos(A)*R,BY+87+FMath::Sin(A)*R,6,On?White:Grey,On?4.f:1.1f);
            }
        }
        Text(TEXT("SUPER HEAVY"),41,BY+157,11,Grey,true);
        Text(TEXT("SPEED"),202,BY+22,13,Grey,true);
        Text(FString::Printf(TEXT("%05.0f"),D->VelocityMps.Size()*3.6),202,BY+43,29,White,true);Text(TEXT("KM/H"),332,BY+58,12,Grey);
        Text(TEXT("ALTITUDE"),202,BY+93,13,Grey,true);
        Text(D->AltitudeM>=1000?FString::Printf(TEXT("%06.2f"),D->AltitudeM/1000):FString::Printf(TEXT("%04.0f"),D->AltitudeM),202,BY+114,29,White,true);
        Text(D->AltitudeM>=1000?TEXT("KM"):TEXT("M"),332,BY+129,12,Grey);
        Line(405,BY+24,405,BY+152,Dim);
        const float CX=VW*.5f;const bool Counting=D->Phase==ERecoveryPhase::Countdown;
        const int Sec=Counting?FMath::CeilToInt(D->GetLaunchSequence().RemainingS):D->MissionTime<0?FMath::CeilToInt(-D->MissionTime):FMath::FloorToInt(D->MissionTime);
        Text(FString::Printf(TEXT("T%s%02d:%02d:%02d"),Counting || D->MissionTime<0?TEXT("-"):TEXT("+"),Sec/3600,(Sec/60)%60,Sec%60),CX-130,BY+30,35,White,true);
        Text(TEXT("STARBASE  /  RETURN TO LAUNCH SITE"),CX-139,BY+78,12,Grey,true);
        Text(Counting?D->GetLaunchSequence().Label():D->GetPhaseLabel(),CX-(Counting?145:100),BY+115,Counting?15:22,Status,true);
        const float RX=VW-470;
        Text(TEXT("MASS"),RX,BY+22,13,Grey,true);Text(FString::Printf(TEXT("%7.1f T"),D->MassKg/1000),RX+150,BY+16,24,White,true);
        Text(TEXT("PROPELLANT"),RX,BY+67,13,Grey,true);
        const double Fuel=D->GetProfile()?D->PropellantKg/D->GetProfile()->PropellantMassKg:0;
        Rect(RX+150,BY+74,234,5,Dim);Rect(RX+150,BY+74,234*Fuel,5,White);
        Text(FString::Printf(TEXT("%6.1f T"),D->PropellantKg/1000),RX+280,BY+89,12,Grey);
        Text(TEXT("RAIL CONTACTS"),RX,BY+118,12,Grey,true);
        Circle(RX+238,BY+128,8,D->SupportContactCount>0?White:Grey,D->SupportContactCount>0?6.f:1.f);
        Circle(RX+270,BY+128,8,D->SupportContactCount==2?White:Grey,D->SupportContactCount==2?6.f:1.f);
        Text(FString::Printf(TEXT("%02d ENGINES   /   %.2f MN"),BurningEngines,TotalThrustN/1.e6),RX,BY+157,11,Grey,true);
        const float RailY=BY-22;static const TCHAR* Phases[]={TEXT("ASCENT"),TEXT("SEPARATION"),TEXT("BOOSTBACK"),TEXT("COAST"),TEXT("ENTRY"),TEXT("LANDING BURN"),TEXT("APPROACH"),TEXT("CAPTURE")};
        const int Current=D->IsLaunchMountReleased()?FMath::Clamp(int(D->LastFlightPhase)-2,0,7):-1;const float Step=(VW-96)/8;
        for(int I=0;I<8;++I){const float X=48+I*Step;Rect(X,RailY,Step-12,2,I<=Current?White:Dim);Text(Phases[I],X,RailY-22,10,I==Current?White:Grey);}
        Rect(28,24,245,43,FLinearColor(0,0,0,.4f));Text(TEXT("STARBASE / LIVE"),43,34,14,White,true);
        Rect(VW-385,24,357,58,FLinearColor(0,0,0,.5f));Text(D->Viewer->GetCameraLabel(),VW-370,33,14,White);
        if(PC)Text(FString::Printf(TEXT("TAB  CAMERAS    ESC  MENU    %.2f× / %.2f×"),PC->EffectivePlaybackRate,PC->PlaybackRate),VW-370,61,10,Grey);
        if(D->IsLaunchMountReleased() && D->LastFlightPhase>=ERecoveryPhase::LandingBurn)
            Text(FString::Printf(TEXT("VZ %+.2f M/S    AXIS %.2f M    HEADING %.1f°"),D->VerticalSpeedMps,D->HorizontalErrorM,D->HeadingErrorDeg),48,BY-76,13,White);
        if(PC && PC->bForceOverlay)
        {
            const auto& Experiment=D->GetExperiment();
            const FString Engine=D->GetEngines().IsValidIndex(Experiment.FailedEngine)?D->GetEngines()[Experiment.FailedEngine].Id.ToString():TEXT("NONE");
            Rect(28,112,440,252,FLinearColor(.002f,.004f,.008f,.85f));
            Text(TEXT("FLIGHT DYNAMICS / LIVE"),44,128,18,White,true);
            Text(TEXT("Forces · logarithmic arrow scale"),44,161,11,Grey);
            Text(TEXT("CYAN engines   WHITE gravity / mass centre"),44,186,12,White);
            Text(TEXT("ORANGE fins   VIOLET drag   YELLOW reaction jets"),44,210,12,White);
            Text(FString::Printf(TEXT("THRUST %.2f MN    Q %.1f KPA    MACH %.2f"),TotalThrustN/1.e6,D->DynamicPressurePa/1000.,D->Mach),44,240,11,White);
            Text(FString::Printf(TEXT("FAILED ENGINE %s    JAMMED FIN %d"),*Engine,Experiment.JammedFin+1),44,266,11,White);
            Text(FString::Printf(TEXT("RCS %s    RESPONSE %.1f×    WIND %.1f×"),Experiment.bReactionJetsDisabled?TEXT("OFF"):TEXT("ON"),Experiment.AttitudeResponse,Experiment.WindScale),44,292,11,White);
            Text(TEXT("L  COMPUTER      I  CLOSE"),44,331,10,Grey);
        }
        return Layer+3;
    }
private:TWeakObjectPtr<ASuperHeavyRecoveryDirector> Director;
};
ASuperHeavyRecoveryGameMode::ASuperHeavyRecoveryGameMode()
{DefaultPawnClass=nullptr;HUDClass=ASuperHeavyRecoveryHUD::StaticClass();PlayerControllerClass=ARecoveryPlayerController::StaticClass();}
void ASuperHeavyRecoveryHUD::DrawHUD()
{
    Super::DrawHUD();
    if(!TelemetryWidget && GEngine && GEngine->GameViewport)
        for(TActorIterator<ASuperHeavyRecoveryDirector> It(GetWorld());It;++It)
        {TelemetryWidget=SNew(SRecoveryTelemetry).Director(*It);GEngine->GameViewport->AddViewportWidgetContent(TelemetryWidget.ToSharedRef(),10);break;}
}
void ASuperHeavyRecoveryHUD::EndPlay(const EEndPlayReason::Type Reason)
{
    if(TelemetryWidget && GEngine && GEngine->GameViewport)GEngine->GameViewport->RemoveViewportWidgetContent(TelemetryWidget.ToSharedRef());
    TelemetryWidget.Reset();Super::EndPlay(Reason);
}
