#include "Recovery/Flight/RecoveryDynamicsModel.h"
#include "Recovery/Flight/RecoveryAtmosphere.h"
#include "Recovery/Shared/FlightGeometry.h"

void FRecoveryDynamicsModel::Reset(const FRecoveryDynamicsConfiguration& Configuration,
    const TArray<FRecoveryEngineState>& EngineGeometry,double PropellantKg,double RcsPropellantKg)
{
    Config=Configuration;State=FRecoveryDynamicsState();
    State.Engines=EngineGeometry;
    for(auto& E:State.Engines){E.ThrustN=0;E.StepImpulseNs=0;E.StepForceBodyN=FVector::ZeroVector;E.DirectionBody=FVector::UpVector;}
    State.ReactionForcesBodyN.Init(FVector::ZeroVector,6);
    State.Forces.Reserve(48);
    State.PropellantKg=PropellantKg;State.RcsPropellantKg=RcsPropellantKg;
    State.Mass=RecoveryMass::Booster(Config,PropellantKg,RcsPropellantKg,true);
}

FVector FRecoveryDynamicsModel::WindAt(double Height,const FRecoveryDynamicsCommand& C) const
{
    return Config.WindVelocityMps*C.Experiment.WindScale*(.4+.6*FMath::Clamp(Height/100.,0.,1.))*
        FMath::Exp(-FMath::Max(0.,Height-10000.)/18000.);
}

void FRecoveryDynamicsModel::AddForce(ERecoveryForceKind Kind,int32 Index,const FVector& ForceN,const FVector& PointM)
{
    State.Forces.Add({Kind,Index,PointM*100.,ForceN});
}

FVector FRecoveryDynamicsModel::AttitudeMoment(const FRecoveryDynamicsCommand& C,double Gain,double Damping) const
{
    const FQuat Q=State.Body.Rotation;
    const FQuat Target=FRotationMatrix::MakeFromZX(C.TargetUpWorld,C.HeadingWorld).ToQuat();
    FQuat Error=Target*Q.Inverse();Error.Normalize();if(Error.W<0)Error=Error*-1.;
    FVector Axis;double Angle;Error.ToAxisAndAngle(Axis,Angle);
    const FVector Inertia=State.Mass.InertiaKgM2;
    const double EffectiveInertia=FMath::Max(1.,(Inertia*Q.UnrotateVector(Axis)).Size());
    const double Authority=State.ThrustN*FMath::Tan(FMath::DegreesToRadians(Config.Engines.MaximumGimbalDeg))*31+
        (State.RcsPropellantKg>0?Config.ReactionControlTorqueNm:0)+
        State.DynamicPressurePa*Config.GridFinAreaM2*Config.GridFinLiftSlope*.4*28.8;
    const double BrakingRate=.65*FMath::Sqrt(2*FMath::Max(.00001,Authority/EffectiveInertia)*Angle);
    const double Rate=FMath::Min3(.20,BrakingRate,Angle*Gain/Damping);
    const FVector Alpha=(Axis*Rate-State.Body.AngularVelocityWorldRadS)*(Damping*C.Experiment.AttitudeResponse);
    const FVector Omega=Q.UnrotateVector(State.Body.AngularVelocityWorldRadS);
    return Inertia*Q.UnrotateVector(Alpha)+FVector::CrossProduct(Omega,Inertia*Omega);
}

void FRecoveryDynamicsModel::Condition(const FRecoveryDynamicsCommand& C,double Dt)
{
    State.GroundClockS+=Dt;
    State.DelugeFlow+=FMath::Clamp(C.DelugeDemand-State.DelugeFlow,-Dt/3.,Dt/1.5);
    for(int32 I=0;I<2;++I)
    {
        const double Maximum=Config.ConditioningVentKgS*(I==0?2./3.:1./3.);
        const double Duty=.55+.45*FMath::Square(FMath::Sin(State.GroundClockS*.32+I*1.8));
        const double Target=C.bGroundSupplyConnected?Maximum*Duty:0.;
        State.ConditioningFlowKgS[I]+=FMath::Clamp(Target-State.ConditioningFlowKgS[I],-Maximum*Dt/.35,Maximum*Dt/.6);
    }
    const double Requested=(State.ConditioningFlowKgS.X+State.ConditioningFlowKgS.Y)*Dt;
    const double Used=FMath::Min(State.PropellantKg,Requested);
    State.ConditioningFlowKgS*=Requested>0?Used/Requested:0;
    State.PropellantKg-=Used;State.ConditioningVentedKg+=Used;
    if(C.bGroundSupplyConnected){State.PropellantKg+=Used;State.GroundSupplyKg+=Used;}
    const auto& Positions=FlightGeometry::ConditioningVentPositionsM();
    const auto& Directions=FlightGeometry::ConditioningVentDirections();
    for(int32 I=0;I<2;++I)
        AddForce(ERecoveryForceKind::Vent,I,State.Body.Rotation.RotateVector(-Directions[I]*State.ConditioningFlowKgS[I]*Config.ConditioningJetSpeedMps),
            BaseM+State.Body.Rotation.RotateVector(Positions[I]));
}

void FRecoveryDynamicsModel::ReactionControl(const FRecoveryDynamicsCommand& C,const FVector& RequestedMoment,double Dt)
{
    const bool Enabled=!C.Experiment.bReactionJetsDisabled && C.EngineCount==0 && !C.bContactShutdown &&
        C.Phase>=ERecoveryPhase::Ascent && C.Phase<ERecoveryPhase::Captured;
    const double Blend=1-FMath::Clamp((State.DynamicPressurePa-100)/1000.,0.,1.);
    const FVector T=Enabled?RequestedMoment.GetClampedToMaxSize(Config.ReactionControlTorqueNm*Blend):FVector::ZeroVector;
    const FVector Demand[]={FVector(0,-T.X/50.,0),FVector(0,T.X/50.,0),FVector(T.Y/50.,0,0),
        FVector(-T.Y/50.,0,0),FVector(-T.Z/9.1,0,0),FVector(T.Z/9.1,0,0)};
    const double Response=1-FMath::Exp(-Dt/FMath::Max(.001,Config.ReactionValveTimeConstantS));
    double Total=0;
    for(int32 I=0;I<6;++I)
    {
        auto& Force=State.ReactionForcesBodyN[I];
        const FVector Target=FVector::DotProduct(Force,Demand[I])<0?FVector::ZeroVector:Demand[I];
        Force=FMath::Lerp(Force,Target,Response);if(Force.Size()<1.)Force=FVector::ZeroVector;
        Total+=Force.Size();
    }
    const double Delivered=RecoveryActuators::FuelLimitedThrust(Total,State.RcsPropellantKg,150.,Dt);
    const double Scale=Total>0?Delivered/Total:0.;
    State.RcsPropellantKg=FMath::Max(0.,State.RcsPropellantKg-Delivered*Dt/(150.*RecoveryAtmosphere::G0));
    State.RcsMomentBodyNm=FVector::ZeroVector;
    const auto& Positions=FlightGeometry::ReactionNozzlePositionsM();
    for(int32 I=0;I<6;++I)
    {
        State.ReactionForcesBodyN[I]*=Scale;
        AddForce(ERecoveryForceKind::ReactionJet,I,State.Body.Rotation.RotateVector(State.ReactionForcesBodyN[I]),
            BaseM+State.Body.Rotation.RotateVector(Positions[I]));
        State.RcsMomentBodyNm+=FVector::CrossProduct(Positions[I]-FVector(0,0,State.Mass.CentreFromBaseM),State.ReactionForcesBodyN[I]);
    }
}

void FRecoveryDynamicsModel::Aerodynamics(const FRecoveryDynamicsCommand& C,double Height,double Mach,double Dt)
{
    const FQuat Q=State.Body.Rotation;
    const FVector Relative=State.Body.VelocityMps-WindAt(Height,C);
    const FVector Local=Q.UnrotateVector(Relative);
    const double Speed=FMath::Max(1.,Local.Size());
    const double BaseCd=Local.Z<0?Config.TailFirstDragCoefficient:Config.AxialDragCoefficient;
    const double Cd=BaseCd*(1+.2*FMath::Exp(-FMath::Square((Mach-1)/.3)));
    const double FinCdA=3*Config.GridFinAreaM2*Config.GridFinDragCoefficient*FMath::Square(Local.Z/Speed);
    const FVector Drag=-State.DynamicPressurePa*(Config.DragAreaM2*Cd+FinCdA)*Relative/Speed;
    const FVector Side=-State.DynamicPressurePa*Config.BodySideAreaM2*Config.BodyNormalCoefficient*FVector(Local.X,Local.Y,0)/Speed;
    State.AeroForceN=Drag+Q.RotateVector(Side);
    const FVector COM=BaseM+Q.GetUpVector()*State.Mass.CentreFromBaseM;
    AddForce(ERecoveryForceKind::Aerodynamic,0,State.AeroForceN,COM);
    FVector Torque=AttitudeMoment(C,.45,1.35);if(C.EngineCount>0)Torque*=.1;
    const double Lever=64.4486-State.Mass.CentreFromBaseM,Radius=6.2;
    const double F3=-Torque.Y/Lever,Sum=Torque.Z/Radius-F3;
    const FVector Demand((Sum-Torque.X/Lever)/2,(Sum+Torque.X/Lever)/2,F3);
    const double PerRad=State.DynamicPressurePa*Config.GridFinAreaM2*Config.GridFinLiftSlope;
    for(int32 I=0;I<3;++I)
    {
        const double Desired=C.bContactShutdown || C.Phase>=ERecoveryPhase::Captured || State.DynamicPressurePa<100 || C.Phase<=ERecoveryPhase::Ascent ?
            0.:FMath::Clamp(FMath::RadiansToDegrees(Demand[I]/FMath::Max(1.,PerRad)),-Config.GridFinMaxAngleDeg,Config.GridFinMaxAngleDeg);
        if(I==C.Experiment.JammedFin)State.GridFinAnglesDeg[I]=C.Experiment.JammedFinAngleDeg;
        else State.GridFinAnglesDeg[I]+=FMath::Clamp(Desired-State.GridFinAnglesDeg[I],-Config.GridFinRateDegS*Dt,Config.GridFinRateDegS*Dt);
    }
    State.GridFinAuthority=FMath::Clamp(State.DynamicPressurePa/1500.,0.,1.);
    if(C.EngineCount==0 && State.DynamicPressurePa>200 && State.GridFinAnglesDeg.Size()>.1)State.FinControlSeconds+=Dt;
    const FVector Forces=State.GridFinAnglesDeg*(PerRad*PI/180.);
    const FVector Positions[]={FVector(Radius,0,Lever),FVector(-Radius,0,Lever),FVector(0,Radius,Lever)};
    const FVector Directions[]={FVector(0,1,0),FVector(0,-1,0),FVector(-1,0,0)};
    for(int32 I=0;I<3;++I)AddForce(ERecoveryForceKind::GridFin,I,Q.RotateVector(Directions[I]*Forces[I]),COM+Q.RotateVector(Positions[I]));
    ReactionControl(C,Torque,Dt);
}

void FRecoveryDynamicsModel::Propulsion(const FRecoveryDynamicsCommand& C,double Dt)
{
    FRecoveryEngineCommand Request;
    Request.RequestedCount=C.EngineCount;Request.FailedEngine=C.Experiment.FailedEngine;
    Request.RatedThrustN=Config.EngineThrustN*State.EngineIspS/Config.SpecificImpulseSeaLevelS;
    Request.SpecificImpulseS=State.EngineIspS;
    Request.RequestedThrustN=FMath::Max(0.,FVector::DotProduct(C.ThrustAccelerationMps2,State.Body.Rotation.GetUpVector())*State.Mass.MassKg);
    auto Step=RecoveryPropulsion::AdvanceEngines(State.Engines,Config.Engines,Request,State.PropellantKg,Dt);
    State.ThrustN=Step.DeliveredImpulseNs/Dt;
    State.PropellantKg=FMath::Max(0.,State.PropellantKg-Step.FuelUsedKg);State.MainFuelConsumedKg+=Step.FuelUsedKg;
    State.Mass=RecoveryMass::Booster(Config,State.PropellantKg,State.RcsPropellantKg,!C.bSeparated);
    State.Throttle=Step.AvailableThrustN>0?FMath::Clamp(State.ThrustN/Step.AvailableThrustN,0.,1.):0.;
    const FVector Moment=C.Phase<=ERecoveryPhase::Countdown || C.Phase>=ERecoveryPhase::Captured || C.bContactShutdown ? FVector::ZeroVector :
        AttitudeMoment(C,C.Phase>=ERecoveryPhase::LandingBurn?2.5:.65,C.Phase>=ERecoveryPhase::LandingBurn?3.2:1.6);
    RecoveryPropulsion::AllocateGimbals(State.Engines,Config.Engines,FVector(0,0,State.Mass.CentreFromBaseM),Moment,Dt,Step);
    for(int32 I=0;I<State.Engines.Num();++I)
    {
        const auto& Engine=State.Engines[I];
        AddForce(ERecoveryForceKind::Engine,I,State.Body.Rotation.RotateVector(Engine.StepForceBodyN),
            BaseM+State.Body.Rotation.RotateVector(Engine.PositionFromBaseM));
        const double MeanThrust=Engine.StepImpulseNs/Dt;
        State.PeakEngineForceRatio=FMath::Max(State.PeakEngineForceRatio,MeanThrust>1.?Engine.StepForceBodyN.Size()/MeanThrust:0.);
        State.PeakGimbalDeg=FMath::Max(State.PeakGimbalDeg,FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Engine.DirectionBody.Z,-1.,1.))));
    }
    State.EngineForceBodyN=Step.ForceBodyN;State.EngineMomentBodyNm=Step.MomentBodyNm;
}

void FRecoveryDynamicsModel::Step(const FRecoveryBodyKinematics& Body,const FRecoveryDynamicsCommand& C,double Dt)
{
    if(Dt<=0 || !FMath::IsFinite(Dt))return;
    State.Body=Body;State.Forces.Reset();State.ElapsedS+=Dt;++State.Steps;
    BaseM=Body.OriginM-Body.Rotation.GetUpVector()*FlightGeometry::BoosterBaseOffsetM;
    const double Height=FlightGeometry::AltitudeM(BaseM*100.);
    const auto Air=RecoveryAtmosphere::Sample(Height,Config.SeaLevelTemperatureOffsetK);
    State.GravityMps2=Air.Gravity;
    State.EngineIspS=FMath::Lerp(Config.SpecificImpulseVacuumS,Config.SpecificImpulseSeaLevelS,FMath::Clamp(Air.Pressure/101325.,0.,1.));
    const FVector Relative=Body.VelocityMps-WindAt(Height,C);
    State.DynamicPressurePa=.5*Air.Density*Relative.SizeSquared();
    Condition(C,Dt);
    State.Mass=RecoveryMass::Booster(Config,State.PropellantKg,State.RcsPropellantKg,!C.bSeparated);
    Aerodynamics(C,Height,Relative.Size()/Air.SoundSpeed,Dt);
    Propulsion(C,Dt);
    const FVector COM=BaseM+Body.Rotation.GetUpVector()*State.Mass.CentreFromBaseM;
    const FVector Down=(FlightGeometry::EarthCenterCm()/100.-COM).GetSafeNormal();
    AddForce(ERecoveryForceKind::Gravity,0,Down*(Air.Gravity*State.Mass.MassKg),COM);
}
