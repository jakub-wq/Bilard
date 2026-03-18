#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PoolHUD.generated.h"

UCLASS()
class BILLARD_API APoolHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Billiards|HUD")
	float CrosshairHalfSize = 6.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Billiards|HUD")
	float CrosshairThickness = 1.75f;

	UPROPERTY(EditDefaultsOnly, Category = "Billiards|HUD")
	FLinearColor CrosshairColor = FLinearColor::White;
};
