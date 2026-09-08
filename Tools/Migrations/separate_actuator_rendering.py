"""Move actuator mesh commands into presentation and consume physical engine state."""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]/'Source/SuperHeavySim'
flight=ROOT/'Private/Recovery/Flight/RecoveryGuidance.cpp'
source=flight.read_text(encoding='utf-8-sig')
start=source.index('void ASuperHeavyRecoveryDirector::UpdateVisualActuators()')
flight.write_text(source[:start],encoding='utf-8')
header=ROOT/'Public/Recovery/Flight/SuperHeavyRecoveryDirector.h'
header.write_text(header.read_text(encoding='utf-8-sig').replace('    void UpdateVisualActuators();\n',''),encoding='utf-8')
director=ROOT/'Private/Recovery/Flight/SuperHeavyRecoveryDirector.cpp'
source=director.read_text(encoding='utf-8-sig').replace(' UpdateVisualActuators();','').replace('    UpdateVisualActuators();\n','')
director.write_text(source,encoding='utf-8')
presentation=ROOT/'Private/Recovery/Presentation/RecoveryPresentationComponent.cpp'
source=presentation.read_text(encoding='utf-8-sig')
source=source.replace('        Plumes.Add(Plume);Nozzles.Add(Socket);EngineIds.Add(C->GetFName());','''        Plumes.Add(Plume);Nozzles.Add(Socket);EngineIds.Add(C->GetFName());
        EngineIndices.Add(D->GetEngines().IndexOfByPredicate([C](const FRecoveryEngineState& E){return E.Id==C->GetFName();}));''')
start=source.index('    Clock+=Dt;')
source=source[:start]+source[start:].replace('    Clock+=Dt;','    SyncActuatorMeshes();\n    Clock+=Dt;',1)
start=source.index('        const FString Id=EngineIds[I].ToString();')
end=source.index('        if(EngineLights[I]->IsVisible()) ++LitEngines;',start)
source=source[:start]+'''        if(!D->GetEngines().IsValidIndex(EngineIndices[I]))continue;
        const auto& Engine=D->GetEngines()[EngineIndices[I]];
        const double Power=FMath::Clamp(Engine.ThrustN/(P->EngineThrustN*D->EngineIspS/P->SpecificImpulseSeaLevelS),0.,1.);
        const bool On=Power>.01;
        const FQuat BodyQ=D->GetBody()->GetComponentQuat();
        const FQuat Gimbal=FQuat::FindBetweenNormals(FVector::UpVector,Engine.DirectionBody);
        const FVector Nozzle=FlightGeometry::BoosterBaseCm(*D->GetBody())+BodyQ.RotateVector(Engine.PositionFromBaseM+Gimbal.RotateVector(Engine.NozzleOffsetBodyM))*100;
        const FQuat Orientation=BodyQ*Gimbal;
        Plumes[I]->SetVisibility(On);
        Plumes[I]->SetWorldLocationAndRotation(Nozzle,Orientation);
        const double Flicker=1+0.035*FMath::Sin(Clock*41+I*2.17)+0.02*FMath::Sin(Clock*73+I);
        Plumes[I]->SetWorldScale3D(FVector(1.35+Vacuum*4,1.35+Vacuum*4,(20+30*Power)*(1+Vacuum*0.9)*Flicker));
        EngineLights[I]->SetWorldLocation(Nozzle-Orientation.GetUpVector()*250);
        EngineLights[I]->SetLightColor(FMath::Lerp(FLinearColor(0.48f,0.67f,1.f),FLinearColor(1.f,0.48f,0.2f),float(Power*0.8)));
        EngineLights[I]->SetVisibility(On && LightScale>0);
        EngineLights[I]->SetIntensity(On?25000.*Power*Flicker*LightScale:0.);
'''+source[end:]
source+='''
void URecoveryPresentationComponent::SyncActuatorMeshes()
{
    const auto* D=Cast<ASuperHeavyRecoveryDirector>(GetOwner());
    if(!D || !D->Vehicle || !D->GetProfile())return;
    auto* Vehicle=D->Vehicle.Get();
    Vehicle->SetGridFinAngleCommand(TEXT("GF_XP"),D->GridFinAnglesDeg.X);
    Vehicle->SetGridFinAngleCommand(TEXT("GF_XM"),D->GridFinAnglesDeg.Y);
    Vehicle->SetGridFinAngleCommand(TEXT("GF_YM"),D->GridFinAnglesDeg.Z);
    const double PerEngine=D->GetProfile()->EngineThrustN*D->EngineIspS/D->GetProfile()->SpecificImpulseSeaLevelS;
    for(const auto& Engine:D->GetEngines())
    {
        Vehicle->SetEngineThrottleCommand(Engine.Id,FMath::Clamp(Engine.ThrustN/PerEngine,0.,1.));
        if(Engine.bGimballed)Vehicle->SetEngineGimbalCommand(Engine.Id,
            FMath::RadiansToDegrees(FMath::Atan2(Engine.DirectionBody.X,Engine.DirectionBody.Z)),
            FMath::RadiansToDegrees(FMath::Atan2(Engine.DirectionBody.Y,Engine.DirectionBody.Z)));
    }
}
'''
presentation.write_text(source,encoding='utf-8')
print('ACTUATOR_PRESENTATION_SEPARATED')
