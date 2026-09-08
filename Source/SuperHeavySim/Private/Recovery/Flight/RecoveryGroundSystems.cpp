#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Components/PrimitiveComponent.h"

void ASuperHeavyRecoveryDirector::TickLaunchSequence(double Dt)
{
    const bool SystemsReady=Experiment.FailedEngine<0 && Experiment.JammedFin<0 &&
        !Experiment.bReactionJetsDisabled && PropellantKg>RuntimeProfile->LandingReserveKg;
    bool ThrustVerified=Engines.Num()==33 && ActualThrustN>MassKg*Gravity*1.1;
    for(const auto& Engine:Engines)
        ThrustVerified&=Engine.ThrustN>RuntimeProfile->EngineThrustN*.9;
    const FString Previous=LaunchSequence.Label();
    LaunchSequence.Advance(Dt,SystemsReady,ThrustVerified);
    MissionTime=-LaunchSequence.RemainingS;
    StatusMessage=LaunchSequence.Label();
    if(Previous!=StatusMessage)
    {
        PhaseEvents.Add(FString::Printf(TEXT("GROUND %.3fs %s"),MissionTime,*StatusMessage));
        UE_LOG(LogTemp,Display,TEXT("GROUND_SEQUENCE t=%.3f event=%s"),MissionTime,*StatusMessage);
    }
    if(LaunchSequence.bAborted)
    {
        SetPhase(ERecoveryPhase::Aborted,TEXT("Launch interlock failed / mount retained"));
        ActiveEngines=0;
        WriteResult(false,StatusMessage);
    }
    else if(LaunchSequence.bReleaseRequested)
    {
        ReleaseLaunchHoldDown();MissionTime=0;
        SetPhase(ERecoveryPhase::Ascent,TEXT("Mount released / thrust verified"));
    }
}

void ASuperHeavyRecoveryDirector::TickGroundConditioning(double Dt)
{
    // A bounded conditioning-flow model, not a full thermodynamic tank model.
    // Ground replenishment is possible only while the launch umbilicals remain
    // connected. Disconnect closes both valves at a finite mechanical rate.
    GroundClockS+=Dt;
    const bool Connected=!bLaunchHoldReleased && LaunchSequence.IsGroundSupplyConnected();
    const double DelugeDemand=Phase==ERecoveryPhase::Countdown || Phase==ERecoveryPhase::Aborted ? LaunchSequence.DelugeDemand() :
        (Phase==ERecoveryPhase::Ascent && AltitudeM<140) || (Phase>=ERecoveryPhase::LandingBurn && Phase<=ERecoveryPhase::Capture && AltitudeM<220) ? 1. : 0.;
    DelugeFlow+=FMath::Clamp(DelugeDemand-DelugeFlow,-Dt/3.,Dt/1.5);
    const double Nominal=RuntimeProfile->ConditioningVentKgS;
    for(int I=0;I<2;++I)
    {
        const double Maximum=Nominal*(I==0?2./3.:1./3.);
        // Replenished conditioning flow pulses smoothly and independently at each
        // vent. This is a valve-duty estimate, not a thermodynamic tank solution.
        const double Duty=.55+.45*FMath::Square(FMath::Sin(GroundClockS*.32+I*1.8));
        const double Target=Connected?Maximum*Duty:0.;
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
