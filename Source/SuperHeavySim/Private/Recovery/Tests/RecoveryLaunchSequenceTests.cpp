#include "Recovery/Flight/RecoveryLaunchSequence.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryLaunchSequenceTest,"Recovery.Ground.TerminalSequence",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryLaunchSequenceTest::RunTest(const FString&)
{
    for(const double Step:{1./120.,1./30.,1./15.})
    {
        FRecoveryLaunchSequence Sequence;
        TestTrue(TEXT("Home has ground replenishment"),Sequence.IsGroundSupplyConnected());
        TestFalse(TEXT("Home cannot ignite engines"),Sequence.IsIgnitionCommanded());
        Sequence.Start();
        double Elapsed=0,FirstWater=-1,FirstIgnition=-1;
        while(Elapsed<61 && !Sequence.bReleaseRequested && !Sequence.bAborted)
        {
            // Independent engine response fixture: readiness only after two seconds of ignition.
            const bool ThrustReady=FirstIgnition>=0 && Elapsed-FirstIgnition>=2.;
            Sequence.Advance(Step,true,ThrustReady);Elapsed+=Step;
            if(FirstWater<0 && Sequence.DelugeDemand()>0)FirstWater=Elapsed;
            if(FirstIgnition<0 && Sequence.IsIgnitionCommanded())FirstIgnition=Elapsed;
            if(Sequence.RemainingS>3.)TestTrue(TEXT("Ground supply remains connected before engine start"),Sequence.IsGroundSupplyConnected());
        }
        TestTrue(TEXT("A healthy vehicle completes the real minute"),Sequence.bReleaseRequested && FMath::Abs(Elapsed-60.)<Step*1.1);
        TestTrue(TEXT("Water starts ten seconds before liftoff"),FMath::Abs(FirstWater-50.)<Step*1.1);
        TestTrue(TEXT("Ignition starts three seconds before liftoff"),FMath::Abs(FirstIgnition-57.)<Step*1.1);
        TestFalse(TEXT("Released vehicle cannot receive ground propellant"),Sequence.IsGroundSupplyConnected());
    }
    FRecoveryLaunchSequence Hold;Hold.Start();Hold.Advance(29,true,false);
    Hold.Advance(4,false,false);
    TestTrue(TEXT("Cold interlock failure holds the actual count"),Hold.bHeld && Hold.RemainingS==31 && !Hold.IsIgnitionCommanded());
    Hold.Advance(1,true,false);
    TestTrue(TEXT("Restored readiness resumes without rewinding or skipping"),!Hold.bHeld && Hold.RemainingS==30);
    Hold.Advance(27,true,false);Hold.Advance(.1,false,false);
    TestTrue(TEXT("Post-ignition failure aborts and retains the mount"),Hold.bAborted && !Hold.bReleaseRequested && !Hold.IsIgnitionCommanded());
    TestEqual(TEXT("Water continues cooling after an ignition abort"),Hold.DelugeDemand(),1.);
    FRecoveryLaunchSequence Weak;Weak.Start();Weak.Advance(60,true,false);
    TestTrue(TEXT("A timer alone cannot release a vehicle without thrust"),Weak.bAborted && !Weak.bReleaseRequested);
    FRecoveryLaunchSequence Invalid;Invalid.Start();Invalid.Advance(-1,true,true);
    TestEqual(TEXT("Negative elapsed time cannot alter the count"),Invalid.RemainingS,60.);
    return true;
}
#endif
