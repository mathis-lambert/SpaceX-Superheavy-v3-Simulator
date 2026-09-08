#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Flight/RecoveryPhysicsComponent.h"
#include "Recovery/Flight/SuperHeavyLaunchTower.h"
#include "Components/PrimitiveComponent.h"
#include "Recovery/Shared/FlightGeometry.h"

void ASuperHeavyRecoveryDirector::InitializeDynamics()
{
    DynamicsState=FRecoveryDynamicsState();
    DynamicsState.Mass=RecoveryMass::Booster(*RuntimeProfile,PropellantKg,RcsPropellantKg,!bSeparated);
    DynamicsState.PropellantKg=PropellantKg;DynamicsState.RcsPropellantKg=RcsPropellantKg;
    auto Configuration=URecoveryPhysicsComponent::BuildGuidanceConfiguration(*RuntimeProfile);
    Configuration.CaptureWorldM=CaptureWorldM;Configuration.LaunchWorldM=LaunchWorldM;
    Configuration.TowerRotation=Tower->GetActorQuat();Configuration.TowerHeightM=Tower->TowerHeightM;
    GuidanceConfiguration=Configuration;
    GuidanceState=FRecoveryGuidanceState();ConsumedGuidanceEvents=0;
    PhysicsModel->InitializeMission(Configuration,Engines,PropellantKg,RcsPropellantKg,MissionGeneration);
    AppliedForces.Reset();
    DynamicsCommand=FRecoveryDynamicsCommand();
    UpdateNavigation();PrepareGroundCommand();SubmitDynamicsCommand();
}

void ASuperHeavyRecoveryDirector::SubmitDynamicsCommand()
{
    DynamicsCommand.Phase=Phase;DynamicsCommand.EngineCount=ActiveEngines;
    DynamicsCommand.bSeparated=bSeparated;DynamicsCommand.bContactShutdown=bContactShutdown;
    DynamicsCommand.Experiment=Experiment;
    DynamicsCommand.SupportContactCount=SupportContactCount;
    DynamicsCommand.bExternalFlightFixture=!ContactFixture.IsEmpty();
    DynamicsCommand.HeadingWorld=Tower->GetActorQuat().RotateVector(FRotator(0,RuntimeProfile->CaptureHeadingDeg,0).Vector());
    if(Phase>=ERecoveryPhase::Captured)
    {
        DynamicsCommand.EngineCount=0;
        DynamicsCommand.ThrustAccelerationMps2=FVector::ZeroVector;
    }
    PhysicsModel->Submit(*Body,DynamicsCommand);
}

void ASuperHeavyRecoveryDirector::ConsumeDynamicsState()
{
    if(!PhysicsModel->Consume(DynamicsState,GuidanceState))return;
    const auto& S=DynamicsState;
    // Compatibility with the authored HUD/Blueprint properties is a read-only
    // projection. No consumer writes actuator or tank state back to the solver.
    MassKg=S.Mass.MassKg;PropellantKg=S.PropellantKg;RcsPropellantKg=S.RcsPropellantKg;
    Engines=S.Engines;ReactionForcesBodyN=S.ReactionForcesBodyN;AppliedForces=S.Forces;
    AppliedForceFrame=FTransform(S.Body.Rotation,S.Body.OriginM*100.);
    MainFuelConsumedKg=S.MainFuelConsumedKg;ConditioningVentedKg=S.ConditioningVentedKg;GroundSupplyKg=S.GroundSupplyKg;
    ConditioningFlowKgS=S.ConditioningFlowKgS;DelugeFlow=S.DelugeFlow;
    ActualThrustN=S.ThrustN;Throttle=S.Throttle;
    GridFinAnglesDeg=S.GridFinAnglesDeg;GridFinAuthority=S.GridFinAuthority;FinControlSeconds=S.FinControlSeconds;
    AeroForceN=S.AeroForceN;RcsTorqueBody=S.RcsMomentBodyNm;EngineIspS=S.EngineIspS;
    LastEngineForceBodyN=S.EngineForceBodyN;LastEngineMomentBodyNm=S.EngineMomentBodyNm;
    PeakEngineForceRatio=S.PeakEngineForceRatio;PeakAppliedGimbalDeg=S.PeakGimbalDeg;
    AppliedGimbal=FVector(S.EngineForceBodyN.X,S.EngineForceBodyN.Y,0);
    ConsumeGuidanceState();
}

FVector ASuperHeavyRecoveryDirector::GetMassCentreCm() const
{
    return Body?FlightGeometry::BoosterBaseCm(*Body)+Body->GetUpVector()*DynamicsState.Mass.CentreFromBaseM*100.:FVector::ZeroVector;
}
