#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PoolPocketTrigger.generated.h"

class USphereComponent;
class UPrimitiveComponent;
struct FHitResult;
class APoolTableManager;
class APoolBall;

UCLASS()
class BILLARD_API APoolPocketTrigger : public AActor
{
	GENERATED_BODY()

public:
	APoolPocketTrigger();
	void SetManager(APoolTableManager* InManager) { Manager = InManager; }
	void SetPocketRadius(float InRadius);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	bool IsEligibleBall(AActor* OtherActor) const;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USphereComponent* TriggerSphere;

	UPROPERTY()
	APoolTableManager* Manager = nullptr;
};
