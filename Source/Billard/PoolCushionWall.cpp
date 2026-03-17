#include "PoolCushionWall.h"

#include "Components/BoxComponent.h"

APoolCushionWall::APoolCushionWall()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionBox->SetHiddenInGame(true);
}

void APoolCushionWall::ConfigureWall(const FVector& InExtent)
{
	CollisionBox->SetBoxExtent(InExtent);
}
