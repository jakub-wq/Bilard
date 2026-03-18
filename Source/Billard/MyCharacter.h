#pragma once

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

	void StartChargingShot();
	void ReleaseShot();
	void EnterAimMode(APoolBall* InCueBall);
	void ExitAimMode();
	void UpdateCueVisual();
	void UpdateAimMode(float DeltaTime);
	void AttachCueToPlayer();
	void ShowMessage(const FString& Message) const;
	bool TryEnterAimMode();
	APoolBall* FindInteractCueBall() const;
	APoolTableManager* GetPoolManager() const;
	void UpdateHUD();
	void EnsureHUDWidget();
	void DrawTrajectoryPreview() const;
	void PositionAimCamera();
	void UpdateNormalFacingToTable();
	FVector GetAimDirection() const;

	UFUNCTION()
	void HandleResetWidgetClicked();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* AimCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* CueMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billiards|Shot")
	float ChargeRate = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billiards|Shot")
	float MaxShotPower = 2600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billiards|Shot")
	float InteractDistance = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billiards|Shot")
	float CueDistanceFromBall = 34.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billiards|Shot")
	float CuePullbackDistance = 22.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billiards|Shot")
	float AimOrbitDistance = 78.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billiards|Shot")
	float AimOrbitHeight = 22.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billiards|Shot")
	float AimPitchDegrees = -8.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Billiards|Shot")
	float CurrentShotPower = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Billiards|Shot")
	bool bIsChargingShot = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Billiards|Shot")
	bool bIsAimMode = false;

	UPROPERTY()
	APoolBall* ActiveCueBall = nullptr;

	UPROPERTY()
	UPoolHUDWidget* HUDWidget = nullptr;

	mutable TWeakObjectPtr<APoolTableManager> CachedPoolManager;
	float AimYawDegrees = 0.0f;
	bool bShowHUDWidget = true;
};
