#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Flight/SuperHeavyLaunchTower.h"
#include "Components/PrimitiveComponent.h"
#include "Components/BoxComponent.h"
#include "Vehicle/SuperHeavyVehicleActor.h"

void ASuperHeavyRecoveryDirector::OnVehicleContact(UPrimitiveComponent* HitComponent,AActor* OtherActor,UPrimitiveComponent* OtherComponent,FVector NormalImpulse,const FHitResult& Hit)
{
    if(OtherActor!=Tower || !OtherComponent || !RuntimeProfile) return;
    int32 Side=-1;
    if(Tower->IsSupport(OtherComponent,Side) && Hit.ImpactNormal.Z>0.4 && Phase>=ERecoveryPhase::Capture)
    {
        // Validate that the impulse is under one of the actual fitting volumes,
        // not the cylindrical hull or a side hit. Both supports are independent.
        // Use one component transform throughout. Mixing its current rotation
        // with the previous telemetry base position mislabels valid support
        // notifications during delayed game frames.
        const FVector Local=Body->GetComponentQuat().UnrotateVector(Hit.ImpactPoint-Body->GetComponentLocation())/100.+FVector(0,0,BaseOffsetM);
        const FVector Plus=Local-RuntimeProfile->CatchLugPlusM,Minus=Local-RuntimeProfile->CatchLugMinusM;
        if(RecoveryContactGeometry::AtFitting(Plus) || RecoveryContactGeometry::AtFitting(Minus))
        {
            // Support state and loads come directly from the solver manifold.
            // Game-frame notifications remain diagnostic for structural hits.
            return;
        }
    }
    ++StructuralContactCount;
    if(StructuralContactCount<=3) UE_LOG(LogTemp,Warning,TEXT("STRUCTURAL_CONTACT %s t=%.3f phase=%s normal=%s point=%s command=%.4f arm=%s body=%s"),*OtherComponent->GetName(),MissionTime,*GetPhaseLabel(),*Hit.ImpactNormal.ToString(),*Hit.ImpactPoint.ToString(),Tower->CommandedClosure,*OtherComponent->GetComponentTransform().ToString(),*Body->GetComponentTransform().ToString());
}

void ASuperHeavyRecoveryDirector::InitializeContactFixture()
{
    // Test-only initial conditions. Afterwards the body evolves under Chaos
    // without propulsion or any target-pose controller.
    ReleaseLaunchHoldDown();
    bSeparated=true;PropellantKg=75000;UpdateMass();
    FVector P=CaptureWorldM+FVector(0,0,1);
    const double Heading=ContactFixture==TEXT("WrongHeading")?0:RuntimeProfile->CaptureHeadingDeg;
    const FQuat Q=Tower->GetActorQuat()*FQuat(FVector::UpVector,FMath::DegreesToRadians(Heading));
    if(ContactFixture==TEXT("SideImpact")) P+=Tower->GetActorRightVector()*12;
    if(ContactFixture==TEXT("AlongRail")) P+=Tower->GetActorForwardVector()*3;
    if(ContactFixture==TEXT("EmptyTower") || ContactFixture==TEXT("OpenTower") || ContactFixture==TEXT("SleepingClose")) P+=Tower->GetActorForwardVector()*100;
    Body->SetSimulatePhysics(false);
    Vehicle->SetActorLocationAndRotation((P+FVector(0,0,BaseOffsetM))*100,Q,false,nullptr,ETeleportType::TeleportPhysics);
    Body->SetWorldLocationAndRotation((P+FVector(0,0,BaseOffsetM))*100,Q,false,nullptr,ETeleportType::TeleportPhysics);
    Tower->ResetMechanism(1);Body->SetSimulatePhysics(true);Body->SetEnableGravity(true);
    if(ContactFixture==TEXT("OpenTower"))Tower->SetArmClosure(0);
    if(ContactFixture==TEXT("SleepingClose"))
    {
        Tower->ResetMechanism(0);
        for(auto* Part:{Tower->LeftArmCollider.Get(),Tower->RightArmCollider.Get(),Tower->LeftRail.Get(),Tower->RightRail.Get()})Part->PutAllRigidBodiesToSleep();
    }
    Body->SetPhysicsLinearVelocity(ContactFixture==TEXT("SideImpact")?-Tower->GetActorRightVector()*600:FVector::ZeroVector);
    if(ContactFixture==TEXT("Overload"))Body->SetPhysicsLinearVelocity(FVector(0,0,-1600));
    bContactShutdown=true;ActualThrustN=0;ActiveEngines=0;Throttle=0;
    RuntimeProfile->WindVelocityMps=FVector::ZeroVector;
    SetPhase(ERecoveryPhase::Capture,TEXT("Unpowered contact fixture"));
    ++MissionGeneration;InitializeDynamics();
}
void ASuperHeavyRecoveryDirector::TickContactFixture(double Dt)
{
    ActualThrustN=0;ActiveEngines=0;Throttle=0;
    SetFlightCommand(FVector::ZeroVector,FVector::UpVector);
    if(ContactFixture==TEXT("SleepingClose") && MissionTime>1.)Tower->SetArmClosure(1);
    if(MissionTime<5)return;
    const bool Both=EverSupportContact[0] && EverSupportContact[1];
    bool Passed=false;
    if(ContactFixture==TEXT("Centered") || ContactFixture==TEXT("AlongRail"))Passed=Both && StructuralContactCount==0 && VelocityMps.Size()<0.15 && FMath::Abs(AltitudeM-CaptureWorldM.Z)<0.1;
    else if(ContactFixture==TEXT("WrongHeading"))Passed=!Both && AltitudeM<CaptureWorldM.Z-0.5;
    else if(ContactFixture==TEXT("SideImpact"))Passed=StructuralContactCount>0 && !Both;
    else if(ContactFixture==TEXT("Overload"))Passed=Tower->BrokenRailMask!=0 && AltitudeM<CaptureWorldM.Z-.5;
    else if(ContactFixture==TEXT("EmptyTower"))Passed=Tower->BrokenRailMask==0 && Tower->RailCompressionM.GetAbsMax()<.01;
    else if(ContactFixture==TEXT("OpenTower"))Passed=Tower->BrokenRailMask==0 && Tower->ArmClosure<.02;
    else if(ContactFixture==TEXT("SleepingClose"))Passed=Tower->BrokenRailMask==0 && Tower->ArmClosure>.98;
    if(ContactFixture!=TEXT("Overload") && ContactFixture!=TEXT("SideImpact"))Passed&=Tower->IsMechanismDynamic() && Tower->BrokenHingeMask==0;
    WriteResult(Passed,TEXT("Unpowered contact fixture evaluated after five seconds"));
}
