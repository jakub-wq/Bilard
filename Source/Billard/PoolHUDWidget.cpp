#include "PoolHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Styling/SlateColor.h"

TSharedRef<SWidget> UPoolHUDWidget::RebuildWidget()
{
	BuildWidgetTree();
	return Super::RebuildWidget();
}

void UPoolHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ResetButton)
	{
		ResetButton->OnClicked.RemoveDynamic(this, &UPoolHUDWidget::HandleResetClicked);
		ResetButton->OnClicked.AddDynamic(this, &UPoolHUDWidget::HandleResetClicked);
	}

	SetAimMode(false);
}

void UPoolHUDWidget::BuildWidgetTree()
{
	ResetButton = nullptr;
	ResetLabel = nullptr;
	CrosshairText = nullptr;
	HintText = nullptr;
	PocketedCountText = nullptr;
	TurnText = nullptr;
	OpponentText = nullptr;
	WinnerText = nullptr;
	BlueScoreText = nullptr;
	RedScoreText = nullptr;
	PowerBar = nullptr;

	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	ResetButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ResetButton"));
	ResetLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResetLabel"));
	ResetLabel->SetText(FText::FromString(TEXT("Reset bil [R]")));
	ResetLabel->SetJustification(ETextJustify::Center);
	ResetButton->AddChild(ResetLabel);
	RootCanvas->AddChild(ResetButton);

	if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(ResetButton->Slot))
	{
		PanelSlot->SetAutoSize(true);
		PanelSlot->SetAnchors(FAnchors(1.0f, 0.0f));
		PanelSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		PanelSlot->SetPosition(FVector2D(-20.0f, 20.0f));
	}

	CrosshairText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CrosshairText"));
	CrosshairText->SetText(FText::FromString(TEXT("+")));
	CrosshairText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	CrosshairText->SetShadowOffset(FVector2D(2.0f, 2.0f));
	CrosshairText->SetShadowColorAndOpacity(FLinearColor::Black);
	CrosshairText->SetRenderScale(FVector2D(1.8f, 1.8f));
	CrosshairText->SetRenderOpacity(1.0f);
	RootCanvas->AddChild(CrosshairText);
	if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(CrosshairText->Slot))
	{
		PanelSlot->SetAutoSize(true);
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PanelSlot->SetPosition(FVector2D(0.0f, 0.0f));
	}

	HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HintText"));
	HintText->SetText(FText::FromString(TEXT("Podejdź do stołu i kliknij celownikiem w białą bilę.")));
	HintText->SetJustification(ETextJustify::Center);
	RootCanvas->AddChild(HintText);
	if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(HintText->Slot))
	{
		PanelSlot->SetAutoSize(true);
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.0f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		PanelSlot->SetPosition(FVector2D(0.0f, 20.0f));
	}

	TurnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TurnText"));
	TurnText->SetText(FText::FromString(TEXT("Tryb treningowy")));
	TurnText->SetJustification(ETextJustify::Center);
	TurnText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.97f, 0.94f)));
	RootCanvas->AddChild(TurnText);
	if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(TurnText->Slot))
	{
		PanelSlot->SetAutoSize(true);
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.0f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		PanelSlot->SetPosition(FVector2D(0.0f, 54.0f));
	}

	OpponentText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("OpponentText"));
	OpponentText->SetText(FText::FromString(TEXT("")));
	OpponentText->SetJustification(ETextJustify::Left);
	OpponentText->SetColorAndOpacity(FSlateColor(FLinearColor(0.84f, 0.86f, 0.90f)));
	RootCanvas->AddChild(OpponentText);
	if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(OpponentText->Slot))
	{
		PanelSlot->SetAutoSize(true);
		PanelSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		PanelSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		PanelSlot->SetPosition(FVector2D(20.0f, 20.0f));
	}

	PocketedCountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PocketedCountText"));
	PocketedCountText->SetText(FText::FromString(TEXT("Wbite bile: 0/15")));
	PocketedCountText->SetJustification(ETextJustify::Right);
	RootCanvas->AddChild(PocketedCountText);
	if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(PocketedCountText->Slot))
	{
		PanelSlot->SetAutoSize(true);
		PanelSlot->SetAnchors(FAnchors(1.0f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(1.0f, 0.5f));
		PanelSlot->SetPosition(FVector2D(-24.0f, 0.0f));
	}

	BlueScoreText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BlueScoreText"));
	BlueScoreText->SetText(FText::FromString(TEXT("Niebieski: 0")));
	BlueScoreText->SetJustification(ETextJustify::Right);
	BlueScoreText->SetColorAndOpacity(FSlateColor(FLinearColor(0.30f, 0.62f, 1.0f)));
	BlueScoreText->SetVisibility(ESlateVisibility::Collapsed);
	RootCanvas->AddChild(BlueScoreText);
	if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(BlueScoreText->Slot))
	{
		PanelSlot->SetAutoSize(true);
		PanelSlot->SetAnchors(FAnchors(1.0f, 0.0f));
		PanelSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		PanelSlot->SetPosition(FVector2D(-24.0f, 56.0f));
	}

	RedScoreText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RedScoreText"));
	RedScoreText->SetText(FText::FromString(TEXT("Czerwony: 0")));
	RedScoreText->SetJustification(ETextJustify::Right);
	RedScoreText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.35f, 0.35f)));
	RedScoreText->SetVisibility(ESlateVisibility::Collapsed);
	RootCanvas->AddChild(RedScoreText);
	if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(RedScoreText->Slot))
	{
		PanelSlot->SetAutoSize(true);
		PanelSlot->SetAnchors(FAnchors(1.0f, 0.0f));
		PanelSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		PanelSlot->SetPosition(FVector2D(-24.0f, 80.0f));
	}

	WinnerText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WinnerText"));
	WinnerText->SetText(FText::FromString(TEXT("")));
	WinnerText->SetJustification(ETextJustify::Center);
	WinnerText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.93f, 0.68f)));
	WinnerText->SetVisibility(ESlateVisibility::Collapsed);
	RootCanvas->AddChild(WinnerText);
	if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(WinnerText->Slot))
	{
		PanelSlot->SetAutoSize(true);
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.25f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	}

	PowerBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("PowerBar"));
	PowerBar->SetPercent(0.0f);
	RootCanvas->AddChild(PowerBar);
	if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(PowerBar->Slot))
	{
		PanelSlot->SetSize(FVector2D(340.0f, 18.0f));
		PanelSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 1.0f));
		PanelSlot->SetPosition(FVector2D(0.0f, -28.0f));
	}
}

void UPoolHUDWidget::SetShotPowerPercent(float InPercent)
{
	if (PowerBar)
	{
		PowerBar->SetPercent(FMath::Clamp(InPercent, 0.0f, 1.0f));
	}
}

void UPoolHUDWidget::SetAimMode(bool bInAimMode)
{
	if (CrosshairText)
	{
		CrosshairText->SetVisibility(ESlateVisibility::Visible);
	}

	if (PowerBar)
	{
		PowerBar->SetVisibility(bInAimMode ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UPoolHUDWidget::SetHintText(const FString& InText)
{
	if (HintText)
	{
		HintText->SetText(FText::FromString(InText));
	}
}

void UPoolHUDWidget::SetPocketedCount(int32 InCount)
{
	if (PocketedCountText)
	{
		PocketedCountText->SetText(FText::FromString(FString::Printf(TEXT("Wbite bile: %d/15"), FMath::Clamp(InCount, 0, 15))));
	}
}

void UPoolHUDWidget::SetTurnText(const FString& InText, const FLinearColor& InColor)
{
	if (TurnText)
	{
		TurnText->SetText(FText::FromString(InText));
		TurnText->SetColorAndOpacity(FSlateColor(InColor));
	}
}

void UPoolHUDWidget::SetOpponentText(const FString& InText, const FLinearColor& InColor)
{
	if (OpponentText)
	{
		OpponentText->SetText(FText::FromString(InText));
		OpponentText->SetColorAndOpacity(FSlateColor(InColor));
		OpponentText->SetVisibility(InText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}

void UPoolHUDWidget::SetWinnerText(const FString& InText)
{
	if (WinnerText)
	{
		WinnerText->SetText(FText::FromString(InText));
		WinnerText->SetVisibility(InText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}

void UPoolHUDWidget::SetBlueScoreText(const FString& InText)
{
	if (BlueScoreText)
	{
		BlueScoreText->SetText(FText::FromString(InText));
	}
}

void UPoolHUDWidget::SetRedScoreText(const FString& InText)
{
	if (RedScoreText)
	{
		RedScoreText->SetText(FText::FromString(InText));
	}
}

void UPoolHUDWidget::SetLocalScoreboardVisible(bool bVisible)
{
	if (PocketedCountText)
	{
		PocketedCountText->SetVisibility(bVisible ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	if (BlueScoreText)
	{
		BlueScoreText->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (RedScoreText)
	{
		RedScoreText->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UPoolHUDWidget::HandleResetClicked()
{
	OnResetClicked.Broadcast();
}
