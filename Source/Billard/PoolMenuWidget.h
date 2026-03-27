#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CueSkin.h"
#include "PoolMatchTypes.h"
#include "PoolMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UPanelWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPoolMenuPlayClicked);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPoolMenuQuitClicked);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPoolMenuCueSkinSelected, ECueSkin, SelectedSkin);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPoolMenuModeSelected, EPoolMatchMode, SelectedMode);

UCLASS()
class BILLARD_API UPoolMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	void SetSubtitleText(const FString& InText);
	void SetSelectedCueSkin(ECueSkin InSkin);
	void SetSelectedMatchMode(EPoolMatchMode InMode);

	UPROPERTY(BlueprintAssignable, Category = "Billiards")
	FPoolMenuPlayClicked OnPlayClicked;

	UPROPERTY(BlueprintAssignable, Category = "Billiards")
	FPoolMenuQuitClicked OnQuitClicked;

	UPROPERTY(BlueprintAssignable, Category = "Billiards")
	FPoolMenuCueSkinSelected OnCueSkinSelected;

	UPROPERTY(BlueprintAssignable, Category = "Billiards")
	FPoolMenuModeSelected OnModeSelected;

protected:
	void BuildWidgetTree();
	void UpdateSkinSelectionVisuals();
	void ShowMainMenu();
	void ShowSettingsMenu();
	void ShowModeMenu();

	UFUNCTION()
	void HandlePlayClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UFUNCTION()
	void HandleGameModeClicked();

	UFUNCTION()
	void HandleSettingsClicked();

	UFUNCTION()
	void HandleBackClicked();

	UFUNCTION()
	void HandleStandardSkinClicked();

	UFUNCTION()
	void HandleBlueSkinClicked();

	UFUNCTION()
	void HandleRedSkinClicked();

	UFUNCTION()
	void HandleYellowSkinClicked();

	UFUNCTION()
	void HandleTrainingModeClicked();

	UFUNCTION()
	void HandleLocalVersusModeClicked();

	UPROPERTY()
	UButton* PlayButton = nullptr;

	UPROPERTY()
	UButton* TrainingModeButton = nullptr;

	UPROPERTY()
	UButton* LocalVersusButton = nullptr;

	UPROPERTY()
	UButton* SettingsButton = nullptr;

	UPROPERTY()
	UButton* QuitButton = nullptr;

	UPROPERTY()
	UButton* BackButton = nullptr;

	UPROPERTY()
	UButton* SettingsBackButton = nullptr;

	UPROPERTY()
	UButton* StandardSkinButton = nullptr;

	UPROPERTY()
	UButton* BlueSkinButton = nullptr;

	UPROPERTY()
	UButton* RedSkinButton = nullptr;

	UPROPERTY()
	UButton* YellowSkinButton = nullptr;

	UPROPERTY()
	UTextBlock* SubtitleText = nullptr;

	UPROPERTY()
	UTextBlock* StandardSkinLabel = nullptr;

	UPROPERTY()
	UTextBlock* BlueSkinLabel = nullptr;

	UPROPERTY()
	UTextBlock* RedSkinLabel = nullptr;

	UPROPERTY()
	UTextBlock* YellowSkinLabel = nullptr;

	UPROPERTY()
	UPanelWidget* MainMenuPanel = nullptr;

	UPROPERTY()
	UPanelWidget* SettingsPanel = nullptr;

	UPROPERTY()
	UPanelWidget* ModePanel = nullptr;

	FString MainSubtitleText = TEXT("Wybierz tryb gry, aby rozpocząć rozgrywkę.");
	ECueSkin SelectedCueSkin = ECueSkin::Standard;
	EPoolMatchMode SelectedMatchMode = EPoolMatchMode::Training;
	bool bSettingsVisible = false;
};
