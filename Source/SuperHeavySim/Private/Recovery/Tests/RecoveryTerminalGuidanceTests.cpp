#include "Recovery/Flight/RecoveryTerminalGuidance.h"
#include "Recovery/Flight/RecoveryPhysicsComponent.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryTerminalPlanTest,"Recovery.Physics.TerminalTransfer",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryTerminalPlanTest::RunTest(const FString&)
{
    auto Config=URecoveryPhysicsComponent::BuildConfiguration(*GetDefault<USuperHeavyRecoveryProfile>());
    FRecoveryTerminalInput Input;
    Input.PositionM=FVector(140,30,500);Input.VelocityMps=FVector(-10,-2,-65);
    Input.TargetM=FVector(24,0,49.8);Input.MassKg=330000;Input.FuelKg=90000;
    Input.CoreThrustN=3*Config.EngineThrustN;Input.LandingThrustN=13*Config.EngineThrustN;Input.IspS=327;
    // Consistent airborne state: current attitude balances the modeled side
    // load at this velocity, with zero incoming net acceleration.
    FVector InitialThrust;
    TestTrue(TEXT("Incoming aerodynamic equilibrium exists"),RecoveryTerminalGuidance::TryRequiredThrustAcceleration(
        FVector::ZeroVector,Input.VelocityMps,Input.PositionM,Input.MassKg,Input.WindMps,Config,FVector::UpVector,InitialThrust));
    Input.UpWorld=InitialThrust.GetSafeNormal();
    const auto Plan=RecoveryTerminalGuidance::Plan(Input,Config);
    AddInfo(FString::Printf(TEXT("candidates=%d thrust=%d attitude=%d clearance=%d fuel=%d horizon=%.2f"),Plan.Candidates,Plan.ThrustRejected,Plan.AttitudeRejected,Plan.ClearanceRejected,Plan.FuelRejected,Plan.HorizonS));
    TestTrue(TEXT("A bounded final transfer exists"),Plan.bFeasible);
    if(Plan.bFeasible)
    {
        TestTrue(TEXT("Transfer reaches the requested physical location"),Plan.PositionAt(Plan.HorizonS).Equals(Input.TargetM,1.e-8));
        TestTrue(TEXT("Transfer reaches the requested contact velocity"),Plan.VelocityAt(Plan.HorizonS).Equals(Input.TargetVelocityMps,1.e-8));
        TestTrue(TEXT("Contact arrival has no residual net acceleration"),Plan.AccelerationAt(Plan.HorizonS).IsNearlyZero(1.e-8));
        TestTrue(TEXT("Transfer retains incoming position and velocity"),Plan.PositionAt(0).Equals(Input.PositionM) && Plan.VelocityAt(0).Equals(Input.VelocityMps));
        TestTrue(TEXT("Planning accounts for consumption"),Plan.EstimatedFuelKg>0 && Plan.EstimatedFuelKg<Input.FuelKg);
        TestTrue(TEXT("Sampled peak is below the physical bank limit"),Plan.PeakThrustN<=Input.LandingThrustN*.95);
        TestTrue(TEXT("Sampled tilt obeys the guidance envelope"),Plan.PeakTiltDeg<=Input.MaxTiltDeg);
    }
    Input.FuelKg=100;
    TestFalse(TEXT("Insufficient fuel cannot certify a transfer"),RecoveryTerminalGuidance::Plan(Input,Config).bFeasible);
    Input.FuelKg=90000;Input.CoreThrustN=0;
    TestFalse(TEXT("No central engines cannot certify a catch transfer"),RecoveryTerminalGuidance::Plan(Input,Config).bFeasible);
    Input.CoreThrustN=3*Config.EngineThrustN;Input.PositionM=FVector(-20,0,60);Input.VelocityMps=FVector::ZeroVector;
    Input.UpWorld=FVector::UpVector;
    TestFalse(TEXT("The planner rejects a path through the tower mast"),RecoveryTerminalGuidance::Plan(Input,Config).bFeasible);
    FVector InvalidThrust(1,2,3);
    TestFalse(TEXT("Zero mass has no finite thrust solution"),RecoveryTerminalGuidance::TryRequiredThrustAcceleration(
        FVector::ZeroVector,FVector::ZeroVector,FVector::ZeroVector,0,FVector::ZeroVector,Config,FVector::UpVector,InvalidThrust));
    TestTrue(TEXT("A failed inversion cannot leak a numeric sentinel into commands"),InvalidThrust.IsZero());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryMeasuredTerminalTest,"Recovery.Physics.MeasuredTerminalApproach",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryMeasuredTerminalTest::RunTest(const FString&)
{
    const auto Config=URecoveryPhysicsComponent::BuildConfiguration(*GetDefault<USuperHeavyRecoveryProfile>());
    // Recorded solver input from TerminalReachable_Nominal_60 at T+346.3 s.
    // Unlike the idealized fixture, it includes wind, current lean and net load.
    FRecoveryTerminalInput I;
    I.PositionM=FVector(-337.661063734,73.374590168,705.540256480);
    I.VelocityMps=FVector(13.332060547,-.078386297,-41.396123047);
    I.AccelerationMps2=FVector(2.024562485,-.492193600,3.600150589);
    I.UpWorld=FVector(.250182110,-.066271122,.965928078);
    I.TargetM=FVector(24,.083866399,46.977923781);I.WindMps=FVector(0,4,0);
    I.MassKg=331916.072352932;I.FuelKg=118834.720924147;I.IspS=328.792782692;
    I.CoreThrustN=7395311.335754103;I.LandingThrustN=32046349.121601105;I.CentreFromBaseM=27.177825050;
    const auto Plan=RecoveryTerminalGuidance::Plan(I,Config);
    TestTrue(TEXT("A transfer exists for the measured incoming flight state"),Plan.bFeasible);
    if(Plan.bFeasible)
    {
        TestTrue(TEXT("Recorded velocity is retained without rewriting the state"),Plan.VelocityAt(0).Equals(I.VelocityMps,1.e-9));
        TestTrue(TEXT("Recorded acceleration remains continuous"),Plan.AccelerationAt(0).Equals(I.AccelerationMps2,1.e-9));
        TestTrue(TEXT("The predicted arrival respects the contact velocity"),Plan.VelocityAt(Plan.HorizonS).Equals(I.TargetVelocityMps,1.e-8));
        TestTrue(TEXT("Rejected candidates remain visible after a successful search"),Plan.Candidates==1+Plan.ThrustRejected+Plan.AttitudeRejected+Plan.ClearanceRejected+Plan.FuelRejected);
    }
    I.FuelKg=1000;
    TestFalse(TEXT("The measured approach cannot ignore a depleted reserve"),RecoveryTerminalGuidance::Plan(I,Config).bFeasible);
    return true;
}
#endif
