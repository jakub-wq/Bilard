#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CueSkin.h"
#include "PoolMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UPanelWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPoolMenuPlayClicked);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPoolMenuQuitClicked);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPoolMenuCueSkinSelected, ECueSkin, SelectedSkin);

UCLASS()
class BILLARD_API UPoolMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	void SetSubtitleText(const FString& InText);
	void SetSelectedCueSkin(ECueSkin InSkin);

	UPROPERTY(BlueprintAssignable, Category = "Billiards")
	FPoolMenuPlayClicked OnPlayClicked;

	UPROPERTY(BlueprintAssignable, Category = "Billiards")
	FPoolMenuQuitClicked OnQuitClicked;

	UPROPERTY(BlueprintAssignable, Category = "Billiards")
	FPoolMenuCueSkinSelected OnCueSkinSelected;

protected:
	void BuildWidgetTree();
	void UpdateSkinSelectionVisuals();
	void ShowMainMenu();
	void ShowSettingsMenu();

	UFUNCTION()
	void HandlePlayClicked();

	UFUNCTION()
	void HandleQuitClicked();

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

	UPROPERTY()
	UButton* PlayButton = nullptr;

	UPROPERTY()
	UButton* SettingsButton = nullptr;

	UPROPERTY()
	UButton* QuitButton = nullptr;

	UPROPERTY()
	UButton* BackButton = nullptr;

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

	FString MainSubtitleText = TEXT("Kliknij Graj, aby rozpocząć partię.");
	ECueSkin SelectedCueSkin = ECueSkin::Standard;
	bool bSettingsVisible = false;
};
