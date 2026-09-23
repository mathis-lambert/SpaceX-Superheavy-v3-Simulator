#include "Recovery/Presentation/RecoverySolarPosition.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoverySolarTest,"Recovery.Presentation.SolarGeography",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoverySolarTest::RunTest(const FString&)
{
    const auto Sun=[](double Hour,double Zone=-5.,int Day=252){return RecoverySolarPosition::Direction(25.9973,-97.1569,Hour,Zone,Day);};
    TestTrue(TEXT("Morning sun east of Starbase"),Sun(8).X>.8 && Sun(8).Z>0);
    TestTrue(TEXT("Evening sun west of Starbase"),Sun(19).X<-.8 && Sun(19).Z>0);
    // At 97.16 W, September solar noon is around 13:25 CDT, not civil 12:00.
    const FVector Noon=Sun(13.4);
    TestTrue(TEXT("Solar noon due south"),FMath::Abs(Noon.X)<.02 && Noon.Y<-.3);
    TestTrue(TEXT("September noon elevation about 70 degrees"),Noon.Z>.93 && Noon.Z<.95);
    TestTrue(TEXT("Same instant in CDT and CST produces identical sunlight"),Sun(13,-5).Equals(Sun(12,-6),1.e-10));
    TestTrue(TEXT("Summer morning is north of east"),Sun(7,-5,172).Y>0);
    TestTrue(TEXT("Winter morning is south of east"),Sun(8,-6,355).Y<0);
    TestTrue(TEXT("Midnight sun below horizon"),Sun(0).Z<0);
    for(int Day:{1,80,172,252,355})for(int Hour=0;Hour<24;++Hour)TestTrue(TEXT("Finite unit direction"),FMath::IsNearlyEqual(Sun(Hour,-5,Day).Size(),1.,1.e-12));
    return true;
}
#endif
