#include "PoolTableManager.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "PoolBall.h"
#include "PoolCushionWall.h"
#include "PoolPocketTrigger.h"

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
	SpawnParams.Name = TEXT("RuntimePoolTable");

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

	TableMeshComponent->SetStaticMesh(TableVisualMesh);
	TableMeshComponent->SetMobility(EComponentMobility::Movable);
	TableMeshComponent->SetWorldScale3D(FVector(1.0f));
	TableMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnedVisualTable->SetActorRotation(FixedTableRotation);
	SpawnedVisualTable->SetActorLocation(FixedTableLocation);
}

APoolTableManager::APoolTableManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APoolTableManager::BeginPlay()
{
	Super::BeginPlay();
	ResetRack();
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
		HalfPlayLength = 82.0f;
		HalfPlayWidth = 36.0f;
		BallRadius = 3.1f;
		PocketRadius = 7.2f;
		SurfacePoint = FixedTableLocation + FVector(0.0f, 0.0f, 44.0f);
	}
	else
	{
		const FTransform MeshTransform = TableMeshComponent->GetComponentTransform();
		TableUpAxis = MeshTransform.GetUnitAxis(EAxis::Z).GetSafeNormal();
		const FVector ForwardAxis = FVector::VectorPlaneProject(MeshTransform.GetUnitAxis(EAxis::X), TableUpAxis).GetSafeNormal();
		const FVector RightAxis = FVector::VectorPlaneProject(MeshTransform.GetUnitAxis(EAxis::Y), TableUpAxis).GetSafeNormal();

		const FBox BoundsBox = TableMeshComponent->GetStaticMesh()->GetBoundingBox();
		const FVector LocalExtent = BoundsBox.GetExtent() * TableMeshComponent->GetComponentScale().GetAbs();
		const bool bXAxisIsLong = LocalExtent.X >= LocalExtent.Y;

		TableLongAxis = bXAxisIsLong ? ForwardAxis : RightAxis;
		TableShortAxis = bXAxisIsLong ? RightAxis : ForwardAxis;
		HalfOuterLength = bXAxisIsLong ? LocalExtent.X : LocalExtent.Y;
		HalfOuterWidth = bXAxisIsLong ? LocalExtent.Y : LocalExtent.X;
		TableCenter = TableMeshComponent->Bounds.Origin;

		// Deterministic playfield, tied to the spawned table visual.
		HalfPlayLength = HalfOuterLength * PlayAreaLengthScale;
		HalfPlayWidth = HalfOuterWidth * PlayAreaWidthScale;
		BallRadius = FMath::Clamp(HalfPlayWidth * BallRadiusScaleOfWidth, 2.8f, 3.5f);
		PocketRadius = BallRadius * PocketRadiusMultiplier;

		// Use world bounds so placement stays correct even if the mesh pivot is not centered on the felt.
		SurfacePoint = TableCenter + TableUpAxis * (LocalExtent.Z * SurfaceHeightScale);
	}

	CueBallStartLocation = SurfacePoint - TableLongAxis * (HalfPlayLength * CueBallLengthFactor) + TableUpAxis * (BallRadius + 0.35f);
	RackCenterLocation = SurfacePoint + TableLongAxis * (HalfPlayLength * RackCenterLengthFactor) + TableUpAxis * (BallRadius + 0.35f);
}

FTransform APoolTableManager::MakeBallTransform(const FVector& WorldLocation) const
{
	return FTransform(FRotator::ZeroRotator, WorldLocation, FVector(1.0f));
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

	for (APoolCushionWall* Wall : CushionWalls)
	{
		if (IsValid(Wall))
		{
			Wall->Destroy();
		}
	}
	CushionWalls.Reset();

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
	CueBall = nullptr;
	PocketedBallCount = 0;
}

void APoolTableManager::SpawnBalls()
{
	const float BallDiameter = BallRadius * 2.0f;
	const float RackGapFactor = 1.01f;
	const float LateralSpacing = BallDiameter * RackGapFactor;
	const float RowSpacing = FMath::Sqrt(3.0f) * BallRadius * RackGapFactor;

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
		CueBall->ConfigureBall(true, 0, FLinearColor::White);
		SpawnedBalls.Add(CueBall);
		InitialBallTransforms.Add(CueBall->GetActorTransform());
	}

	int32 BallIndex = 1;
	const FVector Apex = RackCenterLocation + TableLongAxis * (RowSpacing * 2.0f);
	for (int32 Row = 0; Row < 5; ++Row)
	{
		for (int32 Col = 0; Col <= Row; ++Col)
		{
			const FVector Position = Apex
				- TableLongAxis * (Row * RowSpacing)
				+ TableShortAxis * ((Col - Row * 0.5f) * LateralSpacing)
				+ TableUpAxis * 0.0f;

			APoolBall* Ball = GetWorld()->SpawnActor<APoolBall>(APoolBall::StaticClass(), MakeBallTransform(Position), BallSpawnParams);
			if (Ball)
			{
				Ball->SetBallRadius(BallRadius);
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
	const float CornerLong = HalfPlayLength - PocketRadius * 0.95f;
	const float CornerShort = HalfPlayWidth - PocketRadius * 0.95f;
	const float MiddleShort = HalfPlayWidth - PocketRadius * 0.7f;
	const FVector TriggerDown = -TableUpAxis * (BallRadius * 0.45f);

	const TArray<FVector> PocketPositions = {
		SurfacePoint + TableLongAxis * CornerLong + TableShortAxis * CornerShort + TriggerDown,
		SurfacePoint + TableLongAxis * CornerLong - TableShortAxis * CornerShort + TriggerDown,
		SurfacePoint - TableLongAxis * CornerLong + TableShortAxis * CornerShort + TriggerDown,
		SurfacePoint - TableLongAxis * CornerLong - TableShortAxis * CornerShort + TriggerDown,
		SurfacePoint + TableShortAxis * MiddleShort + TriggerDown,
		SurfacePoint - TableShortAxis * MiddleShort + TriggerDown
	};

	for (const FVector& PocketPosition : PocketPositions)
	{
		if (APoolPocketTrigger* Pocket = GetWorld()->SpawnActor<APoolPocketTrigger>(APoolPocketTrigger::StaticClass(), PocketPosition, FRotator::ZeroRotator))
		{
			Pocket->SetManager(this);
			Pocket->SetPocketRadius(PocketRadius);
			PocketTriggers.Add(Pocket);
		}
	}
}

void APoolTableManager::SpawnWallSegment(const FVector& Center, const FVector& Extent, const FRotator& Rotation)
{
	if (APoolCushionWall* Wall = GetWorld()->SpawnActor<APoolCushionWall>(APoolCushionWall::StaticClass(), Center, Rotation))
	{
		Wall->ConfigureWall(Extent);
		CushionWalls.Add(Wall);
	}
}

void APoolTableManager::SpawnWalls()
{
	const float Thickness = BallRadius * 1.1f;
	const float Height = BallRadius * 4.0f;
	const float FloorHalfHeight = BallRadius * 0.6f;
	const float CornerGap = PocketRadius * 1.35f;
	const float MiddleGap = PocketRadius * 1.55f;
	const float SurfaceWallZ = SurfacePoint.Z + Height * 0.45f;
	const FRotator LongRotation = TableLongAxis.Rotation();
	const FRotator ShortRotation = TableShortAxis.Rotation();

	PlaySurfaceFloor = GetWorld()->SpawnActor<APoolCushionWall>(APoolCushionWall::StaticClass(), SurfacePoint - TableUpAxis * FloorHalfHeight, FRotator::ZeroRotator);
	if (PlaySurfaceFloor)
	{
		PlaySurfaceFloor->ConfigureWall(FVector(HalfPlayLength, HalfPlayWidth, FloorHalfHeight));
	}

	const float LongSegmentHalf = FMath::Max(8.0f, (HalfPlayLength - CornerGap - MiddleGap) * 0.5f);
	const float ShortSegmentHalf = FMath::Max(8.0f, HalfPlayWidth - CornerGap);

	SpawnWallSegment(SurfacePoint + TableShortAxis * HalfPlayWidth + TableLongAxis * (MiddleGap + LongSegmentHalf), FVector(LongSegmentHalf, Thickness, Height), LongRotation);
	SpawnWallSegment(SurfacePoint + TableShortAxis * HalfPlayWidth - TableLongAxis * (MiddleGap + LongSegmentHalf), FVector(LongSegmentHalf, Thickness, Height), LongRotation);
	SpawnWallSegment(SurfacePoint - TableShortAxis * HalfPlayWidth + TableLongAxis * (MiddleGap + LongSegmentHalf), FVector(LongSegmentHalf, Thickness, Height), LongRotation);
	SpawnWallSegment(SurfacePoint - TableShortAxis * HalfPlayWidth - TableLongAxis * (MiddleGap + LongSegmentHalf), FVector(LongSegmentHalf, Thickness, Height), LongRotation);
	SpawnWallSegment(SurfacePoint + TableLongAxis * HalfPlayLength + FVector(0.0f, 0.0f, SurfaceWallZ - SurfacePoint.Z), FVector(Thickness, ShortSegmentHalf, Height), ShortRotation);
	SpawnWallSegment(SurfacePoint - TableLongAxis * HalfPlayLength + FVector(0.0f, 0.0f, SurfaceWallZ - SurfacePoint.Z), FVector(Thickness, ShortSegmentHalf, Height), ShortRotation);

	for (APoolCushionWall* Wall : CushionWalls)
	{
		if (IsValid(Wall))
		{
			FVector Loc = Wall->GetActorLocation();
			Loc.Z = SurfaceWallZ;
			Wall->SetActorLocation(Loc);
		}
	}
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
	SpawnWalls();
	SpawnPockets();
	SpawnBalls();
}

void APoolTableManager::HandleBallPocketed(APoolBall* Ball, const FVector& PocketLocation)
{
	if (!IsValid(Ball) || Ball->IsPocketed())
	{
		return;
	}

	if (Ball->IsCueBall())
	{
		Ball->ResetBall(InitialBallTransforms.IsValidIndex(0) ? InitialBallTransforms[0] : Ball->GetActorTransform());
		CueBall = Ball;
		return;
	}

	++PocketedBallCount;
	Ball->BeginPocketSink(PocketLocation - TableUpAxis * (BallRadius * 2.8f));
}
