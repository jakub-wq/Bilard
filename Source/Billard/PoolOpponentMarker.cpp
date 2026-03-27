#include "PoolOpponentMarker.h"

#include "Components/TextRenderComponent.h"

APoolOpponentMarker::APoolOpponentMarker()
{
	PrimaryActorTick.bCanEverTick = false;

	TextComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TextComponent"));
	SetRootComponent(TextComponent);
	TextComponent->SetHorizontalAlignment(EHTA_Center);
	TextComponent->SetWorldSize(16.0f);
	TextComponent->SetText(FText::FromString(TEXT("Przeciwnik")));
	TextComponent->SetHiddenInGame(true);
}

void APoolOpponentMarker::SetMarkerText(const FString& InText)
{
	if (TextComponent)
	{
		TextComponent->SetText(FText::FromString(InText));
	}
}

void APoolOpponentMarker::SetMarkerColor(const FColor& InColor)
{
	if (TextComponent)
	{
		TextComponent->SetTextRenderColor(InColor);
	}
}

void APoolOpponentMarker::SetMarkerVisible(bool bVisible)
{
	SetActorHiddenInGame(!bVisible);
	SetActorEnableCollision(false);
	if (TextComponent)
	{
		TextComponent->SetHiddenInGame(!bVisible);
	}
}
