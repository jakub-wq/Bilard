#include "PoolGameMode.h"

#include "GameFramework/PlayerController.h"
#include "MyCharacter.h"
#include "PoolTableManager.h"

APoolGameMode::APoolGameMode()
{
	DefaultPawnClass = AMyCharacter::StaticClass();
}

void APoolGameMode::StartPlay()
{
	Super::StartPlay();

	if (!PoolManager)
	{
		PoolManager = GetWorld()->SpawnActor<APoolTableManager>(APoolTableManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	}
}
