#pragma once

#include "CueSkin.h"
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PoolMatchTypes.h"
#include "PoolPlayerController.generated.h"

class UPoolMenuWidget;

UCLASS()
class BILLARD_API APoolPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

protected:
	void ApplyInitialPresentation();
	void EnsureMenuWidget();
	void OpenMenu(bool bSaveState);
	void CloseMenuAndResume();
	void HandleToggleMenu();
	void SetHUDVisible(bool bVisible) const;

	UFUNCTION()
	void HandlePlayClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UFUNCTION()
	void HandleCueSkinSelected(ECueSkin SelectedSkin);

	UFUNCTION()
	void HandleModeSelected(EPoolMatchMode SelectedMode);

	UPROPERTY()
	UPoolMenuWidget* MenuWidget = nullptr;

	bool bMenuVisible = false;
	bool bLoadSavedStateOnResume = true;
};
