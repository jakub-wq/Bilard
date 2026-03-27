#include "PoolGameMode.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "MyCharacter.h"
#include "PoolBall.h"
#include "PoolHUD.h"
#include "PoolPlayerController.h"
#include "PoolSaveGame.h"
#include "PoolTableManager.h"

namespace
{
	constexpr TCHAR PoolSaveSlotName[] = TEXT("PoolAutosave");
	constexpr int32 PoolSaveUserIndex = 0;
}

APoolGameMode::APoolGameMode()
{
	DefaultPawnClass = AMyCharacter::StaticClass();
	HUDClass = APoolHUD::StaticClass();
	PlayerControllerClass = APoolPlayerController::StaticClass();
}

void APoolGameMode::StartPlay()
{
	Super::StartPlay();
	ResolvePoolManager();
	if (PoolManager)
	{
		PoolManager->ResetRack();
	}
}

void APoolGameMode::SetMatchMode(EPoolMatchMode NewMode)
{
	ResolvePoolManager();
	if (PoolManager)
	{
		PoolManager->SetMatchMode(NewMode);
	}
}

EPoolMatchMode APoolGameMode::GetMatchMode() const
{
	return PoolManager ? PoolManager->GetMatchMode() : EPoolMatchMode::Training;
}

void APoolGameMode::ResolvePoolManager()
{
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

bool APoolGameMode::SaveCurrentState()
{
	ResolvePoolManager();
	if (!PoolManager)
	{
		return false;
	}

	UPoolSaveGame* SaveGame = Cast<UPoolSaveGame>(UGameplayStatics::CreateSaveGameObject(UPoolSaveGame::StaticClass()));
	if (!SaveGame)
	{
		return false;
	}

	for (APoolBall* Ball : PoolManager->GetSpawnedBalls())
	{
		if (!IsValid(Ball))
		{
			continue;
		}

		FPoolBallSaveState State;
		State.BallNumber = Ball->GetBallNumber();
		State.bCueBall = Ball->IsCueBall();
		State.bPocketed = Ball->IsPocketed();
		State.Transform = Ball->GetActorTransform();
		SaveGame->BallStates.Add(State);
	}

	SaveGame->PocketedBallCount = PoolManager->GetPocketedBallCount();
	PoolManager->WriteMatchStateToSaveGame(*SaveGame);

	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (APawn* Pawn = PlayerController->GetPawn())
		{
			SaveGame->PlayerTransform = Pawn->GetActorTransform();
			SaveGame->PlayerControlRotation = PlayerController->GetControlRotation();
			SaveGame->bHasPlayerState = true;
		}
	}

	return UGameplayStatics::SaveGameToSlot(SaveGame, PoolSaveSlotName, PoolSaveUserIndex);
}

bool APoolGameMode::LoadSavedState()
{
	if (!HasSavedGameState())
	{
		return false;
	}

	UPoolSaveGame* SaveGame = Cast<UPoolSaveGame>(UGameplayStatics::LoadGameFromSlot(PoolSaveSlotName, PoolSaveUserIndex));
	if (!SaveGame)
	{
		return false;
	}

	ResolvePoolManager();
	if (!PoolManager)
	{
		return false;
	}

	PoolManager->ResetRack();
	PoolManager->LoadMatchStateFromSaveGame(*SaveGame);
	if (!PoolManager->ApplySavedBallStates(SaveGame->BallStates, SaveGame->PocketedBallCount))
	{
		ClearSavedGameState();
		return false;
	}

	if (SaveGame->bHasPlayerState)
	{
		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
		{
			if (APawn* Pawn = PlayerController->GetPawn())
			{
				Pawn->SetActorTransform(SaveGame->PlayerTransform, false, nullptr, ETeleportType::TeleportPhysics);
				PlayerController->SetControlRotation(SaveGame->PlayerControlRotation);
			}
		}
	}

	return true;
}

bool APoolGameMode::HasSavedGameState() const
{
	return UGameplayStatics::DoesSaveGameExist(PoolSaveSlotName, PoolSaveUserIndex);
}

void APoolGameMode::ClearSavedGameState()
{
	if (HasSavedGameState())
	{
		UGameplayStatics::DeleteGameInSlot(PoolSaveSlotName, PoolSaveUserIndex);
	}
}
