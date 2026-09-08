#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Recovery/Flight/SuperHeavyRecoveryProfile.h"
#include "Recovery/Flight/RecoveryActuators.h"
#include "Recovery/Flight/RecoveryGuidanceModel.h"
#include "Recovery/Flight/RecoveryFlightPhase.h"
#include "Recovery/Flight/RecoveryFlightInspection.h"
#include "Recovery/Flight/RecoveryLaunchSequence.h"
#include "Recovery/Presentation/RecoveryCameraTracking.h"
#include "SuperHeavyRecoveryDirector.generated.h"
class UExponentialHeightFogComponent;

class ASuperHeavyLaunchTower;
class ASuperHeavyVehicleActor;
class UPrimitiveComponent;
class ACameraActor;
class UBoxComponent;
class UPhysicsConstraintComponent;
class URecoveryPhysicsAuditComponent;
class URecoveryPhysicsComponent;

/** Autonomous suborbital recovery. Chaos integrates physical thrust, drag and control forces. */
UCLASS(Blueprintable)
class SUPERHEAVYSIM_API ASuperHeavyRecoveryDirector : public AActor
{
    GENERATED_BODY()
public:
    ASuperHeavyRecoveryDirector();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Recovery") TObjectPtr<USuperHeavyRecoveryProfile> MissionProfile;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Recovery") TObjectPtr<ASuperHeavyLaunchTower> Tower;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Recovery") TSubclassOf<ASuperHeavyVehicleActor> VehicleClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Recovery") bool bAutoStart = true;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") ERecoveryPhase Phase = ERecoveryPhase::Ready;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") ERecoveryPhase LastFlightPhase = ERecoveryPhase::Ready;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") double MissionTime = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") double AltitudeM = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") double VerticalSpeedMps = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") double HorizontalErrorM = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") double TiltDeg = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") double Throttle = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") double BrakingDistanceM = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") double CaptureDwell = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") int32 ActiveEngines = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") FString StatusMessage;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") FVector VelocityMps;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") FVector BasePositionM;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") FVector TargetPositionM;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") FString ScenarioName = TEXT("Nominal");
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") TObjectPtr<ASuperHeavyVehicleActor> Vehicle;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") double MassKg=0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") double PropellantKg=0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") double RcsPropellantKg=0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") double DensityKgM3=0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") double PressurePa=0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") double DynamicPressurePa=0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") double Mach=0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") double HeadingErrorDeg=0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") double GridFinAuthority=0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") double PredictedMissM=0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") double TimeToImpactS=0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") double LandingIgnitionAltitudeM=0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") double UnpoweredSeconds=0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") double BoostbackIgnitionAltitudeM=0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") double PeakDynamicPressurePa=0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") double CatchLugErrorM=0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") FVector GridFinAnglesDeg=FVector::ZeroVector;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") FVector PredictedImpactM=FVector::ZeroVector;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Telemetry") bool bSeparated=false;
    UFUNCTION(BlueprintCallable, Category="Recovery") void StartMission();
    UFUNCTION(BlueprintCallable, Category="Recovery") void RestartMission();
    UFUNCTION(BlueprintCallable, Category="Recovery") void AbortMission();
    UFUNCTION(BlueprintCallable, Category="Recovery") void SelectScenario(int32 Index);
    UFUNCTION(BlueprintCallable, Category="Recovery") void CycleCamera();
    UFUNCTION(BlueprintCallable, Category="Recovery") void ToggleFreeCamera();
    UFUNCTION(BlueprintCallable, Category="Recovery") void ToggleTelemetry() { bShowTelemetry=!bShowTelemetry; }
    UFUNCTION(BlueprintPure, Category="Recovery") FString GetCameraLabel() const;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Presentation") bool bShowTelemetry=true;
    bool bFrontendView=false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Contacts") bool bContactShutdown=false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Contacts") int32 SupportContactCount=0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Contacts") FVector2D SupportImpulseNs=FVector2D::ZeroVector;
    static constexpr int32 CameraCount=14;
    static TArray<FString> GetCameraNames();
    int32 GetCameraMode() const { return CameraMode; }
    void SetCameraMode(int32 Mode) { CameraMode=FMath::Clamp(Mode,0,CameraCount-1);CameraZoom=1; }
    UFUNCTION(BlueprintPure, Category="Recovery") FString GetPhaseLabel() const;
    const TArray<FVector2D>& GetTrace() const { return Trace; }
    UPrimitiveComponent* GetBody() const { return Body; }
    const FRecoveryDynamicsState& GetDynamicsState() const { return DynamicsState; }
    const FRecoveryGuidanceState& GetGuidanceState() const { return GuidanceState; }
    FVector GetMassCentreCm() const;
    UPrimitiveComponent* GetUpperStageBody() const;
    FTransform GetUpperStageBaseTransform() const;
    const TArray<FRecoveryEngineState>& GetEngines() const { return Engines; }
    const TArray<FRecoveryForceSample>& GetAppliedForces() const { return AppliedForces; }
    const FTransform& GetForceFrame() const { return AppliedForceFrame; }
    FVector GetWindVelocityMps(double Height) const { return WindAt(Height); }
    const FRecoveryFlightExperiment& GetExperiment() const { return Experiment; }
    void SetFailedEngine(int32 Index);
    void SetJammedFin(int32 Index);
    void SetReactionJetsDisabled(bool Disabled);
    void SetAttitudeResponse(double Value);
    void SetWindScale(double Value);
    void ResetExperiments();
    const TArray<FVector>& GetReactionForcesBodyN() const { return ReactionForcesBodyN; }
    double GetUpperStageThrustN() const { return UpperStageThrustN; }
    FVector2D GetConditioningFlowKgS() const { return ConditioningFlowKgS; }
    const FRecoveryLaunchSequence& GetLaunchSequence() const { return LaunchSequence; }
    double GetDelugeFlow() const { return DelugeFlow; }
    uint32 GetMissionGeneration() const { return MissionGeneration; }
    bool HasMissionResult() const { return bResultWritten; }
    double GetPhaseTimeS() const { return PhaseTime; }
    bool IsLaunchMountReleased() const { return bLaunchHoldReleased; }
    const USuperHeavyRecoveryProfile* GetProfile() const { return RuntimeProfile; }
private:
    friend class URecoveryPresentationComponent;
    friend class URecoveryPhysicsComponent;
    void ConsumeDynamicsState();
    void InitializeDynamics();
    void SubmitDynamicsCommand();
    UPROPERTY(Transient) TObjectPtr<USuperHeavyRecoveryProfile> RuntimeProfile;
    UPROPERTY(Transient) TObjectPtr<UPrimitiveComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<URecoveryPhysicsAuditComponent> PhysicsAudit;
    UPROPERTY(Transient) TObjectPtr<UBoxComponent> UpperStageBody;
    UPROPERTY(Transient) TObjectPtr<UPhysicsConstraintComponent> LaunchHoldDown;
    TArray<FRecoveryEngineState> Engines;
    UPROPERTY(VisibleAnywhere) TObjectPtr<URecoveryPhysicsComponent> PhysicsModel;
    FRecoveryDynamicsState DynamicsState;
    FRecoveryGuidanceState GuidanceState;
    FRecoveryGuidanceConfiguration GuidanceConfiguration;
    int32 ConsumedGuidanceEvents=0;
    void ConsumeGuidanceState();
    FRecoveryDynamicsCommand DynamicsCommand;
    TArray<FRecoveryForceSample> AppliedForces;
    FTransform AppliedForceFrame;
    FRecoveryFlightExperiment Experiment;
    void RecordExperiment(const TCHAR* Action);
    TArray<FVector> ReactionForcesBodyN;
    double UpperStagePropellantKg=0,UpperStageThrustN=0,UpperStageFuelConsumedKg=0;
    double PeakEngineForceRatio=0,PeakAppliedGimbalDeg=0,SeparationVelocityErrorMps=0;
    double SeparationMomentumRelativeError=0,SeparationAngularMomentumRelativeError=0;
    FVector LastEngineForceBodyN=FVector::ZeroVector,LastEngineMomentBodyNm=FVector::ZeroVector;
    bool bLaunchHoldReleased=false;
    FVector2D ConditioningFlowKgS=FVector2D::ZeroVector;
    double ConditioningVentedKg=0,GroundSupplyKg=0;
    FRecoveryLaunchSequence LaunchSequence;
    uint32 MissionGeneration=0;
    double DelugeFlow=0;
    void TickLaunchSequence(double Dt);
    UPROPERTY(Transient) TObjectPtr<ACameraActor> Camera;
    UPROPERTY(Transient) TArray<TObjectPtr<UBoxComponent>> CatchColliders;
    UFUNCTION() void OnVehicleContact(UPrimitiveComponent* HitComponent,AActor* OtherActor,UPrimitiveComponent* OtherComponent,FVector NormalImpulse,const FHitResult& Hit);
    double LastSupportContact[2]={-100,-100};
    bool EverSupportContact[2]={false,false};
    int32 StructuralContactCount=0;
    FString ContactFixture;
    void InitializeContactFixture();
    void TickContactFixture(double Dt);
    FVector LaunchWorldM, CaptureWorldM, LatchPositionM = FVector::ZeroVector;
    FVector AppliedGimbal = FVector::ZeroVector;
    double PhaseTime=0, ActualThrustN=0, BaseOffsetM=35.44, SampleClock=0, PeakAltitudeM=0, PeakTiltDeg=0;
    double CaptureErrorAtLatch=0, CaptureSpeedAtLatch=0, CaptureTiltAtLatch=0;
    bool bExitAfterTest=false, bResultWritten=false, bInitialized=false;
    int32 CameraMode=0, ScenarioIndex=0;
    int32 PreviousCameraMode=0, LastCameraMode=-1;
    double CameraTransitionRemaining=0;
    TWeakObjectPtr<UExponentialHeightFogComponent> LocalHeightFog;
    bool bCameraInitialized=false;
    double CameraZoom=1, FreeCameraSpeedMps=50, StageFraming=0;
    double OrbitYaw=0, OrbitPitch=0, CinematicAzimuth=-0.85;
    bool bOrbitManuallyAdjusted=false;
    FRecoveryChaseTracking ChaseTracking;
    FVector CameraBlendOffset=FVector::ZeroVector, CameraLookBlend=FVector::ZeroVector, LastCameraFocus=FVector::ZeroVector;
    FString Csv;
    FString ReportName;
    bool bChaseReview=false,bEarthReview=false,bIgnoreCameraInput=false;
    TArray<FVector2D> Trace;
    void InitializeVehicle();
    void InitializePhysicalActuators();
    void ResetPhysicalActuators();
    void ReleaseLaunchHoldDown();
    void SeparateUpperStage();
    void TickUpperStage(double Dt);
    void PrepareGroundCommand();
    void SetPhase(ERecoveryPhase NewPhase, const FString& Message);
    void RecordPhase(ERecoveryPhase NewPhase,const FString& Message,double TimeS,double EventAltitudeM,double EventMassKg);
    void UpdateNavigation();
    void SetFlightCommand(const FVector& ThrustAcceleration,const FVector& TargetUp);
    void UpdateMass();
    FVector WindAt(double Height) const;
    double Gravity=9.80665, EngineIspS=327, InitialMassKg=0;
    double MainFuelConsumedKg=0, SeparationMassKg=0, CaptureHeadingAtLatch=0;
    double CaptureLugAtLatch=0, BoostbackDownrangeM=0, PeakDownrangeM=0, PeakSpeedMps=0;
    double LandingBurnSeconds=0, BoostbackSeconds=0, FinControlSeconds=0;
    bool bUnpoweredViolation=false;
    FVector AeroForceN=FVector::ZeroVector, RcsTorqueBody=FVector::ZeroVector;
    TArray<FString> PhaseEvents;
    void UpdateCamera(double Dt);
    void WriteResult(bool bSuccess, const FString& Reason);
    void InitializeFlightCsv();
    void RecordFlightCsvSample();
};
