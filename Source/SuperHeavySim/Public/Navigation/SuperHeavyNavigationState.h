#pragma once

#include "CoreMinimal.h"
#include "SuperHeavyNavigationState.generated.h"

USTRUCT(BlueprintType)
struct SUPERHEAVYSIM_API FSuperHeavyNavigationState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	double TimeSeconds = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	FVector LocationWorldM = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	FVector VelocityWorldMps = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	FVector AccelerationWorldMps2 = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	FRotator RotationWorldDeg = FRotator::ZeroRotator;

	FQuat RotationWorldQuat = FQuat::Identity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	FVector BodyForwardWorld = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	FVector BodyRightWorld = FVector::RightVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	FVector BodyUpWorld = FVector::UpVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	FVector AngularVelocityWorldDegPerSec = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	FVector AngularVelocityBodyDegPerSec = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	double AltitudeM = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	double MassKg = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation|Target")
	FVector PositionErrorToLandingTargetM = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation|Target")
	FVector VelocityRelativeToLandingTargetMps = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation|Target")
	double HorizontalDistanceToLandingTargetM = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation|Target")
	double DistanceToLandingTargetM = 0.0;
};
