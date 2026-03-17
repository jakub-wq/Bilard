#include "PoolPocketTrigger.h"

#include "Components/SphereComponent.h"
#include "PoolBall.h"
#include "PoolTableManager.h"

APoolPocketTrigger::APoolPocketTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
	SetRootComponent(TriggerSphere);
	TriggerSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerSphere->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	TriggerSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	TriggerSphere->SetGenerateOverlapEvents(true);
	TriggerSphere->SetSphereRadius(10.0f);
	TriggerSphere->SetHiddenInGame(true);
}

void APoolPocketTrigger::SetPocketRadius(float InRadius)
{
	TriggerSphere->SetSphereRadius(FMath::Max(1.0f, InRadius));
}

void APoolPocketTrigger::BeginPlay()
{
	Super::BeginPlay();
	TriggerSphere->OnComponentBeginOverlap.AddDynamic(this, &APoolPocketTrigger::HandleOverlap);
}

void APoolPocketTrigger::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (APoolBall* Ball = Cast<APoolBall>(OtherActor))
	{
		if (Manager)
		{
			Manager->HandleBallPocketed(Ball);
		}
		else
		{
			Ball->PocketBall();
		}
	}
}
