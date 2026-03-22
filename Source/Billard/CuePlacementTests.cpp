#include "MyCharacter.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "PoolBall.h"
#include "PoolTableManager.h"

namespace CuePlacementTests
{
	constexpr float Tolerance = 0.01f;
	const FVector BallLocation(120.0f, -45.0f, 8.0f);
	const FVector ShotDirection = FVector(1.0f, 0.0f, 0.0f);
	const FVector UpAxis = FVector::UpVector;
	constexpr float CueDistanceFromBall = 34.0f;
	constexpr float CuePullbackDistance = 22.0f;
	constexpr float CurrentShotPower = 700.0f;
	constexpr float MaxShotPower = 1400.0f;
	constexpr float CueSideOffset = 35.0f;
	constexpr float CueHeightOffset = 1.0f;
	const FRotator CueAimRotationOffset(0.0f, 90.0f, -2.0f);

	UWorld* GetEditorWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}

		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::Editor && Context.World())
			{
				return Context.World();
			}
		}

		return nullptr;
	}

	APoolTableManager* SpawnManager(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		APoolTableManager* Manager = World->SpawnActor<APoolTableManager>(APoolTableManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
		if (Manager)
		{
			Manager->ResetRack();
		}
		return Manager;
	}

	void CleanupManager(APoolTableManager* Manager)
	{
		if (!Manager)
		{
			return;
		}

		AStaticMeshActor* TableActor = Manager->GetTableActorForTests();
		AStaticMeshActor* SpawnedVisualTable = Manager->GetSpawnedVisualTableForTests();
		Manager->CleanupSpawnedActorsForTests();
		if (TableActor && IsValid(TableActor) && TableActor == SpawnedVisualTable)
		{
			TableActor->Destroy();
		}
		Manager->Destroy();
	}

	void TickManagerActors(APoolTableManager* Manager, AMyCharacter* Character, float DeltaTime, int32 Steps)
	{
		if (!Manager)
		{
			return;
		}

		for (int32 Step = 0; Step < Steps; ++Step)
		{
			for (APoolBall* Ball : Manager->GetSpawnedBalls())
			{
				if (Ball)
				{
					Ball->Tick(DeltaTime);
				}
			}

			Manager->Tick(DeltaTime);

			if (Character)
			{
				Character->Tick(DeltaTime);
			}
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCuePlacementBaseOffsetsTest,
	"Billard.CuePlacement.BaseOffsets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FCuePlacementBaseOffsetsTest::RunTest(const FString& Parameters)
{
	using namespace CuePlacementTests;

	const FTransform CueTransform = AMyCharacter::CalculateCueVisualTransform(
		BallLocation,
		ShotDirection,
		UpAxis,
		CueDistanceFromBall,
		CuePullbackDistance,
		0.0f,
		MaxShotPower,
		CueSideOffset,
		CueHeightOffset,
		CueAimRotationOffset,
		false);

	const FVector Relative = CueTransform.GetLocation() - BallLocation;
	const FVector SideAxis = FVector::CrossProduct(UpAxis, ShotDirection).GetSafeNormal();

	TestTrue(TEXT("Kij powinien być za bilą, nie przed nią."), FVector::DotProduct(Relative, ShotDirection) < 0.0f);
	TestTrue(
		TEXT("Odsunięcie za bilą powinno zgadzać się z konfiguracją."),
		FMath::IsNearlyEqual(FVector::DotProduct(Relative, ShotDirection), -CueDistanceFromBall, Tolerance));
	TestTrue(
		TEXT("Boczny offset powinien zgadzać się z konfiguracją."),
		FMath::IsNearlyEqual(FVector::DotProduct(Relative, SideAxis), CueSideOffset, Tolerance));
	TestTrue(
		TEXT("Wysokość kija powinna zgadzać się z konfiguracją."),
		FMath::IsNearlyEqual(FVector::DotProduct(Relative, UpAxis), CueHeightOffset, Tolerance));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCuePlacementChargingPullbackTest,
	"Billard.CuePlacement.ChargingPullback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FCuePlacementChargingPullbackTest::RunTest(const FString& Parameters)
{
	using namespace CuePlacementTests;

	const FTransform IdleTransform = AMyCharacter::CalculateCueVisualTransform(
		BallLocation,
		ShotDirection,
		UpAxis,
		CueDistanceFromBall,
		CuePullbackDistance,
		0.0f,
		MaxShotPower,
		CueSideOffset,
		CueHeightOffset,
		CueAimRotationOffset,
		false);

	const FTransform ChargedTransform = AMyCharacter::CalculateCueVisualTransform(
		BallLocation,
		ShotDirection,
		UpAxis,
		CueDistanceFromBall,
		CuePullbackDistance,
		CurrentShotPower,
		MaxShotPower,
		CueSideOffset,
		CueHeightOffset,
		CueAimRotationOffset,
		true);

	const FVector Delta = ChargedTransform.GetLocation() - IdleTransform.GetLocation();
	const float ExpectedPullback = -(CuePullbackDistance * (CurrentShotPower / MaxShotPower));

	TestTrue(
		TEXT("Ładowanie strzału powinno cofać kij tylko wzdłuż osi strzału."),
		FMath::IsNearlyEqual(FVector::DotProduct(Delta, ShotDirection), ExpectedPullback, Tolerance));
	TestTrue(
		TEXT("Ładowanie nie powinno zmieniać wysokości kija."),
		FMath::IsNearlyZero(FVector::DotProduct(Delta, UpAxis), Tolerance));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCuePlacementRotationOffsetTest,
	"Billard.CuePlacement.RotationOffset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FCuePlacementRotationOffsetTest::RunTest(const FString& Parameters)
{
	using namespace CuePlacementTests;

	const FTransform CueTransform = AMyCharacter::CalculateCueVisualTransform(
		BallLocation,
		ShotDirection,
		UpAxis,
		CueDistanceFromBall,
		CuePullbackDistance,
		0.0f,
		MaxShotPower,
		CueSideOffset,
		CueHeightOffset,
		CueAimRotationOffset,
		false);

	const FRotator ExpectedRotation = FRotationMatrix::MakeFromX(-ShotDirection).Rotator() + CueAimRotationOffset;
	TestTrue(TEXT("Rotacja kija powinna zawierać offset modelu."), CueTransform.Rotator().Equals(ExpectedRotation, Tolerance));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCueMeshAssetBoundsTest,
	"Billard.CuePlacement.MeshAssetBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FCueMeshAssetBoundsTest::RunTest(const FString& Parameters)
{
	UStaticMesh* CueMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Billiards/Cues/CueStick_A_Source.CueStick_A_Source"));
	TestNotNull(TEXT("Powinno dać się załadować mesh kija."), CueMesh);
	if (!CueMesh)
	{
		return false;
	}

	const FBox LocalBounds = CueMesh->GetBoundingBox();
	const FVector Center = LocalBounds.GetCenter();
	const FVector Extent = LocalBounds.GetExtent();
	const float CrossSectionExtent = FMath::Max(Extent.X, Extent.Y);

	AddInfo(FString::Printf(TEXT("Cue mesh local bounds center: %s"), *Center.ToCompactString()));
	AddInfo(FString::Printf(TEXT("Cue mesh local bounds extent: %s"), *Extent.ToCompactString()));

	TestTrue(TEXT("Cue powinien być dużo dłuższy w jednej osi niż w przekroju."), Extent.Z > 8.0f * CrossSectionExtent);
	TestTrue(TEXT("Pivot powinien siedzieć blisko końcówki kija, a nie w jego środku."), FMath::Abs(Center.Z + Extent.Z) < 0.05f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCueRuntimeVisibilityTest,
	"Billard.CuePlacement.RuntimeVisibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FCueRuntimeVisibilityTest::RunTest(const FString& Parameters)
{
	using namespace CuePlacementTests;

	UWorld* World = GetEditorWorld();
	if (!TestNotNull(TEXT("Editor world should exist"), World))
	{
		return false;
	}

	APoolTableManager* Manager = SpawnManager(World);
	if (!TestNotNull(TEXT("PoolTableManager should spawn"), Manager))
	{
		return false;
	}

	FActorSpawnParameters CharacterSpawnParams;
	CharacterSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AMyCharacter* Character = World->SpawnActor<AMyCharacter>(AMyCharacter::StaticClass(), FVector(0.0f, 0.0f, 120.0f), FRotator::ZeroRotator, CharacterSpawnParams);
	if (!TestNotNull(TEXT("Character should spawn"), Character))
	{
		CleanupManager(Manager);
		return false;
	}

	APoolBall* CueBall = Manager->GetCueBall();
	if (!TestNotNull(TEXT("Cue ball should exist"), CueBall))
	{
		Character->Destroy();
		CleanupManager(Manager);
		return false;
	}

	Character->EnterAimModeForTests(CueBall);
	Character->Tick(0.016f);

	UStaticMeshComponent* CueComponent = Character->GetCueMeshForTests();
	if (!TestNotNull(TEXT("Cue mesh component should exist"), CueComponent))
	{
		Character->Destroy();
		CleanupManager(Manager);
		return false;
	}

	const UStaticMesh* ActiveMesh = CueComponent->GetStaticMesh();
	const FBox AssetBox = ActiveMesh ? ActiveMesh->GetBoundingBox() : FBox(EForceInit::ForceInitToZero);
	const float RuntimeCueLength = AssetBox.GetSize().GetMax() * CueComponent->GetComponentScale().GetMax();
	const float DistanceToCueBall = FVector::Dist(CueComponent->GetComponentLocation(), CueBall->GetActorLocation());
	const FVector CueBodyDirection = CueComponent->GetComponentTransform().TransformVectorNoScale(AssetBox.GetCenter()).GetSafeNormal();
	const FVector AimDirection = (Manager->GetRackCenterLocation() - CueBall->GetActorLocation()).GetSafeNormal2D();

	AddInfo(FString::Printf(TEXT("Cue runtime length=%.2f distance_to_ball=%.2f location=%s"), RuntimeCueLength, DistanceToCueBall, *CueComponent->GetComponentLocation().ToCompactString()));

	TestFalse(TEXT("Cue should not be hidden in aim mode."), CueComponent->bHiddenInGame);
	TestTrue(TEXT("Cue should be visible in aim mode."), CueComponent->IsVisible());
	TestTrue(TEXT("Cue should be scaled to a usable visible length."), RuntimeCueLength >= 90.0f);
	TestTrue(TEXT("Cue pivot should remain near the cue ball in aim mode."), DistanceToCueBall <= 20.0f);
	TestTrue(TEXT("Cue body should extend behind the cue ball instead of through the shot target."), FVector::DotProduct(CueBodyDirection, AimDirection) <= -0.25f);

	Character->ExitAimModeForTests();
	Character->Destroy();
	CleanupManager(Manager);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCueBallPlacementModeSyncTest,
	"Billard.CuePlacement.CueBallPlacementModeSync",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FCueBallPlacementModeSyncTest::RunTest(const FString& Parameters)
{
	using namespace CuePlacementTests;

	UWorld* World = GetEditorWorld();
	if (!TestNotNull(TEXT("Editor world should exist"), World))
	{
		return false;
	}

	APoolTableManager* Manager = SpawnManager(World);
	if (!TestNotNull(TEXT("PoolTableManager should spawn"), Manager))
	{
		return false;
	}

	FActorSpawnParameters CharacterSpawnParams;
	CharacterSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AMyCharacter* Character = World->SpawnActor<AMyCharacter>(AMyCharacter::StaticClass(), FVector(0.0f, 0.0f, 120.0f), FRotator::ZeroRotator, CharacterSpawnParams);
	if (!TestNotNull(TEXT("Character should spawn"), Character))
	{
		CleanupManager(Manager);
		return false;
	}

	APoolBall* CueBall = Manager->GetCueBall();
	if (!TestNotNull(TEXT("Cue ball should exist"), CueBall))
	{
		Character->Destroy();
		CleanupManager(Manager);
		return false;
	}

	Manager->HandleBallPocketed(CueBall, Manager->GetCueBallStartLocation());
	TickManagerActors(Manager, nullptr, 1.0f / 60.0f, 240);

	TestTrue(TEXT("Scratch should put the table manager into cue-ball-in-hand mode."), Manager->IsCueBallInHand());

	Character->Tick(0.016f);

	TestTrue(TEXT("Character should enter cue-ball placement mode when the manager exposes ball-in-hand."), Character->IsCueBallPlacementModeForTests());
	TestTrue(TEXT("Cue ball should respawn onto the table for placement."), FVector::Dist(CueBall->GetActorLocation(), Manager->GetCueBallInHandLocation()) < 1.0f);

	Character->Destroy();
	CleanupManager(Manager);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCueBallPlacementModeIgnoresStationarySpinTest,
	"Billard.CuePlacement.CueBallPlacementModeIgnoresStationarySpin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FCueBallPlacementModeIgnoresStationarySpinTest::RunTest(const FString& Parameters)
{
	using namespace CuePlacementTests;

	UWorld* World = GetEditorWorld();
	if (!TestNotNull(TEXT("Editor world should exist"), World))
	{
		return false;
	}

	APoolTableManager* Manager = SpawnManager(World);
	if (!TestNotNull(TEXT("PoolTableManager should spawn"), Manager))
	{
		return false;
	}

	FActorSpawnParameters CharacterSpawnParams;
	CharacterSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AMyCharacter* Character = World->SpawnActor<AMyCharacter>(AMyCharacter::StaticClass(), FVector(0.0f, 0.0f, 120.0f), FRotator::ZeroRotator, CharacterSpawnParams);
	if (!TestNotNull(TEXT("Character should spawn"), Character))
	{
		CleanupManager(Manager);
		return false;
	}

	APoolBall* CueBall = Manager->GetCueBall();
	if (!TestNotNull(TEXT("Cue ball should exist"), CueBall))
	{
		Character->Destroy();
		CleanupManager(Manager);
		return false;
	}

	APoolBall* ObjectBall = nullptr;
	for (APoolBall* Ball : Manager->GetSpawnedBalls())
	{
		if (Ball && !Ball->IsCueBall())
		{
			ObjectBall = Ball;
			break;
		}
	}

	if (!TestNotNull(TEXT("An object ball should exist"), ObjectBall))
	{
		Character->Destroy();
		CleanupManager(Manager);
		return false;
	}

	ObjectBall->SetLinearVelocity(FVector::ZeroVector);
	ObjectBall->SetAngularVelocityDegrees(FVector(0.0f, 0.0f, 90.0f));

	Manager->HandleBallPocketed(CueBall, Manager->GetCueBallStartLocation());
	TickManagerActors(Manager, Character, 1.0f / 60.0f, 240);

	TestTrue(TEXT("Spin-only object balls should not block cue-ball-in-hand mode after a scratch."), Manager->IsCueBallInHand());
	TestTrue(TEXT("Character should still enter cue-ball placement mode when another ball only spins in place."), Character->IsCueBallPlacementModeForTests());

	Character->Destroy();
	CleanupManager(Manager);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCueBallPlacementModeForcesAfterSettleTimeoutTest,
	"Billard.CuePlacement.CueBallPlacementModeForcesAfterSettleTimeout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FCueBallPlacementModeForcesAfterSettleTimeoutTest::RunTest(const FString& Parameters)
{
	using namespace CuePlacementTests;

	UWorld* World = GetEditorWorld();
	if (!TestNotNull(TEXT("Editor world should exist"), World))
	{
		return false;
	}

	APoolTableManager* Manager = SpawnManager(World);
	if (!TestNotNull(TEXT("PoolTableManager should spawn"), Manager))
	{
		return false;
	}

	FActorSpawnParameters CharacterSpawnParams;
	CharacterSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AMyCharacter* Character = World->SpawnActor<AMyCharacter>(AMyCharacter::StaticClass(), FVector(0.0f, 0.0f, 120.0f), FRotator::ZeroRotator, CharacterSpawnParams);
	if (!TestNotNull(TEXT("Character should spawn"), Character))
	{
		CleanupManager(Manager);
		return false;
	}

	APoolBall* CueBall = Manager->GetCueBall();
	if (!TestNotNull(TEXT("Cue ball should exist"), CueBall))
	{
		Character->Destroy();
		CleanupManager(Manager);
		return false;
	}

	APoolBall* ObjectBall = nullptr;
	for (APoolBall* Ball : Manager->GetSpawnedBalls())
	{
		if (Ball && !Ball->IsCueBall())
		{
			ObjectBall = Ball;
			break;
		}
	}

	if (!TestNotNull(TEXT("An object ball should exist"), ObjectBall))
	{
		Character->Destroy();
		CleanupManager(Manager);
		return false;
	}

	Manager->HandleBallPocketed(CueBall, Manager->GetCueBallStartLocation());

	const float DeltaTime = 1.0f / 60.0f;
	for (int32 Step = 0; Step < 240 && !Manager->IsCueBallInHand(); ++Step)
	{
		ObjectBall->SetLinearVelocity(FVector(4.0f, 0.0f, 0.0f));
		TickManagerActors(Manager, Character, DeltaTime, 1);
	}

	TestTrue(TEXT("Cue-ball-in-hand should still begin after the settle timeout even if another ball keeps reporting small motion."), Manager->IsCueBallInHand());
	TestTrue(TEXT("Character should enter placement mode after the forced settle timeout fallback."), Character->IsCueBallPlacementModeForTests());

	Character->Destroy();
	CleanupManager(Manager);
	return true;
}

#endif
