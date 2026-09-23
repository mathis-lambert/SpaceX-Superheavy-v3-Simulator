#include "Recovery/Flight/RecoveryPropulsionModel.h"
#include "Recovery/Flight/RecoveryAtmosphere.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryValveIntegralTest,"Recovery.Physics.ValveImpulseConvergence",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryValveIntegralTest::RunTest(const FString&)
{
    constexpr double Rated=2.4e6,Tau=.25,Closing=.35,Duration=2.;
    // Independent analytic response of dF/dt=(Rated-F)/Tau with F(0)=0.
    const double ExpectedEnd=Rated*(1-FMath::Exp(-Duration/Tau));
    const double ExpectedImpulse=Rated*(Duration-Tau*(1-FMath::Exp(-Duration/Tau)));
    const double ExpectedClosingImpulse=.5*ExpectedEnd*ExpectedEnd/(Rated/Closing);
    for(const double Cadence:{15.,30.,60.,120.,240.})
    {
        double Thrust=0,Impulse=0,Time=0;
        int32 Index=0;
        while(Time<Duration-1.e-12)
        {
            // Unequal steps deliberately exercise the partition invariance.
            const double Dt=FMath::Min(Duration-Time,(++Index%3==0?.5:1.)/Cadence);
            const auto Step=RecoveryPropulsion::AdvanceValve(Thrust,Rated,Rated,Tau,Closing,Dt);
            Thrust=Step.EndThrustN;Impulse+=Step.ImpulseNs;Time+=Dt;
        }
        TestTrue(TEXT("Opening endpoint is independent of frame partition"),FMath::Abs(Thrust-ExpectedEnd)<1.e-6);
        TestTrue(TEXT("Opening impulse matches the analytic response"),FMath::Abs(Impulse-ExpectedImpulse)<1.e-6);
        double ClosingImpulse=0;
        for(int32 I=0;I<int32(Cadence);++I)
        {
            const auto Step=RecoveryPropulsion::AdvanceValve(Thrust,0,Rated,Tau,Closing,1./Cadence);
            Thrust=Step.EndThrustN;ClosingImpulse+=Step.ImpulseNs;
        }
        TestEqual(TEXT("Closing valve reaches zero in finite time"),Thrust,0.);
        TestTrue(TEXT("Shutdown includes the partial final burning step"),FMath::Abs(ClosingImpulse-ExpectedClosingImpulse)<1.e-6);
    }
    const auto Invalid=RecoveryPropulsion::AdvanceValve(123,Rated,Rated,Tau,Closing,0);
    TestEqual(TEXT("A paused valve retains its endpoint"),Invalid.EndThrustN,123.);
    TestEqual(TEXT("A paused valve emits no impulse"),Invalid.ImpulseNs,0.);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryEngineBankBudgetTest,"Recovery.Physics.EngineBankFuelBudget",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryEngineBankBudgetTest::RunTest(const FString&)
{
    FRecoveryEngineParameters Parameters;
    FRecoveryEngineCommand Command;
    Command.RequestedCount=33;Command.RatedThrustN=2.4e6;Command.RequestedThrustN=33*Command.RatedThrustN;
    Command.SpecificImpulseS=327;Command.FailedEngine=7;
    for(const double Dt:{1./15.,1./120.})
    {
        TArray<FRecoveryEngineState> Engines;Engines.SetNum(33);
        constexpr double Fuel=.01;
        const auto Step=RecoveryPropulsion::AdvanceEngines(Engines,Parameters,Command,Fuel,Dt);
        TestEqual(TEXT("A failed engine supplies no opening impulse"),Engines[7].StepImpulseNs,0.);
        TestTrue(TEXT("Exhaust momentum limits the entire engine bank"),FMath::IsNearlyEqual(Step.DeliveredImpulseNs,Fuel*327*RecoveryAtmosphere::G0,1.e-8));
        TestTrue(TEXT("Fuel consumption is the delivered impulse divided by exhaust speed"),FMath::IsNearlyEqual(Step.FuelUsedKg,Fuel,1.e-12));
        double Sum=0;
        for(const auto& Engine:Engines)
        {
            Sum+=Engine.StepImpulseNs;
            TestEqual(TEXT("Depleted engines have no endpoint thrust"),Engine.ThrustN,0.);
        }
        TestTrue(TEXT("Per-engine impulses sum to the fuel-limited bank"),FMath::IsNearlyEqual(Sum,Step.DeliveredImpulseNs,1.e-8));
        TestEqual(TEXT("No residual impulse appears on the next empty step"),RecoveryPropulsion::AdvanceEngines(Engines,Parameters,Command,0,Dt).DeliveredImpulseNs,0.);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryEngineBankForceTest,"Recovery.Physics.EngineBankRealizableForces",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryEngineBankForceTest::RunTest(const FString&)
{
    FRecoveryEngineParameters Parameters;
    FRecoveryEngineCommand Command;
    Command.RequestedCount=3;Command.RatedThrustN=2.4e6;Command.RequestedThrustN=7.2e6;Command.SpecificImpulseS=327;
    TArray<FRecoveryEngineState> Engines;
    for(int32 I=0;I<3;++I)
    {
        FRecoveryEngineState Engine;Engine.bCentral=Engine.bGimballed=true;Engine.ThrustN=Command.RatedThrustN;
        Engine.PositionFromBaseM=FVector(2*FMath::Cos(I*2*PI/3.),2*FMath::Sin(I*2*PI/3.),0);
        Engines.Add(Engine);
    }
    const double Dt=1./120.;
    auto Step=RecoveryPropulsion::AdvanceEngines(Engines,Parameters,Command,1.e6,Dt);
    const FVector COM(0,0,31),Demand(1.e9,-1.e9,1.e9);
    RecoveryPropulsion::AllocateGimbals(Engines,Parameters,COM,Demand,Dt,Step);
    FVector Force=FVector::ZeroVector,Moment=FVector::ZeroVector;
    for(const auto& Engine:Engines)
    {
        TestTrue(TEXT("Vectoring does not multiply delivered force"),FMath::IsNearlyEqual(Engine.StepForceBodyN.Size()*Dt,Engine.StepImpulseNs,1.e-6));
        const double Angle=FMath::Acos(FMath::Clamp(Engine.DirectionBody.Z,-1.,1.));
        TestTrue(TEXT("Gimbal angular rate remains limited"),Angle<=FMath::DegreesToRadians(Parameters.GimbalRateDegS)*Dt+1.e-6);
        TestTrue(TEXT("Gimbal cone remains limited"),Angle<=FMath::DegreesToRadians(Parameters.MaximumGimbalDeg)+1.e-6);
        Force+=Engine.StepForceBodyN;Moment+=FVector::CrossProduct(Engine.PositionFromBaseM-COM,Engine.StepForceBodyN);
    }
    TestTrue(TEXT("Reported force comes from actual per-engine forces"),(Force-Step.ForceBodyN).Size()<1.e-6);
    TestTrue(TEXT("Reported moment comes from nozzle lever arms"),(Moment-Step.MomentBodyNm).Size()<1.e-6);
    TestTrue(TEXT("Impossible requested moments are saturated physically"),Step.MomentBodyNm.Size()<Demand.Size());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryTranslationAllocationTest,"Recovery.Physics.TerminalTranslationAllocation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryTranslationAllocationTest::RunTest(const FString&)
{
    FRecoveryEngineParameters P;TArray<FRecoveryEngineState> Engines;
    for(int32 I=0;I<3;++I)
    {
        FRecoveryEngineState E;E.bCentral=E.bGimballed=true;E.StepImpulseNs=1000000./120.;
        E.PositionFromBaseM=FVector(FMath::Cos(I*2*UE_DOUBLE_PI/3),FMath::Sin(I*2*UE_DOUBLE_PI/3),0);Engines.Add(E);
    }
    FRecoveryPropulsionStep Result;const FVector COM(0,0,30),Force(60000,-30000,3000000);
    for(int32 I=0;I<240;++I)RecoveryPropulsion::AllocateGimbals(Engines,P,COM,FVector::ZeroVector,1./120.,Result,Force,1);
    TestTrue(TEXT("Bounded gimbals deliver the requested lateral translation"),FVector2D(Result.ForceBodyN-Force).Size()<100);
    TestTrue(TEXT("Translation retains its physical counter-torque for residual allocation"),FMath::Abs(Result.MomentBodyNm.Y+30*Result.ForceBodyN.X)<1.e-5);
    double Impulse=0;for(const auto& E:Engines)Impulse+=E.StepForceBodyN.Size()/120.;
    TestTrue(TEXT("Vectoring preserves delivered impulse rather than creating force"),FMath::Abs(Impulse-25000)<1.e-5);
    return true;
}
#endif
