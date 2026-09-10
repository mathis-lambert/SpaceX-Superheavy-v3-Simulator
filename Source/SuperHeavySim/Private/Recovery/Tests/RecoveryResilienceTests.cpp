#include "Recovery/Flight/RecoveryFlightInspection.h"
#include "Recovery/Flight/RecoveryDynamicsModel.h"
#include "Recovery/Flight/RecoveryPhysicsComponent.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryTimedFaultTest,"Recovery.Physics.TimedFaults",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryTimedFaultTest::RunTest(const FString&)
{
    FRecoveryFlightExperiment E;E.Schedule={{3,0,193,1},{3,0,219,1},{2,1,220,3}};
    for(int32 Hz:{15,60,120})
    {
        int32 Off=0;
        for(int32 Step=0;Step<225*Hz;++Step)if(E.AtTime(double(Step)/Hz).bReactionJetsDisabled)++Off;
        TestEqual(TEXT("Two one-second outages, independent of playback cadence"),Off,Hz*2);
    }
    TestFalse(TEXT("Right endpoint restores the manifold"),E.AtTime(194).bReactionJetsDisabled);
    TestEqual(TEXT("A fin fault coexists with RCS restoration"),E.AtTime(220).JammedFin,1);
    E.FailedEngine=8;E.EngineRestoreTimeS=10;
    TestEqual(TEXT("Timed engine restoration reaches physics"),E.AtTime(10).FailedEngine,INDEX_NONE);
    TestEqual(TEXT("Snapshot resolution does not mutate the command"),E.FailedEngine,8);
    E=FRecoveryFlightExperiment();E.FailedEngine=2;E.EngineRestoreTimeS=1;
    E.AdvanceGroundFaults(.75);TestEqual(TEXT("A timed preflight fault remains during its interval"),E.FailedEngine,2);
    E.AdvanceGroundFaults(.25);TestEqual(TEXT("A held countdown cannot trap a timed fault"),E.FailedEngine,INDEX_NONE);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryResidualControlTest,"Recovery.Physics.ResidualControl",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryResidualControlTest::RunTest(const FString&)
{
    const auto Config=URecoveryPhysicsComponent::BuildConfiguration(*GetDefault<USuperHeavyRecoveryProfile>());
    FRecoveryBodyKinematics Body;Body.OriginM=FVector(0,0,100000);
    FRecoveryDynamicsCommand C;C.Phase=ERecoveryPhase::Coast;C.bSeparated=true;C.bGroundSupplyConnected=false;
    C.EngineCount=3;C.TargetUpWorld=FVector(.2,0,1).GetSafeNormal();
    FRecoveryDynamicsModel Model;Model.Reset(Config,{},0,1000);
    for(int32 I=0;I<120;++I)Model.Step(Body,C,1./120.);
    TestTrue(TEXT("Unfulfilled ignition does not suppress available RCS torque"),Model.GetState().RcsMomentBodyNm.Size()>1000);
    C.Experiment.bReactionJetsDisabled=true;
    for(int32 I=0;I<600;++I)Model.Step(Body,C,1./120.);
    TestTrue(TEXT("Disabled valves decay to zero delivered torque"),Model.GetState().RcsMomentBodyNm.IsNearlyZero());
    TestTrue(TEXT("Controller cannot change fixture pose"),Model.GetState().Body.OriginM.Equals(Body.OriginM));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryAlternateFuelTest,"Recovery.Physics.AlternateFuelLimits",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryAlternateFuelTest::RunTest(const FString&)
{
    auto C=URecoveryPhysicsComponent::BuildGuidanceConfiguration(*GetDefault<USuperHeavyRecoveryProfile>());
    FRecoveryDynamicsState D;D.PropellantKg=0;D.RcsPropellantKg=1000;
    FRecoveryBodyKinematics Body;Body.OriginM=FVector(30000,0,20000+FlightGeometry::BoosterBaseOffsetM);Body.VelocityMps=FVector(0,0,-300);
    FRecoveryDynamicsCommand Input;Input.Phase=ERecoveryPhase::Ascent;Input.bSeparated=true;
    FRecoveryGuidanceModel Model;Model.Reset(C);Model.Step(Body,D,Input,1./120.);Model.Step(Body,D,Input,1./120.);
    const auto& S=Model.GetState();
    TestTrue(TEXT("Fuel exhaustion changes the recovery objective"),S.bAlternateRecovery);
    TestFalse(TEXT("An empty tank cannot certify an offshore diversion"),S.bSafeAlternateAvailable);
    TestEqual(TEXT("No invented divert delta-V"),S.AvailableDivertDeltaVMps,0.);
    TestEqual(TEXT("An empty bank stays off while aerodynamic control remains active"),S.Command.EngineCount,0);
    TestFalse(TEXT("Emergency attitude command remains finite"),S.Command.TargetUpWorld.ContainsNaN());
    TestTrue(TEXT("Candidates remain explicit even when none is reachable"),S.AlternateSitesM.Num()==9 && S.AlternateReachable.Num()==9);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryCorrectionDecisionTest,"Recovery.Physics.CorrectionDecision",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryCorrectionDecisionTest::RunTest(const FString&)
{
    auto C=URecoveryPhysicsComponent::BuildGuidanceConfiguration(*GetDefault<USuperHeavyRecoveryProfile>());
    C.AscentDurationS=0;C.FrontReturnOffsetM=0;C.WindVelocityMps=FVector::ZeroVector;
    C.ApogeeM=70000;C.CaptureWorldM=FVector::ZeroVector;
    FRecoveryBodyKinematics B;B.OriginM=FVector(0,0,50000+FlightGeometry::BoosterBaseOffsetM);
    FRecoveryDynamicsState D;D.PropellantKg=C.LandingReserveKg+250000;D.RcsPropellantKg=2000;
    FRecoveryDynamicsCommand Input;Input.Phase=ERecoveryPhase::Ascent;Input.bSeparated=true;
    FRecoveryGuidanceModel Model;Model.Reset(C);
    // Controller contract only: feed measured poses, without claiming that this
    // fixture integrates a physical flight. Full return replays test the solver.
    for(int I=0;I<1800 && Model.GetState().Phase!=ERecoveryPhase::Coast;++I)
    {Model.Step(B,D,Input,1./120.);B.Rotation=FQuat::FindBetweenNormals(FVector::UpVector,Model.GetState().Command.TargetUpWorld);}
    TestEqual(TEXT("Fixture reaches an accepted coast"),Model.GetState().Phase,ERecoveryPhase::Coast);
    B.OriginM.X+=20000;B.VelocityMps=FVector(0,0,-100);
    bool Fired=false;
    for(int I=0;I<180;++I)
    {Model.Step(B,D,Input,1./120.);Fired|=Model.GetState().Command.EngineCount==3;B.Rotation=FQuat::FindBetweenNormals(FVector::UpVector,Model.GetState().Command.TargetUpWorld);}
    TestTrue(TEXT("A displaced footprint requests a corrective burn after alignment"),Fired);
    TestTrue(TEXT("Correction preserves a positive landing reserve budget"),Model.GetState().CorrectionFuelBudgetKg>0);
    TestFalse(TEXT("A permitted correction is not an unpowered-flight violation"),Model.GetState().bUnpoweredViolation);
    TestEqual(TEXT("Guidance does not directly consume or manufacture propellant"),D.PropellantKg,C.LandingReserveKg+250000);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryAlternateBrakingTest,"Recovery.Physics.AlternateBraking",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryAlternateBrakingTest::RunTest(const FString&)
{
    auto C=URecoveryPhysicsComponent::BuildGuidanceConfiguration(*GetDefault<USuperHeavyRecoveryProfile>());
    FRecoveryDynamicsState D;D.PropellantKg=0;D.RcsPropellantKg=1000;
    FRecoveryBodyKinematics B;B.OriginM=FVector(30000,0,400+FlightGeometry::BoosterBaseOffsetM);B.VelocityMps=FVector(60,0,-200);
    FRecoveryDynamicsCommand Input;Input.Phase=ERecoveryPhase::Ascent;Input.bSeparated=true;
    FRecoveryGuidanceModel Model;Model.Reset(C);Model.Step(B,D,Input,1./120.);
    // An independent measured-state fixture for the emergency branch. Supplying
    // fuel here tests the command contract, not a physically flown refuelling.
    D.PropellantKg=50000;Model.Step(B,D,Input,1./120.);
    const auto& S=Model.GetState();
    TestTrue(TEXT("Emergency braking commits before impact"),S.bAlternateBraking);
    TestTrue(TEXT("Upright emergency vehicle opens a real engine bank"),S.Command.EngineCount>0);
    TestTrue(TEXT("Vertical braking takes priority over lateral miss"),S.Command.TargetUpWorld.Z>.9);
    TestTrue(TEXT("Braking request exceeds weight support"),S.Command.ThrustAccelerationMps2.Z>S.Navigation.Gravity);
    TestEqual(TEXT("Guidance does not write the measured descent velocity"),B.VelocityMps.Z,-200.);
    return true;
}
#endif
