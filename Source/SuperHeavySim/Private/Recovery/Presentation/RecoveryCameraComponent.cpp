#include "Recovery/Presentation/RecoveryCameraComponent.h"
#include "Recovery/Presentation/RecoveryPresentationComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Recovery/Presentation/RecoveryAudioComponent.h"
#include "Recovery/Presentation/RecoveryPropulsionVisuals.h"
#include "Recovery/Flight/SuperHeavyRecoveryDirector.h"
#include "Recovery/Flight/SuperHeavyLaunchTower.h"
#include "Recovery/Interface/RecoveryPlayerController.h"
#include "Recovery/Shared/FlightGeometry.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"

URecoveryCameraComponent::URecoveryCameraComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void URecoveryCameraComponent::BeginPlay()
{
    Super::BeginPlay();
    if (auto* Presentation = GetOwner()->FindComponentByClass<URecoveryPresentationComponent>())
        AddTickPrerequisiteComponent(Presentation);
    bChaseReview = FParse::Param(FCommandLine::Get(), TEXT("RecoveryChaseReview"));
    bEarthReview = FParse::Param(FCommandLine::Get(), TEXT("RecoveryEarthReview"));
    bIgnoreCameraInput = FParse::Param(FCommandLine::Get(), TEXT("RecoveryReview")) ||
                         FParse::Param(FCommandLine::Get(), TEXT("RecoveryInteractiveAudit")) ||
                         FParse::Param(FCommandLine::Get(), TEXT("RecoveryCloudReview")) ||
                         FParse::Param(FCommandLine::Get(), TEXT("RecoveryVaporReview"));
    FParse::Value(FCommandLine::Get(), TEXT("RecoveryCamera="), CameraMode);
    CameraMode = FMath::Clamp(CameraMode, 0, CameraCount - 1);
    if (bIgnoreCameraInput && FParse::Value(FCommandLine::Get(), TEXT("RecoveryReviewZoom="), CameraZoom))
        CameraZoom = FMath::Clamp(CameraZoom, .25, 6.);
    Camera = GetWorld()->SpawnActor<ACameraActor>(FVector(24500, -31000, 9500), FRotator::ZeroRotator);
    if (Camera)
    {
        Camera->GetCameraComponent()->SetFieldOfView(55);
        if (auto* PC = GetWorld()->GetFirstPlayerController())
            PC->SetViewTarget(Camera);
    }
}

void URecoveryCameraComponent::TickComponent(float Dt, ELevelTick Type, FActorComponentTickFunction* Function)
{
    Super::TickComponent(Dt, Type, Function);
    UpdateCamera(Dt);
}

void URecoveryCameraComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (Camera)
        Camera->Destroy();
    Super::EndPlay(Reason);
}

void URecoveryCameraComponent::CycleCamera()
{
    SetCameraMode((CameraMode + 1) % CameraCount);
}

void URecoveryCameraComponent::ToggleFreeCamera()
{
    if (CameraMode == 8)
        CameraMode = PreviousCameraMode;
    else
    {
        PreviousCameraMode = CameraMode;
        CameraMode = 8;
        if (Camera)
            FreeCameraSpeedMps = FMath::Clamp(Camera->GetActorLocation().Z / 300., 50., 2000000.);
    }
}

TArray<FString> URecoveryCameraComponent::GetCameraNames()
{
    return {TEXT("Booster orbit"),
            TEXT("Launch site telephoto"),
            TEXT("Engine orbit"),
            TEXT("Tower tracking"),
            TEXT("Grid fin orbit"),
            TEXT("Onboard / ground"),
            TEXT("Chase orbit"),
            TEXT("Cinematic orbit"),
            TEXT("Free flight"),
            TEXT("Coastal observer / 3 km"),
            TEXT("Distant observer / 8 km"),
            TEXT("Starbase orbit"),
            TEXT("Earth horizon"),
            TEXT("Whole Earth")};
}
FString URecoveryCameraComponent::GetCameraLabel() const
{
    return FString::Printf(TEXT("%02d / %s"), CameraMode + 1,
                           *GetCameraNames()[FMath::Clamp(CameraMode, 0, CameraCount - 1)]);
}

void URecoveryCameraComponent::UpdateCamera(double Dt)
{
    const auto* Flight = Cast<ASuperHeavyRecoveryDirector>(GetOwner());
    if (!Flight)
        return;
    auto* Body = Flight->GetBody();
    const auto* Tower = Flight->Tower.Get();
    const auto* RuntimeProfile = Flight->GetProfile();
    if (!Camera || !Body || !Tower || !RuntimeProfile)
        return;
    if (LastMissionGeneration != Flight->GetMissionGeneration())
    {
        LastMissionGeneration = Flight->GetMissionGeneration();
        ChaseTracking = FRecoveryChaseTracking();
    }
    if (bChaseReview)
        CameraMode = 6;
    // Optional unattended visual review; normal play never changes camera by phase.
    if (bEarthReview)
        CameraMode = Flight->MissionTime >= 195 && Flight->MissionTime < 230 ? 12 : 0;
    if (bFrontendView)
    {
        const FVector Site = Tower->GetActorLocation();
        const double Drift = FMath::Sin(GetWorld()->GetRealTimeSeconds() * 0.025) * 1500;
        const FVector Position = Site + FVector(24500 + Drift, -31000, 9500);
        const FVector Focus = Site + FVector(0, 4500, 6000);
        Camera->SetActorLocationAndRotation(Position, (Focus - Position).Rotation());
        Camera->GetCameraComponent()->SetFieldOfView(48);
        LastCameraFocus = Focus;
        bCameraInitialized = true;
        LastCameraMode = -1;
        return;
    }
    auto* PC = GetWorld()->GetFirstPlayerController();
    auto* Settings = Cast<ARecoveryPlayerController>(PC);
    const double ViewDt = Dt / (Settings ? FMath::Max(.01f, Settings->EffectivePlaybackRate) : 1.f);
    const bool AcceptInput = PC && (!Settings || !Settings->IsMenuOpen()) && !bIgnoreCameraInput;
    const double Sensitivity = Settings ? Settings->MouseSensitivity : 0.65;
    const double VerticalSign = Settings && Settings->bInvertVerticalLook ? -1. : 1.;
    const bool Changed = LastCameraMode != CameraMode;
    if (Changed)
    {
        OrbitInput = {};
        SmoothedCameraZoom = CameraZoom;
        OrbitYaw = 0;
        OrbitPitch = 0;
        CinematicAzimuth = -0.85;
        bOrbitManuallyAdjusted = false;
    }
    const FVector Centre = Body->GetComponentLocation();
    const FQuat Q = Body->GetComponentQuat();
    const FVector Up = Q.GetUpVector();
    const FVector Base = Centre - Up * FlightGeometry::BoosterBaseOffsetM * 100;
    // Update in every view so switching to chase preserves the last meaningful
    // flight direction. Below 2 m/s, hold composition through rail settling.
    const FVector ChaseDirection = ChaseTracking.Update(Body->GetPhysicsLinearVelocity() / 100., Dt);
    if (AcceptInput)
    {
        const int Wheel = (PC->WasInputKeyJustPressed(EKeys::MouseScrollDown) ? 1 : 0) -
                          (PC->WasInputKeyJustPressed(EKeys::MouseScrollUp) ? 1 : 0);
        if (CameraMode == 8)
            FreeCameraSpeedMps = FMath::Clamp(FreeCameraSpeedMps * FMath::Pow(1.4, -Wheel), 5., 2000000.);
        else if (Wheel && Settings && !Settings->Photography.bAutomaticFraming &&
                 (CameraMode == 1 || CameraMode == 3 || CameraMode == 9 || CameraMode == 10))
        {
            Settings->Photography.FocalLengthMm =
                FMath::Clamp(float(Settings->Photography.FocalLengthMm * FMath::Pow(1.18, -Wheel)), 12.f, 600.f);
            Settings->SavePreferences();
        }
        else
            CameraZoom = FMath::Clamp(CameraZoom * FMath::Pow(1.18, Wheel), 0.25, 6.);
    }
    if (CameraMode == 8 && bCameraInitialized)
    {
        const double FreeFov =
            Settings && !Settings->Photography.bAutomaticFraming ? Settings->Photography.HorizontalFovDegrees() : 65.;
        Camera->GetCameraComponent()->SetFieldOfView(
            FMath::FInterpTo(Camera->GetCameraComponent()->FieldOfView, FreeFov, ViewDt, 3.));
        if (AcceptInput)
        {
            float DX = 0, DY = 0;
            if (PC->IsInputKeyDown(EKeys::RightMouseButton) && !PC->WasInputKeyJustPressed(EKeys::RightMouseButton))
                PC->GetInputMouseDelta(DX, DY);
            FRotator R = Camera->GetActorRotation();
            R.Yaw += DX * Sensitivity;
            R.Pitch = FMath::Clamp(R.Pitch + DY * Sensitivity * VerticalSign, -89., 89.);
            R.Roll = 0;
            Camera->SetActorRotation(R);
            const double Forward =
                (PC->IsInputKeyDown(EKeys::Up) || PC->IsInputKeyDown(EKeys::W) || PC->IsInputKeyDown(EKeys::Z) ? 1.
                                                                                                               : 0) -
                (PC->IsInputKeyDown(EKeys::Down) || PC->IsInputKeyDown(EKeys::S) ? 1. : 0);
            const double Side =
                (PC->IsInputKeyDown(EKeys::Right) || PC->IsInputKeyDown(EKeys::D) ? 1. : 0) -
                (PC->IsInputKeyDown(EKeys::Left) || PC->IsInputKeyDown(EKeys::A) || PC->IsInputKeyDown(EKeys::Q) ? 1.
                                                                                                                 : 0);
            const double Vertical = (PC->IsInputKeyDown(EKeys::PageUp) || PC->IsInputKeyDown(EKeys::E) ? 1. : 0) -
                                    (PC->IsInputKeyDown(EKeys::PageDown) || PC->IsInputKeyDown(EKeys::B) ? 1. : 0);
            FVector Motion = Camera->GetActorForwardVector() * Forward + Camera->GetActorRightVector() * Side +
                             FVector::UpVector * Vertical;
            const double Speed = FreeCameraSpeedMps;
            Camera->AddActorWorldOffset(Motion.GetClampedToMaxSize(1) * Speed * 100 * Dt /
                                        (Settings ? FMath::Max(.01f, Settings->EffectivePlaybackRate) : 1.f));
        }
        LastCameraMode = CameraMode;
        LastCameraFocus = Camera->GetActorLocation() + Camera->GetActorForwardVector() * 10000;
        return;
    }
    SmoothedCameraZoom = FMath::Lerp(SmoothedCameraZoom, CameraZoom, 1 - FMath::Exp(-FMath::Max(0., ViewDt) / .10));
    const double Zoom = SmoothedCameraZoom;
    // Follow the current physical position exactly. Smooth only composition and
    // user-selected camera changes; interpolating world position trails a rocket
    // by hundreds of metres during ascent.
    StageFraming = FMath::FInterpTo(StageFraming, Flight->bSeparated ? 1. : 0., Dt, 0.7);
    FVector Focus = Centre + Up * (2700 * (1 - StageFraming));
    FVector Position;
    double Fov = 55;
    const FVector Site = Tower->GetActorLocation();
    switch (CameraMode)
    {
    case 1:
        Position = Site + FVector(55000, -72000, 1600);
        Fov =
            FMath::Clamp(FMath::RadiansToDegrees(2 * FMath::Atan2(10500., (Focus - Position).Size())) * Zoom, 1.2, 55.);
        break;
    case 2:
        Focus = Base + Up * 500;
        Position = Base + Q.RotateVector(FVector(1800, -2400, -1200) * Zoom);
        break;
    case 3:
        Focus = Base + Q.RotateVector((RuntimeProfile->CatchLugPlusM + RuntimeProfile->CatchLugMinusM) * 50);
        Position = Site + FVector(6500, -9500, 6900);
        Fov =
            FMath::Clamp(FMath::RadiansToDegrees(2 * FMath::Atan2(2500., (Focus - Position).Size())) * Zoom, 1.2, 50.);
        break;
    case 4:
        Focus = Base + Q.RotateVector(FVector(100, 0, 6380));
        Position = Focus + Q.RotateVector(FVector(1400, -1850, 170) * Zoom);
        Fov = 58;
        break;
    case 5:
        Position = Base + Up * 7050 + Q.GetForwardVector() * 1100 * Zoom;
        Focus = Base + Up * 1300;
        Fov = 72;
        break;
    case 6: {
        Position = Focus + (-ChaseDirection * 15000 + FVector(8000, -11000, 5000)) * Zoom;
        break;
    }
    case 7: {
        if (!bOrbitManuallyAdjusted && (!Settings || Settings->bAutomaticOrbit))
            CinematicAzimuth += ViewDt * 0.004 * (Settings ? Settings->Photography.OrbitSpeed : 1.f);
        const double Angle = CinematicAzimuth;
        const double Radius = FMath::Lerp(42000., 24500., StageFraming) * Zoom;
        Position = Focus + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, Radius * 0.27);
        Fov = 48;
        break;
    }
    case 9:
        Position = Site + FVector(-290000, -80000, 2200);
        Fov = 42 * Zoom;
        break;
    case 10:
        Position = Site + FVector(-775000, -200000, 3500);
        Fov = 38 * Zoom;
        break;
    case 11:
        Position = Site + FVector(-65000, -85000, 38000) * Zoom;
        Focus = Site + FVector(0, 0, 4000);
        Fov = 58;
        break;
    case 12:
        Position = Centre + FVector(0, -2000000, 1000000) * Zoom;
        Focus = Position + FVector(600000, 3000000, -600000);
        Fov = 65;
        break;
    case 13:
        Focus = Site + FVector(0, 0, -637100000);
        Position = Focus + FVector(0, -350000000, 1637100000) * Zoom;
        Fov = 82;
        break;
    default:
        Focus -= Up * (4000 * (1 - StageFraming));
        Position =
            Focus + FMath::Lerp(FVector(37000, -46000, 13000), FVector(11000, -15000, 6500), StageFraming) * Zoom;
        break;
    }
    const bool OrbitCamera = CameraMode == 0 || CameraMode == 2 || CameraMode == 4 || CameraMode == 6 ||
                             CameraMode == 7 || CameraMode == 11 || CameraMode == 13;
    const bool GroundCamera = CameraMode == 1 || CameraMode == 3 || CameraMode == 9 || CameraMode == 10;
    if (GroundCamera && Settings && Settings->Photography.bFixedFraming)
        Focus = Site + FVector(2400, 0, 6000);
    if (OrbitCamera)
    {
        if (AcceptInput && PC->IsInputKeyDown(EKeys::RightMouseButton) &&
            !PC->WasInputKeyJustPressed(EKeys::RightMouseButton))
        {
            float DX = 0, DY = 0;
            PC->GetInputMouseDelta(DX, DY);
            OrbitInput.Add(DX * Sensitivity, DY * Sensitivity * VerticalSign);
            if (FMath::Abs(DX) + FMath::Abs(DY) > 0.01)
                bOrbitManuallyAdjusted = true;
        }
        OrbitInput.Step(ViewDt);
        OrbitYaw = OrbitInput.Yaw;
        OrbitPitch = OrbitInput.Pitch;
        const FVector Offset = Position - Focus;
        FRotator Orbit = Offset.Rotation();
        Orbit.Yaw += OrbitYaw;
        Orbit.Pitch = FMath::Clamp(Orbit.Pitch + OrbitPitch, -85., 85.);
        Position = Focus + Orbit.Vector() * Offset.Size();
    }
    if (CameraMode != 13)
        Position.Z = FMath::Max(Position.Z, Site.Z + 250.);
    if (Settings && !Settings->Photography.bAutomaticFraming)
        Fov = Settings->Photography.HorizontalFovDegrees();
    Fov = FMath::Clamp(Fov, 1.2, Settings && !Settings->Photography.bAutomaticFraming ? 120. : 110.);
    if (bCameraInitialized && Changed)
    {
        CameraBlendOffset = Camera->GetActorLocation() - Position;
        CameraLookBlend = LastCameraFocus - Focus;
        CameraTransitionRemaining = 1.2;
    }
    // Finite duration avoids kilometre-scale exponential tails after a globe view.
    const double NextRemaining = FMath::Max(0., CameraTransitionRemaining - Dt);
    const double Blend = CameraTransitionRemaining > 0 ? FMath::Pow(NextRemaining / CameraTransitionRemaining, 3.) : 0.;
    CameraBlendOffset *= Blend;
    CameraLookBlend *= Blend;
    CameraTransitionRemaining = NextRemaining;
    Position += CameraBlendOffset;
    Focus += CameraLookBlend;
    Camera->SetActorLocation(Position);
    FRotator ViewRotation = (Focus - Position).Rotation();
    if (GroundCamera && Settings && !Changed && !Settings->Photography.bFixedFraming &&
        Settings->Photography.TrackingLagSeconds > .001f)
    {
        const double BlendFactor = 1 - FMath::Exp(-ViewDt / Settings->Photography.TrackingLagSeconds);
        ViewRotation = FQuat::Slerp(Camera->GetActorQuat(), ViewRotation.Quaternion(), BlendFactor).Rotator();
    }
    // Small angular vibration belongs to the camera, never to the vehicle pose.
    // Fade to zero at physical support so secured Chase framing stays stable.
    if (Settings && Flight->Phase != ERecoveryPhase::Captured)
    {
        const double T = GetWorld()->GetRealTimeSeconds();
        const auto* Acoustics = GetOwner()->FindComponentByClass<URecoveryAudioComponent>();
        const double Delivered =
            RecoveryPropulsionVisuals::DeliveredFraction(Flight->GetEngines(), RuntimeProfile->EngineThrustN);
        const double Power = Settings->Photography.MotionStrength *
                             RecoveryAcoustics::VibrationDegrees(Acoustics ? Acoustics->GetHeardPower() : 0,
                                                                 Acoustics ? Acoustics->GetHeardDistanceM() : 0,
                                                                 Delivered, Flight->DynamicPressurePa, CameraMode == 5);
        ViewRotation.Pitch += Power * (FMath::Sin(T * 8.7) + .35 * FMath::Sin(T * 21.1));
        ViewRotation.Yaw += Power * .6 * FMath::Sin(T * 6.3);
    }
    Camera->SetActorRotation(ViewRotation);
    Camera->GetCameraComponent()->SetFieldOfView(
        bCameraInitialized ? FMath::FInterpTo(Camera->GetCameraComponent()->FieldOfView, Fov, Dt, 2.3) : Fov);
    LastCameraFocus = Focus;
    LastCameraMode = CameraMode;
    bCameraInitialized = true;
}
