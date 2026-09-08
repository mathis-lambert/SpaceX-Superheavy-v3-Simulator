#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SuperHeavyLaunchTower.generated.h"

class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UPhysicsConstraintComponent;
class UPrimitiveComponent;
class UBoxComponent;

UCLASS(Blueprintable)
class SUPERHEAVYSIM_API ASuperHeavyLaunchTower : public AActor
{
    GENERATED_BODY()
public:
    ASuperHeavyLaunchTower();
    virtual void OnConstruction(const FTransform& Transform) override;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Site") FVector CaptureOffsetM = FVector(24, 0, 20);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tower", meta=(ClampMin="85", ClampMax="140")) double TowerHeightM = 105;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tower") double ArmContactHeightAboveBaseM = 61.5678;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tower") double ArmClosure = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tower") TObjectPtr<USceneComponent> Carriage;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tower") TObjectPtr<UStaticMeshComponent> LeftArm;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tower") TObjectPtr<UStaticMeshComponent> RightArm;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tower") TObjectPtr<UPhysicsConstraintComponent> CaptureConstraint;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tower") TObjectPtr<UInstancedStaticMeshComponent> Structure;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tower") TObjectPtr<UStaticMeshComponent> ArchitecturalDetails;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Contacts") TObjectPtr<UBoxComponent> LeftRail;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Contacts") TObjectPtr<UBoxComponent> RightRail;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Contacts") TObjectPtr<UBoxComponent> LeftArmCollider;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Contacts") TObjectPtr<UBoxComponent> RightArmCollider;
    UFUNCTION(BlueprintPure, Category="Site") FVector GetCaptureBaseWorld() const;
    UFUNCTION(BlueprintCallable, Category="Tower") void SetArmClosure(double Value);
    bool IsSupport(const UPrimitiveComponent* Component, int32& Side) const;
    void Release();
private:
    void Beam(const FVector& A, const FVector& B, double Width);
};
