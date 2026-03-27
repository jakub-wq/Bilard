#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PoolOpponentMarker.generated.h"

class UTextRenderComponent;

UCLASS()
class BILLARD_API APoolOpponentMarker : public AActor
{
	GENERATED_BODY()

public:
	APoolOpponentMarker();

	void SetMarkerText(const FString& InText);
	void SetMarkerColor(const FColor& InColor);
	void SetMarkerVisible(bool bVisible);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UTextRenderComponent* TextComponent = nullptr;
};
