#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Flight/RecoveryPhysicsComponent.h"
#include "Recovery/Flight/SuperHeavyLaunchTower.h"
#include "Components/PrimitiveComponent.h"
#include "Components/BoxComponent.h"
#include "Recovery/Shared/FlightGeometry.h"

void ASuperHeavyRecoveryDirector::InitializeDynamics()
{
    DynamicsState=FRecoveryDynamicsState();
    DynamicsState.Mass=RecoveryMass::Booster(*RuntimeProfile,PropellantKg,RcsPropellantKg,!bSeparated);
    DynamicsState.PropellantKg=PropellantKg;DynamicsState.RcsPropellantKg=RcsPropellantKg;
    auto Configuration=URecoveryPhysicsComponent::BuildGuidanceConfiguration(*RuntimeProfile);
    Configuration.CaptureWorldM=CaptureWorldM;Configuration.LaunchWorldM=LaunchWorldM;
    Configuration.TowerRotation=Tower->GetActorQuat();Configuration.TowerHeightM=Tower->TowerHeightM;
    Configuration.TowerWorldM=Tower->GetActorLocation()/100.;
    GuidanceConfiguration=Configuration;
    GuidanceState=FRecoveryGuidanceState();ConsumedGuidanceEvents=0;
    UpperStageState=FRecoveryUpperStageState();
    PhysicsModel->InitializeMission(Configuration,Engines,PropellantKg,RcsPropellantKg,
        URecoveryPhysicsComponent::BuildUpperStageConfiguration(*RuntimeProfile),MissionGeneration);
    AppliedForces.Reset();
    DynamicsCommand=FRecoveryDynamicsCommand();
    UpdateNavigation();PrepareGroundCommand();SubmitDynamicsCommand();
}

void ASuperHeavyRecoveryDirector::SubmitDynamicsCommand()
{
    DynamicsCommand.Phase=Phase;DynamicsCommand.EngineCount=ActiveEngines;
    DynamicsCommand.bSeparated=bSeparated;DynamicsCommand.bContactShutdown=bContactShutdown;
    DynamicsCommand.Experiment=Experiment;
    DynamicsCommand.bExternalFlightFixture=!ContactFixture.IsEmpty();
    DynamicsCommand.HeadingWorld=Tower->GetActorQuat().RotateVector(FRotator(0,RuntimeProfile->CaptureHeadingDeg,0).Vector());
    if(Phase>=ERecoveryPhase::Captured)
    {
        DynamicsCommand.EngineCount=0;
        DynamicsCommand.ThrustAccelerationMps2=FVector::ZeroVector;
    }
    PhysicsModel->Submit(*Body,bSeparated?UpperStageBody.Get():nullptr,*Tower,DynamicsCommand);
}

void ASuperHeavyRecoveryDirector::ConsumeDynamicsState()
{
    if(!PhysicsModel->Consume(DynamicsState,GuidanceState,UpperStageState))return;
    UpperStagePropellantKg=UpperStageState.PropellantKg;UpperStageThrustN=UpperStageState.ThrustN;
    UpperStageFuelConsumedKg=UpperStageState.FuelConsumedKg;
    if(UpperStageState.bSeparated)
    {
        SeparationMassKg=UpperStageState.SeparationMassKg;
        SeparationVelocityErrorMps=UpperStageState.SeparationVelocityErrorMps;
        SeparationMomentumRelativeError=UpperStageState.SeparationMomentumRelativeError;
        SeparationAngularMomentumRelativeError=UpperStageState.SeparationAngularMomentumRelativeError;
    }
    const auto& S=DynamicsState;
    Tower->RailLoadN=S.Tower.RailLoadN;Tower->PeakRailLoadN=S.Tower.PeakRailLoadN;
    Tower->BrokenRailMask=S.Tower.BrokenRails;Tower->BrokenHingeMask=S.Tower.BrokenHinges;
    SupportContactCount=S.RailSupport.Count();SupportImpulseNs=S.RailSupport.ImpulseNs;
    EverSupportContact[0]=(S.RailSupport.EverMask&1)!=0;EverSupportContact[1]=(S.RailSupport.EverMask&2)!=0;
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
