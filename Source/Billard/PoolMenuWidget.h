#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PoolMenuWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPoolMenuPlayClicked);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPoolMenuQuitClicked);

UCLASS()
class BILLARD_API UPoolMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	void SetSubtitleText(const FString& InText);

	UPROPERTY(BlueprintAssignable, Category = "Billiards")
	FPoolMenuPlayClicked OnPlayClicked;

	UPROPERTY(BlueprintAssignable, Category = "Billiards")
	FPoolMenuQuitClicked OnQuitClicked;

protected:
	void BuildWidgetTree();

	UFUNCTION()
	void HandlePlayClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UPROPERTY()
	UButton* PlayButton = nullptr;

	UPROPERTY()
	UButton* QuitButton = nullptr;

	UPROPERTY()
	UTextBlock* SubtitleText = nullptr;
};
