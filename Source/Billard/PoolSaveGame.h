#pragma once

#include "CueSkin.h"
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "PoolMatchTypes.h"
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

	UPROPERTY()
	ECueSkin SelectedCueSkin = ECueSkin::Standard;

	UPROPERTY()
	EPoolMatchMode MatchMode = EPoolMatchMode::Training;

	UPROPERTY()
	EPoolPlayerSide ActivePlayer = EPoolPlayerSide::Blue;

	UPROPERTY()
	EPoolPlayerSide Winner = EPoolPlayerSide::Blue;

	UPROPERTY()
	EPoolBallGroup BlueAssignedGroup = EPoolBallGroup::Unassigned;

	UPROPERTY()
	EPoolBallGroup RedAssignedGroup = EPoolBallGroup::Unassigned;

	UPROPERTY()
	int32 BluePocketedCount = 0;

	UPROPERTY()
	int32 RedPocketedCount = 0;

	UPROPERTY()
	bool bMatchFinished = false;

	UPROPERTY()
	FTransform BluePlayerTransform = FTransform::Identity;

	UPROPERTY()
	FRotator BlueControlRotation = FRotator::ZeroRotator;

	UPROPERTY()
	bool bHasBluePlayerState = false;

	UPROPERTY()
	FTransform RedPlayerTransform = FTransform::Identity;

	UPROPERTY()
	FRotator RedControlRotation = FRotator::ZeroRotator;

	UPROPERTY()
	bool bHasRedPlayerState = false;
};
