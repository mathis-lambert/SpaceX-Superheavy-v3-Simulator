#include "Recovery/Flight/RecoveryLandingPrediction.h"
#include "Recovery/Flight/RecoveryAtmosphere.h"

double RecoveryLanding::MinimumTransferTime(double Distance,double Velocity,double Acceleration)
{
    const double A=FMath::Max(.01,Acceleration),D=FMath::Abs(Distance);
    const double Closing=Distance<0?-Velocity:Velocity;
    if(Closing>0 && Closing*Closing>2*A*D)
        return (Closing+2*FMath::Sqrt(.5*Closing*Closing-A*D))/A;
    return (2*FMath::Sqrt(A*D+.5*Closing*Closing)-Closing)/A;
}

FRecoveryLandingPrediction RecoveryLanding::Predict(const FRecoveryDynamicsConfiguration& C,
    const FRecoveryDynamicsState& D,double Altitude,double Velocity,double UpProjection,
    double MaximumDeceleration,int32 FailedEngine)
{
    FRecoveryLandingPrediction R;
    if(!FMath::IsFinite(Altitude) || !FMath::IsFinite(Velocity) || !FMath::IsFinite(UpProjection) ||
        D.Mass.MassKg<=0 || D.PropellantKg<=0 || MaximumDeceleration<=0 || UpProjection<=0)return R;
    int32 CoreCount=0,RingCount=0;
    double Core=0,Ring=0;
    for(int32 I=0;I<D.Engines.Num();++I)
    {
        if(!RecoveryActuators::EngineRequested(D.Engines[I],I,13,FailedEngine))continue;
        if(D.Engines[I].bCentral){++CoreCount;Core+=D.Engines[I].ThrustN;}
        else {++RingCount;Ring+=D.Engines[I].ThrustN;}
    }
    if(CoreCount==0)return R;
    // Bound the assumed vertical component. No optimistic instant upright turn.
    const double Projection=FMath::Clamp(UpProjection,0.1,1.)*.95;
    double Height=Altitude,Mass=D.Mass.MassKg;
    bool CoreOnly=Velocity>=-75;
    constexpr double Dt=.04;
    for(double T=0;T<90;T+=Dt)
    {
        const auto Air=RecoveryAtmosphere::Sample(Height,C.SeaLevelTemperatureOffsetK);
        const double Isp=FMath::Lerp(C.SpecificImpulseVacuumS,C.SpecificImpulseSeaLevelS,FMath::Clamp(Air.Pressure/101325.,0.,1.));
        if(Isp<=0 || C.SpecificImpulseSeaLevelS<=0)return R;
        const double Rated=C.EngineThrustN*Isp/C.SpecificImpulseSeaLevelS;
        const double CoreAvailable=CoreCount*Rated,RingAvailable=RingCount*Rated;
        if(T==0){R.CoreThrustN=CoreAvailable;R.LandingThrustN=CoreAvailable+RingAvailable;}
        const double CoreDecel=.65*(.9*CoreAvailable*Projection/Mass-Air.Gravity);
        if(CoreDecel<=0)return R;
        CoreOnly|=Velocity>=-75;
        const double Mach=FMath::Abs(Velocity)/Air.SoundSpeed;
        const double Cd=C.TailFirstDragCoefficient*(1+.2*FMath::Exp(-FMath::Square((Mach-1)/.3)));
        const double CdA=C.DragAreaM2*Cd+3*C.GridFinAreaM2*C.GridFinDragCoefficient;
        const double Drag=-.5*Air.Density*CdA*FMath::Abs(Velocity)*Velocity/Mass;
        const double Net=CoreOnly?FMath::Min(MaximumDeceleration,CoreDecel):MaximumDeceleration;
        const double Available=CoreAvailable+(CoreOnly?0:RingAvailable);
        const double Throttle=FMath::Clamp(Mass*(Net+Air.Gravity-Drag)/(Projection*Available),C.Engines.MinimumThrottle,.9);
        // The same impulse integrator powers the real engines. Core and ring
        // shutdown tails remain separate during the bank transition.
        const auto CoreStep=RecoveryPropulsion::AdvanceValve(Core,Throttle*CoreAvailable,CoreAvailable,
            C.Engines.OpeningTimeConstantS,C.Engines.ShutdownTimeS,Dt);
        const auto RingStep=RecoveryPropulsion::AdvanceValve(Ring,CoreOnly?0:Throttle*RingAvailable,RingAvailable,
            C.Engines.OpeningTimeConstantS,C.Engines.ShutdownTimeS,Dt);
        Core=CoreStep.EndThrustN;Ring=RingStep.EndThrustN;
        const double Impulse=CoreStep.ImpulseNs+RingStep.ImpulseNs;
        const double Fuel=Impulse/(Isp*RecoveryAtmosphere::G0);
        if(R.FuelKg+Fuel>D.PropellantKg*.95)return R;
        const double Acceleration=Impulse*Projection/(Dt*(Mass-Fuel*.5))+Drag-Air.Gravity;
        Height+=Velocity*Dt+.5*Acceleration*Dt*Dt;
        Velocity+=Acceleration*Dt;Mass-=Fuel;R.FuelKg+=Fuel;R.DurationS=T+Dt;
        R.DistanceM=FMath::Max(R.DistanceM,Altitude-Height);
        if(Velocity>=-.35){R.bFeasible=true;return R;}
    }
    return R;
}
