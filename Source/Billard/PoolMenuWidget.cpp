#include "PoolMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/SlateColor.h"
#include "Styling/SlateTypes.h"

namespace
{
	UButton* MakeMenuButton(UWidgetTree* WidgetTree, const TCHAR* Name, const TCHAR* Label)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		FButtonStyle ButtonStyle = Button->GetStyle();
		ButtonStyle.Normal.TintColor = FSlateColor(FLinearColor(0.08f, 0.12f, 0.10f, 0.94f));
		ButtonStyle.Hovered.TintColor = FSlateColor(FLinearColor(0.14f, 0.30f, 0.20f, 0.97f));
		ButtonStyle.Pressed.TintColor = FSlateColor(FLinearColor(0.10f, 0.22f, 0.16f, 0.97f));
		Button->SetStyle(ButtonStyle);

		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("%sLabel"), Name));
		Text->SetText(FText::FromString(Label));
		Text->SetJustification(ETextJustify::Center);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Text->SetShadowOffset(FVector2D(1.0f, 1.0f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.65f));
		Button->AddChild(Text);

		if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Text->Slot))
		{
			ButtonSlot->SetPadding(FMargin(26.0f, 14.0f));
			ButtonSlot->SetHorizontalAlignment(HAlign_Center);
			ButtonSlot->SetVerticalAlignment(VAlign_Center);
		}

		return Button;
	}
}

TSharedRef<SWidget> UPoolMenuWidget::RebuildWidget()
{
	BuildWidgetTree();
	return Super::RebuildWidget();
}

void UPoolMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);

	if (PlayButton)
	{
		PlayButton->OnClicked.RemoveDynamic(this, &UPoolMenuWidget::HandlePlayClicked);
		PlayButton->OnClicked.AddDynamic(this, &UPoolMenuWidget::HandlePlayClicked);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.RemoveDynamic(this, &UPoolMenuWidget::HandleQuitClicked);
		QuitButton->OnClicked.AddDynamic(this, &UPoolMenuWidget::HandleQuitClicked);
	}
}

void UPoolMenuWidget::BuildWidgetTree()
{
	PlayButton = nullptr;
	QuitButton = nullptr;
	SubtitleText = nullptr;

	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* Dimmer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dimmer"));
	Dimmer->SetBrushColor(FLinearColor(0.01f, 0.03f, 0.02f, 0.78f));
	RootCanvas->AddChild(Dimmer);
	if (UCanvasPanelSlot* DimmerSlot = Cast<UCanvasPanelSlot>(Dimmer->Slot))
	{
		DimmerSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		DimmerSlot->SetOffsets(FMargin(0.0f));
	}

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuPanel"));
	Panel->SetBrushColor(FLinearColor(0.06f, 0.08f, 0.07f, 0.94f));
	RootCanvas->AddChild(Panel);
	if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(Panel->Slot))
	{
		PanelSlot->SetAutoSize(true);
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	}

	UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuLayout"));
	Panel->SetContent(Layout);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	Title->SetText(FText::FromString(TEXT("BILARD")));
	Title->SetJustification(ETextJustify::Center);
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 1.0f, 0.97f)));
	Title->SetShadowOffset(FVector2D(2.0f, 2.0f));
	Title->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.75f));
	Layout->AddChildToVerticalBox(Title);
	if (UVerticalBoxSlot* TitleSlot = Cast<UVerticalBoxSlot>(Title->Slot))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(FMargin(42.0f, 34.0f, 42.0f, 12.0f));
	}

	SubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SubtitleText"));
	SubtitleText->SetText(FText::FromString(TEXT("Kliknij Graj, aby rozpocząć partię.")));
	SubtitleText->SetJustification(ETextJustify::Center);
	SubtitleText->SetAutoWrapText(true);
	SubtitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.85f, 0.81f)));
	Layout->AddChildToVerticalBox(SubtitleText);
	if (UVerticalBoxSlot* SubtitleSlot = Cast<UVerticalBoxSlot>(SubtitleText->Slot))
	{
		SubtitleSlot->SetHorizontalAlignment(HAlign_Fill);
		SubtitleSlot->SetPadding(FMargin(34.0f, 0.0f, 34.0f, 18.0f));
	}

	PlayButton = MakeMenuButton(WidgetTree, TEXT("PlayButton"), TEXT("Graj"));
	Layout->AddChildToVerticalBox(PlayButton);
	if (UVerticalBoxSlot* PlaySlot = Cast<UVerticalBoxSlot>(PlayButton->Slot))
	{
		PlaySlot->SetHorizontalAlignment(HAlign_Fill);
		PlaySlot->SetPadding(FMargin(34.0f, 0.0f, 34.0f, 12.0f));
	}

	QuitButton = MakeMenuButton(WidgetTree, TEXT("QuitButton"), TEXT("Wyjdź"));
	Layout->AddChildToVerticalBox(QuitButton);
	if (UVerticalBoxSlot* QuitSlot = Cast<UVerticalBoxSlot>(QuitButton->Slot))
	{
		QuitSlot->SetHorizontalAlignment(HAlign_Fill);
		QuitSlot->SetPadding(FMargin(34.0f, 0.0f, 34.0f, 34.0f));
	}
}

void UPoolMenuWidget::SetSubtitleText(const FString& InText)
{
	if (SubtitleText)
	{
		SubtitleText->SetText(FText::FromString(InText));
	}
}

void UPoolMenuWidget::HandlePlayClicked()
{
	OnPlayClicked.Broadcast();
}

void UPoolMenuWidget::HandleQuitClicked()
{
	OnQuitClicked.Broadcast();
}
