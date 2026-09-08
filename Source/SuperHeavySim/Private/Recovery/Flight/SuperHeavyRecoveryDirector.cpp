#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Shared/RecoveryAssets.h"
#include "Recovery/Tests/RecoveryDiagnosticsComponent.h"
#include "Recovery/Flight/RecoveryAtmosphere.h"
#include "Recovery/Presentation/RecoveryPresentationComponent.h"
#include "Recovery/Presentation/RecoverySkyComponent.h"
#include "Recovery/Presentation/RecoveryVaporComponent.h"
#include "Recovery/Presentation/RecoverySiteDetailsComponent.h"
#include "Recovery/Presentation/RecoverySiteActivityComponent.h"
#include "Recovery/Presentation/RecoveryAudioComponent.h"
#include "Recovery/Presentation/RecoveryForceDisplayComponent.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Recovery/Interface/RecoveryPlayerController.h"
#include "Recovery/Flight/SuperHeavyLaunchTower.h"
#include "Vehicle/SuperHeavyVehicleActor.h"
#include "Autopilot/SuperHeavyAutopilotComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/BoxComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Components/ChildActorComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformFileManager.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/ConstructorHelpers.h"
#include "PhysicsEngine/PhysicsSettings.h"

#include "Recovery/Shared/RecoveryLog.h"

ASuperHeavyRecoveryDirector::ASuperHeavyRecoveryDirector()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.TickGroup=TG_PrePhysics;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("MissionOrigin"));
    CreateDefaultSubobject<URecoveryPresentationComponent>(TEXT("FlightPresentation"));
    CreateDefaultSubobject<URecoverySkyComponent>(TEXT("SkyAndCamera"));
    CreateDefaultSubobject<URecoveryVaporComponent>(TEXT("ParticipatingVapor"));
    CreateDefaultSubobject<URecoverySiteDetailsComponent>(TEXT("IndustrialSiteDetails"));
    CreateDefaultSubobject<URecoverySiteActivityComponent>(TEXT("SiteActivity"));
    CreateDefaultSubobject<URecoveryAudioComponent>(TEXT("FlightAcoustics"));
    CreateDefaultSubobject<URecoveryDiagnosticsComponent>(TEXT("FlightDiagnostics"));
    CreateDefaultSubobject<URecoveryForceDisplayComponent>(TEXT("ForceInspection"));
    static ConstructorHelpers::FClassFinder<ASuperHeavyVehicleActor> Booster(RecoveryAssets::BP_SuperHeavy);
    VehicleClass=Booster.Class;
}

void ASuperHeavyRecoveryDirector::BeginPlay()
{
    Super::BeginPlay();
    RuntimeProfile=MissionProfile ? DuplicateObject<USuperHeavyRecoveryProfile>(MissionProfile,this) : NewObject<USuperHeavyRecoveryProfile>(this);
    if(!Tower) for(TActorIterator<ASuperHeavyLaunchTower> It(GetWorld()); It; ++It) { Tower=*It; break; }
    if(!Tower) Tower=GetWorld()->SpawnActor<ASuperHeavyLaunchTower>();
    bExitAfterTest=FParse::Param(FCommandLine::Get(),TEXT("RecoveryAutoExit"));
    bChaseReview=FParse::Param(FCommandLine::Get(),TEXT("RecoveryChaseReview"));
    bEarthReview=FParse::Param(FCommandLine::Get(),TEXT("RecoveryEarthReview"));
    bIgnoreCameraInput=FParse::Param(FCommandLine::Get(),TEXT("RecoveryReview")) ||
        FParse::Param(FCommandLine::Get(),TEXT("RecoveryCloudReview")) ||
        FParse::Param(FCommandLine::Get(),TEXT("RecoveryVaporReview"));
    FParse::Value(FCommandLine::Get(),TEXT("RecoveryReportName="),ReportName);
    FString TestScenario;
    if(FParse::Value(FCommandLine::Get(),TEXT("RecoveryScenario="),TestScenario))
        ScenarioIndex=TestScenario==TEXT("Crosswind") ? 1 : TestScenario==TEXT("Offset") ? 2 : 0;
    InitializeVehicle();
    if(!bInitialized) { SetPhase(ERecoveryPhase::Aborted,StatusMessage); WriteResult(false,StatusMessage); return; }
    SelectScenario(ScenarioIndex);
    Camera=GetWorld()->SpawnActor<ACameraActor>();
    FParse::Value(FCommandLine::Get(),TEXT("RecoveryCamera="),CameraMode);
    CameraMode=FMath::Clamp(CameraMode,0,CameraCount-1);
    Camera->GetCameraComponent()->SetFieldOfView(55);
    // Establish a view above the pad before the renderer's first shadow pass.
    UpdateCamera(0);
    if(auto* PC=GetWorld()->GetFirstPlayerController())
    {
        // Disable development view-mode shortcuts on the game player only.
        if(PC->PlayerInput) PC->PlayerInput->DebugExecBindings.RemoveAll([](const FKeyBind& Bind)
        { return Bind.Key==EKeys::F1 || Bind.Key==EKeys::F2 || Bind.Key==EKeys::F3; });
        PC->SetViewTarget(Camera);
        // All viewer input is owned by ARecoveryPlayerController.
    }
    FParse::Value(FCommandLine::Get(),TEXT("RecoveryContactFixture="),ContactFixture);
    if(!ContactFixture.IsEmpty()) InitializeContactFixture();
    else if(bAutoStart && !ARecoveryPlayerController::ShouldShowFrontend() && !FParse::Param(FCommandLine::Get(),TEXT("RecoveryManualStart"))) StartMission();
}

void ASuperHeavyRecoveryDirector::InitializeVehicle()
{
    if(!VehicleClass || !Tower) { StatusMessage=TEXT("Vehicle or tower missing"); return; }
    const FVector Start=Tower->GetActorTransform().TransformPosition(RuntimeProfile->LaunchOffsetM*100)+FVector(0,0,3544);
    const FTransform Xform(Tower->GetActorQuat(),Start,FVector(22.5,22.5,80));
    Vehicle=GetWorld()->SpawnActorDeferred<ASuperHeavyVehicleActor>(VehicleClass,Xform,this,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if(!Vehicle) { StatusMessage=TEXT("Vehicle could not be spawned"); return; }
    Vehicle->FinishSpawning(Xform);
    // Reuse the original visual assembly and atomic commands. Only this director applies propulsion.
    Vehicle->SetActorTickEnabled(false);
    for(auto* C : Vehicle->GetComponents())
    {
        if(auto* AP=Cast<USuperHeavyAutopilotComponent>(C)) { AP->bApplyCommandsToVehicle=false; AP->SetComponentTickEnabled(false); }
        if(C->GetFName()==TEXT("COL_Body_Main")) Body=Cast<UPrimitiveComponent>(C);
    }
    if(!Body) { StatusMessage=TEXT("Physics body missing"); return; }
    Body->SetMassOverrideInKg(NAME_None,RuntimeProfile->LaunchMassKg(),true);
    Body->SetLinearDamping(0);
    Body->SetAngularDamping(0);
    Body->BodyInstance.bUseCCD=true;
    Body->BodyInstance.SetPositionSolverIterationCount(16);
    Body->BodyInstance.SetVelocitySolverIterationCount(8);
    Body->SetNotifyRigidBodyCollision(true);
    Body->OnComponentHit.AddDynamic(this,&ASuperHeavyRecoveryDirector::OnVehicleContact);
    auto* ContactMaterial=NewObject<UPhysicalMaterial>(this);
    ContactMaterial->Friction=0.85f;ContactMaterial->Restitution=0;
    Body->SetPhysMaterialOverride(ContactMaterial);
    Tower->LeftRail->SetPhysMaterialOverride(ContactMaterial);Tower->RightRail->SetPhysMaterialOverride(ContactMaterial);
    // These two contact volumes follow the measured fitting centres. Welding
    // makes them shapes of the same Chaos body, never separately posed bodies.
    for(const FVector Lug : {RuntimeProfile->CatchLugPlusM,RuntimeProfile->CatchLugMinusM})
    {
        auto* Collider=NewObject<UBoxComponent>(Vehicle);
        Collider->SetBoxExtent(FVector(60,45,18));Collider->SetCollisionProfileName(TEXT("PhysicsActor"));
        Collider->SetNotifyRigidBodyCollision(true);Collider->SetGenerateOverlapEvents(false);
        Collider->SetPhysMaterialOverride(ContactMaterial);Collider->BodyInstance.bUseCCD=true;
        Collider->BodyInstance.MassScale=0.0001f;
        Collider->SetWorldLocationAndRotation(Body->GetComponentLocation()+Body->GetComponentQuat().RotateVector((Lug-FVector(0,0,BaseOffsetM))*100),Body->GetComponentQuat());
        Collider->RegisterComponent();Vehicle->AddInstanceComponent(Collider);
        Collider->AttachToComponent(Body,FAttachmentTransformRules::KeepWorldTransform);
        Collider->WeldTo(Body,NAME_None,true);
        Collider->OnComponentHit.AddDynamic(this,&ASuperHeavyRecoveryDirector::OnVehicleContact);
        CatchColliders.Add(Collider);
    }
    // Child artwork must not contribute mass, contacts or unintended welds to the flight body.
    TArray<AActor*> VehicleChildren;
    Vehicle->GetAllChildActors(VehicleChildren,true);
    for(AActor* Child:VehicleChildren)
    {
        TInlineComponentArray<UPrimitiveComponent*> Primitives(Child);
        for(auto* P:Primitives) { P->SetSimulatePhysics(false); P->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
    }
    UWidgetLayoutLibrary::RemoveAllWidgets(this);
    InitializePhysicalActuators();
    bInitialized=Engines.Num()==33;
    if(!bInitialized)StatusMessage=TEXT("Expected 33 measured engine sockets");
}

void ASuperHeavyRecoveryDirector::SelectScenario(int32 Index)
{
    if(!bInitialized) return;
    ScenarioIndex=FMath::Clamp(Index,0,2);
    ReleaseLaunchHoldDown();
    Tower->Release();
    RuntimeProfile=MissionProfile ? DuplicateObject<USuperHeavyRecoveryProfile>(MissionProfile,this) : NewObject<USuperHeavyRecoveryProfile>(this);
    ScenarioName=ScenarioIndex==1 ? TEXT("Crosswind") : ScenarioIndex==2 ? TEXT("Offset") : TEXT("Nominal");
    if(ScenarioIndex==1) RuntimeProfile->WindVelocityMps=FVector(0,14,0);
    if(ScenarioIndex==2) { RuntimeProfile->DryMassKg*=1.05; }
    // One launch/catch axis. The mount is directly between the tower arms.
    RuntimeProfile->LaunchOffsetM.X=Tower->CaptureOffsetM.X;
    RuntimeProfile->LaunchOffsetM.Y=Tower->CaptureOffsetM.Y;
    CaptureWorldM=Tower->GetCaptureBaseWorld()/100;
    LaunchWorldM=Tower->GetActorTransform().TransformPosition(RuntimeProfile->LaunchOffsetM*100)/100;
    Body->SetSimulatePhysics(false);
    Vehicle->SetActorLocationAndRotation((LaunchWorldM+FVector(0,0,BaseOffsetM))*100,Tower->GetActorQuat(),false,nullptr,ETeleportType::TeleportPhysics);
    Body->SetWorldLocationAndRotation((LaunchWorldM+FVector(0,0,BaseOffsetM))*100,Tower->GetActorQuat(),false,nullptr,ETeleportType::TeleportPhysics);
    PropellantKg=RuntimeProfile->PropellantMassKg; RcsPropellantKg=RuntimeProfile->ReactionControlPropellantKg; bSeparated=false;
    UpdateMass(); InitialMassKg=MassKg;
    Body->SetPhysicsLinearVelocity(FVector::ZeroVector);
    Body->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
    MissionTime=0; PhaseTime=0; CaptureDwell=0; ActualThrustN=0; Throttle=0; ActiveEngines=0;
    IntegralXY=FVector::ZeroVector; AppliedGimbal=FVector::ZeroVector;
    PeakAltitudeM=0; PeakTiltDeg=0; SampleClock=0; bResultWritten=false;
    CaptureErrorAtLatch=0; CaptureSpeedAtLatch=0; CaptureTiltAtLatch=0; LatchPositionM=FVector::ZeroVector;
    Csv=TEXT("time_s,phase,x_m,y_m,base_altitude_m,vx_mps,vy_mps,vz_mps,tilt_deg,target_error_m,throttle,engines,arm_closure,mass_kg,propellant_kg,density_kgm3,q_pa,mach,heading_error_deg,fin_xp_deg,fin_xm_deg,fin_ym_deg,thrust_n,predicted_miss_m,lug_error_m\n");
    Trace.Reset(); PhaseEvents.Reset();
    MainFuelConsumedKg=0; SeparationMassKg=0; CaptureHeadingAtLatch=0; CaptureLugAtLatch=0; PredictorClock=0;
    LandingIgnitionAltitudeM=0; UnpoweredSeconds=0; BoostbackIgnitionAltitudeM=0; PeakDynamicPressurePa=0;
    LandingBurnSeconds=0; BoostbackSeconds=0; FinControlSeconds=0; PeakDownrangeM=0; PeakSpeedMps=0;
    bUnpoweredViolation=false; GridFinAnglesDeg=FVector::ZeroVector; AeroForceN=FVector::ZeroVector;
    bContactShutdown=false;SupportContactCount=0;SupportImpulseNs=FVector2D::ZeroVector;
    LastSupportContact[0]=LastSupportContact[1]=-100;EverSupportContact[0]=EverSupportContact[1]=false;
    StructuralContactCount=0;SettledContactSeconds=0;bApproachAligned=false;
    GridFinAuthority=0; PredictedMissM=0; TimeToImpactS=0; PredictedImpactM=FVector::ZeroVector;
    ResetPhysicalActuators();
    ++MissionGeneration;
    LaunchSequence=FRecoveryLaunchSequence();GroundClockS=0;DelugeFlow=0;
    Experiment=FRecoveryFlightExperiment();LandingEngineGroup=13;ChaseTracking=FRecoveryChaseTracking();
    SetPhase(ERecoveryPhase::Ready,TEXT("RTLS / estimated mass & aero / SPACE to launch"));
    UpdateNavigation();
}

void ASuperHeavyRecoveryDirector::StartMission()
{
    if(!bInitialized || Phase!=ERecoveryPhase::Ready) return;
    FString Reason;
    if(!RuntimeProfile->Validate(Reason)) { SetPhase(ERecoveryPhase::Aborted,Reason); WriteResult(false,Reason); return; }
    if(!Tower->GetActorScale3D().Equals(FVector::OneVector,0.001) || Tower->GetActorUpVector().Z<0.9999 ||
        Tower->CaptureOffsetM.Z+Tower->ArmContactHeightAboveBaseM>Tower->TowerHeightM-2 ||
        RuntimeProfile->ApogeeM<Tower->TowerHeightM+30)
    { SetPhase(ERecoveryPhase::Aborted,TEXT("Tower requires unit scale, vertical rails and 30 m of flight clearance")); WriteResult(false,StatusMessage); return; }
    LaunchSequence.Start();MissionTime=-LaunchSequence.RemainingS;
    SetPhase(ERecoveryPhase::Countdown,LaunchSequence.Label());
}

void ASuperHeavyRecoveryDirector::RestartMission() { SelectScenario(ScenarioIndex); StartMission(); }
void ASuperHeavyRecoveryDirector::AbortMission()
{
    if(Phase==ERecoveryPhase::Captured || Phase==ERecoveryPhase::Aborted) return;
    LaunchSequence.Abort();
    SetPhase(ERecoveryPhase::Aborted,TEXT("Operator abort / engines shut down"));
    ActualThrustN=0; ActiveEngines=0; Throttle=0; WriteResult(false,StatusMessage);
}

FString ASuperHeavyRecoveryDirector::GetPhaseLabel() const
{
    static const TCHAR* Labels[]={TEXT("READY"),TEXT("COUNTDOWN"),TEXT("ASCENT"),TEXT("SEPARATION"),TEXT("BOOSTBACK"),TEXT("COAST"),TEXT("ENTRY"),TEXT("LANDING BURN"),TEXT("CAPTURE"),TEXT("SECURED"),TEXT("ABORTED")};
    return Labels[static_cast<int>(Phase)];
}

void ASuperHeavyRecoveryDirector::SetPhase(ERecoveryPhase NewPhase,const FString& Message)
{
    Phase=NewPhase; PhaseTime=0; StatusMessage=Message;
    if(NewPhase!=ERecoveryPhase::Aborted) LastFlightPhase=NewPhase;
    PhaseEvents.Add(FString::Printf(TEXT("%s %.2fs %.0fm %.0fkg"),*GetPhaseLabel(),MissionTime,AltitudeM,MassKg));
    UE_LOG(LogRecovery,Display,TEXT("RECOVERY phase=%s t=%.2f %s"),*GetPhaseLabel(),MissionTime,*Message);
}

void ASuperHeavyRecoveryDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if(!bInitialized) return;
    // Match FChaosScene::SetUpForFrame: the solver caps long frames, notably when
    // the editor is throttled in the background. Mission time and actuator lag
    // must advance by simulated time, not by the discarded wall-clock interval.
    const auto* Physics=UPhysicsSettings::Get();
    const double PhysicsBudget=Physics->bSubstepping ? Physics->MaxSubstepDeltaTime*Physics->MaxSubsteps : Physics->MaxPhysicsDeltaTime;
    const double Dt=PhysicsBudget>0 ? FMath::Min(double(DeltaSeconds),PhysicsBudget) : double(DeltaSeconds);
    if(Dt<=0) return;
    AppliedForces.Reset(48);
    AppliedForceFrame=Body->GetComponentTransform();
    PhaseTime+=Dt;
    if(Phase!=ERecoveryPhase::Ready && Phase!=ERecoveryPhase::Countdown && !bResultWritten) MissionTime+=Dt;
    UpdateNavigation();
    if(Phase==ERecoveryPhase::Countdown) TickLaunchSequence(Dt);
    TickGroundConditioning(Dt);
    PeakAltitudeM=FMath::Max(PeakAltitudeM,AltitudeM); PeakTiltDeg=FMath::Max(PeakTiltDeg,TiltDeg);
    if(Phase==ERecoveryPhase::Ready || Phase==ERecoveryPhase::Countdown)
    {
        ApplyAerodynamics(FVector::UpVector,Dt);
        if(Phase==ERecoveryPhase::Countdown && LaunchSequence.IsIgnitionCommanded())
        {
            ActiveEngines=33;
            ApplyThrust(Body->GetUpVector()*(33*RuntimeProfile->EngineThrustN/MassKg),Body->GetUpVector(),Dt);
        }
    }
    else if(Phase>=ERecoveryPhase::Ascent && Phase<=ERecoveryPhase::Capture)
    {
        if(MissionTime>RuntimeProfile->TimeoutSeconds || (Phase>=ERecoveryPhase::LandingBurn && TiltDeg>70) || AltitudeM < -3 || BasePositionM.ContainsNaN())
        { SetPhase(ERecoveryPhase::Aborted,TEXT("Flight envelope exceeded")); ActualThrustN=0; Throttle=0; ActiveEngines=0; WriteResult(false,StatusMessage); }
        else Guide(Dt);
    }
    if(Phase==ERecoveryPhase::Captured)
    {
        ActualThrustN=0; Throttle=0; ActiveEngines=0;
        // Gravity and aerodynamic loads remain active after shutdown. The rails
        // carry the vehicle; no constraint, pose override or velocity reset.
        ApplyAerodynamics(FVector::UpVector,Dt);
        ApplyThrust(FVector::ZeroVector,FVector::UpVector,Dt);
        const double HoldDrift=(BasePositionM-LatchPositionM).Size();
        if(HoldDrift>1. || TiltDeg>5.)
        { SetPhase(ERecoveryPhase::Aborted,TEXT("Physical support lost after engine shutdown")); WriteResult(false,StatusMessage); }
        else if(PhaseTime>8 && !bResultWritten)
            WriteResult(VelocityMps.Size()<0.15 && EverSupportContact[0] && EverSupportContact[1],TEXT("Physical rail support evaluated for eight seconds with engines off"));
    }
    if(Phase==ERecoveryPhase::Aborted)
    {
        ActiveEngines=0;
        ApplyAerodynamics(Body->GetUpVector(),Dt);
        ApplyThrust(FVector::ZeroVector,Body->GetUpVector(),Dt);
    }
    TickUpperStage(Dt);

    SampleClock+=Dt;
    if(SampleClock>=0.1 && !bResultWritten)
    {
        SampleClock=0;
        Trace.Add(FVector2D(MissionTime,AltitudeM));
        if(Trace.Num()>8000) Trace.RemoveAt(0);
        Csv+=FString::Printf(TEXT("%.3f,%s,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%d,%.3f,%.3f,%.3f,%.8f,%.3f,%.4f,%.4f,%.3f,%.3f,%.3f,%.3f,%.3f,%.4f\n"),MissionTime,*GetPhaseLabel(),BasePositionM.X,BasePositionM.Y,AltitudeM,VelocityMps.X,VelocityMps.Y,VelocityMps.Z,TiltDeg,HorizontalErrorM,Throttle,ActiveEngines,Tower->ArmClosure,MassKg,PropellantKg,DensityKgM3,DynamicPressurePa,Mach,HeadingErrorDeg,GridFinAnglesDeg.X,GridFinAnglesDeg.Y,GridFinAnglesDeg.Z,ActualThrustN,PredictedMissM,CatchLugErrorM);
    }
}


void ASuperHeavyRecoveryDirector::EndPlay(const EEndPlayReason::Type Reason)
{
    if(bInitialized && !bResultWritten && MissionTime>0) WriteResult(false,TEXT("Session ended before capture"));
    Super::EndPlay(Reason);
}
