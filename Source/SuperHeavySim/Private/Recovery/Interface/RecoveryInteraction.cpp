#include "Recovery/Interface/RecoveryInteraction.h"
#include "Recovery/Interface/RecoveryPlayerController.h"
#include "Recovery/Interface/RecoveryFlightDeck.h"
#include "Recovery/Interface/RecoveryMenu.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Camera/PlayerCameraManager.h"
#include "Framework/Application/SlateApplication.h"

void ARecoveryPlayerController::BeginOrbitDrag()
{
    if(bMenuOpen || bAtHome)return;
    float X=0,Y=0;GetMousePosition(X,Y);OrbitPointer=FVector2D(X,Y);
    bOrbitDragging=true;bShowMouseCursor=false;
}
void ARecoveryPlayerController::EndOrbitDrag()
{
    if(!bOrbitDragging)return;
    bOrbitDragging=false;bShowMouseCursor=true;
    SetMouseLocation(FMath::RoundToInt(OrbitPointer.X),FMath::RoundToInt(OrbitPointer.Y));
}
void ARecoveryPlayerController::SelectPart(FRecoverySelection Part)
{Selection=Part;bFlightComputer=false;if(FlightDeck)FlightDeck->Refresh();}
void ARecoveryPlayerController::OpenWeather()
{if(Menu){Menu->ShowPage(ERecoveryMenuPage::Environment);SetMenuVisible(true);}}
void ARecoveryPlayerController::SetWeatherPreset(int32 Index)
{
    WeatherPreset=FMath::Clamp(Index,0,3);
    const double Wind[]={.3,.5,1.,1.4};
    if(auto* D=GetDirector())D->SetWindScale(Wind[WeatherPreset]);
    SavePreferences();
}

void ARecoveryPlayerController::SelectUnderCursor()
{
    if(bMenuOpen || bAtHome || bOrbitDragging)return;
    auto* D=GetDirector();if(!D || !D->GetBody())return;
    FVector Ray,Direction;if(!DeprojectMousePositionToWorld(Ray,Direction))return;
    const FVector Base=FlightGeometry::BoosterBaseCm(*D->GetBody());
    const FQuat Q=D->GetBody()->GetComponentQuat();
    float X=0,Y=0;if(!GetMousePosition(X,Y))return;const FVector2D Mouse(X,Y);
    double Best=22.;FRecoverySelection Found;
    const auto Consider=[&](FVector Local,FVector Normal,ERecoveryPart Kind,int32 Index)
    {
        const FVector P=Base+Q.RotateVector(Local)*100;
        if(FVector::DotProduct(Ray-P,Q.RotateVector(Normal))<=0)return;
        FVector2D Pixel;if(!ProjectWorldLocationToScreen(P,Pixel))return;
        const double Distance=FVector2D::Distance(Pixel,Mouse);if(Distance>=Best)return;
        FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(RecoveryPartSelection),true);
        // The real scene occludes selection; query never changes body collision.
        if(GetWorld()->LineTraceSingleByChannel(Hit,Ray,P,ECC_Visibility,Query) && FVector::Distance(Hit.ImpactPoint,P)>250)return;
        Best=Distance;Found={Kind,Index};
    };
    for(int32 I=0;I<D->GetEngines().Num();++I)Consider(D->GetEngines()[I].PositionFromBaseM,FVector::DownVector,ERecoveryPart::Engine,I);
    const FVector Fins[]={{6.2,0,64.45},{-6.2,0,64.45},{0,6.2,64.45}};
    for(int32 I=0;I<3;++I)Consider(Fins[I],FVector(Fins[I].X,Fins[I].Y,0),ERecoveryPart::Fin,I);
    const auto& Rcs=FlightGeometry::ReactionNozzlePositionsM();
    for(int32 I=0;I<Rcs.Num();++I)Consider(Rcs[I],FVector(Rcs[I].X,Rcs[I].Y,0),ERecoveryPart::Rcs,I);
    SelectPart(Found);
}
FString ARecoveryPlayerController::SelectionLabel() const
{
    const auto* D=GetDirector();if(!D)return TEXT("");
    if(Selection.Kind==ERecoveryPart::Engine && D->GetEngines().IsValidIndex(Selection.Index))return D->GetEngines()[Selection.Index].Id.ToString();
    if(Selection.Kind==ERecoveryPart::Fin)return FString::Printf(TEXT("GRID FIN %02d"),Selection.Index+1);
    if(Selection.Kind==ERecoveryPart::Rcs)return TEXT("REACTION CONTROL");
    return TEXT("");
}
FString ARecoveryPlayerController::SelectionStatus() const
{
    const auto* D=GetDirector();if(!D)return TEXT("");
    const auto E=D->GetExperiment().AtTime(FMath::Max(0.,D->MissionTime));
    if(Selection.Kind==ERecoveryPart::Engine && D->GetEngines().IsValidIndex(Selection.Index))
        return FString::Printf(TEXT("%s  /  %.1f kN"),E.FailedEngine==Selection.Index?TEXT("FAILED"):TEXT("AVAILABLE"),D->GetEngines()[Selection.Index].ThrustN/1000.);
    if(Selection.Kind==ERecoveryPart::Fin && Selection.Index>=0 && Selection.Index<3)
        return FString::Printf(TEXT("%s  /  %.1f deg"),E.JammedFin==Selection.Index?TEXT("JAMMED"):TEXT("AVAILABLE"),D->GridFinAnglesDeg[Selection.Index]);
    return E.bReactionJetsDisabled?TEXT("MANIFOLD DISABLED"):TEXT("MANIFOLD AVAILABLE");
}
void ARecoveryPlayerController::SetSelectedPartFault(bool Disabled,double DurationS)
{
    auto* D=GetDirector();if(!D || !Selection.IsValid())return;
    if(Disabled && DurationS>0){D->SetTimedFault(int32(Selection.Kind),Selection.Index,DurationS);return;}
    if(Selection.Kind==ERecoveryPart::Engine && (Disabled || D->GetExperiment().FailedEngine==Selection.Index))D->SetFailedEngine(Disabled?Selection.Index:INDEX_NONE);
    if(Selection.Kind==ERecoveryPart::Fin && (Disabled || D->GetExperiment().JammedFin==Selection.Index))D->SetJammedFin(Disabled?Selection.Index:INDEX_NONE);
    if(Selection.Kind==ERecoveryPart::Rcs)D->SetReactionJetsDisabled(Disabled);
}
