#include "Recovery/Flight/RecoveryRailSupport.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryRailFootprintTest,"Recovery.Physics.RailFootprint",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryRailFootprintTest::RunTest(const FString&)
{
    TestTrue(TEXT("The capture axis lies inside the load-bearing rail"),RecoveryContactGeometry::OnUsableRailSpan(0));
    TestTrue(TEXT("A fitting three metres along the rail remains supported"),RecoveryContactGeometry::OnUsableRailSpan(3));
    TestFalse(TEXT("The front rail end cannot support the whole fitting"),RecoveryContactGeometry::OnUsableRailSpan(7));
    TestFalse(TEXT("The rear rail end cannot support the whole fitting"),RecoveryContactGeometry::OnUsableRailSpan(-19));
    TestFalse(TEXT("The end margin excludes near-edge contact"),RecoveryContactGeometry::OnUsableRailSpan(6));
    TestTrue(TEXT("The fitting centre is an eligible contact point"),RecoveryContactGeometry::AtFitting(FVector::ZeroVector));
    TestFalse(TEXT("Hull contact outside a fitting cannot count as support"),RecoveryContactGeometry::AtFitting(FVector(0,0,-1)));
    return true;
}
#endif
