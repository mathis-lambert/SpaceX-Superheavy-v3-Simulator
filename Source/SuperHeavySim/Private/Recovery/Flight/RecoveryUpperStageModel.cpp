#include "Recovery/Flight/RecoveryUpperStageModel.h"
#include "Recovery/Flight/RecoveryAtmosphere.h"
#include "Recovery/Shared/FlightGeometry.h"

void FRecoveryUpperStageModel::Reset(const FRecoveryUpperStageConfiguration& Configuration)
{
    Config=Configuration;State=FRecoveryUpperStageState();
    State.PropellantKg=FMath::Max(0.,Config.InitialFuelKg);
    State.Mass=RecoveryMass::UpperStage(Config.DryMassKg+State.PropellantKg);
}

void FRecoveryUpperStageModel::Step(const FRecoveryBodyKinematics& Body,double WindScale,double Dt)
{
    if(Dt<=0 || !FMath::IsFinite(Dt))return;
    State.Body=Body;State.Forces.Reset();++State.Steps;State.ElapsedS+=Dt;
    State.MinimumStepS=FMath::Min(State.MinimumStepS,Dt);State.MaximumStepS=FMath::Max(State.MaximumStepS,Dt);
    const double Rated=6*Config.EngineThrustN;
    const auto Valve=RecoveryPropulsion::AdvanceValve(State.ThrustN,State.PropellantKg>0?Rated:0.,Rated,.4,.35,Dt);
    const double ExhaustSpeed=FMath::Max(0.,Config.SpecificImpulseS)*RecoveryAtmosphere::G0;
    const double Budget=State.PropellantKg*ExhaustSpeed;
    const double Impulse=FMath::Min(Valve.ImpulseNs,Budget);
    const double Used=ExhaustSpeed>0?Impulse/ExhaustSpeed:0.;
    State.ThrustN=Valve.ImpulseNs>=Budget?0.:Valve.EndThrustN;
    State.PropellantKg=FMath::Max(0.,State.PropellantKg-Used);State.FuelConsumedKg+=Used;
    State.DeliveredImpulseNs+=Impulse;
    State.Mass=RecoveryMass::UpperStage(Config.DryMassKg+State.PropellantKg);

    const double Height=FlightGeometry::AltitudeM(Body.OriginM*100.);
    const auto Air=RecoveryAtmosphere::Sample(Height,Config.TemperatureOffsetK);
    const FVector Relative=Body.VelocityMps-RecoveryAtmosphere::WindAt(Config.SurfaceWindMps,Height,WindScale);
    const FVector Local=Body.Rotation.UnrotateVector(Relative);
    const double Speed=FMath::Max(1.,Relative.Size()),Q=.5*Air.Density*Relative.SizeSquared();
    const FVector Drag=-Q*64*.6*Relative/Speed;
    const FVector Side=Body.Rotation.RotateVector(-Q*450*FVector(Local.X,Local.Y,0)/Speed);
    State.Forces.Add({ERecoveryForceKind::Aerodynamic,0,Body.OriginM*100.,Drag+Side});
    const FVector Down=(FlightGeometry::EarthCenterCm()/100.-Body.OriginM).GetSafeNormal();
    State.Forces.Add({ERecoveryForceKind::Gravity,0,Body.OriginM*100.,Down*(Air.Gravity*State.Mass.MassKg)});
    const FVector Base=Body.OriginM-Body.Rotation.GetUpVector()*FlightGeometry::UpperStageCentreFromBaseM;
    const auto& Nozzles=FlightGeometry::UpperStageNozzlePositionsM();
    for(int32 I=0;I<Nozzles.Num();++I)
        State.Forces.Add({ERecoveryForceKind::Engine,I,(Base+Body.Rotation.RotateVector(Nozzles[I]))*100.,
            Body.Rotation.GetUpVector()*(Impulse/(6*Dt))});
}
