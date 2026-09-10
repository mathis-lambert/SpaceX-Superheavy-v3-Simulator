#include "Recovery/Flight/RecoveryPropulsionModel.h"
#include "Recovery/Flight/RecoveryAtmosphere.h"
#include <cmath>

RecoveryPropulsion::FValveStep RecoveryPropulsion::AdvanceValve(double StartN,double TargetN,double RatedN,
    double OpeningTimeConstantS,double ShutdownTimeS,double Dt)
{
    if(Dt<=0 || !FMath::IsFinite(Dt))return {StartN,0};
    StartN=FMath::Max(0.,StartN);
    TargetN=FMath::Clamp(TargetN,0.,FMath::Max(0.,RatedN));
    if(TargetN>0)
    {
        const double Tau=FMath::Max(.001,OpeningTimeConstantS);
        // expm1 retains accuracy when a physics step is small relative to Tau.
        const double Opening=-std::expm1(-Dt/Tau);
        const double End=StartN+(TargetN-StartN)*Opening;
        const double Impulse=TargetN*Dt+(StartN-TargetN)*Tau*Opening;
        return {End,FMath::Max(0.,Impulse)};
    }
    const double Rate=FMath::Max(0.,RatedN)/FMath::Max(.001,ShutdownTimeS);
    if(Rate<=0)return {0,0};
    const double BurningTime=FMath::Min(Dt,StartN/Rate);
    return {FMath::Max(0.,StartN-Rate*Dt),FMath::Max(0.,StartN*BurningTime-.5*Rate*BurningTime*BurningTime)};
}

FRecoveryPropulsionStep RecoveryPropulsion::AdvanceEngines(TArray<FRecoveryEngineState>& Engines,
    const FRecoveryEngineParameters& P,const FRecoveryEngineCommand& C,double AvailableFuelKg,double Dt)
{
    FRecoveryPropulsionStep Step;
    const auto Requested=[&](int32 I)
    {
        const auto& E=Engines[I];
        return RecoveryActuators::EngineRequested(E,I,C.RequestedCount,C.FailedEngine);
    };
    for(int32 I=0;I<Engines.Num();++I)if(Requested(I))Step.AvailableThrustN+=C.RatedThrustN;
    const double Throttle=Step.AvailableThrustN>0 && AvailableFuelKg>0 && C.SpecificImpulseS>0 ?
        FMath::Clamp(C.RequestedThrustN/Step.AvailableThrustN,P.MinimumThrottle,1.) : 0.;
    for(int32 I=0;I<Engines.Num();++I)
    {
        auto& Engine=Engines[I];
        const auto Valve=AdvanceValve(Engine.ThrustN,Requested(I)?Throttle*C.RatedThrustN:0.,
            C.RatedThrustN,P.OpeningTimeConstantS,P.ShutdownTimeS,Dt);
        Engine.ThrustN=Valve.EndThrustN;
        Engine.StepImpulseNs=Valve.ImpulseNs;
        Engine.StepForceBodyN=FVector::ZeroVector;
        Step.DeliveredImpulseNs+=Valve.ImpulseNs;
    }
    const double ExhaustSpeed=FMath::Max(0.,C.SpecificImpulseS)*RecoveryAtmosphere::G0;
    const double Budget=FMath::Max(0.,AvailableFuelKg)*ExhaustSpeed;
    const double Scale=Step.DeliveredImpulseNs>0?FMath::Min(1.,Budget/Step.DeliveredImpulseNs):0.;
    const bool Empty=Step.DeliveredImpulseNs>=Budget;
    for(auto& Engine:Engines)
    {
        Engine.StepImpulseNs*=Scale;
        if(Empty)Engine.ThrustN=0;
    }
    Step.DeliveredImpulseNs*=Scale;
    Step.FuelUsedKg=ExhaustSpeed>0?Step.DeliveredImpulseNs/ExhaustSpeed:0.;
    return Step;
}

void RecoveryPropulsion::AllocateGimbals(TArray<FRecoveryEngineState>& Engines,const FRecoveryEngineParameters& P,
    const FVector& COM,const FVector& DesiredMomentBodyNm,double Dt,FRecoveryPropulsionStep& Step,
    const FVector& DesiredForceBodyN,double TranslationWeight)
{
    if(Dt<=0)return;
    FVector C0=FVector::ZeroVector,C1=FVector::ZeroVector,C2=FVector::ZeroVector,AxialTorque=FVector::ZeroVector;
    double GimballedThrust=0;
    for(const auto& Engine:Engines)
    {
        const double MeanThrust=Engine.StepImpulseNs/Dt;
        const FVector R=Engine.PositionFromBaseM-COM;
        AxialTorque+=FVector::CrossProduct(R,FVector(0,0,MeanThrust));
        if(!Engine.bGimballed || MeanThrust<1.)continue;
        GimballedThrust+=MeanThrust;
        for(const FVector A:{FVector(0,R.Z,-R.Y),FVector(-R.Z,0,R.X)})
        {C0+=A*A.X;C1+=A*A.Y;C2+=A*A.Z;}
    }
    const double Weight=FMath::Clamp(TranslationWeight,0.,1.);
    const FVector GimbalMoment=DesiredMomentBodyNm*FVector(1-Weight,1-Weight,1);
    const FVector Lambda=RecoveryActuators::SolveSymmetric(C0,C1,C2,GimbalMoment-AxialTorque);
    Step.ForceBodyN=Step.MomentBodyNm=FVector::ZeroVector;
    for(auto& Engine:Engines)
    {
        const double MeanThrust=Engine.StepImpulseNs/Dt;
        const FVector R=Engine.PositionFromBaseM-COM;
        FVector Target=FVector::UpVector;
        if(Engine.bGimballed && MeanThrust>1.)
        {
            FVector Side(FVector::DotProduct(FVector(0,R.Z,-R.Y),Lambda),FVector::DotProduct(FVector(-R.Z,0,R.X),Lambda),0);
            Side+=FVector(DesiredForceBodyN.X,DesiredForceBodyN.Y,0)*(Weight*MeanThrust/FMath::Max(1.,GimballedThrust));
            Side=Side.GetClampedToMaxSize(MeanThrust*FMath::Tan(FMath::DegreesToRadians(P.MaximumGimbalDeg)));
            Target=FVector(Side.X,Side.Y,MeanThrust).GetSafeNormal();
        }
        const double Angle=FMath::Acos(FMath::Clamp(FVector::DotProduct(Engine.DirectionBody,Target),-1.,1.));
        const double Fraction=Angle>1.e-8?FMath::Min(1-FMath::Exp(-Dt/P.GimbalTimeConstantS),
            FMath::DegreesToRadians(P.GimbalRateDegS)*Dt/Angle):1.;
        Engine.DirectionBody=FMath::Lerp(Engine.DirectionBody,Target,Fraction).GetSafeNormal();
        Engine.StepForceBodyN=Engine.DirectionBody*MeanThrust;
        Step.ForceBodyN+=Engine.StepForceBodyN;
        Step.MomentBodyNm+=FVector::CrossProduct(R,Engine.StepForceBodyN);
    }
}
