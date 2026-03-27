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
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	void SetShotPowerPercent(float InPercent);
	void SetAimMode(bool bInAimMode);
	void SetHintText(const FString& InText);
	void SetPocketedCount(int32 InCount);
	void SetTurnText(const FString& InText, const FLinearColor& InColor);
	void SetOpponentText(const FString& InText, const FLinearColor& InColor);
	void SetWinnerText(const FString& InText);
	void SetBlueScoreText(const FString& InText);
	void SetRedScoreText(const FString& InText);
	void SetLocalScoreboardVisible(bool bVisible);

	UPROPERTY(BlueprintAssignable, Category = "Billiards")
	FPoolResetClicked OnResetClicked;

protected:
	void BuildWidgetTree();

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
	UTextBlock* TurnText = nullptr;

	UPROPERTY()
	UTextBlock* OpponentText = nullptr;

	UPROPERTY()
	UTextBlock* WinnerText = nullptr;

	UPROPERTY()
	UTextBlock* BlueScoreText = nullptr;

	UPROPERTY()
	UTextBlock* RedScoreText = nullptr;

	UPROPERTY()
	UProgressBar* PowerBar = nullptr;
};
