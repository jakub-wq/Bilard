#pragma once

#include "CoreMinimal.h"
#include "PoolMatchTypes.generated.h"

UENUM(BlueprintType)
enum class EPoolMatchMode : uint8
{
	Training UMETA(DisplayName = "Tryb treningowy"),
	LocalVersus UMETA(DisplayName = "Lokalna gra")
};

UENUM(BlueprintType)
enum class EPoolPlayerSide : uint8
{
	Blue UMETA(DisplayName = "Niebieski"),
	Red UMETA(DisplayName = "Czerwony")
};

UENUM(BlueprintType)
enum class EPoolBallGroup : uint8
{
	Unassigned UMETA(DisplayName = "Nieprzypisane"),
	Yellow UMETA(DisplayName = "Zolte"),
	Red UMETA(DisplayName = "Czerwone"),
	Black UMETA(DisplayName = "Czarna")
};
