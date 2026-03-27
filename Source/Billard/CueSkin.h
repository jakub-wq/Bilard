#pragma once

#include "CoreMinimal.h"
#include "CueSkin.generated.h"

UENUM(BlueprintType)
enum class ECueSkin : uint8
{
	Standard UMETA(DisplayName = "Standardowy"),
	Blue UMETA(DisplayName = "Niebieski"),
	Red UMETA(DisplayName = "Czerwony"),
	Yellow UMETA(DisplayName = "Zolty")
};
