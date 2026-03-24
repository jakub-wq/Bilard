#include "PoolPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"
#include "MyCharacter.h"
#include "PoolGameMode.h"
#include "PoolMenuWidget.h"

void APoolPlayerController::BeginPlay()
{
	Super::BeginPlay();

	EnsureMenuWidget();
	GetWorldTimerManager().SetTimerForNextTick(this, &APoolPlayerController::ApplyInitialPresentation);
}

void APoolPlayerController::ApplyInitialPresentation()
{
	OpenMenu(false);
}

void APoolPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindAction(TEXT("ToggleMenu"), IE_Pressed, this, &APoolPlayerController::HandleToggleMenu);
	}
}

void APoolPlayerController::EnsureMenuWidget()
{
	if (MenuWidget)
	{
		return;
	}

	MenuWidget = CreateWidget<UPoolMenuWidget>(this, UPoolMenuWidget::StaticClass());
	if (!MenuWidget)
	{
		return;
	}

	MenuWidget->AddToPlayerScreen(100);
	MenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	MenuWidget->OnPlayClicked.RemoveDynamic(this, &APoolPlayerController::HandlePlayClicked);
	MenuWidget->OnPlayClicked.AddDynamic(this, &APoolPlayerController::HandlePlayClicked);
	MenuWidget->OnQuitClicked.RemoveDynamic(this, &APoolPlayerController::HandleQuitClicked);
	MenuWidget->OnQuitClicked.AddDynamic(this, &APoolPlayerController::HandleQuitClicked);
}

void APoolPlayerController::OpenMenu(bool bSaveState)
{
	EnsureMenuWidget();
	if (!MenuWidget || bMenuVisible)
	{
		return;
	}

	if (AMyCharacter* MyCharacter = Cast<AMyCharacter>(GetPawn()))
	{
		MyCharacter->PrepareForMenu();
	}

	if (APoolGameMode* GameMode = GetWorld() ? Cast<APoolGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		if (bSaveState)
		{
			GameMode->SaveCurrentState();
			MenuWidget->SetSubtitleText(TEXT("Gra zatrzymana. Aktualny stan rozgrywki został zapisany."));
		}
		else if (GameMode->HasSavedGameState())
		{
			MenuWidget->SetSubtitleText(TEXT("Dostępny jest zapis rozgrywki. Kliknij Graj, aby ją wznowić."));
		}
		else
		{
			MenuWidget->SetSubtitleText(TEXT("Kliknij Graj, aby rozpocząć partię."));
		}
	}

	bMenuVisible = true;
	MenuWidget->SetVisibility(ESlateVisibility::Visible);
	SetPause(true);
	SetHUDVisible(false);
	bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(MenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	MenuWidget->SetFocus();
}

void APoolPlayerController::CloseMenuAndResume()
{
	if (!bMenuVisible || !MenuWidget)
	{
		return;
	}

	if (APoolGameMode* GameMode = GetWorld() ? Cast<APoolGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		if (GameMode->HasSavedGameState())
		{
			GameMode->LoadSavedState();
		}
	}

	bMenuVisible = false;
	MenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	SetPause(false);
	SetHUDVisible(true);
	bShowMouseCursor = false;

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
}

void APoolPlayerController::HandleToggleMenu()
{
	if (!bMenuVisible)
	{
		OpenMenu(true);
	}
}

void APoolPlayerController::SetHUDVisible(bool bVisible) const
{
	if (AMyCharacter* MyCharacter = Cast<AMyCharacter>(GetPawn()))
	{
		MyCharacter->SetInGameHUDVisible(bVisible);
	}
}

void APoolPlayerController::HandlePlayClicked()
{
	CloseMenuAndResume();
}

void APoolPlayerController::HandleQuitClicked()
{
	if (APoolGameMode* GameMode = GetWorld() ? Cast<APoolGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		GameMode->SaveCurrentState();
	}

	SetPause(false);
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}
