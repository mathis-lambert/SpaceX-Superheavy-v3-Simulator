#include "Recovery/Flight/RecoveryUpperStageModel.h"
#include "Recovery/Flight/RecoveryAtmosphere.h"
#include "Recovery/Flight/RecoverySeparationModel.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryUpperStageImpulseTest,"Recovery.Physics.UpperStageImpulseLedger",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryUpperStageImpulseTest::RunTest(const FString&)
{
    FRecoveryUpperStageConfiguration C;C.DryMassKg=100000;C.InitialFuelKg=20000;C.EngineThrustN=2.e6;C.SpecificImpulseS=350;
    FRecoveryBodyKinematics Body;Body.OriginM=FVector(0,0,200000);
    const double ExpectedImpulse=6*C.EngineThrustN*(2.-.4*(1-FMath::Exp(-2./.4)));
    for(int32 Hz:{30,60,120,240})
    {
        FRecoveryUpperStageModel Model;Model.Reset(C);
        double AppliedImpulse=0;
        for(int32 I=0;I<2*Hz;++I)
        {
            Model.Step(Body,1.,1./Hz);
            for(const auto& F:Model.GetState().Forces)
                if(F.Kind==ERecoveryForceKind::Engine)AppliedImpulse+=F.ForceN.Z/Hz;
        }
        const auto& S=Model.GetState();
        TestTrue(TEXT("Actual nozzle forces deliver analytic valve impulse"),FMath::Abs(AppliedImpulse-ExpectedImpulse)<.001);
        TestTrue(TEXT("Fuel loss equals exhaust momentum"),FMath::Abs(S.FuelConsumedKg-ExpectedImpulse/(C.SpecificImpulseS*RecoveryAtmosphere::G0))<1.e-7);
        TestTrue(TEXT("Fuel ledger closes"),FMath::Abs(C.InitialFuelKg-S.PropellantKg-S.FuelConsumedKg)<1.e-7);
        TestEqual(TEXT("Every physical interval is integrated"),S.Steps,uint64(2*Hz));
        TestTrue(TEXT("The force model cannot reposition the stage"),S.Body.OriginM.Equals(Body.OriginM,0.) && S.Body.VelocityMps.IsZero());
    }
    C.InitialFuelKg=.01;
    FRecoveryUpperStageModel Empty;Empty.Reset(C);
    for(int32 I=0;I<120;++I)Empty.Step(Body,1.,1./120.);
    TestTrue(TEXT("Final partial step obeys the finite fuel impulse budget"),
        FMath::Abs(Empty.GetState().DeliveredImpulseNs-C.InitialFuelKg*C.SpecificImpulseS*RecoveryAtmosphere::G0)<1.e-9);
    TestEqual(TEXT("No residual displayed thrust after exhaustion"),Empty.GetState().ThrustN,0.);
    for(const auto& F:Empty.GetState().Forces)
        if(F.Kind==ERecoveryForceKind::Engine)TestTrue(TEXT("No physical thrust after exhaustion"),F.ForceN.IsZero());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryUpperStageCoastTest,"Recovery.Physics.UpperStageCoastLoads",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryUpperStageCoastTest::RunTest(const FString&)
{
    FRecoveryUpperStageConfiguration C;C.DryMassKg=100000;C.EngineThrustN=2.e6;C.SpecificImpulseS=350;
    FRecoveryUpperStageModel Model;Model.Reset(C);
    FRecoveryBodyKinematics Body;Body.OriginM=FVector(30000,0,15000);Body.VelocityMps=FVector(80,25,-100);
    Body.Rotation=FQuat(FVector::RightVector,.4);
    Model.Step(Body,1.,1./120.);
    for(const auto& F:Model.GetState().Forces)
    {
        if(F.Kind==ERecoveryForceKind::Aerodynamic)
            TestTrue(TEXT("Passive air loads remove kinetic energy"),FVector::DotProduct(F.ForceN,Body.VelocityMps)<0.);
        if(F.Kind==ERecoveryForceKind::Gravity)
        {
            TestTrue(TEXT("Gravity points to Earth's centre"),F.ForceN.GetSafeNormal().Equals((FlightGeometry::EarthCenterCm()/100.-Body.OriginM).GetSafeNormal(),1.e-10));
            TestTrue(TEXT("Gravity creates no moment about the centre"),F.PointCm.Equals(Body.OriginM*100.,1.e-8));
        }
    }
    const double Time=Model.GetState().ElapsedS;
    Model.Step(Body,1.,0.);
    TestEqual(TEXT("Pause does not advance upper-stage physics"),Model.GetState().ElapsedS,Time);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoverySeparationTransferTest,"Recovery.Physics.SeparationVelocityField",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoverySeparationTransferTest::RunTest(const FString&)
{
    const auto& C=*GetDefault<USuperHeavyRecoveryProfile>();
    const auto Before=RecoveryMass::Booster(C,480000,5000,true);
    const auto Booster=RecoveryMass::Booster(C,480000,5000,false);
    const auto Stage=RecoveryMass::UpperStage(C.UpperStageMassKg);
    for(double Angle:{0.,.4,2.})
    {
        FRecoveryBodyKinematics Body;Body.OriginM=FVector(45000,6000,75000);
        Body.Rotation=FQuat(FVector(1,2,3).GetSafeNormal(),Angle);
        Body.VelocityMps=FVector(1400,30,950);Body.AngularVelocityWorldRadS=FVector(.04,-.08,.12);
        const auto Split=RecoverySeparation::Split(Body,Before,Booster,Stage);
        TestTrue(TEXT("Split conserves linear momentum"),Split.LinearMomentumRelativeError<1.e-12);
        TestTrue(TEXT("Split conserves spin and orbital angular momentum"),Split.AngularMomentumRelativeError<1.e-9);
        TestTrue(TEXT("Booster pose and angular velocity are inherited exactly"),
            Split.Booster.OriginM.Equals(Body.OriginM,0.) && Split.Booster.Rotation.Equals(Body.Rotation,0.) &&
            Split.Booster.AngularVelocityWorldRadS.Equals(Body.AngularVelocityWorldRadS,0.));
        const FVector CentreDelta=Body.Rotation.GetUpVector()*(96.02-Booster.CentreFromBaseM);
        TestTrue(TEXT("Relative COM velocity is the original rigid-stack rotational field"),
            (Split.UpperStage.VelocityMps-Split.Booster.VelocityMps).Equals(FVector::CrossProduct(Body.AngularVelocityWorldRadS,CentreDelta),1.e-9));
    }
    return true;
}
#endif
