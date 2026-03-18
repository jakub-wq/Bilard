#include "PoolGameMode.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "MyCharacter.h"
#include "PoolHUD.h"
#include "PoolTableManager.h"

APoolGameMode::APoolGameMode()
{
	DefaultPawnClass = AMyCharacter::StaticClass();
	HUDClass = APoolHUD::StaticClass();
}

void APoolGameMode::StartPlay()
{
	Super::StartPlay();

	if (!PoolManager)
	{
		for (TActorIterator<APoolTableManager> It(GetWorld()); It; ++It)
		{
			if (IsValid(*It))
			{
				PoolManager = *It;
				break;
			}
		}
	}

	if (!PoolManager)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Name = TEXT("RuntimePoolTableManager");
		PoolManager = GetWorld()->SpawnActor<APoolTableManager>(APoolTableManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	}
}
