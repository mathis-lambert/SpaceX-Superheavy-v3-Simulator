#pragma once
#include "CoreMinimal.h"
#include "Recovery/Flight/RecoveryFlightPhase.h"
#include "Recovery/Flight/RecoveryFlightInspection.h"
#include "Recovery/Flight/RecoveryMassProperties.h"
#include "Recovery/Flight/RecoveryPropulsionModel.h"

/** Immutable numeric configuration copied from the authored mission profile. */
struct FRecoveryDynamicsConfiguration
{
    FRecoveryEngineParameters Engines;
    double DryMassKg=0,UpperStageMassKg=0,MixtureRatio=0,OxygenDensityKgM3=0,MethaneDensityKgM3=0;
    double OxygenTankBottomM=0,MethaneTankBottomM=0;
    double EngineThrustN=0,SpecificImpulseSeaLevelS=0,SpecificImpulseVacuumS=0;
    double ReactionControlTorqueNm=0,ReactionValveTimeConstantS=0;
    double DragAreaM2=0,AxialDragCoefficient=0,TailFirstDragCoefficient=0;
    double BodySideAreaM2=0,BodyNormalCoefficient=0,GridFinDragCoefficient=0;
    double GridFinAreaM2=0,GridFinLiftSlope=0,GridFinMaxAngleDeg=0,GridFinRateDegS=0;
    double SeaLevelTemperatureOffsetK=0,ConditioningVentKgS=0,ConditioningJetSpeedMps=0;
    FVector WindVelocityMps=FVector::ZeroVector;
};

/** Only the solver adapter knows about Chaos handles and centimetre force units. */
struct FRecoveryBodyKinematics
{
    FVector OriginM=FVector::ZeroVector;
    FQuat Rotation=FQuat::Identity;
    FVector VelocityMps=FVector::ZeroVector;
    FVector AngularVelocityWorldRadS=FVector::ZeroVector;
};

/** Held outer-loop requests. Inner attitude and actuator feedback use live solver state. */
struct FRecoveryDynamicsCommand
{
    FVector ThrustAccelerationMps2=FVector::ZeroVector;
    FVector TargetUpWorld=FVector::UpVector;
    FVector HeadingWorld=FVector::ForwardVector;
    ERecoveryPhase Phase=ERecoveryPhase::Ready;
    int32 EngineCount=0;
    bool bSeparated=false,bContactShutdown=false,bGroundSupplyConnected=true;
    double DelugeDemand=0;
    FRecoveryFlightExperiment Experiment;
};

/** The physics model owns this state. Consumers receive a copy, never a mutable reference. */
struct FRecoveryDynamicsState
{
    TArray<FRecoveryEngineState> Engines;
    TArray<FVector> ReactionForcesBodyN;
    TArray<FRecoveryForceSample> Forces;
    FRecoveryBodyKinematics Body;
    RecoveryMass::FProperties Mass;
    FVector GridFinAnglesDeg=FVector::ZeroVector,AeroForceN=FVector::ZeroVector;
    FVector EngineForceBodyN=FVector::ZeroVector,EngineMomentBodyNm=FVector::ZeroVector,RcsMomentBodyNm=FVector::ZeroVector;
    FVector2D ConditioningFlowKgS=FVector2D::ZeroVector;
    double PropellantKg=0,RcsPropellantKg=0,MainFuelConsumedKg=0,ConditioningVentedKg=0,GroundSupplyKg=0;
    double ThrustN=0,Throttle=0,GridFinAuthority=0,FinControlSeconds=0;
    double EngineIspS=0,DynamicPressurePa=0,GravityMps2=0;
    double DelugeFlow=0,GroundClockS=0,ElapsedS=0;
    double PeakEngineForceRatio=0,PeakGimbalDeg=0;
    uint64 Steps=0;
};

/** Value-only inner flight model. It calculates forces but never integrates or sets a pose. */
class FRecoveryDynamicsModel
{
public:
    void Reset(const FRecoveryDynamicsConfiguration& Configuration,const TArray<FRecoveryEngineState>& EngineGeometry,
        double PropellantKg,double RcsPropellantKg);
    void Step(const FRecoveryBodyKinematics& Body,const FRecoveryDynamicsCommand& Command,double Dt);
    const FRecoveryDynamicsState& GetState() const { return State; }
private:
    FRecoveryDynamicsConfiguration Config;
    FRecoveryDynamicsState State;
    FVector BaseM=FVector::ZeroVector;
    FVector WindAt(double Height,const FRecoveryDynamicsCommand& Command) const;
    FVector AttitudeMoment(const FRecoveryDynamicsCommand& Command,double Gain,double Damping) const;
    void AddForce(ERecoveryForceKind Kind,int32 Index,const FVector& ForceN,const FVector& PointM);
    void Condition(const FRecoveryDynamicsCommand& Command,double Dt);
    void Aerodynamics(const FRecoveryDynamicsCommand& Command,double Height,double Mach,double Dt);
    void ReactionControl(const FRecoveryDynamicsCommand& Command,const FVector& RequestedMoment,double Dt);
    void Propulsion(const FRecoveryDynamicsCommand& Command,double Dt);
};
