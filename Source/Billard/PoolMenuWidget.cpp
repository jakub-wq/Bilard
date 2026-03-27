#include "PoolMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/SlateColor.h"
#include "Styling/SlateTypes.h"

namespace
{
	UButton* MakeMenuButton(
		UWidgetTree* WidgetTree,
		const TCHAR* Name,
		const TCHAR* Label,
		const FLinearColor& NormalTint,
		UTextBlock*& OutLabel)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		FButtonStyle ButtonStyle = Button->GetStyle();
		ButtonStyle.Normal.TintColor = FSlateColor(NormalTint);
		ButtonStyle.Hovered.TintColor = FSlateColor(NormalTint + FLinearColor(0.06f, 0.10f, 0.06f, 0.03f));
		ButtonStyle.Pressed.TintColor = FSlateColor(NormalTint + FLinearColor(0.02f, 0.06f, 0.04f, 0.03f));
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

		OutLabel = Text;
		return Button;
	}

	void UpdateButtonHighlight(UButton* Button, UTextBlock* Label, const FLinearColor& BaseColor, bool bSelected)
	{
		if (!Button || !Label)
		{
			return;
		}

		const FLinearColor HighlightColor = bSelected ? BaseColor + FLinearColor(0.18f, 0.18f, 0.18f, 0.04f) : BaseColor;
		FButtonStyle ButtonStyle = Button->GetStyle();
		ButtonStyle.Normal.TintColor = FSlateColor(HighlightColor);
		ButtonStyle.Hovered.TintColor = FSlateColor(HighlightColor + FLinearColor(0.05f, 0.08f, 0.05f, 0.03f));
		ButtonStyle.Pressed.TintColor = FSlateColor(HighlightColor + FLinearColor(0.02f, 0.04f, 0.02f, 0.03f));
		Button->SetStyle(ButtonStyle);
		Label->SetColorAndOpacity(FSlateColor(bSelected ? FLinearColor(1.0f, 0.98f, 0.84f) : FLinearColor::White));
	}

	FString GetCueSkinLabel(ECueSkin Skin)
	{
		switch (Skin)
		{
		case ECueSkin::Blue:
			return TEXT("Niebieski");
		case ECueSkin::Red:
			return TEXT("Czerwony");
		case ECueSkin::Yellow:
			return TEXT("Żółty");
		case ECueSkin::Standard:
		default:
			return TEXT("Standardowy");
		}
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

	if (SettingsButton)
	{
		SettingsButton->OnClicked.RemoveDynamic(this, &UPoolMenuWidget::HandleSettingsClicked);
		SettingsButton->OnClicked.AddDynamic(this, &UPoolMenuWidget::HandleSettingsClicked);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.RemoveDynamic(this, &UPoolMenuWidget::HandleQuitClicked);
		QuitButton->OnClicked.AddDynamic(this, &UPoolMenuWidget::HandleQuitClicked);
	}

	if (BackButton)
	{
		BackButton->OnClicked.RemoveDynamic(this, &UPoolMenuWidget::HandleBackClicked);
		BackButton->OnClicked.AddDynamic(this, &UPoolMenuWidget::HandleBackClicked);
	}

	if (StandardSkinButton)
	{
		StandardSkinButton->OnClicked.RemoveDynamic(this, &UPoolMenuWidget::HandleStandardSkinClicked);
		StandardSkinButton->OnClicked.AddDynamic(this, &UPoolMenuWidget::HandleStandardSkinClicked);
	}

	if (BlueSkinButton)
	{
		BlueSkinButton->OnClicked.RemoveDynamic(this, &UPoolMenuWidget::HandleBlueSkinClicked);
		BlueSkinButton->OnClicked.AddDynamic(this, &UPoolMenuWidget::HandleBlueSkinClicked);
	}

	if (RedSkinButton)
	{
		RedSkinButton->OnClicked.RemoveDynamic(this, &UPoolMenuWidget::HandleRedSkinClicked);
		RedSkinButton->OnClicked.AddDynamic(this, &UPoolMenuWidget::HandleRedSkinClicked);
	}

	if (YellowSkinButton)
	{
		YellowSkinButton->OnClicked.RemoveDynamic(this, &UPoolMenuWidget::HandleYellowSkinClicked);
		YellowSkinButton->OnClicked.AddDynamic(this, &UPoolMenuWidget::HandleYellowSkinClicked);
	}

	ShowMainMenu();
	UpdateSkinSelectionVisuals();
}

void UPoolMenuWidget::BuildWidgetTree()
{
	PlayButton = nullptr;
	SettingsButton = nullptr;
	QuitButton = nullptr;
	BackButton = nullptr;
	StandardSkinButton = nullptr;
	BlueSkinButton = nullptr;
	RedSkinButton = nullptr;
	YellowSkinButton = nullptr;
	SubtitleText = nullptr;
	StandardSkinLabel = nullptr;
	BlueSkinLabel = nullptr;
	RedSkinLabel = nullptr;
	YellowSkinLabel = nullptr;
	MainMenuPanel = nullptr;
	SettingsPanel = nullptr;

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

	MainMenuPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainMenuPanel"));
	Layout->AddChildToVerticalBox(MainMenuPanel);

	UTextBlock* PlayLabel = nullptr;
	PlayButton = MakeMenuButton(
		WidgetTree,
		TEXT("PlayButton"),
		TEXT("Graj"),
		FLinearColor(0.08f, 0.12f, 0.10f, 0.94f),
		PlayLabel);
	MainMenuPanel->AddChild(PlayButton);
	if (UVerticalBoxSlot* PlaySlot = Cast<UVerticalBoxSlot>(PlayButton->Slot))
	{
		PlaySlot->SetHorizontalAlignment(HAlign_Fill);
		PlaySlot->SetPadding(FMargin(34.0f, 0.0f, 34.0f, 12.0f));
	}

	UTextBlock* SettingsLabel = nullptr;
	SettingsButton = MakeMenuButton(
		WidgetTree,
		TEXT("SettingsButton"),
		TEXT("Ustawienia"),
		FLinearColor(0.09f, 0.10f, 0.15f, 0.94f),
		SettingsLabel);
	MainMenuPanel->AddChild(SettingsButton);
	if (UVerticalBoxSlot* SettingsSlot = Cast<UVerticalBoxSlot>(SettingsButton->Slot))
	{
		SettingsSlot->SetHorizontalAlignment(HAlign_Fill);
		SettingsSlot->SetPadding(FMargin(34.0f, 0.0f, 34.0f, 12.0f));
	}

	UTextBlock* QuitLabel = nullptr;
	QuitButton = MakeMenuButton(
		WidgetTree,
		TEXT("QuitButton"),
		TEXT("Wyjdź"),
		FLinearColor(0.14f, 0.08f, 0.08f, 0.94f),
		QuitLabel);
	MainMenuPanel->AddChild(QuitButton);
	if (UVerticalBoxSlot* QuitSlot = Cast<UVerticalBoxSlot>(QuitButton->Slot))
	{
		QuitSlot->SetHorizontalAlignment(HAlign_Fill);
		QuitSlot->SetPadding(FMargin(34.0f, 0.0f, 34.0f, 34.0f));
	}

	SettingsPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsPanel"));
	Layout->AddChildToVerticalBox(SettingsPanel);

	UTextBlock* SettingsTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingsTitle"));
	SettingsTitle->SetText(FText::FromString(TEXT("Skórka kija")));
	SettingsTitle->SetJustification(ETextJustify::Center);
	SettingsTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.95f, 0.92f)));
	SettingsPanel->AddChild(SettingsTitle);
	if (UVerticalBoxSlot* SettingsTitleSlot = Cast<UVerticalBoxSlot>(SettingsTitle->Slot))
	{
		SettingsTitleSlot->SetHorizontalAlignment(HAlign_Fill);
		SettingsTitleSlot->SetPadding(FMargin(34.0f, 8.0f, 34.0f, 10.0f));
	}

	StandardSkinButton = MakeMenuButton(
		WidgetTree,
		TEXT("StandardSkinButton"),
		TEXT("Standardowy"),
		FLinearColor(0.25f, 0.17f, 0.10f, 0.94f),
		StandardSkinLabel);
	SettingsPanel->AddChild(StandardSkinButton);
	if (UVerticalBoxSlot* SkinSlot = Cast<UVerticalBoxSlot>(StandardSkinButton->Slot))
	{
		SkinSlot->SetHorizontalAlignment(HAlign_Fill);
		SkinSlot->SetPadding(FMargin(34.0f, 0.0f, 34.0f, 10.0f));
	}

	BlueSkinButton = MakeMenuButton(
		WidgetTree,
		TEXT("BlueSkinButton"),
		TEXT("Niebieski"),
		FLinearColor(0.10f, 0.18f, 0.34f, 0.94f),
		BlueSkinLabel);
	SettingsPanel->AddChild(BlueSkinButton);
	if (UVerticalBoxSlot* SkinSlot = Cast<UVerticalBoxSlot>(BlueSkinButton->Slot))
	{
		SkinSlot->SetHorizontalAlignment(HAlign_Fill);
		SkinSlot->SetPadding(FMargin(34.0f, 0.0f, 34.0f, 10.0f));
	}

	RedSkinButton = MakeMenuButton(
		WidgetTree,
		TEXT("RedSkinButton"),
		TEXT("Czerwony"),
		FLinearColor(0.34f, 0.10f, 0.10f, 0.94f),
		RedSkinLabel);
	SettingsPanel->AddChild(RedSkinButton);
	if (UVerticalBoxSlot* SkinSlot = Cast<UVerticalBoxSlot>(RedSkinButton->Slot))
	{
		SkinSlot->SetHorizontalAlignment(HAlign_Fill);
		SkinSlot->SetPadding(FMargin(34.0f, 0.0f, 34.0f, 10.0f));
	}

	YellowSkinButton = MakeMenuButton(
		WidgetTree,
		TEXT("YellowSkinButton"),
		TEXT("Żółty"),
		FLinearColor(0.35f, 0.29f, 0.08f, 0.94f),
		YellowSkinLabel);
	SettingsPanel->AddChild(YellowSkinButton);
	if (UVerticalBoxSlot* SkinSlot = Cast<UVerticalBoxSlot>(YellowSkinButton->Slot))
	{
		SkinSlot->SetHorizontalAlignment(HAlign_Fill);
		SkinSlot->SetPadding(FMargin(34.0f, 0.0f, 34.0f, 10.0f));
	}

	UTextBlock* BackLabel = nullptr;
	BackButton = MakeMenuButton(
		WidgetTree,
		TEXT("BackButton"),
		TEXT("Wróć"),
		FLinearColor(0.09f, 0.10f, 0.15f, 0.94f),
		BackLabel);
	SettingsPanel->AddChild(BackButton);
	if (UVerticalBoxSlot* BackSlot = Cast<UVerticalBoxSlot>(BackButton->Slot))
	{
		BackSlot->SetHorizontalAlignment(HAlign_Fill);
		BackSlot->SetPadding(FMargin(34.0f, 6.0f, 34.0f, 34.0f));
	}
}

void UPoolMenuWidget::SetSubtitleText(const FString& InText)
{
	MainSubtitleText = InText;

	if (SubtitleText)
	{
		SubtitleText->SetText(FText::FromString(bSettingsVisible ? FString::Printf(TEXT("Aktualna skórka kija: %s"), *GetCueSkinLabel(SelectedCueSkin)) : MainSubtitleText));
	}
}

void UPoolMenuWidget::SetSelectedCueSkin(ECueSkin InSkin)
{
	SelectedCueSkin = InSkin;
	UpdateSkinSelectionVisuals();
}

void UPoolMenuWidget::HandlePlayClicked()
{
	OnPlayClicked.Broadcast();
}

void UPoolMenuWidget::HandleQuitClicked()
{
	OnQuitClicked.Broadcast();
}

void UPoolMenuWidget::HandleSettingsClicked()
{
	ShowSettingsMenu();
}

void UPoolMenuWidget::HandleBackClicked()
{
	ShowMainMenu();
}

void UPoolMenuWidget::HandleStandardSkinClicked()
{
	SetSelectedCueSkin(ECueSkin::Standard);
	OnCueSkinSelected.Broadcast(ECueSkin::Standard);
}

void UPoolMenuWidget::HandleBlueSkinClicked()
{
	SetSelectedCueSkin(ECueSkin::Blue);
	OnCueSkinSelected.Broadcast(ECueSkin::Blue);
}

void UPoolMenuWidget::HandleRedSkinClicked()
{
	SetSelectedCueSkin(ECueSkin::Red);
	OnCueSkinSelected.Broadcast(ECueSkin::Red);
}

void UPoolMenuWidget::HandleYellowSkinClicked()
{
	SetSelectedCueSkin(ECueSkin::Yellow);
	OnCueSkinSelected.Broadcast(ECueSkin::Yellow);
}

void UPoolMenuWidget::UpdateSkinSelectionVisuals()
{
	UpdateButtonHighlight(StandardSkinButton, StandardSkinLabel, FLinearColor(0.25f, 0.17f, 0.10f, 0.94f), SelectedCueSkin == ECueSkin::Standard);
	UpdateButtonHighlight(BlueSkinButton, BlueSkinLabel, FLinearColor(0.10f, 0.18f, 0.34f, 0.94f), SelectedCueSkin == ECueSkin::Blue);
	UpdateButtonHighlight(RedSkinButton, RedSkinLabel, FLinearColor(0.34f, 0.10f, 0.10f, 0.94f), SelectedCueSkin == ECueSkin::Red);
	UpdateButtonHighlight(YellowSkinButton, YellowSkinLabel, FLinearColor(0.35f, 0.29f, 0.08f, 0.94f), SelectedCueSkin == ECueSkin::Yellow);

	if (SubtitleText && bSettingsVisible)
	{
		SubtitleText->SetText(FText::FromString(FString::Printf(TEXT("Aktualna skórka kija: %s"), *GetCueSkinLabel(SelectedCueSkin))));
	}
}

void UPoolMenuWidget::ShowMainMenu()
{
	bSettingsVisible = false;

	if (MainMenuPanel)
	{
		MainMenuPanel->SetVisibility(ESlateVisibility::Visible);
	}

	if (SettingsPanel)
	{
		SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (SubtitleText)
	{
		SubtitleText->SetText(FText::FromString(MainSubtitleText));
	}
}

void UPoolMenuWidget::ShowSettingsMenu()
{
	bSettingsVisible = true;

	if (MainMenuPanel)
	{
		MainMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (SettingsPanel)
	{
		SettingsPanel->SetVisibility(ESlateVisibility::Visible);
	}

	UpdateSkinSelectionVisuals();
}
