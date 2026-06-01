#include "Navigation/SuperHeavyNavigationComponent.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

USuperHeavyNavigationComponent::USuperHeavyNavigationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USuperHeavyNavigationComponent::ResetNavigation()
{
	PreviousVelocityWorldMps = FVector::ZeroVector;
	PreviousTimeSeconds = 0.0;
	bHasPreviousState = false;
}

FSuperHeavyNavigationState USuperHeavyNavigationComponent::CaptureNavigationState(const FTransform& LandingTargetWorldTransform, const FVector& LandingTargetVelocityMps)
{
	if (!PhysicsComponent)
	{
		ResolvePhysicsComponent();
	}

	FSuperHeavyNavigationState State;
	State.TimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

	UPrimitiveComponent* Body = PhysicsComponent.Get();
	if (!Body)
	{
		LastState = State;
		return LastState;
	}

	State.LocationWorldM = Body->GetComponentLocation() / 100.0;
	State.VelocityWorldMps = Body->GetPhysicsLinearVelocity() / 100.0;
	State.RotationWorldQuat = Body->GetComponentQuat();
	State.RotationWorldDeg = State.RotationWorldQuat.Rotator();
	State.BodyForwardWorld = Body->GetForwardVector();
	State.BodyRightWorld = Body->GetRightVector();
	State.BodyUpWorld = Body->GetUpVector();
	State.AngularVelocityWorldDegPerSec = Body->GetPhysicsAngularVelocityInDegrees();
	State.AngularVelocityBodyDegPerSec = State.RotationWorldQuat.Inverse().RotateVector(State.AngularVelocityWorldDegPerSec);
	State.AltitudeM = (Body->GetComponentLocation().Z - AltitudeReferenceWorldZCm) / 100.0;
	State.MassKg = Body->GetMass();

	const double DeltaTime = State.TimeSeconds - PreviousTimeSeconds;
	if (bHasPreviousState && DeltaTime > UE_SMALL_NUMBER)
	{
		State.AccelerationWorldMps2 = (State.VelocityWorldMps - PreviousVelocityWorldMps) / DeltaTime;
	}

	const FVector LandingTargetWorldM = LandingTargetWorldTransform.GetLocation() / 100.0;
	State.PositionErrorToLandingTargetM = LandingTargetWorldM - State.LocationWorldM;
	State.VelocityRelativeToLandingTargetMps = LandingTargetVelocityMps - State.VelocityWorldMps;
	State.HorizontalDistanceToLandingTargetM = FVector2D(State.PositionErrorToLandingTargetM.X, State.PositionErrorToLandingTargetM.Y).Length();
	State.DistanceToLandingTargetM = State.PositionErrorToLandingTargetM.Length();

	PreviousVelocityWorldMps = State.VelocityWorldMps;
	PreviousTimeSeconds = State.TimeSeconds;
	bHasPreviousState = true;

	LastState = State;
	return LastState;
}

void USuperHeavyNavigationComponent::ResolvePhysicsComponent()
{
	PhysicsComponent = nullptr;

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	Owner->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
	for (UPrimitiveComponent* Component : PrimitiveComponents)
	{
		if (Component && Component->GetFName() == PhysicsComponentName)
		{
			PhysicsComponent = Component;
			return;
		}
	}

	PhysicsComponent = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
}
