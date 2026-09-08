"""One-time source migration. Replacements assert their expected preconditions."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2] / 'Source/SuperHeavySim'

def replace(path, old, new):
    target=ROOT/path
    source=target.read_text(encoding='utf-8-sig')
    assert old in source, (path, old[:100])
    target.write_text(source.replace(old,new),encoding='utf-8')

guidance=Path('Private/Recovery/Flight/RecoveryGuidance.cpp')
target=ROOT/guidance
source=target.read_text(encoding='utf-8-sig')
start=source.index('void ASuperHeavyRecoveryDirector::ApplyThrust(')
end=source.index('void ASuperHeavyRecoveryDirector::UpdateVisualActuators()',start)
source=source[:start]+source[end:]
start=source.index('    // Equivalent reaction-control couple')
end=source.index('    // Inverse-square gravity correction',start)
source=source[:start]+'    ApplyReactionControl(Torque,Dt);\n'+source[end:]
start=source.index('    const double Pitch=FMath::RadiansToDegrees',source.index('void ASuperHeavyRecoveryDirector::UpdateVisualActuators'))
source=source[:start]+'''    const double PerEngine=RuntimeProfile->EngineThrustN*EngineIspS/RuntimeProfile->SpecificImpulseSeaLevelS;
    for(const auto& Engine:Engines)
    {
        Vehicle->SetEngineThrottleCommand(Engine.Id,FMath::Clamp(Engine.ThrustN/PerEngine,0.,1.));
        if(Engine.bGimballed)
            Vehicle->SetEngineGimbalCommand(Engine.Id,
                FMath::RadiansToDegrees(FMath::Atan2(Engine.DirectionBody.X,Engine.DirectionBody.Z)),
                FMath::RadiansToDegrees(FMath::Atan2(Engine.DirectionBody.Y,Engine.DirectionBody.Z)));
    }
}
'''
source=source.replace('bSeparated=true; UpdateMass(); SeparationMassKg=MassKg; ActualThrustN=0;', 'SeparateUpperStage();')
source=source.replace('bContactShutdown?FVector::ZeroVector:GridFinAnglesDeg*(PerRad*PI/180.)','GridFinAnglesDeg*(PerRad*PI/180.)')
source=source.replace('DynamicPressurePa<100 || Phase==ERecoveryPhase::Ascent ? 0', 'bContactShutdown || Phase>=ERecoveryPhase::Captured || DynamicPressurePa<100 || Phase<=ERecoveryPhase::Ascent ? 0')
# Physics stores actual actuator state; guidance only changes commands.
source=source.replace('if(bContactShutdown) { ActiveEngines=0;ActualThrustN=0;ForceAccel=FVector::ZeroVector;AppliedGimbal=FVector::ZeroVector; }','if(bContactShutdown) { ActiveEngines=0;ForceAccel=FVector::ZeroVector; }')
target.write_text(source,encoding='utf-8')

director=Path('Private/Recovery/Flight/SuperHeavyRecoveryDirector.cpp')
replace(director,'    bInitialized=true;','    InitializePhysicalActuators();\n    bInitialized=Engines.Num()==33;\n    if(!bInitialized)StatusMessage=TEXT("Expected 33 measured engine sockets");')
replace(director,'    Tower->Release();\n    RuntimeProfile=', '    ReleaseLaunchHoldDown();\n    Tower->Release();\n    RuntimeProfile=')
replace(director,'    SetPhase(ERecoveryPhase::Ready,TEXT("V3 RTLS / estimated mass & aero / SPACE to launch"));','    ResetPhysicalActuators();\n    SetPhase(ERecoveryPhase::Ready,TEXT("RTLS / estimated mass & aero / SPACE to launch"));')
replace(director,'''    if(Phase==ERecoveryPhase::Countdown && PhaseTime>=4)
    { Body->SetSimulatePhysics(true); Body->SetEnableGravity(true); SetPhase(ERecoveryPhase::Ascent,TEXT("Liftoff / 33 Raptor engines / full propellant load")); }''','''    if(Phase==ERecoveryPhase::Ready || Phase==ERecoveryPhase::Countdown)
    {
        ApplyAerodynamics(FVector::UpVector,Dt);
        if(Phase==ERecoveryPhase::Countdown && PhaseTime>=3.)
        {
            ActiveEngines=33;
            ApplyThrust(Body->GetUpVector()*(33*RuntimeProfile->EngineThrustN/MassKg),Body->GetUpVector(),Dt);
            if(PhaseTime>=4. && ActualThrustN>MassKg*Gravity*1.1)
            { ReleaseLaunchHoldDown();SetPhase(ERecoveryPhase::Ascent,TEXT("Mount released / 33 engines / physical liftoff")); }
        }
    }''')
# Do not evaluate propulsion twice on the release frame.
replace(director,'    if(Phase>=ERecoveryPhase::Ascent && Phase<=ERecoveryPhase::Capture)','    else if(Phase>=ERecoveryPhase::Ascent && Phase<=ERecoveryPhase::Capture)')
replace(director,'        ApplyAerodynamics(FVector::UpVector,Dt);\n        const double HoldDrift=', '        ApplyAerodynamics(FVector::UpVector,Dt);\n        ApplyThrust(FVector::ZeroVector,FVector::UpVector,Dt);\n        const double HoldDrift=')
replace(director,'    UpdateVisualActuators();\n    if(bReviewScreenshots', '''    if(Phase==ERecoveryPhase::Aborted)
    {
        ActiveEngines=0;
        ApplyAerodynamics(Body->GetUpVector(),Dt);
        ApplyThrust(FVector::ZeroVector,Body->GetUpVector(),Dt);
    }
    TickUpperStage(Dt);
    UpdateVisualActuators();
    if(bReviewScreenshots''')
replace(Path('Private/Recovery/Flight/RecoveryContacts.cpp'),'    bSeparated=true;PropellantKg=75000;UpdateMass();','    ReleaseLaunchHoldDown();\n    bSeparated=true;PropellantKg=75000;UpdateMass();')

presentation=Path('Private/Recovery/Presentation/RecoveryPresentationComponent.cpp')
target=ROOT/presentation
source=target.read_text(encoding='utf-8-sig')
start=source.index('    if(!D->bSeparated)')
end=source.index('    for(int I=0;I<RcsPods.Num();++I)',start)
source=source[:start]+'''    const FTransform ShipPose=D->GetUpperStageBaseTransform();
    UpperStage->SetWorldLocationAndRotation(ShipPose.GetLocation(),ShipPose.GetRotation());
    UpperStage->SetVisibility(P && P->UpperStageMassKg>0 && (!D->bSeparated || (ShipPose.GetLocation()-Base).Size()<4000000));
    const auto& Positions=FlightGeometry::ReactionNozzlePositionsM();
    const auto& Forces=D->GetReactionForcesBodyN();
'''+source[end:]
target.write_text(source,encoding='utf-8')
replace(Path('Public/Recovery/Presentation/RecoveryPresentationComponent.h'),'    FVector ShipPosition,ShipVelocity,ShipUp;\n    FQuat ShipRotation;\n    bool bBuilt=false,bShipReleased=false,bTrailWasRunning=false;','    bool bBuilt=false,bTrailWasRunning=false;')

profile=Path('Private/Recovery/Flight/SuperHeavyRecoveryProfile.cpp')
replace(profile,'       GridFinAreaM2<=0', '''       GimbalRateDegS<=0 || ReactionValveTimeConstantS<=0 || UpperStageDryMassKg<=0 ||
       (UpperStageMassKg>0 && UpperStageDryMassKg>=UpperStageMassKg) || UpperStageIspS<=0 || UpperStageEngineThrustN<=0 ||
       GridFinAreaM2<=0''')

telemetry=Path('Private/Recovery/Flight/RecoveryTelemetry.cpp')
replace(telemetry,'    Result->SetBoolField(TEXT("physical_capture"),true);','''    Result->SetBoolField(TEXT("physical_capture"),true);
    Result->SetStringField(TEXT("dynamics_model"),TEXT("individual-engine-forces-v1"));
    Result->SetNumberField(TEXT("registered_engines"),Engines.Num());
    Result->SetNumberField(TEXT("peak_engine_force_ratio"),PeakEngineForceRatio);
    Result->SetNumberField(TEXT("peak_gimbal_deg"),PeakAppliedGimbalDeg);
    Result->SetNumberField(TEXT("separation_velocity_error_mps"),SeparationVelocityErrorMps);
    Result->SetNumberField(TEXT("upper_stage_propellant_kg"),UpperStagePropellantKg);
    Result->SetNumberField(TEXT("upper_stage_fuel_consumed_kg"),UpperStageFuelConsumedKg);
    Result->SetBoolField(TEXT("upper_stage_physical"),bSeparated && GetUpperStageBody() && GetUpperStageBody()->IsSimulatingPhysics());
    Result->SetBoolField(TEXT("launch_hold_released"),bLaunchHoldReleased);''')
print('Strict actuator dynamics source migration complete.')
