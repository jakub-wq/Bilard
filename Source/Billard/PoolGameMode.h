#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PoolMatchTypes.h"
#include "PoolGameMode.generated.h"

class APoolTableManager;

UCLASS()
class BILLARD_API APoolGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APoolGameMode();
	virtual void StartPlay() override;

	APoolTableManager* GetPoolManager() const { return PoolManager; }
	void SetMatchMode(EPoolMatchMode NewMode);
	EPoolMatchMode GetMatchMode() const;
	bool SaveCurrentState();
	bool LoadSavedState();
	bool HasSavedGameState() const;
	void ClearSavedGameState();

protected:
	void ResolvePoolManager();

	UPROPERTY()
	APoolTableManager* PoolManager = nullptr;
};
