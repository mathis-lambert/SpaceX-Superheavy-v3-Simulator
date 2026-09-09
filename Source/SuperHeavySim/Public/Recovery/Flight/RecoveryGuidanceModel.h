#pragma once
#include "Recovery/Flight/RecoveryDynamicsModel.h"
#include "Recovery/Flight/RecoveryTerminalGuidance.h"
#include "Recovery/Flight/RecoveryLandingPrediction.h"

/** Authored mission and site values copied before simulation begins. */
struct FRecoveryGuidanceConfiguration : FRecoveryDynamicsConfiguration
{
    FVector CaptureWorldM=FVector::ZeroVector,LaunchWorldM=FVector::ZeroVector,TowerWorldM=FVector::ZeroVector;
    FQuat TowerRotation=FQuat::Identity;
    FVector CatchLugPlusM=FVector::ZeroVector,CatchLugMinusM=FVector::ZeroVector;
    double TowerHeightM=0,CaptureHeadingDeg=0;
    double ApogeeM=0,AscentDurationS=0,AscentPitchDeg=0,AscentMaxAccelerationMps2=0;
    double LandingReserveKg=0,BoostbackReserveKg=0,LandingDriftCorrectionS=0;
    double FrontReturnOffsetM=1100;
    double LandingWindLeadS=8;
    double ReturnWindReferenceAltitudeM=0,LandingBurnMarginM=0,LandingDecelerationMps2=0;
    double MaxEntryAngleDeg=0,MaxTiltDeg=0,TimeoutSeconds=0;
    double ContactDescentSpeedMps=.6;
};

enum class ERecoveryGuidanceReason : uint8
{
    None, Separation, Boostback, Coast, ReserveDepleted, Entry, LandingBurn,
    Capture, Captured, PropellantExhausted, OperatorAbort, EnvelopeExceeded,
    SupportLost, SupportEvaluated, ApproachEnvelopeExceeded
};

// Resolve labels only when the game thread consumes an event or exports a result.
const TCHAR* RecoveryGuidanceReasonText(ERecoveryGuidanceReason Reason);

struct FRecoveryGuidanceEvent
{
    ERecoveryPhase Phase=ERecoveryPhase::Ready;
    ERecoveryGuidanceReason Reason=ERecoveryGuidanceReason::None;
    double TimeS=0,SampleTimeS=0,AltitudeM=0,MassKg=0;
};

struct FRecoveryNavigationState
{
    FVector BasePositionM=FVector::ZeroVector,VelocityMps=FVector::ZeroVector;
    double AltitudeM=0,VerticalSpeedMps=0,HorizontalErrorM=0,TiltDeg=0;
    double DensityKgM3=0,PressurePa=0,Gravity=0,DynamicPressurePa=0,Mach=0;
    double EngineIspS=0,HeadingErrorDeg=0,CatchLugErrorM=0,BrakingDistanceM=0,MassKg=0;
};

namespace RecoveryNavigation
{
    FRecoveryNavigationState Evaluate(const FRecoveryGuidanceConfiguration& Config,const FRecoveryBodyKinematics& Body,
        double FuelKg,double RcsFuelKg,bool Separated,const FRecoveryFlightExperiment& Experiment);
}

struct FRecoveryGuidanceState
{
    ERecoveryPhase Phase=ERecoveryPhase::Ready;
    FRecoveryDynamicsCommand Command;
    FRecoveryNavigationState Navigation;
    FRecoveryTerminalPlan TerminalPlan;
    FRecoveryTerminalPlan TerminalCandidate;
    FRecoveryTerminalInput TerminalInput;
    FRecoveryLandingPrediction LandingPrediction;
    FVector TerminalReferenceM=FVector::ZeroVector,TerminalReferenceVelocityMps=FVector::ZeroVector;
    double TerminalPlannedSeconds=0,TerminalBrakingSeconds=0,TerminalPeakTrackingErrorM=0;
    uint64 TerminalReplans=0,TerminalRejectedPlans=0;
    uint64 FrontApproachSamples=0;
    double MinimumMastFrontMarginM=TNumericLimits<double>::Max();
    double MinimumFrontCorridorMarginM=TNumericLimits<double>::Max();
    bool bFrontIngressVerified=false;
    TArray<FRecoveryGuidanceEvent,TInlineAllocator<16>> Events;
    double MissionTimeS=0,SampleTimeS=0,PhaseTimeS=0,ArmClosure=0;
    double ElapsedS=0,MinimumStepS=TNumericLimits<double>::Max(),MaximumStepS=0;
    double TimeToImpactS=0,PredictedMissM=0;
    FVector PredictedImpactM=FVector::ZeroVector,TargetPositionM=FVector::ZeroVector;
    double BoostbackIgnitionAltitudeM=0,BoostbackDownrangeM=0,BoostbackSeconds=0;
    double LandingIgnitionAltitudeM=0,LandingBurnSeconds=0,UnpoweredSeconds=0;
    double LandingIgnitionTimeS=0,LandingIgnitionSpeedMps=0,LandingIgnitionMassKg=0;
    double LandingIgnitionDistanceM=0,LandingIgnitionFuelKg=0,LandingIgnitionThrustN=0;
    double FirstContactTimeS=-1,FirstContactSpeedMps=0,FirstContactVerticalSpeedMps=0;
    double FirstContactTiltDeg=0,LowSlowApproachSeconds=0;
    double SettledContactSeconds=0,CaptureErrorAtLatch=0,CaptureSpeedAtLatch=0,CaptureTiltAtLatch=0;
    double CaptureHeadingAtLatch=0,CaptureLugAtLatch=0;
    FVector LatchPositionM=FVector::ZeroVector;
    bool bFlightStarted=false,bSeparationRequested=false,bContactShutdown=false;
    bool bUnpoweredViolation=false,bResultReady=false,bSuccess=false;
    ERecoveryGuidanceReason ResultReason=ERecoveryGuidanceReason::None;
    uint64 Steps=0;
};

/** Flight decisions use physical time and live solver kinematics, never artwork. */
class FRecoveryGuidanceModel
{
public:
    void Reset(const FRecoveryGuidanceConfiguration& Configuration);
    void Step(const FRecoveryBodyKinematics& Body,const FRecoveryDynamicsState& Dynamics,
        const FRecoveryDynamicsCommand& External,double Dt);
    const FRecoveryGuidanceState& GetState() const { return State; }
private:
    FRecoveryGuidanceConfiguration Config;
    FRecoveryGuidanceState State;
    double PredictorClock=0;
    double TerminalClock=0;
    double LandingPredictionClock=0;
    int32 LandingEngineGroup=13;
    FRecoveryBodyKinematics Body;
    FRecoveryFlightExperiment Experiment;
    void Navigate(const FRecoveryDynamicsState& Dynamics,bool Separated);
    void AuditFrontApproach();
    void PredictBallistic();
    FVector WindAt(double Height) const;
    void Transition(ERecoveryPhase Phase,ERecoveryGuidanceReason Reason);
    void Fail(ERecoveryGuidanceReason Reason);
    void Guide(const FRecoveryDynamicsState& Dynamics,const FRecoveryDynamicsCommand& External,double Dt);
    void GuideLanding(const FRecoveryDynamicsState& Dynamics,const FRecoveryDynamicsCommand& External,double Dt,
        FVector& ThrustAcceleration,FVector& TargetUp);
};
