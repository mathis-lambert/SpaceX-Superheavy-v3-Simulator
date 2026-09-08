#include "Recovery/Flight/RecoveryDynamicsModel.h"
#include "Recovery/Flight/RecoveryPhysicsComponent.h"
#include "Recovery/Flight/RecoveryAtmosphere.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryDynamicsImpulseTest,"Recovery.Physics.DynamicsImpulseLedger",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryDynamicsImpulseTest::RunTest(const FString&)
{
    const auto* Profile=GetDefault<USuperHeavyRecoveryProfile>();
    const auto Config=URecoveryPhysicsComponent::BuildConfiguration(*Profile);
    TArray<FRecoveryEngineState> Geometry;Geometry.SetNum(33);
    FRecoveryBodyKinematics Body;
    Body.OriginM=FVector(0,0,200000+FlightGeometry::BoosterBaseOffsetM);
    FRecoveryDynamicsCommand Command;
    Command.Phase=ERecoveryPhase::Ascent;Command.EngineCount=33;Command.bSeparated=true;
    Command.bGroundSupplyConnected=false;Command.ThrustAccelerationMps2=FVector(0,0,1000);
    // Held, motionless test fixture in vacuum: all engines command rated thrust.
    // The oracle integrates the valve ODE independently of the production bank.
    constexpr double Duration=2.;
    const double Rated=Config.EngineThrustN*Config.SpecificImpulseVacuumS/Config.SpecificImpulseSeaLevelS;
    const double Tau=Config.Engines.OpeningTimeConstantS;
    const double ExpectedImpulse=33*Rated*(Duration-Tau*(1-FMath::Exp(-Duration/Tau)));
    for(const int32 Hz:{30,60,120,240})
    {
        FRecoveryDynamicsModel Model;Model.Reset(Config,Geometry,Profile->PropellantMassKg,Profile->ReactionControlPropellantKg);
        double MeasuredImpulse=0;
        for(int32 I=0;I<Hz*int32(Duration);++I)
        {
            Model.Step(Body,Command,1./Hz);
            for(const auto& Force:Model.GetState().Forces)
                if(Force.Kind==ERecoveryForceKind::Engine)MeasuredImpulse+=Force.ForceN.Z/Hz;
        }
        const auto& State=Model.GetState();
        TestTrue(TEXT("Applied engine forces deliver analytic impulse"),FMath::Abs(MeasuredImpulse-ExpectedImpulse)<.01);
        TestTrue(TEXT("Fuel loss equals delivered exhaust momentum"),FMath::Abs(State.MainFuelConsumedKg-ExpectedImpulse/(Config.SpecificImpulseVacuumS*RecoveryAtmosphere::G0))<1.e-6);
        TestTrue(TEXT("Main tank ledger balances"),FMath::Abs(Profile->PropellantMassKg-State.PropellantKg-State.MainFuelConsumedKg)<1.e-6);
        TestTrue(TEXT("Force model never changes supplied position"),State.Body.OriginM.Equals(Body.OriginM,0.));
        TestTrue(TEXT("Force model never changes supplied velocity"),State.Body.VelocityMps.IsZero());
        TestEqual(TEXT("One model update per requested physical step"),State.Steps,uint64(Hz*2));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryDynamicsLoadsTest,"Recovery.Physics.DynamicsCoastLoads",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryDynamicsLoadsTest::RunTest(const FString&)
{
    const auto* Profile=GetDefault<USuperHeavyRecoveryProfile>();
    auto Config=URecoveryPhysicsComponent::BuildConfiguration(*Profile);
    Config.WindVelocityMps=FVector::ZeroVector;
    FRecoveryDynamicsModel Model;Model.Reset(Config,{},75000,0);
    FRecoveryBodyKinematics Body;
    Body.OriginM=FVector(30000,0,15000+FlightGeometry::BoosterBaseOffsetM);
    Body.VelocityMps=FVector(80,25,-100);
    FRecoveryDynamicsCommand Command;Command.Phase=ERecoveryPhase::Captured;
    Command.bSeparated=true;Command.bContactShutdown=true;Command.bGroundSupplyConnected=false;
    Model.Step(Body,Command,1./120.);
    const auto& State=Model.GetState();
    const FVector COM=Body.OriginM+FVector(0,0,State.Mass.CentreFromBaseM-FlightGeometry::BoosterBaseOffsetM);
    for(const auto& Force:State.Forces)
    {
        if(Force.Kind==ERecoveryForceKind::Gravity)
        {
            TestTrue(TEXT("Gravity points at Earth's centre"),Force.ForceN.GetSafeNormal().Equals((FlightGeometry::EarthCenterCm()/100.-COM).GetSafeNormal(),1.e-10));
            TestTrue(TEXT("Gravity acts through the mass centre"),Force.PointCm.Equals(COM*100.,1.e-6));
        }
        else if(Force.Kind==ERecoveryForceKind::Aerodynamic)
            TestTrue(TEXT("Passive air loads remove relative kinetic energy"),FVector::DotProduct(Force.ForceN,Body.VelocityMps)<0);
        else TestTrue(TEXT("Shutdown cannot invent actuator forces"),Force.ForceN.IsNearlyZero());
    }
    const auto Before=State;
    Model.Step(Body,Command,0);
    TestEqual(TEXT("Pause does not advance physical time"),Model.GetState().ElapsedS,Before.ElapsedS);
    TestEqual(TEXT("Pause does not consume fuel"),Model.GetState().PropellantKg,Before.PropellantKg);
    return true;
}
#endif
