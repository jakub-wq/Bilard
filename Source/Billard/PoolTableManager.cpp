#include "PoolTableManager.h"

#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"
#include "CollisionQueryParams.h"
#include "MyCharacter.h"
#include "PoolBall.h"
#include "PoolCushionWall.h"
#include "PoolPocketTrigger.h"
#include "PoolSaveGame.h"

void APoolTableManager::SpawnOrReuseFixedTable()
{
	if (SpawnedVisualTable && IsValid(SpawnedVisualTable))
	{
		TableActor = SpawnedVisualTable;
		TableMeshComponent = SpawnedVisualTable->GetStaticMeshComponent();
		return;
	}

	if (!TableVisualMesh)
	{
		TableVisualMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Billard.Billard"));
	}

	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		AStaticMeshActor* Candidate = *It;
		if (!IsValid(Candidate) || Candidate == SpawnedVisualTable)
		{
			continue;
		}

		UStaticMeshComponent* MeshComp = Candidate->GetStaticMeshComponent();
		UStaticMesh* Mesh = MeshComp ? MeshComp->GetStaticMesh() : nullptr;
		if (!Mesh)
		{
			continue;
		}

		const FString MeshName = Mesh->GetName();
		if (MeshName.Contains(TEXT("Billard"), ESearchCase::IgnoreCase) || MeshName.Contains(TEXT("Bilard"), ESearchCase::IgnoreCase))
		{
			TableActor = Candidate;
			TableMeshComponent = MeshComp;
			TableMeshComponent->SetMobility(EComponentMobility::Movable);
			TableActor->SetActorLocation(FixedTableLocation);
			TableActor->SetActorRotation(FixedTableRotation);
			TableMeshComponent->SetWorldScale3D(TableVisualScale);
			TableMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			return;
		}
	}

	if (!TableVisualMesh)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	SpawnedVisualTable = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FixedTableLocation, FixedTableRotation, SpawnParams);
	if (!SpawnedVisualTable)
	{
		return;
	}

	TableActor = SpawnedVisualTable;
	TableMeshComponent = SpawnedVisualTable->GetStaticMeshComponent();
	if (!TableMeshComponent)
	{
		return;
	}

	TableMeshComponent->SetMobility(EComponentMobility::Movable);
	TableMeshComponent->SetStaticMesh(TableVisualMesh);
	TableMeshComponent->SetWorldScale3D(TableVisualScale);
	TableMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnedVisualTable->SetActorRotation(FixedTableRotation);
	SpawnedVisualTable->SetActorLocation(FixedTableLocation);
}

APoolTableManager::APoolTableManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void APoolTableManager::BeginPlay()
{
	Super::BeginPlay();
}

void APoolTableManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	HandleEscapedBalls();
	UpdateCueBallRespawn(DeltaTime);

	if (MatchMode == EPoolMatchMode::LocalVersus
		&& bTurnResolutionPending
		&& !bCueBallRespawnPending
		&& !bCueBallInHand
		&& AreBallsStopped()
		&& !IsAnyBallAnimating())
	{
		ResolveLocalMatchTurn();
	}

	if (bDebugDrawRuntimeColliders)
	{
		DrawDebugRuntimeColliders();
	}
}

void APoolTableManager::BuildTableData()
{
	SpawnOrReuseFixedTable();

	if (!TableMeshComponent || !TableMeshComponent->GetStaticMesh())
	{
		TableUpAxis = FVector::UpVector;
		TableLongAxis = FixedTableRotation.RotateVector(FVector::ForwardVector).GetSafeNormal2D();
		TableShortAxis = FixedTableRotation.RotateVector(FVector::RightVector).GetSafeNormal2D();
		TableCenter = FixedTableLocation;
		HalfOuterLength = 120.0f;
		HalfOuterWidth = 70.0f;
		HalfPlayLength = 98.0f;
		HalfPlayWidth = 48.0f;
		BallRadius = 2.86f;
		PocketRadius = 6.75f;
		const float FallbackSurfaceHeight = FMath::Max(0.0f, 44.0f - SurfaceInsetBelowRails);
		SurfacePoint = FixedTableLocation + TableUpAxis * FallbackSurfaceHeight;
	}
	else
	{
		const FTransform MeshTransform = TableMeshComponent->GetComponentTransform();
		TableUpAxis = MeshTransform.GetUnitAxis(EAxis::Z).GetSafeNormal();
		const FVector ForwardAxis = FVector::VectorPlaneProject(MeshTransform.GetUnitAxis(EAxis::X), TableUpAxis).GetSafeNormal();
		const FVector RightAxis = FVector::VectorPlaneProject(MeshTransform.GetUnitAxis(EAxis::Y), TableUpAxis).GetSafeNormal();
		float MeshHalfExtentX = ManualHalfOuterLength;
		float MeshHalfExtentY = ManualHalfOuterWidth;
		float MeshHalfExtentZ = SurfaceHeightOffset;
		if (const UStaticMesh* TableMesh = TableMeshComponent->GetStaticMesh())
		{
			const FBox LocalBounds = TableMesh->GetBoundingBox();
			const FVector LocalSize = LocalBounds.GetSize();
			const FVector MeshScale = TableMeshComponent->GetComponentScale().GetAbs();
			MeshHalfExtentX = LocalSize.X * MeshScale.X * 0.5f;
			MeshHalfExtentY = LocalSize.Y * MeshScale.Y * 0.5f;
			MeshHalfExtentZ = LocalSize.Z * MeshScale.Z * 0.5f;
		}
		const bool bXAxisIsLong = MeshHalfExtentX >= MeshHalfExtentY;

		TableLongAxis = bXAxisIsLong ? ForwardAxis : RightAxis;
		TableShortAxis = bXAxisIsLong ? RightAxis : ForwardAxis;
		TableCenter = TableMeshComponent->Bounds.Origin;
		HalfOuterLength = ManualHalfOuterLength;
		HalfOuterWidth = ManualHalfOuterWidth;
		HalfPlayLength = ManualHalfPlayLength;
		HalfPlayWidth = ManualHalfPlayWidth;
		BallRadius = ManualBallRadius;
		PocketRadius = BallRadius * PocketRadiusMultiplier;
		const float TopReferenceHeight = FMath::Max(SurfaceHeightOffset, MeshHalfExtentZ * 0.96f);
		const float SurfaceHeight = FMath::Max(0.0f, TopReferenceHeight - SurfaceInsetBelowRails);
		SurfacePoint = TableCenter + TableUpAxis * SurfaceHeight;
	}

	CueBallStartLocation = SurfacePoint - TableLongAxis * (HalfPlayLength * CueBallLengthFactor) + TableUpAxis * BallRadius;
	RackCenterLocation = SurfacePoint + TableLongAxis * (HalfPlayLength * RackCenterLengthFactor) + TableUpAxis * BallRadius;
}

FTransform APoolTableManager::MakeBallTransform(const FVector& WorldLocation) const
{
	return FTransform(FRotator::ZeroRotator, WorldLocation, FVector(1.0f));
}

FVector APoolTableManager::MakeTablePoint(float AlongLong, float AlongShort, float AlongUp) const
{
	return SurfacePoint + TableLongAxis * AlongLong + TableShortAxis * AlongShort + TableUpAxis * AlongUp;
}

void APoolTableManager::DrawDebugRuntimeColliders() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float Thickness = FMath::Max(0.1f, DebugColliderLineThickness);
	const int32 SphereSegments = FMath::Clamp(DebugSphereSegments, 6, 64);

	for (const APoolCushionWall* Wall : CushionWalls)
	{
		if (!IsValid(Wall))
		{
			continue;
		}

		const UBoxComponent* CollisionBox = Wall->FindComponentByClass<UBoxComponent>();
		if (!CollisionBox)
		{
			continue;
		}

		DrawDebugBox(
			World,
			CollisionBox->GetComponentLocation(),
			CollisionBox->GetScaledBoxExtent(),
			CollisionBox->GetComponentQuat(),
			FColor(255, 64, 64),
			false,
			0.0f,
			0,
			Thickness);
	}

	if (IsValid(PlaySurfaceFloor))
	{
		if (const UBoxComponent* FloorBox = PlaySurfaceFloor->FindComponentByClass<UBoxComponent>())
		{
			DrawDebugBox(
				World,
				FloorBox->GetComponentLocation(),
				FloorBox->GetScaledBoxExtent(),
				FloorBox->GetComponentQuat(),
				FColor(64, 220, 255),
				false,
				0.0f,
				0,
				Thickness);
		}
	}

	for (const APoolPocketTrigger* Pocket : PocketTriggers)
	{
		if (!IsValid(Pocket))
		{
			continue;
		}

		const USphereComponent* TriggerSphere = Pocket->FindComponentByClass<USphereComponent>();
		if (!TriggerSphere)
		{
			continue;
		}

		DrawDebugSphere(
			World,
			TriggerSphere->GetComponentLocation(),
			TriggerSphere->GetScaledSphereRadius(),
			SphereSegments,
			FColor(255, 220, 32),
			false,
			0.0f,
			0,
			Thickness);
	}
}

void APoolTableManager::DestroySpawnedActors()
{
	for (APoolPocketTrigger* Pocket : PocketTriggers)
	{
		if (IsValid(Pocket))
		{
			Pocket->Destroy();
		}
	}
	PocketTriggers.Reset();
	PocketLocations.Reset();

	for (APoolCushionWall* Wall : CushionWalls)
	{
		if (IsValid(Wall))
		{
			Wall->Destroy();
		}
	}
	CushionWalls.Reset();

	for (AStaticMeshActor* SupportActor : SupportActors)
	{
		if (IsValid(SupportActor))
		{
			SupportActor->Destroy();
		}
	}
	SupportActors.Reset();

	if (IsValid(PlaySurfaceFloor))
	{
		PlaySurfaceFloor->Destroy();
		PlaySurfaceFloor = nullptr;
	}

	for (APoolBall* Ball : SpawnedBalls)
	{
		if (IsValid(Ball))
		{
			Ball->Destroy();
		}
	}
	SpawnedBalls.Reset();
	InitialBallTransforms.Reset();
	CountedPocketedBalls.Reset();
	CueBall = nullptr;
	PendingCueBallRespawn.Reset();
	PendingCueBallRespawnTransform = FTransform::Identity;
	CueBallRespawnTimer = 0.0f;
	CueBallInHandSettleTimer = 0.0f;
	bCueBallRespawnPending = false;
	bCueBallInHand = false;
	CueBallInHandLocation = FVector::ZeroVector;
	PocketedBallCount = 0;
	ResetLocalMatchState();
}

void APoolTableManager::HandleEscapedBalls()
{
	const float EscapeLongLimit = HalfOuterLength + BallRadius * 0.75f;
	const float EscapeShortLimit = HalfOuterWidth + BallRadius * 0.75f;
	const float CornerMouthLongLimit = HalfPlayLength - BallRadius * 0.35f;
	const float CornerMouthShortLimit = HalfPlayWidth - BallRadius * 0.35f;
	const float CornerApproachLongLimit = HalfPlayLength - PocketRadius * 2.15f;
	const float CornerApproachShortLimit = HalfPlayWidth - PocketRadius * 2.15f;

	for (APoolBall* Ball : SpawnedBalls)
	{
		if (!IsValid(Ball) || Ball->IsPocketed())
		{
			continue;
		}

		const FVector RelativeToSurface = Ball->GetActorLocation() - SurfacePoint;
		const float AlongLong = FVector::DotProduct(RelativeToSurface, TableLongAxis);
		const float AlongShort = FVector::DotProduct(RelativeToSurface, TableShortAxis);
		const bool bInsideOuterEscapeBounds = FMath::Abs(AlongLong) <= EscapeLongLimit && FMath::Abs(AlongShort) <= EscapeShortLimit;
		const bool bInsideCornerMouthBounds = FMath::Abs(AlongLong) <= CornerMouthLongLimit || FMath::Abs(AlongShort) <= CornerMouthShortLimit;
		const bool bInsideCornerApproachBounds = FMath::Abs(AlongLong) <= CornerApproachLongLimit || FMath::Abs(AlongShort) <= CornerApproachShortLimit;
		FVector NearestPocketLocation = FVector::ZeroVector;
		float PocketDistance = TNumericLimits<float>::Max();
		const bool bNearPocket = FindNearestPocketLocation(Ball->GetActorLocation(), NearestPocketLocation, PocketDistance);
		const FVector PocketOffset = NearestPocketLocation - SurfacePoint;
		const bool bNearestPocketIsCorner = bNearPocket && FMath::Abs(FVector::DotProduct(PocketOffset, TableLongAxis)) > HalfPlayLength * 0.35f;
		const bool bInsideEarlyCornerCaptureZone =
			bNearestPocketIsCorner
			&& FMath::Abs(AlongLong) > HalfPlayLength - PocketRadius * 2.45f
			&& FMath::Abs(AlongShort) > HalfPlayWidth - PocketRadius * 2.45f;
		if (bInsideEarlyCornerCaptureZone && PocketDistance <= PocketRadius * (CornerPocketCaptureMultiplier + 0.35f))
		{
			TryResolveBallEscape(Ball, RelativeToSurface, AlongLong, AlongShort);
			continue;
		}

		if (bInsideOuterEscapeBounds && bInsideCornerMouthBounds && bInsideCornerApproachBounds)
		{
			if (bNearPocket)
			{
				const float EarlyCaptureDistance = PocketRadius * (bNearestPocketIsCorner ? CornerPocketCaptureMultiplier : SidePocketTriggerMultiplier);
				if (PocketDistance <= EarlyCaptureDistance)
				{
					TryResolveBallEscape(Ball, RelativeToSurface, AlongLong, AlongShort);
				}
			}

			continue;
		}

		TryResolveBallEscape(Ball, RelativeToSurface, AlongLong, AlongShort);
	}
}

bool APoolTableManager::TryResolveBallEscape(APoolBall* Ball, const FVector& RelativeToSurface, float AlongLong, float AlongShort)
{
	if (!IsValid(Ball))
	{
		return false;
	}

	auto DebugCapture = [&](const TCHAR* Reason, const FVector& PocketLocation, const float CaptureDistance, const FColor& Color)
	{
		if (!bDebugDrawEscapeResolution)
		{
			return;
		}

		UWorld* World = GetWorld();
		if (!World)
		{
			return;
		}

		const FVector Start = Ball->GetActorLocation();
		DrawDebugLine(World, Start, PocketLocation, Color, false, 1.5f, 0, DebugColliderLineThickness + 1.0f);
		DrawDebugSphere(World, Start, BallRadius * 0.45f, 12, Color, false, 1.5f, 0, DebugColliderLineThickness);
		DrawDebugString(World, Start + TableUpAxis * (BallRadius * 2.4f), Reason, nullptr, Color, 1.5f, false, 1.0f);
		UE_LOG(LogTemp, Warning, TEXT("PoolTableManager capture: %s | Ball=%s | PocketDistance=%.2f"), Reason, *GetNameSafe(Ball), CaptureDistance);
	};

	auto DebugCorrection = [&](const FVector& CorrectedLocation, const FVector& ReflectedVelocity, const TCHAR* Reason)
	{
		if (!bDebugDrawEscapeResolution)
		{
			return;
		}

		UWorld* World = GetWorld();
		if (!World)
		{
			return;
		}

		const FVector Start = Ball->GetActorLocation();
		DrawDebugLine(World, Start, CorrectedLocation, FColor::Cyan, false, 1.5f, 0, DebugColliderLineThickness + 1.0f);
		DrawDebugSphere(World, CorrectedLocation, BallRadius * 0.4f, 12, FColor::Cyan, false, 1.5f, 0, DebugColliderLineThickness);
		DrawDebugDirectionalArrow(World, CorrectedLocation, CorrectedLocation + ReflectedVelocity.GetSafeNormal() * FMath::Max(BallRadius * 3.0f, 10.0f), 8.0f, FColor::Blue, false, 1.5f, 0, DebugColliderLineThickness);
		DrawDebugString(World, CorrectedLocation + TableUpAxis * (BallRadius * 2.4f), Reason, nullptr, FColor::Cyan, 1.5f, false, 1.0f);
		UE_LOG(LogTemp, Warning, TEXT("PoolTableManager correction: %s | Ball=%s | AlongLong=%.2f | AlongShort=%.2f"), Reason, *GetNameSafe(Ball), AlongLong, AlongShort);
	};

	FVector NearestPocketLocation = FVector::ZeroVector;
	float PocketDistance = TNumericLimits<float>::Max();
	const bool bNearPocket = FindNearestPocketLocation(Ball->GetActorLocation(), NearestPocketLocation, PocketDistance);
	const float OuterLongLimit = HalfOuterLength - BallRadius * 0.35f;
	const float OuterShortLimit = HalfOuterWidth - BallRadius * 0.35f;
	const bool bOutsideOuterBounds = FMath::Abs(AlongLong) > OuterLongLimit || FMath::Abs(AlongShort) > OuterShortLimit;
	const FVector PocketOffset = NearestPocketLocation - SurfacePoint;
	const float PocketLong = FVector::DotProduct(PocketOffset, TableLongAxis);
	const bool bNearestPocketIsCorner = FMath::Abs(PocketLong) > HalfPlayLength * 0.35f;
	const bool bPastCornerMouth =
		bNearestPocketIsCorner
		&& FMath::Abs(AlongLong) > HalfPlayLength - BallRadius * 0.35f
		&& FMath::Abs(AlongShort) > HalfPlayWidth - BallRadius * 0.35f;
	const bool bInsideCornerFunnel =
		bNearestPocketIsCorner
		&& FMath::Abs(AlongLong) > HalfPlayLength - PocketRadius * 1.35f
		&& FMath::Abs(AlongShort) > HalfPlayWidth - PocketRadius * 1.35f;
	const bool bInsideCornerApproachZone =
		bNearestPocketIsCorner
		&& FMath::Abs(AlongLong) > HalfPlayLength - PocketRadius * 2.15f
		&& FMath::Abs(AlongShort) > HalfPlayWidth - PocketRadius * 2.15f;
	const bool bInsideForcedCornerCaptureZone =
		bNearestPocketIsCorner
		&& FMath::Abs(AlongLong) > HalfPlayLength - PocketRadius * 2.85f
		&& FMath::Abs(AlongShort) > HalfPlayWidth - PocketRadius * 2.85f;
	const float CollisionHalfLength = FMath::Max(BallRadius * 7.0f, HalfPlayLength - CollisionInsetLong);
	const float CollisionHalfWidth = FMath::Max(BallRadius * 5.0f, HalfPlayWidth - CollisionInsetShort);
	const float SafeLongLimit = CollisionHalfLength - BallRadius * 1.05f;
	const float SafeShortLimit = CollisionHalfWidth - BallRadius * 1.05f;
	const bool bWouldClampAndReflect = FMath::Abs(AlongLong) > SafeLongLimit || FMath::Abs(AlongShort) > SafeShortLimit;
	const FVector PlanarVelocity = FVector::VectorPlaneProject(Ball->GetLinearVelocity(), TableUpAxis);
	const FVector ToPocketPlanar = FVector::VectorPlaneProject(NearestPocketLocation - Ball->GetActorLocation(), TableUpAxis);
	const float PocketApproachSpeed = FVector::DotProduct(PlanarVelocity, ToPocketPlanar.GetSafeNormal());
	const float PlanarSpeed = PlanarVelocity.Size();
	const bool bInsideCornerJawCaptureZone =
		bNearestPocketIsCorner
		&& FMath::Abs(AlongLong) > HalfPlayLength - PocketRadius * 2.75f
		&& FMath::Abs(AlongShort) > HalfPlayWidth - PocketRadius * 1.85f;
	const bool bInsideLooseCueBallCornerCaptureZone =
		bNearestPocketIsCorner
		&& FMath::Abs(AlongLong) > HalfPlayLength - PocketRadius * 3.45f
		&& FMath::Abs(AlongShort) > HalfPlayWidth - PocketRadius * 2.55f;
	const float PocketCaptureDistance = PocketRadius * (bNearestPocketIsCorner
		? (bOutsideOuterBounds ? CornerPocketCaptureMultiplier : CornerPocketTriggerMultiplier)
		: (bOutsideOuterBounds ? 1.9f : 1.45f));
	const bool bShouldForceCornerCapture =
		bInsideForcedCornerCaptureZone
		&& bNearPocket
		&& PocketDistance <= PocketRadius * CornerPocketForceCaptureMultiplier;
	const bool bShouldCaptureInCornerFunnel =
		bInsideCornerFunnel
		&& bNearPocket
		&& PocketDistance <= PocketRadius * (CornerPocketTriggerMultiplier + 0.15f);
	const bool bShouldCaptureOnCornerApproach =
		bInsideCornerApproachZone
		&& bNearPocket
		&& PocketDistance <= PocketRadius * CornerPocketCaptureMultiplier;
	const bool bShouldCaptureCueBallOnCornerJaw =
		Ball->IsCueBall()
		&& (bInsideCornerJawCaptureZone || bInsideLooseCueBallCornerCaptureZone)
		&& bNearPocket
		&& PlanarSpeed > 5.0f
		&& PocketDistance <= PocketRadius * (bWouldClampAndReflect ? CornerPocketForceCaptureMultiplier : (CornerPocketCaptureMultiplier + 1.05f))
		&& (PocketApproachSpeed > 1.0f || bWouldClampAndReflect);
	if ((bOutsideOuterBounds || bPastCornerMouth) && bNearPocket)
	{
		if (Ball->IsCueBall())
		{
			DebugCapture(TEXT("OutsideBoundsPocketCapture"), NearestPocketLocation, PocketDistance, FColor::Yellow);
			QueueCueBallRespawn(Ball, NearestPocketLocation);
			return true;
		}

		DebugCapture(TEXT("OutsideBoundsPocketCapture"), NearestPocketLocation, PocketDistance, FColor::Yellow);
			StartBallPocketSink(Ball, NearestPocketLocation);
		return true;
	}

	if (bShouldForceCornerCapture)
	{
		if (Ball->IsCueBall())
		{
			DebugCapture(TEXT("ForcedCornerCapture"), NearestPocketLocation, PocketDistance, FColor::Purple);
			QueueCueBallRespawn(Ball, NearestPocketLocation);
			return true;
		}

		DebugCapture(TEXT("ForcedCornerCapture"), NearestPocketLocation, PocketDistance, FColor::Purple);
		StartBallPocketSink(Ball, NearestPocketLocation);
		return true;
	}

	if (bShouldCaptureInCornerFunnel)
	{
		if (Ball->IsCueBall())
		{
			DebugCapture(TEXT("CornerFunnelCapture"), NearestPocketLocation, PocketDistance, FColor::Green);
			QueueCueBallRespawn(Ball, NearestPocketLocation);
			return true;
		}

		DebugCapture(TEXT("CornerFunnelCapture"), NearestPocketLocation, PocketDistance, FColor::Green);
		StartBallPocketSink(Ball, NearestPocketLocation);
		return true;
	}

	if (bShouldCaptureOnCornerApproach)
	{
		if (Ball->IsCueBall())
		{
			DebugCapture(TEXT("CornerApproachCapture"), NearestPocketLocation, PocketDistance, FColor::Emerald);
			QueueCueBallRespawn(Ball, NearestPocketLocation);
			return true;
		}

		DebugCapture(TEXT("CornerApproachCapture"), NearestPocketLocation, PocketDistance, FColor::Emerald);
		StartBallPocketSink(Ball, NearestPocketLocation);
		return true;
	}

	if (bShouldCaptureCueBallOnCornerJaw)
	{
		DebugCapture(TEXT("CornerJawCueBallCapture"), NearestPocketLocation, PocketDistance, FColor::Cyan);
		QueueCueBallRespawn(Ball, NearestPocketLocation);
		return true;
	}

	if (bNearPocket && PocketDistance <= PocketCaptureDistance)
	{
		if (Ball->IsCueBall())
		{
			DebugCapture(TEXT("PocketRadiusCapture"), NearestPocketLocation, PocketDistance, FColor::Orange);
			QueueCueBallRespawn(Ball, NearestPocketLocation);
			return true;
		}

		DebugCapture(TEXT("PocketRadiusCapture"), NearestPocketLocation, PocketDistance, FColor::Orange);
		StartBallPocketSink(Ball, NearestPocketLocation);
		return true;
	}

	const float ClampedLong = FMath::Clamp(AlongLong, -SafeLongLimit, SafeLongLimit);
	const float ClampedShort = FMath::Clamp(AlongShort, -SafeShortLimit, SafeShortLimit);
	const FVector CorrectedLocation = SurfacePoint
		+ TableLongAxis * ClampedLong
		+ TableShortAxis * ClampedShort
		+ TableUpAxis * BallRadius;

	FVector ReflectedVelocity = FVector::VectorPlaneProject(Ball->GetLinearVelocity(), TableUpAxis);
	const float LongOverflow = FMath::Abs(AlongLong) - SafeLongLimit;
	const float ShortOverflow = FMath::Abs(AlongShort) - SafeShortLimit;

	if (LongOverflow >= ShortOverflow)
	{
		const float LongSpeed = FVector::DotProduct(ReflectedVelocity, TableLongAxis);
		if (FMath::Sign(LongSpeed) == FMath::Sign(AlongLong))
		{
			ReflectedVelocity -= TableLongAxis * (LongSpeed * (bOutsideOuterBounds ? 2.15f : 1.75f));
		}
	}
	else
	{
		const float ShortSpeed = FVector::DotProduct(ReflectedVelocity, TableShortAxis);
		if (FMath::Sign(ShortSpeed) == FMath::Sign(AlongShort))
		{
			ReflectedVelocity -= TableShortAxis * (ShortSpeed * (bOutsideOuterBounds ? 2.15f : 1.75f));
		}
	}

	DebugCorrection(CorrectedLocation, ReflectedVelocity, TEXT("ClampAndReflect"));
	Ball->TeleportBall(CorrectedLocation);
	Ball->SetLinearVelocity(ReflectedVelocity * (bOutsideOuterBounds ? 0.58f : 0.72f));
	return false;
}

bool APoolTableManager::FindNearestPocketLocation(const FVector& WorldLocation, FVector& OutPocketLocation, float& OutPlanarDistance) const
{
	if (PocketLocations.IsEmpty())
	{
		return false;
	}

	const FVector WorldPlanar = FVector::VectorPlaneProject(WorldLocation - SurfacePoint, TableUpAxis);
	float BestDistanceSquared = TNumericLimits<float>::Max();
	bool bFoundPocket = false;

	for (const FVector& PocketLocation : PocketLocations)
	{
		const FVector PocketPlanar = FVector::VectorPlaneProject(PocketLocation - SurfacePoint, TableUpAxis);
		const float DistanceSquared = FVector::DistSquared(PocketPlanar, WorldPlanar);
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			OutPocketLocation = PocketLocation;
			bFoundPocket = true;
		}
	}

	OutPlanarDistance = bFoundPocket ? FMath::Sqrt(BestDistanceSquared) : TNumericLimits<float>::Max();
	return bFoundPocket;
}

bool APoolTableManager::IsCueBallPlacementLocationValid(const FVector& DesiredLocation, const APoolBall* IgnoredBall) const
{
	const float CollisionHalfLength = FMath::Max(BallRadius * 7.0f, HalfPlayLength - CollisionInsetLong);
	const float CollisionHalfWidth = FMath::Max(BallRadius * 5.0f, HalfPlayWidth - CollisionInsetShort);
	const float SafeLongLimit = CollisionHalfLength - BallRadius * 1.25f;
	const float SafeShortLimit = CollisionHalfWidth - BallRadius * 1.25f;
	const FVector Relative = DesiredLocation - SurfacePoint;
	const float AlongLong = FVector::DotProduct(Relative, TableLongAxis);
	const float AlongShort = FVector::DotProduct(Relative, TableShortAxis);
	const float AlongUp = FVector::DotProduct(Relative, TableUpAxis);

	if (FMath::Abs(AlongLong) > SafeLongLimit || FMath::Abs(AlongShort) > SafeShortLimit || FMath::Abs(AlongUp - BallRadius) > BallRadius * 0.75f)
	{
		return false;
	}

	for (const APoolPocketTrigger* Pocket : PocketTriggers)
	{
		const USphereComponent* Sphere = Pocket ? Pocket->FindComponentByClass<USphereComponent>() : nullptr;
		if (!Sphere)
		{
			continue;
		}

		const float Clearance = FVector::Dist(DesiredLocation, Sphere->GetComponentLocation()) - Sphere->GetScaledSphereRadius();
		if (Clearance <= BallRadius * 0.5f)
		{
			return false;
		}
	}

	for (const APoolBall* Ball : SpawnedBalls)
	{
		if (!IsValid(Ball) || Ball == IgnoredBall || Ball->IsPocketed())
		{
			continue;
		}

		if (FVector::DistSquared(Ball->GetActorLocation(), DesiredLocation) < FMath::Square(BallRadius * 2.15f))
		{
			return false;
		}
	}

	return true;
}

FVector APoolTableManager::FindCueBallPlacementLocation(const FVector& PreferredLocation) const
{
	const FVector PreferredOnPlane =
		SurfacePoint
		+ TableLongAxis * FVector::DotProduct(PreferredLocation - SurfacePoint, TableLongAxis)
		+ TableShortAxis * FVector::DotProduct(PreferredLocation - SurfacePoint, TableShortAxis)
		+ TableUpAxis * BallRadius;
	if (IsCueBallPlacementLocationValid(PreferredOnPlane, CueBall))
	{
		return PreferredOnPlane;
	}

	const float MaxLong = FMath::Max(BallRadius * 7.0f, HalfPlayLength - CollisionInsetLong) - BallRadius * 1.3f;
	const float MaxShort = FMath::Max(BallRadius * 5.0f, HalfPlayWidth - CollisionInsetShort) - BallRadius * 1.3f;
	const float LongStep = BallRadius * 1.6f;
	const float ShortStep = BallRadius * 1.6f;

	for (int32 LongRing = 0; LongRing < 14; ++LongRing)
	{
		const float LongOffset = LongRing * LongStep;
		for (int32 ShortRing = 0; ShortRing < 14; ++ShortRing)
		{
			const float ShortOffset = ShortRing * ShortStep;
			for (const float LongSign : { -1.0f, 1.0f })
			{
				for (const float ShortSign : { -1.0f, 1.0f })
				{
					const FVector Candidate =
						SurfacePoint
						+ TableLongAxis * FMath::Clamp(LongSign * LongOffset, -MaxLong, MaxLong)
						+ TableShortAxis * FMath::Clamp(ShortSign * ShortOffset, -MaxShort, MaxShort)
						+ TableUpAxis * BallRadius;
					if (IsCueBallPlacementLocationValid(Candidate, CueBall))
					{
						return Candidate;
					}
				}
			}
		}
	}

	return CueBallStartLocation;
}

bool APoolTableManager::UpdateCueBallInHandPreview(const FVector& DesiredLocation)
{
	if (!bCueBallInHand || !IsValid(CueBall))
	{
		return false;
	}

	CueBallInHandLocation = FindCueBallPlacementLocation(DesiredLocation);
	CueBall->ResetBall(MakeBallTransform(CueBallInHandLocation));
	CueBall->SetLinearVelocity(FVector::ZeroVector);
	return true;
}

bool APoolTableManager::ConfirmCueBallPlacement(const FVector& DesiredLocation)
{
	if (!bCueBallInHand || !IsValid(CueBall))
	{
		return false;
	}

	const FVector Candidate =
		SurfacePoint
		+ TableLongAxis * FVector::DotProduct(DesiredLocation - SurfacePoint, TableLongAxis)
		+ TableShortAxis * FVector::DotProduct(DesiredLocation - SurfacePoint, TableShortAxis)
		+ TableUpAxis * BallRadius;
	if (!IsCueBallPlacementLocationValid(Candidate, CueBall))
	{
		return false;
	}

	CueBallInHandLocation = Candidate;
	CueBall->ResetBall(MakeBallTransform(CueBallInHandLocation));
	CueBall->SetLinearVelocity(FVector::ZeroVector);
	bCueBallInHand = false;
	return true;
}

void APoolTableManager::SpawnBalls()
{
	const float BallDiameter = BallRadius * 2.0f;
	const float RackGapFactor = 1.01f;
	const float LateralSpacing = BallDiameter * RackGapFactor;
	const float RowSpacing = FMath::Sqrt(3.0f) * BallRadius * RackGapFactor;
	const FVector BallPlaneOrigin = SurfacePoint + TableUpAxis * BallRadius;

	const TArray<FLinearColor> Colors = {
		FLinearColor(1.0f, 1.0f, 0.0f),
		FLinearColor(0.0f, 0.2f, 1.0f),
		FLinearColor(1.0f, 0.0f, 0.0f),
		FLinearColor(1.0f, 0.5f, 0.0f),
		FLinearColor(0.5f, 0.0f, 0.5f),
		FLinearColor(0.0f, 1.0f, 1.0f),
		FLinearColor(0.0f, 0.75f, 0.1f),
		FLinearColor(0.35f, 0.2f, 0.05f),
		FLinearColor(1.0f, 0.0f, 1.0f),
		FLinearColor(1.0f, 0.95f, 0.35f),
		FLinearColor(0.3f, 0.5f, 1.0f),
		FLinearColor(1.0f, 0.35f, 0.35f),
		FLinearColor(1.0f, 0.65f, 0.25f),
		FLinearColor(0.8f, 0.5f, 0.95f),
		FLinearColor(0.8f, 0.8f, 0.8f)
	};

	FActorSpawnParameters BallSpawnParams;
	BallSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	CueBall = GetWorld()->SpawnActor<APoolBall>(APoolBall::StaticClass(), MakeBallTransform(CueBallStartLocation), BallSpawnParams);
	if (CueBall)
	{
		CueBall->SetBallRadius(BallRadius);
		CueBall->SetMovementPlane(TableUpAxis, BallPlaneOrigin);
		CueBall->ConfigureBall(true, 0, FLinearColor::White);
		SpawnedBalls.Add(CueBall);
		InitialBallTransforms.Add(CueBall->GetActorTransform());
	}

	int32 BallIndex = 1;
	const FVector Apex = RackCenterLocation - TableLongAxis * (RowSpacing * 2.0f);
	for (int32 Row = 0; Row < 5; ++Row)
	{
		for (int32 Col = 0; Col <= Row; ++Col)
		{
			const FVector Position = Apex
				+ TableLongAxis * (Row * RowSpacing)
				+ TableShortAxis * ((Col - Row * 0.5f) * LateralSpacing)
				+ TableUpAxis * 0.0f;

			APoolBall* Ball = GetWorld()->SpawnActor<APoolBall>(APoolBall::StaticClass(), MakeBallTransform(Position), BallSpawnParams);
			if (Ball)
			{
				Ball->SetBallRadius(BallRadius);
				Ball->SetMovementPlane(TableUpAxis, BallPlaneOrigin);
				const FLinearColor Color = Colors.IsValidIndex(BallIndex - 1) ? Colors[BallIndex - 1] : FLinearColor::Gray;
				Ball->ConfigureBall(false, BallIndex, Color);
				SpawnedBalls.Add(Ball);
				InitialBallTransforms.Add(Ball->GetActorTransform());
				++BallIndex;
			}
		}
	}
}

void APoolTableManager::SpawnPockets()
{
	const float CornerPocketLongInset = FMath::Max(PocketRadius * 0.01f, BallRadius * 0.05f);
	const float CornerPocketShortInset = FMath::Max(PocketRadius * 0.01f, BallRadius * 0.05f);
	const float SidePocketShortInset = FMath::Max(PocketRadius * 0.02f, BallRadius * 0.1f);
	const float CornerLong = FMath::Max(0.0f, HalfOuterLength - CornerPocketLongInset);
	const float CornerShort = FMath::Max(0.0f, HalfOuterWidth - CornerPocketShortInset);
	const float MiddleShort = FMath::Max(0.0f, HalfPlayWidth - SidePocketShortInset);
	const FVector TriggerDown = -TableUpAxis * (BallRadius * 0.95f);

	const TArray<FVector> PocketPositions = {
		MakeTablePoint(CornerLong, CornerShort) + TriggerDown,
		MakeTablePoint(CornerLong, -CornerShort) + TriggerDown,
		MakeTablePoint(-CornerLong, CornerShort) + TriggerDown,
		MakeTablePoint(-CornerLong, -CornerShort) + TriggerDown,
		MakeTablePoint(0.0f, MiddleShort) + TriggerDown,
		MakeTablePoint(0.0f, -MiddleShort) + TriggerDown
	};

	PocketLocations = PocketPositions;

	for (const FVector& PocketPosition : PocketPositions)
	{
		if (APoolPocketTrigger* Pocket = GetWorld()->SpawnActor<APoolPocketTrigger>(APoolPocketTrigger::StaticClass(), PocketPosition, FRotator::ZeroRotator))
		{
			Pocket->SetManager(this);
			const FVector PocketOffset = PocketPosition - SurfacePoint;
			const bool bIsSidePocket = FMath::Abs(FVector::DotProduct(PocketOffset.GetSafeNormal(), TableLongAxis)) < 0.35f;
			Pocket->SetPocketRadius(PocketRadius * (bIsSidePocket ? SidePocketTriggerMultiplier : CornerPocketTriggerMultiplier));
			PocketTriggers.Add(Pocket);
		}
	}
}

void APoolTableManager::SpawnWallSegment(const FVector& Center, const FVector& Extent, const FVector& Direction)
{
	if (!GetWorld() || Direction.IsNearlyZero())
	{
		return;
	}

	const FRotator Rotation = FRotationMatrix::MakeFromXZ(Direction.GetSafeNormal(), TableUpAxis).Rotator();
	if (APoolCushionWall* Wall = GetWorld()->SpawnActor<APoolCushionWall>(APoolCushionWall::StaticClass(), Center, Rotation))
	{
		Wall->ConfigureWall(Extent);
		CushionWalls.Add(Wall);
	}
}

void APoolTableManager::SpawnWalls()
{
	const float CollisionHalfLength = FMath::Max(BallRadius * 7.0f, HalfPlayLength - CollisionInsetLong);
	const float CollisionHalfWidth = FMath::Max(BallRadius * 5.0f, HalfPlayWidth - CollisionInsetShort);
	const float Thickness = FMath::Max(5.2f, BallRadius * 2.1f);
	const float Height = BallRadius * 6.2f;
	const float FloorHalfHeight = BallRadius * 0.7f;
	const float WallCenterUp = Height * 0.5f;
	const float RailInset = FMath::Max(Thickness * 0.2f, BallRadius * 0.12f);
	const float RailLong = FMath::Max(BallRadius * 6.0f, CollisionHalfLength - RailInset);
	const float RailShort = FMath::Max(BallRadius * 4.0f, CollisionHalfWidth - RailInset);
	const float CornerGap = FMath::Max(BallRadius * 5.0f, PocketRadius * CornerPocketGapMultiplier);
	const float MiddleGap = FMath::Max(BallRadius * 4.0f, PocketRadius * SidePocketGapMultiplier);
	const float CornerJawLength = PocketRadius * 0.8f;
	const float SideJawLength = PocketRadius * SideJawLengthMultiplier;
	const float CornerJawCenterOffset = 0.0f;
	const float CornerTrim = FMath::Clamp(CornerRailTrim, 0.0f, PocketRadius * 0.9f);
	const float LongStraightHalf = FMath::Max(BallRadius * 1.8f, (RailLong - CornerGap - MiddleGap) * 0.5f - CornerTrim * 0.5f);
	const float ShortStraightHalf = FMath::Max(BallRadius * 1.8f, RailShort - CornerGap - CornerTrim);
	const FRotator FloorRotation = FRotationMatrix::MakeFromXZ(TableLongAxis, TableUpAxis).Rotator();

	PlaySurfaceFloor = GetWorld()->SpawnActor<APoolCushionWall>(APoolCushionWall::StaticClass(), MakeTablePoint(0.0f, 0.0f, -FloorHalfHeight), FloorRotation);
	if (PlaySurfaceFloor)
	{
		PlaySurfaceFloor->ConfigureWall(FVector(RailLong + Thickness, RailShort + Thickness, FloorHalfHeight));
	}

	for (const float ShortSign : { -1.0f, 1.0f })
	{
		SpawnWallSegment(MakeTablePoint(MiddleGap + LongStraightHalf, ShortSign * RailShort, WallCenterUp), FVector(LongStraightHalf, Thickness, Height), TableLongAxis);
		SpawnWallSegment(MakeTablePoint(-(MiddleGap + LongStraightHalf), ShortSign * RailShort, WallCenterUp), FVector(LongStraightHalf, Thickness, Height), TableLongAxis);

		for (const float LongSign : { -1.0f, 1.0f })
		{
			const FVector JawDirection = (-LongSign * TableLongAxis - ShortSign * TableShortAxis).GetSafeNormal();
			SpawnWallSegment(
				MakeTablePoint(LongSign * (MiddleGap + SideJawLength * 0.55f), ShortSign * (RailShort - Thickness * 0.35f), WallCenterUp),
				FVector(SideJawLength, Thickness, Height),
				JawDirection);
		}
	}

	for (const float LongSign : { -1.0f, 1.0f })
	{
		SpawnWallSegment(MakeTablePoint(LongSign * RailLong, 0.0f, WallCenterUp), FVector(ShortStraightHalf, Thickness, Height), TableShortAxis);

		for (const float ShortSign : { -1.0f, 1.0f })
		{
			const FVector JawDirection = (-LongSign * TableLongAxis - ShortSign * TableShortAxis).GetSafeNormal();
			SpawnWallSegment(
				MakeTablePoint(LongSign * (RailLong - CornerJawCenterOffset), ShortSign * (RailShort - CornerJawCenterOffset), WallCenterUp),
				FVector(CornerJawLength, Thickness, Height),
				JawDirection);
		}
	}
}

void APoolTableManager::SpawnTableSupports()
{
	if (!GetWorld())
	{
		return;
	}

	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Bois.Bois"));
	if (!BaseMaterial)
	{
		BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}
	if (!CubeMesh)
	{
		return;
	}

	float MeshHalfHeight = 24.0f;
	if (TableMeshComponent && TableMeshComponent->GetStaticMesh())
	{
		MeshHalfHeight = TableMeshComponent->GetStaticMesh()->GetBoundingBox().GetExtent().Z * TableMeshComponent->GetComponentScale().GetAbs().Z;
	}

	const FVector TableBottomCenter = TableCenter - TableUpAxis * MeshHalfHeight;
	const FVector TraceStart = TableBottomCenter + TableUpAxis * 4.0f;
	const FVector TraceEnd = TableBottomCenter - TableUpAxis * 250.0f;
	FCollisionQueryParams FloorTraceParams(SCENE_QUERY_STAT(PoolTableSupportFloorTrace), false, this);
	if (IsValid(TableActor))
	{
		FloorTraceParams.AddIgnoredActor(TableActor);
	}

	FHitResult FloorHit;
	float LegHeight = 58.0f;
	if (GetWorld()->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, ECC_Visibility, FloorTraceParams))
	{
		LegHeight = FVector::DotProduct(TraceStart - FloorHit.ImpactPoint, TableUpAxis) + 2.0f;
	}

	const float LegWidth = FMath::Max(14.0f, BallRadius * 4.8f);
	LegHeight = FMath::Clamp(LegHeight, 26.0f, 96.0f);
	const float LegHalfHeight = LegHeight * 0.5f;
	const float LegHalfWidth = LegWidth * 0.5f;
	const float LegInsetLong = FMath::Max(LegHalfWidth + 6.0f, HalfOuterLength - 16.0f);
	const float LegInsetShort = FMath::Max(LegHalfWidth + 6.0f, HalfOuterWidth - 12.0f);
	const FVector SupportCenterBase = TableBottomCenter - TableUpAxis * (LegHalfHeight - 3.0f);
	const FRotator SupportRotation = FRotationMatrix::MakeFromXZ(TableLongAxis, TableUpAxis).Rotator();

	auto ConfigureSupport = [&](const FVector& Location, const FVector& Dimensions)
	{
		AStaticMeshActor* Support = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, SupportRotation);
		if (!Support)
		{
			return;
		}

		UStaticMeshComponent* MeshComp = Support->GetStaticMeshComponent();
		MeshComp->SetMobility(EComponentMobility::Movable);
		MeshComp->SetStaticMesh(CubeMesh);
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComp->SetWorldScale3D(FVector(Dimensions.X / 100.0f, Dimensions.Y / 100.0f, Dimensions.Z / 100.0f));
		if (BaseMaterial)
		{
			if (BaseMaterial->GetPathName().Contains(TEXT("/Engine/BasicShapes/")))
			{
				if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMaterial, Support))
				{
					const FLinearColor WoodTint(0.18f, 0.08f, 0.04f);
					MID->SetVectorParameterValue(TEXT("Color"), WoodTint);
					MID->SetVectorParameterValue(TEXT("BaseColor"), WoodTint);
					MeshComp->SetMaterial(0, MID);
				}
			}
			else
			{
				MeshComp->SetMaterial(0, BaseMaterial);
			}
		}
		SupportActors.Add(Support);
	};

	for (const float LongSign : { -1.0f, 1.0f })
	{
		for (const float ShortSign : { -1.0f, 1.0f })
		{
			ConfigureSupport(
				SupportCenterBase + TableLongAxis * (LongSign * LegInsetLong) + TableShortAxis * (ShortSign * LegInsetShort),
				FVector(LegWidth, LegWidth, LegHeight));
		}
	}

	ConfigureSupport(
		TableBottomCenter - TableUpAxis * FMath::Min(LegHeight * 0.38f, 16.0f),
		FVector(HalfOuterLength * 0.65f, 14.0f, 12.0f));
}

bool APoolTableManager::AreBallsStopped() const
{
	for (const APoolBall* Ball : SpawnedBalls)
	{
		if (IsValid(Ball) && Ball->IsMoving())
		{
			return false;
		}
	}
	return true;
}

void APoolTableManager::ResetRack()
{
	DestroySpawnedActors();
	BuildTableData();
	SpawnTableSupports();
	SpawnWalls();
	SpawnPockets();
	SpawnBalls();
	if (MatchMode == EPoolMatchMode::LocalVersus)
	{
		ApplyCurrentPlayerView();
	}
}

void APoolTableManager::HandleBallPocketed(APoolBall* Ball, const FVector& PocketLocation)
{
	if (!IsValid(Ball))
	{
		return;
	}

	if (Ball->IsCueBall())
	{
		if (IsCueBallScratchAlreadyHandled(Ball))
		{
			UE_LOG(
				LogTemp,
				Verbose,
				TEXT("CueBall scratch ignored because hand-in-ball is already active: ball=%s pocket=%s"),
				*GetNameSafe(Ball),
				*PocketLocation.ToCompactString());
			return;
		}

		UE_LOG(
			LogTemp,
			Display,
			TEXT("CueBall scratch detected: ball=%s pocket=%s velocity=%s"),
			*GetNameSafe(Ball),
			*PocketLocation.ToCompactString(),
			*Ball->GetLinearVelocity().ToCompactString());
		QueueCueBallRespawn(Ball, PocketLocation);
		if (MatchMode == EPoolMatchMode::LocalVersus)
		{
			bScratchCommittedThisTurn = true;
		}
		return;
	}

	if (MatchMode == EPoolMatchMode::LocalVersus)
	{
		RecordLocalPocketedBall(Ball);
	}
	StartBallPocketSink(Ball, PocketLocation);
}

bool APoolTableManager::IsCueBallScratchAlreadyHandled(const APoolBall* Ball) const
{
	if (!IsValid(Ball))
	{
		return false;
	}

	return Ball == CueBall && (bCueBallRespawnPending || bCueBallInHand || PendingCueBallRespawn.Get() == Ball);
}

bool APoolTableManager::RegisterObjectBallPocketed(APoolBall* Ball)
{
	if (!IsValid(Ball) || Ball->IsCueBall())
	{
		return false;
	}

	if (CountedPocketedBalls.Contains(Ball))
	{
		return false;
	}

	CountedPocketedBalls.Add(Ball);
	++PocketedBallCount;
	return true;
}

void APoolTableManager::StartBallPocketSink(APoolBall* Ball, const FVector& PocketLocation)
{
	if (!IsValid(Ball) || Ball->IsCueBall())
	{
		return;
	}

	RegisterObjectBallPocketed(Ball);
	if (!Ball->IsPocketed() && !Ball->IsSinkingIntoPocket())
	{
		Ball->BeginPocketSink(PocketLocation - TableUpAxis * (BallRadius * PocketSinkDepthMultiplier));
	}
}

void APoolTableManager::SetMatchMode(EPoolMatchMode NewMode)
{
	if (MatchMode == NewMode)
	{
		return;
	}

	MatchMode = NewMode;
	ResetRack();
}

void APoolTableManager::NotifyShotTaken(const FTransform& PlayerTransform, const FRotator& ControlRotation)
{
	if (MatchMode != EPoolMatchMode::LocalVersus || bMatchFinished)
	{
		return;
	}

	if (ActivePlayer == EPoolPlayerSide::Blue)
	{
		BlueSavedTransform = PlayerTransform;
		BlueSavedControlRotation = ControlRotation;
		bHasBlueSavedView = true;
	}
	else
	{
		RedSavedTransform = PlayerTransform;
		RedSavedControlRotation = ControlRotation;
		bHasRedSavedView = true;
	}

	PocketedThisTurn.Reset();
	bScratchCommittedThisTurn = false;
	bBlackPocketedThisTurn = false;
	bTurnResolutionPending = true;
}

FString APoolTableManager::GetHUDTurnText() const
{
	if (MatchMode == EPoolMatchMode::Training)
	{
		return TEXT("Tryb treningowy");
	}

	if (bMatchFinished)
	{
		return FString::Printf(TEXT("Koniec gry - wygrał gracz %s"), *GetPlayerLabel(WinningPlayer));
	}

	const EPoolBallGroup AssignedGroup = GetAssignedGroup(ActivePlayer);
	if (AssignedGroup == EPoolBallGroup::Yellow)
	{
		return FString::Printf(TEXT("Tura: Gracz %s (żółte)"), *GetPlayerLabel(ActivePlayer));
	}

	if (AssignedGroup == EPoolBallGroup::Red)
	{
		return FString::Printf(TEXT("Tura: Gracz %s (czerwone)"), *GetPlayerLabel(ActivePlayer));
	}

	return FString::Printf(TEXT("Tura: Gracz %s"), *GetPlayerLabel(ActivePlayer));
}

FString APoolTableManager::GetHUDOpponentText() const
{
	if (MatchMode != EPoolMatchMode::LocalVersus || bMatchFinished)
	{
		return TEXT("");
	}

	return GetPlayerCameraLabel(GetOpponent(ActivePlayer));
}

FString APoolTableManager::GetHUDWinnerText() const
{
	if (MatchMode != EPoolMatchMode::LocalVersus || !bMatchFinished)
	{
		return TEXT("");
	}

	return FString::Printf(TEXT("Wygrał gracz %s. Rozgrywka zakończona."), *GetPlayerLabel(WinningPlayer));
}

void APoolTableManager::WriteMatchStateToSaveGame(UPoolSaveGame& SaveGame) const
{
	SaveGame.MatchMode = MatchMode;
	SaveGame.ActivePlayer = ActivePlayer;
	SaveGame.Winner = WinningPlayer;
	SaveGame.BlueAssignedGroup = BlueAssignedGroup;
	SaveGame.RedAssignedGroup = RedAssignedGroup;
	SaveGame.BluePocketedCount = BluePocketedCount;
	SaveGame.RedPocketedCount = RedPocketedCount;
	SaveGame.bMatchFinished = bMatchFinished;
	SaveGame.BluePlayerTransform = BlueSavedTransform;
	SaveGame.BlueControlRotation = BlueSavedControlRotation;
	SaveGame.bHasBluePlayerState = bHasBlueSavedView;
	SaveGame.RedPlayerTransform = RedSavedTransform;
	SaveGame.RedControlRotation = RedSavedControlRotation;
	SaveGame.bHasRedPlayerState = bHasRedSavedView;
}

void APoolTableManager::LoadMatchStateFromSaveGame(const UPoolSaveGame& SaveGame)
{
	MatchMode = SaveGame.MatchMode;
	ActivePlayer = SaveGame.ActivePlayer;
	WinningPlayer = SaveGame.Winner;
	BlueAssignedGroup = SaveGame.BlueAssignedGroup;
	RedAssignedGroup = SaveGame.RedAssignedGroup;
	BluePocketedCount = SaveGame.BluePocketedCount;
	RedPocketedCount = SaveGame.RedPocketedCount;
	bMatchFinished = SaveGame.bMatchFinished;
	BlueSavedTransform = SaveGame.BluePlayerTransform;
	BlueSavedControlRotation = SaveGame.BlueControlRotation;
	bHasBlueSavedView = SaveGame.bHasBluePlayerState;
	RedSavedTransform = SaveGame.RedPlayerTransform;
	RedSavedControlRotation = SaveGame.RedControlRotation;
	bHasRedSavedView = SaveGame.bHasRedPlayerState;
	bTurnResolutionPending = false;
	bScratchCommittedThisTurn = false;
	bBlackPocketedThisTurn = false;
	PocketedThisTurn.Reset();
}

void APoolTableManager::ResolveLocalMatchTurn()
{
	if (MatchMode != EPoolMatchMode::LocalVersus || bMatchFinished)
	{
		bTurnResolutionPending = false;
		return;
	}

	const EPoolPlayerSide Shooter = ActivePlayer;
	const EPoolPlayerSide Opponent = GetOpponent(Shooter);
	bool bSwitchTurn = bScratchCommittedThisTurn;
	bool bShooterPocketedOwnGroup = false;
	bool bAssignedThisShot = false;

	for (const TWeakObjectPtr<APoolBall>& BallPtr : PocketedThisTurn)
	{
		APoolBall* Ball = BallPtr.Get();
		if (!IsValid(Ball))
		{
			continue;
		}

		const EPoolBallGroup Group = GetBallGroupForBall(Ball);
		if (Group == EPoolBallGroup::Black)
		{
			FinishLocalMatch(Opponent, FString::Printf(TEXT("Gracz %s wbił czarną bilę."), *GetPlayerLabel(Shooter)));
			bTurnResolutionPending = false;
			return;
		}

		EPoolBallGroup ShooterGroup = GetAssignedGroup(Shooter);
		if (ShooterGroup == EPoolBallGroup::Unassigned)
		{
			SetAssignedGroup(Shooter, Group);
			SetAssignedGroup(Opponent, Group == EPoolBallGroup::Yellow ? EPoolBallGroup::Red : EPoolBallGroup::Yellow);
			ShooterGroup = Group;
			bAssignedThisShot = true;
		}

		if (Group == ShooterGroup)
		{
			SetPlayerPocketedCount(Shooter, GetPocketedCountForPlayer(Shooter) + 1);
			bShooterPocketedOwnGroup = true;
		}
		else
		{
			SetPlayerPocketedCount(Opponent, GetPocketedCountForPlayer(Opponent) + 1);
			bSwitchTurn = true;
		}
	}

	if (GetPocketedCountForPlayer(Shooter) >= 7)
	{
		FinishLocalMatch(Shooter, FString::Printf(TEXT("Gracz %s wbił wszystkie swoje bile."), *GetPlayerLabel(Shooter)));
		bTurnResolutionPending = false;
		return;
	}

	if (GetPocketedCountForPlayer(Opponent) >= 7)
	{
		FinishLocalMatch(Opponent, FString::Printf(TEXT("Gracz %s wbił wszystkie swoje bile."), *GetPlayerLabel(Opponent)));
		bTurnResolutionPending = false;
		return;
	}

	if (!bShooterPocketedOwnGroup || bSwitchTurn || (!bAssignedThisShot && PocketedThisTurn.Num() == 0))
	{
		ActivePlayer = Opponent;
	}

	PocketedThisTurn.Reset();
	bScratchCommittedThisTurn = false;
	bBlackPocketedThisTurn = false;
	bTurnResolutionPending = false;
	ApplyCurrentPlayerView();
}

void APoolTableManager::ApplyCurrentPlayerView()
{
	if (MatchMode != EPoolMatchMode::LocalVersus || bMatchFinished)
	{
		return;
	}

	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	AMyCharacter* MyCharacter = PlayerController ? Cast<AMyCharacter>(PlayerController->GetPawn()) : nullptr;
	if (!MyCharacter)
	{
		return;
	}

	bool bHasSavedView = false;
	FTransform DesiredTransform = FTransform::Identity;
	FRotator DesiredRotation = FRotator::ZeroRotator;

	if (ActivePlayer == EPoolPlayerSide::Blue && bHasBlueSavedView)
	{
		bHasSavedView = true;
		DesiredTransform = BlueSavedTransform;
		DesiredRotation = BlueSavedControlRotation;
	}
	else if (ActivePlayer == EPoolPlayerSide::Red && bHasRedSavedView)
	{
		bHasSavedView = true;
		DesiredTransform = RedSavedTransform;
		DesiredRotation = RedSavedControlRotation;
	}

	if (!bHasSavedView)
	{
		const FVector AimLocation = GetCueBallStartLocation() - GetTableLongAxis() * 190.0f;
		const FVector Desired = GetCueBallStartLocation() - AimLocation;
		DesiredTransform = FTransform(FRotator(0.0f, Desired.Rotation().Yaw, 0.0f), FVector(AimLocation.X, AimLocation.Y, MyCharacter->GetActorLocation().Z));
		DesiredRotation = Desired.Rotation();
	}

	MyCharacter->ApplyExternalView(DesiredTransform, DesiredRotation);
}

void APoolTableManager::ResetLocalMatchState()
{
	ActivePlayer = EPoolPlayerSide::Blue;
	WinningPlayer = EPoolPlayerSide::Blue;
	BlueAssignedGroup = EPoolBallGroup::Unassigned;
	RedAssignedGroup = EPoolBallGroup::Unassigned;
	BluePocketedCount = 0;
	RedPocketedCount = 0;
	bTurnResolutionPending = false;
	bScratchCommittedThisTurn = false;
	bBlackPocketedThisTurn = false;
	bMatchFinished = false;
	PocketedThisTurn.Reset();
	BlueSavedTransform = FTransform::Identity;
	BlueSavedControlRotation = FRotator::ZeroRotator;
	bHasBlueSavedView = false;
	RedSavedTransform = FTransform::Identity;
	RedSavedControlRotation = FRotator::ZeroRotator;
	bHasRedSavedView = false;
}

void APoolTableManager::RecordLocalPocketedBall(APoolBall* Ball)
{
	if (!IsValid(Ball) || Ball->IsCueBall())
	{
		return;
	}

	PocketedThisTurn.Add(Ball);
	if (GetBallGroupForBall(Ball) == EPoolBallGroup::Black)
	{
		bBlackPocketedThisTurn = true;
	}
}

EPoolBallGroup APoolTableManager::GetBallGroupForBall(const APoolBall* Ball) const
{
	if (!IsValid(Ball) || Ball->IsCueBall())
	{
		return EPoolBallGroup::Unassigned;
	}

	if (Ball->GetBallNumber() == 8)
	{
		return EPoolBallGroup::Black;
	}

	return Ball->GetBallNumber() <= 7 ? EPoolBallGroup::Yellow : EPoolBallGroup::Red;
}

FString APoolTableManager::GetPlayerLabel(EPoolPlayerSide Player) const
{
	return Player == EPoolPlayerSide::Blue ? TEXT("Niebieski") : TEXT("Czerwony");
}

FString APoolTableManager::GetPlayerCameraLabel(EPoolPlayerSide Player) const
{
	return FString::Printf(TEXT("Przeciwnik %s"), *GetPlayerLabel(Player));
}

EPoolPlayerSide APoolTableManager::GetOpponent(EPoolPlayerSide Player) const
{
	return Player == EPoolPlayerSide::Blue ? EPoolPlayerSide::Red : EPoolPlayerSide::Blue;
}

bool APoolTableManager::IsAnyBallAnimating() const
{
	for (const APoolBall* Ball : SpawnedBalls)
	{
		if (IsValid(Ball) && Ball->IsSinkingIntoPocket())
		{
			return true;
		}
	}

	return false;
}

int32 APoolTableManager::GetPocketedCountForPlayer(EPoolPlayerSide Player) const
{
	return Player == EPoolPlayerSide::Blue ? BluePocketedCount : RedPocketedCount;
}

void APoolTableManager::SetPlayerPocketedCount(EPoolPlayerSide Player, int32 NewCount)
{
	if (Player == EPoolPlayerSide::Blue)
	{
		BluePocketedCount = FMath::Clamp(NewCount, 0, 7);
	}
	else
	{
		RedPocketedCount = FMath::Clamp(NewCount, 0, 7);
	}
}

EPoolBallGroup APoolTableManager::GetAssignedGroup(EPoolPlayerSide Player) const
{
	return Player == EPoolPlayerSide::Blue ? BlueAssignedGroup : RedAssignedGroup;
}

void APoolTableManager::SetAssignedGroup(EPoolPlayerSide Player, EPoolBallGroup Group)
{
	if (Player == EPoolPlayerSide::Blue)
	{
		BlueAssignedGroup = Group;
	}
	else
	{
		RedAssignedGroup = Group;
	}
}

void APoolTableManager::FinishLocalMatch(EPoolPlayerSide WinningSide, const FString& Reason)
{
	bMatchFinished = true;
	WinningPlayer = WinningSide;
	bTurnResolutionPending = false;
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, Reason);
	}
}

void APoolTableManager::BeginCueBallInHand(APoolBall* Ball)
{
	if (!IsValid(Ball))
	{
		return;
	}

	bCueBallInHand = true;
	CueBall = Ball;
	CueBallInHandLocation = FindCueBallPlacementLocation(CueBallStartLocation);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("CueBallInHand enabled: ball=%s location=%s pendingSettle=%.2f"),
		*GetNameSafe(Ball),
		*CueBallInHandLocation.ToCompactString(),
		CueBallInHandSettleTimer);
	Ball->ResetBall(MakeBallTransform(CueBallInHandLocation));
	Ball->SetLinearVelocity(FVector::ZeroVector);
}

void APoolTableManager::QueueCueBallRespawn(APoolBall* Ball, const FVector& PocketLocation)
{
	if (!IsValid(Ball))
	{
		return;
	}

	if (IsCueBallScratchAlreadyHandled(Ball))
	{
		UE_LOG(
			LogTemp,
			Verbose,
			TEXT("CueBall respawn already pending or active; ignoring duplicate queue: ball=%s pocket=%s"),
			*GetNameSafe(Ball),
			*PocketLocation.ToCompactString());
		return;
	}

	PendingCueBallRespawn = Ball;
	PendingCueBallRespawnTransform = InitialBallTransforms.IsValidIndex(0)
		? InitialBallTransforms[0]
		: MakeBallTransform(CueBallStartLocation);
	CueBallRespawnTimer = FMath::Max(0.0f, CueBallRespawnDelay);
	CueBallInHandSettleTimer = 0.0f;
	bCueBallRespawnPending = true;
	bCueBallInHand = false;
	CueBall = Ball;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("CueBall respawn queued: ball=%s delay=%.2f pocket=%s"),
		*GetNameSafe(Ball),
		CueBallRespawnTimer,
		*PocketLocation.ToCompactString());

	if (!Ball->IsPocketed() && !Ball->IsSinkingIntoPocket())
	{
		Ball->BeginPocketSink(PocketLocation - TableUpAxis * (BallRadius * PocketSinkDepthMultiplier));
	}
}

void APoolTableManager::UpdateCueBallRespawn(float DeltaTime)
{
	if (!bCueBallRespawnPending)
	{
		return;
	}

	APoolBall* Ball = PendingCueBallRespawn.Get();
	if (!IsValid(Ball))
	{
		PendingCueBallRespawn.Reset();
		bCueBallRespawnPending = false;
		CueBallRespawnTimer = 0.0f;
		return;
	}

	if (Ball->IsSinkingIntoPocket())
	{
		return;
	}

	CueBallRespawnTimer = FMath::Max(0.0f, CueBallRespawnTimer - DeltaTime);
	if (CueBallRespawnTimer > KINDA_SMALL_NUMBER)
	{
		return;
	}

	if (!AreBallsSettledForCueBallInHand())
	{
		CueBallInHandSettleTimer += DeltaTime;
		UE_LOG(
			LogTemp,
			Verbose,
			TEXT("CueBallInHand waiting for settle: elapsed=%.2f timeout=%.2f"),
			CueBallInHandSettleTimer,
			CueBallInHandSettleTimeout);
		if (CueBallInHandSettleTimer < FMath::Max(0.0f, CueBallInHandSettleTimeout))
		{
			return;
		}

		UE_LOG(
			LogTemp,
			Display,
			TEXT("CueBallInHand settle timeout reached: elapsed=%.2f timeout=%.2f"),
			CueBallInHandSettleTimer,
			CueBallInHandSettleTimeout);
	}

	BeginCueBallInHand(Ball);
	PendingCueBallRespawn.Reset();
	PendingCueBallRespawnTransform = FTransform::Identity;
	CueBallRespawnTimer = 0.0f;
	CueBallInHandSettleTimer = 0.0f;
	bCueBallRespawnPending = false;
}

bool APoolTableManager::AreBallsSettledForCueBallInHand() const
{
	// Scratch in a corner can leave other balls with residual spin after they have
	// effectively stopped translating across the cloth. That spin should not block
	// entering hand-in-ball mode.
	constexpr float MaxPlacementBlockingLinearSpeed = 2.0f;
	const float MaxPlacementBlockingSpeedSq = FMath::Square(MaxPlacementBlockingLinearSpeed);
	const APoolBall* PendingCueBall = PendingCueBallRespawn.Get();

	for (const APoolBall* Ball : SpawnedBalls)
	{
		if (!IsValid(Ball) || Ball->IsPocketed() || Ball == PendingCueBall)
		{
			continue;
		}

		if (Ball->GetLinearVelocity().SizeSquared() > MaxPlacementBlockingSpeedSq)
		{
			return false;
		}
	}

	return true;
}

bool APoolTableManager::ApplySavedBallStates(const TArray<FPoolBallSaveState>& SavedStates, int32 SavedPocketedBallCount)
{
	CountedPocketedBalls.Reset();

	const float MaxLongDistance = HalfPlayLength + BallRadius * 6.0f;
	const float MaxShortDistance = HalfPlayWidth + BallRadius * 6.0f;
	const float MaxUpDistance = BallRadius * 5.0f;
	const float MinUpDistance = -BallRadius * 2.0f;

	for (const FPoolBallSaveState& SavedState : SavedStates)
	{
		if (SavedState.bPocketed)
		{
			continue;
		}

		const FVector Location = SavedState.Transform.GetLocation();
		const FVector RelativeToSurface = Location - SurfacePoint;
		const float AlongLong = FVector::DotProduct(RelativeToSurface, TableLongAxis);
		const float AlongShort = FVector::DotProduct(RelativeToSurface, TableShortAxis);
		const float AlongUp = FVector::DotProduct(RelativeToSurface, TableUpAxis);
		if (FMath::Abs(AlongLong) > MaxLongDistance
			|| FMath::Abs(AlongShort) > MaxShortDistance
			|| AlongUp < MinUpDistance
			|| AlongUp > MaxUpDistance)
		{
			return false;
		}
	}

	TMap<int32, APoolBall*> BallLookup;
	for (APoolBall* Ball : SpawnedBalls)
	{
		if (IsValid(Ball))
		{
			BallLookup.Add(Ball->IsCueBall() ? 0 : Ball->GetBallNumber(), Ball);
		}
	}

	for (const FPoolBallSaveState& SavedState : SavedStates)
	{
		const int32 BallKey = SavedState.bCueBall ? 0 : SavedState.BallNumber;
		APoolBall* const* FoundBall = BallLookup.Find(BallKey);
		if (!FoundBall || !IsValid(*FoundBall))
		{
			continue;
		}

		APoolBall* Ball = *FoundBall;
		if (SavedState.bCueBall && SavedState.bPocketed)
		{
			Ball->ResetBall(MakeBallTransform(CueBallStartLocation));
			CueBall = Ball;
			continue;
		}

		Ball->ResetBall(SavedState.Transform);
		if (SavedState.bPocketed)
		{
			Ball->PocketBall();
			if (!SavedState.bCueBall)
			{
				CountedPocketedBalls.Add(Ball);
			}
		}
		else if (SavedState.bCueBall)
		{
			CueBall = Ball;
		}
	}

	if (SavedPocketedBallCount <= 0)
	{
		CountedPocketedBalls.Reset();
	}
	PocketedBallCount = FMath::Clamp(SavedPocketedBallCount, 0, 15);
	return true;
}
