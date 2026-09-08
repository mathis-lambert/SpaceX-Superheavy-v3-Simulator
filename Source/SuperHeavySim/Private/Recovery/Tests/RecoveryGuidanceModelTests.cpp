#include "Recovery/Flight/RecoveryGuidanceModel.h"
#include "Recovery/Flight/RecoveryPhysicsComponent.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryGuidanceBatchTest,"Recovery.Physics.GuidancePhysicalClock",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryGuidanceBatchTest::RunTest(const FString&)
{
    const auto* Profile=GetDefault<USuperHeavyRecoveryProfile>();
    auto Config=URecoveryPhysicsComponent::BuildGuidanceConfiguration(*Profile);
    Config.WindVelocityMps=FVector::ZeroVector;
    FRecoveryDynamicsState Dynamics;Dynamics.PropellantKg=Profile->PropellantMassKg;Dynamics.RcsPropellantKg=Profile->ReactionControlPropellantKg;
    FRecoveryDynamicsCommand External;External.Phase=ERecoveryPhase::Ascent;
    constexpr int32 PhysicsHz=120,Duration=15;
    FVector Reference=FVector::ZeroVector;
    for(const int32 GameHz:{15,30,60})
    {
        FRecoveryGuidanceModel Model;Model.Reset(Config);
        int32 Step=0;
        for(int32 Frame=0;Frame<GameHz*Duration;++Frame)
            for(int32 Substep=0;Substep<PhysicsHz/GameHz;++Substep)
            {
                // Independently prescribed test kinematics arrive at each
                // physical step, grouped into different render-frame batches.
                const double Time=double(++Step)/PhysicsHz;
                FRecoveryBodyKinematics Body;Body.OriginM=FVector(0,0,1000+200*Time+FlightGeometry::BoosterBaseOffsetM);
                Body.VelocityMps=FVector(0,0,200);
                Model.Step(Body,Dynamics,External,1./PhysicsHz);
            }
        const auto& State=Model.GetState();
        const double U=(Duration-10.)/(Config.AscentDurationS-10.);
        const double Pitch=Config.AscentPitchDeg*UE_DOUBLE_PI/180.*U*U*(3-2*U);
        TestTrue(TEXT("Pitch program uses physical time"),State.Command.TargetUpWorld.Equals(FVector(FMath::Sin(Pitch),0,FMath::Cos(Pitch)),1.e-10));
        TestTrue(TEXT("Navigation reads the last physical sample"),FMath::Abs(State.Navigation.AltitudeM-4000.)<1.e-6);
        TestTrue(TEXT("Guidance clock integrates actual physics steps"),FMath::Abs(State.MissionTimeS-Duration)<1.e-10);
        TestEqual(TEXT("No substep is skipped inside a slow frame"),State.Steps,uint64(PhysicsHz*Duration));
        if(GameHz==15)Reference=State.Command.ThrustAccelerationMps2;
        else TestTrue(TEXT("Frame batching cannot change the command"),State.Command.ThrustAccelerationMps2.Equals(Reference,1.e-12));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryGuidanceEventsTest,"Recovery.Physics.GuidanceMechanicalAcknowledgement",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryGuidanceEventsTest::RunTest(const FString&)
{
    const auto* Profile=GetDefault<USuperHeavyRecoveryProfile>();
    auto Config=URecoveryPhysicsComponent::BuildGuidanceConfiguration(*Profile);Config.WindVelocityMps=FVector::ZeroVector;
    FRecoveryDynamicsState Dynamics;Dynamics.PropellantKg=Config.LandingReserveKg+Config.BoostbackReserveKg-1;Dynamics.RcsPropellantKg=7000;
    FRecoveryBodyKinematics Body;Body.OriginM=FVector(0,0,25000+FlightGeometry::BoosterBaseOffsetM);Body.VelocityMps=FVector(0,0,200);
    FRecoveryDynamicsCommand External;External.Phase=ERecoveryPhase::Ascent;
    FRecoveryGuidanceModel Model;Model.Reset(Config);
    for(int32 I=0;I<480;++I)Model.Step(Body,Dynamics,External,1./120.);
    TestTrue(TEXT("Fuel threshold requests mechanical stage separation"),Model.GetState().bSeparationRequested);
    TestEqual(TEXT("Boostback waits for the independent body to exist"),Model.GetState().Phase,ERecoveryPhase::Separation);
    TestEqual(TEXT("Waiting for separation does not ignite engines"),Model.GetState().Command.EngineCount,0);
    const auto Separation=Model.GetState().Events[0];
    TestEqual(TEXT("Event identifies the separation decision"),Separation.Reason,ERecoveryGuidanceReason::Separation);
    TestTrue(TEXT("First decision uses the initial physical sample"),Separation.SampleTimeS==0 && FMath::Abs(Separation.TimeS-1./120.)<1.e-12);
    TestTrue(TEXT("Event preserves altitude before asynchronous delivery"),FMath::Abs(Separation.AltitudeM-25000.)<1.e-6);
    const double StackMass=Config.DryMassKg+Config.UpperStageMassKg+Dynamics.PropellantKg+Dynamics.RcsPropellantKg;
    TestTrue(TEXT("Separation request still measures the attached stack"),FMath::Abs(Separation.MassKg-StackMass)<1.e-6);
    External.bSeparated=true;Model.Step(Body,Dynamics,External,1./120.);
    TestEqual(TEXT("Acknowledged separation permits aligned boostback"),Model.GetState().Phase,ERecoveryPhase::Boostback);
    TestEqual(TEXT("Boostback requests thirteen bounded actuators"),Model.GetState().Command.EngineCount,13);
    TestTrue(TEXT("Mechanical acknowledgement removes upper-stage mass from navigation"),FMath::Abs(Model.GetState().Navigation.MassKg-(StackMass-Config.UpperStageMassKg))<1.e-6);
    TestEqual(TEXT("Later samples cannot rewrite earlier event evidence"),Model.GetState().Events[0].MassKg,Separation.MassKg);
    const double Time=Model.GetState().MissionTimeS;
    Model.Step(Body,Dynamics,External,0);
    TestEqual(TEXT("Pause cannot advance the guidance clock"),Model.GetState().MissionTimeS,Time);
    External.Phase=ERecoveryPhase::Aborted;Model.Step(Body,Dynamics,External,1./120.);
    TestEqual(TEXT("Operator abort overrides the autonomous phase"),Model.GetState().Phase,ERecoveryPhase::Aborted);
    TestEqual(TEXT("Abort requests valve closure"),Model.GetState().Command.EngineCount,0);
    TestTrue(TEXT("Abort carries no thrust demand"),Model.GetState().Command.ThrustAccelerationMps2.IsZero());
    Model.Reset(Config);
    TestFalse(TEXT("Reset removes previous flight ownership"),Model.GetState().bFlightStarted);
    TestEqual(TEXT("Reset clears old events"),Model.GetState().Events.Num(),0);
    Model.Step(Body,Dynamics,External,1./120.);
    TestFalse(TEXT("Ground abort cannot start a flight"),Model.GetState().bFlightStarted);
    TestEqual(TEXT("Ground abort leaves the flight clock stopped"),Model.GetState().Steps,uint64(0));
    return true;
}
#endif
