#pragma once

#include "CueSkin.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MyCharacter.generated.h"

class UCameraComponent;
class UInputComponent;
class UStaticMeshComponent;
class APoolBall;
class APoolTableManager;
class UPoolHUDWidget;

UCLASS()
class BILLARD_API AMyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMyCharacter();
	virtual void Tick(float DeltaTime) override;
	void PrepareForMenu();
	void SetInGameHUDVisible(bool bVisible);
	void SetCueSkin(ECueSkin NewSkin, bool bSavePreference = true);
	ECueSkin GetSelectedCueSkin() const { return SelectedCueSkin; }
	void ApplyExternalView(const FTransform& PlayerTransform, const FRotator& ControlRotation);

	static FTransform CalculateCueVisualTransform(
		const FVector& BallLocation,
		const FVector& ShotDirection,
		const FVector& UpAxis,
		float CueDistanceFromBall,
		float CuePullbackDistance,
		float CurrentShotPower,
		float MaxShotPower,
		float CueSideOffset,
		float CueHeightOffset,
		const FRotator& CueAimRotationOffset,
		bool bIsChargingShot);

#if WITH_DEV_AUTOMATION_TESTS
	UStaticMeshComponent* GetCueMeshForTests() const { return CueMesh; }
	APoolBall* GetActiveCueBallForTests() const { return ActiveCueBall; }
	bool IsCueBallPlacementModeForTests() const { return bIsCueBallPlacementMode; }
	void EnterAimModeForTests(APoolBall* InCueBall) { EnterAimMode(InCueBall); }
	void ExitAimModeForTests() { ExitAimMode(); }
#endif

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void TurnAtRate(float Value);
	void LookUpAtRate(float Value);

	void PrimaryActionPressed();
	void PrimaryActionReleased();
	void CancelShot();
	void ResetBalls();
	void ToggleCrouch();

	void StartChargingShot();
	void ReleaseShot();
	void EnterAimMode(APoolBall* InCueBall);
	void ExitAimMode();
	void EnterCueBallPlacementMode();
	void ExitCueBallPlacementMode();
	void UpdateCueVisual();
	void UpdateAimMode(float DeltaTime);
	void UpdateCueBallPlacementMode(float DeltaTime);
	void SyncCueBallPlacementMode();
	void AttachCueToPlayer();
	void ShowMessage(const FString& Message) const;
	bool TryEnterAimMode();
	APoolBall* FindInteractCueBall() const;
	APoolTableManager* GetPoolManager() const;
	void UpdateHUD();
	void EnsureHUDWidget();
	void PositionAimCamera();
	void PositionCueBallPlacementCamera();
	void UpdateNormalFacingToTable();
	FVector GetAimDirection() const;
	void ApplyCueSkin(ECueSkin NewSkin);
	void LoadCueSkinPreference();
	void SaveCueSkinPreference() const;

	UFUNCTION()
	void HandleResetWidgetClicked();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* AimCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* CueMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billiards|Shot")
	float ChargeRate = 520.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billiards|Shot")
	float MaxShotPower = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billiards|Shot")
	float InteractDistance = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billiards|Shot")
	float CueDistanceFromBall = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billiards|Shot")
	float CuePullbackDistance = 22.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billiards|Shot")
	float CueSideOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billiards|Shot")
	float CueHeightOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billiards|Shot")
	float AimOrbitDistance = 78.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billiards|Shot")
	float AimOrbitHeight = 22.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billiards|Shot")
	float AimPitchDegrees = -8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billiards|Placement")
	float PlacementCameraHeight = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billiards|Placement")
	float PlacementMoveSpeed = 110.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billiards|Shot")
	FRotator CueAimRotationOffset = FRotator(90.0f, 0.0f, -2.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billiards|Shot")
	float DesiredCueLength = 128.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Billiards|Shot")
	float CurrentShotPower = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Billiards|Shot")
	bool bIsChargingShot = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Billiards|Shot")
	bool bIsAimMode = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Billiards|Shot")
	ECueSkin SelectedCueSkin = ECueSkin::Standard;

	UPROPERTY()
	APoolBall* ActiveCueBall = nullptr;

	UPROPERTY()
	UPoolHUDWidget* HUDWidget = nullptr;

	mutable TWeakObjectPtr<APoolTableManager> CachedPoolManager;
	float AimYawDegrees = 0.0f;
	FVector CueBallPlacementLocation = FVector::ZeroVector;
	float PlacementForwardInput = 0.0f;
	float PlacementRightInput = 0.0f;
	bool bIsCueBallPlacementMode = false;
	bool bShowHUDWidget = true;
};
