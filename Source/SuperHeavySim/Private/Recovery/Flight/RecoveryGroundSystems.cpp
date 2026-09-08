#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Components/PrimitiveComponent.h"

void ASuperHeavyRecoveryDirector::TickGroundConditioning(double Dt)
{
    // A bounded conditioning-flow model, not a full thermodynamic tank model.
    // Ground replenishment is possible only while the launch umbilicals remain
    // connected. Disconnect closes both valves at a finite mechanical rate.
    const bool Connected=Phase==ERecoveryPhase::Ready || (Phase==ERecoveryPhase::Countdown && PhaseTime<3.);
    const double Nominal=RuntimeProfile->ConditioningVentKgS;
    for(int I=0;I<2;++I)
    {
        const double Maximum=Nominal*(I==0?2./3.:1./3.);
        const double Target=Connected?Maximum:0.;
        ConditioningFlowKgS[I]+=FMath::Clamp(Target-ConditioningFlowKgS[I],-Maximum*Dt/.35,Maximum*Dt/.6);
    }
    const double Requested=(ConditioningFlowKgS.X+ConditioningFlowKgS.Y)*Dt;
    const double Used=FMath::Min(PropellantKg,Requested);
    ConditioningFlowKgS*=Requested>0?Used/Requested:0;
    PropellantKg=FMath::Max(0.,PropellantKg-Used);
    ConditioningVentedKg+=Used;
    if(Connected){PropellantKg+=Used;GroundSupplyKg+=Used;}
    if(Used<=0)return;
    UpdateMass();
    const FQuat Q=Body->GetComponentQuat();
    const FVector Base=FlightGeometry::BoosterBaseCm(*Body);
    const auto& Positions=FlightGeometry::ConditioningVentPositionsM();
    const auto& Outflow=FlightGeometry::ConditioningVentDirections();
    for(int I=0;I<2;++I)
        ApplyVehicleForce(ERecoveryForceKind::Vent,I,Q.RotateVector(-Outflow[I]*ConditioningFlowKgS[I]*RuntimeProfile->ConditioningJetSpeedMps),
            Base+Q.RotateVector(Positions[I])*100);
}
