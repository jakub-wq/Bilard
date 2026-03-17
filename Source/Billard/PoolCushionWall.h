#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PoolCushionWall.generated.h"

class UBoxComponent;

UCLASS()
class BILLARD_API APoolCushionWall : public AActor
{
	GENERATED_BODY()

public:
	APoolCushionWall();
	void ConfigureWall(const FVector& InExtent);

protected:
	UPROPERTY(VisibleAnywhere)
	UBoxComponent* CollisionBox;
};
