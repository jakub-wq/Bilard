#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
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

protected:
	UPROPERTY()
	APoolTableManager* PoolManager = nullptr;
};
