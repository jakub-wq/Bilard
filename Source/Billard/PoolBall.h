#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PoolBall.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;

UCLASS()
class BILLARD_API APoolBall : public AActor
{
	GENERATED_BODY()

public:
	APoolBall();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	void ConfigureBall(bool bInCueBall, int32 InBallNumber, const FLinearColor& InColor);
	void ResetBall(const FTransform& NewTransform);
	void PocketBall();
	bool IsMoving() const;
	void ApplyShotImpulse(const FVector& Direction, float Power);
	void SetBallRadius(float InRadius);
	float GetBallRadius() const { return BallRadiusCm; }

	UStaticMeshComponent* GetBallMesh() const { return BallMesh; }
	bool IsCueBall() const { return bCueBall; }
	bool IsPocketed() const { return bPocketed; }
	int32 GetBallNumber() const { return BallNumber; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* BallMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Billiards")
	float LinearDamping = 1.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Billiards")
	float AngularDamping = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Billiards")
	float SleepVelocityThreshold = 2.5f;

	UPROPERTY(VisibleInstanceOnly, Category = "Billiards")
	bool bCueBall = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Billiards")
	bool bPocketed = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Billiards")
	int32 BallNumber = 0;

	UPROPERTY()
	UMaterialInterface* BaseMaterial = nullptr;

	float BallRadiusCm = 5.7f;
	FTransform InitialTransform;
};
