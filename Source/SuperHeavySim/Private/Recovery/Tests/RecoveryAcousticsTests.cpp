#include "Recovery/Presentation/RecoveryAcoustics.h"
#include "Recovery/Presentation/RecoveryPropulsionVisuals.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryAcousticArrivalTest,"Recovery.Presentation.AcousticArrival",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryAcousticArrivalTest::RunTest(const FString&)
{
    using namespace RecoveryAcoustics;
    const FVector Listener(1000,0,0);
    const double Delay=TravelSeconds(FVector::ZeroVector,Listener);
    TestTrue(TEXT("One kilometre delay follows ambient temperature"),Delay>2.8 && Delay<3.1);
    TestTrue(TEXT("Warmer air shortens propagation"),TravelSeconds(FVector::ZeroVector,Listener,25)<TravelSeconds(FVector::ZeroVector,Listener,-15));
    for(double Step:{1./15.,1./60.})
    {
        FHistory History;
        for(int I=0;I<=600;++I)
        {
            const double T=I*Step;
            History.Add({T,FVector::ZeroVector,FVector::ZeroVector,FVector4(T>=1 && T<2?1:0,0,0,0)});
            if(T<1+Delay-Step)TestEqual(TEXT("No sound arrives ahead of its wavefront"),History.Hear(T,Listener).Channels.X,0.);
            if(T>1+Delay+Step && T<2+Delay-Step)TestEqual(TEXT("Impulse retains power after travelling"),History.Hear(T,Listener).Channels.X,1.);
            if(T>2+Delay+Step)TestEqual(TEXT("Shutdown arrives after the same propagation delay"),History.Hear(T,Listener).Channels.X,0.);
        }
        History.Reset();TestEqual(TEXT("Restart cannot replay an old launch"),History.Hear(300,Listener).Channels.X,0.);
    }
    FHistory Ring;
    for(int I=0;I<20000;++I)Ring.Add({I*.05,FVector::ZeroVector,FVector::ZeroVector,FVector4(1,0,0,0)});
    TestEqual(TEXT("History memory is bounded"),Ring.Num(),8192);
    TestEqual(TEXT("Distant sound survives ring wrap"),Ring.Hear(999,FVector(8000,0,0)).Channels.X,1.);
    Ring.Add({0,FVector::ZeroVector,FVector::ZeroVector,FVector4(0,0,0,0)});
    TestEqual(TEXT("Time reversal clears history"),Ring.Num(),1);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryAcousticMixTest,"Recovery.Presentation.AcousticMixAndExhaust",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryAcousticMixTest::RunTest(const FString&)
{
    using namespace RecoveryAcoustics;
    TestTrue(TEXT("Distant observers hear less pressure"),Gain(8000,1)<Gain(3000,1) && Gain(3000,1)<Gain(100,1));
    TestEqual(TEXT("Airborne sound vanishes in near vacuum"),Air(73000),0.);
    TestTrue(TEXT("Receding source pitch falls"),Doppler(FVector(-100,0,0),FVector::ZeroVector,FVector::ForwardVector)<1);
    TestTrue(TEXT("Approaching source pitch rises"),Doppler(FVector(100,0,0),FVector::ZeroVector,FVector::ForwardVector)>1);
    for(double Velocity:{-1000000.,-500.,0.,500.,1000000.})
    { const double Pitch=Doppler(FVector(Velocity,0,0),FVector::ZeroVector,FVector::ForwardVector);TestTrue(TEXT("Mach cone and cuts have bounded pitch"),Pitch>=.65 && Pitch<=1.45); }
    double Slow=1,Fast=1;
    for(int I=0;I<15;++I)Slow=Smooth(Slow,0,1./15.);
    for(int I=0;I<120;++I)Fast=Smooth(Fast,0,1./120.);
    TestTrue(TEXT("Audio release is independent of frame rate"),FMath::IsNearlyEqual(Slow,Fast,1.e-10));
    TestTrue(TEXT("Release becomes quiet promptly"),Slow<.02);
    TestEqual(TEXT("No idle camera vibration without excitation"),VibrationDegrees(0,100,0,0,false),0.);
    TestTrue(TEXT("Camera motion is tightly bounded"),VibrationDegrees(100,0,100,1.e9,true)<=.12);
    const double Tail=RecoveryPropulsionVisuals::ExhaustEnvelope(1,0,.1);
    TestTrue(TEXT("Extinction persists briefly and fades"),Tail>.3 && Tail<.4);
    TestTrue(TEXT("Tail never undercuts delivered thrust"),RecoveryPropulsionVisuals::ExhaustEnvelope(.1,.5,1)>=.5);
    return true;
}
#endif
