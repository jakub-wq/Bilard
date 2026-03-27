#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PoolMatchTypes.h"
#include "PoolTableManager.generated.h"

class APoolBall;
class APoolPocketTrigger;
class APoolCushionWall;
class APoolOpponentMarker;
class AStaticMeshActor;
class UStaticMeshComponent;
class UStaticMesh;
class UPoolSaveGame;
struct FPoolBallSaveState;

UCLASS()
class BILLARD_API APoolTableManager : public AActor
{
	GENERATED_BODY()

public:
	APoolTableManager();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	APoolBall* GetCueBall() const { return CueBall; }
	bool IsCueBallInHand() const { return bCueBallInHand; }
	bool AreBallsStopped() const;
	void ResetRack();
	void HandleBallPocketed(APoolBall* Ball, const FVector& PocketLocation);
	bool UpdateCueBallInHandPreview(const FVector& DesiredLocation);
	bool ConfirmCueBallPlacement(const FVector& DesiredLocation);
	bool ApplySavedBallStates(const TArray<FPoolBallSaveState>& SavedStates, int32 SavedPocketedBallCount);
	int32 GetPocketedBallCount() const { return PocketedBallCount; }
	const TArray<APoolBall*>& GetSpawnedBalls() const { return SpawnedBalls; }
	void SetMatchMode(EPoolMatchMode NewMode);
	EPoolMatchMode GetMatchMode() const { return MatchMode; }
	bool IsMatchFinished() const { return bMatchFinished; }
	void NotifyShotTaken(const FTransform& PlayerTransform, const FRotator& ControlRotation);
	FString GetHUDTurnText() const;
	FString GetHUDOpponentText() const;
	FString GetHUDWinnerText() const;
	FString GetHUDBlueScoreText() const;
	FString GetHUDRedScoreText() const;
	FLinearColor GetHUDTurnColor() const;
	FLinearColor GetHUDOpponentColor() const;
	bool ShouldShowLocalScoreboard() const { return MatchMode == EPoolMatchMode::LocalVersus; }
	void WriteMatchStateToSaveGame(UPoolSaveGame& SaveGame) const;
	void LoadMatchStateFromSaveGame(const UPoolSaveGame& SaveGame);

	FVector GetCueBallStartLocation() const { return CueBallStartLocation; }
	FVector GetCueBallInHandLocation() const { return CueBallInHandLocation; }
	FVector GetRackCenterLocation() const { return RackCenterLocation; }
	FVector GetTableLongAxis() const { return TableLongAxis; }
	FVector GetTableShortAxis() const { return TableShortAxis; }
	FVector GetTableUpAxis() const { return TableUpAxis; }
	FVector GetTableSurfaceCenter() const { return SurfacePoint + TableUpAxis * BallRadius; }
	float GetBallRadius() const { return BallRadius; }
	float GetPocketRadius() const { return PocketRadius; }
	float GetSurfaceZ() const { return SurfacePoint.Z; }
	bool IsReady() const { return TableMeshComponent != nullptr; }

	#if WITH_DEV_AUTOMATION_TESTS
		const TArray<APoolPocketTrigger*>& GetPocketTriggersForTests() const { return PocketTriggers; }
		const TArray<APoolCushionWall*>& GetCushionWallsForTests() const { return CushionWalls; }
		APoolCushionWall* GetPlaySurfaceFloorForTests() const { return PlaySurfaceFloor; }
		const TArray<AStaticMeshActor*>& GetSupportActorsForTests() const { return SupportActors; }
		AStaticMeshActor* GetTableActorForTests() const { return TableActor; }
		AStaticMeshActor* GetSpawnedVisualTableForTests() const { return SpawnedVisualTable; }
		float GetHalfOuterLengthForTests() const { return HalfOuterLength; }
		float GetHalfOuterWidthForTests() const { return HalfOuterWidth; }
		float GetHalfPlayLengthForTests() const { return HalfPlayLength; }
		float GetHalfPlayWidthForTests() const { return HalfPlayWidth; }
		float GetCollisionInsetLongForTests() const { return CollisionInsetLong; }
	float GetCollisionInsetShortForTests() const { return CollisionInsetShort; }
	FVector GetSurfacePointForTests() const { return SurfacePoint; }
	void CleanupSpawnedActorsForTests() { DestroySpawnedActors(); }
#endif

protected:
	void SpawnOrReuseFixedTable();
	void BuildTableData();
	void SpawnBalls();
	void SpawnPockets();
	void SpawnWalls();
	void SpawnTableSupports();
	void DestroySpawnedActors();
	void HandleEscapedBalls();
	void QueueCueBallRespawn(APoolBall* Ball, const FVector& PocketLocation);
	void UpdateCueBallRespawn(float DeltaTime);
	void BeginCueBallInHand(APoolBall* Ball);
	bool AreBallsSettledForCueBallInHand() const;
	bool IsCueBallScratchAlreadyHandled(const APoolBall* Ball) const;
	bool RegisterObjectBallPocketed(APoolBall* Ball);
	void StartBallPocketSink(APoolBall* Ball, const FVector& PocketLocation);
	void ResolveLocalMatchTurn();
	void ApplyCurrentPlayerView();
	void ResetLocalMatchState();
	void RecordLocalPocketedBall(APoolBall* Ball);
	void UpdateOpponentMarkers();
	EPoolBallGroup GetBallGroupForBall(const APoolBall* Ball) const;
	FString GetPlayerLabel(EPoolPlayerSide Player) const;
	FString GetPlayerCameraLabel(EPoolPlayerSide Player) const;
	EPoolPlayerSide GetOpponent(EPoolPlayerSide Player) const;
	bool IsAnyBallAnimating() const;
	int32 GetPocketedCountForPlayer(EPoolPlayerSide Player) const;
	void SetPlayerPocketedCount(EPoolPlayerSide Player, int32 NewCount);
	EPoolBallGroup GetAssignedGroup(EPoolPlayerSide Player) const;
	void SetAssignedGroup(EPoolPlayerSide Player, EPoolBallGroup Group);
	void FinishLocalMatch(EPoolPlayerSide WinningPlayer, const FString& Reason);
	bool IsCueBallPlacementLocationValid(const FVector& DesiredLocation, const APoolBall* IgnoredBall = nullptr) const;
	FVector FindCueBallPlacementLocation(const FVector& PreferredLocation) const;
	bool TryResolveBallEscape(APoolBall* Ball, const FVector& RelativeToSurface, float AlongLong, float AlongShort);
	bool FindNearestPocketLocation(const FVector& WorldLocation, FVector& OutPocketLocation, float& OutPlanarDistance) const;
	FTransform MakeBallTransform(const FVector& WorldLocation) const;
	FVector MakeTablePoint(float AlongLong, float AlongShort, float AlongUp = 0.0f) const;
	void SpawnWallSegment(const FVector& Center, const FVector& Extent, const FVector& Direction);
	void DrawDebugRuntimeColliders() const;

	UPROPERTY(EditAnywhere, Category = "Billiards|Setup")
	FVector FixedTableLocation = FVector(0.0f, 0.0f, 67.5f);

	UPROPERTY(EditAnywhere, Category = "Billiards|Setup")
	FRotator FixedTableRotation = FRotator(0.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, Category = "Billiards|Setup")
	FVector TableVisualScale = FVector(1.35f);

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float SurfaceHeightOffset = 17.2f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float SurfaceInsetBelowRails = 2.6f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float ManualHalfOuterLength = 143.0f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float ManualHalfOuterWidth = 79.0f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float ManualHalfPlayLength = 134.5f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float ManualHalfPlayWidth = 69.9f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float CollisionInsetLong = -2.0f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float CollisionInsetShort = -2.0f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float ManualBallRadius = 2.85f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float PocketRadiusMultiplier = 2.45f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float CornerPocketGapMultiplier = 2.75f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float SidePocketGapMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float CornerPocketTriggerMultiplier = 3.15f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float SidePocketTriggerMultiplier = 1.45f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float CornerPocketCaptureMultiplier = 3.35f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float CornerRailTrim = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float CornerPocketForceCaptureMultiplier = 4.15f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float PocketInset = 1.2f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float WallThicknessMultiplier = 0.95f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float CornerJawLengthMultiplier = 1.45f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float SideJawLengthMultiplier = 1.2f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float PocketSinkDepthMultiplier = 5.2f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float CueBallRespawnDelay = 0.65f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float CueBallInHandSettleTimeout = 1.1f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float CueBallLengthFactor = 0.33f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Tuning")
	float RackCenterLengthFactor = 0.18f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Debug")
	bool bDebugDrawRuntimeColliders = false;

	UPROPERTY(EditAnywhere, Category = "Billiards|Debug")
	bool bDebugDrawEscapeResolution = false;

	UPROPERTY(EditAnywhere, Category = "Billiards|Debug", meta = (ClampMin = "0.1"))
	float DebugColliderLineThickness = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Billiards|Debug", meta = (ClampMin = "6", ClampMax = "64"))
	int32 DebugSphereSegments = 24;

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
	TArray<FVector> PocketLocations;

	UPROPERTY()
	TArray<APoolCushionWall*> CushionWalls;

	UPROPERTY()
	TArray<AStaticMeshActor*> SupportActors;

	UPROPERTY()
	APoolCushionWall* PlaySurfaceFloor = nullptr;

	UPROPERTY()
	APoolBall* CueBall = nullptr;

	UPROPERTY(Transient)
	TWeakObjectPtr<APoolBall> PendingCueBallRespawn;

	UPROPERTY(Transient)
	FTransform PendingCueBallRespawnTransform = FTransform::Identity;

	UPROPERTY(Transient)
	TSet<TObjectPtr<APoolBall>> CountedPocketedBalls;

	UPROPERTY(Transient)
	float CueBallRespawnTimer = 0.0f;

	UPROPERTY(Transient)
	float CueBallInHandSettleTimer = 0.0f;

	UPROPERTY(Transient)
	bool bCueBallRespawnPending = false;

	UPROPERTY(Transient)
	bool bCueBallInHand = false;

	UPROPERTY(Transient)
	FVector CueBallInHandLocation = FVector::ZeroVector;

	TArray<FTransform> InitialBallTransforms;

	int32 PocketedBallCount = 0;
	EPoolMatchMode MatchMode = EPoolMatchMode::Training;
	EPoolPlayerSide ActivePlayer = EPoolPlayerSide::Blue;
	EPoolPlayerSide WinningPlayer = EPoolPlayerSide::Blue;
	EPoolBallGroup BlueAssignedGroup = EPoolBallGroup::Unassigned;
	EPoolBallGroup RedAssignedGroup = EPoolBallGroup::Unassigned;
	int32 BluePocketedCount = 0;
	int32 RedPocketedCount = 0;
	bool bTurnResolutionPending = false;
	bool bScratchCommittedThisTurn = false;
	bool bBlackPocketedThisTurn = false;
	bool bMatchFinished = false;
	TArray<TWeakObjectPtr<APoolBall>> PocketedThisTurn;
	FTransform BlueSavedTransform = FTransform::Identity;
	FRotator BlueSavedControlRotation = FRotator::ZeroRotator;
	bool bHasBlueSavedView = false;
	FTransform RedSavedTransform = FTransform::Identity;
	FRotator RedSavedControlRotation = FRotator::ZeroRotator;
	bool bHasRedSavedView = false;
	APoolOpponentMarker* BlueOpponentMarker = nullptr;
	APoolOpponentMarker* RedOpponentMarker = nullptr;
};
