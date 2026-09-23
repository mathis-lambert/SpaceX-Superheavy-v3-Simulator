#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Recovery/Flight/RecoveryTowerMechanics.h"
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
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mechanics") FRecoveryTowerMechanics Mechanics;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Site") FVector CaptureOffsetM = FVector(24, 0, 20);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tower", meta=(ClampMin="85", ClampMax="140")) double TowerHeightM = 105;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tower") double ArmContactHeightAboveBaseM = 61.5678;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tower") double ArmClosure = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tower") TObjectPtr<USceneComponent> Carriage;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tower") TObjectPtr<UStaticMeshComponent> LeftArm;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tower") TObjectPtr<UStaticMeshComponent> RightArm;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tower") TObjectPtr<UInstancedStaticMeshComponent> Structure;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tower") TObjectPtr<UStaticMeshComponent> ArchitecturalDetails;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Contacts") TObjectPtr<UBoxComponent> LeftRail;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Contacts") TObjectPtr<UBoxComponent> RightRail;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Contacts") TObjectPtr<UBoxComponent> LeftArmCollider;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Contacts") TObjectPtr<UBoxComponent> RightArmCollider;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mechanics") TObjectPtr<UPhysicsConstraintComponent> LeftHinge;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mechanics") TObjectPtr<UPhysicsConstraintComponent> RightHinge;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mechanics") TObjectPtr<UPhysicsConstraintComponent> LeftSuspension;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mechanics") TObjectPtr<UPhysicsConstraintComponent> RightSuspension;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mechanics") TObjectPtr<UStaticMeshComponent> LeftRailVisual;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mechanics") TObjectPtr<UStaticMeshComponent> RightRailVisual;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> RailActuatorVisuals;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mechanics") FVector2D RailCompressionM=FVector2D::ZeroVector;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mechanics") FVector2D RailLoadN=FVector2D::ZeroVector;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mechanics") FVector2D PeakRailLoadN=FVector2D::ZeroVector;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mechanics") double CommandedClosure=0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mechanics") int32 BrokenRailMask=0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mechanics") int32 BrokenHingeMask=0;
    UFUNCTION(BlueprintPure, Category="Site") FVector GetCaptureBaseWorld() const;
    UFUNCTION(BlueprintCallable, Category="Tower") void SetArmClosure(double Value);
    bool IsSupport(const UPrimitiveComponent* Component, int32& Side) const;
    void Release();
    /** Only a complete mission reset / test initial condition may place hardware. */
    void ResetMechanism(double InitialClosure=0);
    bool IsMechanismDynamic() const;
private:
    void Beam(const FVector& A, const FVector& B, double Width);
    void PlaceMechanism(double Closure);
    void ConfigureMechanism();
    void BuildMechanismVisuals();
    void UpdateMechanismVisuals();
    bool bMechanismInitialized=false;
};
