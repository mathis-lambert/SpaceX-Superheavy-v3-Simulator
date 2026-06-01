#pragma once

#include "Components/ActorComponent.h"
#include "Navigation/SuperHeavyNavigationState.h"
#include "SuperHeavyNavigationComponent.generated.h"

class UPrimitiveComponent;

UCLASS(ClassGroup = (SuperHeavy), meta = (BlueprintSpawnableComponent))
class SUPERHEAVYSIM_API USuperHeavyNavigationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USuperHeavyNavigationComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
	FName PhysicsComponentName = TEXT("COL_Body_Main");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
	double AltitudeReferenceWorldZCm = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	FSuperHeavyNavigationState LastState;

	UFUNCTION(BlueprintCallable, Category = "Navigation")
	void ResetNavigation();

	UFUNCTION(BlueprintCallable, Category = "Navigation")
	FSuperHeavyNavigationState CaptureNavigationState(const FTransform& LandingTargetWorldTransform, const FVector& LandingTargetVelocityMps);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Navigation")
	UPrimitiveComponent* GetPhysicsComponent() const { return PhysicsComponent.Get(); }

protected:
	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> PhysicsComponent;

	FVector PreviousVelocityWorldMps = FVector::ZeroVector;
	double PreviousTimeSeconds = 0.0;
	bool bHasPreviousState = false;

	void ResolvePhysicsComponent();
};
