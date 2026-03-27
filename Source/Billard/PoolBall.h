#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PoolBall.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;
class USphereComponent;
class UPrimitiveComponent;
class UPhysicalMaterial;
struct FHitResult;

UCLASS()
class BILLARD_API APoolBall : public AActor
{
	GENERATED_BODY()

public:
	APoolBall();

	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	void ConfigureBall(bool bInCueBall, int32 InBallNumber, const FLinearColor& InColor);
	void ResetBall(const FTransform& NewTransform);
	void PocketBall();
	void BeginPocketSink(const FVector& SinkTargetLocation);
	bool IsMoving() const;
	void ApplyShotImpulse(const FVector& Direction, float Power, const FVector& TableUpAxis);
	void SetBallRadius(float InRadius);
	void SetMovementPlane(const FVector& InTableUpAxis, const FVector& InPlaneOrigin);
	FVector GetLinearVelocity() const;
	void SetLinearVelocity(const FVector& NewVelocity);
	FVector GetAngularVelocityDegrees() const;
	void SetAngularVelocityDegrees(const FVector& NewVelocity);
	void TeleportBall(const FVector& NewLocation);
	float GetBallRadius() const { return BallRadiusCm; }
	static float CalculateShotImpulseMagnitude(float RequestedPower, float ImpulseScale, float MinImpulse, float MaxImpulse);
	static FVector ClampVelocityToTablePlane(const FVector& LinearVelocity, const FVector& UpAxis, float MaxPlanarSpeed);
	static void ComputeElasticCollisionResponse(
		const FVector& VelocityA,
		const FVector& VelocityB,
		const FVector& ContactNormal,
		float Restitution,
		FVector& OutVelocityA,
		FVector& OutVelocityB);

	UStaticMeshComponent* GetBallMesh() const { return BallMesh; }
	bool IsCueBall() const { return bCueBall; }
	bool IsPocketed() const { return bPocketed; }
	bool IsSinkingIntoPocket() const { return bSinkingIntoPocket; }
	int32 GetBallNumber() const { return BallNumber; }

protected:
	UStaticMesh* ResolveImportedBallMesh(bool bInCueBall, int32 InBallNumber) const;
	void ApplyFallbackColor(const FLinearColor& InColor);
	void ConfigurePhysicsState();
	void UpdateVisualMeshTransform();
	void EnsurePhysicsMaterial();
	void UpdatePocketSink(float DeltaTime);
	void ApplyBallCollisionResponse(APoolBall* OtherBall);

	UFUNCTION()
	void HandleCollision(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* BallMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Billiards")
	float LinearDamping = 0.38f;

	UPROPERTY(EditDefaultsOnly, Category = "Billiards")
	float AngularDamping = 0.78f;

	UPROPERTY(EditDefaultsOnly, Category = "Billiards")
	float SleepVelocityThreshold = 1.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Billiards")
	float MaxPlanarSpeed = 900.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Billiards")
	float ShotImpulseScale = 0.24f;

	UPROPERTY(EditDefaultsOnly, Category = "Billiards")
	float MinShotImpulse = 52.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Billiards")
	float MaxShotImpulse = 350.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Billiards|Collision")
	float BallCollisionRestitution = 0.97f;

	UPROPERTY(EditDefaultsOnly, Category = "Billiards|Collision")
	float BallCollisionMinClosingSpeed = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Billiards|Collision")
	float BallCollisionStruckBallTangentDamping = 0.12f;

	UPROPERTY(EditDefaultsOnly, Category = "Billiards|Collision")
	float ClothFriction = 0.12f;

	UPROPERTY(EditDefaultsOnly, Category = "Billiards|Collision")
	float ClothRestitution = 0.03f;

	UPROPERTY(EditDefaultsOnly, Category = "Billiards|Pocket")
	float PocketSinkDuration = 0.46f;

	UPROPERTY(VisibleInstanceOnly, Category = "Billiards")
	bool bCueBall = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Billiards")
	bool bPocketed = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Billiards")
	bool bSinkingIntoPocket = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Billiards")
	int32 BallNumber = 0;

	UPROPERTY()
	UMaterialInterface* BaseMaterial = nullptr;

	UPROPERTY(Transient)
	UPhysicalMaterial* BallPhysicalMaterial = nullptr;

	UPROPERTY(Transient)
	bool bUsingImportedVisual = false;

	float BallRadiusCm = 5.7f;
	FVector TableUpVector = FVector::UpVector;
	FVector MovementPlaneOrigin = FVector::ZeroVector;
	FTransform InitialTransform;
	FVector PocketSinkStart = FVector::ZeroVector;
	FVector PocketSinkControlPoint = FVector::ZeroVector;
	FVector PocketSinkTarget = FVector::ZeroVector;
	float PocketSinkAlpha = 0.0f;
	bool bHasMovementPlane = false;
	TWeakObjectPtr<APoolBall> LastCollisionBall;
	uint64 LastCollisionFrame = 0;
};
