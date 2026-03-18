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

void UPoolHUDWidget::HandleResetClicked()
{
	OnResetClicked.Broadcast();
}
