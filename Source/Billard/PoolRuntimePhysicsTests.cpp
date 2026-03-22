#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "PoolBall.h"
#include "PoolCushionWall.h"
#include "PoolPocketTrigger.h"
#include "PoolTableManager.h"

	namespace PoolRuntimePhysicsTests
	{
		constexpr float TickStep = 1.0f / 120.0f;

		struct FCornerShotScenario
		{
			const TCHAR* Name = TEXT("");
			float AlongLongFromRail = 1.0f;
			float AlongShortFromRail = 1.0f;
			FVector2D AimWeights = FVector2D(1.0f, 1.0f);
			float Speed = 180.0f;
			float SimulationDuration = 0.75f;
			float AllowedReverseSpeed = 95.0f;
		};

		struct FShotObservation
		{
			float MinPocketDistance = TNumericLimits<float>::Max();
			float MaxReverseSpeed = 0.0f;
			bool bReachedPocketMouth = false;
			bool bBecamePocketed = false;
			bool bEnteredPocketTrigger = false;
			float MinTriggerClearance = TNumericLimits<float>::Max();
			float MinWallClearance = TNumericLimits<float>::Max();
			FString TraceSummary;
		};

		struct FShotTraceSample
		{
			int32 StepIndex = 0;
			FVector Location = FVector::ZeroVector;
			FVector Velocity = FVector::ZeroVector;
			float PocketDistance = TNumericLimits<float>::Max();
			float TriggerClearance = TNumericLimits<float>::Max();
			float WallClearance = TNumericLimits<float>::Max();
		};

		UWorld* CreateTestWorld()
		{
			if (!GEngine)
			{
				return nullptr;
			}

			static int32 RuntimeWorldCounter = 0;
			const FString WorldNameString = FString::Printf(TEXT("AutomationRuntimeWorld_%d"), RuntimeWorldCounter++);
			const FName WorldName(*WorldNameString);
			UPackage* WorldPackage = CreatePackage(*FString::Printf(TEXT("/Temp/%s"), *WorldNameString));
			const UWorld::InitializationValues IVS = UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(true)
				.EnableTraceCollision(true)
				.CreatePhysicsScene(true);
			UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false, WorldName, WorldPackage, false, ERHIFeatureLevel::Num, &IVS, false);
			if (!TestWorld)
			{
				return nullptr;
			}

			TestWorld->SetShouldTick(true);
			TestWorld->AddToRoot();

			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(TestWorld);

			FURL URL;
			TestWorld->InitializeActorsForPlay(URL);
			TestWorld->BeginPlay();
			return TestWorld;
		}

		void DestroyTestWorld(UWorld* World)
		{
			if (!World || !GEngine)
			{
				return;
			}

			if (World->HasBegunPlay())
			{
				World->BeginTearingDown();
				World->EndPlay(EEndPlayReason::Quit);
			}

			World->RemoveFromRoot();
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

	void TickWorld(UWorld* World, float Duration)
	{
		if (!World)
		{
			return;
		}

		const int32 StepCount = FMath::CeilToInt(Duration / TickStep);
		for (int32 Index = 0; Index < StepCount; ++Index)
		{
			World->Tick(LEVELTICK_All, TickStep);

			for (TActorIterator<AActor> It(World); It; ++It)
			{
				AActor* Actor = *It;
				if (!IsValid(Actor) || !Actor->PrimaryActorTick.bCanEverTick)
				{
					continue;
				}

				Actor->Tick(TickStep);
			}
		}
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

	TArray<APoolPocketTrigger*> GetCornerPockets(APoolTableManager* Manager)
	{
		TArray<APoolPocketTrigger*> CornerPockets;
		if (!Manager)
		{
			return CornerPockets;
		}

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

		CornerPockets.Sort([Manager](const APoolPocketTrigger& A, const APoolPocketTrigger& B)
		{
			const FVector SurfacePoint = Manager->GetSurfacePointForTests();
			const FVector OffsetA = A.GetActorLocation() - SurfacePoint;
			const FVector OffsetB = B.GetActorLocation() - SurfacePoint;
			const float LongA = FVector::DotProduct(OffsetA, Manager->GetTableLongAxis());
			const float LongB = FVector::DotProduct(OffsetB, Manager->GetTableLongAxis());
			if (!FMath::IsNearlyEqual(LongA, LongB, 0.1f))
			{
				return LongA < LongB;
			}

			const float ShortA = FVector::DotProduct(OffsetA, Manager->GetTableShortAxis());
			const float ShortB = FVector::DotProduct(OffsetB, Manager->GetTableShortAxis());
			return ShortA < ShortB;
		});

		return CornerPockets;
	}

	FVector MakeCornerShotSpawnLocation(APoolTableManager* Manager, APoolPocketTrigger* CornerPocket, const FCornerShotScenario& Scenario)
	{
		const FVector PocketOffset = CornerPocket->GetActorLocation() - Manager->GetSurfacePointForTests();
		const float LongSign = FMath::Sign(FVector::DotProduct(PocketOffset, Manager->GetTableLongAxis()));
		const float ShortSign = FMath::Sign(FVector::DotProduct(PocketOffset, Manager->GetTableShortAxis()));
		return
			Manager->GetSurfacePointForTests()
			+ Manager->GetTableLongAxis() * (LongSign * (Manager->GetHalfPlayLengthForTests() - Manager->GetPocketRadius() * Scenario.AlongLongFromRail))
			+ Manager->GetTableShortAxis() * (ShortSign * (Manager->GetHalfPlayWidthForTests() - Manager->GetPocketRadius() * Scenario.AlongShortFromRail))
			+ Manager->GetTableUpAxis() * Manager->GetBallRadius();
	}

	FVector MakeCornerShotDirection(APoolTableManager* Manager, APoolPocketTrigger* CornerPocket, const FCornerShotScenario& Scenario)
	{
		const FVector PocketOffset = CornerPocket->GetActorLocation() - Manager->GetSurfacePointForTests();
		const float LongSign = FMath::Sign(FVector::DotProduct(PocketOffset, Manager->GetTableLongAxis()));
		const float ShortSign = FMath::Sign(FVector::DotProduct(PocketOffset, Manager->GetTableShortAxis()));
		return (
			Manager->GetTableLongAxis() * LongSign * Scenario.AimWeights.X
			+ Manager->GetTableShortAxis() * ShortSign * Scenario.AimWeights.Y).GetSafeNormal();
	}

	FShotObservation SimulateCornerShot(UWorld* World, APoolTableManager* Manager, APoolBall* Ball, APoolPocketTrigger* CornerPocket, const FCornerShotScenario& Scenario)
	{
		FShotObservation Observation;
		if (!World || !Manager || !Ball || !CornerPocket)
		{
			return Observation;
		}

		const FVector SpawnLocation = MakeCornerShotSpawnLocation(Manager, CornerPocket, Scenario);
		const FVector ShotDirection = MakeCornerShotDirection(Manager, CornerPocket, Scenario);
		const FVector PocketLocation = CornerPocket->GetActorLocation();
		const USphereComponent* TriggerSphere = CornerPocket->FindComponentByClass<USphereComponent>();
		TArray<FShotTraceSample> TraceSamples;

		Ball->ResetBall(FTransform(FRotator::ZeroRotator, SpawnLocation, FVector(1.0f)));
		Ball->SetAngularVelocityDegrees(FVector::ZeroVector);
		Ball->SetLinearVelocity(ShotDirection * Scenario.Speed);

		const int32 StepCount = FMath::CeilToInt(Scenario.SimulationDuration / TickStep);
		for (int32 StepIndex = 0; StepIndex < StepCount; ++StepIndex)
		{
			TickWorld(World, TickStep);

			const FVector ToPocket = FVector::VectorPlaneProject(PocketLocation - Ball->GetActorLocation(), Manager->GetTableUpAxis());
			const float PocketDistance = ToPocket.Size();
			Observation.MinPocketDistance = FMath::Min(Observation.MinPocketDistance, PocketDistance);
			Observation.bReachedPocketMouth |= PocketDistance <= Manager->GetPocketRadius() * 1.2f;

			const FVector PlanarVelocity = FVector::VectorPlaneProject(Ball->GetLinearVelocity(), Manager->GetTableUpAxis());
			const float ReverseSpeed = -FVector::DotProduct(PlanarVelocity, ShotDirection);
			Observation.MaxReverseSpeed = FMath::Max(Observation.MaxReverseSpeed, ReverseSpeed);

			float TriggerClearance = TNumericLimits<float>::Max();
			if (TriggerSphere)
			{
				TriggerClearance = FVector::Dist(Ball->GetActorLocation(), TriggerSphere->GetComponentLocation())
					- (TriggerSphere->GetScaledSphereRadius() + Ball->GetBallRadius());
				Observation.MinTriggerClearance = FMath::Min(Observation.MinTriggerClearance, TriggerClearance);
				Observation.bEnteredPocketTrigger |= TriggerClearance <= 0.0f;
			}

			float ClosestWallClearance = TNumericLimits<float>::Max();
			for (const APoolCushionWall* Wall : Manager->GetCushionWallsForTests())
			{
				if (!Wall)
				{
					continue;
				}

				const UBoxComponent* Box = Wall->FindComponentByClass<UBoxComponent>();
				if (!Box)
				{
					continue;
				}

				FVector ClosestPoint = Ball->GetActorLocation();
				Box->GetClosestPointOnCollision(Ball->GetActorLocation(), ClosestPoint);
				const float WallClearance = FVector::Dist(Ball->GetActorLocation(), ClosestPoint) - Ball->GetBallRadius();
				ClosestWallClearance = FMath::Min(ClosestWallClearance, WallClearance);
			}
			Observation.MinWallClearance = FMath::Min(Observation.MinWallClearance, ClosestWallClearance);

			FShotTraceSample& Sample = TraceSamples.Emplace_GetRef();
			Sample.StepIndex = StepIndex;
			Sample.Location = Ball->GetActorLocation();
			Sample.Velocity = Ball->GetLinearVelocity();
			Sample.PocketDistance = PocketDistance;
			Sample.TriggerClearance = TriggerClearance;
			Sample.WallClearance = ClosestWallClearance;
			if (TraceSamples.Num() > 8)
			{
				TraceSamples.RemoveAt(0);
			}

			if (Ball->IsPocketed() || Ball->IsSinkingIntoPocket() || Ball->IsHidden())
			{
				Observation.bBecamePocketed = true;
				break;
			}
		}

		TArray<FString> SampleStrings;
		for (const FShotTraceSample& Sample : TraceSamples)
		{
			SampleStrings.Add(FString::Printf(
				TEXT("step=%d loc=%s vel=%s pocket=%.2f trigger=%.2f wall=%.2f"),
				Sample.StepIndex,
				*Sample.Location.ToCompactString(),
				*Sample.Velocity.ToCompactString(),
				Sample.PocketDistance,
				Sample.TriggerClearance,
				Sample.WallClearance));
		}
		Observation.TraceSummary = FString::Join(SampleStrings, TEXT(" | "));

		return Observation;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPocketCaptureRuntimeTest,
	"Billard.Runtime.PocketCapture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FPocketCaptureRuntimeTest::RunTest(const FString& Parameters)
{
	using namespace PoolRuntimePhysicsTests;

	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Runtime test world should exist"), World))
	{
		return false;
	}

	APoolTableManager* Manager = SpawnManager(World);
	if (!TestNotNull(TEXT("PoolTableManager should spawn"), Manager))
	{
		return false;
	}

	APoolBall* TestBall = nullptr;
	for (APoolBall* Ball : Manager->GetSpawnedBalls())
	{
		if (Ball && !Ball->IsCueBall())
		{
			TestBall = Ball;
			break;
		}
	}

	if (!TestNotNull(TEXT("Object ball should exist"), TestBall))
	{
		CleanupManager(Manager);
		return false;
	}

	const FVector SpawnLocation =
		Manager->GetSurfacePointForTests()
		+ Manager->GetTableLongAxis() * (Manager->GetHalfOuterLengthForTests() + Manager->GetBallRadius() * 0.8f)
		+ Manager->GetTableShortAxis() * (Manager->GetHalfOuterWidthForTests() - Manager->GetBallRadius() * 1.8f)
		+ Manager->GetTableUpAxis() * Manager->GetBallRadius();

	TestBall->TeleportBall(SpawnLocation);
	TestBall->SetLinearVelocity(FVector::ZeroVector);
	Manager->Tick(0.016f);

	TestTrue(TEXT("Ball near pocket mouth should get captured and pocketed instead of escaping outside the table."), TestBall->IsPocketed() || TestBall->IsHidden());

	CleanupManager(Manager);
	DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCornerPocketScoreWhileSinkingRuntimeTest,
	"Billard.Runtime.CornerPocketScoreWhileSinking",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FCornerPocketScoreWhileSinkingRuntimeTest::RunTest(const FString& Parameters)
{
	using namespace PoolRuntimePhysicsTests;

	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Runtime test world should exist"), World))
	{
		return false;
	}

	APoolTableManager* Manager = SpawnManager(World);
	if (!TestNotNull(TEXT("PoolTableManager should spawn"), Manager))
	{
		DestroyTestWorld(World);
		return false;
	}

	APoolBall* TestBall = nullptr;
	for (APoolBall* Ball : Manager->GetSpawnedBalls())
	{
		if (Ball && !Ball->IsCueBall())
		{
			TestBall = Ball;
			break;
		}
	}

	APoolPocketTrigger* CornerPocket = nullptr;
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
			CornerPocket = Pocket;
			break;
		}
	}

	if (!TestNotNull(TEXT("Object ball should exist"), TestBall) || !TestNotNull(TEXT("Corner pocket should exist"), CornerPocket))
	{
		CleanupManager(Manager);
		DestroyTestWorld(World);
		return false;
	}

	const FVector PocketLocation = CornerPocket->GetActorLocation();
	TestBall->ResetBall(FTransform(FRotator::ZeroRotator, PocketLocation + Manager->GetTableUpAxis() * Manager->GetBallRadius(), FVector(1.0f)));
	TestBall->BeginPocketSink(PocketLocation - Manager->GetTableUpAxis() * (Manager->GetBallRadius() * 5.2f));
	Manager->HandleBallPocketed(TestBall, PocketLocation);
	Manager->HandleBallPocketed(TestBall, PocketLocation);

	TestEqual(TEXT("Corner pocket scoring should count a sinking object ball exactly once."), Manager->GetPocketedBallCount(), 1);
	TestTrue(TEXT("Ball should remain pocketed or in sink animation after scoring."), TestBall->IsPocketed() || TestBall->IsSinkingIntoPocket());

	CleanupManager(Manager);
	DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTwoBallCollisionRuntimeTest,
	"Billard.Runtime.TwoBallCollision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FTwoBallCollisionRuntimeTest::RunTest(const FString& Parameters)
{
	using namespace PoolRuntimePhysicsTests;

	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Runtime test world should exist"), World))
	{
		return false;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APoolBall* BallA = World->SpawnActor<APoolBall>(APoolBall::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	APoolBall* BallB = World->SpawnActor<APoolBall>(APoolBall::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (!TestNotNull(TEXT("Ball A should spawn"), BallA) || !TestNotNull(TEXT("Ball B should spawn"), BallB))
	{
		if (BallA) BallA->Destroy();
		if (BallB) BallB->Destroy();
		return false;
	}

	const float Radius = BallA->GetBallRadius();
	const FVector PlaneOrigin(0.0f, 0.0f, Radius);
	BallA->SetMovementPlane(FVector::UpVector, PlaneOrigin);
	BallB->SetMovementPlane(FVector::UpVector, PlaneOrigin);
	BallA->TeleportBall(FVector(-13.0f, 0.0f, Radius));
	BallB->TeleportBall(FVector(0.0f, 0.0f, Radius));
	BallA->SetLinearVelocity(FVector(280.0f, 0.0f, 0.0f));
	BallB->SetLinearVelocity(FVector::ZeroVector);

	TickWorld(World, 0.45f);

	const FVector VelocityA = BallA->GetLinearVelocity();
	const FVector VelocityB = BallB->GetLinearVelocity();
	AddInfo(FString::Printf(TEXT("Collision velocities A=%s B=%s"), *VelocityA.ToCompactString(), *VelocityB.ToCompactString()));

	TestTrue(TEXT("After a straight collision, the struck ball should carry forward motion."), VelocityB.X > 40.0f);
	TestTrue(TEXT("The striking ball should slow down noticeably after transferring momentum."), VelocityA.X < 180.0f);
	TestTrue(TEXT("A straight collision should not create large sideways teleport-like drift."), FMath::Abs(VelocityB.Y) < 35.0f);

	BallA->Destroy();
	BallB->Destroy();
	DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAngledTwoBallCollisionRuntimeTest,
	"Billard.Runtime.AngledTwoBallCollision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FAngledTwoBallCollisionRuntimeTest::RunTest(const FString& Parameters)
{
	using namespace PoolRuntimePhysicsTests;

	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Runtime test world should exist"), World))
	{
		return false;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APoolBall* BallA = World->SpawnActor<APoolBall>(APoolBall::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	APoolBall* BallB = World->SpawnActor<APoolBall>(APoolBall::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (!TestNotNull(TEXT("Ball A should spawn"), BallA) || !TestNotNull(TEXT("Ball B should spawn"), BallB))
	{
		if (BallA) BallA->Destroy();
		if (BallB) BallB->Destroy();
		return false;
	}

	const float Radius = BallA->GetBallRadius();
	const FVector PlaneOrigin(0.0f, 0.0f, Radius);
	BallA->SetMovementPlane(FVector::UpVector, PlaneOrigin);
	BallB->SetMovementPlane(FVector::UpVector, PlaneOrigin);
	BallA->TeleportBall(FVector(-12.8f, -2.3f, Radius));
	BallB->TeleportBall(FVector(0.0f, 0.0f, Radius));
	BallA->SetLinearVelocity(FVector(240.0f, 20.0f, 0.0f));
	BallB->SetLinearVelocity(FVector::ZeroVector);

	TickWorld(World, 0.55f);

	const FVector VelocityA = BallA->GetLinearVelocity();
	const FVector VelocityB = BallB->GetLinearVelocity();
	AddInfo(FString::Printf(TEXT("Angled collision velocities A=%s B=%s"), *VelocityA.ToCompactString(), *VelocityB.ToCompactString()));

	TestTrue(TEXT("Po skośnym kontakcie trafiona bila powinna przejąć wyraźny ruch do przodu."), VelocityB.X > 30.0f);
	TestTrue(TEXT("Po skośnym kontakcie trafiona bila może dostać niewielki boczny składnik, ale nie losowy odrzut."), FMath::Abs(VelocityB.Y) < VelocityB.X * 0.65f + 5.0f);
	TestTrue(TEXT("Bila uderzająca powinna zachować część własnego składnika bocznego po cięciu."), FMath::Abs(VelocityA.Y) > 2.0f);

	BallA->Destroy();
	BallB->Destroy();
	DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCueBallScratchRespawnRuntimeTest,
	"Billard.Runtime.CueBallScratchRespawn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCornerCueBallScratchRuntimeTest,
	"Billard.Runtime.CornerCueBallScratch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCornerCueBallJawScratchRuntimeTest,
	"Billard.Runtime.CornerCueBallJawScratch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCornerCueBallScratchDuplicateTriggerRuntimeTest,
	"Billard.Runtime.CornerCueBallScratchDuplicateTrigger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAllCornerCueBallScratchRuntimeTest,
	"Billard.Runtime.AllCornerCueBallScratch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAllCornerObjectBallPocketRuntimeTest,
	"Billard.Runtime.AllCornerObjectBallPocket",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCornerPocketObjectBallShotMatrixRuntimeTest,
	"Billard.Runtime.CornerPocket.ObjectBallShotMatrix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCornerPocketCueBallShotMatrixRuntimeTest,
	"Billard.Runtime.CornerPocket.CueBallScratchShotMatrix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCueBallScratchRespawnRuntimeTest::RunTest(const FString& Parameters)
{
	using namespace PoolRuntimePhysicsTests;

	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Runtime test world should exist"), World))
	{
		return false;
	}

	APoolTableManager* Manager = SpawnManager(World);
	if (!TestNotNull(TEXT("PoolTableManager should spawn"), Manager))
	{
		DestroyTestWorld(World);
		return false;
	}

	APoolBall* CueBall = Manager->GetCueBall();
	if (!TestNotNull(TEXT("Cue ball should exist"), CueBall))
	{
		CleanupManager(Manager);
		DestroyTestWorld(World);
		return false;
	}

	const FVector CueBallStart = CueBall->GetActorLocation();
	const TArray<APoolPocketTrigger*>& Pockets = Manager->GetPocketTriggersForTests();
	if (!TestTrue(TEXT("Pocket triggers should exist"), !Pockets.IsEmpty()))
	{
		CleanupManager(Manager);
		DestroyTestWorld(World);
		return false;
	}

	Manager->HandleBallPocketed(CueBall, Pockets[0]->GetActorLocation());
	TestTrue(TEXT("Cue ball should start a pocket sink instead of instantly teleporting back to spawn."), CueBall->IsSinkingIntoPocket());

	TickWorld(World, 0.18f);
	TestTrue(TEXT("Cue ball should still be away from its spawn point during the sink animation."), FVector::Dist(CueBall->GetActorLocation(), CueBallStart) > CueBall->GetBallRadius());

	TickWorld(World, 1.25f);
	TestFalse(TEXT("Cue ball should finish the pocket animation before respawn completes."), CueBall->IsSinkingIntoPocket());
	TestFalse(TEXT("Cue ball should no longer be pocketed after returning to the table."), CueBall->IsPocketed());
	TestTrue(TEXT("After a scratch, the table manager should switch into cue-ball-in-hand mode."), Manager->IsCueBallInHand());
	TestTrue(TEXT("Cue ball should come back onto the table near a legal starting placement."), FVector::Dist(CueBall->GetActorLocation(), Manager->GetCueBallInHandLocation()) < 1.0f);

	const FVector NewPlacement =
		Manager->GetTableSurfaceCenter()
		- Manager->GetTableLongAxis() * (Manager->GetHalfPlayLengthForTests() * 0.22f)
		+ Manager->GetTableShortAxis() * (Manager->GetHalfPlayWidthForTests() * 0.18f)
		+ Manager->GetTableUpAxis() * Manager->GetBallRadius();
	TestTrue(TEXT("Manager should accept a legal cue-ball placement after scratch."), Manager->ConfirmCueBallPlacement(NewPlacement));
	TestFalse(TEXT("Ball-in-hand mode should end after confirming a legal placement."), Manager->IsCueBallInHand());
	TestTrue(TEXT("Cue ball should move to the manager-approved placement."), FVector::Dist(CueBall->GetActorLocation(), Manager->GetCueBallInHandLocation()) < 1.0f);

	CleanupManager(Manager);
	DestroyTestWorld(World);
	return true;
}

bool FCornerCueBallScratchRuntimeTest::RunTest(const FString& Parameters)
{
	using namespace PoolRuntimePhysicsTests;

	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Runtime test world should exist"), World))
	{
		return false;
	}

	APoolTableManager* Manager = SpawnManager(World);
	if (!TestNotNull(TEXT("PoolTableManager should spawn"), Manager))
	{
		DestroyTestWorld(World);
		return false;
	}

	APoolBall* CueBall = Manager->GetCueBall();
	if (!TestNotNull(TEXT("Cue ball should exist"), CueBall))
	{
		CleanupManager(Manager);
		DestroyTestWorld(World);
		return false;
	}

	const TArray<APoolPocketTrigger*>& Pockets = Manager->GetPocketTriggersForTests();
	if (!TestTrue(TEXT("Pocket triggers should exist"), !Pockets.IsEmpty()))
	{
		CleanupManager(Manager);
		DestroyTestWorld(World);
		return false;
	}

	APoolPocketTrigger* CornerPocket = nullptr;
	for (APoolPocketTrigger* Pocket : Pockets)
	{
		if (!Pocket)
		{
			continue;
		}

		const FVector PocketOffset = Pocket->GetActorLocation() - Manager->GetSurfacePointForTests();
		const float AlongLong = FMath::Abs(FVector::DotProduct(PocketOffset, Manager->GetTableLongAxis()));
		if (AlongLong > Manager->GetHalfPlayLengthForTests() * 0.35f)
		{
			CornerPocket = Pocket;
			break;
		}
	}

	if (!TestNotNull(TEXT("A corner pocket trigger should exist"), CornerPocket))
	{
		CleanupManager(Manager);
		DestroyTestWorld(World);
		return false;
	}

	const FVector CornerPocketLocation = CornerPocket->GetActorLocation();
	const FVector PocketOffset = CornerPocketLocation - Manager->GetSurfacePointForTests();
	const float LongSign = FMath::Sign(FVector::DotProduct(PocketOffset, Manager->GetTableLongAxis()));
	const float ShortSign = FMath::Sign(FVector::DotProduct(PocketOffset, Manager->GetTableShortAxis()));
	const FVector SpawnLocation =
		Manager->GetSurfacePointForTests()
		+ Manager->GetTableLongAxis() * (LongSign * (Manager->GetHalfPlayLengthForTests() - Manager->GetPocketRadius() * 1.05f))
		+ Manager->GetTableShortAxis() * (ShortSign * (Manager->GetHalfPlayWidthForTests() - Manager->GetPocketRadius() * 1.05f))
		+ Manager->GetTableUpAxis() * Manager->GetBallRadius();
	const FVector ShotDirection = (
		Manager->GetTableLongAxis() * LongSign
		+ Manager->GetTableShortAxis() * ShortSign).GetSafeNormal();

	CueBall->TeleportBall(SpawnLocation);
	CueBall->SetLinearVelocity(ShotDirection * 175.0f);

	TickWorld(World, 0.65f);
	TestTrue(TEXT("Cue ball entering a corner pocket should start sinking."), CueBall->IsPocketed() || CueBall->IsSinkingIntoPocket());

	TickWorld(World, 1.35f);
	TestTrue(TEXT("Corner scratch should activate cue-ball-in-hand mode."), Manager->IsCueBallInHand());
	TestTrue(TEXT("Cue ball should return to a valid placement point after a corner scratch."), FVector::Dist(CueBall->GetActorLocation(), Manager->GetCueBallInHandLocation()) < 1.0f);

	CleanupManager(Manager);
	DestroyTestWorld(World);
	return true;
}

bool FCornerCueBallJawScratchRuntimeTest::RunTest(const FString& Parameters)
{
	using namespace PoolRuntimePhysicsTests;

	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Runtime test world should exist"), World))
	{
		return false;
	}

	APoolTableManager* Manager = SpawnManager(World);
	if (!TestNotNull(TEXT("PoolTableManager should spawn"), Manager))
	{
		DestroyTestWorld(World);
		return false;
	}

	APoolBall* CueBall = Manager->GetCueBall();
	if (!TestNotNull(TEXT("Cue ball should exist"), CueBall))
	{
		CleanupManager(Manager);
		DestroyTestWorld(World);
		return false;
	}

	APoolPocketTrigger* CornerPocket = nullptr;
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
			CornerPocket = Pocket;
			break;
		}
	}

	if (!TestNotNull(TEXT("A corner pocket trigger should exist"), CornerPocket))
	{
		CleanupManager(Manager);
		DestroyTestWorld(World);
		return false;
	}

	const FVector CornerPocketLocation = CornerPocket->GetActorLocation();
	const FVector PocketOffset = CornerPocketLocation - Manager->GetSurfacePointForTests();
	const float LongSign = FMath::Sign(FVector::DotProduct(PocketOffset, Manager->GetTableLongAxis()));
	const float ShortSign = FMath::Sign(FVector::DotProduct(PocketOffset, Manager->GetTableShortAxis()));
	const FVector SpawnLocation =
		Manager->GetSurfacePointForTests()
		+ Manager->GetTableLongAxis() * (LongSign * (Manager->GetHalfPlayLengthForTests() - Manager->GetPocketRadius() * 2.35f))
		+ Manager->GetTableShortAxis() * (ShortSign * (Manager->GetHalfPlayWidthForTests() - Manager->GetPocketRadius() * 1.2f))
		+ Manager->GetTableUpAxis() * Manager->GetBallRadius();
	const FVector ShotDirection = (
		Manager->GetTableLongAxis() * LongSign * 0.95f
		+ Manager->GetTableShortAxis() * ShortSign * 0.35f).GetSafeNormal();

	CueBall->TeleportBall(SpawnLocation);
	CueBall->SetLinearVelocity(ShotDirection * 190.0f);

	TickWorld(World, 0.75f);
	TestTrue(TEXT("Cue ball grazing the corner jaw should still get captured as a scratch."), CueBall->IsPocketed() || CueBall->IsSinkingIntoPocket());

	TickWorld(World, 1.35f);
	TestTrue(TEXT("Corner jaw scratch should activate cue-ball-in-hand mode."), Manager->IsCueBallInHand());
	TestTrue(TEXT("Cue ball should return onto the table for placement after a corner jaw scratch."), FVector::Dist(CueBall->GetActorLocation(), Manager->GetCueBallInHandLocation()) < 1.0f);

	CleanupManager(Manager);
	DestroyTestWorld(World);
	return true;
}

bool FCornerCueBallScratchDuplicateTriggerRuntimeTest::RunTest(const FString& Parameters)
{
	using namespace PoolRuntimePhysicsTests;

	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Runtime test world should exist"), World))
	{
		return false;
	}

	APoolTableManager* Manager = SpawnManager(World);
	if (!TestNotNull(TEXT("PoolTableManager should spawn"), Manager))
	{
		DestroyTestWorld(World);
		return false;
	}

	APoolBall* CueBall = Manager->GetCueBall();
	if (!TestNotNull(TEXT("Cue ball should exist"), CueBall))
	{
		CleanupManager(Manager);
		DestroyTestWorld(World);
		return false;
	}

	APoolPocketTrigger* CornerPocket = nullptr;
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
			CornerPocket = Pocket;
			break;
		}
	}

	if (!TestNotNull(TEXT("A corner pocket trigger should exist"), CornerPocket))
	{
		CleanupManager(Manager);
		DestroyTestWorld(World);
		return false;
	}

	const FVector CornerPocketLocation = CornerPocket->GetActorLocation();
	const FVector PocketOffset = CornerPocketLocation - Manager->GetSurfacePointForTests();
	const float LongSign = FMath::Sign(FVector::DotProduct(PocketOffset, Manager->GetTableLongAxis()));
	const float ShortSign = FMath::Sign(FVector::DotProduct(PocketOffset, Manager->GetTableShortAxis()));
	const FVector SpawnLocation =
		Manager->GetSurfacePointForTests()
		+ Manager->GetTableLongAxis() * (LongSign * (Manager->GetHalfPlayLengthForTests() - Manager->GetPocketRadius() * 1.05f))
		+ Manager->GetTableShortAxis() * (ShortSign * (Manager->GetHalfPlayWidthForTests() - Manager->GetPocketRadius() * 1.05f))
		+ Manager->GetTableUpAxis() * Manager->GetBallRadius();
	const FVector ShotDirection = (
		Manager->GetTableLongAxis() * LongSign
		+ Manager->GetTableShortAxis() * ShortSign).GetSafeNormal();

	CueBall->TeleportBall(SpawnLocation);
	CueBall->SetLinearVelocity(ShotDirection * 175.0f);

	TickWorld(World, 2.0f);
	TestTrue(TEXT("Corner scratch should activate cue-ball-in-hand mode before any duplicate trigger arrives."), Manager->IsCueBallInHand());

	Manager->HandleBallPocketed(CueBall, CornerPocketLocation);
	TickWorld(World, 0.1f);

	TestTrue(TEXT("A duplicate cue-ball pocket callback should not cancel cue-ball-in-hand mode."), Manager->IsCueBallInHand());
	TestTrue(TEXT("Cue ball should stay on the legal placement point after a duplicate trigger."), FVector::Dist(CueBall->GetActorLocation(), Manager->GetCueBallInHandLocation()) < 1.0f);

	CleanupManager(Manager);
	DestroyTestWorld(World);
	return true;
}

bool FAllCornerCueBallScratchRuntimeTest::RunTest(const FString& Parameters)
{
	using namespace PoolRuntimePhysicsTests;

	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Runtime test world should exist"), World))
	{
		return false;
	}

	APoolTableManager* Manager = SpawnManager(World);
	if (!TestNotNull(TEXT("PoolTableManager should spawn"), Manager))
	{
		DestroyTestWorld(World);
		return false;
	}

	APoolBall* CueBall = Manager->GetCueBall();
	if (!TestNotNull(TEXT("Cue ball should exist"), CueBall))
	{
		CleanupManager(Manager);
		DestroyTestWorld(World);
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
		DestroyTestWorld(World);
		return false;
	}

	for (int32 Index = 0; Index < CornerPockets.Num(); ++Index)
	{
		APoolPocketTrigger* CornerPocket = CornerPockets[Index];
		const FVector CornerPocketLocation = CornerPocket->GetActorLocation();
		const FVector PocketOffset = CornerPocketLocation - Manager->GetSurfacePointForTests();
		const float LongSign = FMath::Sign(FVector::DotProduct(PocketOffset, Manager->GetTableLongAxis()));
		const float ShortSign = FMath::Sign(FVector::DotProduct(PocketOffset, Manager->GetTableShortAxis()));
		const FVector SpawnLocation =
			Manager->GetSurfacePointForTests()
			+ Manager->GetTableLongAxis() * (LongSign * (Manager->GetHalfPlayLengthForTests() - Manager->GetPocketRadius() * 1.05f))
			+ Manager->GetTableShortAxis() * (ShortSign * (Manager->GetHalfPlayWidthForTests() - Manager->GetPocketRadius() * 1.05f))
			+ Manager->GetTableUpAxis() * Manager->GetBallRadius();
		const FVector ShotDirection = (
			Manager->GetTableLongAxis() * LongSign
			+ Manager->GetTableShortAxis() * ShortSign).GetSafeNormal();

		CueBall->ResetBall(FTransform(FRotator::ZeroRotator, SpawnLocation, FVector(1.0f)));
		CueBall->SetLinearVelocity(ShotDirection * 175.0f);
		TickWorld(World, 0.65f);

		TestTrue(FString::Printf(TEXT("Corner %d scratch should start pocket sink."), Index), CueBall->IsPocketed() || CueBall->IsSinkingIntoPocket());

		TickWorld(World, 1.35f);
		TestTrue(FString::Printf(TEXT("Corner %d scratch should activate cue-ball-in-hand mode."), Index), Manager->IsCueBallInHand());
		TestTrue(FString::Printf(TEXT("Corner %d should return cue ball onto the table for placement."), Index), FVector::Dist(CueBall->GetActorLocation(), Manager->GetCueBallInHandLocation()) < 1.0f);

		TestTrue(FString::Printf(TEXT("Corner %d should allow confirming legal cue-ball placement."), Index), Manager->ConfirmCueBallPlacement(Manager->GetCueBallStartLocation()));
		TestFalse(FString::Printf(TEXT("Corner %d should leave cue-ball-in-hand mode after confirming placement."), Index), Manager->IsCueBallInHand());
	}

	CleanupManager(Manager);
	DestroyTestWorld(World);
	return true;
}

bool FAllCornerObjectBallPocketRuntimeTest::RunTest(const FString& Parameters)
{
	using namespace PoolRuntimePhysicsTests;

	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Runtime test world should exist"), World))
	{
		return false;
	}

	APoolTableManager* Manager = SpawnManager(World);
	if (!TestNotNull(TEXT("PoolTableManager should spawn"), Manager))
	{
		DestroyTestWorld(World);
		return false;
	}

	APoolBall* TestBall = nullptr;
	for (APoolBall* Ball : Manager->GetSpawnedBalls())
	{
		if (Ball && !Ball->IsCueBall())
		{
			TestBall = Ball;
			break;
		}
	}

	if (!TestNotNull(TEXT("Object ball should exist"), TestBall))
	{
		CleanupManager(Manager);
		DestroyTestWorld(World);
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
		DestroyTestWorld(World);
		return false;
	}

	for (int32 Index = 0; Index < CornerPockets.Num(); ++Index)
	{
		APoolPocketTrigger* CornerPocket = CornerPockets[Index];
		const FVector CornerPocketLocation = CornerPocket->GetActorLocation();
		const FVector PocketOffset = CornerPocketLocation - Manager->GetSurfacePointForTests();
		const float LongSign = FMath::Sign(FVector::DotProduct(PocketOffset, Manager->GetTableLongAxis()));
		const float ShortSign = FMath::Sign(FVector::DotProduct(PocketOffset, Manager->GetTableShortAxis()));
		const FVector SpawnLocation =
			Manager->GetSurfacePointForTests()
			+ Manager->GetTableLongAxis() * (LongSign * (Manager->GetHalfPlayLengthForTests() - Manager->GetPocketRadius() * 0.8f))
			+ Manager->GetTableShortAxis() * (ShortSign * (Manager->GetHalfPlayWidthForTests() - Manager->GetPocketRadius() * 0.8f))
			+ Manager->GetTableUpAxis() * Manager->GetBallRadius();
		const FVector ShotDirection = (
			Manager->GetTableLongAxis() * LongSign
			+ Manager->GetTableShortAxis() * ShortSign).GetSafeNormal();

		TestBall->ResetBall(FTransform(FRotator::ZeroRotator, SpawnLocation, FVector(1.0f)));
		TestBall->SetLinearVelocity(ShotDirection * 180.0f);
		TickWorld(World, 0.45f);

		TestTrue(FString::Printf(TEXT("Corner %d object ball should get pocketed."), Index), TestBall->IsPocketed() || TestBall->IsSinkingIntoPocket() || TestBall->IsHidden());
		TestEqual(FString::Printf(TEXT("Corner %d should increment the pocketed object-ball count."), Index), Manager->GetPocketedBallCount(), Index + 1);
	}

	CleanupManager(Manager);
	DestroyTestWorld(World);
	return true;
}

bool FCornerPocketObjectBallShotMatrixRuntimeTest::RunTest(const FString& Parameters)
{
	using namespace PoolRuntimePhysicsTests;

	UWorld* World = CreateTestWorld();
	if (!TestNotNull(TEXT("Runtime test world should exist"), World))
	{
		return false;
	}

	APoolTableManager* Manager = SpawnManager(World);
	if (!TestNotNull(TEXT("PoolTableManager should spawn"), Manager))
	{
		DestroyTestWorld(World);
		return false;
	}

	APoolBall* TestBall = nullptr;
	for (APoolBall* Ball : Manager->GetSpawnedBalls())
	{
		if (Ball && !Ball->IsCueBall())
		{
			TestBall = Ball;
			break;
		}
	}

	const TArray<APoolPocketTrigger*> CornerPockets = GetCornerPockets(Manager);
	if (!TestNotNull(TEXT("Object ball should exist"), TestBall) || !TestEqual(TEXT("There should be four corner pockets."), CornerPockets.Num(), 4))
	{
		CleanupManager(Manager);
		DestroyTestWorld(World);
		return false;
	}

	const TArray<FCornerShotScenario> Scenarios = {
		{ TEXT("CenterlineMedium"), 1.00f, 1.00f, FVector2D(1.0f, 1.0f), 175.0f, 0.85f, 75.0f },
		{ TEXT("JawGrazeFast"), 2.10f, 1.10f, FVector2D(0.94f, 0.34f), 195.0f, 0.95f, 85.0f },
		{ TEXT("WiderCut"), 1.35f, 1.55f, FVector2D(0.72f, 1.0f), 185.0f, 0.95f, 90.0f }
	};

	for (int32 PocketIndex = 0; PocketIndex < CornerPockets.Num(); ++PocketIndex)
	{
		for (const FCornerShotScenario& Scenario : Scenarios)
		{
			const FShotObservation Observation = SimulateCornerShot(World, Manager, TestBall, CornerPockets[PocketIndex], Scenario);
			AddInfo(FString::Printf(
				TEXT("ObjectBall pocket=%d scenario=%s minDistance=%.2f reverse=%.2f reachedMouth=%d pocketed=%d"),
				PocketIndex,
				Scenario.Name,
				Observation.MinPocketDistance,
				Observation.MaxReverseSpeed,
				Observation.bReachedPocketMouth ? 1 : 0,
				Observation.bBecamePocketed ? 1 : 0));

			TestTrue(
				FString::Printf(TEXT("Object ball should pocket for corner %d scenario %s."), PocketIndex, Scenario.Name),
				Observation.bBecamePocketed);
			TestTrue(
				FString::Printf(TEXT("Object ball should not visibly rebound off a phantom corner collider for corner %d scenario %s."), PocketIndex, Scenario.Name),
				Observation.MaxReverseSpeed <= Scenario.AllowedReverseSpeed);

			TestBall->ResetBall(FTransform(FRotator::ZeroRotator, Manager->GetRackCenterLocation(), FVector(1.0f)));
			TestBall->SetLinearVelocity(FVector::ZeroVector);
			TestBall->SetAngularVelocityDegrees(FVector::ZeroVector);
			TickWorld(World, 0.1f);
		}
	}

	CleanupManager(Manager);
	DestroyTestWorld(World);
	return true;
}

bool FCornerPocketCueBallShotMatrixRuntimeTest::RunTest(const FString& Parameters)
{
	using namespace PoolRuntimePhysicsTests;

	const TArray<FCornerShotScenario> Scenarios = {
		{ TEXT("CenterScratch"), 1.05f, 1.05f, FVector2D(1.0f, 1.0f), 175.0f, 0.85f, 80.0f },
		{ TEXT("JawScratch"), 2.25f, 1.18f, FVector2D(0.95f, 0.35f), 190.0f, 0.95f, 90.0f }
	};

	for (int32 PocketIndex = 0; PocketIndex < 4; ++PocketIndex)
	{
		for (const FCornerShotScenario& Scenario : Scenarios)
		{
			UWorld* World = CreateTestWorld();
			if (!TestNotNull(TEXT("Runtime test world should exist"), World))
			{
				return false;
			}

			APoolTableManager* Manager = SpawnManager(World);
			if (!TestNotNull(TEXT("PoolTableManager should spawn"), Manager))
			{
				DestroyTestWorld(World);
				return false;
			}

			APoolBall* CueBall = Manager->GetCueBall();
			const TArray<APoolPocketTrigger*> CornerPockets = GetCornerPockets(Manager);
			if (!TestNotNull(TEXT("Cue ball should exist"), CueBall) || !TestEqual(TEXT("There should be four corner pockets."), CornerPockets.Num(), 4))
			{
				CleanupManager(Manager);
				DestroyTestWorld(World);
				return false;
			}

			const FShotObservation Observation = SimulateCornerShot(World, Manager, CueBall, CornerPockets[PocketIndex], Scenario);
			AddInfo(FString::Printf(
				TEXT("CueBall pocket=%d scenario=%s minDistance=%.2f reverse=%.2f reachedMouth=%d enteredTrigger=%d minTrigger=%.2f minWall=%.2f pocketed=%d"),
				PocketIndex,
				Scenario.Name,
				Observation.MinPocketDistance,
				Observation.MaxReverseSpeed,
				Observation.bReachedPocketMouth ? 1 : 0,
				Observation.bEnteredPocketTrigger ? 1 : 0,
				Observation.MinTriggerClearance,
				Observation.MinWallClearance,
				Observation.bBecamePocketed ? 1 : 0));
			if (!Observation.bBecamePocketed)
			{
				AddInfo(FString::Printf(TEXT("CueBallTrace pocket=%d scenario=%s %s"), PocketIndex, Scenario.Name, *Observation.TraceSummary));
			}

			TestTrue(
				FString::Printf(TEXT("Cue ball should scratch into corner %d scenario %s."), PocketIndex, Scenario.Name),
				Observation.bBecamePocketed);
			TestTrue(
				FString::Printf(TEXT("Cue ball should not bounce back off a phantom jaw collider for corner %d scenario %s."), PocketIndex, Scenario.Name),
				Observation.MaxReverseSpeed <= Scenario.AllowedReverseSpeed);

			TickWorld(World, 1.5f);
			TestTrue(
				FString::Printf(TEXT("Cue ball scratch should enter cue-ball-in-hand mode for corner %d scenario %s."), PocketIndex, Scenario.Name),
				Manager->IsCueBallInHand());
			TestTrue(
				FString::Printf(TEXT("Cue ball should return to the approved placement after corner %d scenario %s."), PocketIndex, Scenario.Name),
				FVector::Dist(CueBall->GetActorLocation(), Manager->GetCueBallInHandLocation()) < 1.0f);

			TestTrue(
				FString::Printf(TEXT("Cue ball placement should be confirmable after corner %d scenario %s."), PocketIndex, Scenario.Name),
				Manager->ConfirmCueBallPlacement(Manager->GetCueBallStartLocation()));
			TestFalse(
				FString::Printf(TEXT("Cue-ball-in-hand should end after confirming placement for corner %d scenario %s."), PocketIndex, Scenario.Name),
				Manager->IsCueBallInHand());

			CleanupManager(Manager);
			DestroyTestWorld(World);
		}
	}
	return true;
}

#endif
