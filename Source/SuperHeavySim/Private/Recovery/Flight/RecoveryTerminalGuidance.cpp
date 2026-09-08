#include "Recovery/Flight/RecoveryTerminalGuidance.h"
#include "Recovery/Flight/RecoveryAtmosphere.h"
#include "Recovery/Shared/FlightGeometry.h"

FVector FRecoveryTerminalPlan::PositionAt(double T) const
{return PositionM+VelocityMps*T+InitialAccelerationMps2*(.5*T*T)+(CubicMps3+(QuarticMps4+QuinticMps5*T)*T)*(T*T*T);}
FVector FRecoveryTerminalPlan::VelocityAt(double T) const
{return VelocityMps+InitialAccelerationMps2*T+(3*CubicMps3+(4*QuarticMps4+5*QuinticMps5*T)*T)*(T*T);}
FVector FRecoveryTerminalPlan::AccelerationAt(double T) const
{return InitialAccelerationMps2+(6*CubicMps3+(12*QuarticMps4+20*QuinticMps5*T)*T)*T;}

bool RecoveryTerminalGuidance::TryRequiredThrustAcceleration(const FVector& Net,const FVector& Velocity,
    const FVector& Position,double Mass,const FVector& Wind,const FRecoveryDynamicsConfiguration& C,
    const FVector& InitialUp,FVector& ThrustAcceleration)
{
    ThrustAcceleration=FVector::ZeroVector;
    if(Mass<=0 || !FMath::IsFinite(Mass) || InitialUp.Z<=0)return false;
    const auto Air=RecoveryAtmosphere::Sample(FlightGeometry::AltitudeM(Position*100.),C.SeaLevelTemperatureOffsetK);
    const FVector Relative=Velocity-Wind;
    const double Speed=FMath::Max(1.,Relative.Size()),Q=.5*Air.Density*Relative.SizeSquared();
    const double Cd=C.TailFirstDragCoefficient*(1+.2*FMath::Exp(-FMath::Square((Speed/Air.SoundSpeed-1)/.3)));
    const auto Aero=[&](const FVector& Tilt)
    {
        const FVector Up=FVector(Tilt.X,Tilt.Y,1).GetSafeNormal();
        const double Axial=FVector::DotProduct(Relative,Up);
        const double CdA=C.DragAreaM2*Cd+3*C.GridFinAreaM2*C.GridFinDragCoefficient*FMath::Square(Axial/Speed);
        return (-Q*CdA*Relative/Speed-Q*C.BodySideAreaM2*C.BodyNormalCoefficient*(Relative-Up*Axial)/Speed)/Mass;
    };
    const auto Residual=[&](const FVector& Tilt)
    {
        const FVector A=Aero(Tilt);
        const double Z=Net.Z+Air.Gravity-A.Z;
        return FVector(Z*Tilt.X+A.X-Net.X,Z*Tilt.Y+A.Y-Net.Y,0);
    };
    // Continue the nearby aerodynamic equilibrium, rather than switching to
    // a different root when body normal load exceeds thrust authority.
    FVector Tilt=FVector(InitialUp.X,InitialUp.Y,0)/InitialUp.Z;
    // Solve the coupling between body normal load and thrust direction. A
    // non-finite/ill-conditioned candidate will fail the feasibility checks.
    for(int32 Iteration=0;Iteration<8;++Iteration)
    {
        const FVector R=Residual(Tilt);
        if(R.SizeSquared()<1.e-10)break;
        constexpr double E=.0001;
        const FVector X=(Residual(Tilt+FVector(E,0,0))-R)/E;
        const FVector Y=(Residual(Tilt+FVector(0,E,0))-R)/E;
        const double D=X.X*Y.Y-X.Y*Y.X;
        if(!FMath::IsFinite(D) || FMath::Abs(D)<1.e-8)return false;
        const FVector Delta((R.X*Y.Y-R.Y*Y.X)/D,(X.X*R.Y-X.Y*R.X)/D,0);
        Tilt-=Delta.GetClampedToMaxSize(.3);
    }
    if(Tilt.ContainsNaN() || Residual(Tilt).Size()>.05)return false;
    const double Vertical=Net.Z+Air.Gravity-Aero(Tilt).Z;
    if(!FMath::IsFinite(Vertical) || Vertical<=0)return false;
    ThrustAcceleration=FVector(Tilt.X,Tilt.Y,1)*Vertical;
    return !ThrustAcceleration.ContainsNaN();
}

FRecoveryTerminalPlan RecoveryTerminalGuidance::Plan(const FRecoveryTerminalInput& I,const FRecoveryDynamicsConfiguration& C)
{
    FRecoveryTerminalPlan Result;
    if(I.MassKg<=0 || I.FuelKg<=0 || I.CoreThrustN<=0 || I.IspS<=0)return Result;
    constexpr int32 Samples=24;
    const double MinimumThrust=I.CoreThrustN*C.Engines.MinimumThrottle;
    for(double T=1.;T<=45.;T+=.5)
    {
        ++Result.Candidates;
        FRecoveryTerminalPlan Candidate;
        Candidate.PositionM=I.PositionM;Candidate.VelocityMps=I.VelocityMps;Candidate.HorizonS=T;
        Candidate.InitialAccelerationMps2=I.AccelerationMps2;
        const FVector Error=I.TargetM-I.PositionM-I.VelocityMps*T-I.AccelerationMps2*(.5*T*T);
        const FVector DeltaV=I.TargetVelocityMps-I.VelocityMps-I.AccelerationMps2*T;
        const FVector DeltaA=-I.AccelerationMps2;
        Candidate.CubicMps3=Error*(10./(T*T*T))-DeltaV*(4./(T*T))+DeltaA*(.5/T);
        Candidate.QuarticMps4=Error*(-15./(T*T*T*T))+DeltaV*(7./(T*T*T))-DeltaA/(T*T);
        Candidate.QuinticMps5=Error*(6./(T*T*T*T*T))-DeltaV*(3./(T*T*T*T))+DeltaA*(.5/(T*T*T));
        FVector PreviousUp=I.UpWorld;
        double Mass=I.MassKg,PreviousThrust=0;
        bool Feasible=true;
        for(int32 Sample=0;Sample<=Samples;++Sample)
        {
            const double Time=T*Sample/Samples,Dt=T/Samples;
            const FVector P=Candidate.PositionAt(Time),V=Candidate.VelocityAt(Time);
            FVector ThrustAcceleration;
            if(!TryRequiredThrustAcceleration(Candidate.AccelerationAt(Time),V,P,Mass,I.WindMps,C,PreviousUp,ThrustAcceleration))
            {++Result.ThrustRejected;Feasible=false;break;}
            const FVector Force=ThrustAcceleration*Mass;
            const double Thrust=Force.Size();
            const FVector Up=Force.GetSafeNormal();
            const double Tilt=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Up.Z,-1.,1.)));
            const double Rate=FMath::Acos(FMath::Clamp(FVector::DotProduct(Up,PreviousUp),-1.,1.))/Dt;
            const bool CoreRange=Thrust<=I.CoreThrustN*.95;
            const bool LandingRange=Thrust>=I.LandingThrustN*C.Engines.MinimumThrottle && Thrust<=I.LandingThrustN*.95;
            // The initial sample is the existing actuator transient. Enforce
            // the command envelope on future samples, not retroactively on it.
            if(!FMath::IsFinite(Thrust) || Force.Z<=0 || (Sample>0 && (Thrust<MinimumThrust || (!CoreRange && !LandingRange))))
            {++Result.ThrustRejected;Feasible=false;break;}
            if(Sample>0 && (Tilt>I.MaxTiltDeg || Rate>RecoveryActuators::MaximumBodyRateRadS*.85))
            {++Result.AttitudeRejected;Feasible=false;break;}
            const FVector Base=P-Up*I.CentreFromBaseM;
            const FVector Top=Base+Up*70.88;
            const FVector Middle=I.TowerRotation.UnrotateVector((Base+Top)*.5-I.TowerWorldM);
            const FVector Half=I.TowerRotation.UnrotateVector((Top-Base)*.5).GetAbs()+FVector(4.5,4.5,0);
            // Conservative sampled body bounds against the mast, plus the final
            // opening. Actual arm/fitting contact is still resolved by Chaos.
            const double H=Base.Z-I.CaptureBaseHeightM;
            const FQuat Attitude=FRotationMatrix::MakeFromZX(Up,I.HeadingWorld).ToQuat();
            const double Axis=FVector2D(Base+Attitude.RotateVector(I.FittingMidFromBaseM)-I.TargetFittingWorldM).Size();
            const FVector AtRail=Base+Up*((I.TargetFittingWorldM.Z-Base.Z)/FMath::Max(.1,Up.Z));
            const FVector RailLocal=I.TowerRotation.UnrotateVector(AtRail-I.TargetFittingWorldM);
            const bool InsideArmSpan=FMath::Abs(RailLocal.X+6)<17.5 && Base.Z<I.TargetFittingWorldM.Z && Top.Z>I.TargetFittingWorldM.Z;
            if((Base.Z<I.TowerWorldM.Z+I.TowerHeightM+1 && Top.Z>I.TowerWorldM.Z &&
                FMath::Abs(Middle.X)<Half.X+7 && FMath::Abs(Middle.Y)<Half.Y+7) ||
                (InsideArmSpan && FMath::Abs(RailLocal.Y)+4.5/FMath::Max(.1,Up.Z)+.55>10.) ||
                (Sample==Samples && Axis>.5) || H<-.6)
            {++Result.ClearanceRejected;Feasible=false;break;}
            if(Sample>0)
            {
                const double Fuel=.5*(Thrust+PreviousThrust)*Dt/(I.IspS*RecoveryAtmosphere::G0);
                Candidate.EstimatedFuelKg+=Fuel;Mass-=Fuel;
                if(Candidate.EstimatedFuelKg>I.FuelKg*.9){++Result.FuelRejected;Feasible=false;break;}
            }
            Candidate.PeakThrustN=FMath::Max(Candidate.PeakThrustN,Thrust);
            Candidate.PeakTiltDeg=FMath::Max(Candidate.PeakTiltDeg,Tilt);
            PreviousUp=Up;PreviousThrust=Thrust;
        }
        if(Feasible)
        {
            Candidate.bFeasible=true;Candidate.Candidates=Result.Candidates;
            Candidate.ThrustRejected=Result.ThrustRejected;Candidate.AttitudeRejected=Result.AttitudeRejected;
            Candidate.ClearanceRejected=Result.ClearanceRejected;Candidate.FuelRejected=Result.FuelRejected;
            return Candidate;
        }
    }
    return Result;
}
