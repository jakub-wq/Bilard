#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PoolTableManager.generated.h"

class APoolBall;
class APoolPocketTrigger;
class APoolCushionWall;
class AStaticMeshActor;
class UStaticMeshComponent;
class UStaticMesh;
struct FPoolBallSaveState;

UCLASS()
class BILLARD_API APoolTableManager : public AActor
{
	GENERATED_BODY()

public:
	APoolTableManager();
	virtual void BeginPlay() override;

	APoolBall* GetCueBall() const { return CueBall; }
	bool AreBallsStopped() const;
	void ResetRack();
	void HandleBallPocketed(APoolBall* Ball, const FVector& PocketLocation);
	void ApplySavedBallStates(const TArray<FPoolBallSaveState>& SavedStates, int32 SavedPocketedBallCount);
	int32 GetPocketedBallCount() const { return PocketedBallCount; }
	const TArray<APoolBall*>& GetSpawnedBalls() const { return SpawnedBalls; }

	FVector GetCueBallStartLocation() const { return CueBallStartLocation; }
	FVector GetRackCenterLocation() const { return RackCenterLocation; }
	FVector GetTableLongAxis() const { return TableLongAxis; }
	FVector GetTableShortAxis() const { return TableShortAxis; }
	FVector GetTableUpAxis() const { return TableUpAxis; }
	float GetBallRadius() const { return BallRadius; }
	float GetPocketRadius() const { return PocketRadius; }
	float GetSurfaceZ() const { return SurfacePoint.Z; }
	bool IsReady() const { return TableMeshComponent != nullptr; }

protected:
	void SpawnOrReuseFixedTable();
	void BuildTableData();
	void SpawnBalls();
	void SpawnPockets();
	void SpawnWalls();
	void DestroySpawnedActors();
	FTransform MakeBallTransform(const FVector& WorldLocation) const;
	FVector MakeTablePoint(float AlongLong, float AlongShort, float AlongUp = 0.0f) const;
	void SpawnWallSegment(const FVector& Center, const FVector& Extent, const FVector& Direction);

	UPROPERTY(EditAnywhere, Category = "Billiards|Setup")
	FVector FixedTableLocation = FVector(0.0f, 0.0f, 45.0f);

	UPROPERTY(EditAnywhere, Category = "Billiards|Setup")
	FRotator FixedTableRotation = FRotator(0.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float PlayAreaLengthScale = 0.90f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float PlayAreaWidthScale = 0.82f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float BallRadiusScaleOfWidth = 0.032f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float PocketRadiusMultiplier = 2.35f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float PocketInset = 1.2f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float WallThicknessMultiplier = 0.95f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float SurfaceHeightScale = 0.96f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float CornerJawLengthMultiplier = 1.45f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float SideJawLengthMultiplier = 1.2f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float PocketSinkDepthMultiplier = 5.2f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float CueBallLengthFactor = 0.33f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float RackCenterLengthFactor = 0.18f;

	UPROPERTY(VisibleInstanceOnly, Category = "Billiards")
	AStaticMeshActor* TableActor = nullptr;

	UPROPERTY()
	AStaticMeshActor* SpawnedVisualTable = nullptr;

	UPROPERTY()
	UStaticMesh* TableVisualMesh = nullptr;

	UPROPERTY(Transient)
	UStaticMeshComponent* TableMeshComponent = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Billiards")
	FVector TableCenter = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, Category = "Billiards")
	FVector TableLongAxis = FVector::ForwardVector;

	UPROPERTY(VisibleInstanceOnly, Category = "Billiards")
	FVector TableShortAxis = FVector::RightVector;

	UPROPERTY(VisibleInstanceOnly, Category = "Billiards")
	FVector TableUpAxis = FVector::UpVector;

	UPROPERTY(VisibleInstanceOnly, Category = "Billiards")
	FVector SurfacePoint = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, Category = "Billiards")
	float HalfOuterLength = 100.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Billiards")
	float HalfOuterWidth = 50.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Billiards")
	float HalfPlayLength = 70.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Billiards")
	float HalfPlayWidth = 35.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Billiards")
	float BallRadius = 5.7f;

	UPROPERTY(VisibleInstanceOnly, Category = "Billiards")
	float PocketRadius = 11.5f;

	UPROPERTY(VisibleInstanceOnly, Category = "Billiards")
	FVector CueBallStartLocation = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, Category = "Billiards")
	FVector RackCenterLocation = FVector::ZeroVector;

	UPROPERTY()
	TArray<APoolBall*> SpawnedBalls;

	UPROPERTY()
	TArray<APoolPocketTrigger*> PocketTriggers;

	UPROPERTY()
	TArray<APoolCushionWall*> CushionWalls;

	UPROPERTY()
	APoolCushionWall* PlaySurfaceFloor = nullptr;

	UPROPERTY()
	APoolBall* CueBall = nullptr;

	TArray<FTransform> InitialBallTransforms;

	int32 PocketedBallCount = 0;
};
