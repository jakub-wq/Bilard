#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "PoolSaveGame.generated.h"

USTRUCT()
struct FPoolBallSaveState
{
	GENERATED_BODY()

	UPROPERTY()
	int32 BallNumber = INDEX_NONE;

	UPROPERTY()
	bool bCueBall = false;

	UPROPERTY()
	bool bPocketed = false;

	UPROPERTY()
	FTransform Transform = FTransform::Identity;
};

UCLASS()
class BILLARD_API UPoolSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FPoolBallSaveState> BallStates;

	UPROPERTY()
	int32 PocketedBallCount = 0;

	UPROPERTY()
	FTransform PlayerTransform = FTransform::Identity;

	UPROPERTY()
	FRotator PlayerControlRotation = FRotator::ZeroRotator;

	UPROPERTY()
	bool bHasPlayerState = false;
};
