#include "PoolTableManager.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Math/RotationMatrix.h"
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
		HalfPlayLength = 98.0f;
		HalfPlayWidth = 48.0f;
		BallRadius = 2.86f;
		PocketRadius = 6.75f;
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

FVector APoolTableManager::MakeTablePoint(float AlongLong, float AlongShort, float AlongUp) const
{
	return SurfacePoint + TableLongAxis * AlongLong + TableShortAxis * AlongShort + TableUpAxis * AlongUp;
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
	const float PocketCenterInset = FMath::Max(PocketRadius * 0.48f, BallRadius * PocketInset);
	const float CornerLong = FMath::Max(0.0f, HalfPlayLength - PocketCenterInset);
	const float CornerShort = FMath::Max(0.0f, HalfPlayWidth - PocketCenterInset);
	const float MiddleShort = FMath::Max(0.0f, HalfPlayWidth - FMath::Max(PocketRadius * 0.35f, BallRadius * PocketInset * 0.85f));
	const FVector TriggerDown = -TableUpAxis * (BallRadius * 0.65f);

	const TArray<FVector> PocketPositions = {
		MakeTablePoint(CornerLong, CornerShort) + TriggerDown,
		MakeTablePoint(CornerLong, -CornerShort) + TriggerDown,
		MakeTablePoint(-CornerLong, CornerShort) + TriggerDown,
		MakeTablePoint(-CornerLong, -CornerShort) + TriggerDown,
		MakeTablePoint(0.0f, MiddleShort) + TriggerDown,
		MakeTablePoint(0.0f, -MiddleShort) + TriggerDown
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
	const float Thickness = FMath::Max(2.0f, BallRadius * WallThicknessMultiplier);
	const float Height = BallRadius * 3.8f;
	const float FloorHalfHeight = BallRadius * 0.7f;
	const float WallCenterUp = Height * 0.55f;
	const float RailInset = FMath::Max(Thickness * 0.85f, BallRadius * 0.95f);
	const float RailLong = FMath::Max(BallRadius * 6.0f, HalfPlayLength - RailInset);
	const float RailShort = FMath::Max(BallRadius * 4.0f, HalfPlayWidth - RailInset);
	const float CornerGap = PocketRadius * 1.05f;
	const float MiddleGap = PocketRadius * 1.15f;
	const float CornerJawLength = PocketRadius * CornerJawLengthMultiplier;
	const float SideJawLength = PocketRadius * SideJawLengthMultiplier;
	const float LongStraightHalf = FMath::Max(BallRadius * 2.2f, (RailLong - CornerGap - MiddleGap) * 0.5f);
	const float ShortStraightHalf = FMath::Max(BallRadius * 2.0f, RailShort - CornerGap);
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
		SpawnWallSegment(MakeTablePoint(LongSign * RailLong, 0.0f, WallCenterUp), FVector(Thickness, ShortStraightHalf, Height), TableShortAxis);

		for (const float ShortSign : { -1.0f, 1.0f })
		{
			const FVector JawDirection = (-LongSign * TableLongAxis - ShortSign * TableShortAxis).GetSafeNormal();
			SpawnWallSegment(
				MakeTablePoint(LongSign * (RailLong - CornerGap * 0.55f), ShortSign * (RailShort - CornerGap * 0.55f), WallCenterUp),
				FVector(CornerJawLength, Thickness, Height),
				JawDirection);
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
	Ball->BeginPocketSink(PocketLocation - TableUpAxis * (BallRadius * PocketSinkDepthMultiplier));
}

void APoolTableManager::ApplySavedBallStates(const TArray<FPoolBallSaveState>& SavedStates, int32 SavedPocketedBallCount)
{
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
		}
		else if (SavedState.bCueBall)
		{
			CueBall = Ball;
		}
	}

	PocketedBallCount = FMath::Clamp(SavedPocketedBallCount, 0, 15);
}
