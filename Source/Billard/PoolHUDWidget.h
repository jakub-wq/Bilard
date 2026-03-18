#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PoolHUDWidget.generated.h"

class UButton;
class UTextBlock;
class UProgressBar;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPoolResetClicked);

UCLASS()
class BILLARD_API UPoolHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	void SetShotPowerPercent(float InPercent);
	void SetAimMode(bool bInAimMode);
	void SetHintText(const FString& InText);
	void SetPocketedCount(int32 InCount);

	UPROPERTY(BlueprintAssignable, Category = "Billiards")
	FPoolResetClicked OnResetClicked;

protected:
	UFUNCTION()
	void HandleResetClicked();

	UPROPERTY()
	UButton* ResetButton = nullptr;

	UPROPERTY()
	UTextBlock* ResetLabel = nullptr;

	UPROPERTY()
	UTextBlock* CrosshairText = nullptr;

	UPROPERTY()
	UTextBlock* HintText = nullptr;

	UPROPERTY()
	UTextBlock* PocketedCountText = nullptr;

	UPROPERTY()
	UProgressBar* PowerBar = nullptr;
};
