#include "Recovery/Presentation/RecoveryForceDisplayComponent.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Interface/RecoveryPlayerController.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"
#include "Misc/App.h"

URecoveryForceDisplayComponent::URecoveryForceDisplayComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.TickGroup=TG_PostPhysics;
}
void URecoveryForceDisplayComponent::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Fn)
{
    Super::TickComponent(Dt,Type,Fn);
    const auto* D=Cast<ASuperHeavyRecoveryDirector>(GetOwner());
    const auto* PC=Cast<ARecoveryPlayerController>(GetWorld()->GetFirstPlayerController());
    if(!FApp::CanEverRender() || !D || !D->GetBody() || !PC || !PC->bForceOverlay || PC->IsMenuOpen())return;
    // Solver sample positions already contain physical centimetres. Blueprint
    // artwork scale must never be applied a second time when rebasing them.
    const FTransform Current(D->GetBody()->GetComponentQuat(),D->GetBody()->GetComponentLocation());
    const auto& Previous=D->GetForceFrame();
    static const FColor Colors[]={FColor::Cyan,FColor(195,130,255),FColor(255,170,55),FColor::Yellow,FColor(145,225,255),FColor::White};
    for(const auto& Sample:D->GetAppliedForces())
    {
        const double Magnitude=Sample.ForceN.Size();
        if(Magnitude<1.)continue;
        const FVector Start=Current.TransformPosition(Previous.InverseTransformPosition(Sample.PointCm));
        const FVector Direction=Current.TransformVectorNoScale(Previous.InverseTransformVectorNoScale(Sample.ForceN)).GetSafeNormal();
        // One common logarithmic length mapping keeps small jets and MN loads
        // readable together. The panel explicitly labels this as a log display.
        const double Length=FMath::LogX(10.,1+Magnitude/100.)*900*PC->ForceVectorScale;
        const FVector End=Start+Direction*Length;
        const FColor Color=Colors[int(Sample.Kind)];
        DrawDebugDirectionalArrow(GetWorld(),Start,End,100,Color,false,0,1,2);
        // Engine values are summarized in the panel: 33 overlapping labels hide
        // the vehicle even though the individual force arrows remain useful.
        if(Sample.Kind!=ERecoveryForceKind::Engine)
            DrawDebugString(GetWorld(),End,FString::Printf(TEXT("%.2f kN"),Magnitude/1000.),nullptr,Color,0,false,.85f);
    }
    DrawDebugSphere(GetWorld(),D->GetMassCentreCm(),90,12,FColor::White,false,0,1,2);
}
