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
    Input.UpWorld=RecoveryTerminalGuidance::RequiredThrustAcceleration(FVector::ZeroVector,Input.VelocityMps,
        Input.PositionM,Input.MassKg,Input.WindMps,Config).GetSafeNormal();
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
    TestFalse(TEXT("The planner rejects a path through the tower mast"),RecoveryTerminalGuidance::Plan(Input,Config).bFeasible);
    return true;
}
#endif
