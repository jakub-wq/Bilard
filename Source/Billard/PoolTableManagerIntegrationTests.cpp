#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "PoolBall.h"
#include "PoolCushionWall.h"
#include "PoolPocketTrigger.h"
#include "PoolTableManager.h"

namespace PoolTableManagerIntegrationTests
{
	constexpr float Tolerance = 0.5f;

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

	APoolTableManager* SpawnTestManager(UWorld* World)
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

	FVector ToTableLocal(const APoolTableManager* Manager, const FVector& WorldLocation)
	{
		const FVector Relative = WorldLocation - Manager->GetSurfacePointForTests();
		return FVector(
			FVector::DotProduct(Relative, Manager->GetTableLongAxis()),
			FVector::DotProduct(Relative, Manager->GetTableShortAxis()),
			FVector::DotProduct(Relative, Manager->GetTableUpAxis()));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPoolTableManagerSpawnsExpectedActorsTest,
	"Billard.Integration.PoolTableManager.SpawnsExpectedActors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FPoolTableManagerSpawnsExpectedActorsTest::RunTest(const FString& Parameters)
{
	using namespace PoolTableManagerIntegrationTests;

	UWorld* World = GetEditorWorld();
	if (!TestNotNull(TEXT("Editor world should exist"), World))
	{
		return false;
	}

	APoolTableManager* Manager = SpawnTestManager(World);
	if (!TestNotNull(TEXT("PoolTableManager should spawn"), Manager))
	{
		return false;
	}

	TestEqual(TEXT("Manager should spawn 16 balls."), Manager->GetSpawnedBalls().Num(), 16);
	TestEqual(TEXT("Manager should spawn 6 pocket triggers."), Manager->GetPocketTriggersForTests().Num(), 6);
	TestEqual(TEXT("Manager should spawn 14 cushion wall segments including diagonal pocket jaws."), Manager->GetCushionWallsForTests().Num(), 14);
	TestNotNull(TEXT("Manager should create play surface floor collider."), Manager->GetPlaySurfaceFloorForTests());
	TestTrue(TEXT("Manager should report ready after ResetRack."), Manager->IsReady());

	CleanupManager(Manager);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPoolTableManagerPocketPlacementTest,
	"Billard.Integration.PoolTableManager.PocketPlacement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FPoolTableManagerPocketPlacementTest::RunTest(const FString& Parameters)
{
	using namespace PoolTableManagerIntegrationTests;

	APoolTableManager* Manager = SpawnTestManager(GetEditorWorld());
	if (!TestNotNull(TEXT("PoolTableManager should spawn"), Manager))
	{
		return false;
	}

	int32 CornerPocketCount = 0;
	int32 SidePocketCount = 0;
	float CornerRadiusSum = 0.0f;
	float SideRadiusSum = 0.0f;

	for (APoolPocketTrigger* Pocket : Manager->GetPocketTriggersForTests())
	{
		if (!TestNotNull(TEXT("Pocket trigger should be valid"), Pocket))
		{
			continue;
		}

		const FVector Local = ToTableLocal(Manager, Pocket->GetActorLocation());
		const USphereComponent* Sphere = Pocket->FindComponentByClass<USphereComponent>();
		AddInfo(FString::Printf(TEXT("Pocket local=%s"), *Local.ToCompactString()));

		TestTrue(TEXT("Pocket should stay inside play bounds along long axis."), FMath::Abs(Local.X) <= Manager->GetHalfPlayLengthForTests() + Tolerance);
		TestTrue(TEXT("Pocket should stay inside play bounds along short axis."), FMath::Abs(Local.Y) <= Manager->GetHalfPlayWidthForTests() + Tolerance);
		TestTrue(TEXT("Pocket trigger should sit slightly below the cloth surface."), Local.Z < 0.0f);

		const bool bCornerPocket = FMath::Abs(Local.X) > Manager->GetHalfPlayLengthForTests() * 0.45f;
		if (bCornerPocket)
		{
			++CornerPocketCount;
			CornerRadiusSum += Sphere ? Sphere->GetScaledSphereRadius() : 0.0f;
		}
		else
		{
			++SidePocketCount;
			SideRadiusSum += Sphere ? Sphere->GetScaledSphereRadius() : 0.0f;
		}
	}

	TestEqual(TEXT("There should be 4 corner pockets."), CornerPocketCount, 4);
	TestEqual(TEXT("There should be 2 side pockets."), SidePocketCount, 2);
	TestTrue(TEXT("Corner pockets should now be larger than side pockets."), CornerPocketCount > 0 && SidePocketCount > 0 && (CornerRadiusSum / CornerPocketCount) > (SideRadiusSum / SidePocketCount));

	CleanupManager(Manager);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPoolTableManagerWallLayoutTest,
	"Billard.Integration.PoolTableManager.WallLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FPoolTableManagerWallLayoutTest::RunTest(const FString& Parameters)
{
	using namespace PoolTableManagerIntegrationTests;

	APoolTableManager* Manager = SpawnTestManager(GetEditorWorld());
	if (!TestNotNull(TEXT("PoolTableManager should spawn"), Manager))
	{
		return false;
	}

	int32 StraightWallCount = 0;
	int32 DiagonalJawCount = 0;
	const float BallRadius = Manager->GetBallRadius();
	const float CollisionHalfLength = FMath::Max(BallRadius * 7.0f, Manager->GetHalfPlayLengthForTests() - Manager->GetCollisionInsetLongForTests());
	const float CollisionHalfWidth = FMath::Max(BallRadius * 5.0f, Manager->GetHalfPlayWidthForTests() - Manager->GetCollisionInsetShortForTests());
	const float Thickness = FMath::Max(5.2f, BallRadius * 2.1f);
	const float RailInset = FMath::Max(Thickness * 0.2f, BallRadius * 0.12f);
	const float ExpectedRailLong = FMath::Max(BallRadius * 6.0f, CollisionHalfLength - RailInset);
	const float ExpectedRailShort = FMath::Max(BallRadius * 4.0f, CollisionHalfWidth - RailInset);

	for (APoolCushionWall* Wall : Manager->GetCushionWallsForTests())
	{
		if (!TestNotNull(TEXT("Wall should be valid"), Wall))
		{
			continue;
		}

		UBoxComponent* Box = Wall->FindComponentByClass<UBoxComponent>();
		if (!TestNotNull(TEXT("Wall collision box should exist"), Box))
		{
			continue;
		}

		const FVector Forward = FVector::VectorPlaneProject(Wall->GetActorForwardVector(), Manager->GetTableUpAxis()).GetSafeNormal();
		const float AlongLong = FMath::Abs(FVector::DotProduct(Forward, Manager->GetTableLongAxis()));
		const float AlongShort = FMath::Abs(FVector::DotProduct(Forward, Manager->GetTableShortAxis()));
		const FVector LocalCenter = ToTableLocal(Manager, Wall->GetActorLocation());
		const FVector Extent = Box->GetUnscaledBoxExtent();
			AddInfo(FString::Printf(TEXT("Wall local=%s forwardLong=%.2f forwardShort=%.2f"), *LocalCenter.ToCompactString(), AlongLong, AlongShort));

			if (AlongLong > 0.92f || AlongShort > 0.92f)
			{
				++StraightWallCount;

			if (AlongLong > 0.92f)
			{
				TestTrue(
					TEXT("Długie bandy powinny siedzieć blisko zewnętrznej krawędzi pola gry, a nie za blisko środka stołu."),
					FMath::IsNearlyEqual(FMath::Abs(LocalCenter.Y), ExpectedRailShort, 1.25f));
				TestTrue(
					TEXT("Długa banda powinna mieć długość w osi swojego kierunku, a nie rozszerzać się za bardzo w głąb stołu."),
					Extent.X > Extent.Y);
			}
			else
			{
				TestTrue(
					TEXT("Krótkie bandy powinny siedzieć bliżej końca stołu, a nie zbyt daleko za widoczną krawędzią."),
					FMath::IsNearlyEqual(FMath::Abs(LocalCenter.X), ExpectedRailLong, 1.25f));
				TestTrue(
						TEXT("Krótka banda powinna mieć długość w osi krótkiej bandy, a nie być obróconym boksem wchodzącym w stół."),
						Extent.X > Extent.Y);
				}
			}
			else
			{
				++DiagonalJawCount;
				TestTrue(
					TEXT("Ukośna szczęka łuzy powinna mieć wyraźną składową wzdłuż długiej i krótkiej osi stołu."),
					AlongLong > 0.25f && AlongShort > 0.25f);
				TestTrue(
					TEXT("Ukośna szczęka powinna być dłuższa w osi swojego biegu niż w grubości collidera."),
					Extent.X > Extent.Y);
			}
		}

		TestEqual(TEXT("There should be 6 straight wall segments."), StraightWallCount, 6);
		TestEqual(TEXT("There should be 8 diagonal jaw segments around the pocket mouths."), DiagonalJawCount, 8);

		CleanupManager(Manager);
		return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPoolTableManagerBallSafeAreaTest,
	"Billard.Integration.PoolTableManager.BallSafeArea",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FPoolTableManagerBallSafeAreaTest::RunTest(const FString& Parameters)
{
	using namespace PoolTableManagerIntegrationTests;

	APoolTableManager* Manager = SpawnTestManager(GetEditorWorld());
	if (!TestNotNull(TEXT("PoolTableManager should spawn"), Manager))
	{
		return false;
	}

	const float BallRadius = Manager->GetBallRadius();
	const float SafeLongLimit = FMath::Max(BallRadius * 7.0f, Manager->GetHalfPlayLengthForTests() - Manager->GetCollisionInsetLongForTests()) - BallRadius * 1.05f;
	const float SafeShortLimit = FMath::Max(BallRadius * 5.0f, Manager->GetHalfPlayWidthForTests() - Manager->GetCollisionInsetShortForTests()) - BallRadius * 1.05f;
	const FVector CueStart = Manager->GetCueBallStartLocation();
	float CueStartNearestWallDistance = TNumericLimits<float>::Max();

	for (APoolBall* Ball : Manager->GetSpawnedBalls())
	{
		if (!TestNotNull(TEXT("Ball should be valid"), Ball))
		{
			continue;
		}

		const FVector Local = ToTableLocal(Manager, Ball->GetActorLocation());
		AddInfo(FString::Printf(TEXT("Ball %d local=%s"), Ball->GetBallNumber(), *Local.ToCompactString()));

		TestTrue(TEXT("Ball should spawn within safe long bounds."), FMath::Abs(Local.X) <= SafeLongLimit + Tolerance);
		TestTrue(TEXT("Ball should spawn within safe short bounds."), FMath::Abs(Local.Y) <= SafeShortLimit + Tolerance);
		TestTrue(TEXT("Ball should sit on the cloth plane."), FMath::Abs(Local.Z - BallRadius) <= 1.0f);

		for (APoolPocketTrigger* Pocket : Manager->GetPocketTriggersForTests())
		{
			USphereComponent* Sphere = Pocket ? Pocket->FindComponentByClass<USphereComponent>() : nullptr;
			if (!Sphere)
			{
				continue;
			}

			const float Clearance = FVector::Dist(Ball->GetActorLocation(), Sphere->GetComponentLocation()) - Sphere->GetScaledSphereRadius();
			TestTrue(TEXT("Ball should not spawn inside a pocket trigger."), Clearance > BallRadius * 0.35f);
		}

		for (APoolCushionWall* Wall : Manager->GetCushionWallsForTests())
		{
			UBoxComponent* Box = Wall ? Wall->FindComponentByClass<UBoxComponent>() : nullptr;
			if (!Box)
			{
				continue;
			}

			FVector ClosestPoint = FVector::ZeroVector;
			const float Distance = Box->GetClosestPointOnCollision(Ball->GetActorLocation(), ClosestPoint);
			if (Distance >= 0.0f)
			{
				TestTrue(TEXT("Ball should not spawn intersecting a cushion wall."), Distance > BallRadius * 0.35f);
			}

			const float CueDistance = Box->GetClosestPointOnCollision(CueStart, ClosestPoint);
			if (CueDistance >= 0.0f)
			{
				CueStartNearestWallDistance = FMath::Min(CueStartNearestWallDistance, CueDistance);
			}
		}
	}

	TestTrue(
		TEXT("W miejscu startu białej bili nie powinno być niewidzialnej ściany od collidera bandy."),
		CueStartNearestWallDistance > BallRadius * 3.0f);

	CleanupManager(Manager);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPoolTableManagerCornerPocketApproachClearanceTest,
	"Billard.Integration.PoolTableManager.CornerPocketApproachClearance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FPoolTableManagerCornerPocketApproachClearanceTest::RunTest(const FString& Parameters)
{
	using namespace PoolTableManagerIntegrationTests;

	APoolTableManager* Manager = SpawnTestManager(GetEditorWorld());
	if (!TestNotNull(TEXT("PoolTableManager should spawn"), Manager))
	{
		return false;
	}

	TArray<APoolPocketTrigger*> CornerPockets;
	for (APoolPocketTrigger* Pocket : Manager->GetPocketTriggersForTests())
	{
		if (!Pocket)
		{
			continue;
		}

		const FVector PocketOffset = Pocket->GetActorLocation() - Manager->GetSurfacePointForTests();
		const float AlongLong = FMath::Abs(FVector::DotProduct(PocketOffset, Manager->GetTableLongAxis()));
		if (AlongLong > Manager->GetHalfPlayLengthForTests() * 0.35f)
		{
			CornerPockets.Add(Pocket);
		}
	}

	if (!TestEqual(TEXT("There should be four corner pockets."), CornerPockets.Num(), 4))
	{
		CleanupManager(Manager);
		return false;
	}

	const float BallRadius = Manager->GetBallRadius();
	const float SampleRadius = BallRadius * 0.9f;
	const float ClearanceTolerance = BallRadius * 0.2f;
	constexpr int32 SampleCount = 24;

	for (int32 Index = 0; Index < CornerPockets.Num(); ++Index)
	{
		APoolPocketTrigger* CornerPocket = CornerPockets[Index];
		const FVector PocketLocation = CornerPocket->GetActorLocation();
		const FVector PocketOffset = PocketLocation - Manager->GetSurfacePointForTests();
		const float LongSign = FMath::Sign(FVector::DotProduct(PocketOffset, Manager->GetTableLongAxis()));
		const float ShortSign = FMath::Sign(FVector::DotProduct(PocketOffset, Manager->GetTableShortAxis()));

		const FVector Start =
			Manager->GetSurfacePointForTests()
			+ Manager->GetTableLongAxis() * (LongSign * (Manager->GetHalfPlayLengthForTests() - Manager->GetPocketRadius() * 2.55f))
			+ Manager->GetTableShortAxis() * (ShortSign * (Manager->GetHalfPlayWidthForTests() - Manager->GetPocketRadius() * 1.55f))
			+ Manager->GetTableUpAxis() * BallRadius;
		const FVector End =
			Manager->GetSurfacePointForTests()
			+ Manager->GetTableLongAxis() * (LongSign * (Manager->GetHalfPlayLengthForTests() - Manager->GetPocketRadius() * 0.75f))
			+ Manager->GetTableShortAxis() * (ShortSign * (Manager->GetHalfPlayWidthForTests() - Manager->GetPocketRadius() * 0.75f))
			+ Manager->GetTableUpAxis() * BallRadius;

		float MinWallDistance = TNumericLimits<float>::Max();

		for (int32 SampleIndex = 0; SampleIndex <= SampleCount; ++SampleIndex)
		{
			const float Alpha = static_cast<float>(SampleIndex) / static_cast<float>(SampleCount);
			const FVector SamplePoint = FMath::Lerp(Start, End, Alpha);

			for (APoolCushionWall* Wall : Manager->GetCushionWallsForTests())
			{
				UBoxComponent* Box = Wall ? Wall->FindComponentByClass<UBoxComponent>() : nullptr;
				if (!Box)
				{
					continue;
				}

				FVector ClosestPoint = FVector::ZeroVector;
				const float Distance = Box->GetClosestPointOnCollision(SamplePoint, ClosestPoint);
				if (Distance >= 0.0f)
				{
					MinWallDistance = FMath::Min(MinWallDistance, Distance);
				}
			}
		}

		AddInfo(FString::Printf(TEXT("Corner pocket %d min approach clearance=%.2f"), Index, MinWallDistance));
		TestTrue(
			FString::Printf(TEXT("Corner pocket %d should leave enough clearance for a ball-sized approach corridor."), Index),
			MinWallDistance + ClearanceTolerance >= SampleRadius);
	}

	CleanupManager(Manager);
	return true;
}

#endif
