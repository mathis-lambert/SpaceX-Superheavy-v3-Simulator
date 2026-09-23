#include "Recovery/Presentation/RecoverySurfaceHistory.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoverySurfaceHistoryTest,"Recovery.Presentation.SurfaceHistory",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoverySurfaceHistoryTest::RunTest(const FString&)
{
    FRecoverySurfaceHistory Fine,Coarse;
    for(int I=0;I<600;++I)Fine.Update(1,.1,1,.4);
    for(int I=0;I<60;++I)Coarse.Update(1,1,1,.4);
    TestTrue(TEXT("Deluge wets the pad"),Fine.Wetness>.9);
    TestTrue(TEXT("History is timestep invariant"),FMath::Abs(Fine.Wetness-Coarse.Wetness)<1.e-10 && FMath::Abs(Fine.Residue-Coarse.Residue)<1.e-10);
    const double Before=Fine.Wetness,Residue=Fine.Residue;
    Fine.Update(1,0,0,0);TestEqual(TEXT("Pause preserves wetness"),Fine.Wetness,Before);
    Fine.Update(1,60,0,0);TestTrue(TEXT("Surface dries gradually after flow ends"),Fine.Wetness<Before && Fine.Wetness>.5);
    TestEqual(TEXT("Residue persists after the event"),Fine.Residue,Residue);
    Fine.Update(2,0,0,0);TestEqual(TEXT("New mission clears state"),Fine.Wetness+Fine.Residue,0.);
    return true;
}
#endif
