#include "Recovery/Flight/RecoveryLandingPrediction.h"
#include "Recovery/Flight/RecoveryPhysicsComponent.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryLandingPredictionTest,"Recovery.Physics.LandingReachability",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryLandingPredictionTest::RunTest(const FString&)
{
    auto C=URecoveryPhysicsComponent::BuildConfiguration(*GetDefault<USuperHeavyRecoveryProfile>());
    FRecoveryDynamicsState D;D.Mass.MassKg=400000;D.PropellantKg=180000;
    for(int32 I=0;I<33;++I){FRecoveryEngineState E;E.bCentral=I<3;E.bGimballed=I<13;D.Engines.Add(E);}
    const auto Estimate=[&](double Speed=400,int32 Failed=INDEX_NONE)
    {return RecoveryLanding::Predict(C,D,3000,-Speed,.98,35,Failed);};
    const auto Base=Estimate();
    TestTrue(TEXT("Rest-to-rest transfer retains the analytic bang-bang time"),FMath::IsNearlyEqual(RecoveryLanding::MinimumTransferTime(100,0,4),10.,1.e-10));
    TestTrue(TEXT("An already braking arrival needs only its stopping time"),FMath::IsNearlyEqual(RecoveryLanding::MinimumTransferTime(50,20,4),5.,1.e-10));
    TestTrue(TEXT("Travel in the wrong direction takes longer"),RecoveryLanding::MinimumTransferTime(100,-10,4)>RecoveryLanding::MinimumTransferTime(100,10,4));
    TestTrue(TEXT("An airborne restart can stop inside the remaining fuel budget"),Base.bFeasible && Base.FuelKg>0 && Base.FuelKg<D.PropellantKg);
    TestTrue(TEXT("The projection contains a real positive braking distance and time"),Base.DistanceM>500 && Base.DurationS>5);
    TestTrue(TEXT("More descent energy requires earlier ignition"),Estimate(450).DistanceM>Base.DistanceM);
    D.Mass.MassKg*=1.10;
    TestTrue(TEXT("More mass increases required braking distance"),Estimate().DistanceM>Base.DistanceM);
    D.Mass.MassKg=400000;
    C.Engines.OpeningTimeConstantS=1.;
    TestTrue(TEXT("Slower physical valves need more altitude"),Estimate().DistanceM>Base.DistanceM);
    C.Engines.OpeningTimeConstantS=.25;
    const auto FailedCore=Estimate(400,0);
    TestTrue(TEXT("A failed core reduces available final-stage authority"),FailedCore.CoreThrustN<Base.CoreThrustN);
    TestTrue(TEXT("A failed core cannot improve reachability"),!FailedCore.bFeasible || FailedCore.DistanceM>Base.DistanceM);
    D.PropellantKg=100;
    TestFalse(TEXT("The predictor cannot borrow fuel"),Estimate().bFeasible);
    D.PropellantKg=180000;
    for(auto& E:D.Engines)E.bCentral=false;
    TestFalse(TEXT("No final engine bank cannot certify a capture"),Estimate().bFeasible);
    TestFalse(TEXT("An inverted vehicle cannot be assumed upright instantly"),RecoveryLanding::Predict(C,D,3000,-400,-1,35,INDEX_NONE).bFeasible);
    return true;
}
#endif
