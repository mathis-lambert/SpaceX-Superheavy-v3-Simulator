#include "Recovery/Flight/RecoveryWaterContact.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Recovery/Presentation/RecoveryCameraTracking.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryWaterContactTest,"Recovery.Physics.WaterContact",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryWaterContactTest::RunTest(const FString&)
{
    const auto Map=FRecoveryWaterMap::Load();
    TestEqual(TEXT("All packaged mask layers loaded"),Map->Layers.Num(),3);
    TestTrue(TEXT("Offshore Gulf is water"),Map->IsWater(FVector(20000,0,0)));
    TestFalse(TEXT("Launch site is dry"),Map->IsWater(FVector(24,0,0)));
    FRecoveryWaterMap Ocean;
    FRecoveryWaterMap::FLayer L;L.Width=L.Height=8;L.West=-180;L.East=180;L.South=-90;L.North=90;L.Bits.Init(255,8);Ocean.Layers.Add(L);
    FRecoveryBodyKinematics Body;Body.OriginM=FVector(0,0,150);
    RecoveryMass::FProperties Mass;Mass.MassKg=300000;Mass.CentreFromBaseM=30;
    TArray<FRecoveryForceSample> Forces;
    TestEqual(TEXT("Airborne body receives no water load"),RecoveryWaterContact::Apply(Body,Mass,35.44,2,67,4.5,&Ocean,1./120,Forces),0.);
    Body.OriginM.Z=-90;
    const double Volume=RecoveryWaterContact::Apply(Body,Mass,35.44,2,67,4.5,&Ocean,1./120,Forces);
    TestTrue(TEXT("Submerged sealed volume is geometric"),FMath::IsNearlyEqual(Volume,PI*4.5*4.5*67,1.e-6));
    FVector Load=FVector::ZeroVector;for(const auto& F:Forces)Load+=F.ForceN;
    TestTrue(TEXT("Archimedes load equals displaced water weight"),FMath::IsNearlyEqual(Load.Z/(1025*9.80665*Volume),1.,1.e-6));
    Body.VelocityMps=FVector(80,0,-150);Forces.Reset();
    RecoveryWaterContact::Apply(Body,Mass,35.44,2,67,4.5,&Ocean,1./15,Forces);
    Load=FVector::ZeroVector;for(const auto& F:Forces)Load+=F.ForceN;
    TestTrue(TEXT("Water resists horizontal motion"),Load.X<0);
    TestTrue(TEXT("High-speed contact remains finite"),!Load.ContainsNaN() && Load.Size()<1.e10);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryOrbitInputTest,"Recovery.Presentation.OrbitInput",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryOrbitInputTest::RunTest(const FString&)
{
    FRecoveryOrbitInput Fine,Coarse;Fine.Add(25,20);Coarse.Add(25,20);
    Fine.Step(1./120);TestTrue(TEXT("Mouse packet does not jump straight to target"),Fine.Yaw>0 && Fine.Yaw<25);
    for(int32 I=1;I<120;++I)Fine.Step(1./120);
    for(int32 I=0;I<30;++I)Coarse.Step(1./30);
    TestTrue(TEXT("Smoothing is independent of render cadence"),FMath::Abs(Fine.Yaw-Coarse.Yaw)<1.e-8);
    Fine.Yaw=179;Fine.TargetYaw=-179;Fine.Step(.01);
    TestTrue(TEXT("Yaw crosses wrap by shortest arc"),FMath::Abs(FMath::FindDeltaAngleDegrees(179.,Fine.Yaw))<2);
    Fine.Add(0,900);TestEqual(TEXT("Pitch remains bounded"),Fine.TargetPitch,75.);
    return true;
}
#endif
