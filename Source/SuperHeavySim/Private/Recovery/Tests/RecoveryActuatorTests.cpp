#include "Recovery/Flight/RecoveryActuators.h"
#include "Recovery/Flight/RecoveryMassProperties.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryFuelImpulseTest,"Recovery.Physics.FuelImpulseBudget",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryFuelImpulseTest::RunTest(const FString&)
{
    const double Isp=327.,Fuel=.001;
    const double MaximumImpulse=Fuel*Isp*9.80665;
    for(const double Dt:{1./120.,1./30.,.2})
    {
        const double Thrust=RecoveryActuators::FuelLimitedThrust(1.e6,Fuel,Isp,Dt);
        TestTrue(TEXT("Delivered impulse cannot exceed exhaust momentum"),FMath::IsNearlyEqual(Thrust*Dt,MaximumImpulse,1.e-10));
        TestEqual(TEXT("Empty tanks cannot deliver impulse"),RecoveryActuators::FuelLimitedThrust(1.e6,0,Isp,Dt),0.);
    }
    TestEqual(TEXT("Invalid elapsed time produces no thrust"),RecoveryActuators::FuelLimitedThrust(100,1,Isp,0),0.);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryEngineMomentTest,"Recovery.Physics.EngineMomentAllocation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryEngineMomentTest::RunTest(const FString&)
{
    for(const int Count:{3,13})
    {
        TArray<FVector> Positions;
        FVector C0=FVector::ZeroVector,C1=FVector::ZeroVector,C2=FVector::ZeroVector;
        for(int I=0;I<Count;++I)
        {
            const double Angle=I*2*PI/Count;
            const FVector R(2*FMath::Cos(Angle),2*FMath::Sin(Angle),-31);
            Positions.Add(R);
            for(const FVector A:{FVector(0,R.Z,-R.Y),FVector(-R.Z,0,R.X)})
            {C0+=A*A.X;C1+=A*A.Y;C2+=A*A.Z;}
        }
        for(const FVector Request:{FVector(2.e6,0,0),FVector(0,-3.e6,0),FVector(0,0,2.e5),FVector(1.e6,-2.e6,1.e5)})
        {
            const FVector L=RecoveryActuators::SolveSymmetric(C0,C1,C2,Request);
            FVector Moment=FVector::ZeroVector,NetForce=FVector::ZeroVector;
            for(const FVector R:Positions)
            {
                const FVector F(FVector::DotProduct(FVector(0,R.Z,-R.Y),L),FVector::DotProduct(FVector(-R.Z,0,R.X),L),0);
                NetForce+=F;Moment+=FVector::CrossProduct(R,F);
            }
            TestTrue(TEXT("Engine forces generate the requested moment before actuator saturation"),(Moment-Request).Size()<1.e-6);
            if(Request.X==0 && Request.Y==0)TestTrue(TEXT("Symmetric roll allocation has no spurious translation"),NetForce.Size()<1.e-6);
        }
    }
    TestTrue(TEXT("Singular geometry cannot invent authority"),RecoveryActuators::SolveSymmetric(FVector::ZeroVector,FVector::ZeroVector,FVector::ZeroVector,FVector(1)).IsZero());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryStageMassTest,"Recovery.Physics.StageMassConservation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryStageMassTest::RunTest(const FString&)
{
    const auto* P=GetDefault<USuperHeavyRecoveryProfile>();
    for(const double Fuel:{3650000.,480000.,75000.,0.})
    {
        const auto Stack=RecoveryMass::Booster(*P,Fuel,7000,true);
        const auto Booster=RecoveryMass::Booster(*P,Fuel,7000,false);
        const auto Ship=RecoveryMass::UpperStage(P->UpperStageMassKg);
        TestTrue(TEXT("Separation preserves total vehicle mass"),FMath::IsNearlyEqual(Stack.MassKg,Booster.MassKg+Ship.MassKg,1.e-6));
        const double Moment=Booster.MassKg*Booster.CentreFromBaseM+Ship.MassKg*(Ship.CentreFromBaseM+71.02);
        TestTrue(TEXT("The first moment of mass is conserved"),FMath::IsNearlyEqual(Moment,Stack.MassKg*Stack.CentreFromBaseM,.001));
        const double OffsetInertia=Booster.MassKg*FMath::Square(Booster.CentreFromBaseM-Stack.CentreFromBaseM)+Ship.MassKg*FMath::Square(96.02-Stack.CentreFromBaseM);
        TestTrue(TEXT("Stage inertia obeys the parallel-axis theorem"),FMath::Abs(Booster.InertiaKgM2.X+Ship.InertiaKgM2.X+OffsetInertia-Stack.InertiaKgM2.X)<.001);
    }
    return true;
}
#include "Recovery/Presentation/RecoveryCameraTracking.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryChaseContactTest,"Recovery.Presentation.ChaseContactStability",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryChaseContactTest::RunTest(const FString&)
{
    for(const double Dt:{1./60,1./30,1./15})
    {
        FRecoveryChaseTracking Track;
        for(int I=0;I<600;++I)Track.Update(FVector(0,0,-20),Dt);
        const FVector Settled=Track.Direction;
        for(int I=0;I<600;++I)
        {
            const FVector Noise=FVector(FMath::Sin(I*2.3),FMath::Cos(I*1.7),I%2?1.:-1.)*.03;
            TestTrue(TEXT("Resting contact velocity cannot rotate the chase frame"),Track.Update(Noise,Dt).Equals(Settled,1.e-10));
        }
        const FVector Before=Track.Direction;
        const FVector After=Track.Update(FVector(50,0,0),Dt);
        TestTrue(TEXT("Reacquisition obeys the camera angular-rate bound"),FMath::Acos(FMath::Clamp(FVector::DotProduct(Before,After),-1.,1.))<=FMath::DegreesToRadians(65.)*Dt+1.e-6);
        TestTrue(TEXT("Camera direction remains finite after reacquisition"),!After.ContainsNaN());
    }
    return true;
}
#endif
