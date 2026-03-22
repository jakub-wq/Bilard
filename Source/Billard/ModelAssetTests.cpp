#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/StaticMesh.h"
#include "PoolTableManager.h"
#include "UObject/Field.h"
#include "UObject/UnrealType.h"

#include <type_traits>

namespace ModelAssetTests
{
	constexpr float Tolerance = 0.05f;

	template <typename ValueType>
	ValueType GetPropertyValueChecked(const UObject* Object, const TCHAR* PropertyName)
	{
		const FProperty* Property = Object ? Object->GetClass()->FindPropertyByName(PropertyName) : nullptr;
		check(Property);

		if constexpr (std::is_same_v<ValueType, float>)
		{
			const FFloatProperty* FloatProperty = CastFieldChecked<FFloatProperty>(Property);
			return FloatProperty->GetPropertyValue_InContainer(Object);
		}
		else if constexpr (std::is_same_v<ValueType, FVector>)
		{
			const FStructProperty* StructProperty = CastFieldChecked<FStructProperty>(Property);
			check(StructProperty->Struct == TBaseStructure<FVector>::Get());
			return *StructProperty->ContainerPtrToValuePtr<FVector>(Object);
		}
		else
		{
			static_assert(sizeof(ValueType) == 0, "Unsupported reflected property type.");
		}
	}

	TArray<FString> GetBallAssetPaths()
	{
		TArray<FString> AssetPaths;
		AssetPaths.Add(TEXT("/Game/Billiards/Balls/CueBall_Source.CueBall_Source"));
		for (int32 BallNumber = 1; BallNumber <= 15; ++BallNumber)
		{
			AssetPaths.Add(FString::Printf(TEXT("/Game/Billiards/Balls/Ball_%02d_Source.Ball_%02d_Source"), BallNumber, BallNumber));
		}
		return AssetPaths;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRequiredMeshesExistTest,
	"Billard.Models.RequiredMeshesExist",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FRequiredMeshesExistTest::RunTest(const FString& Parameters)
{
	const TArray<FString> RequiredAssets = {
		TEXT("/Game/Billard.Billard"),
		TEXT("/Game/Billiards/Cues/CueStick_A_Source.CueStick_A_Source"),
		TEXT("/Game/Billiards/Cues/CueStick_B_Source.CueStick_B_Source"),
	};

	for (const FString& AssetPath : RequiredAssets)
	{
		TestNotNull(*FString::Printf(TEXT("Missing required mesh %s"), *AssetPath), LoadObject<UStaticMesh>(nullptr, *AssetPath));
	}

	for (const FString& AssetPath : ModelAssetTests::GetBallAssetPaths())
	{
		TestNotNull(*FString::Printf(TEXT("Missing required ball mesh %s"), *AssetPath), LoadObject<UStaticMesh>(nullptr, *AssetPath));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBallMeshesAreCenteredAndRoundTest,
	"Billard.Models.BallMeshesAreCenteredAndRound",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FBallMeshesAreCenteredAndRoundTest::RunTest(const FString& Parameters)
{
	for (const FString& AssetPath : ModelAssetTests::GetBallAssetPaths())
	{
		const UStaticMesh* BallMesh = LoadObject<UStaticMesh>(nullptr, *AssetPath);
		if (!TestNotNull(*FString::Printf(TEXT("Could not load %s"), *AssetPath), BallMesh))
		{
			continue;
		}

		const FBox Box = BallMesh->GetBoundingBox();
		const FVector Center = Box.GetCenter();
		const FVector Extent = Box.GetExtent();
		const float Radius = Extent.GetMax();
		AddInfo(FString::Printf(TEXT("%s center=%s extent=%s"), *AssetPath, *Center.ToCompactString(), *Extent.ToCompactString()));

		TestTrue(
			*FString::Printf(TEXT("%s should be centered near local origin"), *AssetPath),
			Center.Size() <= Radius * 0.15f + ModelAssetTests::Tolerance);
		TestTrue(
			*FString::Printf(TEXT("%s should be close to spherical"), *AssetPath),
			FMath::Abs(Extent.X - Extent.Y) <= Radius * 0.1f + ModelAssetTests::Tolerance
				&& FMath::Abs(Extent.Y - Extent.Z) <= Radius * 0.1f + ModelAssetTests::Tolerance);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCueMeshesUseEndPivotTest,
	"Billard.Models.CueMeshesUseEndPivot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FCueMeshesUseEndPivotTest::RunTest(const FString& Parameters)
{
	for (const FString& AssetPath : {
		TEXT("/Game/Billiards/Cues/CueStick_A_Source.CueStick_A_Source"),
		TEXT("/Game/Billiards/Cues/CueStick_B_Source.CueStick_B_Source"),
	})
	{
		const UStaticMesh* CueMesh = LoadObject<UStaticMesh>(nullptr, AssetPath);
		if (!TestNotNull(*FString::Printf(TEXT("Could not load %s"), *AssetPath), CueMesh))
		{
			continue;
		}

		const FBox Box = CueMesh->GetBoundingBox();
		const FVector Center = Box.GetCenter();
		const FVector Extent = Box.GetExtent();
		const float CrossSectionExtent = FMath::Max(Extent.X, Extent.Y);
		AddInfo(FString::Printf(TEXT("%s center=%s extent=%s"), *AssetPath, *Center.ToCompactString(), *Extent.ToCompactString()));

		TestTrue(*FString::Printf(TEXT("%s should be long and slim"), *AssetPath), Extent.Z > 8.0f * CrossSectionExtent);
		TestTrue(*FString::Printf(TEXT("%s should keep pivot near one cue end"), *AssetPath), FMath::Abs(Center.Z + Extent.Z) < 0.05f);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTableMeshMatchesRuntimeTuningTest,
	"Billard.Models.TableMeshMatchesRuntimeTuning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FTableMeshMatchesRuntimeTuningTest::RunTest(const FString& Parameters)
{
	const UStaticMesh* TableMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Billard.Billard"));
	if (!TestNotNull(TEXT("Could not load table mesh /Game/Billard.Billard"), TableMesh))
	{
		return false;
	}

	const APoolTableManager* Defaults = GetDefault<APoolTableManager>();
	const FVector TableVisualScale = ModelAssetTests::GetPropertyValueChecked<FVector>(Defaults, TEXT("TableVisualScale"));
	const float ManualHalfOuterLength = ModelAssetTests::GetPropertyValueChecked<float>(Defaults, TEXT("ManualHalfOuterLength"));
	const float ManualHalfOuterWidth = ModelAssetTests::GetPropertyValueChecked<float>(Defaults, TEXT("ManualHalfOuterWidth"));
	const float ManualHalfPlayLength = ModelAssetTests::GetPropertyValueChecked<float>(Defaults, TEXT("ManualHalfPlayLength"));
	const float ManualHalfPlayWidth = ModelAssetTests::GetPropertyValueChecked<float>(Defaults, TEXT("ManualHalfPlayWidth"));
	const float CollisionInsetLong = ModelAssetTests::GetPropertyValueChecked<float>(Defaults, TEXT("CollisionInsetLong"));
	const float CollisionInsetShort = ModelAssetTests::GetPropertyValueChecked<float>(Defaults, TEXT("CollisionInsetShort"));
	const float BallRadius = ModelAssetTests::GetPropertyValueChecked<float>(Defaults, TEXT("ManualBallRadius"));

	const FVector MeshSize = TableMesh->GetBoundingBox().GetSize();
	const FVector ScaledHalfExtent = MeshSize * TableVisualScale.GetAbs() * 0.5f;
	const float MeshHalfLong = FMath::Max(ScaledHalfExtent.X, ScaledHalfExtent.Y);
	const float MeshHalfShort = FMath::Min(ScaledHalfExtent.X, ScaledHalfExtent.Y);
	const float SafeLongLimit = FMath::Max(BallRadius * 7.0f, ManualHalfPlayLength - CollisionInsetLong) - BallRadius * 1.05f;
	const float SafeShortLimit = FMath::Max(BallRadius * 5.0f, ManualHalfPlayWidth - CollisionInsetShort) - BallRadius * 1.05f;
	const float LongMargin = ManualHalfOuterLength - SafeLongLimit;
	const float ShortMargin = ManualHalfOuterWidth - SafeShortLimit;

	AddInfo(FString::Printf(
		TEXT("Table scaled half extents long=%.2f short=%.2f; configured outer long=%.2f short=%.2f; safe collision long=%.2f short=%.2f"),
		MeshHalfLong,
		MeshHalfShort,
		ManualHalfOuterLength,
		ManualHalfOuterWidth,
		SafeLongLimit,
		SafeShortLimit));

	TestTrue(TEXT("Configured table outer length should fit inside the visual mesh."), ManualHalfOuterLength <= MeshHalfLong + 5.0f);
	TestTrue(TEXT("Configured table outer width should fit inside the visual mesh."), ManualHalfOuterWidth <= MeshHalfShort + 5.0f);
	TestTrue(TEXT("Configured play area must stay inside the configured outer table bounds."), ManualHalfPlayLength < ManualHalfOuterLength && ManualHalfPlayWidth < ManualHalfOuterWidth);
	TestTrue(TEXT("Long-axis collision safe zone should leave meaningful cushion margin."), LongMargin >= BallRadius * 2.5f);
	TestTrue(TEXT("Short-axis collision safe zone should leave meaningful cushion margin."), ShortMargin >= BallRadius * 2.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTablePocketLayoutSanityTest,
	"Billard.Models.TablePocketLayoutSanity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FTablePocketLayoutSanityTest::RunTest(const FString& Parameters)
{
	const APoolTableManager* Defaults = GetDefault<APoolTableManager>();
	const float ManualHalfPlayLength = ModelAssetTests::GetPropertyValueChecked<float>(Defaults, TEXT("ManualHalfPlayLength"));
	const float ManualHalfPlayWidth = ModelAssetTests::GetPropertyValueChecked<float>(Defaults, TEXT("ManualHalfPlayWidth"));
	const float BallRadius = ModelAssetTests::GetPropertyValueChecked<float>(Defaults, TEXT("ManualBallRadius"));
	const float PocketRadiusMultiplier = ModelAssetTests::GetPropertyValueChecked<float>(Defaults, TEXT("PocketRadiusMultiplier"));

	const float PocketRadius = BallRadius * PocketRadiusMultiplier;
	const float PocketCenterInset = FMath::Max(PocketRadius * 0.2f, BallRadius * 0.45f);
	const float CornerLong = FMath::Max(0.0f, ManualHalfPlayLength - PocketCenterInset);
	const float CornerShort = FMath::Max(0.0f, ManualHalfPlayWidth - PocketCenterInset);
	const float MiddleShort = FMath::Max(0.0f, ManualHalfPlayWidth - FMath::Max(PocketRadius * 0.08f, BallRadius * 0.2f));

	AddInfo(FString::Printf(
		TEXT("Pocket radius=%.2f cornerLong=%.2f cornerShort=%.2f middleShort=%.2f"),
		PocketRadius,
		CornerLong,
		CornerShort,
		MiddleShort));

	TestTrue(TEXT("Pocket radius should be larger than the ball radius."), PocketRadius > BallRadius);
	TestTrue(TEXT("Corner pocket placement should remain inside the play rectangle."), CornerLong < ManualHalfPlayLength && CornerShort < ManualHalfPlayWidth);
	TestTrue(TEXT("Side pocket placement should remain inside the play rectangle."), MiddleShort < ManualHalfPlayWidth);
	return true;
}

#endif
